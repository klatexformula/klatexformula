/***************************************************************************
 *   file klfmime.cpp
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

#include <klfbackend.h>

#include <klfguiutil.h>
#include "klfconfig.h"
#include "klflib.h"
#include "klfmime.h"
#include "klfmime_p.h"

#ifdef Q_WS_MAC
#include "macosx/klfmacclipboard.h"
#endif

#define OPENOFFICE_DRAWING_MIMETYPE "application/x-openoffice-drawing;windows_formatname=\"Drawing Format\""

#ifndef Q_WS_MAC
// provide empty non-functional function so that it can be called in any
// context (also on linux, win: don't need #ifdef Q_WS_MAC)
inline void __klf_init_the_macpasteboardmime() { }
#endif

// ---------------------------------------------------------------------

bool KLFMimeExporter::supportsKey(const QString& key) const
{
  bool result =  (keys().indexOf(key) >= 0) ;
  klfDbg("key = "<<key<<" ; result="<<result) ;
  return result;
}

// static
KLFMimeExporter * KLFMimeExporter::mimeExporterLookup(const QString& key)
{
  KLF_DEBUG_BLOCK(KLF_FUNC_NAME) ;
  initMimeExporterList();

  int k;
  for (k = 0; k < p_mimeExporterList.size(); ++k) {
    klfDbg("Testing exporter #"<<k<<": "<<p_mimeExporterList[k]);
    klfDbg("\t: "<<p_mimeExporterList[k]->exporterName()) ;
    if (p_mimeExporterList[k]->supportsKey(key))
      return p_mimeExporterList[k];
  }

  // no exporter found.
  return NULL;
}
// static
KLFMimeExporter * KLFMimeExporter::mimeExporterLookupByName(const QString& exporter, const QString& key)
{
  KLF_DEBUG_BLOCK(KLF_FUNC_NAME) ;
  initMimeExporterList();

  int k;
  for (k = 0; k < p_mimeExporterList.size(); ++k)
    if (p_mimeExporterList[k]->exporterName() == exporter) // find the given 'exporter'
      if (key.isEmpty() || p_mimeExporterList[k]->supportsKey(key)) // check for 'key' support
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
void KLFMimeExporter::initMimeExporterList()
{
  KLF_DEBUG_BLOCK(KLF_FUNC_NAME) ;
  if (p_mimeExporterList.isEmpty()) {
    // ensure an instance of KLFMacPasteboardMime object
    __klf_init_the_macpasteboardmime();
    p_mimeExporterList
      << new KLFMimeExporterImage(qApp)
      << new KLFMimeExporterUrilist(qApp)
      << new KLFMimeExporterHTML(qApp)
      << new KLFMimeExporterLibFmts(qApp)
      << new KLFMimeExporterGlowImage(qApp)
      ;
  }
}

// static
QList<KLFMimeExporter*> KLFMimeExporter::p_mimeExporterList = QList<KLFMimeExporter*>();

// ---------------------------------------------------------------------

KLFMimeExportProfile::KLFMimeExportProfile(const QString& pname, const QString& desc,
					   const QList<ExportType>& exporttypes)
  : p_profileName(pname), p_description(desc), p_exportTypes(exporttypes)
{
}

KLFMimeExportProfile::KLFMimeExportProfile(const KLFMimeExportProfile& o)
  : p_profileName(o.p_profileName), p_description(o.p_description),
    p_exportTypes(o.p_exportTypes)
{
}

KLFMimeExporter * KLFMimeExportProfile::exporterLookupFor(int k, bool warnNotFound) const
{
  KLF_DEBUG_BLOCK(KLF_FUNC_NAME) ;

  KLF_ASSERT_CONDITION(k >= 0 && k < p_exportTypes.size(),
		       "Index "<<k<<" out of bounds (size="<<p_exportTypes.size()<<")",
		       return NULL ) ;

  KLFMimeExporter * exporter = NULL;
  if ( ! p_exportTypes[k].exporter.isEmpty() ) {
    // lookup the exporter by name, and make sure that it supports the 'mimetype' key
    exporter = KLFMimeExporter::mimeExporterLookupByName(p_exportTypes[k].exporter, p_exportTypes[k].mimetype);
  } else {
    // lookup the exporter by mime-type
    exporter = KLFMimeExporter::mimeExporterLookup(p_exportTypes[k].mimetype);
  }

  if (warnNotFound)
    KLF_ASSERT_NOT_NULL(exporter,
			"Can't find exporter "<<p_exportTypes[k].exporter<<" for export-type #"<<k
			<<"for key "<<p_exportTypes[k].mimetype,   return NULL ) ;
  return exporter;
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

  KLFMimeExporter *exporter = exporterLookupFor(k, true);
  if (exporter == NULL)
    return QString();

  return exporter->windowsFormatName(p_exportTypes[k].mimetype);
}

QStringList KLFMimeExportProfile::availableExporterMimeTypes() const
{
  KLF_DEBUG_BLOCK(KLF_FUNC_NAME) ;

  QStringList oktypes;
  int k;
  for (k = 0; k < p_exportTypes.size(); ++k) {
    if (exporterLookupFor(k, false) != NULL)
      oktypes << p_exportTypes[k].mimetype;
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
  QStringList mimetypes = imgexporter.keys();
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
	  if (lang.isEmpty() || lang.startsWith(klfconfig.UI.locale) || klfconfig.UI.locale.startsWith(lang)) {
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
	  if ( (lang.startsWith(klfconfig.UI.locale) || klfconfig.UI.locale.startsWith(lang)) &&
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
    KLFMimeExportProfile profile(pname, description, exporttypes);

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

  // ensure an instance of KLFMacPasteboardMime object
  __klf_init_the_macpasteboardmime();

  pOutput = output;

  set_possible_qt_image_data();
}
KLFMimeData::~KLFMimeData()
{
  KLF_DEBUG_BLOCK(KLF_FUNC_NAME) ;
}

// private
void KLFMimeData::set_possible_qt_image_data()
{
  KLF_DEBUG_BLOCK(KLF_FUNC_NAME) ;

  // Handle platform-specific default image type with 'application/x-qt-image' and setImage()
  int index;
  if ( (index=pExportProfile.indexOfMimeType(QLatin1String("application/x-qt-image"))) >= 0) {
    // set the image data form the exporter
    KLFMimeExporter * exporter = pExportProfile.exporterLookupFor(index);
    if (exporter != NULL) {
      QByteArray img_data = exporter->data(QLatin1String("application/x-qt-image"), pOutput);
      QImage img;
      QList<QByteArray> try_formats = QList<QByteArray>() << "PNG" << "JPEG" << "BMP";
      int k;
      for (k = 0; k < try_formats.size() && img.isNull(); ++k)
	img.loadFromData(img_data, try_formats[k].constData());
      KLF_ASSERT_CONDITION( ! img.isNull() ,
			    "Can't get image for application/x-qt-image for profile "<<pExportProfile.profileName(),
			    return; )  ;
      setImageData(img);
    }
  }
}

QStringList KLFMimeData::formats() const
{
  KLF_DEBUG_BLOCK(KLF_FUNC_NAME) ;

  QStringList fmts = pExportProfile.availableExporterMimeTypes();
  if (fmts.contains("application/x-qt-image")) {
    if (pQtOwnedFormats.size() == 0)
      pQtOwnedFormats = QMimeData::formats();
    fmts << pQtOwnedFormats;
  }

  klfDbg("format list: "<<fmts) ;
  return fmts;
}

QVariant KLFMimeData::retrieveData(const QString& mimetype, QVariant::Type type) const
{
  KLF_DEBUG_BLOCK(KLF_FUNC_NAME) ;

  if (mimetype == QLatin1String("application/x-qt-image") ||
      pQtOwnedFormats.contains(mimetype))
    return QMimeData::retrieveData(mimetype, type);

  int index = pExportProfile.indexOfMimeType(mimetype);
  if (index < 0) {
    // do not treat this as an error since on windows we seem to have requests for 'text/uri-list' even
    // if that mime type is not returned by formats()
    klfDbg("Can't find mime-type "<<mimetype<<" in export profile "<<pExportProfile.profileName()
	   <<" ?!?");
    return QVariant();
  }

  klfDbg("exporting "<<mimetype<<" ...");
  KLFMimeExporter *exporter = pExportProfile.exporterLookupFor(index);

  KLF_ASSERT_NOT_NULL(exporter,
		      "Can't find an exporter for mime-type "<<mimetype<<"." ,
		      return QVariant(); )  ;
  
  // get the data
  QByteArray data = exporter->data(mimetype, pOutput);
  
  klfDbg("exporting mimetype "<<mimetype<<": data length is "<<data.size());
  
  return QVariant::fromValue<QByteArray>(data);
}




// ---------------------------------------------------------------------

static QMap<QString,QByteArray> get_qt_image_formats()
{
  KLF_DEBUG_BLOCK(KLF_FUNC_NAME) ;

  QMap<QString,QByteArray> imageFormats;
  // get qt's image formats
  QList<QByteArray> qtimgfmts = QImageWriter::supportedImageFormats();
  int k;
  for (k = 0; k < qtimgfmts.size(); ++k) {
    if ( imageFormats.key(qtimgfmts[k]).isEmpty() ) {
      QString mime = QString::fromLatin1("image/")+QString::fromLatin1(qtimgfmts[k]).toLower();
      imageFormats[mime] = qtimgfmts[k];
    }
  }
  return imageFormats;
}

QMap<QString,QByteArray> KLFMimeExporterImage::imageFormats = QMap<QString,QByteArray>();

QStringList KLFMimeExporterImage::keys() const
{
  KLF_DEBUG_BLOCK(KLF_FUNC_NAME) ;

  // image formats that are always supported. Qt image formats are added too.
  static QStringList staticKeys
    = QStringList() << "image/png" << "image/eps" << "application/eps" << "application/postscript"
		    << OPENOFFICE_DRAWING_MIMETYPE << "application/x-qt-image"
		    // add duplicate for png, see below
		    << "image/x-win-png-office-art";

  if (imageFormats.isEmpty()) {
    // populate qt's image formats
    QMap<QString,QByteArray> ifmts = get_qt_image_formats();
    QMap<QString,QByteArray>::iterator it;
    for (it = ifmts.begin(); it != ifmts.end(); ++it) {
      QString mime = it.key();
      QByteArray qtfmt = it.value();
      // add this image format, if not already provided by staticKeys
      if (staticKeys.indexOf(mime) == -1)
	imageFormats[mime] = qtfmt;
      // add duplicate mime types for some formats, to be able to specify multiple windows format
      // names for them, eg. "Bitmap" and "Windows Bitmap" :
      if (mime == "image/bmp") {
	imageFormats["image/x-win-bmp"] = qtfmt;
      } else if (mime == "image/jpeg") {
	imageFormats["image/x-win-jfif"] = qtfmt;
	imageFormats["image/x-win-jfif-office-art"] = qtfmt;
      } else if (mime == "image/png") {
	imageFormats["image/x-win-png-office-art"] = qtfmt;
      }
    }
  }

  QStringList keys = staticKeys;

  if (!klfconfig.BackendSettings.execEpstopdf.isEmpty())
    keys <<"application/pdf"; // add PDF only if we have PDF

  keys << imageFormats.keys();

  return keys;
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

  QString key = keymime;
  klfDbg("key="<<key);

  if (key == "image/png")
    return klfoutput.pngdata;
  if (key == "image/eps" || key == "application/eps" || key == "application/postscript")
    return klfoutput.epsdata;
  if (key == "application/pdf") {
#ifdef KLF_DEBUG
    if (klfoutput.pdfdata.isEmpty())
      klfDbg("---warning: don't have PDF data ---") ;
#endif
    return klfoutput.pdfdata;
  }
  if (key == OPENOFFICE_DRAWING_MIMETYPE)
    return klf_openoffice_drawing(klfoutput);
  if (key == "application/x-qt-image")
    return klfoutput.pngdata;

  // rely on qt's image saving routines for other formats
  klfDbg("Will use Qt's image format exporting");
  
  if ( ! imageFormats.contains(key) )
    return QByteArray();

  QByteArray imgdata;
  QBuffer imgdatawriter(&imgdata);
  imgdatawriter.open(QIODevice::WriteOnly);
  klfoutput.result.save(&imgdatawriter, imageFormats[key]);
  imgdatawriter.close();

  klfDbg("got data: size="<<imgdata.size());
  return imgdata;
}



// ---------------------------------------------------------------------

QMap<qint64,QString> KLFMimeExporterUrilist::tempFilesForImageCacheKey = QMap<qint64,QString>();

QStringList KLFMimeExporterUrilist::keys() const
{
  KLF_DEBUG_BLOCK(KLF_FUNC_NAME) ;
  return QStringList() << "text/x-moz-url" << "text/uri-list";
}

// static
QString KLFMimeExporterUrilist::tempFileForOutput(const KLFBackend::klfOutput& output)
{
  qint64 imgcachekey = output.result.cacheKey();

  QString tempfilename;

  if (tempFilesForImageCacheKey.contains(imgcachekey)) {
    tempfilename = tempFilesForImageCacheKey[imgcachekey];
  } else {
    QString templ = klfconfig.BackendSettings.tempDir +
      QString("/klf_%1_XXXXXX.png").arg(QDateTime::currentDateTime().toString("yyyy-MM-dd_hh-mm"));
    QTemporaryFile *tempfile = new QTemporaryFile(templ, qApp);
    tempfile->setAutoRemove(true); // will be deleted when klatexformula exists (= qApp destroyed)
    if (tempfile->open() == false) {
      qWarning("Can't open temp png file for mimetype text/uri-list: template is %s",
	       qPrintable(templ));
      return QByteArray();
    } else {
      QString errStr;
      bool res = KLFBackend::saveOutputToFile(output, tempfile->fileName(), "PNG", &errStr);
      if (!res) {
	qWarning()<<KLF_FUNC_NAME<<": Can't save to temp file "<<tempfile->fileName()<<": "<<errStr;
      } else {
	tempfilename = tempfile->fileName();
	tempfile->write(output.pngdata);
	tempfile->close();
	// cache this temp file for other formats' use or other QMimeData instantiation...
	tempFilesForImageCacheKey[imgcachekey] = tempfilename;
      }
    }
  }

  return tempfilename;
}

QByteArray KLFMimeExporterUrilist::data(const QString& key, const KLFBackend::klfOutput& output)
{
  KLF_DEBUG_BLOCK(KLF_FUNC_NAME) ;

  Q_UNUSED(key) ;
  klfDbg("key="<<key) ;

  QString tempfilename = tempFileForOutput(output);
  QByteArray urilist = (QUrl::fromLocalFile(tempfilename).toString()+QLatin1String("\n")).toLatin1();
  return urilist;
}

QString KLFMimeExporterUrilist::windowsFormatName(const QString& mime) const
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

QStringList KLFMimeExporterHTML::keys() const
{
  return QStringList() << QLatin1String("text/html");
}

QByteArray KLFMimeExporterHTML::data(const QString& key, const KLFBackend::klfOutput& klfoutput)
{
  QString fname = KLFMimeExporterUrilist::tempFileForOutput(klfoutput);

  QSize imgsize = klfoutput.result.size();
  int imgDpi = klfoutput.input.dpi;
  int dispDpi = 100;

  QString latex = klfoutput.input.latex;
  // remove initial comments from latex code...
  QStringList latexlines = latex.split("\n");
  while (latexlines.size() && QRegExp("\\s*\\%.*").exactMatch(latexlines[0]))
    latexlines.removeAt(0);
  latex = latexlines.join("\n");

  QString fn = toAttrTextS(fname);
  QString l = toAttrTextS(latex);
  fn.replace("\"", "&#34;");
  l.replace("\"", "&#34;");
  QString w = QString::number((int)(1.5 * imgsize.width() * dispDpi/imgDpi));
  QString h = QString::number((int)(1.5 * imgsize.height() * dispDpi/imgDpi));
  QString win = QString::number(1.5 * imgsize.width() / imgDpi);
  QString hin = QString::number(1.5 * imgsize.height() / imgDpi);

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

  // make the equations larger, so it is not too cramped up
  const double DPI_FACTOR = 1.6;
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



QStringList KLFMimeExporterLibFmts::keys() const
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

QStringList KLFMimeExporterGlowImage::keys() const
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



