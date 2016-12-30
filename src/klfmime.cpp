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
#include <QUrlQuery>
#include <QBuffer>
#include <QDir>
#include <QDomDocument>
#include <QDomNode>
#include <QDomElement>
#include <QPainter>
#include <QTextCodec>
#include <QTextDocument>

#if defined(KLF_WS_MAC)
#include <QMacPasteboardMime> // qRegisterDraggedTypes()
#elif defined(KLF_WS_WIN)
#else // X11 -- nothing required
#endif

#include <klfutil.h>
#include <klfguiutil.h>
#include <klfbackend.h>
#include <klffilterprocess.h>

#include "klfconfig.h"
#include "klflib.h"
#include "klfmime.h"
#include "klfmime_p.h"

// #define OPENOFFICE_DRAWING_MIMETYPE "application/x-openoffice-drawing;windows_formatname=\"Drawing Format\""


// ---------------------------------------------------------------------

// struct KLFMimeString
// {
//   KLFMimeString(const QString& key)
//     : mime_full(key), mime_base(), mime_attrs()
//   {
//     // see if we got any attributes to the mime-type
//     int i_semicolon = key.indexOf(';');
//     if (i_semicolon >= 0) {
//       mime_base = key.left(i_semicolon);
      
//       QString qs = key.mid(i_semicolon+1);
//       QUrl url;
//       url.setQuery(qs.toLatin1());

//       QList<QPair<QString,QString> > qitems = QUrlQuery(url).queryItems();
//       for (int i = 0; i < qitems.size(); ++i) {
//         klfDbg("got mime query item " << qitems[i].first << "=" << qitems[i].second) ;
//         mime_attrs[qitems[i].first] = qitems[i].second;
//       }

//       // QRegExp rx("(\\w+)=([^=;]*|\"(?:[^\"]|(?:\\\\.))*\")");
//       // int pos = i_semicolon+1;
//       // while ( (pos = rx.indexIn(key, pos)) >= 0 )  {
//       //   mime_attrs[rx.cap(1)] = rx.cap(2);
//       // }
//     } else {
//       mime_base = key;
//     }
//     klfDbg("KLFMimeString() constr complete, mime_full="<<mime_full<<", mime_base="
//            <<mime_base<<", mime_attrs="<<mime_attrs) ;
//   }

//   QString mime_full;
//   QString mime_base;

//   QMap<QString,QString> mime_attrs;
// };


// ---------------------------------------------------------------------


// ---------------------------------------------------------------------

QList<KLFMimeExportProfile::ExportType::DefaultQtFormat>
KLFMimeExportProfile::ExportType::defaultQtFormatsOnThisWs() const
{
  KLF_DEBUG_BLOCK(KLF_FUNC_NAME) ;
  QList<DefaultQtFormat> qtfmtlist;
  for (int kk = 0; kk < defaultQtFormats.size(); ++kk) {
    if (!defaultQtFormats[kk].onlyOnWs.size() || defaultQtFormats[kk].onlyOnWs.contains(KLF_WS)) {
      qtfmtlist << defaultQtFormats[kk];
    }
  }
  return qtfmtlist;
}


KLFMimeExportProfile::KLFMimeExportProfile(const QString& pname, const QString& desc,
					   const QList<ExportType>& exporttypes, const QString& inSubMenu)
  : p_profileName(pname), p_description(desc), p_exportTypes(exporttypes), p_inSubMenu(inSubMenu)
{
}

KLFMimeExportProfile::KLFMimeExportProfile()
  : p_profileName(), p_description(), p_exportTypes(), p_inSubMenu()
{
}

