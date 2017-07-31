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
#include <QDomDocument>
#include <QTimer>

#include <klfdefs.h>
#include <klfutil.h>
#include <klfguiutil.h>

#include <klfuserscript.h>
#include <klfbackend.h>

#include "klfconfig.h"
#include "klfstyle.h"
#include "klflib.h"

#include "klfexporter.h"





//
// Utility: escape the value of an HTML attribute
//
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

static inline QByteArray toHtmlAttrTextAscii(QString s)
{
  // there are only ascii chars in the output of toHtmlAttrText()
  return toHtmlAttrText(s).toLatin1();
}





// ==============================================================================


class KLFBackendFormatsExporter : public QObject, public KLFExporter
{
  Q_OBJECT
public:
  KLFBackendFormatsExporter(QObject *parent)
    : QObject(parent), KLFExporter()
  {
  }
  virtual ~KLFBackendFormatsExporter()
  {
  }

  virtual QString exporterName() const
  {
    return QLatin1String("KLFBackendFormatsExporter");
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
    bool res = false;
    {
      QBuffer buffer(&data);
      buffer.open(QIODevice::WriteOnly);
      res = KLFBackend::saveOutputToDevice(output, &buffer, format, &error);
      buffer.close();
    }

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
    return QLatin1String("KLFTexExporter");
  }

  virtual QStringList supportedFormats(const KLFBackend::klfOutput& ) const
  {
    return QStringList() << "tex-with-klf-metainfo" << "tex";
  }

  virtual QString titleFor(const QString & format)
  {
    if (format == "tex") {
      return tr("LaTeX source");
    } else if (format == "tex-with-klf-metainfo") {
      return tr("LaTeX source (with KLF metainfo)");
    }
    klfWarning("Invalid format: " << format) ;
    return QString();
  }

  virtual QStringList fileNameExtensionsFor(const QString & format)
  {
    if (format == "tex") {
      return QStringList() << "tex" << "latex";
    } else if (format == "tex-with-klf-metainfo") {
      return QStringList() << "klftex" << "tex" << "latex";
    }
    klfWarning("Invalid format: " << format) ;
    return QStringList();
  }

  virtual QByteArray getData(const QString& format, const KLFBackend::klfOutput& output,
                             const QVariantMap&  = QVariantMap())
  {
    clearErrorString();

    if (format == "tex" || format == "tex-with-klf-metainfo") {
      
      bool include_meta = (format == "tex-with-klf-metainfo");

      QByteArray data;
      if (include_meta) {
        data = "%%KLF:LaTeX-save\n";
        data += "%%KLF:date: "+QDateTime::currentDateTime().toString(Qt::ISODate) + "\n%%KLF: \n";
      }
        
      data += output.input.latex.toUtf8();
      if (!data.endsWith("\n")) {
        data += "\n";
      }
      
      if (include_meta) {
        data += "%%KLF: \n";
        data += "%%KLF: ";

        // save style now as a LaTeX comment
        KLFStyle style(output.input);
        // only save file name as script path may differ:
        style.userScript = QFileInfo(style.userScript).fileName();
        QByteArray styledata = klfSave(&style, "XML");
        styledata = "\n"+styledata;
        styledata.replace("\n", "\n%%KLF:style: ");

        data += styledata + "\n";
      }

      return data;
    }

    klfWarning("Invalid format: " << format) ;
    return QByteArray();
  }
};



// =============================================================================




class KLFOpenOfficeDrawExporter : public QObject, public KLFExporter
{
  Q_OBJECT
public:
  KLFOpenOfficeDrawExporter(QObject *parent)
    : QObject(parent), KLFExporter()
  {
  }
  virtual ~KLFOpenOfficeDrawExporter()
  {
  }

  virtual QString exporterName() const
  {
    return QLatin1String("KLFOpenOfficeDrawExporter");
  }

