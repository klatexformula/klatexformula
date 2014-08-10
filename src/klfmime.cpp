/***************************************************************************
 *   file klfmime.cpp
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
#include <QApplication>
#include <QMap>
#include <QUrl>
#include <QBuffer>
#include <QDir>
#include <QDomDocument>
#include <QDomNode>
#include <QDomElement>
#include <QPainter>
#include <QTextCodec>
#include <QTextDocument>

#include <klfutil.h>
#include <klfguiutil.h>
#include <klfbackend.h>
#include <klffilterprocess.h>

#include "klfconfig.h"
#include "klflib.h"
#include "klfmime.h"
#include "klfmime_p.h"

#ifdef Q_WS_MAC
#include "macosx/klfmacclipboard.h"
#endif

#define OPENOFFICE_DRAWING_MIMETYPE "application/x-openoffice-drawing;windows_formatname=\"Drawing Format\""


// ---------------------------------------------------------------------

struct KLFMimeString
{
  KLFMimeString(const QString& key)
  {
    mime_full = key;
    // see if we got any attributes to the mime-type
    int i_semicolon = key.indexOf(';');
    if (i_semicolon >= 0) {
      mime_base = key.left(i_semicolon);
      
      QRegExp rx("(\\w+)=([^=;]*|\"(?:[^\"]|(?:\\\\.))*\")");
      int pos = i_semicolon+1;
      while ( (pos = rx.indexIn(key, pos)) >= 0 )  {
        mime_attrs[rx.cap(1)] = rx.cap(2);
      }
    } else {
      mime_base = key;
    }
  }

  QString mime_full;
  QString mime_base;

  QMap<QString,QString> mime_attrs;
};


// ---------------------------------------------------------------------



inline static bool matches_mimetype(const QString& key, const QString& test)
{
  // image/png  matches only image/png
  // text/plain  and  text/plain;charset=UTF-8   both match  text/plain
  return test.startsWith(key) && ( test.size() == key.size() || test[key.size()] == QLatin1Char(';') );
}

bool KLFMimeExporter::supportsKey(const QString& key, const KLFBackend::klfOutput * output) const
{
  QStringList mykeys = keys(output);
  int k;
  for (k = 0; k < mykeys.size(); ++k) {
    if (matches_mimetype(mykeys[k], key))
      break;
  }
  bool result = (k < mykeys.size()) ;
  klfDbg("key = "<<key<<" ; result="<<result) ;
  return result;
}

// static
QByteArray KLFMimeExporter::exportData(const QString& key, const KLFBackend::klfOutput& klfoutput,
				       const QString& exportername)
{
  KLF_DEBUG_BLOCK(KLF_FUNC_NAME) ;
  klfDbg("key="<<key<<"; exporername="<<exportername) ;

  if (!exportername.isEmpty()) {
    klfDbg("looking up exporter name "<<exportername) ;
    KLFMimeExporter *exporter = mimeExporterLookupByName(exportername, key);
    KLF_ASSERT_NOT_NULL(exporter, "Can't find exporter "<<exportername<<" responsible for "<<key<<"!",
			return QByteArray(); ) ;
    return exporter->data(key, klfoutput) ;
  }
  // find all exporters that can provide export type \c key
  QList<KLFMimeExporter*> exporters = mimeExporterFullLookup(key);
  foreach (KLFMimeExporter *e, exporters) {
    klfDbg("Try to export "<<key<<" with exporter "<<e->exporterName()<<"...") ;
    QByteArray data = e->data(key, klfoutput);
    if (!data.isEmpty()) {
      klfDbg("... success") ;
      // successfully exported data
      return data;
    }
  }
  // failed to export data
  klfDbg("Failed to export data as "<<key<<".") ;
  return QByteArray();
}

// static
KLFMimeExporter * KLFMimeExporter::mimeExporterLookup(const QString& key, const KLFBackend::klfOutput * output)
{
  KLF_DEBUG_BLOCK(KLF_FUNC_NAME) ;
  klfDbg("key="<<key) ;

  initMimeExporterList();

  int k;
  for (k = 0; k < p_mimeExporterList.size(); ++k) {
    klfDbg("Testing exporter #"<<k<<": "<<p_mimeExporterList[k]);
    klfDbg("\t: "<<p_mimeExporterList[k]->exporterName()) ;
    if (p_mimeExporterList[k]->supportsKey(key, output))
      return p_mimeExporterList[k];
  }

  // no exporter found.
  return NULL;
}
// static
QList<KLFMimeExporter*> KLFMimeExporter::mimeExporterFullLookup(const QString& key,
								const KLFBackend::klfOutput * output)
{
  KLF_DEBUG_BLOCK(KLF_FUNC_NAME) ;
  initMimeExporterList();

  QList<KLFMimeExporter*> exporters;

  int k;
  for (k = 0; k < p_mimeExporterList.size(); ++k) {
    klfDbg("Testing exporter #"<<k<<": "<<p_mimeExporterList[k]);
    klfDbg("\t: "<<p_mimeExporterList[k]->exporterName()) ;
    if (p_mimeExporterList[k]->supportsKey(key, output))
      exporters << p_mimeExporterList[k];
  }

  // if no exporter was found, an empty list is returned
  return exporters;
}
// static
KLFMimeExporter * KLFMimeExporter::mimeExporterLookupByName(const QString& exporter, const QString& key,
							    const KLFBackend::klfOutput * output)
{
  KLF_DEBUG_BLOCK(KLF_FUNC_NAME) ;
  klfDbg("exporter="<<exporter<<"; key="<<key) ;

  initMimeExporterList();

  int k;
  for (k = 0; k < p_mimeExporterList.size(); ++k)
    if (p_mimeExporterList[k]->exporterName() == exporter) // find the given 'exporter'
      if (key.isEmpty() || p_mimeExporterList[k]->supportsKey(key, output)) // check for 'key' support
	return p_mimeExporterList[k];

  // no exporter found.
  return NULL;
}

  
// static
QList<KLFMimeExporter *> KLFMimeExporter::mimeExporterList()
{
  KLF_DEBUG_BLOCK(KLF_FUNC_NAME) ;
  initMimeExporterList();
  return p_mimeExporterList;
}

// static
void KLFMimeExporter::registerMimeExporter(KLFMimeExporter *exporter, bool overrides)
{
  KLF_DEBUG_BLOCK(KLF_FUNC_NAME) ;

  initMimeExporterList();

  KLF_ASSERT_NOT_NULL( exporter ,
		       "Cannot register a NULL exporter!",
		       return )  ;

  QString ename = exporter->exporterName();
  klfDbg("want to register exporter "<<ename<<", making sure no duplicate names...") ;

  // make sure there are no duplicate names
  int k;
  for (k = 0; k < p_mimeExporterList.size(); ++k) {
    klfDbg("making sure p_mimeExporterList["<<k<<"]->exporterName() [="<<p_mimeExporterList[k]->exporterName()<<"]"
	   <<"  !=  ename [="<<ename<<"]") ;
    KLF_ASSERT_CONDITION(p_mimeExporterList[k]->exporterName() != ename,
			 "An exporter with same name "<<ename<<" is already registered!",
			 return ) ;
  }

  klfDbg("registering exporter "<<ename<<", overrides="<<overrides) ;

  if (overrides)
    p_mimeExporterList.push_front(exporter);
  else
    p_mimeExporterList.push_back(exporter);
}
// static
void KLFMimeExporter::unregisterMimeExporter(KLFMimeExporter *exporter)
{
  KLF_DEBUG_BLOCK(KLF_FUNC_NAME) ;
  p_mimeExporterList.removeAll(exporter);
}


// static
void KLFMimeExporter::reloadMimeExporterList()
{
  KLF_DEBUG_BLOCK(KLF_FUNC_NAME) ;
  p_mimeExporterList.clear();
  initMimeExporterList();
}

// static
void KLFMimeExporter::initMimeExporterList()
{
  KLF_DEBUG_BLOCK(KLF_FUNC_NAME) ;
  klfDbg("initMimeExporterList");

  if (p_mimeExporterList.isEmpty()) {
    // ensure an instance of KLFMacPasteboardMime object
#ifdef Q_WS_MAC
    __klf_init_the_macpasteboardmime();
#endif
    p_mimeExporterList
      << new KLFMimeExporterImage(qApp)
      << new KLFMimeExporterUrilist(qApp)
      << new KLFMimeExporterUrilistPDF(qApp)
      << new KLFMimeExporterHTML(qApp)
      << new KLFMimeExporterLibFmts(qApp)
      << new KLFMimeExporterGlowImage(qApp)
      ;
    // now, create one instance of KLFMimeExporterUserScript per export type user script ...
    extern QStringList klf_user_scripts;
    for (int k = 0; k < klf_user_scripts.size(); ++k) {
      KLFExportTypeUserScriptInfo usinfo = KLFUserScriptInfo(klf_user_scripts[k]);
      klfDbg("k="<<k<<" loading user script "<<usinfo.scriptName());
      if (usinfo.category() == QLatin1String("klf-export-type")) {

	// add the mime exporter
	KLFMimeExporterUserScript * exporter = new KLFMimeExporterUserScript(klf_user_scripts[k], qApp);
	p_mimeExporterList << exporter;

	// and add corresponding export profile for this script
	QList<KLFMimeExportProfile::ExportType> elist;
	QStringList mimes = usinfo.mimeTypes();
	int j;
	for (j = 0; j < mimes.size(); ++j) {
	  elist << KLFMimeExportProfile::ExportType(mimes[j], QString(), exporter->exporterName());
	}
	KLFMimeExportProfile profile("userscript-" + usinfo.scriptName(),
				     usinfo.outputFilenameExtensions().join(",").toUpper()+" ("
				     +usinfo.scriptName()+")",
				     elist, QObject::tr("User scripts", "[[KLFMimeExporter; submenu title]]"));
	KLFMimeExportProfile::addExportProfile(profile);

      }
    }

  }
}

// static
QList<KLFMimeExporter*> KLFMimeExporter::p_mimeExporterList = QList<KLFMimeExporter*>();

// ---------------------------------------------------------------------

KLFMimeExportProfile::KLFMimeExportProfile(const QString& pname, const QString& desc,
					   const QList<ExportType>& exporttypes, const QString& inSubMenu)
  : p_profileName(pname), p_description(desc), p_exportTypes(exporttypes), p_inSubMenu(inSubMenu)
{
}

KLFMimeExportProfile::KLFMimeExportProfile(const KLFMimeExportProfile& o)
  : p_profileName(o.p_profileName), p_description(o.p_description),
    p_exportTypes(o.p_exportTypes), p_inSubMenu(o.p_inSubMenu)
{
}

QByteArray KLFMimeExportProfile::exportData(int n, const KLFBackend::klfOutput& output) const
{
  KLF_DEBUG_BLOCK(KLF_FUNC_NAME) ;

  return KLFMimeExporter::exportData(p_exportTypes[n].mimetype, output, p_exportTypes[n].exporter);
}

KLFMimeExporter * KLFMimeExportProfile::exporterLookupFor(int k, const KLFBackend::klfOutput * output,
							  bool warnNotFound) const
{
  KLF_DEBUG_BLOCK(KLF_FUNC_NAME) ;
  klfDbg("k="<<k<<" mimetype[k]="<<p_exportTypes[k].mimetype) ;

  KLF_ASSERT_CONDITION(k >= 0 && k < p_exportTypes.size(),
		       "Index "<<k<<" out of bounds (size="<<p_exportTypes.size()<<")",
		       return NULL ) ;

  KLFMimeExporter * exporter = NULL;
  if ( ! p_exportTypes[k].exporter.isEmpty() ) {
    klfDbg("looking up the exporter by name "<<p_exportTypes[k].exporter) ;
    // lookup the exporter by name, and make sure that it supports the 'mimetype' key
    exporter = KLFMimeExporter::mimeExporterLookupByName(p_exportTypes[k].exporter, p_exportTypes[k].mimetype,
							 output);
  } else {
    // lookup the exporter by mime-type
    exporter = KLFMimeExporter::mimeExporterLookup(p_exportTypes[k].mimetype, output);
  }

  if (warnNotFound)
    KLF_ASSERT_NOT_NULL(exporter,
			"Can't find exporter "<<p_exportTypes[k].exporter<<" for export-type #"<<k
			<<" for key "<<p_exportTypes[k].mimetype,   return NULL ) ;
  return exporter;
}
QList<KLFMimeExporter*> KLFMimeExportProfile::exporterFullLookupFor(int k, const KLFBackend::klfOutput * output,
								    bool warnNotFound) const
{
  KLF_DEBUG_BLOCK(KLF_FUNC_NAME) ;

  KLF_ASSERT_CONDITION(k >= 0 && k < p_exportTypes.size(),
		       "Index "<<k<<" out of bounds (size="<<p_exportTypes.size()<<")",
		       return QList<KLFMimeExporter*>() ) ;

  QList<KLFMimeExporter*> exporters;
  if ( ! p_exportTypes[k].exporter.isEmpty() ) {
    // lookup the exporter by name, and make sure that it supports the 'mimetype' key
    KLFMimeExporter *e = KLFMimeExporter::mimeExporterLookupByName(p_exportTypes[k].exporter,
                                                                   p_exportTypes[k].mimetype,
                                                                   output);
    KLF_ASSERT_NOT_NULL(e, "Can't find exporter "<<p_exportTypes[k].exporter<<" for export-type #"<<k
                        <<" and mime-type "<<p_exportTypes[k].mimetype,
                        return QList<KLFMimeExporter*>() );
    exporters << e;
  } else {
    // lookup the exporter by mime-type
    exporters = KLFMimeExporter::mimeExporterFullLookup(p_exportTypes[k].mimetype, output);
  }

  if (warnNotFound)
    KLF_ASSERT_CONDITION(!exporters.isEmpty(),
			 "Can't find exporter "<<p_exportTypes[k].exporter<<" for export-type #"<<k
			 <<"for mime-type "<<p_exportTypes[k].mimetype,
                         return QList<KLFMimeExporter*>()
                         ) ;

  klfDbg("Found "<<exporters.size()<<" exporters for mime type "<<p_exportTypes[k].mimetype
         <<" with exporter named "<<p_exportTypes[k].exporter);
#ifdef KLF_DEBUG
  QStringList dbg_list;
  for (int k = 0; k < exporters.size(); ++k)
    dbg_list.append(exporters[k]->exporterName());
  klfDbg("\texporter list is "<<dbg_list.join(","));
#endif

  return exporters;
}

QStringList KLFMimeExportProfile::mimeTypes() const
{
  KLF_DEBUG_BLOCK(KLF_FUNC_NAME) ;

  QStringList mimetypes;
  int k;
  for (k = 0; k < p_exportTypes.size(); ++k)
    mimetypes << p_exportTypes[k].mimetype;
  return mimetypes;
}

int KLFMimeExportProfile::indexOfMimeType(const QString& mimeType) const
{
  KLF_DEBUG_BLOCK(KLF_FUNC_NAME) ;

  int k;
  for (k = 0; k < p_exportTypes.size(); ++k)
    if (p_exportTypes[k].mimetype == mimeType)
      return k;
  return -1;
}

QStringList KLFMimeExportProfile::respectiveWinTypes() const
{
  KLF_DEBUG_BLOCK(KLF_FUNC_NAME) ;

  QStringList wintypes;
  int k;
  for (k = 0; k < p_exportTypes.size(); ++k)
    wintypes << respectiveWinType(k);
  return wintypes;
}
QString KLFMimeExportProfile::respectiveWinType(int k) const
{
  KLF_DEBUG_BLOCK(KLF_FUNC_NAME) ; klfDbg("k="<<k) ;

  KLF_ASSERT_CONDITION(k >= 0 && k < p_exportTypes.size(),
		       "Index "<<k<<" out of bounds (size="<<p_exportTypes.size()<<")",
		       return QString() ) ;

  if ( ! p_exportTypes[k].wintype.isEmpty() )
    return p_exportTypes[k].wintype;

  QList<KLFMimeExporter*> exporters = exporterFullLookupFor(k, NULL, true);
  foreach (KLFMimeExporter *e, exporters) {
    QString s = e->windowsFormatName(p_exportTypes[k].mimetype);
    if (!s.isEmpty())
      return s;
  }
  return QString();
}

QStringList KLFMimeExportProfile::availableExporterMimeTypes(const KLFBackend::klfOutput * output) const
{
  KLF_DEBUG_BLOCK(KLF_FUNC_NAME) ;

  QStringList oktypes;
  int k;
  for (k = 0; k < p_exportTypes.size(); ++k) {
    if (exporterLookupFor(k, output, false) != NULL) {
      klfDbg("found ok type: "<<p_exportTypes[k].mimetype) ;
      oktypes << p_exportTypes[k].mimetype;
    }
  }
  return oktypes;
}


// static
QList<KLFMimeExportProfile> KLFMimeExportProfile::p_exportProfileList = QList<KLFMimeExportProfile>();

// static
QList<KLFMimeExportProfile> KLFMimeExportProfile::exportProfileList()
{
  KLF_DEBUG_BLOCK(KLF_FUNC_NAME) ;

  ensureLoadedExportProfileList();
  return p_exportProfileList;
}
// static
void KLFMimeExportProfile::addExportProfile(const KLFMimeExportProfile& exportProfile)
{
  KLF_DEBUG_BLOCK(KLF_FUNC_NAME) ;

  ensureLoadedExportProfileList();
  p_exportProfileList.push_front(exportProfile);
}
// static
KLFMimeExportProfile KLFMimeExportProfile::findExportProfile(const QString& pname)
{
  KLF_DEBUG_BLOCK(KLF_FUNC_NAME) ;

  ensureLoadedExportProfileList();

  int k;
  for (k = 0; k < p_exportProfileList.size(); ++k)
    if (p_exportProfileList[k].profileName() == pname)
      return p_exportProfileList[k];

  // not found, return first (ie. default) export profile
  return p_exportProfileList[0];
}

void KLFMimeExportProfile::ensureLoadedExportProfileList()
{
  KLF_DEBUG_BLOCK(KLF_FUNC_NAME) ;

  if ( ! p_exportProfileList.isEmpty())
    return;

  // read export profile list from XML
  
  // find correct XML file
  QStringList fcandidates;

  QStringList dirs;
  dirs << klfconfig.homeConfigDir  + "/conf/export_mime_profiles.d"
       << klfconfig.globalShareDir + "/conf/export_mime_profiles.d"
       << ":/conf/export_mime_profiles.d";
  int j, k;
  for (j = 0; j < dirs.size(); ++j) {
    // add all files in ...dirs[j].../*.xml
    QDir d(dirs[j]);
    QStringList entrylist;
    entrylist = d.entryList(QStringList()<<QLatin1String("*.xml"), QDir::Files|QDir::Readable, QDir::Name);
    for (k = 0; k < entrylist.size(); ++k)
      fcandidates << d.filePath(entrylist[k]);
  }

  for (k = 0; k < fcandidates.size(); ++k) {
    if (QFile::exists(fcandidates[k]))
      loadFromXMLFile(fcandidates[k]);
  }

  // create a temp exporter on the stack to see which keys it supports, and add all avail formats
  // to an extra export profile
  QList<ExportType> exporttypes;
  KLFMimeExporterImage imgexporter(NULL);
  QStringList mimetypes = imgexporter.keys(NULL);
  for (k = 0; k < mimetypes.size(); ++k) {
    if (mimetypes[k].startsWith("image/") || mimetypes[k] == "application/x-qt-image")
      exporttypes << ExportType(mimetypes[k], imgexporter.windowsFormatName(mimetypes[k]));
  }
  KLFMimeExportProfile allimgfmts
    = KLFMimeExportProfile("all_image_formats",
			   QObject::tr("All Available Image Formats"),
			   exporttypes);
  p_exportProfileList << allimgfmts;

}


// static, private
void KLFMimeExportProfile::loadFromXMLFile(const QString& fname)
{
  KLF_DEBUG_BLOCK(KLF_FUNC_NAME) ;

  klfDbg("Loading from file "<<fname<<"; our locale is "<<klfconfig.UI.locale) ;

  QFile file(fname);
  if ( ! file.open(QIODevice::ReadOnly) ) {
    qWarning()<<KLF_FUNC_NAME<<": Error: Can't open export mime profiles XML file "<<fname<<": "
	      <<file.errorString()<<"!";
    return;
  }

  QDomDocument doc("export-profile-list");
  QString errMsg; int errLine, errCol;
  bool r = doc.setContent(&file, false, &errMsg, &errLine, &errCol);
  if (!r) {
    qWarning()<<KLF_FUNC_NAME<<": Error parsing file "<<fname<<": "<<errMsg<<" at line "<<errLine<<", col "<<errCol;
    return;
  }
  file.close();

  QDomElement root = doc.documentElement();
  if (root.nodeName() != "export-profile-list") {
    qWarning("%s: Error parsing XML for export mime profiles from file `%s': Bad root node `%s'.\n",
	     KLF_FUNC_NAME, qPrintable(fname), qPrintable(root.nodeName()));
    return;
  }

  QStringList descriptionLangs;

  // read XML file
  QDomNode n;
  for (n = root.firstChild(); ! n.isNull(); n = n.nextSibling()) {
    QDomElement e = n.toElement(); // try to convert the node to an element.
    if ( e.isNull() || n.nodeType() != QDomNode::ElementNode )
      continue;
    if ( e.nodeName() == "add-macosx-type-rules" ) {
#ifdef Q_WS_MAC
      __klf_add_macosx_type_rules(fname, e);
#else
      klfDbg("Ignoring Mac OS X type rules on non-mac window system") ;
#endif
      continue;
    }
    if ( e.nodeName() != "profile" ) {
      qWarning("%s: WARNING in parsing XML \"%s\" : ignoring unexpected tag `%s', expected <profile>.\n",
	       KLF_FUNC_NAME, qPrintable(fname), qPrintable(e.nodeName()));
      continue;
    }
    // read profile
    QString pname = e.attribute("name");

    // note: profile may already exist, we will check for that later.

    klfDbg("Reading profile "<<pname<<" ...") ;
    
    QString description;
    QString inSubMenu;
    // xml:lang attribute for profile description is obsolete, we use now Qt-linguist translated value...
    QString curDescriptionLang;
    QList<ExportType> exporttypes;

    QDomNode en;
    for (en = e.firstChild(); ! en.isNull(); en = en.nextSibling() ) {
      if ( en.isNull() || en.nodeType() != QDomNode::ElementNode )
	continue;
      QDomElement ee = en.toElement();
      if ( en.nodeName() == "description" ) {
	// xml:lang attribute for profile description is obsolete, we use now Qt-linguist translated value...
	QString lang = ee.hasAttribute("xml:lang") ? ee.attribute("xml:lang") : QString() ;
	klfDbg("<description>: lang="<<lang<<"; hasAttribute(xml:lang)="<<ee.hasAttribute("xml:lang")
	       <<"; current description="<<description<<",lang="<<curDescriptionLang) ;
	if (description.isEmpty()) {
	  // no description yet
	  if (lang.isEmpty() || lang.startsWith(klfconfig.UI.locale) || klfconfig.UI.locale().startsWith(lang)) {
	    // correct locale
	    //	    klfDbg("remembering description tag with lang="<<lang);
	    description = qApp->translate("xmltr_exportprofiles", ee.text().toUtf8().constData(),
					  "[[tag: <description>]]", QCoreApplication::UnicodeUTF8);
	    // xml:lang attribute for profile description is obsolete, we use now Qt-linguist translated value...
	    curDescriptionLang = lang;
	  }
	  // otherwise skip this tag
	} else {
	  // see if this locale is correct and more specific
	  if ( (lang.startsWith(klfconfig.UI.locale) || klfconfig.UI.locale().startsWith(lang)) &&
	       (curDescriptionLang.isEmpty() || lang.startsWith(curDescriptionLang) ) ) {
	    // then keep it and replace the other
	    //	    klfDbg("remembering description tag with lang="<<lang);
	    description = ee.text();
	    curDescriptionLang = lang;
	  }
	  // otherwise skip this tag
	}
	continue;
      }
      if ( en.nodeName() == "in-submenu" ) {
	inSubMenu = qApp->translate("xmltr_exportprofiles", ee.text().toUtf8().constData(),
				       "[[tag: <in-submenu>]]", QCoreApplication::UnicodeUTF8);
	continue;
      }
      if ( en.nodeName() != "export-type" ) {
	qWarning("%s: WARNING in parsing XML '%s': ignoring unexpected tag `%s' in profile `%s'!\n",
		 KLF_FUNC_NAME, qPrintable(fname), qPrintable(en.nodeName()), qPrintable(pname));
	continue;
      }
      QDomNodeList mimetypetags = ee.elementsByTagName("mime-type");
      if (mimetypetags.size() != 1) {
	qWarning()<<KLF_FUNC_NAME<<": in XML file "<<fname<<", profile "<<pname
		  <<": exactly ONE <mime-type> tag must be present in each <export-type>...</export-type>.";
	continue;
      }
      QDomNodeList wintypetags = ee.elementsByTagName("windows-type");
      if (wintypetags.size() > 1) {
	qWarning()<<KLF_FUNC_NAME<<": in XML file "<<fname<<", profile "<<pname
		  <<": expecting at most ONE <windows-type> tag in each <export-type>...</export-type>.";
	continue;
      }
      QDomNodeList exporternametags = ee.elementsByTagName("exporter-name");
      if (exporternametags.size() > 1) {
	qWarning()<<KLF_FUNC_NAME<<": in XML file "<<fname<<", profile "<<pname
		  <<": expected at most ONE <exporter-name> tag in each <export-type>...</export-type>.";
	continue;
      }
      QString mimetype = mimetypetags.at(0).toElement().text().trimmed();
      QString wintype = QString();
      if (wintypetags.size() == 1) {
	wintype = wintypetags.at(0).toElement().text().trimmed();
      }
      QString exportername = QString();
      if (exporternametags.size() == 1) {
	exportername = exporternametags.at(0).toElement().text().trimmed();
      }

      exporttypes << ExportType(mimetype,  wintype,  exportername);
    }

    // add this profile
    KLFMimeExportProfile profile(pname, description, exporttypes, inSubMenu);

    // check if a profile has not already been declared with same name
    int kp;
    for (kp = 0; kp < p_exportProfileList.size(); ++kp) {
      if (p_exportProfileList[kp].profileName() == pname) {
	break;
      }
    }
    if (kp == p_exportProfileList.size()) {
      // the profile is new
      klfDbg("Adding profile "<<pname<<" to mime export profiles") ;
      if (pname == "default") {
	// prepend default profile, so it's at beginning
	p_exportProfileList.prepend(profile);
      } else {
	p_exportProfileList << profile;
	// xml:lang attribute for profile description is obsolete, we use now Qt-linguist translated value...
	descriptionLangs << curDescriptionLang;
      }
    } else {
      // profile already exists, append data to it
      KLFMimeExportProfile oldp = p_exportProfileList[kp];
      // see if this description provides a better translation
      if (!description.isEmpty() &&
	  (descriptionLangs[kp].isEmpty() || curDescriptionLang.startsWith(descriptionLangs[kp]))) {
	// keep this description
      } else {
	description = oldp.description();
      }
      KLFMimeExportProfile finalp(pname, description,
				  oldp.exportTypes()+exporttypes // concatenate lists
				  );
      p_exportProfileList[kp] = finalp;
    }
  }
}


// ---------------------------------------------------------------------




KLFMimeData::KLFMimeData(const QString& exportProfile, const KLFBackend::klfOutput& output)
  : QMimeData(), pExportProfile(KLFMimeExportProfile::findExportProfile(exportProfile))
{
  KLF_DEBUG_BLOCK(KLF_FUNC_NAME) ;
  klfDbg("export profile "<<exportProfile) ;

  // ensure an instance of KLFMacPasteboardMime object
#ifdef Q_WS_MAC
  __klf_init_the_macpasteboardmime();
#endif

  pOutput = output;

  set_possible_qt_handled_data();

  klfDbg("have formats: "<<formats()) ;
}
KLFMimeData::~KLFMimeData()
{
  KLF_DEBUG_BLOCK(KLF_FUNC_NAME) ;
}

// private
void KLFMimeData::set_possible_qt_handled_data()
{
  KLF_DEBUG_BLOCK(KLF_FUNC_NAME) ;

  // Handle platform-specific default image type with 'application/x-qt-image' and setImage()
  int index;
  if ( (index=pExportProfile.indexOfMimeType(QLatin1String("application/x-qt-image"))) >= 0) {
    klfDbg("Will set image data with setImageData() from the data given by the exporter(s)...");
    // set the image data from the exporter
    QList<KLFMimeExporter*> exporters = pExportProfile.exporterFullLookupFor(index);
    QByteArray img_data;
    foreach (KLFMimeExporter* e, exporters) {
      img_data = e->data(QLatin1String("application/x-qt-image"), pOutput);
      if (!img_data.isEmpty())
	break; // got data
    }
    if (!img_data.isEmpty()) {
      QImage img;
      QList<QByteArray> try_formats = QList<QByteArray>() << "PNG" << "JPEG" << "BMP";
      int k;
      for (k = 0; k < try_formats.size() && img.isNull(); ++k)
	img.loadFromData(img_data, try_formats[k].constData());
      KLF_ASSERT_CONDITION( ! img.isNull() ,
			    "Can't get image for application/x-qt-image for profile "<<pExportProfile.profileName(),
			    return; )  ;
      setImageData(img);
    } else {
      klfWarning("Can't set image data for profile "<<pExportProfile.profileName()
		 <<": all eligible exporters failed to provide an image.") ;
    }
  }

  // Also let Qt handle URLs
  if ( (index=pExportProfile.indexOfMimeType(QLatin1String("text/uri-list"))) >= 0) {
    klfDbg("Will set URL data with setUrls() from the data given by the exporter(s)...");
    // set the URL data from the exporter
    QList<KLFMimeExporter*> exporters = pExportProfile.exporterFullLookupFor(index);
    klfDbg("Got exporter list: len="<<exporters.size());
    QByteArray urls_data;
    foreach (KLFMimeExporter* e, exporters) {
      klfDbg("Exporting URL: trying exporter "<<e->exporterName());
      urls_data = e->data(QLatin1String("text/uri-list"), pOutput);
      if (!urls_data.isEmpty())
	break; // got data
    }
    if (!urls_data.isEmpty()) {
      QList<QUrl> urls;
      QList<QByteArray> urlsdatalist = urls_data.split('\n');
      int k;
      for (k = 0; k < urlsdatalist.size(); ++k) {
	if (urlsdatalist[k].trimmed().isEmpty())
	  continue;
	urls << QUrl::fromEncoded(urlsdatalist[k]);
      }
      klfDbg("setting qt-handled url list: "<<urls) ;
      setUrls(urls);
    } else {
      klfWarning("Can't set URLs data for profile "<<pExportProfile.profileName()
		 <<": all eligible exporters failed to provide a URL list.") ;
    }
  }
}

QStringList KLFMimeData::formats() const
{
  KLF_DEBUG_BLOCK(KLF_FUNC_NAME) ;

  QStringList fmts = pExportProfile.availableExporterMimeTypes(&pOutput);
  if (true/*fmts.contains("application/x-qt-image")*/) {
    if (pQtOwnedFormats.size() == 0)
      pQtOwnedFormats = QMimeData::formats();
    klfDbg("Formats added by qt: "<<pQtOwnedFormats) ;
    fmts << pQtOwnedFormats;
  }

  klfDbg("format list: "<<fmts) ;
  return fmts;
}