QStringList KLFMimeExportProfile::collectedAvailableMimeTypes(KLFExporterManager * exporterManager,
                                                              const KLFBackend::klfOutput & klfoutput) const
{
  KLF_DEBUG_BLOCK(KLF_FUNC_NAME) ;
  QStringList mimeTypes;
  int k;
  for (k = 0; k < p_exportTypes.size(); ++k) {
    // try to resolve exporter, and query whether it can produce data from the given output
    KLFExporter * exporter = exporterManager->exporterByName(p_exportTypes[k].exporterName) ;
    if (exporter != NULL && exporter->supports(p_exportTypes[k].exporterFormat, klfoutput)) {
      mimeTypes << p_exportTypes[k].mimeTypes;
    }
  }
  klfDbg("mime types: " << mimeTypes) ;
  return mimeTypes;
}
QStringList KLFMimeExportProfile::collectedAvailableMacFlavors(KLFExporterManager * exporterManager,
                                                               const KLFBackend::klfOutput & klfoutput) const
{
  KLF_DEBUG_BLOCK(KLF_FUNC_NAME) ;
  QStringList macFlavors;
  int k;
  for (k = 0; k < p_exportTypes.size(); ++k) {
    // try to resolve exporter, and query whether it can produce data from the given output
    KLFExporter * exporter = exporterManager->exporterByName(p_exportTypes[k].exporterName) ;
    if (exporter != NULL && exporter->supports(p_exportTypes[k].exporterFormat, klfoutput)) {
      macFlavors << p_exportTypes[k].macFlavors;
    } else {
      klfDbg("Ignoring exporter " << p_exportTypes[k].exporterName << "; exporter = " << exporter);
    }
  }
  klfDbg("mac flavors: " << macFlavors) ;
  return macFlavors;
}
QStringList KLFMimeExportProfile::collectedAvailableWinFormats(KLFExporterManager * exporterManager,
                                                               const KLFBackend::klfOutput & klfoutput) const
{
  KLF_DEBUG_BLOCK(KLF_FUNC_NAME) ;
  QStringList winFormats;
  int k;
  for (k = 0; k < p_exportTypes.size(); ++k) {
    // try to resolve exporter, and query whether it can produce data from the given output
    KLFExporter * exporter = exporterManager->exporterByName(p_exportTypes[k].exporterName) ;
    if (exporter != NULL && exporter->supports(p_exportTypes[k].exporterFormat, klfoutput)) {
      winFormats << p_exportTypes[k].winFormats;
    }
  }
  klfDbg("win formats: " << winFormats) ;
  return winFormats;
}

int KLFMimeExportProfile::indexOfAvailableMimeType(const QString& mimeType,
                                                   KLFExporterManager * exporterManager,
                                                   const KLFBackend::klfOutput & klfoutput) const
{
  KLF_DEBUG_BLOCK(KLF_FUNC_NAME) ;
  int k;
  for (k = 0; k < p_exportTypes.size(); ++k) {
    if (p_exportTypes[k].mimeTypes.contains(mimeType)) {
      // try to resolve exporter, and query whether it can produce data from the given output
      KLFExporter * exporter = exporterManager->exporterByName(p_exportTypes[k].exporterName) ;
      if (exporter != NULL && exporter->supports(p_exportTypes[k].exporterFormat, klfoutput)) {
        return k;
      }
    }
  }
  return -1;
}
int KLFMimeExportProfile::indexOfAvailableWinFormat(const QString& winFormat,
                                                    KLFExporterManager * exporterManager,
                                                    const KLFBackend::klfOutput & klfoutput) const
{
  KLF_DEBUG_BLOCK(KLF_FUNC_NAME) ;
  int k;
  for (k = 0; k < p_exportTypes.size(); ++k) {
    if (p_exportTypes[k].winFormats.contains(winFormat)) {
      // try to resolve exporter, and query whether it can produce data from the given output
      KLFExporter * exporter = exporterManager->exporterByName(p_exportTypes[k].exporterName) ;
      if (exporter != NULL && exporter->supports(p_exportTypes[k].exporterFormat, klfoutput)) {
        return k;
      }
    }
  }
  return -1;
}
int KLFMimeExportProfile::indexOfAvailableMacFlavor(const QString& macFlavor,
                                                    KLFExporterManager * exporterManager,
                                                    const KLFBackend::klfOutput & klfoutput) const
{
  KLF_DEBUG_BLOCK(KLF_FUNC_NAME) ;
  int k;
  for (k = 0; k < p_exportTypes.size(); ++k) {
    if (p_exportTypes[k].macFlavors.contains(macFlavor)) {
      KLFExporter * exporter = exporterManager->exporterByName(p_exportTypes[k].exporterName) ;
      if (exporter != NULL && exporter->supports(p_exportTypes[k].exporterFormat, klfoutput)) {
        return k;
      }
      klfDbg("Ignoring exporter " << p_exportTypes[k].exporterName << "; exporter = " << exporter);
    }
  }
  return -1;
}


