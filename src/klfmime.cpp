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

#include <QApplication>
#include <QMap>
#include <QCryptographicHash>
#include <QUrl>
#include <QBuffer>

#include "klfconfig.h"
#include "klfmime.h"



// ---------------------------------------------------------------------

bool KLFMimeExporter::supportsKey(const QString& key) const
{
  return  (keys().indexOf(key) >= 0) ;
}

// static
KLFMimeExporter * KLFMimeExporter::mimeExporterLookup(const QString& key)
{
  initMimeExporterList();

  int k;
  for (k = 0; k < p_mimeExporterList.size(); ++k)
    if (p_mimeExporterList[k]->supportsKey(key))
      return p_mimeExporterList[k];

  // no exporter found.
  return NULL;
}
  
// static
QList<KLFMimeExporter *> KLFMimeExporter::mimeExporterList()
{
  initMimeExporterList();
  return p_mimeExporterList;
}

// static
void KLFMimeExporter::addMimeExporter(KLFMimeExporter *exporter, bool overrides)
{
  initMimeExporterList();
  if (overrides)
    p_mimeExporterList.push_front(exporter);
  else
    p_mimeExporterList.push_back(exporter);
}

// static
void KLFMimeExporter::initMimeExporterList()
{
  if (p_mimeExporterList.isEmpty())
    p_mimeExporterList
      << new KLFMimeExporterImage(qApp)
      << new KLFMimeExporterUrilist(qApp)
      ;
}

// static
QList<KLFMimeExporter*> KLFMimeExporter::p_mimeExporterList = QList<KLFMimeExporter*>();

// ---------------------------------------------------------------------

KLFMimeExportProfile::KLFMimeExportProfile(const QString& pname, const QString& desc,
					   const QStringList& mtypes,
					   const QStringList& wintypes)
  : p_profileName(pname), p_description(desc), p_mimeTypes(mtypes),
    p_respectiveWinTypes(wintypes)
{
  // fill missing respective win types with empty strings
  while (p_respectiveWinTypes.size() < p_mimeTypes.size())
    p_respectiveWinTypes << QString();

  // remove any extra win types that should be there
  while (p_respectiveWinTypes.size() > p_mimeTypes.size()) {
    qWarning("Removed respWinType=%s (list sizes unequal) in export profile %s",
	     qPrintable(p_respectiveWinTypes.last()), qPrintable(pname));
    p_respectiveWinTypes.removeLast();
  }

}

KLFMimeExportProfile::KLFMimeExportProfile(const KLFMimeExportProfile& o)
  : p_profileName(o.p_profileName), p_description(o.p_description),
    p_mimeTypes(o.p_mimeTypes), p_respectiveWinTypes(o.p_respectiveWinTypes)
{
}


QString KLFMimeExportProfile::respectiveWinType(int k) const
{
  if (k < 0 || k >= p_respectiveWinTypes.size())
    return QString();

  if ( ! p_respectiveWinTypes[k].isEmpty() )
    return p_respectiveWinTypes[k];
  return p_mimeTypes[k];
}


QList<KLFMimeExportProfile> KLFMimeExportProfile::p_exportProfileList = QList<KLFMimeExportProfile>();

// static
QList<KLFMimeExportProfile> KLFMimeExportProfile::exportProfileList()
{
  initExportProfileList();
  return p_exportProfileList;
}
// static
void KLFMimeExportProfile::addExportProfile(const KLFMimeExportProfile& exportProfile)
{
  initExportProfileList();
  p_exportProfileList.push_front(exportProfile);
}
void KLFMimeExportProfile::initExportProfileList()
{
  if ( ! p_exportProfileList.isEmpty())
    return;

  p_exportProfileList
    << KLFMimeExportProfile(QObject::tr("Default", "[[mime export profile]]"),
			    QObject::tr("Default export profile", "[[mime export profile]]"),
			    QStringList() << "image/png" << "text/x-moz-url" << "text/uri-list",
			    QStringList() << "PNG" << "FileName" << QString()
			    )
    << KLFMimeExportProfile(QObject::tr("PNG only", "[[mime export profile]]"),
			    QObject::tr("Export Profile with PNG only", "[[mime export profile]]"),
			    QStringList() << "image/png",
			    QStringList() << "PNG"
			    )  ;
  // create a temp exporter on the stack to see which keys it supports, and add all avail formats
  QStringList mimetypes;
  QStringList wintypes;
  KLFMimeExporterImage imgexporter(NULL);
  mimetypes << imgexporter.keys();
  int k;
  for (k = 0; k < mimetypes.size(); ++k) {
    QString mime = mimetypes[k];
    QString wtype;
    if (mime == "application/pdf")
      wintypes << "PDF";
    else if (mime == "image/png")
      wintypes << "PNG";
    else if (mime == "image/bmp")
      wintypes << "Bitmap";
    else
      wintypes << mime;
  }
  KLFMimeExportProfile allfmts
    = KLFMimeExportProfile(QObject::tr("All Image Formats", "[[mime export profile]]"),
			   QObject::tr("Exports All Available Qt Image Formats"),
			   mimetypes, wintypes);
  p_exportProfileList << allfmts;

}