QVariant KLFMimeData::retrieveData(const QString& mimetype, QVariant::Type type) const
{
  KLF_DEBUG_BLOCK(KLF_FUNC_NAME) ;

  const_cast<KLFMimeData*>(this)->emitDroppedData(mimetype);

  klfDbg("retrieveData: mimetype="<<mimetype<<"; type="<<type) ;

  if (mimetype == QLatin1String("application/x-qt-mime-type-name")) {
    // looks like this is some Qt-specific thing that returns the handcrafted
    // wrapper mime type name; let qt do the job
    klfDbg("Letting Qt handle "<<mimetype<<" w/ type="<<type) ;
    return QMimeData::retrieveData(mimetype, type);
  }

  if (mimetype == QLatin1String("application/x-qt-image") ||
      pQtOwnedFormats.contains(mimetype)) {
    // these types are handled directly by QMimeData
    klfDbg("Letting Qt handle "<<mimetype<<" w/ type="<<type) ;
    return QMimeData::retrieveData(mimetype, type);
  }

  int index = pExportProfile.indexOfMimeType(mimetype);
  if (index < 0) {
    // do not treat this as an error since on windows we seem to have requests for 'text/uri-list' even
    // if that mime type is not returned by formats()
    klfDbg("Can't find mime-type "<<mimetype<<" in export profile "<<pExportProfile.profileName()
	   <<" ?!?");
    return QMimeData::retrieveData(mimetype, type);
  }

  klfDbg("exporting "<<mimetype<<" ... index="<<index);

  // get the data
  QByteArray data = pExportProfile.exportData(index, pOutput);
  
  klfDbg("exporting mimetype "<<mimetype<<": data length is "<<data.size());
  
  return QVariant::fromValue<QByteArray>(data);
}




