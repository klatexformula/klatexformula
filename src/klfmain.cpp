/***************************************************************************
 *   file klfmain.cpp
 *   This file is part of the KLatexFormula Project.
 *   Copyright (C) 2009 by Philippe Faist
 *   philippe.faist at bluewin.ch
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.             *
 ***************************************************************************/
/* $Id$ */

#include "klfmain.h"

#include <QDebug>
#include <QString>
#include <QList>
#include <QObject>
#include <QDomDocument>
#include <QFile>
#include <QFileInfo>
#include <QResource>
#include <QDir>
#include <QTranslator>
#include <QLibraryInfo>

#include "klfpluginiface.h"
#include "klfutil.h"

// a list of locale names available for KLatexFormula
QList<KLFTranslationInfo> klf_avail_translations;


// not static so we can get this value from other modules in the project
char version[] = KLF_VERSION_STRING;
int version_maj = -1;
int version_min = -1;
int version_release = -1;



QList<KLFPluginInfo> klf_plugins;



QList<KLFAddOnInfo> klf_addons;
bool klf_addons_canimport = false;



KLFAddOnInfo::KLFAddOnInfo(QString rccfpath, bool isFresh)
{
  d = new Private;
  d->ref = 1;
  qDebug()<<"KLFAddOnInfo: Private has ref "<< d->ref;

  QFileInfo fi(rccfpath);
  
  d->fname = fi.fileName();
  d->dir = fi.absolutePath();
  d->fpath = fi.absoluteFilePath();

  d->islocal = fi.isWritable() || QFileInfo(d->dir).isWritable();

  d->isfresh = isFresh;

  QByteArray rccinfodata;

  // mount the resource locally
  QString mountroot;
  QString suffix;
  QString minrccfpath = rccfpath.section("/", -1, -1, QString::SectionSkipEmpty);
  int k = 0;
  // find a unique mountroot name
  while (QFileInfo(mountroot = QString(":/klfaddon_rccmount/%1%2").arg(minrccfpath, suffix)).exists()) {
    suffix = QString("_%1").arg(++k);
  }
  d->rccmountroot = mountroot;
  qDebug()<<"Mounting resource to "<<d->rccmountroot;

  bool ok = QResource::registerResource(d->fpath, mountroot);
  if (!ok) {
    qWarning("Failed to register resource `%s'.", qPrintable(rccfpath));
    return;
  }

  // read the RCC's info.xml content
  { // code block brackets {} are to close the file immediately after reading.
    QFile infofile(mountroot+QLatin1String("/rccinfo/info.xml"));
    infofile.open(QIODevice::ReadOnly);
    rccinfodata = infofile.readAll();
  }

  // read translation list
  QDir i18ndir(mountroot+QLatin1String("/i18n/"));
  d->translations = i18ndir.entryList(QStringList() << "*.qm", QDir::Files);
  
  // parse resource's rccinfo/info.xml file
  QDomDocument xmldoc;
  xmldoc.setContent(rccinfodata);
  // and explore the document
  QDomElement xmlroot = xmldoc.documentElement();
  if (xmlroot.nodeName() != "rccinfo") {
    qWarning("Add-on file `%s' has invalid XML information.", qPrintable(rccfpath));
    d->title = QObject::tr("(Name Not Provided)", "[KLFAddOnInfo: add-on information XML data is invalid]");
    d->description = QObject::tr("(Invalid XML Data Provided By Add-On)",
				 "[KLFAddOnInfo: add-on information XML data is invalid]");
    d->author = QObject::tr("(No Author Provided)",
			    "[KLFAddOnInfo: add-on information XML data is invalid]");
    return;
  }
  QDomNode n;
  for (n = xmlroot.firstChild(); ! n.isNull(); n = n.nextSibling()) {
    QDomElement e = n.toElement();
    if ( e.isNull() || n.nodeType() != QDomNode::ElementNode )
      continue;
    if ( e.nodeName() == "title" ) {
      d->title = e.text().trimmed();
    }
    if ( e.nodeName() == "description" ) {
      d->description = e.text().trimmed();
    }
    if ( e.nodeName() == "author" ) {
      d->author = e.text().trimmed();
    }
  }

  initPlugins();
}

KLF_EXPORT QDebug& operator<<(QDebug& str, const KLFAddOnInfo::PluginSysInfo& i)
{
  return str << "KLFAddOnInfo::PluginSysInfo(qtminver="<<i.qtminversion<<"; klfminver="<<i.klfminversion
	     << "; os="<<i.os<<"; arch="<<i.arch<<")";
}

