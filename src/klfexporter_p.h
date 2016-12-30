/***************************************************************************
 *   file klfexporter_p.h
 *   This file is part of the KLatexFormula Project.
 *   Copyright (C) 2016 by Philippe Faist
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


#ifndef KLFEXPORTER_P_H
#define KLFEXPORTER_P_H

#include <QDateTime>
#include <QByteArray>
#include <QBuffer>

#include <klfdefs.h>
#include <klfutil.h>
#include <klfguiutil.h>

#include <klfuserscript.h>
#include <klfbackend.h>

#include "klfconfig.h"
#include "klfstyle.h"
#include "klflib.h"

#include "klfexporter.h"


extern QString klfbackend_last_userscript_output;







// ==============================================================================


class KLFBackendOutputFormatsExporter : public QObject, public KLFExporter
{
  Q_OBJECT
public:
  KLFBackendOutputFormatsExporter(QObject *parent)
    : QObject(parent), KLFExporter()
  {
  }
  virtual ~KLFBackendOutputFormatsExporter()
  {
  }

  virtual QString exporterName() const
  {
    return QLatin1String("backend");
  }

  virtual QStringList supportedFormats(const KLFBackend::klfOutput& output) const
  {
    return KLFBackend::availableSaveFormats(output);
  }

  virtual QString titleFor(const QString & format)
  {
    return tr("%1 Format").arg(format.toUpper());
  }

  virtual QStringList fileNameExtensionsFor(const QString & format)
  {
    QStringList exts;
    if (format.toUpper() == "JPEG") {
      exts << "jpg" << "jpeg";
    } else {
      exts << format.toLower();
    }
    return exts;
  }

  virtual QByteArray getData(const QString& format, const KLFBackend::klfOutput& output,
                             const QVariantMap&  = QVariantMap())
  {
    clearErrorString();

    QString error;
    QByteArray data;

    QBuffer buffer(&data);
    buffer.open(QIODevice::WriteOnly);
    bool res = KLFBackend::saveOutputToDevice(output, &buffer, format, &error);
    buffer.close();

    if ( ! res ) {
      setErrorString(error);
      return QByteArray();
    }

    return data;
  }

};




// ==============================================================================



class KLFTexExporter : public QObject, public KLFExporter
{
  Q_OBJECT
public:
  KLFTexExporter(QObject *parent)
    : QObject(parent), KLFExporter()
  {
  }
  virtual ~KLFTexExporter()
  {
  }

  virtual QString exporterName() const
  {
    return QLatin1String("tex-source");
  }

  virtual QStringList supportedFormats(const KLFBackend::klfOutput& ) const
  {
    return QStringList() << "latex-source";
  }

  virtual QString titleFor(const QString & format)
  {
    if (format == "latex-source") {
      return tr("LaTeX source", "[[human-readable format name for latex source exporter]]");
    }
    klfWarning("Invalid format: " << format) ;
    return QString();
  }

  virtual QStringList fileNameExtensionsFor(const QString & format)
  {
    if (format == "latex-source") {
      return QStringList() << "klftex" << "tex" << "latex";
    }
    klfWarning("Invalid format: " << format) ;
    return QStringList();
  }

  virtual QByteArray getData(const QString& format, const KLFBackend::klfOutput& output,
                             const QVariantMap&  = QVariantMap())
  {
    clearErrorString();

    if (format == "latex-source") {

      QByteArray data = "%%KLF:LaTeX-save\n";
      data += "%%KLF:date: "+QDateTime::currentDateTime().toString(Qt::ISODate) + "\n%%KLF: \n";

      data += output.input.latex.toUtf8();
      if (!data.endsWith("\n")) {
        data += "\n";
      }
      data += "%%KLF: \n";
      data += "%%KLF: ";

      // save style now as a LaTeX comment
      KLFStyle style(output.input);
      style.userScript = QFileInfo(style.userScript).fileName(); // only save file name as script path may differ
      QByteArray styledata = klfSave(&style, "XML");
      styledata = "\n"+styledata;
      styledata.replace("\n", "\n%%KLF:style: ");

      data += styledata + "\n";

      return data;
    }

    klfWarning("Invalid format: " << format) ;
    return QByteArray();
  }
};




// ==============================================================================






// escape the value of an HTML attribute
static inline QString toHtmlAttrText(QString s)
{
  KLF_DEBUG_BLOCK(KLF_FUNC_NAME) ;
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
}



class KLFHtmlDataExporter : public QObject, public KLFExporter
{
  Q_OBJECT
public:
  KLFHtmlDataExporter(QObject *parent)
    : QObject(parent), KLFExporter()
  {
  }
  virtual ~KLFHtmlDataExporter()
  {
  }

  virtual QString exporterName() const
  {
    return QLatin1String("html-data");
  }

  virtual QStringList supportedFormats(const KLFBackend::klfOutput& ) const
  {
    return QStringList() << "html";
  }

  virtual QString titleFor(const QString & format)
  {
    if (format == "html") {
      return tr("HTML fragment with embedded image data");
    }
    klfWarning("Invalid format: " << format) ;
    return QString();
  }

  virtual QStringList fileNameExtensionsFor(const QString & format)
  {
    if (format == "html") {
      return QStringList() << "html";
    }
    klfWarning("Invalid format: " << format) ;
    return QStringList();
  }

  virtual QByteArray getData(const QString& format, const KLFBackend::klfOutput& output,
                             const QVariantMap& params = QVariantMap())
  {
    clearErrorString();

    if (format != "html") {
      klfWarning("Invalid format: " << format) ;
      return QByteArray();
    }

    int html_export_dpi = params.value("html_export_dpi", (int)klfconfig.ExportData.htmlExportDpi).toInt();
    int html_export_display_dpi = params.value("html_export_display_dpi",
                                               (int)klfconfig.ExportData.htmlExportDisplayDpi).toInt();

    if (output.input.dpi < html_export_dpi * 1.25f) {
      html_export_dpi = output.input.dpi;
    } 

    QImage img = output.result;
    if (html_export_dpi > 0 && html_export_dpi != output.input.dpi) {
      // needs to be rescaled
      QSize targetSize = img.size();
      targetSize *= (double) html_export_dpi / output.input.dpi;
      klfDbg("scaling to "<<html_export_dpi<<" DPI from "<<output.input.dpi<<" DPI... targetSize="<<targetSize) ;
      img = klfImageScaled(img, targetSize);
    }

    QSize imgsize = output.result.size();
    imgsize *= (float) html_export_dpi / output.input.dpi;

    QString latexattr = toHtmlAttrText(klfLatexToPseudoTex(output.input.latex));

    QString w_in = QString::number((float)imgsize.width() / html_export_display_dpi, 'f', 3);
    QString h_in = QString::number((float)imgsize.height() / html_export_display_dpi, 'f', 3);

    QByteArray png;
    {
      QBuffer buffer(&png);
      buffer.open(QIODevice::WriteOnly);
      img.save(&buffer, "PNG"); // writes image into ba in PNG format
    }
    QByteArray base64imgdata = png.toBase64();
    
    klfDbg("origimg/size="<<output.result.size()<<"; origDPI="<<output.input.dpi
           <<"; html_export_dpi="<<html_export_dpi
           <<"; html_export_display_dpi="<<html_export_display_dpi<<"; imgsize="<<imgsize
           <<"; w_in="<<w_in<<"; h_in="<<h_in) ;
    
    QString html =
      QString::fromLatin1("<img src=\"data:image/png;base64,%1\" alt=\"%2\" title=\"%3\" "
                          "style=\"width: %4in; height: %5in; vertical-align: middle;\">")
      .arg(base64imgdata, latexattr, latexattr, w_in, h_in);

    return html.toUtf8();
  }
};









// =============================================================================
// USER SCRIPT EXPORTS
// =============================================================================


class KLFUserScriptExporter : public QObject, public KLFExporter
{
  Q_OBJECT
public:
  KLFUserScriptExporter(const QString& userscript, QObject *parent)
    : QObject(parent), KLFExporter(), pUserScript(userscript)
  {
  }
  virtual ~KLFUserScriptExporter()
  {
  }


  virtual QString exporterName() const
  {
    return QLatin1String("UserScript:") + pUserScript.userScriptBaseName();
  }

  virtual QStringList supportedFormats(const KLFBackend::klfOutput& ) const
  {
    /// \bug FIXME: make sure that the input-file-type is an available format from klfoutput
    return pUserScript.formats();
  }

  virtual QString titleFor(const QString & format)
  {
    return pUserScript.getFormat(format).formatTitle();
  }

  virtual QStringList fileNameExtensionsFor(const QString & format)
  {
    return pUserScript.getFormat(format).fileNameExtensions();
  }

  virtual QByteArray getData(const QString& format, const KLFBackend::klfOutput& klfoutput,
                             const QVariantMap& /*params*/ = QVariantMap())
  {
    KLF_DEBUG_BLOCK(KLF_FUNC_NAME) ;
    klfDbg("format="<<format) ;

    clearErrorString();

    // `params` is not used at the moment. In the future, `params` may correspond to
    // export options (e.g., jpeg quality) provided by the user in a separate dialog after
    // specifying the file name to save to.
    //
    //klfDbg("param="<<param) ;

    // now actually call the script
    KLFUserScriptFilterProcess p(pUserScript.userScriptPath(), &klfoutput.settings);

    if (klfconfig.UserScripts.userScriptConfig.contains(pUserScript.userScriptPath())) {
      p.addUserScriptConfig(klfconfig.UserScripts.userScriptConfig.value(pUserScript.userScriptPath()));
    }

    QString infmt = pUserScript.inputDataType();

    QString ver = KLF_VERSION_STRING;
    ver.replace(".", "x"); // make friendly names with chars in [a-zA-Z0-9]
    
    QString tempfnametmpl = klfoutput.settings.tempdir + "/klftmpexporter" + ver + "xXXXXXX";

    QString ext = infmt.toLower();
    QTemporaryFile f_in(tempfnametmpl + "." + ext);
    f_in.setAutoRemove(true);
    f_in.open();
  
    QString errstr;
    // prepare the input data to the script--
    KLFBackend::saveOutputToDevice(klfoutput, &f_in, infmt, &errstr);
    if (!errstr.isEmpty()) {
      klfWarning("Can't save klf output to format "<<infmt<<" [needed for input to "
                 <<pUserScript.userScriptBaseName()<<"]: "<<errstr) ;
      return QByteArray();
    }
    f_in.close();

    p.addArgv(QStringList() << f_in.fileName());
    p.addArgv(QStringList() << "--format="+format);

    QStringList addenv;
    addenv << QLatin1String("KLF_OUTPUT_WIDTH_PT=") + QString::number(klfoutput.width_pt)
           << QLatin1String("KLF_OUTPUT_HEIGHT_PT=") + QString::number(klfoutput.height_pt)
           << QLatin1String("KLF_OUTPUT_WIDTH_PX=") + QString::number(klfoutput.result.width())
           << QLatin1String("KLF_OUTPUT_HEIGHT_PX=") + QString::number(klfoutput.result.height())
           << klfInputToEnvironmentForUserScript(klfoutput.input)
           << klfSettingsToEnvironmentForUserScript(klfoutput.settings);

    p.addExecEnviron(addenv);

    // buffers to collect output
    QByteArray stdoutdata;
    QByteArray stderrdata;
    p.collectStdoutTo(&stdoutdata);
    p.collectStderrTo(&stderrdata);

    QString outfext = (QStringList()<<pUserScript.getFormat(format).fileNameExtensions()<<"out").first();

    QByteArray foutdata;
    QMap<QString,QByteArray*> outdatamap;
    QString outfn;
    if (!pUserScript.hasStdoutOutput()) {
      QFileInfo infi(f_in.fileName());
      outfn = infi.absolutePath() + "/" + infi.completeBaseName() + "." + outfext;
      outdatamap[outfn] = &foutdata;
      klfDbg("outfn = " << outfn) ;
    }

    // now, run the user script
    bool ok = p.run(QByteArray(), outdatamap);

    if (outfn.size() && QFile::exists(outfn)) {
      QFile::remove(outfn);
    }

    if (!ok) {
      // error
      setErrorString(tr("Error running user script %1: %2").arg(pUserScript.userScriptName())
                     .arg(p.resultErrorString()));
      klfWarning("Error: " << qPrintable(errorString())) ;
      klfbackend_last_userscript_output = p.resultErrorString();
      return QByteArray();
    }

    // for user script debugging
    klfbackend_last_userscript_output
      = "<b>STDOUT</b>\n<pre>" + QString(stdoutdata).toHtmlEscaped() + "</pre>\n<br/><b>STDERR</b>\n<pre>"
      + QString(stderrdata).toHtmlEscaped() + "</pre>";
  
    klfDbg("Ran script "<<pUserScript.userScriptPath()<<": stdout="<<stdoutdata<<"\n\tstderr="<<stderrdata) ;

    // and retreive the output data
    if (!foutdata.isEmpty()) {
      klfDbg("Got file output data: size="<<foutdata.size()) ;
      return foutdata;
    }

    return stdoutdata;
  }