  virtual QStringList supportedFormats(const KLFBackend::klfOutput& ) const
  {
    return QStringList() << "odg";
  }

  virtual QString titleFor(const QString & format)
  {
    if (format == "odg") {
      return tr("OpenOffice Draw");
    }
    klfWarning("Invalid format: " << format) ;
    return QString();
  }

  virtual QStringList fileNameExtensionsFor(const QString & format)
  {
    if (format == "odg") {
      return QStringList() << "odg";
    }
    klfWarning("Invalid format: " << format) ;
    return QStringList();
  }

  virtual QByteArray getData(const QString& format, const KLFBackend::klfOutput& klfoutput,
                             const QVariantMap&  = QVariantMap())
  {
    clearErrorString();

    if (format != "odg") {
      klfWarning("Invalid format: " << format) ;
      return QByteArray();
    }

    QByteArray pngdata = klfoutput.pngdata;
    
    QByteArray templ;
    {
      QFile templfile(":/data/ooodrawingtemplate");
      templfile.open(QIODevice::ReadOnly);
      templ = templfile.readAll();
    }
    
    QString fgcols = QColor(klfoutput.input.fg_color).name();
    QString bgcols;
    if (qAlpha(klfoutput.input.bg_color) > 0) {
      bgcols = QColor(klfoutput.input.fg_color).name();
    } else {
      bgcols = "-";
    }

    templ.replace("<!--KLF_PNG_BASE64_DATA-->", pngdata.toBase64());

    templ.replace("<!--KLF_INPUT_LATEX-->", toHtmlAttrTextAscii(klfoutput.input.latex));
    templ.replace("<!--KLF_INPUT_MATHMODE-->", toHtmlAttrTextAscii(klfoutput.input.mathmode));
    templ.replace("<!--KLF_INPUT_PREAMBLE-->", toHtmlAttrTextAscii(klfoutput.input.preamble));
    templ.replace("<!--KLF_INPUT_FGCOLOR-->", toHtmlAttrTextAscii(fgcols));
    templ.replace("<!--KLF_INPUT_BGCOLOR-->",	toHtmlAttrTextAscii(bgcols));
    templ.replace("<!--KLF_INPUT_DPI-->", toHtmlAttrTextAscii(QString::number(klfoutput.input.dpi)));
    templ.replace("<!--KLF_SETTINGS_TBORDEROFFSET_PSPT-->",
                  toHtmlAttrTextAscii(QString::number(klfoutput.settings.tborderoffset)));
    templ.replace("<!--KLF_SETTINGS_RBORDEROFFSET_PSPT-->",
                  toHtmlAttrTextAscii(QString::number(klfoutput.settings.rborderoffset)));
    templ.replace("<!--KLF_SETTINGS_BBORDEROFFSET_PSPT-->",
                  toHtmlAttrTextAscii(QString::number(klfoutput.settings.bborderoffset)));
    templ.replace("<!--KLF_SETTINGS_LBORDEROFFSET_PSPT-->",
                  toHtmlAttrTextAscii(QString::number(klfoutput.settings.lborderoffset)));

    templ.replace("<!--KLF_INPUT_LATEX_BASE64-->", klfoutput.input.latex.toLocal8Bit().toBase64());
    templ.replace("<!--KLF_INPUT_MATHMODE_BASE64-->", klfoutput.input.mathmode.toLocal8Bit().toBase64());
    templ.replace("<!--KLF_INPUT_PREAMBLE_BASE64-->", klfoutput.input.preamble.toLocal8Bit().toBase64());
    templ.replace("<!--KLF_INPUT_FGCOLOR_BASE64-->", fgcols.toLocal8Bit().toBase64());
    templ.replace("<!--KLF_INPUT_BGCOLOR_BASE64-->", bgcols.toLocal8Bit().toBase64());

    templ.replace("<!--KLF_OOOLATEX_ARGS-->", toHtmlAttrTextAscii("12§display§"+klfoutput.input.latex));

    // scale equation (eg. make them larger, so it is not too cramped up)
    const double DPI_FACTOR = klfconfig.ExportData.oooExportScale;

    // cm/inch = 2.54
    // include an elargment factor in these tags
    templ.replace("<!--KLF_IMAGE_WIDTH_CM-->",
                  QString::number(DPI_FACTOR * 2.54 * klfoutput.result.width()/klfoutput.input.dpi, 'f', 2).toUtf8());
    templ.replace("<!--KLF_IMAGE_HEIGHT_CM-->",
                  QString::number(DPI_FACTOR * 2.54 * klfoutput.result.height()/klfoutput.input.dpi, 'f', 2).toUtf8());
    // same, without the enlargment factor
    templ.replace("<!--KLF_IMAGE_ORIG_WIDTH_CM-->",
                  QString::number(2.54 * klfoutput.result.width()/klfoutput.input.dpi, 'f', 2).toUtf8());
    templ.replace("<!--KLF_IMAGE_ORIG_HEIGHT_CM-->",
                  QString::number(2.54 * klfoutput.result.height()/klfoutput.input.dpi, 'f', 2).toUtf8());

    templ.replace("<!--KLF_IMAGE_WIDTH_PX-->", QString::number(klfoutput.result.width()).toUtf8());
    templ.replace("<!--KLF_IMAGE_HEIGHT_PX-->", QString::number(klfoutput.result.height()).toUtf8());
    templ.replace("<!--KLF_IMAGE_ASPECT_RATIO-->",
                  QString::number((double)klfoutput.result.width()/klfoutput.result.height(), 'f', 3).toUtf8());

    klfDbg("final templ: "<<templ);

    return templ;
  }

};