// ---------------------------------------------------------------------

static KLFMapInitData<QString> datatypeformime_data[] = {
  { "image/png", "PNG" },
  { "image/x-win-png-office-art", "PNG" },
  { "image/eps", "EPS" },
  { "application/eps", "EPS" },
  { "application/postscript", "PS" },
  { "application/pdf", "PDF" },
  { "image/svg+xml", "SVG" },
  { "application/x-dvi", "DVI" },
  { "image/jpeg", "JPEG" },
  { "application/x-qt-image", "PNG" },
  { NULL, QString() }
};
static QMap<QString,QString> datatypeformime = klfMakeMap(datatypeformime_data);


QStringList KLFMimeExporterImage::keys(const KLFBackend::klfOutput * output) const
{
  KLF_DEBUG_BLOCK(KLF_FUNC_NAME) ;

  QStringList backendformats = KLFBackend::availableSaveFormats(output);

  QSet<QString> keys;

  foreach (QString fmt, backendformats) {
    if (fmt == "PNG") {
      keys << "image/png"
	// add duplicate for png, see below
	   << "image/x-win-png-office-art";
    } else if (fmt == "EPS") {
      keys << "image/eps" << "application/eps";
    } else if (fmt == "PS") {
      keys << "application/postscript";
    } else if (fmt == "PDF") {
      keys << "application/pdf";
    } else if (fmt == "SVG") {
      keys << "image/svg+xml";
    } else if (fmt == "DVI") {
      keys << "application/x-dvi";
    } else if (fmt == "JPEG") {
      keys << "image/jpeg";
    } else {
      // other backend image format, probably from Qt's export image formats
      keys << "image/"+fmt.toLower();
    }
  }

  // always have these formats...
  keys << "image/png" << OPENOFFICE_DRAWING_MIMETYPE << "application/x-qt-image";

  return keys.toList();
}

