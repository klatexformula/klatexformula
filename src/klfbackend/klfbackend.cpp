/***************************************************************************
 *   file klfbackend.cpp
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

#include <stdio.h>
#include <stdlib.h>
#include <ctype.h> // isspace()
#include <sys/time.h>
#include <math.h> // fabs()

#include <QtGlobal>
#include <QByteArray>
#include <QSet>
#include <QCoreApplication>
#include <QRegExp>
#include <QFile>
#include <QDateTime>
#include <QTextStream>
#include <QBuffer>
#include <QDir>
#include <QColor>
#include <QTextDocument>
#include <QImageWriter>
#include <QTextCodec>
#include <QTemporaryDir>

#include <klfutil.h>
#include <klfsysinfo.h>
#include <klfdatautil.h>

#include "klfblockprocess.h"
#include "klffilterprocess.h"
#include "klfuserscript.h"
#include "klfbackend.h"
#include "klfbackend_p.h"



/** \mainpage
 *
 * <div style="width: 60%; padding: 0 20%; text-align: justify; line-height: 150%">
 * This documentation is the API documentation for the KLatexFormula library backend that
 * you may want to use in your programs. It is a GPL-licensed library based on QT 5 that
 * converts a LaTeX equation given as text into graphics, specifically PNG, EPS or PDF (and the
 * image is available as a QImage&mdash;so any format supported by %Qt is available.
 *
 * Starting %KLFBackend version 4.0, this library uses Qt5.
 *
 * Some utilities to save the output (in various formats) to a file or a QIODevice
 * are provided, see KLFBackend::saveOutputToFile() and KLFBackend::saveOutputToDevice().
 *
 * All the core functionality is based in the class \ref KLFBackend .
 *
 * This library has been tested to work in non-GUI applications (ie. FALSE in QApplication constructor,
 * or with QCoreApplication).
 *</div>
 */





// some standard guess settings for system configurations

#ifdef KLF_EXTRA_SEARCH_PATHS
#  define EXTRA_PATHS_PRE	KLF_EXTRA_SEARCH_PATHS ,
//#  define EXTRA_PATHS		KLF_EXTRA_SEARCH_PATHS
#else
#  define EXTRA_PATHS_PRE
//#  define EXTRA_PATHS
#endif


#if defined(Q_OS_WIN32) || defined(Q_OS_WIN64)
QStringList progLATEX = QStringList() << "latex.exe";
QStringList progDVIPS = QStringList() << "dvips.exe";
QStringList progGS = QStringList() << "gswin32c.exe" << "gswin64c.exe" << "mgs.exe";
//QStringList progEPSTOPDF = QStringList() << "epstopdf.exe";
static const char * standard_extra_paths[] = {
  EXTRA_PATHS_PRE
  "C:\\Program Files*\\MiKTeX*\\miktex\\bin",
  "C:\\texlive\\*\\bin\\win*",
  "C:\\Program Files*\\gs*\\gs*\\bin",
  NULL
};
#elif defined(KLF_WS_MAC)
QStringList progLATEX = QStringList() << "latex";
QStringList progDVIPS = QStringList() << "dvips";
QStringList progGS = QStringList() << "gs";
//QStringList progEPSTOPDF = QStringList() << "epstopdf";
static const char * standard_extra_paths[] = {
  EXTRA_PATHS_PRE
  "/usr/texbin:/Library/TeX/texbin:/usr/local/bin:/opt/local/bin:/sw/bin:/sw/usr/bin",
  NULL
};
#else
QStringList progLATEX = QStringList() << "latex";
QStringList progDVIPS = QStringList() << "dvips";
QStringList progGS = QStringList() << "gs";
//QStringList progEPSTOPDF = QStringList() << "epstopdf";
static const char * standard_extra_paths[] = {
  EXTRA_PATHS_PRE
  NULL
};
#endif



// ---------------------------------

KLFAbstractLatexMetaInfo::KLFAbstractLatexMetaInfo()
{
}
KLFAbstractLatexMetaInfo::~KLFAbstractLatexMetaInfo()
{
}

void KLFAbstractLatexMetaInfo::saveMetaInfo(const KLFBackend::klfInput& in,
					    const KLFBackend::klfSettings& settings)
{
  static QString boolstr[2] = { QLatin1String("true"), QLatin1String("false") } ;
  
  saveField("AppVersion", QString::fromLatin1("KLatexFormula " KLF_VERSION_STRING));
  saveField("Application",
	       QObject::tr("Created with KLatexFormula version %1", "KLFBackend::saveOutputToFile")
	       .arg(KLF_VERSION_STRING));
  saveField("Software", QString::fromLatin1("KLatexFormula " KLF_VERSION_STRING));
  saveField("InputLatex", in.latex);
  saveField("InputMathMode", in.mathmode);
  saveField("InputPreamble", in.preamble);
  saveField("InputFontSize", QString::number(in.fontsize, 'g', 2));
  saveField("InputFgColor", QString("rgb(%1, %2, %3)").arg(qRed(in.fg_color))
	       .arg(qGreen(in.fg_color)).arg(qBlue(in.fg_color)));
  saveField("InputBgColor", QString("rgba(%1, %2, %3, %4)").arg(qRed(in.bg_color))
	       .arg(qGreen(in.bg_color)).arg(qBlue(in.bg_color))
	       .arg(qAlpha(in.bg_color)));
  saveField("InputDPI", QString::number(in.dpi));
  saveField("InputVectorScale", QString::number(in.vectorscale, 'g', 4));
  saveField("InputBypassTemplate", boolstr[(int)in.bypassTemplate]);
  saveField("InputUserScript", QFileInfo(in.userScript).fileName());
  QString usparams;
  klfSaveVariantToText(QVariant(klfMapToVariantMap(in.userScriptParam)), true);
  saveField("InputUserScriptParams", usparams);
  saveField("SettingsTBorderOffset", QString::number(settings.tborderoffset));
  saveField("SettingsRBorderOffset", QString::number(settings.rborderoffset));
  saveField("SettingsBBorderOffset", QString::number(settings.bborderoffset));
  saveField("SettingsLBorderOffset", QString::number(settings.lborderoffset));
  saveField("SettingsOutlineFonts", boolstr[(int)settings.outlineFonts]);
  saveField("SettingsCalcEpsBoundingBox", boolstr[(int)settings.calcEpsBoundingBox]);
  saveField("SettingsWantRaw", boolstr[(int)settings.wantRaw]);
  saveField("SettingsWantPDF", boolstr[(int)settings.wantPDF]);
  saveField("SettingsWantSVG", boolstr[(int)settings.wantSVG]);

  klfDbg("saved meta-info.") ;
}


KLFImageLatexMetaInfo::KLFImageLatexMetaInfo(QImage *imgwrite) : _w(imgwrite) { }

void KLFImageLatexMetaInfo::saveField(const QString& k, const QString& v)
{
  // QImageWriter::setText() uses QString::simplified() and does not save whitespace properly :(
  // so encode text in some appropriate way.
  _w->setText(k, klfDataToEscaped(v.toUtf8(), '%'));
}
QString KLFImageLatexMetaInfo::loadField(const QString &k) {
  return QString::fromUtf8(klfEscapedToData(_w->text(k).toLatin1(), '%'));
}


KLF_EXPORT QByteArray klf_escape_ps_string(const QString& v)
{
  // write escape codes
  int i;
  // if v is just ascii, no need to encode it in unicode
  bool isascii = true;
  for (i = 0; i < v.length(); ++i) {
    if (v[i] < 0 || v[i] > 126) {
      isascii = false;
      break;
    }
  }
  QByteArray vdata;
  if (isascii) {
    vdata = v.toLatin1();
    
    QByteArray escaped;
    for (i = 0; i < vdata.size(); ++i) {
      char c = vdata[i];
      klfDbg("Char: "<<c);
      if (QChar(vdata[i]).isLetterOrNumber() || c == ' ' || c == '.' || c == ',' || c == '/')
	escaped += c;
      else if (c == '\n')
	escaped += "\\n";
      else if (c == '\r')
	escaped += "\\r";
      else if (c == '\t')
	escaped += "\\t";
      else if (c == '\\')
	escaped += "\\\\";
      else if (c == '(')
	escaped += "\\(";
      else if (c == ')')
	escaped += "\\)";
      else {
	klfDbg("escaping char: (int)c="<<(int)c<<" (uint)c="<<uint(c)<<", octal="<<klfFmtCC("%03o", (uint)c));
	escaped += QString("\\%1").arg((unsigned int)(unsigned char)c, 3, 8, QChar('0')).toLatin1();
      }
    }
    
    return "("+escaped+")";
  }

  // otherwise, do unicode encoding
  
  QTextCodec *codec = QTextCodec::codecForName("UTF-16BE");
  vdata = codec->fromUnicode(v);
  klfDbg("vdata is "<<klfDataToEscaped(vdata));
  
  QByteArray hex;
  for (i = 0; i < (vdata.size()-1); i += 2) {
    hex += klfFmt("%02x%02x ", (unsigned int)(unsigned char)vdata[i], (unsigned int)(unsigned char)vdata[i+1]);
  }
  return "<" + hex + ">";
}



KLFPdfmarksWriteLatexMetaInfo::KLFPdfmarksWriteLatexMetaInfo(QByteArray * string)
  : _s(string)
{
  // See the following for more info:
  // http://stackoverflow.com/questions/3010015/pdfmark-for-docinfo-metadata-in-pdf-is-not-accepting-accented-characters-in-keyw
  // http://www.justskins.com/forums/adding-metadata-to-pdf-68647.html
  
  _s->append( // ensure pdfmark symbol defined in postscript
	     "/pdfmark where { pop } { /globaldict where { pop globaldict } { userdict } ifelse "
	     "/pdfmark /cleartomark load put } ifelse\n"
	     // now the proper PDFmarks DOCINFO dictionary
	     "[ "
	     );
}

QString KLFPdfmarksWriteLatexMetaInfo::loadField(const QString& )
{
  KLF_ASSERT_CONDITION(false, "N/A.", return QString(); ) ;
}
void KLFPdfmarksWriteLatexMetaInfo::saveField(const QString& k, const QString& v)
{
  savePDFField("KLF"+k, v);
}
void KLFPdfmarksWriteLatexMetaInfo::finish()
{
  _s->append("  /DOCINFO pdfmark\n");
}
void KLFPdfmarksWriteLatexMetaInfo::savePDFField(const QString& k, const QString& v)
{
  QByteArray datavalue = klf_escape_ps_string(v);
  
  _s->append( "  /"+k+" " + datavalue + "\n");
}