// =============================================================================






class KLFHtmlDataExporter : public QObject, public KLFExporter
{
  Q_OBJECT
private:

  KLFExporterNameAndFormatList svg_exporter_formats;

public:
  KLFHtmlDataExporter(QObject *parent)
    : QObject(parent), KLFExporter(),
      svg_exporter_formats( KLFExporterNameAndFormatList()
                            << KLFExporterNameAndFormat("KLFBackendFormatsExporter", "SVG")
                            << KLFExporterNameAndFormat("UserScript:svg-dvisvgm", "svg")
                            << KLFExporterNameAndFormat("UserScript:inkscapeformats", "svg") )

  {
  }
  virtual ~KLFHtmlDataExporter()
  {
  }

  virtual QString exporterName() const
  {
    return QLatin1String("KLFHtmlDataExporter");
  }

  virtual QStringList supportedFormats(const KLFBackend::klfOutput& output) const
  {
    QStringList list;
    list << "html" << "html-png";

    KLFExporterManager * exporterManager = getExporterManager();
    if (exporterManager != NULL &&
        exporterManager->supportsOneOfExporterNamesAndFormats( svg_exporter_formats, output )) {
      list << "html-svg";
    }

    return list;
  }

  virtual QString titleFor(const QString & format)
  {
    if (format == "html") {
      return tr("HTML fragment with embedded graphics");
    } else if (format == "html-png") {
      return tr("HTML fragment with embedded PNG image");
    } else if (format == "html-svg") {
      return tr("HTML fragment with embedded SVG image");
    }
    klfWarning("Invalid format: " << format) ;
    return QString();
  }

  virtual QStringList fileNameExtensionsFor(const QString & format)
  {
    if (format == "html") {
      return QStringList() << "html";
    } else if (format == "html-png") {
      return QStringList() << "html";
    } else if (format == "html-svg") {
      return QStringList() << "html";
    }
    klfWarning("Invalid format: " << format) ;
    return QStringList();
  }

