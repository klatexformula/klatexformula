/***************************************************************************
 *   file klfmain.cpp
 *   This file is part of the KLatexFormula Project.
 *   Copyright (C) 2011 by Philippe Faist
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
#include <QDateTime>

#include <klfutil.h>
#include "klfpluginiface.h"
#include "klfconfig.h"
#include "klfmain.h"


KLF_EXPORT QList<KLFTranslationInfo> klf_avail_translations;

KLF_EXPORT QList<QTranslator*> klf_translators;





QList<KLFPluginInfo> klf_plugins;



QList<KLFAddOnInfo> klf_addons;
bool klf_addons_canimport = false;

/** \internal */
struct KLFAddOnInfo::Private {
  int ref; // number of times this data structure is referenced
  
  QString dir;
  QString fname;
  QString fpath;
  bool islocal;
  
  QString title;
  QString author;
  QString description;
  QString klfminversion;
  
  QString rccmountroot;
    
  QStringList pluginList;
  QMap<QString,PluginSysInfo> plugins;
  
  QStringList translations;

  QStringList userScripts;
  
  bool isfresh;
  
  QStringList errors;
};


KLFAddOnInfo::KLFAddOnInfo(QString rccfpath, bool isFresh)
{
  d = new Private;
  d->ref = 1;
  klfDbg( "KLFAddOnInfo: rccfpath="<<rccfpath<<", Private has ref "<< d->ref ) ;

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
  klfDbg( "Mounting resource "<<rccfpath<<" to "<<d->rccmountroot ) ;

  bool ok = QResource::registerResource(d->fpath, mountroot);
  KLF_ASSERT_CONDITION(ok, "Failed to register resource "<<rccfpath, return; ) ;

  // read the RCC's info.xml content
  { // code block brackets {} are to close the file immediately after reading.
    QFile infofile(mountroot+QLatin1String("/rccinfo/info.xml"));
    infofile.open(QIODevice::ReadOnly);
    rccinfodata = infofile.readAll();
  }

  // read translation list
  QDir i18ndir(mountroot+QLatin1String("/i18n/"));
  d->translations = i18ndir.entryList(QStringList() << "*.qm", QDir::Files);

  QDir userscriptdir(mountroot+QLatin1String("/userscripts/"));
  d->userScripts = userscriptdir.entryList(QStringList() << "*", QDir::Files);

  // set default values for title, author, description, and klfminversion in case the XML file
  // does not provide any
  d->title = QObject::tr("(Name Not Provided)", "[KLFAddOnInfo: add-on information XML data is invalid]");
  d->description = QObject::tr("(Invalid XML Data Provided By Add-On)",
			       "[KLFAddOnInfo: add-on information XML data is invalid]");
  d->klfminversion = QString();
  d->author = QObject::tr("(No Author Provided)",
			  "[KLFAddOnInfo: add-on information XML data is invalid]");
  
  // parse resource's rccinfo/info.xml file
  QDomDocument xmldoc;
  xmldoc.setContent(rccinfodata);
  // and explore the document
  QDomElement xmlroot = xmldoc.documentElement();
  if (xmlroot.nodeName() != "rccinfo") {
    qWarning("Add-on file `%s' has invalid XML information.", qPrintable(rccfpath));
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
    if ( e.nodeName() == "author" ) {
      d->author = e.text().trimmed();
    }
    if ( e.nodeName() == "description" ) {
      d->description = e.text().trimmed();
    }
    if ( e.nodeName() == "klfminversion" ) {
      d->klfminversion = e.text().trimmed();
    }
  }

  initPlugins();
}

KLFAddOnInfo::KLFAddOnInfo(const KLFAddOnInfo& other)
{
  d = other.d;
  if (d)
    d->ref++;
}

KLFAddOnInfo::~KLFAddOnInfo()
{
  if (d) {
    d->ref--;
    if (d->ref <= 0) {
      // finished reading this resource, un-register it.
      QResource::unregisterResource(d->fpath, d->rccmountroot);
      delete d;
    }
  }
}