private:
  KLFExportTypeUserScriptInfo pUserScript;
};





// =============================================================================
// "Temporary" export types specifically designed for drag&drop or copy&paste
// =============================================================================



/** KLFExporter implementation for exporting \c "text/x-moz-url" and \c "text/uri-list" to
 * a temporary file of a given type
 */
class KLF_EXPORT KLFTempFileUriExporter : public QObject, public KLFExporter
{
  Q_OBJECT
public:
  KLFTempFileUriExporter(QObject *parent) : QObject(parent) { }
  virtual ~KLFTempFileUriExporter() { }

  virtual QString exporterName() const
  {
    return QString::fromLatin1("KLFTempFileUriExporter");
  }

  virtual bool isSuitableForFileSave() const { return false; }

  virtual QStringList supportedFormats(const KLFBackend::klfOutput & output) const
  {
    return KLFBackend::availableSaveFormats(output);
  }

  virtual QString titleFor(const QString & format) { return tr("Uri of temporary %1 file").arg(format); }

  virtual QByteArray getData(const QString& format, const KLFBackend::klfOutput& klfoutput,
                             const QVariantMap& params = QVariantMap())
  {
    int req_dpi = params.value("dpi", QVariant(-1)).toInt();

    QString tempfilename = tempFileForOutput(format, klfoutput, req_dpi);
    QUrl url = QUrl::fromLocalFile(tempfilename);

    QByteArray urilist = (url.toString()+QLatin1String("\n")).toUtf8();
    return urilist;    
  }