QString KLFMimeExporterImage::windowsFormatName(const QString& mime) const
{
  KLF_DEBUG_BLOCK(KLF_FUNC_NAME) ;

  QString wtype;
  if (mime == "application/pdf")
    return "PDF";
  else if (mime == "application/eps")
    return "Encapsulated PostScript";
  else if (mime == "image/png")
    return "PNG";
  else if (mime == "image/jpg" || mime == "image/jpeg")
    // standards should only allow image/jpeg, but just in case we treat also erroneous "image/jpg"
    return "JPEG";
  else if (mime == "image/x-win-jfif")
    return "JFIF";
  else if (mime == "image/x-win-jfif-office-art")
    return "JFIF+Office Art"; // for Ms Office
  else if (mime == "image/x-win-png-office-art")
    return "PNG+Office Art"; // for Ms Office
  else if (mime == "image/bmp")
    return "Bitmap";
  else if (mime == "image/x-win-bmp")
    return "Windows Bitmap";
  else if (mime == OPENOFFICE_DRAWING_MIMETYPE)
    return "Drawing Format";
  else if (mime == "application/x-qt-image")
    return mime; // let Qt translate this one

  return mime;
}



QByteArray klf_openoffice_drawing(const KLFBackend::klfOutput& klfoutput);

QByteArray KLFMimeExporterImage::data(const QString& keymime, const KLFBackend::klfOutput& klfoutput)
{
  KLF_DEBUG_BLOCK(KLF_FUNC_NAME) ;

  // added by us, not klfbackend
  if (keymime == OPENOFFICE_DRAWING_MIMETYPE)
    return klf_openoffice_drawing(klfoutput);

  KLFMimeString mimestr(keymime);
  QString keymime_base = mimestr.mime_base;

  QString datatype = datatypeformime.value(keymime_base, QString());

  if (datatype == QString() && keymime_base.startsWith("image/"))
    datatype = keymime_base.mid(qstrlen("image/")).toUpper();

  KLF_ASSERT_CONDITION( !datatype.isEmpty(), "Can't find corresponding datatype for "<<keymime_base<<" !",
			return QByteArray(); ) ;

  QByteArray data;
  QString errorstring;
  bool result;

  { // separate block to ensure the buffer is destroyed before accessing the saved data.
    QBuffer buf(&data);
    buf.open(QIODevice::WriteOnly);

    result = KLFBackend::saveOutputToDevice(klfoutput, &buf, datatype, &errorstring);
  }

  if (!result) { // error
    klfWarning("Data export error ("<<datatype<<"): "<<errorstring) ;
    return QByteArray();
  }

  return data;
}