  virtual QByteArray getData(const QString& format, const KLFBackend::klfOutput& output,
                             const QVariantMap& params = QVariantMap())
  {
    clearErrorString();

    if (format == "html") {
      return get_html_mix_data(output, params) ;
    } else if (format == "html-png") {
      return get_html_png_data(output, params) ;
    } else if (format == "html-svg") {
      return get_html_svg_data(output, params) ;
    } else {
      klfWarning("Invalid format: " << format) ;
      return QByteArray();
    }
  }

  inline QByteArray get_html_mix_data(const KLFBackend::klfOutput& output, const QVariantMap& params)
  {
    // generate SVG tags with PNG fallback

    // use this really neat hack for SVG with PNG fallback:
    // http://www.kaizou.org/2009/03/inline-svg-fallback/


    // try to obtain SVG data
    QByteArray svgdata;

    KLFExporterManager * exMgr = getExporterManager();
    if (exMgr == NULL) {
      klfWarning("Can't get SVG data: exporter manager is NULL!") ;
      svgdata = QByteArray();
    } else {
      svgdata = exMgr->getDataByExporterNamesAndFormats( svg_exporter_formats, output, params );
    }
    if (svgdata.size() == 0) {
      // can't get SVG data, fall back to only PNG
      klfDbg("Can't get SVG, falling back to PNG only") ;
      return get_html_png_data(output, params) ;
    }

    // // First, get the fallback HTML-img code with PNG
    // QVariantMap p = params;
    // p["close_img_tag"] = true;
    // QByteArray htmlpngdata = get_html_png_data(output, p);
    //
    // QString ws = QString::number(output.width_pt, 'f', 4);
    // QString hs = QString::number(output.height_pt, 'f', 4);
    //    
    // QByteArray data = QString::fromLatin1(
    //     "<svg version=\"1.1\" width=\"%1pt\" height=\"%2pt\" viewBox=\"0 0 %3 %4\" xmlns=\"http://www.w3.org/2000/svg\" xmlns:xlink=\"http://www.w3.org/1999/xlink\"> "
    //     ).arg(ws, hs, ws, hs).toUtf8() +
    //   "<switch>" +
    //   QString::fromLatin1("<foreignObject width=\"%1\" height=\"%2\">").arg(ws, hs).toUtf8() +
    //   "<img src=\"data:image/svg+xml;base64," + svgdata.toBase64() + "\" "
    //   +  QString::fromLatin1(" style=\"width: %4pt; height: %5pt; vertical-align: middle;\" />").arg(ws, hs).toUtf8() +
    //   "</foreignObject>"
    //   "<foreignObject width=\"0\" height=\"0\">"
    //   + htmlpngdata +
    //   "</foreignObject>"
    //   "</switch>"
    //   "</svg>";
    //
    // data += "<p></p>"; // <p></p> hopefully puts Thunderbird editor cursor there and not
    //                    // inside the SVG hierarchy.
    // return data;


    // first of all, sanitize the XML document. E.g., single-quote attrs -> double quotes
    { QDomDocument svgdomdoc;
      svgdomdoc.setContent(svgdata);
      svgdata = "";
      QBuffer buf(&svgdata);
      QTextStream stream(&buf);
      buf.open(QIODevice::WriteOnly);
      svgdomdoc.save(stream, 2);
    }

    // remove the <?xml...?> header, and add vertical-align style property
    QString svgstring = QString::fromUtf8(svgdata).trimmed() ;
    QRegExp xmlheaderrx("\\<\\?\\s*xml\\s*version=['\"][^'\">]+[\"']\\s*encoding=[\"'][^'\">]+[\"']\\?\\>\\s*");
    svgstring.replace(xmlheaderrx, "");
    QRegExp svgrx("\\<svg\\s*");
    svgstring.replace(svgrx, "<svg style=\"vertical-align:middle\" ");
    
    // First, get the fallback HTML-img code with PNG
    QVariantMap p = params;
    p["close_img_tag"] = true;
    QByteArray htmlpngdata = get_html_png_data(output, p);

    // Further tweak SVG data to insert fallback code
    
    QRegExp svgendrx("\\</\\s*svg\\s*>\\s*$");
    svgstring.replace(svgendrx,
                      QString::fromUtf8("<switch>"
                                        "<g></g>"
                                        "<foreignObject width=\"0\" height=\"0\">"
                                        + htmlpngdata +
                                        "</foreignObject>"
                                        "</switch>"
                                        "</svg>"));
    
    return svgstring.toUtf8() + "<p></p>"; // <p></p> hopefully puts Thunderbird editor
                                           // cursor there and not inside the SVG
                                           // hierarchy.

    //svgdata = svgstring.toUtf8();
    // // Add the fallback
    // QByteArray fallbackfakesvg =
    //   "<svg version=\"1.1\" width=\"0pt\" height=\"0pt\""
    //   " xmlns=\"http://www.w3.org/2000/svg\" xmlns:xlink=\"http://www.w3.org/1999/xlink\">"
    //   "<switch>"
    //   "<g></g>" // empty graphics
    //   "<foreignObject width=\"0\" height=\"0\">"
    //   + htmlpngdata +
    //   "</foreignObject>"
    //   "</switch>"
    //   "</svg>";
    // return "<div style=\"display:inline\">" + svgdata + fallbackfakesvg + "</div>";
  }