// -----------------------------------------------------------------------------


struct KLFMimeExportProfileManagerPrivate
{
  KLF_PRIVATE_HEAD(KLFMimeExportProfileManager)
  {
  }

  static QList<KLFMimeExportProfile> loadProfilesFromXMLFile(const QString& fname);
  
  QList<KLFMimeExportProfile> exportProfileList;
};


KLFMimeExportProfileManager::KLFMimeExportProfileManager()
{
  KLF_INIT_PRIVATE(KLFMimeExportProfileManager) ;
}
KLFMimeExportProfileManager::~KLFMimeExportProfileManager()
{
  KLF_DELETE_PRIVATE ;
}

QList<KLFMimeExportProfile> KLFMimeExportProfileManager::exportProfileList()
{
  return d->exportProfileList;
}
void KLFMimeExportProfileManager::addExportProfile(const KLFMimeExportProfile& exportProfile)
{
  KLF_DEBUG_BLOCK(KLF_FUNC_NAME) ;
  d->exportProfileList << exportProfile;
}
void KLFMimeExportProfileManager::addExportProfiles(const QList<KLFMimeExportProfile>& exportProfiles)
{
  KLF_DEBUG_BLOCK(KLF_FUNC_NAME) ;
  d->exportProfileList << exportProfiles;
}
KLFMimeExportProfile KLFMimeExportProfileManager::findExportProfile(const QString& pname)
{
  KLF_DEBUG_BLOCK(KLF_FUNC_NAME) ;

  int k;
  for (k = 0; k < d->exportProfileList.size(); ++k) {
    if (d->exportProfileList[k].profileName() == pname) {
      return d->exportProfileList[k];
    }
  }

  // not found, return first (ie. default) export profile
  klfWarning("Export profile not found: " << pname) ;
  return d->exportProfileList.first();
}

// static
QList<KLFMimeExportProfile> KLFMimeExportProfileManager::loadKlfExportProfileList()
{
  KLF_DEBUG_BLOCK(KLF_FUNC_NAME) ;

  QList<KLFMimeExportProfile> profiles;

  // read export profiles from KLF XML files
  
  // find XML files
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
    for (k = 0; k < entrylist.size(); ++k) {
      fcandidates << d.filePath(entrylist[k]);
    }
  }

  for (k = 0; k < fcandidates.size(); ++k) {
    if (QFile::exists(fcandidates[k])) {
      profiles << KLFMimeExportProfileManagerPrivate::loadProfilesFromXMLFile(fcandidates[k]);
    }
  }

  return profiles;
}