// ---------------------------------


static QMutex klf_mutex;

struct GsInfo
{
  GsInfo() { }

  QString version;
  int version_maj;
  int version_min;
  QString help;
  QSet<QString> availdevices;
};

// cache gs version/help/etc. information (for each gs executable, in case there are several)
static QMap<QString,GsInfo> gsInfo = QMap<QString,GsInfo>();

static void initGsInfo(const KLFBackend::klfSettings *settings, bool isMainThread);







// ---------------------------------


KLFBackend::TemplateGenerator::TemplateGenerator()
{
}
KLFBackend::TemplateGenerator::~TemplateGenerator()
{
}

KLFBackend::DefaultTemplateGenerator::DefaultTemplateGenerator()
{
}
KLFBackend::DefaultTemplateGenerator::~DefaultTemplateGenerator()
{
}

QString KLFBackend::DefaultTemplateGenerator::generateTemplate(const klfInput& in,
                                                               const klfSettings& /*settings*/)
{
  QString latexin;
  QString s;

  latexin = in.mathmode;
  latexin.replace("...", in.latex);

  /// \todo 'minimal' or 'article' by default ???
  s += "\\documentclass{article}\n"
    "\\usepackage[dvips]{color}\n";
  s += in.preamble;
  s += "\n"
    "\\begin{document}\n"
    "\\thispagestyle{empty}\n";
  if (in.fontsize > 0) {
    s += QString("\\fontsize{%1}{%2}\\selectfont\n").arg(in.fontsize, 0, 'f', 2).arg(in.fontsize*1.2, 0, 'f', 2);
  }
  s += QString("\\definecolor{klffgcolor}{rgb}{%1,%2,%3}\n").arg(qRed(in.fg_color)/255.0)
    .arg(qGreen(in.fg_color)/255.0).arg(qBlue(in.fg_color)/255.0);
  s += QString("\\definecolor{klfbgcolor}{rgb}{%1,%2,%3}\n").arg(qRed(in.bg_color)/255.0)
    .arg(qGreen(in.bg_color)/255.0).arg(qBlue(in.bg_color)/255.0);
  if (qAlpha(in.bg_color)>0)
    s += "\\pagecolor{klfbgcolor}%\n";
  s += "{\\color{klffgcolor} ";
  s += latexin;
  s += "%\n"
    "}\n"
    "\\end{document}\n";

  return s;
}




// ---------------------------------

KLFBackend::KLFBackend()
{
}




#define D_RX "([0-9eE.-]+)"

// A Bounding Box
struct klfbbox {
  double x1, x2, y1, y2;
};



static bool calculate_gs_eps_bbox(const QByteArray& epsdata, const QString& epsFile, klfbbox *bbox,
				  KLFBackend::klfOutput * resError, const KLFBackend::klfSettings& settings,
				  bool isMainThread);
static bool read_eps_bbox(const QByteArray& epsdata, klfbbox *bbox, KLFBackend::klfOutput * resError);
static void correct_eps_bbox(const QByteArray& epsdata,
                             const klfbbox& bbox_corrected, const klfbbox& bbox_orig,
			     double vectorscale, QRgb bgcolor, QByteArray * epsdatacorrected);

static void replace_svg_width_or_height(QByteArray *svgdata, const char * attr, double val);


static inline bool has_userscript_output(const QSet<QString>& fmts, const QString& format)
{
  return fmts.contains(format);
  //  if (!fmts.contains(format))
  //    return false;
  //  return fn.isEmpty() ? true : QFile::exists(fn);
}



typedef QSet<QString> KLFStringSet;

KLF_EXPORT KLFStringSet klfbackend_fmts =
  KLFStringSet()
  /* */   << "latex" << "dvi" << "eps-raw" << "eps-bbox" << "eps-processed"
/*   */   << "png" << "pdf" << "svg-gs" << "svg" ;


KLF_EXPORT KLFStringSet klfbackend_dependencies(const QString& fmt, bool recursive = false)
{
  static KLFStringSet fn_lock = KLFStringSet();

  if (fn_lock.contains(fmt)) {
    klfWarning("Dependency loop detected for format "<<fmt) ;
    return KLFStringSet();
  }
  fn_lock << fmt;

  KLFStringSet s;
  if (fmt == QLatin1String("tex") || fmt == QLatin1String("latex")) {
    // no dependency
  } else if (fmt == QLatin1String("dvi")) {
    s << "latex";
  } else if (fmt == QLatin1String("eps-raw")) {
    s << "dvi";
  } else if (fmt == QLatin1String("eps-bbox")) {
    s << "eps-raw";
  } else if (fmt == QLatin1String("eps-processed")) {
    s << "eps-bbox";
  } else if (fmt == QLatin1String("png")) {
    s << "eps-processed";
  } else if (fmt == QLatin1String("pdf")) {
    s << "eps-processed";
  } else if (fmt == QLatin1String("svg-gs")) {
    s << "eps-processed";
  } else if (fmt == QLatin1String("svg")) {
    s << "svg-gs";
  } else {
    klfWarning("Unknown format : "<<fmt) ;
  }
  if (!recursive) {
    fn_lock.remove(fmt);
    return s;
  }
  // explore dependencies recursively 
  KLFStringSet basedeps = s;
  foreach (QString str, basedeps) {
    KLFStringSet subdeps = klfbackend_dependencies(str, true);
    foreach (QString subdep, subdeps) {
      s << subdep;
    }
  }

  fn_lock.remove(fmt);
  return s;
}

static inline bool assert_have_formats_for(const KLFStringSet& outputs, const KLFStringSet& skipfmts,
					   const QString& forwhat)
{
  KLFStringSet fmtlist = klfbackend_dependencies(forwhat);
  foreach (QString s, fmtlist) {
    if (skipfmts.contains(s) && !outputs.contains(s)) {
      klfWarning("User Script Skipped format "<<s<<" which is necessary for "<<forwhat) ;
      return false;
    }
  }
  return true;
}

#define ASSERT_HAVE_FORMATS_FOR(forwhat)				\
  { if (!assert_have_formats_for(us_outputs, us_skipfmts, forwhat)) {	\
      res.status = KLFERR_USERSCRIPT_BADSKIPFORMATS; 			\
      res.errorstr = QObject::tr("User Script broke dependencies in skip-formats list", "KLFBackend"); \
      return res;							\
    }									\
  }