  inline QByteArray get_html_svg_data(const KLFBackend::klfOutput& output, const QVariantMap& params)
  {
    // try to obtain SVG data
    KLFExporterManager * exMgr = getExporterManager();
    if (exMgr == NULL) {
      klfWarning("Can't get SVG data: exporter manager is NULL!") ;
      return QByteArray();
    }
    QByteArray svgdata = exMgr->getDataByExporterNamesAndFormats( svg_exporter_formats, output, params );

    if (svgdata.size() == 0) {
      klfWarning("Can't get SVG data, no exporter managed to provide us SVG.") ;
      return QByteArray();
    }

    // Doesn't seem to work great:
    //
    // QString svgstring = QString::fromUtf8(svgdata) ;
    // QRegExp xmlheaderrx("\\<\\?xml version='[^'>]+' encoding='[^'>]+'\\?\\>");
    // svgstring.replace(xmlheaderrx, "");
    // klfDbg("Got svgstring :\n" << svgstring) ;
    // return ("<div style=\"display: inline\">" + svgstring + "</div>").toUtf8();


    // So try to use <img src="data:image/svg+xml;base64,..."> instead

    QString latexattr = toHtmlAttrText(klfLatexToPseudoTex(output.input.latex));
    QString w_in = QString::number(output.width_pt / 72.0, 'f', 4);
    QString h_in = QString::number(output.width_pt / 72.0, 'f', 4);

    return "<img src=\"data:image/svg+xml;base64," + svgdata.toBase64() + "\" "
      +  (QString::fromLatin1(" alt=\"%1\" title=\"%2\" style=\"width: %3in; height: %4in; vertical-align: middle;\" />")
          .arg(latexattr, latexattr, w_in, h_in)).toUtf8();
  }

  inline QByteArray get_png_data_for_dpi(const KLFBackend::klfOutput & output, int html_export_dpi)
  {
    QImage img = output.result;
    if (html_export_dpi > 0 && html_export_dpi != output.input.dpi) {
      // needs to be rescaled
      QSize targetSize = img.size();
      targetSize *= (double) html_export_dpi / output.input.dpi;
      klfDbg("scaling to "<<html_export_dpi<<" DPI from "<<output.input.dpi<<" DPI... targetSize="<<targetSize) ;
      img = klfImageScaled(img, targetSize);
    }
    QByteArray png;
    { QBuffer buffer(&png);
      buffer.open(QIODevice::WriteOnly);
      img.save(&buffer, "PNG"); // writes image into ba in PNG format
    }
    return png;
  }