// static
QList<KLFMimeExportProfile> KLFMimeExportProfileManagerPrivate::loadProfilesFromXMLFile(const QString& fname)
{
  KLF_DEBUG_BLOCK(KLF_FUNC_NAME) ;

  QList<KLFMimeExportProfile> profiles;

  klfDbg("Loading from file "<<fname) ;

  QFile file(fname);
  if ( ! file.open(QIODevice::ReadOnly) ) {
    klfWarning("Can't open export mime profiles XML file "<<fname<<": " <<file.errorString()<<"");
    return QList<KLFMimeExportProfile>();
  }

  QDomDocument doc("export-profile-list");
  QString errMsg; int errLine, errCol;
  bool r = doc.setContent(&file, false, &errMsg, &errLine, &errCol);
  if (!r) {
    klfWarning("Error parsing file "<<fname<<": "<<errMsg<<" at line "<<errLine<<", col "<<errCol) ;
    return QList<KLFMimeExportProfile>();
  }
  file.close();

  QDomElement root = doc.documentElement();
  if (root.nodeName() != "export-profile-list") {
    qWarning("%s: Error parsing XML for export mime profiles from file `%s': Bad root node `%s'.\n",
	     KLF_FUNC_NAME, qPrintable(fname), qPrintable(root.nodeName()));
    return QList<KLFMimeExportProfile>();
  }

  QString klfprofilesxmlversion = root.attribute("version", QString("1"));

  // we can only read files which are version == 4.0
  //
  // The version string is the KLF version at which the given XML format is introduced
  // (e.g. if a new profile type is introduced in KLF 4.1, then it keeps the compatibility
  // "4.0" except if 4.1 introduces a new XML structure.
  if (klfprofilesxmlversion != QString("4.0")) {
    klfWarning("XML profile list XML description " << fname << " is not compatible with the current "
               "XML version " << klfprofilesxmlversion) ;
    return QList<KLFMimeExportProfile>();
  }

  // read XML file
  QDomNode n;
  for (n = root.firstChild(); ! n.isNull(); n = n.nextSibling()) {
    QDomElement e = n.toElement(); // try to convert the node to an element.
    if ( e.isNull() || n.nodeType() != QDomNode::ElementNode )
      continue;
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
    QList<KLFMimeExportProfile::ExportType> exportTypes;

    QDomNode en;
    for (en = e.firstChild(); ! en.isNull(); en = en.nextSibling() ) {
      if ( en.isNull() || en.nodeType() != QDomNode::ElementNode ) {
	continue;
      }
      QDomElement ee = en.toElement();
      if ( ee.nodeName() == "description" ) {
	if (!description.isEmpty()) {
          klfWarning("Ignoring duplicate <description> tag in profile " << pname) ;
          continue;
        } else {
          description = qApp->translate("xmltr_exportprofiles", ee.text().toUtf8().constData(),
                                        "[[tag: <description>]]");
        }
	continue;
      }
      if ( ee.nodeName() == "include-export-types-from" ) {
        QString includeProfile = ee.attribute("profile", QString()) ;
        if (includeProfile.isEmpty()) {
          klfWarning("In XML file " << fname << ", profile " << pname << ": You need to "
                     << "specify a profile to include (with the syntax "
                     << "<include-export-types-from profile=\"other-profile\">)") ;
          continue;
        }
        // find profile in current profile list
        int k;
        for (k = 0; k < profiles.size(); ++k) {
          if (profiles[k].profileName() == includeProfile) {
            break; // found
          }
        }
        if (k == profiles.size()) {
          klfWarning("In XML file " << fname << ", profile " << pname << ": include-export-types-from tag"
                       << "refers to inexistant profile " << includeProfile ) ;
        } else {
          klfDbg("Adding all export types from profile " << includeProfile << ", index " << k << " in list") ;
          exportTypes << profiles[k].exportTypes() ;
        }
	continue;
      }
      if ( ee.nodeName() == "in-submenu" ) {
	inSubMenu = qApp->translate("xmltr_exportprofiles", ee.text().toUtf8().constData(),
				       "[[tag: <in-submenu>]]");
	continue;
      }
      if ( ee.nodeName() == "export-type" ) {

        if ( ! ee.hasAttribute("exporter") ) {
          klfWarning("<export-type> does not have mandatory attribute exporter=\"...\", ignoring.") ;
          continue;
        }
        if ( ! ee.hasAttribute("format") ) {
          klfWarning("<export-type> does not have mandatory attribute format=\"...\", ignoring.") ;
          continue;
        }

        KLFMimeExportProfile::ExportType exportType(ee.attribute("exporter", QString()),
                                                    ee.attribute("format", QString()));

        klfDbg("Reading <export-type exporter=" << exportType.exporterName
               << " format=" << exportType.exporterFormat << "> ...") ;

        QDomNode n2;
        for (n2 = ee.firstChild(); !n2.isNull(); n2 = n2.nextSibling()) {
          if ( n2.isNull() || n2.nodeType() != QDomNode::ElementNode ) {
            continue;
          }
          QDomElement ee2 = n2.toElement();
          if ( ee2.nodeName() == "default-qt-formats" ) {
            QString qttype = ee2.attribute("type");
            if (qttype != "color" && qttype != "html" && qttype != "image" &&
                qttype != "text" && qttype != "urls") {
              klfWarning("In XML file " << fname << ", profile " << pname << ", export-type "
                         << "(exporter="<<exportType.exporterName<<",format="<<exportType.exporterFormat<<")"
                         << ": ignoring invalid default qt format " << qttype ) ;
              continue;
            }
            QStringList only_on_list = ee2.attribute("only-on", QString("")).split(',', QString::SkipEmptyParts);
            exportType.defaultQtFormats
              << KLFMimeExportProfile::ExportType::DefaultQtFormat(qttype, only_on_list);
            klfDbg("added default-qt-format qttype="<<qttype<<", only_on_list="<<only_on_list) ;
            continue;
          }
          if ( ee2.nodeName() == "mime-type" ) {
            QString mimeType = ee2.text().trimmed();
            exportType.mimeTypes << mimeType;
            continue;
          }
          if ( ee2.nodeName() == "win-format" ) {
            QString winFormat = ee2.text().trimmed();
            exportType.winFormats << winFormat;
            continue;
          }
          if ( ee2.nodeName() == "mac-flavor" ) {
            QString macFlavor = ee2.text().trimmed();
            exportType.macFlavors << macFlavor;
            continue;
          }
          klfWarning("In XML file " << fname << ", profile " << pname << ", export-type "
                     << "(exporter="<<exportType.exporterName<<",format="<<exportType.exporterFormat<<")"
                     << ": ignoring unexpected tag " << ee2.nodeName() ) ;
        }

        exportTypes << exportType;

        continue;
      }

      klfWarning( "Parsing XML file " << fname << ": ignoring unexpected tag " << en.nodeName()
                  << " in profile " << pname ) ;
    } // for

    // add this profile
    KLFMimeExportProfile profile(pname, description, exportTypes, inSubMenu);

    // check if a profile has not already been declared with same name
    int kp;
    for (kp = 0; kp < profiles.size(); ++kp) {
      if (profiles[kp].profileName() == pname) {
        klfWarning("Ignoring duplicate definition of export profile " << pname );
	return QList<KLFMimeExportProfile>();
      }
    }
    // the profile is new
    klfDbg("Adding profile "<<pname<<" to mime export profiles") ;
    if (pname == "default") {
      // prepend default profile, so it's at beginning
      profiles.prepend(profile);
    } else {
      profiles << profile;
    }
  } // for

  return profiles;
}