QString KLFAddOnInfo::dir() const { return d->dir; }
QString KLFAddOnInfo::fname() const { return d->fname; }
QString KLFAddOnInfo::fpath() const { return d->fpath; }
bool KLFAddOnInfo::islocal() const { return d->islocal; }
QString KLFAddOnInfo::title() const { return d->title; }
QString KLFAddOnInfo::author() const { return d->author; }
QString KLFAddOnInfo::description() const { return d->description; }
QString KLFAddOnInfo::klfminversion() const { return d->klfminversion; }
QString KLFAddOnInfo::rccmountroot() const { return d->rccmountroot; }

QStringList KLFAddOnInfo::translations() const { return d->translations; }
bool KLFAddOnInfo::isfresh() const { return d->isfresh; }
QStringList KLFAddOnInfo::errors() const { return d->errors; }
QStringList KLFAddOnInfo::pluginList() const { return d->pluginList; }
KLFAddOnInfo::PluginSysInfo KLFAddOnInfo::pluginSysInfo(const QString& plugin) const
{ return d->plugins[plugin]; }
QString KLFAddOnInfo::pluginLocalSubDirName(const QString& plugin) const
{
  if ( ! d->plugins[plugin].klfminversion.isEmpty() )
    return QString("%1/klf%2").arg(QLatin1String("sysarch_")+KLFSysInfo::makeSysArch(d->plugins[plugin].os,
										     d->plugins[plugin].arch),
				   d->plugins[plugin].klfminversion);
  return QString(".");
}
QStringList KLFAddOnInfo::userScripts() const { return d->userScripts; }



KLF_EXPORT QDebug& operator<<(QDebug& str, const KLFAddOnInfo::PluginSysInfo& i)
{
  return str << "KLFAddOnInfo::PluginSysInfo(qtminver="<<i.qtminversion<<"; klfminver="<<i.klfminversion
	     << "; os="<<i.os<<"; arch="<<i.arch<<")";
}


bool KLFAddOnInfo::PluginSysInfo::isCompatibleWithCurrentSystem() const
{
  return
    (klfminversion.isEmpty()
     || klfVersionCompare(klfminversion, KLF_VERSION_STRING) <= 0) &&
    (qtminversion.isEmpty()
     || klfVersionCompare(qtminversion, qVersion()) <= 0) &&
    os == KLFSysInfo::osString() &&
    arch.split(',').contains(KLFSysInfo::arch()) ;
}

void KLFAddOnInfo::addError(const QString& s)
{
  if (d->errors.indexOf(s) == -1)
    d->errors << s;
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
  QStringList unorderedplugins = plugdir.entryList(KLF_DLL_EXT_LIST, QDir::Files);
  int k;
  for (k = 0; k < unorderedplugins.size(); ++k) {
    d->pluginList << unorderedplugins[k];
    d->plugins[unorderedplugins[k]] = defpinfo;
  }

  if (!QFile::exists(plugdir.absoluteFilePath("plugindirinfo.xml"))) {
    klfDbg( "KLFAddOnInfo("<<d->fname<<"): No specific plugin directories. plugdirinfo.xml="
	    <<plugdir.absoluteFilePath("plugindirinfo.xml") ) ;
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
      klfDbg( "Node: type="<<nn.nodeType()<<"; name="<<ee.nodeName() ) ;
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
    klfDbg( "\tRead psi="<<psi ) ;
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
    QStringList plugins = plugsubdir.entryList(KLF_DLL_EXT_LIST, QDir::Files);
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

  klfDbg( "Loaded plugins: list="<<d->pluginList<<"; map="<<d->plugins ) ;
}


QStringList KLFAddOnInfo::localPluginList() const
{
  QStringList lplugins;
  for (int k = 0; k < d->pluginList.size(); ++k) {
    if ( d->plugins[d->pluginList[k]].isCompatibleWithCurrentSystem() )
      lplugins << QDir::cleanPath(pluginLocalSubDirName(d->pluginList[k])+"/"+QFileInfo(d->pluginList[k]).fileName());
  }
  return lplugins;
}






// -------




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
  KLF_DEBUG_BLOCK(KLF_FUNC_NAME) ;
  klfDbg("i18nfile.fpath="<<i18nfile.fpath<<" is translation to "<<i18nfile.locale) ;

  QFileInfo fi(i18nfile.fpath);

  klfDbg("fi.canonicalPath()="<<fi.canonicalPath()<<", Qt translations location="
	 <<QFileInfo(QLibraryInfo::location(QLibraryInfo::TranslationsPath)).canonicalFilePath()) ;

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
      klfDbg("translation "<<i18nfile.locale<<" is already registered. needs nice name?="<< needsNiceName) ;
    }
  }

  klfDbg("Needs registration?="<<needsRegistration<<"; needs nice name?="<<needsNiceName) ;
  if ( ! needsRegistration && ! needsNiceName ) {
    // needs nothing more !
    return;
  }

  klfDbg("will load translation file "<<fi.completeBaseName()<<", abs path="<<fi.absolutePath()) ;

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


