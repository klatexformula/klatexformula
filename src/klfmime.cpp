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
#include <QCryptographicHash>
#include <QUrl>
#include <QBuffer>
#include <QDir>
#include <QDomDocument>
#include <QDomNode>
#include <QDomElement>

#include <klfbackend.h>

#include "klfconfig.h"
#include "klflib.h"
#include "klfmime.h"


#define OPENOFFICE_DRAWING_MIMETYPE "application/x-openoffice-drawing;windows_formatname=\"Drawing Format\""


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
      << new KLFMimeExporterLibFmts(qApp)
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
  ensureLoadedExportProfileList();
  return p_exportProfileList;
}
// static
void KLFMimeExportProfile::addExportProfile(const KLFMimeExportProfile& exportProfile)
{
  ensureLoadedExportProfileList();
  p_exportProfileList.push_front(exportProfile);
}
// static
KLFMimeExportProfile KLFMimeExportProfile::findExportProfile(const QString& pname)
{
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
  if ( ! p_exportProfileList.isEmpty())
    return;

  // read export profile list from XML
  
  // find correct XML file
  QStringList fcandidates;

  QStringList dirs;
  dirs << klfconfig.homeConfigDir  + "/data/export_mime_profiles.d"
       << klfconfig.globalShareDir + "/data/export_mime_profiles.d"
       << ":/data/export_mime_profiles.d";
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
  QStringList mimetypes;
  QStringList wintypes;
  KLFMimeExporterImage imgexporter(NULL);
  mimetypes << imgexporter.keys();
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
  KLFMimeExportProfile allimgfmts
    = KLFMimeExportProfile("all_image_formats",
			   QObject::tr("All Available Image Formats"),
			   mimetypes, wintypes);
  p_exportProfileList << allimgfmts;

}