KLFBackend::klfOutput KLFBackend::getLatexFormula(const klfInput& input, const klfSettings& usersettings,
						  bool isMainThread)
{
  // ALLOW ONLY ONE RUNNING getLatexFormula() AT A TIME 
  QMutexLocker mutexlocker(&klf_mutex);

  KLF_DEBUG_TIME_BLOCK(KLF_FUNC_NAME) ;

  klfSettings settings;
  settings = usersettings;

  klfInput in;
  in = input;

  bool ok;

  klfDbg("called. latex="<<in.latex);

  { // get full, expanded exec environment
    QStringList curenv = klfCurrentEnvironment();
    klfDbg("current environment is "<<curenv) ;
    settings.execenv = klfMergeEnvironment(curenv, settings.execenv,
					   QStringList() << "PATH" << "TEXINPUTS" << "BIBINPUTS",
					   KlfEnvPathPrepend|KlfEnvMergeExpandVars);
  }

  klfDbg("execution environment for sub-processes is "<<settings.execenv) ;

  
  klfOutput res;
  res.status = KLFERR_NOERROR;
  res.errorstr = QString();
  res.result = QImage();
  res.pngdata_raw = QByteArray();
  res.pngdata = QByteArray();
  res.dvidata = QByteArray();
  res.epsdata_raw = QByteArray();
  res.epsdata = QByteArray();
  res.pdfdata = QByteArray();
  res.svgdata = QByteArray();
  res.input = in;
  res.settings = settings;


  // read GS version, will need later
  initGsInfo(&settings, isMainThread);
  if (!gsInfo.contains(settings.gsexec)) {
    res.status = KLFERR_NOGSVERSION;
    res.errorstr = QObject::tr("Can't query version of ghostscript located at `%1'.", "KLFBackend")
      .arg(settings.gsexec);
    return res;
  }

  const GsInfo thisGsInfo = gsInfo.value(settings.gsexec);

  klfDebugf(("%s: queried ghostscript version: %s", KLF_FUNC_NAME, qPrintable(thisGsInfo.version))) ;

  // force some rules on settings

  // if calcEpsBoundingBox is being used, we need to add bg color at "correcting bbox time"
  QRgb bgcolor_when_correcting_bbox = qRgba(0,0,0,0);
  klfDebugf(("%s: settings.calcEpsBoundingBox=%d, in.bg_color=[RGBA %d,%d,%d,%d]", KLF_FUNC_NAME,
	     settings.calcEpsBoundingBox, qRed(in.bg_color), qGreen(in.bg_color), qBlue(in.bg_color),
	     qAlpha(in.bg_color)));

  if (settings.calcEpsBoundingBox &&
      qAlpha(in.bg_color) != 0 && (in.bg_color & qRgb(255,255,255)) != qRgb(255,255,255)) {
    bgcolor_when_correcting_bbox = in.bg_color;
    in.bg_color = qRgba(0,0,0,0);
  }


  // PROCEDURE (V3.3)
  //
  // EACH STEP MIGHT BE DONE BY A USER SCRIPT INSTEAD IF THAT IS REQUESTED.
  //
  // - generate LaTeX file
  //
  // - latex                            --> get DVI file
  //
  // - dvips -E file.dvi -o file.eps    --> get (first) EPS file
  //
  // - gs -dNOPAUSE -dSAFER -sDEVICE=bbox -q -dBATCH file.eps      --> calculate correct bbox for EPS file
  //
  //   will output something like
  //     %%BoundingBox: int(X1) int(Y1) int(X2) int(Y2)
  //     %%HiResBoundingBox: X1 Y1 X2 Y2
  //
  // - read file.eps, modify e.g. as file-bbox.eps: replace
  //     %%BoundingBox ***
  //   by
  //     %%HiResBoundingBox: 0 0 (X2-X1) (Y2-Y1)
  //     -X1 -Y1 translate
  //   while of course taking into account manual corrections given by [lrtb]borderoffset settings/overrides
  //
  // EITHER (gs >= 9.01 && !outlinefonts)
  //   ### PhF: update this doc!! it's wrong!!
  //  - gs -dNOPAUSE -dSAFER -dSetPageSize -sDEVICE=ps2write -dEPSCrop -sOutputFile=file-corrected.(e)ps
  //       -q -dBATCH  file-bbox.eps    --> generate (E)PS file w/ correct page size
  // OR
  //  - gs -dNOCACHE -dNOPAUSE -dSAFER -sDEVICE=pswrite -dEPSCrop -sOutputFile=file-corrected.eps -q -dBATCH
  //       file-bbox.eps                --> generate post-processed (E)PS file
  //
  // - gs -dNOPAUSE -dSAFER -sDEVICE=pdfwrite -sOutputFile=file.pdf -q -dBATCH file-corrected.eps
  //       with added pdfmarks..
  //
  // - if (reports-has-device(svg)) {
  //
  //     - gs -dNOPAUSE -dSAFER -sDEVICE=svg -r72x72 -sOutputFile=file.svg -q -dBATCH file-corrected.eps
  //
  //     - modify SVG file to replace  width='WWpt' height='HHpt' by 
  //         width='(X2-X1)px' height='(Y2-Y1)px'
  //       with data given by gs before, with full precision
  //
  // - }

  QString ver = KLF_VERSION_STRING;
  ver.replace(".", "x"); // make friendly names with chars in [a-zA-Z0-9]
  // the base name for all our temp files
  //  QString tempfname = settings.tempdir + "/klftmp" + ver + "T" + QDateTime::currentDateTime().toString("hhmmss")
  //    + "p"+ QString("%1").arg(QCoreApplication::applicationPid(), 0, 26);
  QString temptemplate = settings.tempdir + "/klftmp"+ver+"-XXXXXX";

  // create the temporary directory
  QTemporaryDir tempdir(temptemplate);

  if (!tempdir.isValid()) {
    // failed to create temporary directory
    res.errorstr = QObject::tr("Failed to create temporary directory inside `%1'",
                               "KLFBackend").arg(settings.tempdir);
    res.status = KLFERR_TEMPDIR_FAIL;
    return res;
  }

  QString tempfname = tempdir.path() + "/klftemp";

  klfDbg("Temp location base name is "<<tempfname) ;

  QString fnTex = tempfname + ".tex";
  QString fnDvi = tempfname + ".dvi";
  QString fnRawEps = tempfname + ".eps";
  QString fnBBoxEps = tempfname + "-bbox.eps";
  QString fnProcessedEps = tempfname + "-processed.eps";
  QString fnRawPng = tempfname + "-raw.png";
  QString fnPdfMarks = tempfname + ".pdfmarks";
  QString fnPdf = tempfname + ".pdf";
  QString fnGsSvg = tempfname + "-gs.svg";
  // some user scripts may provide directly .svg (even though the default process chain
  // processes raw svg in memory, not generating final svg file)
  QString fnSvg = tempfname + ".svg";

  // we need non-outlinedfont EPS data anyway.
  QByteArray rawepsdata;
  QByteArray bboxepsdata;
  QByteArray gssvgdata;


  QString latexsimplified = in.latex.trimmed();
  if (latexsimplified.isEmpty()) {
    res.errorstr = QObject::tr("You must specify a LaTeX formula!", "KLFBackend");
    res.status = KLFERR_MISSINGLATEXFORMULA;
    return res;
  }

  if (!in.bypassTemplate) {
    if (in.mathmode.contains("...") == 0) {
      res.status = KLFERR_MISSINGMATHMODETHREEDOTS;
      res.errorstr = QObject::tr("The math mode string doesn't contain '...'!", "KLFBackend");
      return res;
    }
  }

  // prepare LaTeX file
  {
    QFile file(fnTex);
    bool r = file.open(QIODevice::WriteOnly);
    if ( ! r ) {
      res.status = KLFERR_TEXWRITEFAIL;
      res.errorstr = QObject::tr("Can't open file for writing: '%1'!", "KLFBackend").arg(fnTex);
      return res;
    }
    QTextStream stream(&file);
    if (!in.bypassTemplate) {
      TemplateGenerator *t = NULL;
      DefaultTemplateGenerator deft;
      if (settings.templateGenerator != NULL) {
	klfDbg("using custom template generator") ;
	t = settings.templateGenerator;
	KLF_ASSERT_NOT_NULL(t, "Template Generator is NULL! Using default!",  t = &deft; ) ;
      } else {
	t = &deft;
      }
      stream << t->generateTemplate(in, settings);
    } else {
      stream << in.latex;
    }
  }

  KLFStringSet us_outputs;
  KLFStringSet us_skipfmts;
  KLFStringSet our_skipfmts;

  if (!in.userScript.isEmpty()) {
    // user has provided us a wrapper script. Query it and use it

    KLFBackendEngineUserScriptInfo scriptinfo(in.userScript);

    if (scriptinfo.scriptInfoError() != KLFERR_NOERROR) {
      res.status = scriptinfo.scriptInfoError();
      res.errorstr = scriptinfo.scriptInfoErrorString();
      return res;
    }

    if ( (!scriptinfo.klfMinVersion().isEmpty()
	  && klfVersionCompare(scriptinfo.klfMinVersion(), KLF_VERSION_STRING) > 0) ||
	 (!scriptinfo.klfMaxVersion().isEmpty()
	  && klfVersionCompare(scriptinfo.klfMaxVersion(), KLF_VERSION_STRING) < 0) ) {
      res.status = KLFERR_USERSCRIPT_BADKLFVERSION;
      res.errorstr = QObject::tr("User Script `%1' is not compatible with current version of KLatexFormula.",
				 "KLFBackend").arg(scriptinfo.name());
      return res;
    }

    if (scriptinfo.category() != QLatin1String("klf-backend-engine")) {
      res.status = KLFERR_USERSCRIPT_BADCATEGORY;
      res.errorstr = QObject::tr("User Script `%1' is not usable as backend latex engine!",
				 "KLFBackend").arg(scriptinfo.name());
      return res;
    }

    // and run the script with the latex input
    QStringList addenv;
    addenv
      // program executables
      << "KLF_TEMPDIR=" + settings.tempdir
      << "KLF_LATEX=" + settings.latexexec
      << "KLF_DVIPS=" + settings.dvipsexec
      << "KLF_GS=" + settings.gsexec
      << "KLF_GS_VERSION=" + thisGsInfo.version
      << "KLF_GS_DEVICES=" + QStringList(thisGsInfo.availdevices.toList()).join(",")
      // input
      << klfInputToEnvironmentForUserScript(in)
      // more advanced settings
      << klfSettingsToEnvironmentForUserScript(settings)
      // file names (all formed with same basename...) to access by the script
      << "KLF_TEMPFNAME=" + tempfname // the base name for all our temp files
      << "KLF_FN_TEX=" + fnTex
      << "KLF_FN_LATEX=" + fnTex
      << "KLF_FN_DVI=" + fnDvi
      << "KLF_FN_EPS_RAW=" + fnRawEps
      << "KLF_FN_EPS_BBOX=" + fnBBoxEps
      << "KLF_FN_EPS_PROCESSED=" + fnProcessedEps
      << "KLF_FN_PNG=" + fnRawPng
      << "KLF_FN_PDFMARKS=" + fnPdfMarks
      << "KLF_FN_PDF=" + fnPdf
      << "KLF_FN_SVG_GS=" + fnGsSvg
      << "KLF_FN_SVG=" + fnSvg
      ;

    { // now run the script
      KLFUserScriptFilterProcess p(scriptinfo.userScriptPath(), &settings);

      p.addExecEnviron(addenv);

      p.setProcessAppEvents(false);

      QByteArray stderrdata;
      QByteArray stdoutdata;
      p.collectStderrTo(&stderrdata);
      p.collectStdoutTo(&stdoutdata);

      p.addArgv(QDir::toNativeSeparators(fnTex));

      QMap<QString,QByteArray*> outdata;
      QStringList outfmts = scriptinfo.spitsOut();
      foreach (QString fmt, outfmts) {
	us_outputs << fmt;
	if (fmt == QLatin1String("latex")) {
	  // user script overwrote the tex/latex file, don't collect the tex file data as it is not
	  // needed (not considered as useful output!). This new tex file will be seen and accessed
	  // by 'latex' in the next process block after userscript if needed.
	} else if (fmt == QLatin1String("dvi")) {
	  outdata[fnDvi] = &res.dvidata;
	} else if (fmt == QLatin1String("eps-raw")) {
	  // if we don't need this, it will be removed below from the list
	  outdata[fnRawEps] = &rawepsdata;
	} else if (fmt == QLatin1String("eps-bbox")) {
	  outdata[fnBBoxEps] = &bboxepsdata;
	} else if (fmt == QLatin1String("eps-processed")) {
	  outdata[fnProcessedEps] = &res.epsdata;
	} else if (fmt == QLatin1String("png")) {
	  if (settings.wantRaw) {
	    outdata[fnRawPng] = &res.pngdata_raw;
          }
	} else if (fmt == QLatin1String("pdf")) {
	  if (settings.wantPDF) {
	    outdata[fnPdf] = &res.pdfdata;
          }
	} else if (fmt == QLatin1String("svg-gs")) {
	  // ignore this data, not returned in klfOutput but the created file is used to generate
	  // the processed SVG
	  outdata[fnGsSvg] = & gssvgdata;
	} else if (fmt == QLatin1String("svg")) {
	  if (settings.wantSVG) {
	    outdata[fnSvg] = &res.svgdata;
          }
	} else {
	  klfWarning("Can't handle output format from user script: "<<fmt) ;
	}
      }
      if (us_outputs.isEmpty()) {
	us_outputs << "dvi"; // by default, the script is assumed to provide DVI.
      }
      if (us_outputs.contains("eps-bbox") && !settings.wantRaw) {
	// don't need to fetch initial raw eps data
	if (outdata.contains("eps-raw"))
	  outdata.remove(fnRawEps);
      }
      if (us_outputs.contains("eps-processed") && !settings.wantRaw) {
	// don't need to fetch bbox raw eps data
	if (outdata.contains("eps-bbox"))
	  outdata.remove(fnBBoxEps);
      }

      QStringList skipfmts = scriptinfo.skipFormats();
      bool invert = false;
      int tempi;
      if ((tempi = skipfmts.indexOf("ALL_EXCEPT")) >= 0) {
	invert = true;
	skipfmts.removeAt(tempi);
	foreach (QString f, klfbackend_fmts) { us_skipfmts << f; }
      }
      foreach (QString fmt, skipfmts) {
	if (!klfbackend_fmts.contains(fmt)) {
	  klfWarning("User Script Info: Unknown format to skip: "<<fmt) ;
	}
	if (!invert) {
	  us_skipfmts << fmt;
	} else {
	  us_skipfmts.remove(fmt);
	}
      }
      our_skipfmts = us_skipfmts;
      foreach (QString fmt, outfmts) {
        if (our_skipfmts.contains(fmt)) {
          klfWarning("User Script Info: format " << fmt << " provided by script is also marked "
                     "as to be skipped!") ;
        }
      }

      if ((us_outputs.contains("eps-processed") || our_skipfmts.contains("eps-processed")) && !settings.wantRaw) {
	our_skipfmts << "eps-bbox";
      }
      if ((us_outputs.contains("eps-bbox") || our_skipfmts.contains("eps-bbox")) && !settings.wantRaw) {
	our_skipfmts << "eps-raw";
      }
      if (us_outputs.contains("svg") || our_skipfmts.contains("svg")) {
	our_skipfmts << "svg-gs"; // don't need svg-gs if we're not generating svg
      }

      klfDbg("us_skipfmts = " << us_skipfmts) ;

      ok = p.run(outdata);

      if (!ok) {
        res.errorstr = p.resultErrorString();
        switch (p.resultStatus()) {
        case KLFFP_NOSTART:       res.status = KLFERR_USERSCRIPT_NORUN; break;
        case KLFFP_NOEXIT:        res.status = KLFERR_USERSCRIPT_NONORMALEXIT; break;
        case KLFFP_NOSUCCESSEXIT: res.status = KLFERR_PROGERR_USERSCRIPT; break;
        case KLFFP_NODATA:        res.status = KLFERR_USERSCRIPT_NOOUTPUT; break;
        case KLFFP_DATAREADFAIL:  res.status = KLFERR_USERSCRIPT_OUTPUTREADFAIL; break;
        default:
          res.status = p.resultStatus();
        }
	return res;
      }

      // make sure all promised files have appeared
      foreach (QString fmt, outfmts) {
        QString corrfname;
	if (fmt == QLatin1String("latex")) {
          corrfname = fnTex;
	} else if (fmt == QLatin1String("dvi")) {
          corrfname = fnDvi;
	} else if (fmt == QLatin1String("eps-raw")) {
          corrfname = fnRawEps;
	} else if (fmt == QLatin1String("eps-bbox")) {
          corrfname = fnBBoxEps;
	} else if (fmt == QLatin1String("eps-processed")) {
          corrfname = fnProcessedEps;
	} else if (fmt == QLatin1String("png")) {
          corrfname = fnRawPng;
	} else if (fmt == QLatin1String("pdf")) {
          corrfname = fnPdf;
	} else if (fmt == QLatin1String("svg-gs")) {
          corrfname = fnGsSvg;
	} else if (fmt == QLatin1String("svg")) {
          corrfname = fnSvg;
        } else {
          klfWarning("Unknown format: " << fmt) ;
          continue;
        }
        if (!QFile::exists(corrfname)) {
          klfWarning("Promised format " << fmt << " did not appear after calling user script.") ;
        }
      }
    }
  }

  klfDbg("our_skipfmts = " << our_skipfmts) ;


  if (!has_userscript_output(us_outputs, "dvi") && !our_skipfmts.contains("dvi")) {
    // execute latex
    klfDbg("preparing to launch latex.") ;

    if (settings.latexexec.isEmpty()) {
      res.status = KLFERR_NOLATEXPROG;
      res.errorstr = QObject::tr("No latex executable given!\n", "KLFBackend");
      return res;
    }

    KLFBackendFilterProgram p(QLatin1String("LaTeX"), &settings, isMainThread, tempdir.path());
    p.resErrCodes[KLFFP_NOSTART] = KLFERR_LATEX_NORUN;
    p.resErrCodes[KLFFP_NOEXIT] = KLFERR_LATEX_NONORMALEXIT;
    p.resErrCodes[KLFFP_NOSUCCESSEXIT] = KLFERR_PROGERR_LATEX;
    p.resErrCodes[KLFFP_NODATA] = KLFERR_LATEX_NOOUTPUT;
    p.resErrCodes[KLFFP_DATAREADFAIL] = KLFERR_LATEX_OUTPUTREADFAIL;

    p.setArgv(QStringList() << settings.latexexec << QDir::toNativeSeparators(fnTex));

    QByteArray userinputforerrors = "h\nr\n";
    
    ok = p.run(userinputforerrors, fnDvi, &res.dvidata);
    if (!ok) {
      p.errorToOutput(&res);
      return res;
    }
  }

  if (!has_userscript_output(us_outputs, "eps-raw") && !our_skipfmts.contains("eps-raw")) {

    ASSERT_HAVE_FORMATS_FOR("eps-raw") ;

    if (settings.dvipsexec.isEmpty()) {
      res.status = KLFERR_NODVIPSPROG;
      res.errorstr = QObject::tr("No dvips executable given!\n", "KLFBackend");
      return res;
    }

    // execute dvips -E
    KLFBackendFilterProgram p(QLatin1String("dvips"), &settings, isMainThread, tempdir.path());
    p.resErrCodes[KLFFP_NOSTART] = KLFERR_DVIPS_NORUN;
    p.resErrCodes[KLFFP_NOEXIT] = KLFERR_DVIPS_NONORMALEXIT;
    p.resErrCodes[KLFFP_NOSUCCESSEXIT] = KLFERR_PROGERR_DVIPS;
    p.resErrCodes[KLFFP_NODATA] = KLFERR_DVIPS_NOOUTPUT;
    p.resErrCodes[KLFFP_DATAREADFAIL] = KLFERR_DVIPS_OUTPUTREADFAIL;

    QFileInfo dvipsinf(settings.dvipsexec);
    if (!dvipsinf.filePath().isEmpty()) {
      // add the explicit dvips path to the PATH environment, in case dvips needs to
      // execute helpers such as mktexpk
      p.addExecEnviron(QStringList() << (
                           QLatin1String("PATH=") + dvipsinf.absoluteFilePath() + QLatin1String(":") +
                           QProcessEnvironment::systemEnvironment().value("PATH")
                           )) ;
    }

    p.setArgv(QStringList() << settings.dvipsexec << "-E" << QDir::toNativeSeparators(fnDvi)
	      << "-o" << QDir::toNativeSeparators(fnRawEps));

    ok = p.run(fnRawEps, &rawepsdata);

    if (!ok) {
      p.errorToOutput(&res);
      return res;
    }
    klfDbg("read raw EPS; rawepsdata/length="<<rawepsdata.size()) ;
  } // end of 'dvips' block

  // the settings requires, save the intermediary data in to result output
  if (settings.wantRaw)
    res.epsdata_raw = rawepsdata;

  // This now also returned in 'res', directly saved there.
  //  // width and height of the (final) EPS bbox in postscript points
  //  double width_pt = 0, height_pt = 0;

  if (!has_userscript_output(us_outputs, "eps-bbox") && !our_skipfmts.contains("eps-bbox")) {
    // find correct bounding box of EPS file, and modify EPS data manually to add boffset and
    // translate to (0,0,width,height)

    ASSERT_HAVE_FORMATS_FOR("eps-bbox") ;

    klfbbox bbox, bbox_corrected;

    if (settings.calcEpsBoundingBox) {
      bool ok = calculate_gs_eps_bbox(QByteArray(), fnRawEps, &bbox, &res, settings, isMainThread);
      if (!ok)
	return res; // res was set by the function
    } else {
      bool ok = read_eps_bbox(rawepsdata, &bbox, &res);
      if (!ok)
	return res; // res was set by the function
    }

    bbox.x1 -= settings.lborderoffset;
    bbox.y1 -= settings.bborderoffset;
    bbox.x2 += settings.rborderoffset;
    bbox.y2 += settings.tborderoffset;

    res.width_pt = bbox.x2 - bbox.x1;
    res.height_pt = bbox.y2 - bbox.y1;

    // now correct the bbox to (0,0,width,height)

    bbox_corrected.x1 = 0;
    bbox_corrected.y1 = 0;
    bbox_corrected.x2 = res.width_pt;
    bbox_corrected.y2 = res.height_pt;

    // and generate corrected raw EPS

    correct_eps_bbox(rawepsdata, bbox_corrected, bbox, in.vectorscale,
		     bgcolor_when_correcting_bbox,  &bboxepsdata);

    klfDbg("corrected bbox to "<<bbox.x1<<","<<bbox.y1<<","<<bbox.x2<<","<<bbox.y2);
  } else if (!our_skipfmts.contains("eps-bbox")) {
    // userscript generated bbox-corrected EPS for us, but we still
    // need to set width_pt and height_pt appropriately.

    klfbbox bb;

    // read from fnRawEps, fnBBoxEps or fnProcessedEps ?
    QString fn;
    //QByteArray *dat;
    if (us_outputs.contains("eps-processed")) {
      fn = fnProcessedEps;
      //dat = & res.epsdata;
    } else {
      fn = fnBBoxEps;
      //dat = & bboxepsdata;
    }

    if (settings.calcEpsBoundingBox) {
      bool ok = calculate_gs_eps_bbox(QByteArray(), fn, &bb, &res, settings, isMainThread);
      if (!ok)
	return res; // res was set by the function
    } else {
      bool ok = read_eps_bbox(bboxepsdata, &bb, &res);
      if (!ok)
	return res; // res was set by the function
    }

    res.width_pt = bb.x2 - bb.x1;
    res.height_pt = bb.y2 - bb.y1;
  } // end 'correct bbox in eps' block

  // the settings requires, save the intermediary data in to result output
  if (settings.wantRaw)
    res.epsdata_bbox = bboxepsdata;
  
  if (!has_userscript_output(us_outputs, "eps-processed") && !our_skipfmts.contains("eps-processed")) {
    // need to process EPS, i.e. outline fonts

    ASSERT_HAVE_FORMATS_FOR("eps-processed") ;

    if (settings.outlineFonts) {
      // post-process EPS file to outline fonts if requested

      if (settings.gsexec.isEmpty()) {
        res.status = KLFERR_NOGSPROG;
        res.errorstr = QObject::tr("No gs executable given!\n", "KLFBackend");
        return res;
      }

      KLFBackendFilterProgram p(QLatin1String("gs (EPS Post-Processing Outline Fonts)"), &settings, isMainThread,
                                tempdir.path());
      p.resErrCodes[KLFFP_NOSTART] = KLFERR_GSPOSTPROC_NORUN;
      p.resErrCodes[KLFFP_NOEXIT] = KLFERR_GSPOSTPROC_NONORMALEXIT;
      p.resErrCodes[KLFFP_NOSUCCESSEXIT] = KLFERR_PROGERR_GSPOSTPROC;
      p.resErrCodes[KLFFP_NODATA] = KLFERR_GSPOSTPROC_NOOUTPUT;
      p.resErrCodes[KLFFP_DATAREADFAIL] = KLFERR_GSPOSTPROC_OUTPUTREADFAIL;

      QString psdevice;
      QStringList gsoptions;
      const char *env_gs_device = getenv("KLFBACKEND_GS_PS_DEVICE");
      if (env_gs_device != NULL) {
        psdevice = QString::fromLatin1(env_gs_device);
        gsoptions = QStringList() << "-dNOCACHE -dNoOutputFonts";
      } else if (thisGsInfo.version_maj < 9 ||
                 (thisGsInfo.version_maj == 9 && thisGsInfo.version_min <= 7)) {
        // until 9.07, we can still use 'pswrite' with -dNOCACHE
        psdevice = QLatin1String("pswrite");
        gsoptions = QStringList() << "-dNOCACHE";
      } else if (thisGsInfo.version_maj > 9 ||
                 (thisGsInfo.version_maj == 9 && thisGsInfo.version_min >= 15)) {
        // Ghostscript removed the pswrite device after 9.07; The ghostscript developers
        // were very responsive and helpful to my feedback, and thanks to their prompt
        // reaction, starting from 9.15, we can use the ps2write device with the option
        // "-dNoOutputFonts".
        psdevice = QLatin1String("ps2write");
        gsoptions = QStringList() << "-dNoOutputFonts";
      } else {
        res.status = KLFERR_GSPOSTPROC_NOOUTLINEFONTS;
        res.errorstr = QObject::tr("Installed Ghostscript version %1 may not be used to create font outlines."
                                   " Please upgrade to gs >= 9.15 (or downgrade to gs <= 9.07).",
                                   "KLFBackend").arg(thisGsInfo.version);
        return res;
      }

      p.addArgv(settings.gsexec);
      p.addArgv(QStringList() << gsoptions
		<< "-dNOPAUSE" << "-dSAFER" << "-dEPSCrop" << QString::fromLatin1("-sDEVICE=%1").arg(psdevice)
		<< "-sOutputFile="+QDir::toNativeSeparators(fnProcessedEps)
		<< "-q" << "-dBATCH" << "-");
      
      ok = p.run(bboxepsdata, fnProcessedEps, &res.epsdata);
      if (!ok) {
	p.errorToOutput(&res);
	return res;
      }

      klfDebugf(("%s: res.epsdata has length=%d", KLF_FUNC_NAME, res.epsdata.size())) ;

    } else {
      // no post-processed EPS, copy raw (bbox-corrected) EPS data:
      res.epsdata = bboxepsdata;
    }
  }

  if (!has_userscript_output(us_outputs, "png") && !our_skipfmts.contains("png")) {

    ASSERT_HAVE_FORMATS_FOR("png") ;

    if (settings.gsexec.isEmpty()) {
      res.status = KLFERR_NOGSPROG;
      res.errorstr = QObject::tr("No gs executable given!\n", "KLFBackend");
      return res;
    }

    // run 'gs' to get PNG data
    KLFBackendFilterProgram p(QLatin1String("gs (PNG)"), &settings, isMainThread, tempdir.path());
    p.resErrCodes[KLFFP_NOSTART] = KLFERR_GSPNG_NORUN;
    p.resErrCodes[KLFFP_NOEXIT] = KLFERR_GSPNG_NONORMALEXIT;
    p.resErrCodes[KLFFP_NOSUCCESSEXIT] = KLFERR_PROGERR_GSPNG;
    p.resErrCodes[KLFFP_NODATA] = KLFERR_GSPNG_NOOUTPUT;
    p.resErrCodes[KLFFP_DATAREADFAIL] = KLFERR_GSPNG_OUTPUTREADFAIL;

    /** \bug .... CORRECT DPI FOR Vector Scale SETTING !!!!!!!!!!!!.............
     *     but do that cleverly; ie. make sure that the EPS was indeed vector-scaled up. Possibly
     *     run the EPS generator twice, once to scale it up, the other for PNG conversion reference.
     */
    // ### wait... do we want vector scaling to apply to the PNG as well??

    p.setArgv(QStringList() << settings.gsexec
	      << "-dNOPAUSE" << "-dSAFER" << "-dTextAlphaBits=4" << "-dGraphicsAlphaBits=4"
	      << "-r"+QString::number(in.dpi) << "-dEPSCrop" << "-dMaxBitmap=2147483647");
    if (qAlpha(in.bg_color) > 0) { // we're forcing a background color
      p.addArgv("-sDEVICE=png16m");
    } else {
      p.addArgv("-sDEVICE=pngalpha");
    }
    p.addArgv(QStringList() << "-sOutputFile="+QDir::toNativeSeparators(fnRawPng) << "-q" << "-dBATCH" << "-");

    ok = p.run(bboxepsdata, fnRawPng, &res.pngdata_raw);
    if (!ok) {
      p.errorToOutput(&res);
      return res;
    }

    res.result.loadFromData(res.pngdata_raw, "PNG");
  } // raw PNG
  else {
    if (us_skipfmts.contains("png")) {
      klfWarning("PNG format was skipped by user script. The QImage object will be invalid.") ;
      res.result = QImage();
      res.pngdata = QByteArray();
      res.pngdata_raw = QByteArray();
    }
    if (!has_userscript_output(us_outputs, "png") || !QFile::exists(fnRawPng)) {
      klfWarning("PNG format is required to initialize the QImage object, but was not generated by user script.") ;
      res.result = QImage();
    } else {
      // load PNG into res.result
      res.result.load(fnRawPng);
    }
  }

  if (!our_skipfmts.contains("png")) { // generate tagged/labeled PNG

    // store some meta-information into result
    KLFImageLatexMetaInfo metainfo(&res.result);
    metainfo.saveMetaInfo(in, settings);
    
    { // create "final" PNG data
      QBuffer buf(&res.pngdata);
      buf.open(QIODevice::WriteOnly);

      bool r = res.result.save(&buf, "PNG");
      if (!r) {
	klfWarning("Can't save \"final\" PNG data.") ;
	res.pngdata = res.pngdata_raw;
      }
    }

    klfDbg("prepared final PNG data.") ;
  }

  if ( settings.wantPDF && !has_userscript_output(us_outputs, "pdf") && !our_skipfmts.contains("pdf") ) {

    ASSERT_HAVE_FORMATS_FOR("pdf") ;

    // prepare PDFMarks
    { QFile fpdfmarks(fnPdfMarks);
      bool r = fpdfmarks.open(QIODevice::WriteOnly);
      if ( ! r ) {
	res.status = KLFERR_PDFMARKSWRITEFAIL;
	res.errorstr = QObject::tr("Can't open file for writing: '%1'!", "KLFBackend").arg(fnPdfMarks);
	return res;
      }
      QByteArray pdfmarkstr;
      KLFPdfmarksWriteLatexMetaInfo pdfmetainfo(&pdfmarkstr);
      pdfmetainfo.savePDFField("Title", in.latex);
      pdfmetainfo.savePDFField("Keywords", "KLatexFormula KLF LaTeX equation formula");
      pdfmetainfo.savePDFField("Creator", "KLatexFormula " KLF_VERSION_STRING);
      pdfmetainfo.saveMetaInfo(in, settings);
      pdfmetainfo.finish();
      fpdfmarks.write(pdfmarkstr);
      // file is ready.
    }

    if (settings.gsexec.isEmpty()) {
      res.status = KLFERR_NOGSPROG;
      res.errorstr = QObject::tr("No gs executable given!\n", "KLFBackend");
      return res;
    }

    // run 'gs' to get PDF data
    KLFBackendFilterProgram p(QLatin1String("gs (PDF)"), &settings, isMainThread, tempdir.path());
    p.resErrCodes[KLFFP_NOSTART] = KLFERR_GSPDF_NORUN;
    p.resErrCodes[KLFFP_NOEXIT] = KLFERR_GSPDF_NONORMALEXIT;
    p.resErrCodes[KLFFP_NOSUCCESSEXIT] = KLFERR_PROGERR_GSPDF;
    p.resErrCodes[KLFFP_NODATA] = KLFERR_GSPDF_NOOUTPUT;
    p.resErrCodes[KLFFP_DATAREADFAIL] = KLFERR_GSPDF_OUTPUTREADFAIL;

    p.setArgv(QStringList() << settings.gsexec
	      << "-dNOPAUSE" << "-dSAFER" << "-sDEVICE=pdfwrite"
	      << "-sOutputFile="+QDir::toNativeSeparators(fnPdf)
	      << "-q" << "-dBATCH" << "-" << fnPdfMarks);

    // input: res.epsdata is the processed EPS file, or the raw EPS + bbox/page correction if no post-processing
    ok = p.run(res.epsdata, fnPdf, &res.pdfdata);
    if (!ok) {
      p.errorToOutput(&res);
      return res;
    }
  }

  if (settings.wantSVG) {

    if (!has_userscript_output(us_outputs, "svg-gs") &&
	!our_skipfmts.contains("svg-gs")) {

      ASSERT_HAVE_FORMATS_FOR("svg-gs") ;

      // run 'gs' to get SVG (raw from gs)
      if (!thisGsInfo.availdevices.contains("svg")) {
	// not OK to get SVG...
	klfWarning("ghostscript cannot create SVG");
	res.status = KLFERR_GSSVG_NOSVG;
	res.errorstr = QObject::tr("This ghostscript (%1) cannot generate SVG.", "KLFBackend").arg(settings.gsexec);
	return res;
      }

      if (settings.gsexec.isEmpty()) {
        res.status = KLFERR_NOGSPROG;
        res.errorstr = QObject::tr("No gs executable given!\n", "KLFBackend");
        return res;
      }

      KLFBackendFilterProgram p(QLatin1String("gs (SVG)"), &settings, isMainThread, tempdir.path());
      p.resErrCodes[KLFFP_NOSTART] = KLFERR_GSSVG_NORUN;
      p.resErrCodes[KLFFP_NOEXIT] = KLFERR_GSSVG_NONORMALEXIT;
      p.resErrCodes[KLFFP_NOSUCCESSEXIT] = KLFERR_PROGERR_GSSVG;
      p.resErrCodes[KLFFP_NODATA] = KLFERR_GSSVG_NOOUTPUT;
      p.resErrCodes[KLFFP_DATAREADFAIL] = KLFERR_GSSVG_OUTPUTREADFAIL;
      
      p.addArgv(settings.gsexec);
      // unconditionally outline fonts, otherwise output is horrible
      p.addArgv(QStringList() << "-dNOCACHE" << "-dNOPAUSE" << "-dSAFER" << "-dEPSCrop" << "-sDEVICE=svg"
		<< "-sOutputFile="+QDir::toNativeSeparators(fnGsSvg)
		<< "-q" << "-dBATCH" << "-");

      // input: res.epsdata is the processed EPS file, or the raw EPS file if no post-processing
      ok = p.run(bboxepsdata, fnGsSvg, &gssvgdata);
      if (!ok) {
	p.errorToOutput(&res);
	return res;
      }
    }

    if (!has_userscript_output(us_outputs, "svg") && !our_skipfmts.contains("svg")) {

      ASSERT_HAVE_FORMATS_FOR("svg") ;

      // and now re-touch SVG generated by ghostscript that is not very clean...
      // find the first occurences of width='' and height='' and set them to the
      // appropriate width and heights given by BBox read earlier

      klfDebugf(("%s: gssvgdata/length=%d", KLF_FUNC_NAME, gssvgdata.size())) ;

      replace_svg_width_or_height(&gssvgdata, "width=", res.width_pt);
      replace_svg_width_or_height(&gssvgdata, "height=", res.height_pt);

      klfDebugf(("%s: now, gssvgdata/length=%d", KLF_FUNC_NAME, gssvgdata.size())) ;

      res.svgdata = gssvgdata;
    }
  } // end if(wantSVG)

  klfDbg("end of function.") ;

  return res;
}