// -----------------------------------------------------------------------------

struct KLFMimeDataPrivate
{
  KLF_PRIVATE_HEAD(KLFMimeData)
  {
  }
  
  KLFMimeExportProfile exportProfile;
  KLFExporterManager * exporterManager;
  KLFBackend::klfOutput output;

  QStringList qtManagedMimeTypes;

  QByteArray getDataFor(KLFMimeExportProfile::ExportType exportType, const QVariantMap & params)
  {
    KLFExporter * exporter = exporterManager->exporterByName(exportType.exporterName);
    if (exporter == NULL) {
      klfWarning("Can't find exporter name = " << exportType.exporterName << "!") ;
      return QByteArray();
    }
  
    // get the data
    return exporter->getData(exportType.exporterFormat, output, params);
  }

  void setDefaultQtFormats();

  QStringList macFlavorsToProxyMimes(const QStringList & flavlist) const;
  QString macFlavorFromProxyMime(const QString & mimeType) const;

  void ensureAllPlatformTypesRegistered();
};

KLFMimeData::KLFMimeData(const KLFMimeExportProfile& exportProfile,
                         KLFExporterManager * exporterManager,
                         const KLFBackend::klfOutput& output)
  : QMimeData()
{
  KLF_DEBUG_BLOCK(KLF_FUNC_NAME) ;

  KLF_INIT_PRIVATE(KLFMimeData) ;

  klfDbg("Using export profile "<<exportProfile.profileName()) ;

  d->exporterManager = exporterManager;
  d->output = output;

  KLF_ASSERT_NOT_NULL(d->exporterManager, "Exporter Manager is NULL!!!", std::terminate()) ;

  d->exportProfile = exportProfile;

  d->setDefaultQtFormats();

  d->ensureAllPlatformTypesRegistered();

  klfDbg("have formats: "<<formats()) ;
}
KLFMimeData::~KLFMimeData()
{
  KLF_DEBUG_BLOCK(KLF_FUNC_NAME) ;

  KLF_DELETE_PRIVATE ;
}