  inline QByteArray get_html_png_data(const KLFBackend::klfOutput& output, const QVariantMap& params)
  {
    int html_export_dpi = params.value("html_export_dpi", (int)klfconfig.ExportData.htmlExportDpi).toInt();
    int html_export_display_dpi = params.value("html_export_display_dpi",
                                               (int)klfconfig.ExportData.htmlExportDisplayDpi).toInt();

    if (output.input.dpi < html_export_dpi * 1.25f) {
      html_export_dpi = output.input.dpi;
    } 

    QByteArray png = get_png_data_for_dpi(output, html_export_dpi);

    QSize imgsize = output.result.size();
    imgsize *= (float) html_export_dpi / output.input.dpi;

    QString latexattr = toHtmlAttrText(klfLatexToPseudoTex(output.input.latex));

    QString w_in = QString::number((float)imgsize.width() / html_export_display_dpi, 'f', 3);
    QString h_in = QString::number((float)imgsize.height() / html_export_display_dpi, 'f', 3);

    QByteArray base64imgdata = png.toBase64();
    
    klfDbg("origimg/size="<<output.result.size()<<"; origDPI="<<output.input.dpi
           <<"; html_export_dpi="<<html_export_dpi
           <<"; html_export_display_dpi="<<html_export_display_dpi<<"; imgsize="<<imgsize
           <<"; w_in="<<w_in<<"; h_in="<<h_in) ;
    
    QString html =
      QString::fromLatin1("<img src=\"data:image/png;base64,%1\" alt=\"%2\" title=\"%3\" "
                          "style=\"width: %4in; height: %5in; vertical-align: middle;\">")
      .arg(base64imgdata, latexattr, latexattr, w_in, h_in);

    if (params.value("close_img_tag", false).toBool()) {
      html += "</img>";
    }

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
                             const QVariantMap& params = QVariantMap())
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

    KLFExporterNameAndFormatList inexpfmts = pUserScript.inputExportersFormats();

    QByteArray input_data;
    KLFExporterNameAndFormat which_input_data_format;
    KLFExporterManager * exMgr = getExporterManager();
    if (exMgr == NULL) {
      klfWarning("Can't get data for user script " << pUserScript.userScriptName()
                 << ": exporter manager is NULL!") ;
      input_data = QByteArray();
    } else {
      input_data = exMgr->getDataByExporterNamesAndFormats( inexpfmts, klfoutput, params, &which_input_data_format );
    }
    if (input_data.size() == 0 || which_input_data_format.exporterName.size() == 0) {
      // can't get SVG data, fall back to only PNG
      klfWarning("Can't get data for user script " << pUserScript.userScriptName()
                 << ": no available input format found!") ;
      return QByteArray();
    }

    QString ver = KLF_VERSION_STRING;
    ver.replace(".", "x"); // make friendly names with chars in [a-zA-Z0-9]
    
    QString tempfnametmpl = klfoutput.settings.tempdir + "/klftmpexporter" + ver + "xXXXXXX";

    KLFExporter * exporter = exMgr->exporterByName(which_input_data_format.exporterName) ;
    QStringList extlist = exporter->fileNameExtensionsFor(which_input_data_format.format);
    QString ext;
    if (extlist.size()) {
      ext = extlist[0];
    } else {
      ext = "tmp";
    }

    QTemporaryFile f_in(tempfnametmpl + "." + ext);
    f_in.setAutoRemove(true);
    f_in.open();
  
    QString errstr;
    f_in.write(input_data);
    f_in.close();

    p.addArgv(QStringList() << f_in.fileName());
    p.addArgv(QStringList() << "--format="+format);