KLF_EXPORT void klf_reload_translations(QCoreApplication *app, const QString& currentLocale)
{
  // refresh and load all translations. Translations are files found in the form
  //   :/i18n/<name>_<locale>.qm   or   homeconfig/i18n/<name>_<locale>.qm
  //
  // this function may be called at run-time to change language.

  int j, k;

  // clear any already set translators
  for (k = 0; k < klf_translators.size(); ++k) {
    app->removeTranslator(klf_translators[k]);
    delete klf_translators[k];
  }
  klf_translators.clear();

  // we will find all possible .qm files and store them in this structure for easy access
  // structure is indexed by name, then locale specificity
  QMap<QString, QMap<int, QList<KLFI18nFile> > > i18nFiles;
  // a list of names. this is redundant for  i18nFiles.keys()
  QSet<QString> names;

  QStringList i18ndirlist;
  // add any add-on specific translations
  for (k = 0; k < klf_addons.size(); ++k) {
    i18ndirlist << klf_addons[k].rccmountroot()+"/i18n";
  }
  i18ndirlist << ":/i18n"
	      << klfconfig.homeConfigDirI18n
	      << klfconfig.globalShareDir+"/i18n"
	      << QLibraryInfo::location(QLibraryInfo::TranslationsPath);

  for (j = 0; j < i18ndirlist.size(); ++j) {
    // explore this directory; we expect a list of *.qm files
    QDir i18ndir(i18ndirlist[j]);
    if ( ! i18ndir.exists() )
      continue;
    QStringList files = i18ndir.entryList(QStringList() << QString::fromLatin1("*.qm"), QDir::Files);
    for (k = 0; k < files.size(); ++k) {
      KLFI18nFile i18nfile(i18ndir.absoluteFilePath(files[k]));
      //      qDebug("Found i18n file %s (name=%s,locale=%s,lc-spcif.=%d)", qPrintable(i18nfile.fpath),
      //	     qPrintable(i18nfile.name), qPrintable(i18nfile.locale), i18nfile.locale_specificity);
      i18nFiles[i18nfile.name][i18nfile.locale_specificity] << i18nfile;
      names << i18nfile.name;
      klfDbg("Found translation "<<i18nfile.fpath);
      klf_add_avail_translation(i18nfile);
    }
  }

  // get locale
  QString lc = currentLocale;
  if (lc.isEmpty())
    lc = "en_US";
  QStringList lcparts = lc.split("_");

  //  qDebug("Required locale is %s", qPrintable(lc));

  // a list of translation files to load (absolute paths)
  QStringList translationsToLoad;

  // now, load a suitable translator for each encountered name.
  for (QSet<QString>::const_iterator it = names.begin(); it != names.end(); ++it) {
    QString name = *it;
    QMap< int, QList<KLFI18nFile> > translations = i18nFiles[name];
    int specificity = lcparts.size();  // start with maximum specificity for required locale
    while (specificity >= 0) {
      // try to load a translation matching this specificity and locale
      QString testlocale = QStringList(lcparts.mid(0, specificity)).join("_");
      //      qDebug("Testing locale string %s...", qPrintable(testlocale));
      // search list:
      QList<KLFI18nFile> list = translations[specificity];
      for (j = 0; j < list.size(); ++j) {
	if (list[j].locale == testlocale) {
	  //	  qDebug("Found translation file.");
	  // got matching translation file ! Load it !
	  translationsToLoad << list[j].fpath;
	  // and stop searching translation files for this name (break while condition);
	  specificity = -1;
	}
      }
      // If we didn't find a suitable translation, try less specific locale name
      specificity--;
    }
  }
  // now we have a full list of translation files to load stored in  translationsToLoad .

  // Load Translations:
  for (j = 0; j < translationsToLoad.size(); ++j) {
    // load this translator
    //    qDebug("Loading translator %s for %s", qPrintable(translationsToLoad[j]), qPrintable(lc));
    QTranslator *translator = new QTranslator(app);
    QFileInfo fi(translationsToLoad[j]);
    //    qDebug("translator->load(\"%s\", \"%s\", \"_\", \"%s\")", qPrintable(fi.completeBaseName()),
    //	   qPrintable(fi.absolutePath()),  qPrintable("."+fi.suffix()));
    bool res = translator->load(fi.completeBaseName(), fi.absolutePath(), "_", "."+fi.suffix());
    if ( res ) {
      app->installTranslator(translator);
      klf_translators << translator;
    } else {
      qWarning("Failed to load translator %s.", qPrintable(translationsToLoad[j]));
      delete translator;
    }
  }
}