// static, private
void KLFMimeExportProfile::loadFromXMLFile(const QString& fname)
{
  int k;

  klfDbg("Loading from file "<<fname<<"; our locale is "<<klfconfig.UI.locale) ;

  QFile file(fname);
  if ( ! file.open(QIODevice::ReadOnly) ) {
    qWarning()<<KLF_FUNC_NAME<<": Error: Can't open export mime profiles XML file "<<fname<<": "<<file.errorString()<<"!";
    return;
  }

  QDomDocument doc("export-profile-list");
  doc.setContent(&file);
  file.close();

  QDomElement root = doc.documentElement();
  if (root.nodeName() != "export-profile-list") {
    qWarning("%s: Error parsing XML for export mime profiles from file `%s': Bad root node.\n",
	     KLF_FUNC_NAME, qPrintable(fname));
    return;
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
    // check that name is unique
    bool pname_ok = true;
    for (k = 0; k < p_exportProfileList.size(); ++k) {
      if (p_exportProfileList[k].profileName() == pname) {
	qWarning()<<KLF_FUNC_NAME<<": file "<<fname<<": profile with name "<<pname<<" already declared! Ignoring.";
	pname_ok = false;
	break;
      }
    }
    if (!pname_ok)
      continue;
    
    QString description;
    QString curDescriptionLang;
    QStringList mimetypes;
    QStringList wintypes;

    QDomNode en;
    for (en = e.firstChild(); ! en.isNull(); en = en.nextSibling() ) {
      if ( en.isNull() || en.nodeType() != QDomNode::ElementNode )
	continue;
      QDomElement ee = en.toElement();
      if ( en.nodeName() == "description" ) {
	QString lang = ee.hasAttribute("xml:lang") ? ee.attribute("xml:lang") : QString() ;
	/* // DEBUG -->
	klfDbg("got <description>:"); 
	QDomNamedNodeMap attrlist = ee.attributes();
	int kk;
	for (kk = 0; kk < attrlist.size(); ++kk) {
	  klfDbg("description attribute: "<<attrlist.item(kk).toAttr().name()<<" : "<<attrlist.item(kk).toAttr().value());
	}
	// <-- */
	//	klfDbg("<description>: lang="<<lang<<"; hasAttribute(xml:lang)="<<ee.hasAttribute("xml:lang"));
	if (description.isEmpty()) {
	  // no description yet
	  if (lang.isEmpty() || lang.startsWith(klfconfig.UI.locale) || klfconfig.UI.locale.startsWith(lang)) {
	    // correct locale
	    //	    klfDbg("remembering description tag with lang="<<lang);
	    description = ee.text();
	  }
	  // otherwise skip this tag
	} else {
	  // see if this locale is correct and more specific
	  if ( (lang.startsWith(klfconfig.UI.locale) || klfconfig.UI.locale.startsWith(lang)) &&
	       (curDescriptionLang.isEmpty() || lang.startsWith(curDescriptionLang) ) ) {
	    // then keep it and replace the other
	    //	    klfDbg("remembering description tag with lang="<<lang);
	    description = ee.text();
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
      QDomNodeList wintypetags = ee.elementsByTagName("windows-type");
      if (mimetypetags.size() != 1) {
	qWarning()<<KLF_FUNC_NAME<<": in XML file "<<fname<<", profile "<<pname
		  <<": exactly ONE <mime-type> tag must be present in each <export-type>...</export-type>.";
	continue;
      }
      if (wintypetags.size() != 1) {
	qWarning()<<KLF_FUNC_NAME<<": in XML file "<<fname<<", profile "<<pname
		  <<": exactly ONE <windows-type> tag must be present in each <export-type>...</export-type>.";
	continue;
      }
      mimetypes << mimetypetags.at(0).toElement().text().trimmed();
      wintypes << wintypetags.at(0).toElement().text().trimmed();
    }

    // add this profile
    KLFMimeExportProfile profile(pname, description, mimetypes, wintypes);
    
    if (pname == "default") {
      // prepend default profile, so it's at beginning
      p_exportProfileList.prepend(profile);
    } else {
      p_exportProfileList << profile;
    }
  }
}



// ---------------------------------------------------------------------

QMap<QString,QByteArray> KLFMimeExporterImage::imageFormats = QMap<QString,QByteArray>();

QStringList KLFMimeExporterImage::keys() const
{
  // image formats that are always supported. Qt image formats are added too.
  static QStringList staticKeys
    = QStringList() << "image/png" << "image/eps" << "application/eps" << "application/postscript"
		    << "application/pdf" << OPENOFFICE_DRAWING_MIMETYPE;

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

QByteArray klf_openoffice_drawing(const KLFBackend::klfOutput& klfoutput);

QByteArray KLFMimeExporterImage::data(const QString& key, const KLFBackend::klfOutput& klfoutput)
{
  klfDbg("key="<<key);

  if (key == "image/png")
    return klfoutput.pngdata;
  if (key == "image/eps" || key == "application/eps" || key == "application/postscript")
    return klfoutput.epsdata;
  if (key == "application/pdf")
    return klfoutput.pdfdata;
  if (key == OPENOFFICE_DRAWING_MIMETYPE)
    return klf_openoffice_drawing(klfoutput);

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





// -----------------------------------


static QByteArray toAttrText(const QString& sbase)
{
  QString s = sbase; // we need a non-const string to .replace() on
  klfDbg("s="<<s);
  QRegExp replaceCharsRX("([^a-zA-Z0-9 _-])");
  int pos = 0;
  while ((pos = replaceCharsRX.indexIn(s, pos)) != -1) {
    QString entity = "&#x"+QString::number(replaceCharsRX.cap(1)[0].unicode(), 16).toUpper()+";" ;
    klfDbg("replacing char at pos="<<pos<<" by entity="<<entity<<": s(pos...pos+5)="<<s.mid(pos,5));
    s.replace(pos, replaceCharsRX.matchedLength(), entity);
    pos += entity.length();
  }
  klfDbg("final string: "<<s);
  return s.toUtf8();

  //  QString s2 = sbase;
  //  return s2.replace("&", "&amp;").replace("\"", "&quot;").replace("'", "&apos;").replace("<", "&lt")
  //    .replace(">", "&gt;").replace("\r", "&#xD;").replace("\t", "&#x9;").replace("\n", "&#xA;")
  //    .replace("!", "&#x21;").toUtf8();
}


QByteArray klf_openoffice_drawing(const KLFBackend::klfOutput& klfoutput)
{
  QByteArray pngdata;
  QBuffer buf(&pngdata);
  buf.open(QIODevice::WriteOnly);
  QString errstr;
  bool r = KLFBackend::saveOutputToDevice(klfoutput, &buf, "PNG", &errstr);
  if (!r) {
    qWarning()<<KLF_FUNC_NAME<<": Error: "<<errstr;
    return QByteArray();
  }

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

  // make the equationn larger, so it is not too cramped up
  const double DPI_FACTOR = 2.0;
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
  klfDbg("()");
  klfDbg("list of keys: "<<KLFAbstractLibEntryMimeEncoder::allEncodingMimeTypes());
  return KLFAbstractLibEntryMimeEncoder::allEncodingMimeTypes();
}

QByteArray KLFMimeExporterLibFmts::data(const QString& key, const KLFBackend::klfOutput& output)
{
  klfDbg("key="<<key);
  KLFAbstractLibEntryMimeEncoder *encoder =
    KLFAbstractLibEntryMimeEncoder::findEncoderFor(key, true);
  if (encoder == NULL) {
    // warning already issued in findEncoderFor(.., TRUE)
    return QByteArray();
  }

  KLFLibEntry e = KLFLibEntry(output.input.latex, QDateTime::currentDateTime(),
			      output.result, KLFStyle(output.input));

  QByteArray data = encoder->encodeMime(KLFLibEntryList()<<e, QVariantMap(), key);

  if (!data.size())
    qWarning()<<KLF_FUNC_NAME<<": "<<key<<" encoder returned empty data!";

  klfDbg("got data, size="<<data.size());
  return data;
}