void KLFMimeDataPrivate::setDefaultQtFormats()
{
  KLF_DEBUG_BLOCK(KLF_FUNC_NAME) ;

  QList<KLFMimeExportProfile::ExportType> exportTypes = exportProfile.exportTypes();
  int k;
  for (k = 0; k < exportTypes.size(); ++k) {
    KLFMimeExportProfile::ExportType exportType = exportTypes[k];
    QList<KLFMimeExportProfile::ExportType::DefaultQtFormat> qtformats
      = exportType.defaultQtFormatsOnThisWs();
    klfDbg("Qt formats on this window-system for export type "<<k<<" ("<<exportType.exporterName<<") ; list size="
           << qtformats.size() ) ;

    if (!qtformats.size()) {
      continue;
    }

    // maybe we should think of a way of querying the user for export params? but this
    // should not be done here interactively, because this is called on "copy" or "drag
    // start", not upon dropped (as in retrievedData()) .... just think about this .....
    QVariantMap params;

    // this export type needs to be exported via QMimeData::setXXX(...), so we need to
    // prepare the data to set
    QByteArray data = getDataFor(exportType, params);
    int j;
    for (j = 0; j < qtformats.size(); ++j) {
      KLFMimeExportProfile::ExportType::DefaultQtFormat qtformat = qtformats[j];
      if (qtformat.qtType == "color") {
        klfWarning("Qt default format \"color\" NOT YET IMPLEMENTED!!! FIXME!! ..........") ;
        K->setColorData(QColor(0,0,0)); // dummy, there's not much to do anyway...
      } else if (qtformat.qtType == "html") {
        klfDbg("adding html data, data = " << QString::fromUtf8(data)) ;
        K->setHtml(QString::fromUtf8(data));
      } else if (qtformat.qtType == "image") {
        QImage img;
        img.loadFromData(data);
        klfDbg("adding image size="<<img.size()) ;
        K->setImageData(img);
      } else if (qtformat.qtType == "text") {
        klfDbg("adding text="<<QString::fromUtf8(data)) ;
        K->setText(QString::fromUtf8(data));
      } else if (qtformat.qtType == "urls") {
        QList<QUrl> urllist;
        QStringList urls = QString::fromUtf8(data).split('\n');
        for (int kk = 0; kk < urls.size(); ++kk) {
          urllist << QUrl(urls[kk]);
        }
        klfDbg("adding URLs = " << urllist) ;
        K->setUrls(urllist) ;
      } else {
        klfWarning("Ignoring invalid default Qt format " << qtformat.qtType) ;
      }
    }
  }

  qtManagedMimeTypes = K->QMimeData::formats();
  klfDbg("Qt managed mime types = " << qtManagedMimeTypes) ;
}

QStringList KLFMimeDataPrivate::macFlavorsToProxyMimes(const QStringList & flavlist) const
{
  KLF_DEBUG_BLOCK(KLF_FUNC_NAME) ;
  QStringList mimes;
  foreach (QString f, flavlist) {
    mimes << QString::fromUtf8(KLF_MIME_PROXY_MAC_FLAVOR_PREFIX "%1").arg(f);
  }
  return mimes;
}
QString KLFMimeDataPrivate::macFlavorFromProxyMime(const QString& proxyMime) const
{
  KLF_DEBUG_BLOCK(KLF_FUNC_NAME) ;
  const QString prefix = QLatin1String(KLF_MIME_PROXY_MAC_FLAVOR_PREFIX);
  if (proxyMime.startsWith(prefix)) {
    return proxyMime.mid(prefix.size());
  }
  klfWarning(proxyMime << " is not a mac flavor proxy mime type !") ;
  return QString();
}