static bool calculate_gs_eps_bbox(const QByteArray& epsData, const QString& epsFile, klfbbox *bbox,
				  KLFBackend::klfOutput * resError, const KLFBackend::klfSettings& settings,
				  bool isMainThread)
{
  KLF_DEBUG_BLOCK(KLF_FUNC_NAME) ;
  // find correct bounding box of EPS file, using ghostscript

  int i;

  if (settings.gsexec.isEmpty()) {
    resError->status = KLFERR_NOGSPROG;
    resError->errorstr = QObject::tr("No gs executable given!\n", "KLFBackend");
    return false;
  }

  KLFBackendFilterProgram p(QLatin1String("GhostScript (bbox)"), &settings, isMainThread, settings.tempdir);
  p.resErrCodes[KLFFP_NOSTART] = KLFERR_GSBBOX_NORUN;
  p.resErrCodes[KLFFP_NOEXIT] = KLFERR_GSBBOX_NONORMALEXIT;
  p.resErrCodes[KLFFP_NOSUCCESSEXIT] = KLFERR_PROGERR_GSBBOX;
  p.resErrCodes[KLFFP_NODATA] = KLFERR_GSBBOX_NOOUTPUT;
  // p.resErrCodes[KLFFP_DATAREADFAIL]  unused (used only for reading output files)

  p.setOutputStdout(true);
  p.setOutputStderr(true);
  
  QByteArray bboxdata;

  p.setArgv(QStringList() << settings.gsexec << "-dNOPAUSE" << "-dSAFER" << "-sDEVICE=bbox" << "-q" << "-dBATCH"
	    << (epsFile.isEmpty() ? QString::fromLatin1("-") : epsFile));

  bool ok = p.run(epsData /*stdin*/, QString() /*no output file*/, &bboxdata/*collect stdout*/);
  if (!ok) {
    p.errorToOutput(resError);
    return false;
  }
  
  klfDbg("gs provided output:\n"<<bboxdata);

  // parse gs' bbox data
  QRegExp rx_gsbbox("%%HiResBoundingBox\\s*:\\s+" D_RX "\\s+" D_RX "\\s+" D_RX "\\s+" D_RX "");
  i = rx_gsbbox.indexIn(QString::fromLatin1(bboxdata));
  if (i < 0) {
    resError->status = KLFERR_GSBBOX_NOBBOX;
    resError->errorstr = QObject::tr("Ghostscript did not provide parsable BBox output!", "KLFBackend");
    return false;
  }
  bbox->x1 = rx_gsbbox.cap(1).toDouble();
  bbox->y1 = rx_gsbbox.cap(2).toDouble();
  bbox->x2 = rx_gsbbox.cap(3).toDouble();
  bbox->y2 = rx_gsbbox.cap(4).toDouble();

  return true;
}


