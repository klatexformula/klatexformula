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

#include <QString>
#include <QList>
#include <QObject>
#include <QDomDocument>
#include <QFile>
#include <QFileInfo>
#include <QResource>
#include <QDir>
#include <QTranslator>

#include <klfpluginiface.h>

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


KLFAddOnInfo::KLFAddOnInfo(QString rccfpath)
{
  QFileInfo fi(rccfpath);
  
  fname = fi.fileName();
  dir = fi.absolutePath();
  fpath = fi.absoluteFilePath();

  islocal = fi.isWritable() || QFileInfo(dir).isWritable();

  isfresh = false;

  QByteArray rccinfodata;
  QStringList pluginlist;
  { // try registering resource to see its content
    const char * temproot = ":/__klfaddoninfo_temp_resource";
    QResource::registerResource(fpath, temproot);
    QFile infofile(temproot+QString::fromLatin1("/rccinfo/info.xml"));
    infofile.open(QIODevice::ReadOnly);
    rccinfodata = infofile.readAll();
    // read plugin list
    QDir plugdir(temproot+QString::fromLatin1("/plugins/"));
    this->plugins = plugdir.entryList(QStringList() << KLF_DLL_EXT, QDir::Files);
    // read translation list
    QDir i18ndir(temproot+QString::fromLatin1("/i18n/"));
    this->translations = i18ndir.entryList(QStringList() << "*.qm", QDir::Files);
    // finished reading this resource, un-register it.
    QResource::unregisterResource(fpath, temproot);
  }
  // parse resource's rccinfo/info.xml file
  QDomDocument xmldoc;
  xmldoc.setContent(rccinfodata);

  // and explore the document
  QDomElement xmlroot = xmldoc.documentElement();
  if (xmlroot.nodeName() != "rccinfo") {
    qWarning("Add-on file `%s' has invalid XML information.", qPrintable(rccfpath));
    this->title = QObject::tr("(Name Not Provided)", "[KLFAddOnInfo: add-on information XML data is invalid]");
    this->description = QObject::tr("(Invalid XML Data Provided By Add-On)",
				    "[KLFAddOnInfo: add-on information XML data is invalid]");
    this->author = QObject::tr("(No Author Provided)",
			       "[KLFAddOnInfo: add-on information XML data is invalid]");
    return;
  }
  QDomNode n;
  for (n = xmlroot.firstChild(); ! n.isNull(); n = n.nextSibling()) {
    QDomElement e = n.toElement();
    if ( e.isNull() || n.nodeType() != QDomNode::ElementNode )
      continue;
    if ( e.nodeName() == "title" ) {
      this->title = e.text().trimmed();
    }
    if ( e.nodeName() == "description" ) {
      this->description = e.text().trimmed();
    }
    if ( e.nodeName() == "author" ) {
      this->author = e.text().trimmed();
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
  qDebug("Want to add avail translation: %s [%s]", qPrintable(i18nfile.fpath), qPrintable(i18nfile.locale));
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
  QFileInfo fi(i18nfile.fpath);
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