// ---------------------------------------------------------------------

inline qint64 qHash(const QRegExp& rx) { return qHash(rx.pattern()); }

QMap<qint64,QMap<int,QString> > KLFMimeExporterUrilist::tempFilesForImageCacheKey =
  QMap<qint64,QMap<int,QString> >();

QStringList KLFMimeExporterUrilist::keys(const KLFBackend::klfOutput *) const
{
  KLF_DEBUG_BLOCK(KLF_FUNC_NAME) ;
  return QStringList() << "text/x-moz-url" << "text/uri-list" << "text/x-klf-mac-fileurl";
}

// static
QString KLFMimeExporterUrilist::tempFileForOutput(const KLFBackend::klfOutput& output, int targetDpi)
{
  qint64 imgcachekey = output.result.cacheKey();

  QString tempfilename;

  if (targetDpi <= 0)
    targetDpi = output.input.dpi;

  if (tempFilesForImageCacheKey.contains(imgcachekey) &&
      tempFilesForImageCacheKey[imgcachekey].contains(targetDpi)) {
    return tempFilesForImageCacheKey[imgcachekey][targetDpi];
  }

  QString templ = klfconfig.BackendSettings.tempDir +
    QString::fromLatin1("/klf_%2_XXXXXX.%1.png")
    .arg(targetDpi).arg(QDateTime::currentDateTime().toString("yyyy-MM-dd_hh-mm"));
  QTemporaryFile *tempfile = new QTemporaryFile(templ, qApp);
  tempfile->setAutoRemove(true); // will be deleted when klatexformula exists (= qApp destroyed)
  if (tempfile->open() == false) {
    qWarning("Can't open temp png file for mimetype text/uri-list: template is %s",
	     qPrintable(templ));
    return QByteArray();
  }
  QString errStr;
  QImage img = output.result;
  if (targetDpi > 0 && targetDpi != output.input.dpi) {
    // scale image as needed
    QSize targetSize = img.size();
    targetSize *= (float) targetDpi / output.input.dpi;
    klfDbg("scaling to "<<targetDpi<<" DPI from "<<output.input.dpi<<" DPI... targetSize="<<targetSize) ;
    img = klfImageScaled(img, targetSize);
  }

  bool res = img.save(tempfile, "PNG");
  if (!res) {
    qWarning()<<KLF_FUNC_NAME<<": Can't save to temp file "<<tempfile->fileName()<<": "<<errStr;
    return QString();
  }

  tempfilename = tempfile->fileName();
  tempfile->close();
  // cache this temp file for other formats' use or other QMimeData instantiation...
  tempFilesForImageCacheKey[imgcachekey][targetDpi] = tempfilename;

  return tempfilename;
}

QByteArray KLFMimeExporterUrilist::data(const QString& key, const KLFBackend::klfOutput& output)
{
  KLF_DEBUG_BLOCK(KLF_FUNC_NAME) ;

  klfDbg("key="<<key) ;

  int req_dpi = -1;
  KLFMimeString mimestr(key);
  for (QMap<QString,QString>::const_iterator it = mimestr.mime_attrs.begin(); it != mimestr.mime_attrs.end(); ++it) {
    const QString& attr_key = it.key();
    const QString& attr_val = it.value();
    if (attr_key == "dpi") {
      req_dpi = attr_val.toInt();
    } else {
      klfWarning("Unrecognized attribute in mime-type: "<<attr_key<<" in "<<key);
    }
  }

  QString tempfilename = tempFileForOutput(output, req_dpi);
  QUrl url = QUrl::fromLocalFile(tempfilename);
#ifdef Q_WS_MAC
  //  if (key == "text/x-klf-mac-fileurl") {
  //  url.setHost("localhost");
  //  }
#endif
  QByteArray urilist = (url.toString()+QLatin1String("\n")).toLatin1();
  return urilist;
}

QString KLFMimeExporterUrilist::windowsFormatName(const QString& mime) const
{
  KLF_DEBUG_BLOCK(KLF_FUNC_NAME) ;

  if (mime == "text/x-moz-url")
    return "FileName";
  return mime;
}



// ---------------------------------------------------------------------

QMap<qint64,QString> KLFMimeExporterUrilistPDF::tempFilesForImageCacheKey = QMap<qint64,QString>();

QStringList KLFMimeExporterUrilistPDF::keys(const KLFBackend::klfOutput *) const
{
  KLF_DEBUG_BLOCK(KLF_FUNC_NAME) ;
  return QStringList() << "text/x-moz-url" << "text/uri-list" << "text/x-klf-mac-fileurl";
}

// static
QString KLFMimeExporterUrilistPDF::tempFileForOutput(const KLFBackend::klfOutput& output)
{
  qint64 cachekey = qHash(output.pdfdata);

  if (output.pdfdata.isEmpty()) {
    klfWarning("Request to export as PDF but we have no data!");
    return QString();
  }

  if (tempFilesForImageCacheKey.contains(cachekey)) {
    return tempFilesForImageCacheKey[cachekey];
  }

  QString templ = klfconfig.BackendSettings.tempDir +
    QString::fromLatin1("/klf_%2_XXXXXX.pdf")
    .arg(QDateTime::currentDateTime().toString("yyyy-MM-dd_hh-mm"));
  QTemporaryFile *tempfile = new QTemporaryFile(templ, qApp);
  tempfile->setAutoRemove(true); // will be deleted when klatexformula exists (= qApp destroyed)
  if (tempfile->open() == false) {
    klfWarning("Can't open temp pdf file for mimetype text/uri-list: template is "<<templ);
    return QString();
  }

  tempfile->write(output.pdfdata);

  QString tempfilename = tempfile->fileName();
  tempfile->close();

  // cache this temp file for other formats' use or other QMimeData instantiation...
  tempFilesForImageCacheKey[cachekey] = tempfilename;

  return tempfilename;
}