  static QString tempFileForOutput(const QString & reqfmt, const KLFBackend::klfOutput& output,
                                   int targetDpi = -1)
  {
    KLF_DEBUG_BLOCK(KLF_FUNC_NAME) ;

    QString format = reqfmt.toLower();
  
    if (targetDpi <= 0) {
      targetDpi = output.input.dpi;
    }

    // a hash value of the output's result and the target dpi
    qint64 hashkey = output.result.cacheKey() ^ targetDpi;

    if (tempFilesCache[format].contains(hashkey)) {
      klfDbg("found cached temporary file: " << tempFilesCache[format][hashkey]) ;
      return tempFilesCache[format][hashkey];
    }

    QString templ = klfconfig.BackendSettings.tempDir +
      QString::fromLatin1("/klf_%2_XXXXXX.%1.%3")
      .arg(targetDpi).arg(QDateTime::currentDateTime().toString("yyyy-MM-dd_hh-mm")).arg(format);

    klfDbg("Attempting to create temp file from template name " << templ) ;

    QTemporaryFile *tempfile = new QTemporaryFile(templ, qApp);
    tempfile->setAutoRemove(true); // will be deleted when klatexformula exists (= qApp destroyed)
    if (tempfile->open() == false) {
      klfWarning("Can't create or open temp file for KLFTempFileUriExporter: template is " << templ) ;
      return QString();
    }

    QString tempfilename = QFileInfo(tempfile->fileName()).absoluteFilePath();

    if (isVectorFormat(format) || targetDpi <= 0 || targetDpi == output.input.dpi) {
      QString errStr;
      bool res = KLFBackend::saveOutputToDevice(output, tempfile, format, &errStr);
      if (!res) {
        klfWarning("Can't save to temp file " << tempfilename << ": " << errStr) ;
        tempfile->close();
        return QString();
      }
    } else { // need to rescale image to given DPI
      QImage img = output.result;
      QSize targetSize = img.size();
      targetSize *= (double) targetDpi / output.input.dpi;
      klfDbg("scaling to "<<targetDpi<<" DPI from "<<output.input.dpi<<" DPI... targetSize="<<targetSize) ;
      img = klfImageScaled(img, targetSize);
      bool res = img.save(tempfile, "PNG");
      if (!res) {
        klfWarning("Can't save dpi-rescaled image to temp file " << tempfile->fileName());
        tempfile->close();
        return QString();
      }
    }

    tempfile->close();

    // cache this temp file for other formats' use or other QMimeData instantiation...
    tempFilesCache[format][hashkey] = tempfilename;

    klfDbg("Wrote temp file with name " << tempfilename) ;

    return tempfilename;
  }

private:

  static bool isVectorFormat(const QString fmt) // fmt must be lower-case
  {
    if (fmt == "pdf" || fmt == "eps" || fmt == "ps" || fmt == "svg" || fmt == "dvi") {
      return true;
    }
    return false;
  }

  static QMap<QString,QMap<qint64,QString> > tempFilesCache;
};







#endif