KLF_EXPORT QString klfFindTranslatedDataFile(const QString& baseFileName, const QString& extension)
{
  QString loc = klfconfig.UI.locale;
  QStringList suffixes;
  suffixes << "_"+loc
	   << "_"+loc.section('_',0,0)
	   << "";
  int k = 0;
  QString fn;
  while ( k < suffixes.size() &&
	  ! QFile::exists(fn = QString("%1%2%3").arg(baseFileName, suffixes[k], extension)) ) {
    klfDbg( "base="<<baseFileName<<" extn="<<extension<<"; tried fn="<<fn ) ;
    ++k;
  }
  if (k >= suffixes.size()) {
    qWarning()<<KLF_FUNC_NAME<<": Can't find good translated file for "<<qPrintable(baseFileName+extension)
	      <<"! last try was "<<fn;
    return QString();
  }
  return fn;
}




KLF_EXPORT void klfDataStreamWriteHeader(QDataStream& stream, const QString headermagic)
{
  // QIODevice inherits QObject ... use dynamic properties
  stream.device()->setProperty("klfDataStreamAppVersion",
			       QVariant::fromValue<QString>(KLF_DATA_STREAM_APP_VERSION));

  // header always written in QDataStream version Qt_3_3
  stream.setVersion(QDataStream::Qt_3_3);
  stream << headermagic
	 << (qint16)KLF_DATA_STREAM_APP_VERSION_MAJ
	 << (qint16)KLF_DATA_STREAM_APP_VERSION_MIN
	 << (qint16)QDataStream::Qt_4_4;
  stream.setVersion(QDataStream::Qt_4_4);
  // stream is ready to be written to

}

KLF_EXPORT bool klfDataStreamReadHeader(QDataStream& stream, const QStringList possibleHeaders,
					QString *readHeader, QString *readCompatKLFVersion)
{
  KLF_DEBUG_BLOCK(KLF_FUNC_NAME) ;

  QString s;
  stream.setVersion(QDataStream::Qt_3_3);
  stream >> s;
  if (!possibleHeaders.contains(s) || stream.status() != QDataStream::Ok) {
    klfDbg("Read bad header: "<<s) ;
    if (readHeader != NULL)
      *readHeader = QString();
    return false;
  }
  if (readHeader != NULL)
    *readHeader = s;

  // read KLF-compat writing version
  qint16 vmaj, vmin;
  stream >> vmaj >> vmin;
  if (stream.status() != QDataStream::Ok) {
    if (readCompatKLFVersion)
      *readCompatKLFVersion = QString();
    return false;
  }
  klfDbg("read app compat version = "<<vmaj<<"."<<vmin) ;

  QString compatKLFVersion = QString("%1.%2").arg(vmaj).arg(vmin);

  if (vmaj > klfVersionMaj() || (vmaj == klfVersionMaj() && vmin > klfVersionMin())) {
    if (readCompatKLFVersion != NULL)
      *readCompatKLFVersion = compatKLFVersion; 
    return false;
  }

  // decide on QDataStream version
  if (vmaj <= 2) { // 2.x: version # not saved into stream, use Qt_3_3
    stream.setVersion(QDataStream::Qt_3_3);
  } else { // 3.x+: read version # from stream and set it
    qint16 version;
    stream >> version;
    stream.setVersion(version);
  }

  // set the compatibility version for reading data
  // QIODevice inherits QObject ... use dynamic properties
  stream.device()->setProperty("klfDataStreamAppVersion", QVariant::fromValue<QString>(compatKLFVersion));

  // the stream is ready to read data from
  return true;
}