    QStringList addenv;
    addenv << QLatin1String("KLF_OUTPUT_WIDTH_PT=") + QString::number(klfoutput.width_pt)
           << QLatin1String("KLF_OUTPUT_HEIGHT_PT=") + QString::number(klfoutput.height_pt)
           << QLatin1String("KLF_OUTPUT_WIDTH_PX=") + QString::number(klfoutput.result.width())
           << QLatin1String("KLF_OUTPUT_HEIGHT_PX=") + QString::number(klfoutput.result.height())
           << QLatin1String("KLF_INPUTDATA_EXPORTER_NAME=") + which_input_data_format.exporterName
           << QLatin1String("KLF_INPUTDATA_FORMAT=") + which_input_data_format.format
           << klfInputToEnvironmentForUserScript(klfoutput.input)
           << klfSettingsToEnvironmentForUserScript(klfoutput.settings)
      ;      

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

    p.setProcessAppEvents(true);

    bool ok = false;
    {
      KLFPleaseWaitPopup waitPopup(tr("Please wait while user script ‘%1’ is being run ...")
                                 .arg(pUserScript.userScriptBaseName()));
      // only show the popup after a short delay
      QTimer timer;
      connect(&timer, SIGNAL(timeout()), &waitPopup, SLOT(showPleaseWait()));
      timer.setSingleShot(true);
      timer.start(1000);

      // now, run the user script
      ok = p.run(QByteArray(), outdatamap);

      timer.stop();
    }

    if (outfn.size() && QFile::exists(outfn)) {
      QFile::remove(outfn);
    }

    if (!ok) {
      // error
      setErrorString(tr("Error running user script %1: %2").arg(pUserScript.userScriptBaseName())
                     .arg(p.resultErrorString()));
      klfWarning("Error: " << qPrintable(errorString())) ;
      return QByteArray();
    }

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

    const QString sep = params.value("separator", QString::fromLatin1("\n")).toString();

    QByteArray urilist = (url.toString() + sep).toUtf8();
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




/** KLFExporter implementation for exporting \c "text/x-moz-url" and \c "text/uri-list" to
 * a temporary file of a given type
 */
class KLF_EXPORT KLFTempImgRefHtmlExporter : public QObject, public KLFExporter
{
  Q_OBJECT
public:
  KLFTempImgRefHtmlExporter(QObject *parent) : QObject(parent) { }
  virtual ~KLFTempImgRefHtmlExporter() { }

  virtual QString exporterName() const
  {
    return QString::fromLatin1("KLFTempImgRefHtmlExporter");
  }

  virtual bool isSuitableForFileSave() const { return false; }

  virtual QStringList supportedFormats(const KLFBackend::klfOutput & ) const
  {
    return QStringList() << "html";
  }

  virtual QString titleFor(const QString & format) {
    if (format == "html") {
      return tr("HTML (with reference to temporary image file)");
    }
    klfWarning("Invalid format: " << format) ;
    return QString();
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

    QString tempfilename = KLFTempFileUriExporter::tempFileForOutput("PNG", output, html_export_dpi);

    QSize imgsize = output.result.size();
    imgsize *= (float) html_export_dpi / output.input.dpi;

    QString w_in = QString::number((float)imgsize.width() / html_export_display_dpi, 'f', 3);
    QString h_in = QString::number((float)imgsize.height() / html_export_display_dpi, 'f', 3);

    QString latexattr = toHtmlAttrText(klfLatexToPseudoTex(output.input.latex));

    klfDbg("origimg/size="<<output.result.size()<<"; origDPI="<<output.input.dpi
           <<"; html_export_dpi="<<html_export_dpi
           <<"; html_export_display_dpi="<<html_export_display_dpi<<"; imgsize="<<imgsize
           <<"; w_in="<<w_in<<"; h_in="<<h_in) ;
    
    QString html =
      QString::fromLatin1("<img src=\"file://%1\" alt=\"%2\" title=\"%3\" "
                          "style=\"width: %4in; height: %5in; vertical-align: middle;\">")
      .arg(QFileInfo(tempfilename).absoluteFilePath(), latexattr, latexattr, w_in, h_in);

    if (params.value("close_img_tag", false).toBool()) {
      html += "</img>";
    }

    return html.toUtf8();
  }


};





#endif