// ---------------------------------------------------------------------

QMap<QString,QByteArray> KLFMimeExporterImage::imageFormats = QMap<QString,QByteArray>();

QStringList KLFMimeExporterImage::keys() const
{
  // image formats that are always supported. Qt image formats are added too.
  static QStringList staticKeys
    = QStringList() << "image/png" << "image/eps" << "application/eps" << "application/postscript"
		    << "application/pdf";

  if (imageFormats.isEmpty()) {
    // populate qt's image formats
    QList<QByteArray> qtimgfmts = QImageWriter::supportedImageFormats();
    int k;
    for (k = 0; k < qtimgfmts.size(); ++k) {
      if ( imageFormats.key(qtimgfmts[k]).isEmpty() ) {
	QString mime = QString::fromLatin1("image/")+QString::fromLatin1(qtimgfmts[k]).toLower();
	// add this image format, if not already provided by staticKeys
	if (staticKeys.indexOf(mime) == -1)
	  imageFormats[mime] = qtimgfmts[k];
      }
    }
  }
  return staticKeys << imageFormats.keys();
}

QByteArray KLFMimeExporterImage::data(const QString& key, const KLFBackend::klfOutput& klfoutput)
{
  if (key == "image/png")
    return klfoutput.pngdata;
  if (key == "image/eps" || key == "application/eps" || key == "application/postscript")
    return klfoutput.epsdata;
  if (key == "application/pdf")
    return klfoutput.pdfdata;

  // rely on qt's image saving routines for other formats

  if ( ! imageFormats.contains(key) )
    return QByteArray();

  QByteArray imgdata;
  QBuffer imgdatawriter(&imgdata);
  imgdatawriter.open(QIODevice::WriteOnly);
  klfoutput.result.save(&imgdatawriter, imageFormats[key]);
  imgdatawriter.close();
  return imgdata;
}



// ---------------------------------------------------------------------

QMap<QByteArray,QString> KLFMimeExporterUrilist::tempFilesForImageMD5 = QMap<QByteArray,QString>();

QStringList KLFMimeExporterUrilist::keys() const
{
  return QStringList() << "text/x-moz-url" << "text/uri-list";
}

QByteArray KLFMimeExporterUrilist::data(const QString& /*key*/, const KLFBackend::klfOutput& output)
{
  // make an MD5-hash of the image to uniquely identify this output
  QByteArray imgmd5 = QCryptographicHash::hash(output.pngdata, QCryptographicHash::Md5);

  QString tempfilename;

  if (tempFilesForImageMD5.contains(imgmd5)) {
    tempfilename = tempFilesForImageMD5[imgmd5];
  } else {
    QString templ = klfconfig.BackendSettings.tempDir+"/klf_temp_export_XXXXXX.png";
    QTemporaryFile *tempfile = new QTemporaryFile(templ, qApp);
    if (tempfile->open() == false) {
      qWarning("Can't open temp png file for mimetype text/uri-list: template is %s",
	       qPrintable(templ));
      return QByteArray();
    } else {
      tempfilename = tempfile->fileName();
      tempfile->write(output.pngdata);
      tempfile->close();
    }
    // cache this temp file for other formats' use
    tempFilesForImageMD5[imgmd5] = tempfilename;
  }

  QByteArray urilist = (QUrl::fromLocalFile(tempfilename).toString()+QLatin1String("\n")).toLatin1();
  return urilist;
}