QByteArray KLFMimeExporterUrilistPDF::data(const QString& key, const KLFBackend::klfOutput& output)
{
  KLF_DEBUG_BLOCK(KLF_FUNC_NAME) ;

  Q_UNUSED(key) ;
  klfDbg("key="<<key) ;

  QString tempfilename = tempFileForOutput(output);
  QUrl url = QUrl::fromLocalFile(tempfilename);
#ifdef Q_WS_MAC
  //  if (key == "text/x-klf-mac-fileurl") {
  //  url.setHost("localhost");
  //  }
#endif
  QByteArray urilist = (url.toString()+QLatin1String("\n")).toLatin1();
  return urilist;
}

QString KLFMimeExporterUrilistPDF::windowsFormatName(const QString& mime) const
{
  KLF_DEBUG_BLOCK(KLF_FUNC_NAME) ;

  if (mime == "text/x-moz-url")
    return "FileName";
  return mime;
}




// -----------------------------------

static QString toAttrTextS(const QString& sbase)
{
  QString s = sbase; // we need a non-const string to .replace() on
  klfDbg("s="<<s);
  QRegExp replaceCharsRX("([^a-zA-Z0-9/ ._-])");
  int pos = 0;
  while ((pos = replaceCharsRX.indexIn(s, pos)) != -1) {
    QString entity = "&#x"+QString::number(replaceCharsRX.cap(1)[0].unicode(), 16).toUpper()+";" ;
    klfDbg("replacing char at pos="<<pos<<" by entity="<<entity<<": s(pos...pos+5)="<<s.mid(pos,5));
    s.replace(pos, replaceCharsRX.matchedLength(), entity);
    pos += entity.length();
  }
  klfDbg("final string: "<<s);
  return s;

  //  QString s2 = sbase;
  //  return s2.replace("&", "&amp;").replace("\"", "&quot;").replace("'", "&apos;").replace("<", "&lt")
  //    .replace(">", "&gt;").replace("\r", "&#xD;").replace("\t", "&#x9;").replace("\n", "&#xA;")
  //    .replace("!", "&#x21;").toUtf8();
}

static QByteArray toAttrText(const QString& sbase)
{
  return toAttrTextS(sbase).toUtf8();
}

QStringList KLFMimeExporterHTML::keys(const KLFBackend::klfOutput *) const
{
  return QStringList() << QLatin1String("text/html");
}

static QHash<QRegExp, QString> klf_getReplaceDisplayLatex()
{
  static QHash<QRegExp, QString> replaceDisplayLatex;
  if (replaceDisplayLatex.isEmpty()) {
    // fill it up !

    //    \! \, \; \:  -> simple space
    replaceDisplayLatex[QRegExp("\\\\[,;:!]")] = " ";
    //    \text{Hello}, \mathrm{Hilbert-Einstein}  -->  {the text}
    replaceDisplayLatex[QRegExp("\\\\(?:text|mathrm)\\{((?:\\w|\\s|[._-])*)\\}")] = "{\\1}";

    //    \var(epsilon|phi|...)    ->   \epsilon,\phi,...
    replaceDisplayLatex[QRegExp("\\\\var([a-zA-Z]+)")] = "\\\\1";

  }
  return replaceDisplayLatex;
}

QByteArray KLFMimeExporterHTML::data(const QString& key, const KLFBackend::klfOutput& klfoutput)
{
  if (key != QLatin1String("text/html")) {
    qWarning()<<KLF_FUNC_NAME<<": key="<<key<<" is not \"text/html\"";
    return QByteArray();
  }

  int htmldpi = klfconfig.ExportData.htmlExportDpi;
  if (klfoutput.input.dpi < htmldpi * 1.25f)
    htmldpi = klfoutput.input.dpi;

  QString fname = KLFMimeExporterUrilist::tempFileForOutput(klfoutput, htmldpi);

  int dispDpi = klfconfig.ExportData.htmlExportDisplayDpi;

  QSize imgsize = klfoutput.result.size();
  imgsize *= (float) htmldpi / klfoutput.input.dpi;

  QString latex = klfoutput.input.latex;
  // remove initial comments from latex code...
  QStringList latexlines = latex.split("\n");
  while (latexlines.size() && QRegExp("\\s*\\%.*").exactMatch(latexlines[0]))
    latexlines.removeAt(0);
  latex = latexlines.join("\n");

  // format LaTeX code nicely to be displayed
  QHash<QRegExp, QString> replaceDisplayLatex = klf_getReplaceDisplayLatex();
  for (QHash<QRegExp, QString>::iterator it = replaceDisplayLatex.begin();
       it != replaceDisplayLatex.end(); ++it) {
    latex.replace(it.key(), it.value());
  }

  QString fn = toAttrTextS(fname);
  QString l = toAttrTextS(latex);
  //  QString w = QString::number((int)(imgsize.width() * dispDpi/htmldpi));
  //  QString h = QString::number((int)(imgsize.height() * dispDpi/htmldpi));
  QString win = QString::number((float)imgsize.width() / dispDpi, 'f', 3);
  QString hin = QString::number((float)imgsize.height() / dispDpi, 'f', 3);

  klfDbg("origimg/size="<<klfoutput.result.size()<<"; origDPI="<<klfoutput.input.dpi<<"; htmldpi="<<htmldpi
	 <<"; dispDPI="<<dispDpi<<"; imgsize="<<imgsize<<"; win="<<win<<"; hin="<<hin) ;

  QString html =
    QString("<img src=\"file://%1\" alt=\"%2\" title=\"%3\" " //"width=\"%4\" height=\"%5\" "
	    " style=\"width: %4in; height: %5in; vertical-align: middle;\">")
    .arg(fn, l, l, win, hin);

#ifdef Q_WS_MAC
  return html.toUtf8();
#else
  QTextCodec *codec = QTextCodec::codecForName("UTF-16");
  return codec->fromUnicode(html);
#endif
}

QString KLFMimeExporterHTML::windowsFormatName(const QString& key) const
{
  if (key == QLatin1String("text/html"))
    return "HTML";
  return key;
}


// -----------------------------------


QByteArray klf_openoffice_drawing(const KLFBackend::klfOutput& klfoutput)
{
  KLF_DEBUG_BLOCK(KLF_FUNC_NAME) ;

  QByteArray pngdata = klfoutput.pngdata;

  QFile templfile(":/data/ooodrawingtemplate");
  templfile.open(QIODevice::ReadOnly);
  QByteArray templ = templfile.readAll();

  QString fgcols = QColor(klfoutput.input.fg_color).name();
  QString bgcols;
  if (qAlpha(klfoutput.input.bg_color) > 0)
    bgcols = QColor(klfoutput.input.fg_color).name();
  else
    bgcols = "-";

  templ.replace(QByteArray("<!--KLF_PNG_BASE64_DATA-->"), pngdata.toBase64());

  templ.replace(QByteArray("<!--KLF_INPUT_LATEX-->"), toAttrText(klfoutput.input.latex));
  templ.replace(QByteArray("<!--KLF_INPUT_MATHMODE-->"), toAttrText(klfoutput.input.mathmode));
  templ.replace(QByteArray("<!--KLF_INPUT_PREAMBLE-->"), toAttrText(klfoutput.input.preamble));
  templ.replace(QByteArray("<!--KLF_INPUT_FGCOLOR-->"), toAttrText(fgcols));
  templ.replace(QByteArray("<!--KLF_INPUT_BGCOLOR-->"),	toAttrText(bgcols));
  templ.replace(QByteArray("<!--KLF_INPUT_DPI-->"), toAttrText(QString::number(klfoutput.input.dpi)));
  templ.replace(QByteArray("<!--KLF_SETTINGS_TBORDEROFFSET_PSPT-->"),
		toAttrText(QString::number(klfoutput.settings.tborderoffset)));
  templ.replace(QByteArray("<!--KLF_SETTINGS_RBORDEROFFSET_PSPT-->"),
		toAttrText(QString::number(klfoutput.settings.rborderoffset)));
  templ.replace(QByteArray("<!--KLF_SETTINGS_BBORDEROFFSET_PSPT-->"),
		toAttrText(QString::number(klfoutput.settings.bborderoffset)));
  templ.replace(QByteArray("<!--KLF_SETTINGS_LBORDEROFFSET_PSPT-->"),
		toAttrText(QString::number(klfoutput.settings.lborderoffset)));

  templ.replace(QByteArray("<!--KLF_INPUT_LATEX_BASE64-->"), klfoutput.input.latex.toLocal8Bit().toBase64());
  templ.replace(QByteArray("<!--KLF_INPUT_MATHMODE_BASE64-->"), klfoutput.input.mathmode.toLocal8Bit().toBase64());
  templ.replace(QByteArray("<!--KLF_INPUT_PREAMBLE_BASE64-->"), klfoutput.input.preamble.toLocal8Bit().toBase64());
  templ.replace(QByteArray("<!--KLF_INPUT_FGCOLOR_BASE64-->"), fgcols.toLocal8Bit().toBase64());
  templ.replace(QByteArray("<!--KLF_INPUT_BGCOLOR_BASE64-->"), bgcols.toLocal8Bit().toBase64());

  templ.replace(QByteArray("<!--KLF_OOOLATEX_ARGS-->"), toAttrText("12§display§"+klfoutput.input.latex));

  // scale equation (eg. make them larger, so it is not too cramped up)
  const double DPI_FACTOR = klfconfig.ExportData.oooExportScale;

  // cm/inch = 2.54
  // include an elargment factor in these tags
  templ.replace(QByteArray("<!--KLF_IMAGE_WIDTH_CM-->"),
		QString::number(DPI_FACTOR * 2.54 * klfoutput.result.width()/klfoutput.input.dpi, 'f', 2).toUtf8());
  templ.replace(QByteArray("<!--KLF_IMAGE_HEIGHT_CM-->"),
		QString::number(DPI_FACTOR * 2.54 * klfoutput.result.height()/klfoutput.input.dpi, 'f', 2).toUtf8());
  // same, without the enlargment factor
  templ.replace(QByteArray("<!--KLF_IMAGE_ORIG_WIDTH_CM-->"),
		QString::number(2.54 * klfoutput.result.width()/klfoutput.input.dpi, 'f', 2).toUtf8());
  templ.replace(QByteArray("<!--KLF_IMAGE_ORIG_HEIGHT_CM-->"),
		QString::number(2.54 * klfoutput.result.height()/klfoutput.input.dpi, 'f', 2).toUtf8());

  templ.replace(QByteArray("<!--KLF_IMAGE_WIDTH_PX-->"), QString::number(klfoutput.result.width()).toUtf8());
  templ.replace(QByteArray("<!--KLF_IMAGE_HEIGHT_PX-->"), QString::number(klfoutput.result.height()).toUtf8());
  templ.replace(QByteArray("<!--KLF_IMAGE_ASPECT_RATIO-->"),
		QString::number((double)klfoutput.result.width()/klfoutput.result.height(), 'f', 3).toUtf8());

  klfDbg("final templ: "<<templ);

  return templ;
}