static bool parse_bbox_values(const QString& str, klfbbox *bbox)
{
  // parse bbox values
  QRegExp rx_bbvalues("" D_RX "\\s+" D_RX "\\s+" D_RX "\\s+" D_RX "");
  int i = rx_bbvalues.indexIn(str);
  if (i < 0) {
    return false;
  }
  bbox->x1 = rx_bbvalues.cap(1).toDouble();
  bbox->y1 = rx_bbvalues.cap(2).toDouble();
  bbox->x2 = rx_bbvalues.cap(3).toDouble();
  bbox->y2 = rx_bbvalues.cap(4).toDouble();
  return true;
}

static bool s_starts_with(const char * x, int len_x, const char *test, int len_test)
{
  if (len_x < len_test)
    return false;
  return !strncmp(x, test, len_test);
}

static bool read_eps_bbox(const QByteArray& epsdata, klfbbox *bbox, KLFBackend::klfOutput * resError)
{
  KLF_DEBUG_BLOCK(KLF_FUNC_NAME) ;

  static const char * hibboxtag = "%%HiResBoundingBox:";
  static const char * bboxtag = "%%BoundingBox:";
  static const int hibboxtaglen = strlen(hibboxtag);
  static const int bboxtaglen = strlen(bboxtag);

  // Read dvips' bounding box.
  QBuffer buf;
  buf.setData(epsdata);
  bool r = buf.open(QIODevice::ReadOnly | QIODevice::Text);
  if (!r) {
    klfWarning("What's going on!!?! can't open buffer for reading? Will Fail!!!") ;
  }

  QString nobboxerrstr =
    QObject::tr("DVIPS did not provide parsable %%BoundingBox: in its output!", "KLFBackend");

  char linebuffer[512];
  int n;
  bool gotepsbbox = false;
  int still_look_for_hiresbbox_lines = 5;
  while ((n = buf.readLine(linebuffer, sizeof(linebuffer)-1)) > 0) {
    if (gotepsbbox && still_look_for_hiresbbox_lines-- < 0) {
      // if we already got the %BoundingBox, and we've been looking at more than a certian number of lines
      // after that, abort because usually %BoundingBox and %HiResBoundingBox are together...
      klfDbg("stopped looking for hires-bbox.") ;
      break;
    }
    if (s_starts_with(linebuffer, n, hibboxtag, hibboxtaglen)) {
      // got hi-res bounding-box
      bool ok = parse_bbox_values(QString::fromLatin1(linebuffer+hibboxtaglen), bbox);
      if (!ok) {
        if (resError) {
          resError->status = KLFERR_DVIPS_OUTPUTNOBBOX;
          resError->errorstr = nobboxerrstr;
        }
	return false;
      }
      klfDbg("got hires-bbox.") ;
      // all ok, got hi-res bbox
      return true;
    }
    if (s_starts_with(linebuffer, n, bboxtag, bboxtaglen)) {
      // got bounding-box.
      bool ok = parse_bbox_values(QString::fromLatin1(linebuffer+bboxtaglen), bbox);
      if (!ok) {
	continue;
      }
      // stand by, continue in case we have a hi-res bbox.
      gotepsbbox = true;
      klfDbg("got normal bbox.") ;
      continue;
    }
  }

  // didn't get a hi-res bbox. see if we still got a regular %BoundingBox: and return that.
  if (gotepsbbox) {
    // bbox pointer is already set
    return true;
  }

  if (resError) {
    resError->status = KLFERR_DVIPS_OUTPUTNOBBOX;
    resError->errorstr = nobboxerrstr;
  }
  return false;
}