void KLFAddOnInfo::initPlugins()
{
  // read plugin list
  QDir plugdir(d->rccmountroot+QLatin1String("/plugins/"));
  PluginSysInfo defpinfo;
  defpinfo.qtminversion = ""; // by default -> no version check (empty string)
  defpinfo.klfminversion = ""; // by default -> no version check (empty string)
  defpinfo.os = KLFSysInfo::osString(); // by default, current OS.
  defpinfo.arch = KLFSysInfo::arch(); // by default, current arch.
  d->plugins = QMap<QString,PluginSysInfo>();
  d->pluginList = QStringList();

  // first add all plugins that are in :/plugins
  QStringList unorderedplugins = plugdir.entryList(QStringList() << KLF_DLL_EXT, QDir::Files);
  int k;
  for (k = 0; k < unorderedplugins.size(); ++k) {
    d->pluginList << unorderedplugins[k];
    d->plugins[unorderedplugins[k]] = defpinfo;
  }

  if (!QFile::exists(plugdir.absoluteFilePath("plugindirinfo.xml"))) {
    qDebug()<<"KLFAddOnInfo("<<d->fname<<"): No specific plugin directories. plugdirinfo.xml="
	    <<plugdir.absoluteFilePath("plugindirinfo.xml");
    return;
  }

  // no plugin dirs.
  QFile plugdirinfofile(d->rccmountroot+QLatin1String("/plugins/plugindirinfo.xml"));
  plugdirinfofile.open(QIODevice::ReadOnly);
  QByteArray plugdirinfodata = plugdirinfofile.readAll();

  // here, key is the plugin _dir_ itself.
  QMap<QString,PluginSysInfo> pdirinfos;

  // parse resource's plugins/plugindirinfo.xml file
  QDomDocument xmldoc;
  xmldoc.setContent(plugdirinfodata);
  // and explore the document
  QDomElement xmlroot = xmldoc.documentElement();
  if (xmlroot.nodeName() != "klfplugindirs") {
    qWarning("KLFAddOnInfo: Add-on plugin dir info file `%s' has invalid XML information.",
	     qPrintable(d->fpath));
    return;
  }
  QDomNode n;
  for (n = xmlroot.firstChild(); ! n.isNull(); n = n.nextSibling()) {
    QDomElement e = n.toElement();
    if ( e.isNull() || n.nodeType() != QDomNode::ElementNode )
      continue;
    if ( e.nodeName() != "klfplugindir" ) {
      qWarning("KLFAddOnInfo(%s): plugindirinfo.xml: skipping unexpected node %s.", qPrintable(d->fpath),
	       qPrintable(e.nodeName()));
      continue;
    }
    // read a plugin dir info
    PluginSysInfo psi;
    QDomNode nn;
    for (nn = e.firstChild(); ! nn.isNull(); nn = nn.nextSibling()) {
      QDomElement ee = nn.toElement();
      qDebug()<<"Node: type="<<nn.nodeType()<<"; name="<<ee.nodeName();
      if ( ee.isNull() || nn.nodeType() != QDomNode::ElementNode )
	continue;
      if ( ee.nodeName() == "dir" ) {
	psi.dir = ee.text().trimmed();
      } else if ( ee.nodeName() == "qtminversion" ) {
	psi.qtminversion = ee.text().trimmed();
      } else if ( ee.nodeName() == "klfminversion" ) {
	psi.klfminversion = ee.text().trimmed();
      } else if ( ee.nodeName() == "os" ) {
	psi.os = ee.text().trimmed();
      } else if ( ee.nodeName() == "arch" ) {
	psi.arch = ee.text().trimmed();
      } else {
	qWarning("KLFAddOnInfo(%s): plugindirinfo.xml: skipping unexpected node in <klfplugindirs>: %s.",
		 qPrintable(d->fpath), qPrintable(ee.nodeName()));
      }
    }
    qDebug()<<"\tRead psi="<<psi;
    // add this plugin dir info
    pdirinfos[psi.dir] = psi;
  }

  QStringList morePluginsList;
  for (QMap<QString,PluginSysInfo>::const_iterator it = pdirinfos.begin(); it != pdirinfos.end(); ++it) {
    QString dir = it.key();
    PluginSysInfo psi = it.value();
    if ( ! QFileInfo(d->rccmountroot + "/plugins/" + dir).exists() ) {
      qWarning("KLFAddOnInfo(%s): Plugin dir '%s' given in XML info does not exist in resource!",
	       qPrintable(d->fpath), qPrintable(dir));
      continue;
    }
    QDir plugsubdir(d->rccmountroot + "/plugins/" + dir);
    QStringList plugins = plugsubdir.entryList(QStringList() << KLF_DLL_EXT, QDir::Files);
    int j;
    for (j = 0; j < plugins.size(); ++j) {
      QString p = dir+"/"+plugins[j];
      morePluginsList << p;
      d->plugins[p] = psi;
    }
  }
  // prepend morePluginsList to d->pluginList
  QStringList fullList = morePluginsList;
  fullList << d->pluginList;
  d->pluginList = fullList;

  qDebug()<<"Loaded plugins: list="<<d->pluginList<<"; map="<<d->plugins;
}