// ---------------------------



QStringList KLFMimeExporterLibFmts::keys(const KLFBackend::klfOutput *) const
{
  KLF_DEBUG_BLOCK(KLF_FUNC_NAME) ;

  klfDbg("list of keys: "<<KLFAbstractLibEntryMimeEncoder::allEncodingMimeTypes());
  return KLFAbstractLibEntryMimeEncoder::allEncodingMimeTypes();
}

QByteArray KLFMimeExporterLibFmts::data(const QString& key, const KLFBackend::klfOutput& output)
{
  KLF_DEBUG_BLOCK(KLF_FUNC_NAME) ;

  klfDbg("key="<<key);
  KLFAbstractLibEntryMimeEncoder *encoder =
    KLFAbstractLibEntryMimeEncoder::findEncoderFor(key, true);
  if (encoder == NULL) {
    // warning already issued in findEncoderFor(.., TRUE)
    return QByteArray();
  }

  KLFLibEntry e = KLFLibEntry(output.input.latex, QDateTime::currentDateTime(),
			      output.result.scaled(klfconfig.UI.labelOutputFixedSize, Qt::KeepAspectRatio,
						   Qt::SmoothTransformation),
			      KLFStyle(output.input));

  QByteArray data = encoder->encodeMime(KLFLibEntryList()<<e, QVariantMap(), key);

  if (!data.size())
    qWarning()<<KLF_FUNC_NAME<<": "<<key<<" encoder returned empty data!";

  klfDbg("got data, size="<<data.size());
  return data;
}




// -------------------------------

QStringList KLFMimeExporterGlowImage::keys(const KLFBackend::klfOutput *) const
{
  KLF_DEBUG_BLOCK(KLF_FUNC_NAME) ;

  return QStringList() << "image/png" << "application/x-qt-image";
}

QByteArray KLFMimeExporterGlowImage::data(const QString& key, const KLFBackend::klfOutput& output)
{
  KLF_DEBUG_BLOCK(KLF_FUNC_NAME) ;

  klfDbg("key = "<<key) ;
  
  QImage img(output.result.size() + QSize(1,1)*2*klfconfig.UI.glowEffectRadius, QImage::Format_ARGB32);

  // initialize image to transparent
  img.fill(qRgba(0,0,0,0));

  { // now draw the glowed equation
    QPainter p(&img);
    p.translate(klfconfig.UI.glowEffectRadius, klfconfig.UI.glowEffectRadius);
    klfDrawGlowedImage(&p, output.result, klfconfig.UI.glowEffectColor, klfconfig.UI.glowEffectRadius);
  }

  QByteArray data;
  { // save the image as PNG data into our data qbytearray
    QBuffer buf(&data);
    buf.open(QIODevice::WriteOnly);
    img.save(&buf, "PNG");
  }

  // and return the PNG data
  return data;
}


// -----------------------------

struct KLFExportTypeUserScriptInfoPrivate
{
  KLF_PRIVATE_HEAD(KLFExportTypeUserScriptInfo)
  {
    klfDbg("Initializing 'private': ptr="<<ptr<<", K="<<K) ;
    normalizedlists = false;
  }
  void copy(KLFExportTypeUserScriptInfoPrivate *other)
  {
    normalizedlists = other->normalizedlists;
  }

  bool normalizedlists;

  void normalizeLists()
  {
    KLF_DEBUG_BLOCK(KLF_FUNC_NAME) ;
    klfDbg("K="<<K) ;
    klfDbg("K->scriptname="<<K->scriptName()) ;
    if (K->info("__klf_exporttypeuserscriptinfo_normalizedlists").toBool()) {
      normalizedlists = true;
      return;
    }
    QStringList mtypes = K->info("MimeType").toStringList();
    QStringList fexts = K->info("OutputFilenameExtension").toStringList();
    QStringList fdesc = K->info("OutputFormatDescription").toStringList();
    klfDbg("lists before normalization: mtypes="<<mtypes<<";  fexts="<<fexts<<";  fdesc="<<fdesc) ;
    int sz = qMax(mtypes.size(), qMax(fexts.size(), fdesc.size()));
    while (mtypes.size() < sz)
      mtypes.append(QString());
    while (fexts.size() < sz)
      fexts.append(QString());
    while (fdesc.size() < sz) {
      QString s = fexts[fdesc.size()].toUpper();
      if (s.isEmpty()) {
	s = mtypes[fdesc.size()];
      }
      fdesc.append(QObject::tr("%1 File", "[[default file type description from file extension]]").arg(s));
    }
    klfDbg("normalized lists.") ;
    K->internalSetProperty("MimeType", QVariant(mtypes));
    K->internalSetProperty("OutputFilenameExtension", QVariant(fexts));
    K->internalSetProperty("OutputFormatDescription", QVariant(fdesc));
    K->internalSetProperty("__klf_exporttypeuserscriptinfo_normalizedlists", QVariant(true));
    normalizedlists = true;
  }

  QString listrangecheck(const QString& prop, int index)
  {
    QStringList l = K->info(prop).toStringList();
    if (index < 0 || index >= l.size()) {
      klfWarning("User Script "<<K->scriptName()<<": index "<<index<<" out of range for property "<<prop) ;
      return QString();
    }
    return l[index];
  }

  friend class KLFExportTypeUserScriptInfo;
};

KLFExportTypeUserScriptInfo::KLFExportTypeUserScriptInfo(const QString& scriptFileName,
							 KLFBackend::klfSettings * settings)
  : KLFUserScriptInfo(scriptFileName, settings, klfconfig.UserScripts.userScriptConfig.value(scriptFileName))
{
  KLF_INIT_PRIVATE(KLFExportTypeUserScriptInfo) ;
}

KLFExportTypeUserScriptInfo::KLFExportTypeUserScriptInfo(const KLFExportTypeUserScriptInfo& copy)
  : KLFUserScriptInfo(copy)
{
  KLF_INIT_PRIVATE(KLFExportTypeUserScriptInfo) ;
  d->copy(copy.d);
}
KLFExportTypeUserScriptInfo::KLFExportTypeUserScriptInfo(const KLFUserScriptInfo& copy)
  : KLFUserScriptInfo(copy)
{
  KLF_INIT_PRIVATE(KLFExportTypeUserScriptInfo) ;
}

KLFExportTypeUserScriptInfo::~KLFExportTypeUserScriptInfo()
{
  KLF_DELETE_PRIVATE ;
}