/** \internal
 * Write the corrected bbox settings to the EPS data.
 *
 * If vectorscale is not 1.0, then the bbox is scaled by the given factor when written to the EPS data.
 */
static void correct_eps_bbox(const QByteArray& rawepsdata, const klfbbox& bbox_corrected,
			     const klfbbox& bbox_orig, double vectorscale, QRgb bgcolor,
			     QByteArray * epsdatacorrected)
{
  KLF_DEBUG_BLOCK(KLF_FUNC_NAME) ;

  static const char * bboxdecl = "%%BoundingBox:";
  static int bboxdecl_len = strlen(bboxdecl);

  double offx = bbox_corrected.x1 - bbox_orig.x1;
  double offy = bbox_corrected.y1 - bbox_orig.y1;

  // in raw EPS data, find '%%BoundingBox:' and length of the full BoundingBox instruction
  int i, len;
  char nl[] = "\0\0\0";
  i = rawepsdata.indexOf(bboxdecl);
  if (i < 0) {
    i = 0;
    len = 0;
  } else {
    int j = i+bboxdecl_len;
    while (j < (int)rawepsdata.size() && rawepsdata[j] != '\r' && rawepsdata[j] != '\n')
      ++j;
    len = j-i;
    if (rawepsdata[j] == '\r' && j < (int)rawepsdata.size()-1 && rawepsdata[j+1] == '\n') {
      nl[0] = '\r', nl[1] = '\n';
    } else {
      nl[0] = rawepsdata[j];
    }
  }

  double dwi = bbox_corrected.x2 * vectorscale;
  double dhi = bbox_corrected.y2 * vectorscale;
  int wi = (int)(dwi + 0.99999) ;
  int hi = (int)(dhi + 0.99999) ;
  char buffer[1024];
  int buffer_len;
  // recall that '%%' in printf is replaced by a single '%'...
  snprintf(buffer, sizeof(buffer)-1,
	   "%%%%BoundingBox: 0 0 %d %d%s"
	   "%%%%HiResBoundingBox: 0 0 %s %s%s",
	   wi, hi, nl,
	   klfFmtDoubleCC(dwi, 'g', 6), klfFmtDoubleCC(dhi, 'g', 6), nl);
  buffer_len = strlen(buffer);

  char backgroundfillps[1024] = "";
  if (qAlpha(bgcolor) > 0) {
    klfDbg("we have a bg color, so draw the background. bgcolor="<<klfFmt("%#10x", (uint)bgcolor));
    snprintf(backgroundfillps, sizeof(backgroundfillps)-1,
	    // draw the background color, if any
	    "newpath "
	    "-2 -2 moveto "
	    "%s -2 lineto "
	    "%s %s lineto "
	    "-2 %s lineto "
	    "closepath "
	    "gsave "
	    "%s %s %s setrgbcolor "
	    "fill "
	    "grestore %s",
	    klfFmtDoubleCC(dwi+1, 'g', 6),
	    klfFmtDoubleCC(dwi+1, 'g', 6), klfFmtDoubleCC(dhi+1, 'g', 6),
	    klfFmtDoubleCC(dhi+1, 'g', 6),
	    // and the color, in RGB components:
	    klfFmtDoubleCC(qRed(bgcolor)/255.0, 'f', 6),
	    klfFmtDoubleCC(qGreen(bgcolor)/255.0, 'f', 6),
	    klfFmtDoubleCC(qBlue(bgcolor)/255.0, 'f', 6),
	    nl
	  );
  }

  char buffer2[1024];
  //int buffer2_len;
  snprintf(buffer2, sizeof(buffer2)-1,
	   "%s"
	   "%%%%Page 1 1%s"
	   "%%%%PageBoundingBox 0 0 %d %d%s"
	   "<< /PageSize [%d %d] >> setpagedevice%s"
	   "%s"
	   "%s %s scale%s"
	   "%s %s translate%s"
	   ,
	   nl,
	   nl,
	   wi, hi, nl,
	   wi, hi, nl,
	   backgroundfillps,
	   klfFmtDoubleCC(vectorscale, 'f'), klfFmtDoubleCC(vectorscale, 'f'), nl,
	   klfFmtDoubleCC(offx, 'f'), klfFmtDoubleCC(offy, 'f'), nl);
  //buffer2_len = strlen(buffer2);

  //    char buffer2[128];
  //    snprintf(buffer2, 127, "%sgrestore%s", nl, nl);

  klfDbg("buffer is `"<<buffer<<"', length="<<buffer_len) ;
  klfDbg("rawepsdata has length="<<rawepsdata.size()) ;

  // and modify the raw EPS data, to replace "%%BoundingBox:" instruction by our stuff...
  QByteArray neweps;
  neweps = rawepsdata;
  neweps.replace(i, len, buffer);

  const char * endsetupstr = "%%EndSetup";
  int i2 = neweps.indexOf(endsetupstr);
  if (i2 < 0)
    i2 = i + buffer_len; // add our info after modified %%BoundingBox'es instructions if %%EndSetup not found
  else
    i2 +=  strlen(endsetupstr);

  neweps.replace(i2, 0, buffer2);
  
  klfDbg("neweps has now length="<<neweps.size());
  klfDebugf(("New eps bbox is [0 0 %.6g %.6g] with translate [%.6g %.6g] and scale %.6g.",
	     dwi, dhi, offx, offy, vectorscale));

  *epsdatacorrected = neweps;
}