KLFAddOnInfo::KLFAddOnInfo(const KLFAddOnInfo& other)
{
  d = other.d;
  if (d)
    d->ref++;
  qDebug()<<"KLFAddOnInfo: Copy constructor Private d has ref "<< d->ref;
}

KLFAddOnInfo::~KLFAddOnInfo()
{
  qDebug()<<"KLFAddOnInfo: destructor Private d has ref "<< d->ref;

  if (d) {
    d->ref--;
    if (d->ref <= 0) {
      // finished reading this resource, un-register it.
      QResource::unregisterResource(d->fpath, d->rccmountroot);
      delete d;
    }
  }
}




KLFI18nFile::KLFI18nFile(QString filepath)
{
  QFileInfo fi(filepath);
  QString fn = fi.fileName();
  QDir d = fi.absoluteDir();

  int firstunderscore = fn.indexOf('_');
  int endbasename = fn.endsWith(".qm") ? fn.length() - 3 : fn.length() ;
  if (firstunderscore == -1)
    firstunderscore = endbasename; // no locale part if no underscore
  // ---
  fpath = d.absoluteFilePath(fn);
  name = fn.mid(0, firstunderscore);
  locale = fn.mid(firstunderscore+1, endbasename-(firstunderscore+1));
  locale_specificity = (locale.split('_', QString::SkipEmptyParts)).size() ;
}




void klf_add_avail_translation(KLFI18nFile i18nfile)
{
  QFileInfo fi(i18nfile.fpath);

  if ( fi.canonicalPath() ==
       QFileInfo(QLibraryInfo::location(QLibraryInfo::TranslationsPath)).canonicalFilePath()
       || i18nfile.name == "qt" ) {
    // ignore Qt's translations as available languages (identified as being in Qt's
    // translation path or as a locally named qt_XX.qm
    return;
  }

  // Check if this locale is registered, and if it has a nice translated name.
  bool needsRegistration = true;
  bool needsNiceName = true;
  int alreadyRegisteredIndex = -1;

  int kk;
  for (kk = 0; kk < klf_avail_translations.size(); ++kk) {
    if (klf_avail_translations[kk].localename == i18nfile.locale) {
      needsRegistration = false;
      alreadyRegisteredIndex = kk;
      needsNiceName = ! klf_avail_translations[kk].hasnicetranslatedname;
    }
  }
  if ( ! needsRegistration && ! needsNiceName ) {
    // needs nothing more !
    return;
  }

  // needs something (registration and/or nice name)
  QTranslator translator;
  translator.load(fi.completeBaseName(), fi.absolutePath(), "_", "."+fi.suffix());
  KLFTranslationInfo ti;
  ti.localename = i18nfile.locale;
  struct klf_qtTrNoop3 { const char *source; const char *comment; };
  klf_qtTrNoop3 lang
    = QT_TRANSLATE_NOOP3("QObject", "English (US)",
			 "[[The Language (possibly with Country) you are translating to, e.g. `Deutsch']]");
  ti.translatedname = translator.translate("QObject", lang.source, lang.comment);
  ti.hasnicetranslatedname = true;
  if (ti.translatedname == "English" || ti.translatedname.isEmpty()) {
    QLocale lc(i18nfile.locale);
    QString s;
    if ( i18nfile.locale.indexOf("_") != -1 ) {
      // has country information in locale
      s = QString("%1 (%2)").arg(QLocale::languageToString(lc.language()))
	.arg(QLocale::countryToString(lc.country()));
    } else {
      s = QString("%1").arg(QLocale::languageToString(lc.language()));
    }
    ti.translatedname = s;
    ti.hasnicetranslatedname = false;
  }
  if (needsRegistration)
    klf_avail_translations.append(ti);
  else if (needsNiceName && ti.hasnicetranslatedname)
    klf_avail_translations[alreadyRegisteredIndex] = ti;
}