QStringList KLFExportTypeUserScriptInfo::mimeTypes() const
{
  d->normalizeLists();
  return info("MimeType").toStringList();
}
QStringList KLFExportTypeUserScriptInfo::outputFilenameExtensions() const
{
  d->normalizeLists();
  return info("OutputFilenameExtension").toStringList();
}
QStringList KLFExportTypeUserScriptInfo::outputFormatDescriptions() const
{
  d->normalizeLists();
  return info("OutputFormatDescription").toStringList();
}
QString KLFExportTypeUserScriptInfo::inputDataType() const
{
  KLF_DEBUG_BLOCK(KLF_FUNC_NAME) ;
  klfDbg("this="<<this<<"; info('mimetype') is "<<info("MimeType") );
  d->normalizeLists();
  return info("InputDataType").toString();
}

int KLFExportTypeUserScriptInfo::count() const
{
  d->normalizeLists();
  return info("MimeType").toStringList().size();
}
int KLFExportTypeUserScriptInfo::findMimeType(const QString& mimeType) const
{
  d->normalizeLists();
  return info("MimeType").toStringList().indexOf(mimeType);
}

QString KLFExportTypeUserScriptInfo::mimeType(int index) const
{
  d->normalizeLists();
  return d->listrangecheck("MimeType", index);
}
QString KLFExportTypeUserScriptInfo::outputFilenameExtension(int index) const
{
  d->normalizeLists();
  return d->listrangecheck("OutputFilenameExtension", index);
}
QString KLFExportTypeUserScriptInfo::outputFormatDescription(int index) const
{
  d->normalizeLists();
  return d->listrangecheck("OutputFormatDescription", index);
}

bool KLFExportTypeUserScriptInfo::wantStdinInput() const
{
  return info("WantStdinInput").toBool();
}
bool KLFExportTypeUserScriptInfo::hasStdoutOutput() const
{
  return info("HasStdoutOutput").toBool();
}


// --------------------------

struct KLFExportUserScriptPrivate {
  KLF_PRIVATE_HEAD(KLFExportUserScript)
  {
    info = NULL;
  }

  KLFExportTypeUserScriptInfo * info;
};

KLFExportUserScript::KLFExportUserScript(const QString& scriptFileName, KLFBackend::klfSettings * settings)
{
  KLF_INIT_PRIVATE(KLFExportUserScript) ;

  d->info = new KLFExportTypeUserScriptInfo(scriptFileName, settings);
}
KLFExportUserScript::~KLFExportUserScript()
{
  delete d->info;
  KLF_DELETE_PRIVATE ;
}


KLFExportTypeUserScriptInfo KLFExportUserScript::info() const
{
  return *d->info;
}

QStringList KLFExportUserScript::availableMimeTypes(const KLFBackend::klfOutput * output) const
{
  KLF_DEBUG_BLOCK(KLF_FUNC_NAME) ;

  QStringList availbackendsave = KLFBackend::availableSaveFormats(output);
  klfDbg("availbackendsave="<<availbackendsave) ;
  if (availbackendsave.contains(d->info->inputDataType(), Qt::CaseInsensitive)) {
    QStringList mlist = d->info->mimeTypes();
    klfDbg("keys: "<<mlist) ;
    return mlist;
  }

  klfDbg("The output does not provide format "<<d->info->inputDataType()<<", needed to feed as input "
	 "to user script "<<d->info->scriptName()) ;
  return QStringList();
}

// klfbackend.cpp
extern QString klfbackend_last_userscript_output;

QByteArray KLFExportUserScript::getData(const QString& key, const KLFBackend::klfOutput& klfoutput)
{
  KLF_DEBUG_BLOCK(KLF_FUNC_NAME) ;
  klfDbg("key="<<key) ;

  int mtypeindex = d->info->mimeTypes().indexOf(key);
  if (mtypeindex < 0) {
    klfWarning("User Script "<<d->info->scriptName()<<" does not support key "<<key<<" !") ;
    return QByteArray();
  }
  QString outfext;
  if (!d->info->hasStdoutOutput()) {
    if (mtypeindex >= d->info->outputFilenameExtensions().size()) {
      klfWarning("User Script "<<d->info->scriptName()
		 <<" does not provide matching output filename extension for "<<key<<" !") ;
      return QByteArray();
    }
    outfext = d->info->outputFilenameExtensions() [mtypeindex];
  }

  // now actually call the script

  KLFUserScriptFilterProcess p(d->info->fileName(), &klfoutput.settings);

  if (klfconfig.UserScripts.userScriptConfig.contains(d->info->fileName()))
    p.addUserScriptConfig(klfconfig.UserScripts.userScriptConfig.value(d->info->fileName()));

  QString infmt = d->info->inputDataType();

  QTemporaryFile *f_in = NULL;
  if (!d->info->wantStdinInput()) {
    QString ver = KLF_VERSION_STRING;
    ver.replace(".", "x"); // make friendly names with chars in [a-zA-Z0-9]
    QString ext = infmt.toLower();
    QString tempinfname = klfoutput.settings.tempdir + "/klftmpmime" + ver + "xXXXXXX" + "."+ext;
    
    f_in = new QTemporaryFile(tempinfname);
    f_in->setAutoRemove(true);
    f_in->open();
  }

  QString errstr;
  QIODevice *where = NULL;
  QBuffer buf;
  QByteArray indata;
  if (f_in)
    where = f_in;
  else {
    buf.setBuffer(&indata);
    buf.open(QIODevice::WriteOnly);
    where = &buf;
  }
  KLFBackend::saveOutputToDevice(klfoutput, where, infmt, &errstr);
  if (!errstr.isEmpty()) {
    klfWarning("Can't save output to format "<<infmt<<": "<<errstr) ;
    return QByteArray();
  }
  where->close();

  p.setArgv(QStringList() << d->info->fileName());
  if (f_in != NULL) {
    p.addArgv(QStringList() << f_in->fileName());
  }
  p.addArgv(QStringList() << "--mimetype="+key);

  /** \todo provide more information to the script present in the klfOutput object,
   * e.g. width_pt, height_pt etc. as environment variables.
   *
   * Should be easy!!
   */

  // collect output
  QByteArray stdoutdata;
  QByteArray stderrdata;
  p.collectStdoutTo(&stdoutdata);
  p.collectStderrTo(&stderrdata);

  QByteArray foutdata;
  QMap<QString,QByteArray*> outdatamap;
  QString outfn;
  if (!d->info->hasStdoutOutput()) {
    if (f_in == NULL) {
      klfWarning(d->info->fileName()<<": if you take stdin input, you have to provide stdout output, sorry.") ;
      return QByteArray();
    }
    QFileInfo infi(f_in->fileName());
    outfn = infi.absolutePath() + "/" + infi.completeBaseName() + "." + outfext;
    outdatamap[outfn] = &foutdata;
    klfDbg("outfn="<<outfn) ;
  }

  // now, run the user script
  bool ok = p.run(indata, outdatamap);

  if (outfn.size() && QFile::exists(outfn)) {
    QFile::remove(outfn);
  }
  if (f_in != NULL)
    delete f_in;

  if (!ok) {
    // error
    klfWarning("Can't run userscript "<<d->info->fileName()<<": "<<p.resultErrorString()) ;
    klfbackend_last_userscript_output = p.resultErrorString();
    return QByteArray();
  }

  // for user script debugging
  klfbackend_last_userscript_output
    = "<b>STDOUT</b>\n<pre>" + Qt::escape(stdoutdata) + "</pre>\n<br/><b>STDERR</b>\n<pre>"
    + Qt::escape(stderrdata) + "</pre>";
  
  klfDbg("Ran script "<<d->info->fileName()<<": stdout="<<stdoutdata<<"\n\tstderr="<<stderrdata) ;

  // and retreive the output data
  if (!foutdata.isEmpty()) {
    klfDbg("Got file output data: size="<<foutdata.size()) ;
    return foutdata;
  }

  return stdoutdata;
}



// -------------------


KLFMimeExporterUserScript::KLFMimeExporterUserScript(const QString& uscript, QObject *parent)
  : QObject(parent), pUserScript(uscript, NULL)
{
}

QString KLFMimeExporterUserScript::exporterName() const
{
  return QString::fromLatin1("UserScript:") + QFileInfo(pUserScript.info().fileName()).fileName();
}

QStringList KLFMimeExporterUserScript::keys(const KLFBackend::klfOutput * output) const
{
  return pUserScript.availableMimeTypes(output);
}


QByteArray KLFMimeExporterUserScript::data(const QString& key, const KLFBackend::klfOutput& klfoutput)
{
  KLF_DEBUG_BLOCK(KLF_FUNC_NAME) ;

  return pUserScript.getData(key, klfoutput);
}