void KLFMimeDataPrivate::ensureAllPlatformTypesRegistered()
{
  KLF_DEBUG_BLOCK(KLF_FUNC_NAME) ;
#if defined(KLF_WS_MAC)
  QStringList allMacFlavors;
  QList<KLFMimeExportProfile::ExportType> exportTypes = exportProfile.exportTypes();
  for (int k = 0; k < exportTypes.size(); ++k) {
    allMacFlavors << exportTypes[k].macFlavors;
  }
  klfDbg("Regstering mac flavors: " << allMacFlavors) ;
  qRegisterDraggedTypes(QStringList() << allMacFlavors);
#elif defined(KLF_WS_WIN)
  ....
#endif
}


QStringList KLFMimeData::formats() const
{
  KLF_DEBUG_BLOCK(KLF_FUNC_NAME) ;

  QStringList fmts;
#if defined(KLF_WS_MAC)
  fmts = d->macFlavorsToProxyMimes(d->exportProfile.collectedAvailableMacFlavors(
                                       d->exporterManager,
                                       d->output
                                       ));
#elif defined(KLF_WS_WIN)
  fmts = d->winFormatsToProxyMimes(d->exportProfile.collectedAvailableWinFormats(
                                       d->exporterManager,
                                       d->output
                                       ));
#else // by default, do X11  [#if defined(KLF_WS_X11)]
  fmts = pExportProfile.collectedMimeTypes(& d->output);
#endif

  klfDbg("our mime formats: " << fmts) ;

  QStringList qtfmts = QMimeData::formats();
  klfDbg("Formats added by qt: " << qtfmts) ;
  fmts << qtfmts;

  klfDbg("full format list: " << fmts) ;
  return fmts;
}

QVariant KLFMimeData::retrieveData(const QString& mimetype, QVariant::Type type) const
{
  KLF_DEBUG_BLOCK(KLF_FUNC_NAME) ;

  klfDbg("retrieveData: mimetype="<<mimetype<<"; type="<<type) ;

  if (d->qtManagedMimeTypes.contains(mimetype) || mimetype.startsWith("application/x-qt-")) {
    // this mime type is handled by Qt
    klfDbg("Letting Qt handle "<<mimetype<<" w/ type="<<type) ;
    return QMimeData::retrieveData(mimetype, type);
  }

  int index;
#if defined(KLF_WS_MAC)
  index = d->exportProfile.indexOfAvailableMacFlavor(
      d->macFlavorFromProxyMime(mimetype),
      d->exporterManager,
      d->output
      );
#elif defined(KLF_WS_WIN)
  index = d->exportProfile.indexOfAvailableWinFormat(
      d->winFormatFromProxyMime(mimetype)
      d->exporterManager,
      d->output
      );
#else // by default, do X11  [#if defined(KLF_WS_X11)]
  index = d->exportProfile.indexOfAvailableMimeType(
      mimetype,
      d->exporterManager,
      d->output
      );
#endif
  if (index < 0) {
    // do not treat this as an error since on windows we seem to have requests for 'text/uri-list' even
    // if that mime type is not returned by formats()
    klfDbg("Can't find mime-type "<<mimetype<<" in export profile "<<d->exportProfile.profileName()
	   <<" ?!?");
    return QMimeData::retrieveData(mimetype, type);
  }

  klfDbg("exporting "<<mimetype<<" ... index="<<index);

  KLFMimeExportProfile::ExportType etype = d->exportProfile.exportType(index);

  /// \todo TODO: add the query-string part of the MIME type into parameters in params for
  ///       the exporter ............... or query the user .... .......
  QVariantMap params;

  QByteArray data = d->getDataFor(etype, params);
  
  klfDbg("exported mimetype "<<mimetype<<": data length is "<<data.size());
  
  return QVariant::fromValue<QByteArray>(data);
}

