static void replace_svg_width_or_height(QByteArray *svgdata, const char * attreq, double val)
{
  QByteArray & svgdataref = * svgdata;

  int i = svgdataref.indexOf(attreq);
  int j = i;
  while (j < (int)svgdataref.size() && (!isspace(svgdataref[j]) && svgdataref[j] != '>'))
    ++j;

  char buffer[1024];
  snprintf(buffer, sizeof(buffer)-1, "%s'%s'", attreq, klfFmtDoubleCC(val, 'f', 3));

  svgdata->replace(i, j-i, buffer);
}
   




KLF_EXPORT bool operator==(const KLFBackend::klfInput& a, const KLFBackend::klfInput& b)
{
  return a.latex == b.latex &&
    a.mathmode == b.mathmode &&
    a.preamble == b.preamble &&
    fabs(a.fontsize - b.fontsize) < 0.001 &&
    a.fg_color == b.fg_color &&
    a.bg_color == b.bg_color &&
    a.dpi == b.dpi &&
    fabs(a.vectorscale - b.vectorscale) < 0.001 &&
    a.bypassTemplate == b.bypassTemplate &&
    a.userScript == b.userScript;
}

KLF_EXPORT bool operator==(const KLFBackend::klfSettings& a, const KLFBackend::klfSettings& b)
{
  return
    a.tempdir == b.tempdir &&
    a.latexexec == b.latexexec &&
    a.dvipsexec == b.dvipsexec &&
    a.gsexec == b.gsexec &&
    a.epstopdfexec == b.epstopdfexec &&
    a.tborderoffset == b.tborderoffset &&
    a.rborderoffset == b.rborderoffset &&
    a.bborderoffset == b.bborderoffset &&
    a.lborderoffset == b.lborderoffset &&
    a.calcEpsBoundingBox == b.calcEpsBoundingBox &&
    a.outlineFonts == b.outlineFonts &&
    a.wantRaw == b.wantRaw &&
    a.wantPDF == b.wantPDF &&
    a.wantSVG == b.wantSVG &&
    a.execenv == b.execenv &&
    a.templateGenerator == b.templateGenerator ;
}


QStringList KLFBackend::availableSaveFormats(const klfOutput * output)
{
  if (output != NULL)
    return availableSaveFormats(*output);

  QStringList fmts;
  fmts << "PNG" << "PS" << "EPS" << "DVI" << "PDF" << "SVG";

  QList<QByteArray> imgfmts = QImageWriter::supportedImageFormats();
  foreach (QByteArray f, imgfmts) {
    f = f.trimmed().toUpper();
    if (f == "JPG")
      f = "JPEG"; // only report "JPEG", not both "JPG" and "JPEG"
    if (fmts.contains(f))
      continue;
    fmts << QString::fromLatin1(f);
  }
  return fmts;
}

QStringList KLFBackend::availableSaveFormats(const klfOutput& klfoutput)
{
  QStringList formats;
  // Most popular formats on top of list (pdf, png)
  if (!klfoutput.pdfdata.isEmpty())
    formats << "PDF";
  if (!klfoutput.pngdata.isEmpty())
    formats << "PNG";
  if (!klfoutput.svgdata.isEmpty())
    formats << "SVG";
  if (!klfoutput.epsdata.isEmpty())
    formats << "PS" << "EPS";
  if (!klfoutput.dvidata.isEmpty())
    formats << "DVI";
  // and, of course, all Qt-available image formats
  QList<QByteArray> imgfmts = QImageWriter::supportedImageFormats();
  foreach (QByteArray f, imgfmts) {
    f = f.trimmed().toUpper();
    if (f == "JPG")
      f = "JPEG"; // only report "JPEG", not both "JPG" and "JPEG"
    if (formats.contains(f))
      continue;
    formats << QString::fromLatin1(f);
  }
  return formats;
}

bool KLFBackend::saveOutputToDevice(const klfOutput& klfoutput, QIODevice *device,
				    const QString& fmt, QString *errorStringPtr)
{
  QString format = fmt.trimmed().toUpper();

  // now choose correct data source and write to fout
  if (format == "PNG") {
    device->write(klfoutput.pngdata);
  } else if (format == "EPS" || format == "PS") {
    device->write(klfoutput.epsdata);
  } else if (format == "DVI") {
    device->write(klfoutput.dvidata);
  } else if (format == "PDF") {
    if (klfoutput.pdfdata.isEmpty()) {
      QString error = QObject::tr("PDF format is not available!",
				  "KLFBackend::saveOutputToFile");
      qWarning("%s", qPrintable(error));
      if (errorStringPtr != NULL)
	errorStringPtr->operator=(error);
      return false;
    }
    device->write(klfoutput.pdfdata);
  } else if (format == "SVG") {
    if (klfoutput.svgdata.isEmpty()) {
      QString error = QObject::tr("SVG format is not available!",
				  "KLFBackend::saveOutputToFile");
      qWarning("%s", qPrintable(error));
      if (errorStringPtr != NULL)
	errorStringPtr->operator=(error);
      return false;
    }
    device->write(klfoutput.svgdata);
 } else {
    bool res = klfoutput.result.save(device, format.toLatin1());
    if ( ! res ) {
      QString errstr = QObject::tr("Unable to save image in format `%1'!",
				   "KLFBackend::saveOutputToDevice").arg(format);
      qWarning("%s", qPrintable(errstr));
      if (errorStringPtr != NULL)
	*errorStringPtr = errstr;
      return false;
    }
  }

  return true;
}

bool KLFBackend::saveOutputToFile(const klfOutput& klfoutput, const QString& fileName,
				  const QString& fmt, QString *errorStringPtr)
{
  QString format = fmt;
  // determine format first
  if (format.isEmpty() && !fileName.isEmpty()) {
    QFileInfo fi(fileName);
    if ( ! fi.suffix().isEmpty() )
      format = fi.suffix();
  }
  if (format.isEmpty())
    format = QLatin1String("PNG");
  format = format.trimmed().toUpper();
  // got format. choose output now and prepare write
  QFile fout;
  if (fileName.isEmpty() || fileName == "-") {
    if ( ! fout.open(stdout, QIODevice::WriteOnly) ) {
      QString error = QObject::tr("Unable to open stdout for write! Error: %1",
				  "KLFBackend::saveOutputToFile").arg(fout.error());
      qWarning("%s", qPrintable(error));
      if (errorStringPtr != NULL)
	*errorStringPtr = error;
      return false;
    }
  } else {
    fout.setFileName(fileName);
    if ( ! fout.open(QIODevice::WriteOnly) ) {
      QString error = QObject::tr("Unable to write to file `%1'! Error: %2",
				  "KLFBackend::saveOutputToFile")
	.arg(fileName).arg(fout.error());
      qWarning("%s", qPrintable(error));
      if (errorStringPtr != NULL)
	*errorStringPtr = error;
      return false;
    }
  }

  return saveOutputToDevice(klfoutput, &fout, format, errorStringPtr);
}