// -----

// user scripts

KLF_EXPORT QStringList klf_user_scripts;


void klf_reload_user_scripts()
{
  KLF_DEBUG_BLOCK(KLF_FUNC_NAME) ;

  QStringList pathlist;
  pathlist << klfconfig.homeConfigDirUserScripts
	   << klfconfig.globalShareDir+"/userscripts";

  klfDbg("looking for user scripts in "<<pathlist) ;

  // First, look to see if there are any user scripts to install from add-on resources

  int i, j, k;
  for (k = 0; k < klf_addons.size(); ++k) {
    QStringList uscripts = klf_addons[k].userScripts();
    for (j = 0; j < uscripts.size(); ++j) {
      // maybe install this user script
      // so check at filesystem locations to see if it's there
      QString foundpath = QString();
      QString resfn = klf_addons[k].rccmountroot() + "/userscripts/" + uscripts[j];
      QString locfn = klfconfig.homeConfigDirUserScripts + "/" + uscripts[j];
      for (i = 0; i < pathlist.size(); ++i) {
	QString s = pathlist[i]+"/"+uscripts[j];
	klfDbg("testing "<<s) ;
	if (QFile::exists(s)) { // found
	  foundpath = s;
	  break;
	}
      }
      bool needsinstall = false;
      if (foundpath.isEmpty()) {
	needsinstall = true;
      } else {
	// compare modification times to see whether we need to reupdate userscript
	QDateTime installeduserscript_dt = QFileInfo(foundpath).lastModified();
	QDateTime resourceuserscript_dt = QFileInfo(klf_addons[k].fpath()).lastModified();
	klfDbg("Comparing resource datetime ("<<qPrintable(resourceuserscript_dt.toString())
	       <<") with installed userscript datetime ("<<qPrintable(installeduserscript_dt.toString())<<")") ;
	needsinstall = (installeduserscript_dt.isNull() || resourceuserscript_dt.isNull() ||
			( resourceuserscript_dt > installeduserscript_dt ));
      }
      if (!needsinstall)
	continue;
      // now install that user script
      if (QFile::exists(locfn)) QFile::remove(locfn);
      // copy userscript to local userscript dir
      klfDbg( "\tcopy "<<resfn<<" to "<<locfn ) ;
      bool res = QFile::copy( resfn , locfn );
      if ( ! res ) {
	klf_addons[k].addError(QObject::tr("Failed to install userscript '%1' locally.",
					   "[[userscript error message]]").arg(uscripts[j]));
	qWarning("Unable to copy plugin '%s' to local directory!", qPrintable(uscripts[j]));
	continue;
      } else {
	QFile::setPermissions(locfn, QFile::ReadOwner|QFile::WriteOwner|QFile::ExeOwner|
			      QFile::ReadUser|QFile::WriteUser|QFile::ExeUser|
			      QFile::ReadGroup|QFile::ExeGroup|QFile::ReadOther|QFile::ExeOther);
	klfDbg("Copied userscript "<<resfn<<" to local directory "<<locfn<<".") ;
      }

      if (!foundpath.isEmpty() && !needsinstall)
	locfn = foundpath;
    } // loop over add-on's userscripts
  } // loop over add-ons

  // now find all user scripts

  KLFUserScriptInfo::clearCacheAll();

  klf_user_scripts.clear();
  for (int kkl = 0; kkl < pathlist.size(); ++kkl) {
    QStringList l = klfSearchFind(pathlist[kkl]+"/*");
    // filter out some unwanted entries
    for (int j = 0; j < l.size(); ++j) {
      if (l[j].endsWith("/.") || l[j].endsWith("/.."))
	continue;
      if (!QFileInfo(l[j]).isFile()) {
	continue;
      }
      if (l[j].endsWith("~") || l[j].endsWith(".bkp"))
	continue; // skip any "old"/"backup" files
      if (!QFileInfo(l[j]).isExecutable()) {
	klfWarning("File "<<l[j]<<" in userscripts/ is ignored as it is not executable.");
	continue;
      }
      klfDbg("User script: "<<l[j]) ;
      klf_user_scripts << l[j];
    }
  }
  klfDbg("Searched in path="<<pathlist<<"; scripts="<<klf_user_scripts) ;
}