#if 0









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

  if (mime == "application/pdf")
    return "application/pdf"; // "PDF"; -- doesn't seem to work.
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
  KLF_DEBUG_BLOCK(KLF_FUNC_NAME) ;
  
  qint64 imgcachekey = output.result.cacheKey();

  QString tempfilename;

  if (targetDpi <= 0)
    targetDpi = output.input.dpi;

  if (tempFilesForImageCacheKey.contains(imgcachekey) &&
      tempFilesForImageCacheKey[imgcachekey].contains(targetDpi)) {
    klfDbg("found cached image in cache, have DPIs: " << tempFilesForImageCacheKey[imgcachekey].values()) ;
    return tempFilesForImageCacheKey[imgcachekey][targetDpi];
  }

  QString templ = klfconfig.BackendSettings.tempDir +
    QString::fromLatin1("/klf_%2_XXXXXX.%1.png")
    .arg(targetDpi).arg(QDateTime::currentDateTime().toString("yyyy-MM-dd_hh-mm"));

  klfDbg("Attempting to create temp file from template name " << templ) ;

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

  tempfilename = QFileInfo(tempfile->fileName()).absoluteFilePath();
  tempfile->close();
  // cache this temp file for other formats' use or other QMimeData instantiation...
  tempFilesForImageCacheKey[imgcachekey][targetDpi] = tempfilename;

  klfDbg("Wrote temp file with name " << tempfilename) ;

  return tempfilename;
}

QByteArray KLFMimeExporterUrilist::data(const QString& key, const KLFBackend::klfOutput& output)
{
  KLF_DEBUG_BLOCK(KLF_FUNC_NAME) ;

  klfDbg("key="<<key) ;

  int req_dpi = -1;
  KLFMimeString mimestr(key);
  klfDbg("parsing mime attributes " << mimestr.mime_attrs) ;
  for (QMap<QString,QString>::const_iterator it = mimestr.mime_attrs.begin(); it != mimestr.mime_attrs.end(); ++it) {
    const QString& attr_key = it.key();
    const QString& attr_val = it.value();
    klfDbg("parsing attr_key = " << attr_key << " ; attr_val = " << attr_val) ;
    if (attr_key == "dpi") {
      req_dpi = attr_val.toInt();
    } else {
      klfWarning("Unrecognized attribute in mime-type: "<<attr_key<<" in "<<key);
    }
  }

  klfDbg("req_dpi = " << req_dpi) ;

  QString tempfilename = tempFileForOutput(output, req_dpi);
  QUrl url = QUrl::fromLocalFile(tempfilename);
#ifdef KLF_WS_MAC
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
  KLF_DEBUG_BLOCK(KLF_FUNC_NAME) ;
  
  qint64 cachekey = qHash(output.pdfdata);

  if (output.pdfdata.isEmpty()) {
    klfWarning("Request to export as PDF but we have no data!");
    return QString();
  }

  if (tempFilesForImageCacheKey.contains(cachekey)) {
    klfDbg("found cached image in " << tempFilesForImageCacheKey.values()) ;
    return tempFilesForImageCacheKey[cachekey];
  }

  QString templ = klfconfig.BackendSettings.tempDir +
    QString::fromLatin1("/klf_%2_XXXXXX.pdf")
    .arg(QDateTime::currentDateTime().toString("yyyy-MM-dd_hh-mm"));

  klfDbg("Attempting to create temp file from template name " << templ) ;

  QTemporaryFile *tempfile = new QTemporaryFile(templ, qApp);
  tempfile->setAutoRemove(true); // will be deleted when klatexformula exists (= qApp destroyed)
  if (tempfile->open() == false) {
    klfWarning("Can't open temp pdf file for mimetype text/uri-list: template is "<<templ);
    return QString();
  }

  tempfile->write(output.pdfdata);

  QString tempfilename = tempfile->fileName();
  tempfile->close();

  klfDbg("Wrote temp file with name " << tempfilename) ;

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
#ifdef KLF_WS_MAC
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

#ifdef KLF_WS_MAC
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





#endif // cut out implementations of KLF****MimeExporters