bool KLFBackend::detectSettings(klfSettings *settings, const QString& extraPath, bool isMainThread)
{
  KLF_DEBUG_TIME_BLOCK(KLF_FUNC_NAME) ;

  QStringList stdextrapaths;
  int k, j;
  for (k = 0; standard_extra_paths[k] != NULL; ++k) {
    stdextrapaths.append(standard_extra_paths[k]);
  }
  QString extra_paths = stdextrapaths.join(QString("")+KLF_PATH_SEP);
  if (!extraPath.isEmpty())
    extra_paths += KLF_PATH_SEP + extraPath;

  // temp dir
  settings->tempdir = QDir::fromNativeSeparators(QDir::tempPath());

  // sensible defaults
  settings->lborderoffset = 1;
  settings->tborderoffset = 1;
  settings->rborderoffset = 1;
  settings->bborderoffset = 1;
  
  settings->epstopdfexec = QString(); // obsolete, no longer used

  // you'll want PDF
  settings->wantPDF = true;

  settings->wantSVG = false; // will be set to TRUE once we verify 'gs' available devices information

  // find executables
  struct { QString * target_setting; QStringList prog_names; }  progs_to_find[] = {
    { & settings->latexexec, progLATEX },
    { & settings->dvipsexec, progDVIPS },
    { & settings->gsexec, progGS },
    //    { & settings->epstopdfexec, progEPSTOPDF },
    { NULL, QStringList() }
  };
  // replace @executable_path in extra_paths
  klfDbg(klfFmtCC("Our base extra paths are: %s", qPrintable(extra_paths))) ;
  QString ourextrapaths = extra_paths;
  ourextrapaths.replace("@executable_path", qApp->applicationDirPath());
  klfDbg(klfFmtCC("Our extra paths are: %s", qPrintable(ourextrapaths))) ;
  // and actually search for those executables
  for (k = 0; progs_to_find[k].target_setting != NULL; ++k) {
    klfDbg("Looking for "+progs_to_find[k].prog_names.join(" or ")) ;
    for (j = 0; j < (int)progs_to_find[k].prog_names.size(); ++j) {
      klfDbg("Testing `"+progs_to_find[k].prog_names[j]+"'") ;
      *progs_to_find[k].target_setting
	= klfSearchPath(progs_to_find[k].prog_names[j], ourextrapaths);
      if (!progs_to_find[k].target_setting->isEmpty()) {
	klfDbg("Found! at `"+ *progs_to_find[k].target_setting+"'") ;
	break; // found a program
      }
    }
  }

  bool r1 = detectOptionSettings(settings, isMainThread);

  bool result_failure =
    settings->tempdir.isEmpty() || settings->latexexec.isEmpty() || settings->dvipsexec.isEmpty() ||
    settings->gsexec.isEmpty() || !r1;

  return !result_failure;
}

KLF_EXPORT bool KLFBackend::detectOptionSettings(klfSettings * settings, bool isMainThread)
{
  KLF_DEBUG_TIME_BLOCK(KLF_FUNC_NAME) ;

  bool r0 = klf_detect_execenv(settings);
  if (!r0) {
    return false;
  }

  settings->wantSVG = false;

  bool ok = true;
  if (settings->gsexec.length()) {
    initGsInfo(settings, isMainThread);
    if (!gsInfo.contains(settings->gsexec)) {
      klfWarning("Cannot get 'gs' devices information with "<<(settings->gsexec+" --version/--help"));
      ok = false;
    } else if (gsInfo[settings->gsexec].availdevices.contains("svg")) {
      settings->wantSVG = true;
    }
  }

  return ok;
}


KLF_EXPORT bool klf_detect_execenv(KLFBackend::klfSettings *settings)
{
  KLF_DEBUG_TIME_BLOCK(KLF_FUNC_NAME) ;

  // detect mgs.exe as ghostscript and setup its environment properly
  QFileInfo gsfi(settings->gsexec);
  if (gsfi.fileName() == "mgs.exe") {
    QString mgsenv = QString("")
      + QDir::toNativeSeparators(gsfi.absolutePath()+"/../../ghostscript/base")
      + QString(KLF_PATH_SEP)
      + QDir::toNativeSeparators(gsfi.absolutePath()+"/../../fonts");
    settings->execenv = klfSetEnvironmentVariable(settings->execenv, "MIKTEX_GS_LIB", mgsenv);
    klfDbg("Adjusting environment for mgs.exe: `MIKTEX_GS_LIB="+mgsenv+"'") ;
  }

  return true;
}




// static 
void initGsInfo(const KLFBackend::klfSettings *settings, bool isMainThread)
{
  KLF_DEBUG_TIME_BLOCK(KLF_FUNC_NAME) ;

  if (gsInfo.contains(settings->gsexec)) // info already cached
    return;

  if (settings->gsexec.isEmpty()) {
    // no GS executable given
    return;
  }

  QString gsver;
  { // test 'gs' version, to see if we can provide SVG data
    KLFBackendFilterProgram p(QLatin1String("gs (test version)"), settings, isMainThread, settings->tempdir);
    //    p.resErrCodes[KLFFP_NOSTART] = ;
    //     p.resErrCodes[KLFFP_NOEXIT] = ;
    //     p.resErrCodes[KLFFP_NOSUCCESSEXIT] = ;
    //     p.resErrCodes[KLFFP_NODATA] = ;
    //     p.resErrCodes[KLFFP_DATAREADFAIL] = ;
    
    p.setExecEnviron(settings->execenv);
    
    // I think there is a problem with app events processing if this is run during application start-up
    p.setProcessAppEvents(false);

    p.setArgv(QStringList() << settings->gsexec << "--version");
    
    QByteArray ba_gsver;
    bool ok = p.run(QString(), &ba_gsver);
    if (ok) {
      gsver = QString::fromLatin1(ba_gsver);
      klfDbg("gs version text (untrimmed): "<<gsver) ;

      gsver = gsver.trimmed();
    }
  }

  QString gshelp;
  KLFStringSet availdevices;
  { // test 'gs' version, to see if we can provide SVG data
    KLFBackendFilterProgram p(QLatin1String("gs (query help)"), settings, isMainThread, settings->tempdir);
    //    p.resErrCodes[KLFFP_NOSTART] = ;
    //     p.resErrCodes[KLFFP_NOEXIT] = ;
    //     p.resErrCodes[KLFFP_NOSUCCESSEXIT] = ;
    //     p.resErrCodes[KLFFP_NODATA] = ;
    //     p.resErrCodes[KLFFP_DATAREADFAIL] = ;
    
    QStringList ee = settings->execenv;
    // make sure we have gs' output in english
    ee = klfSetEnvironmentVariable(ee, QLatin1String("LANG"), QLatin1String("en_US.UTF-8"));
    p.setExecEnviron(ee);
    
    // I think there is a problem with app events processing if this is run during application start-up
    p.setProcessAppEvents(false);

    p.setArgv(QStringList() << settings->gsexec << "--help");
    
    QByteArray ba_gshelp;
    bool ok = p.run(QString(), &ba_gshelp);
    if (ok) {
      gshelp = QString::fromLatin1(ba_gshelp);

      klfDbg("gs help text: "<<gshelp) ;
      // parse available devices
      const char * availdevstr = "Available devices:";
      int k = gshelp.indexOf(availdevstr, 0, Qt::CaseInsensitive) ;
      if (k == -1) {
	klfWarning("Unable to parse gs' available devices.") ;
      } else {
	k += strlen(availdevstr); // point to after 'available devices:' string
	// find end of available devices list, given by a line not starting with whitespace
	int kend = gshelp.indexOf(QRegExp("\\n\\S"), k);
	if (kend == -1)
	  kend = gshelp.length();
	// now split this large string into the devices list
	QStringList devlist = gshelp.mid(k, kend-k).split(QRegExp("(\\s|[\r\n])+"), QString::SkipEmptyParts);
	availdevices = KLFStringSet::fromList(devlist);
	klfDbg("Detected devices: "<<availdevices) ;
      }
    }
  }

  int gsvermaj = -1;
  int gsvermin = -1;
  QRegExp rx_version("^(\\d+)\\.(\\d+)");
  int foundver = rx_version.indexIn(gsver);
  if (foundver >= 0) {
    gsvermaj = rx_version.cap(1).toInt();
    gsvermin = rx_version.cap(2).toInt();
  }

  klfDbg("GS Version: "<<gsver<<" = "<<gsvermaj<<"."<<gsvermin);

  GsInfo i;
  i.version = gsver;
  i.version_maj = gsvermaj;
  i.version_min = gsvermin;
  i.help = gshelp;
  i.availdevices = availdevices;

  gsInfo[settings->gsexec] = i;
}





KLF_EXPORT QStringList klfSettingsToEnvironmentForUserScript(const KLFBackend::klfSettings& settings)
{
  QStringList env;
  env << "KLF_SETTINGS_BORDEROFFSET=" + klfFmt("%.3g,%.3g,%.3g,%.3g pt", settings.tborderoffset,
					       settings.rborderoffset, settings.bborderoffset, settings.lborderoffset)
      << "KLF_SETTINGS_OUTLINEFONTS=" + QString::fromLatin1(settings.outlineFonts ? "1" : "0")
      << "KLF_SETTINGS_CALCEPSBOUNDINGBOX=" + QString::fromLatin1(settings.calcEpsBoundingBox ? "1" : "0")
      << "KLF_SETTINGS_WANT_RAW=" + QString::fromLatin1(settings.wantRaw ? "1" : "0")
      << "KLF_SETTINGS_WANT_PDF=" + QString::fromLatin1(settings.wantPDF ? "1" : "0")
      << "KLF_SETTINGS_WANT_SVG=" + QString::fromLatin1(settings.wantSVG ? "1" : "0");
  return env;
}

KLF_EXPORT QStringList klfInputToEnvironmentForUserScript(const KLFBackend::klfInput& in)
{
  QStringList env;
  QString fgcol = QString("rgba(%1,%2,%3,%4)").arg(qRed(in.fg_color))
    .arg(qGreen(in.fg_color)).arg(qBlue(in.fg_color)).arg(qAlpha(in.fg_color));
  QString bgcol = QString("rgba(%1,%2,%3,%4)").arg(qRed(in.bg_color))
    .arg(qGreen(in.bg_color)).arg(qBlue(in.bg_color)).arg(qAlpha(in.bg_color));
  env << "KLF_INPUT_LATEX=" + in.latex
      << "KLF_INPUT_MATHMODE=" + in.mathmode
      << "KLF_INPUT_PREAMBLE=" + in.preamble
      << "KLF_INPUT_FONTSIZE=" + QString::number(in.fontsize)
      << "KLF_INPUT_FG_COLOR_WEB=" + QColor(in.fg_color).name()
      << "KLF_INPUT_FG_COLOR_RGBA=" + fgcol
      << "KLF_INPUT_BG_COLOR_TRANSPARENT=" + QString::fromLatin1(qAlpha(in.bg_color) > 50 ? "0" : "1")
      << "KLF_INPUT_BG_COLOR_WEB=" + QColor(in.bg_color).name()
      << "KLF_INPUT_BG_COLOR_RGBA=" + bgcol
      << "KLF_INPUT_DPI=" + QString::number(in.dpi)
      << "KLF_INPUT_VECTORSCALE=" + klfFmt("%.6g", in.vectorscale)
      << "KLF_INPUT_USERSCRIPT=" + in.userScript
      << "KLF_INPUT_BYPASS_TEMPLATE=" + QString::number(in.bypassTemplate)
    ;
  
  // and add custom user parameters
  QMap<QString,QString>::const_iterator cit;
  for (cit = in.userScriptParam.constBegin(); cit != in.userScriptParam.constEnd(); ++cit) {
    env << "KLF_ARG_"+cit.key() + "=" + cit.value();
  }

  return env;
}
