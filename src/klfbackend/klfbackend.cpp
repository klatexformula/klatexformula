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

#include <QtGlobal>
#include <QByteArray>
#include <QSet>
#include <QApplication>
#include <QRegExp>
#include <QFile>
#include <QDateTime>
#include <QTextStream>
#include <QBuffer>
#include <QDir>
#include <QColor>
#include <QTextDocument>
#include <QImageWriter>

#include <klfutil.h>
#include <klfdatautil.h>

#include "klfblockprocess.h"
#include "klfbackend.h"

// legacy macros that used to be universal for both Qt 3 and 4
// but qt3 support was dropped as of klf 3.3
#include "../klftools/klflegacymacros_p.h"


/** \mainpage
 *
 * <div style="width: 60%; padding: 0 20%; text-align: justify; line-height: 150%">
 * This documentation is the API documentation for the KLatexFormula library backend that
 * you may want to use in your programs. It is a GPL-licensed library based on QT 3 or QT 4 that
 * converts a LaTeX equation given as text into graphics, specifically PNG, EPS or PDF (and the
 * image is available as a QImage&mdash;so any format supported by Qt is available.
 *
 * Some utilities to save the output (in various formats) to a file or a QIODevice
 * are provided, see KLFBackend::saveOutputToFile() and KLFBackend::saveOutputToDevice().
 *
 * All the core functionality is based in the class \ref KLFBackend . Some extra general utilities are
 * available in \ref klfdefs.h , such as klfVersionCompare(), \ref KLFSysInfo, \ref klfDbg,
 * klfFmt(), klfSearchFind(), etc.
 *
 * This library will compile indifferently on QT 3 and QT 4 with the same source code.
 * The base API is the same, although some specific overloaded functions may differ or not be available
 * in either version for Qt 3 or Qt 4. Those members are documented as such.
 *
 * To compile with Qt 4, you may add \c KLFBACKEND_QT4 to your defines, ie. pass option \c -DKLFBACKEND_QT4
 * to gcc; however this is set automatically in \ref klfdefs.h if Qt 4 is detected.
 *
 * This library has been tested to work in non-GUI applications (ie. FALSE in QApplication constructor,
 * or with QCoreApplication in Qt 4).
 *</div>
 */



/** \page pageUserScript User Scripts
 *
 * \todo ......... WRITE DOC................
 *
 * Format list (e.g. for 'spitsOut()' and 'skipFormats()':
 * \todo FORMAT LIST
 *
 * \bug NEEDS TESTING: user script output formats, skipped formats, parameters, etc.
 */



// some standard guess settings for system configurations

#ifdef KLF_EXTRA_SEARCH_PATHS
#  define EXTRA_PATHS_PRE	KLF_EXTRA_SEARCH_PATHS ,
#  define EXTRA_PATHS		KLF_EXTRA_SEARCH_PATHS
#else
#  define EXTRA_PATHS_PRE
#  define EXTRA_PATHS
#endif


#if defined(Q_OS_WIN32) || defined(Q_OS_WIN64)
QStringList progLATEX = QStringList() << "latex.exe";
QStringList progDVIPS = QStringList() << "dvips.exe";
QStringList progGS = QStringList() << "gswin32c.exe" << "mgs.exe";
//QStringList progEPSTOPDF = QStringList() << "epstopdf.exe";
static const char * standard_extra_paths[] = {
  EXTRA_PATHS_PRE
  "C:\\Program Files\\MiKTeX*\\miktex\\bin",
  "C:\\Program Files\\gs\\gs*\\bin",
  NULL
};
#elif defined(Q_WS_MAC)
QStringList progLATEX = QStringList() << "latex";
QStringList progDVIPS = QStringList() << "dvips";
QStringList progGS = QStringList() << "gs";
//QStringList progEPSTOPDF = QStringList() << "epstopdf";
static const char * standard_extra_paths[] = {
  EXTRA_PATHS_PRE
  "/usr/texbin:/usr/local/bin:/sw/bin:/sw/usr/bin",
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

static void cleanup(QString tempfname);

static QMutex klf_mutex;

struct GsInfo
{
  GsInfo() { }

  QString version;
  QString help;
  QSet<QString> availdevices;
};

// cache gs version/help/etc. information (for each gs executable, in case there are several)
static QMap<QString,GsInfo> gsInfo = QMap<QString,GsInfo>();

static void initGsInfo(const KLFBackend::klfSettings *settings);


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

QString KLFBackend::DefaultTemplateGenerator::generateTemplate(const klfInput& in, const klfSettings& /*settings*/)
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
  s += QString("\\definecolor{klffgcolor}{rgb}{%1,%2,%3}\n").arg(qRed(in.fg_color)/255.0)
    .arg(qGreen(in.fg_color)/255.0).arg(qBlue(in.fg_color)/255.0);
  s += QString("\\definecolor{klfbgcolor}{rgb}{%1,%2,%3}\n").arg(qRed(in.bg_color)/255.0)
    .arg(qGreen(in.bg_color)/255.0).arg(qBlue(in.bg_color)/255.0);
  if (qAlpha(in.bg_color)>0)
    s += "\\pagecolor{klfbgcolor}\n";
  s += "{\\color{klffgcolor} ";
  s += latexin;
  s += "\n}\n"
    "\\end{document}\n";

  return s;
}




// ---------------------------------

KLFBackend::KLFBackend()
{
}


// Utility function
static QString progErrorMsg(QString progname, int exitstatus, QString stderrstr, QString stdoutstr)
{
  QString stdouthtml = stdoutstr;
  QString stderrhtml = stderrstr;
  stdouthtml.replace("&", "&amp;");
  stdouthtml.replace("<", "&lt;");
  stdouthtml.replace(">", "&gt;");
  stderrhtml.replace("&", "&amp;");
  stderrhtml.replace("<", "&lt;");
  stderrhtml.replace(">", "&gt;");

  if (stderrstr.isEmpty() && stdoutstr.isEmpty())
    return QObject::tr("<p><b>%1</b> reported an error (exit status %2). No Output was generated.</p>",
		       "KLFBackend")
	.arg(progname).arg(exitstatus);
  if (stderrstr.isEmpty())
    return
      QObject::tr("<p><b>%1</b> reported an error (exit status %2). Here is full stdout output:</p>\n"
		  "<pre>\n%3</pre>", "KLFBackend")
      .arg(progname).arg(exitstatus).arg(stdouthtml);
  if (stdoutstr.isEmpty())
    return
      QObject::tr("<p><b>%1</b> reported an error (exit status %2). Here is full stderr output:</p>\n"
		  "<pre>\n%3</pre>", "KLFBackend")
      .arg(progname).arg(exitstatus).arg(stderrhtml);
  
  return QObject::tr("<p><b>%1</b> reported an error (exit status %2). Here is full stderr output:</p>\n"
		     "<pre>\n%3</pre><p>And here is full stdout output:</p><pre>\n%4</pre>", "KLFBackend")
    .arg(progname).arg(exitstatus).arg(stderrhtml).arg(stdouthtml);
}


/* * Internal.
 * \internal */
struct cleanup_caller {
  QString tempfname;
  cleanup_caller(QString fn) : tempfname(fn) { }
  ~cleanup_caller() {
    cleanup(tempfname);
  }
};

static QString klf_expand_env_vars(const QString& envexpr)
{
  QString s = envexpr;
  QRegExp rx("\\$(?:(\\$|(?:[A-Za-z0-9_]+))|\\{([A-Za-z0-9_]+)\\})");
  int i = 0;
  while ( (i = rx.rx_indexin_i(s, i)) != -1 ) {
    // match found, replace it
    QString envvarname = rx.cap(1);
    if (envvarname.isEmpty() || envvarname == QLatin1String("$")) {
      // note: empty variable name expands to a literal '$'
      s.replace(i, rx.matchedLength(), QLatin1String("$"));
      i += 1;
      continue;
    }
    const char *svalue = getenv(qPrintable(envvarname));
    QString qsvalue = (svalue != NULL) ? QString::fromLocal8Bit(svalue) : QString();
    s.replace(i, rx.matchedLength(), qsvalue);
    i += qsvalue.length();
  }
  // replacements performed
  return s;
}

static void klf_append_replace_env_var(QStringList *list, const QString& var, const QString& line)
{
  // search for declaration of var in list
  int k;
  for (k = 0; k < (int)list->size(); ++k) {
    if (list->operator[](k).startsWith(var+QString("="))) {
      list->operator[](k) = line;
      return;
    }
  }
  // declaration not found, just append
  list->append(line);
}



#define KLFFP_NOERR 0
#define KLFFP_NOSTART 1
#define KLFFP_NOEXIT 2
#define KLFFP_NOSUCCESSEXIT 3
#define KLFFP_NODATA 4
#define KLFFP_DATAREADFAIL 5
#define KLFFP_PAST_LAST_VALUE 6

/** \internal */
struct KLFFilterProgram {

  KLFFilterProgram(const QString& pTitle = QString(), const KLFBackend::klfSettings *settings = NULL)
    : progTitle(pTitle), outputStdout(true), outputStderr(false), exitStatus(-1), exitCode(-1)
  {
    for (int i = 0; i < KLFFP_PAST_LAST_VALUE; ++i) {
      resErrCodes[i] = i;
    }

    if (settings != NULL) {
      programCwd = settings->tempdir;
      execEnviron = settings->execenv;
    }

    collectStdout = NULL;
    collectStderr = NULL;
  }

  QString progTitle;
  int resErrCodes[KLFFP_PAST_LAST_VALUE];
  QString programCwd;
  QStringList execEnviron;

  QStringList argv;

  /** Set this to false to ignore output on stdout of the program. */
  bool outputStdout;
  /** Set this to true to also read stderr as part of the output. If false (the default), stderr
   * output is only reported in the error message in case nothing came out on stdout. */
  bool outputStderr;

  QByteArray *collectStdout;
  QByteArray *collectStderr;

  // these fields are set after calling run()
  int exitStatus;
  int exitCode;

  /** \internal
   */
  bool run(const QString& outFileName, QByteArray *outdata, KLFBackend::klfOutput *resError)
  {
    return run(QByteArray(), outFileName, outdata, resError);
  }

  /** \internal
   *
   */
  bool run(const QByteArray& indata, const QString& outFileName, QByteArray *outdata,
	   KLFBackend::klfOutput *resError)
  {
    QMap<QString,QByteArray*> fout; fout[outFileName] = outdata;
    return run(indata, fout, resError);
  }

  /** \internal
   */
  bool run(const QMap<QString, QByteArray*> outdata, KLFBackend::klfOutput *resError)
  {
    return run(QByteArray(), outdata, resError);
  }

  /**
   * \internal
   *
   * \note multiple output files possible. Each data is retreived into a pointer given by outdata map.
   *
   * \param indata a QByteArray to write into the program's standard input
   * \param outdata a QMap with keys being files that are created by the program. These files are read and
   *   their contents stored in the QByteArray's pointed by the corresponding pointer.
   * \param resError the klfOutput object is initialized to the corresponding error if an error occurred.
   *
   * \returns TRUE/FALSE for success/failure, respectively.
   */
  bool run(const QByteArray& indata, const QMap<QString, QByteArray*> outdatalist, KLFBackend::klfOutput *resError)
  {
    KLFBlockProcess proc;

    exitCode = 0;
    exitStatus = 0;

    KLF_ASSERT_CONDITION(argv.size() > 0, "argv array is empty! No program is given!", return false; ) ;

    proc.setWorkingDirectory(programCwd);

    klfDbg("about to exec "<<progTitle<<" ...") ;
    klfDbg("\t"<<qPrintable(argv.join(" "))) ;
    bool r = proc.startProcess(argv, indata, execEnviron);
    klfDbg(progTitle<<" returned.") ;

    if (!r) {
      qDebug("%s: cannot launch %s", KLF_FUNC_NAME, qPrintable(progTitle));
      if (resError != NULL) {
	resError->status = resErrCodes[KLFFP_NOSTART];
	resError->errorstr = QObject::tr("Unable to start %1 program `%2'!", "KLFBackend").arg(progTitle, argv[0]);
      }
      return false;
    }
    if (!proc.processNormalExit()) {
      qDebug("%s: %s did not exit normally (crashed)", KLF_FUNC_NAME, qPrintable(progTitle));
#ifdef KLFBACKEND_QT4
      exitStatus = proc.exitStatus();
#else
      exitStatus = 1;
#endif
      exitCode = -1;
      if (resError != NULL) {
	resError->status = resErrCodes[KLFFP_NOEXIT];
	resError->errorstr = QObject::tr("Program %1 crashed!", "KLFBackend").arg(progTitle);
      }
      return false;
    }
    if (proc.processExitStatus() != 0) {
      exitStatus = 0;
      exitCode = proc.processExitStatus();
      qDebug("%s: %s exited with code %d", KLF_FUNC_NAME, qPrintable(progTitle), exitCode);
      if (resError != NULL) {
	resError->status = resErrCodes[KLFFP_NOSUCCESSEXIT];
	resError->errorstr = progErrorMsg(progTitle, proc.processExitStatus(), proc.readStderrString(),
					  proc.readStdoutString());
      }
      return false;
    }

    if (collectStdout != NULL)
      *collectStdout = proc.getAllStdout();
    if (collectStderr != NULL)
      *collectStderr = proc.getAllStderr();

    for (QMap<QString,QByteArray*>::const_iterator it = outdatalist.begin(); it != outdatalist.end(); ++it) {
      QString outFileName = it.key();
      QByteArray * outdata = it.value();
      
      KLF_ASSERT_NOT_NULL(outdata, "Given NULL outdata pointer for file "<<outFileName<<" !", return false; ) ;

      klfDbg("Will collect output in file "<<(outFileName.isEmpty()?QString("(stdout)"):outFileName)
	     <<" to its corresponding QByteArray pointer="<<outdata) ;

      if (outFileName.isEmpty()) {
	// empty outFileName means to use standard output
	*outdata = QByteArray();
	if (outputStdout) {
	  QByteArray stdoutdata = (collectStdout != NULL) ? *collectStdout : proc.getAllStdout();
	  ba_cat(outdata, stdoutdata);
	}
	if (outputStderr) {
	  QByteArray stderrdata = (collectStderr != NULL) ? *collectStderr : proc.getAllStderr();
	  ba_cat(outdata, stderrdata);
	}
	if (outdata->isEmpty()) {
	  // no data
	  QString stderrstr = (!outputStderr) ? ("\n"+proc.readStderrString()) : QString();
	  klfDbg(progTitle<<" did not provide any data. "<<stderrstr);
	  if (resError != NULL) {
	    resError->status = resErrCodes[KLFFP_NODATA];
	    resError->errorstr = QObject::tr("Program %1 did not provide any output data.", "KLFBackend")
	      .arg(progTitle) + stderrstr;
	  }
	  return false;
	}
	// read standard output to buffer, continue with other possible outputs
	continue;
      }

      if (!QFile::exists(outFileName)) {
	qDebug("%s: File `%s' did not appear after running %s", KLF_FUNC_NAME, qPrintable(outFileName),
	       qPrintable(progTitle));
	if (resError != NULL) {
	  resError->status = resErrCodes[KLFFP_NODATA];
	  resError->errorstr = QObject::tr("Output file didn't appear after having called %1!", "KLFBackend")
	    .arg(progTitle);
	}
	return false;
      }

      // read output file into outdata
      QFile outfile(outFileName);
      r = outfile.open(QIODevice::ReadOnly);
      if ( ! r ) {
	qDebug("%s: File `%s' cannot be read (after running %s)", KLF_FUNC_NAME, qPrintable(outFileName),
	       qPrintable(progTitle));
	if (resError != NULL) {
	  resError->status = resErrCodes[KLFFP_DATAREADFAIL];
	  resError->errorstr = QObject::tr("Can't read file '%1'!\n", "KLFBackend").arg(outFileName);
	}
	return false;
      }
      
      *outdata = outfile.readAll();
      klfDbg("Read file "<<outFileName<<", got data, length="<<outdata->size());
    }

    klfDbg(progTitle<<" was successfully run and output successfully retrieved.") ;

    // all OK
    exitStatus = 0;
    exitCode = 0;
    return true;
  }
};


#define D_RX "([0-9eE.-]+)"

// A Bounding Box
struct klfbbox {
  double x1, x2, y1, y2;
};



static bool calculate_gs_eps_bbox(const QByteArray& epsdata, const QString& epsFile, klfbbox *bbox,
				  KLFBackend::klfOutput * resError, const KLFBackend::klfSettings& settings);
static bool read_eps_bbox(const QByteArray& epsdata, klfbbox *bbox, KLFBackend::klfOutput * resError);
static void correct_eps_bbox(const QByteArray& epsdata, const klfbbox& bbox_corrected, const klfbbox& bbox_orig,
			     QByteArray * epsdatacorrected);

static void replace_svg_width_or_height(QByteArray *svgdata, const char * attr, double val);


static inline bool has_userscript_output(const QSet<QString>& fmts, const QString& format)
{
  return fmts.contains(format);
  //  if (!fmts.contains(format))
  //    return false;
  //  return fn.isEmpty() ? true : QFile::exists(fn);
}


// for user debugging...
KLF_EXPORT QString klfbackend_last_userscript_output;


typedef QSet<QString> KLFStringSet;

KLF_EXPORT KLFStringSet klfbackend_fmts =
  KLFStringSet()
  /* */   << "tex" << "latex" << "dvi" << "eps-raw" << "eps-bbox" << "eps-processed"
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
    s << "tex";
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




KLFBackend::klfOutput KLFBackend::getLatexFormula(const klfInput& in, const klfSettings& usersettings)
{
  // ALLOW ONLY ONE RUNNING getLatexFormula() AT A TIME 
  QMutexLocker mutexlocker(&klf_mutex);

  klfSettings settings;
  settings = usersettings;

  int k;
  bool ok;

  qDebug("%s: %s: KLFBackend::getLatexFormula() called. latex=%s", KLF_FUNC_NAME, KLF_SHORT_TIME,
	 qPrintable(in.latex));

  { // get full, expanded exec environment
    QStringList execenv = klf_cur_environ();
    for (k = 0; k < (int)settings.execenv.size(); ++k) {
      int eqpos = settings.execenv[k].s_indexOf(QChar('='));
      if (eqpos == -1) {
	qWarning("%s: badly formed environment definition in `environ': %s", KLF_FUNC_NAME,
		 qPrintable(settings.execenv[k]));
	continue;
      }
      QString varname = settings.execenv[k].mid(0, eqpos);
      QString newenvdef = klf_expand_env_vars(settings.execenv[k]);
      klf_append_replace_env_var(&execenv, varname, newenvdef);
    }
    settings.execenv = execenv;
  }

  qDebug("%s: %s: execution environment for sub-processes:\n%s", KLF_FUNC_NAME, KLF_SHORT_TIME,
	 qPrintable(settings.execenv.join("\n")));

  
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
  initGsInfo(&settings);
  if (!gsInfo.contains(settings.gsexec)) {
    res.status = KLFERR_NOGSVERSION;
    res.errorstr = QObject::tr("Can't query version of ghostscript located at `%1'.", "KLFBackend")
      .arg(settings.gsexec);
    return res;
  }

  klfDebugf(("%s: queried ghostscript version: %s", KLF_FUNC_NAME, qPrintable(gsInfo[settings.gsexec].version))) ;

  // force some rules on settings

  // calcEpsBoundingBox may not be used if the bg is not white or transparent
  klfDebugf(("%s: settings.calcEpsBoundingBox=%d, in.bg_color=[RGBA %d,%d,%d,%d]", KLF_FUNC_NAME,
	     settings.calcEpsBoundingBox, qRed(in.bg_color), qGreen(in.bg_color), qBlue(in.bg_color),
	     qAlpha(in.bg_color)));
  if (qAlpha(in.bg_color) != 0 && (in.bg_color & qRgb(255,255,255)) != qRgb(255,255,255)) {
    // bg not transparent AND bg not white
    klfDebugf(("%s: forcing calcEpsBoundingBox to FALSE because you have non-transparent/non-white bg",
	       KLF_FUNC_NAME)) ;
    settings.calcEpsBoundingBox = false;
  }


  // PROCEDURE (V3.3)
  //
  // - generate LaTeX file
  //
  // - latex                            --> get DVI file
  //             EXCEPT: if a user wrapper script is given, run that instead.
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
  //  - gs -dNOPAUSE -dSAFER -dSetPageSize -sDEVICE=ps2write -dEPSCrop -sOutputFile=file-corrected.(e)ps
  //       -q -dBATCH  file-bbox.eps    --> generate (E)PS file w/ correct page size
  // OR
  //  - gs -dNOCACHE -dNOPAUSE -dSAFER -sDEVICE=epswrite -dEPSCrop -sOutputFile=file-corrected.eps -q -dBATCH
  //       file-bbox.eps                --> generate post-processed (E)PS file
  //
  // - gs -dNOPAUSE -dSAFER -sDEVICE=pdfwrite -sOutputFile=file.pdf -q -dBATCH file-corrected.eps
  //
  // - if (version(gs) >= 8.64) {
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
  QString tempfname = settings.tempdir + "/klftmp" + ver + "T" + QDateTime::currentDateTime().toString("hhmmss");

  QString fnTex = tempfname + ".tex";
  QString fnDvi = tempfname + ".dvi";
  QString fnRawEps = tempfname + ".eps";
  QString fnBBoxEps = tempfname + "-bbox.eps";
  QString fnProcessedEps = tempfname + "-processed.eps";
  QString fnRawPng = tempfname + "-raw.png";
  QString fnPdf = tempfname + ".pdf";
  QString fnGsSvg = tempfname + "-gs.svg";
  // some user scripts may provide directly .svg (even though the default process chain
  // processes raw svg in memory, not generating final svg file)
  QString fnSvg = tempfname + ".svg";

  // we need non-outlinedfont EPS data anyway.
  QByteArray rawepsdata;
  QByteArray bboxepsdata;
  QByteArray gssvgdata;

  // upon destruction (on stack) of this object, cleanup() will be
  // automatically called as wanted
  cleanup_caller cleanupcallerinstance(tempfname);

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

    KLFUserScriptInfo scriptinfo(in.userScript, &settings);

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
    QString fgcol = QString("rgba(%1,%2,%3,%4)").arg(qRed(in.fg_color))
      .arg(qGreen(in.fg_color)).arg(qBlue(in.fg_color)).arg(qAlpha(in.fg_color));
    QString bgcol = QString("rgba(%1,%2,%3,%4)").arg(qRed(in.bg_color))
      .arg(qGreen(in.bg_color)).arg(qBlue(in.bg_color)).arg(qAlpha(in.bg_color));
    QStringList addenv;
    addenv
      // program executables
      << "KLF_TEMPDIR=" + settings.tempdir
      << "KLF_TEMPFNAME=" + tempfname // the base name for all our temp files
      << "KLF_LATEX=" + settings.latexexec
      << "KLF_DVIPS=" + settings.dvipsexec
      << "KLF_GS=" + settings.gsexec
      << "KLF_GS_VERSION=" + gsInfo.value(settings.gsexec).version
      << "KLF_GS_DEVICES=" + QStringList(gsInfo.value(settings.gsexec).availdevices.toList()).join(",")
      // input
      << "KLF_INPUT_LATEX=" + in.latex
      << "KLF_INPUT_MATHMODE=" + in.mathmode
      << "KLF_INPUT_PREAMBLE=" + in.preamble
      << "KLF_INPUT_FG_COLOR_WEB=" + QColor(in.fg_color).name()
      << "KLF_INPUT_FG_COLOR_RGBA=" + fgcol
      << "KLF_INPUT_BG_COLOR_TRANSPARENT=" + QString::fromLatin1(qAlpha(in.bg_color) > 50 ? "1" : "0")
      << "KLF_INPUT_BG_COLOR_WEB=" + QColor(in.bg_color).name()
      << "KLF_INPUT_BG_COLOR_RGBA=" + bgcol
      << "KLF_INPUT_DPI=" + QString::number(in.dpi)
      << "KLF_INPUT_USERSCRIPT=" + in.userScript
      << "KLF_INPUT_BYPASS_TEMPLATE=" + in.bypassTemplate
      // more advanced settings
      << "KLF_SETTINGS_BORDEROFFSET=" + klfFmt("%.3g,%.3g,%.3g,%.3g pt", settings.tborderoffset,
					       settings.rborderoffset, settings.bborderoffset, settings.lborderoffset)
      << "KLF_SETTINGS_OUTLINEFONTS=" + QString::fromLatin1(settings.outlineFonts ? "1" : "0")
      << "KLF_SETTINGS_CALCEPSBOUNDINGBOX=" + QString::fromLatin1(settings.calcEpsBoundingBox ? "1" : "0")
      << "KLF_SETTINGS_WANT_RAW=" + QString::fromLatin1(settings.wantRaw ? "1" : "0")
      << "KLF_SETTINGS_WANT_PDF=" + QString::fromLatin1(settings.wantPDF ? "1" : "0")
      << "KLF_SETTINGS_WANT_SVG=" + QString::fromLatin1(settings.wantSVG ? "1" : "0")
      // file names (all formed with same basename...) to access by the script
      << "KLF_FN_TEX=" + fnTex
      << "KLF_FN_LATEX=" + fnTex
      << "KLF_FN_DVI=" + fnDvi
      << "KLF_FN_EPS_RAW=" + fnRawEps
      << "KLF_FN_EPS_PROCESSED=" + fnProcessedEps
      << "KLF_FN_PNG=" + fnRawPng
      << "KLF_FN_PDF=" + fnPdf
      << "KLF_FN_SVG_GS=" + fnGsSvg
      << "KLF_FN_SVG=" + fnSvg
      ;

    // and add custom user parameters
    QMap<QString,QString>::const_iterator cit;
    for (cit = in.userScriptParam.constBegin(); cit != in.userScriptParam.constEnd(); ++cit) {
      addenv << "KLF_ARG_"+cit.key() + "=" + cit.value();
    }

    { // now run the script
      KLFFilterProgram p("[user script "+scriptinfo.name()+"]", &settings);
      p.resErrCodes[KLFFP_NOSTART] = KLFERR_USERSCRIPT_NORUN;
      p.resErrCodes[KLFFP_NOEXIT] = KLFERR_USERSCRIPT_NONORMALEXIT;
      p.resErrCodes[KLFFP_NOSUCCESSEXIT] = KLFERR_PROGERR_USERSCRIPT;
      p.resErrCodes[KLFFP_NODATA] = KLFERR_USERSCRIPT_NOOUTPUT;
      p.resErrCodes[KLFFP_DATAREADFAIL] = KLFERR_USERSCRIPT_OUTPUTREADFAIL;
      p.execEnviron << addenv;

      QByteArray stderrdata;
      QByteArray stdoutdata;
      p.collectStderr = &stderrdata;
      p.collectStdout = &stdoutdata;

      p.argv << in.userScript << QDir::toNativeSeparators(fnTex);

      QMap<QString,QByteArray*> outdata;
      QStringList outfmts = scriptinfo.spitsOut();
      foreach (QString fmt, outfmts) {
	us_outputs << fmt;
	if (fmt == QLatin1String("tex") || fmt == QLatin1String("latex")) {
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
	  if (settings.wantRaw)
	    outdata[fnRawPng] = &res.pngdata_raw;
	} else if (fmt == QLatin1String("pdf")) {
	  if (settings.wantPDF)
	    outdata[fnPdf] = &res.pdfdata;
	} else if (fmt == QLatin1String("svg-gs")) {
	  // ignore this data, not returned in klfOutput but the created file is used to generate
	  // the processed SVG
	  outdata[fnGsSvg] = & gssvgdata;
	} else if (fmt == QLatin1String("svg")) {
	  if (settings.wantSVG)
	    outdata[fnSvg] = &res.svgdata;
	} else {
	  klfWarning("Can't handle output format from user script: "<<fmt) ;
	}
      }
      if (us_outputs.isEmpty())
	us_outputs << "dvi"; // by default, the script is assumed to provide DVI.
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
      if ((tempi = skipfmts.indexOf("ALLEXCEPT")) >= 0) {
	invert = true;
	skipfmts.removeAt(tempi);
	foreach (QString f, klfbackend_fmts) { us_skipfmts << f; }
      }
      foreach (QString fmt, skipfmts) {
	if (!klfbackend_fmts.contains(fmt)) {
	  klfWarning("User Script Info: Unknown format to skip: "<<fmt) ;
	}
	if (!invert) {
	  if (outdata.contains(fmt)) {
	    klfWarning("User Script Info: If a format is provided by the script, don't mark it as to skip. fmt="<<fmt) ;
	    continue;
	  }
	  us_skipfmts << fmt;
	} else {
	  us_skipfmts.remove(fmt);
	}
      }
      our_skipfmts = us_skipfmts;

      if ((us_outputs.contains("eps-processed") || our_skipfmts.contains("eps-processed")) && !settings.wantRaw) {
	our_skipfmts << "eps-bbox";
      }
      if ((us_outputs.contains("eps-bbox") || our_skipfmts.contains("eps-bbox")) && !settings.wantRaw) {
	our_skipfmts << "eps-raw";
      }
      if (us_outputs.contains("svg") || our_skipfmts.contains("svg")) {
	our_skipfmts << "svg-gs"; // don't need svg-gs if we're not generating svg
      }

      ok = p.run(outdata, &res);

      // for user script debugging
      klfbackend_last_userscript_output
        = "<b>STDOUT</b>\n<pre>" + Qt::escape(stdoutdata) + "</pre>\n<br/><b>STDERR</b>\n<pre>"
        + Qt::escape(stderrdata) + "</pre>";

      if (!ok) {
	return res;
      }
    }
  }


  if (!has_userscript_output(us_outputs, "dvi") && !our_skipfmts.contains("dvi")) {
    // execute latex
    KLFFilterProgram p(QLatin1String("LaTeX"), &settings);
    p.resErrCodes[KLFFP_NOSTART] = KLFERR_LATEX_NORUN;
    p.resErrCodes[KLFFP_NOEXIT] = KLFERR_LATEX_NONORMALEXIT;
    p.resErrCodes[KLFFP_NOSUCCESSEXIT] = KLFERR_PROGERR_LATEX;
    p.resErrCodes[KLFFP_NODATA] = KLFERR_LATEX_NOOUTPUT;
    p.resErrCodes[KLFFP_DATAREADFAIL] = KLFERR_LATEX_OUTPUTREADFAIL;

    p.argv << settings.latexexec << QDir::toNativeSeparators(fnTex);
    
    ok = p.run(fnDvi, &res.dvidata, &res);
    if (!ok) {
      return res;
    }
  }

  if (!has_userscript_output(us_outputs, "eps-raw") && !our_skipfmts.contains("eps-raw")) {

    ASSERT_HAVE_FORMATS_FOR("eps-raw") ;

    // execute dvips -E
    KLFFilterProgram p(QLatin1String("dvips"), &settings);
    p.resErrCodes[KLFFP_NOSTART] = KLFERR_DVIPS_NORUN;
    p.resErrCodes[KLFFP_NOEXIT] = KLFERR_DVIPS_NONORMALEXIT;
    p.resErrCodes[KLFFP_NOSUCCESSEXIT] = KLFERR_PROGERR_DVIPS;
    p.resErrCodes[KLFFP_NODATA] = KLFERR_DVIPS_NOOUTPUT;
    p.resErrCodes[KLFFP_DATAREADFAIL] = KLFERR_DVIPS_OUTPUTREADFAIL;

    p.argv << settings.dvipsexec << "-E" << QDir::toNativeSeparators(fnDvi)
	   << "-o" << QDir::toNativeSeparators(fnRawEps);

    ok = p.run(fnRawEps, &rawepsdata, &res);

    if (!ok) {
      return res;
    }
    qDebug("%s: read raw EPS; rawepsdata/length=%d", KLF_FUNC_NAME, rawepsdata.size());
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
      bool ok = calculate_gs_eps_bbox(QByteArray(), fnRawEps, &bbox, &res, settings);
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

    correct_eps_bbox(rawepsdata, bbox_corrected, bbox, &bboxepsdata);
  } else if (!our_skipfmts.contains("eps-bbox")) {
    // userscript generated bbox-corrected EPS for us, but we still
    // need to set width_pt and height_pt appropriately.

    klfbbox bb;

    // read from fnRawEps, fnBBoxEps or fnProcessedEps ?
    QString fn;
    QByteArray *dat;
    if (us_outputs.contains("eps-processed")) {
      fn = fnProcessedEps;
      dat = & res.epsdata;
    } else {
      fn = fnBBoxEps;
      dat = & bboxepsdata;
    }

    if (settings.calcEpsBoundingBox) {
      bool ok = calculate_gs_eps_bbox(QByteArray(), fn, &bb, &res, settings);
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

      KLFFilterProgram p(QLatin1String("gs (EPS Post-Processing Outline Fonts)"), &settings);
      p.resErrCodes[KLFFP_NOSTART] = KLFERR_GSPOSTPROC_NORUN;
      p.resErrCodes[KLFFP_NOEXIT] = KLFERR_GSPOSTPROC_NONORMALEXIT;
      p.resErrCodes[KLFFP_NOSUCCESSEXIT] = KLFERR_PROGERR_GSPOSTPROC;
      p.resErrCodes[KLFFP_NODATA] = KLFERR_GSPOSTPROC_NOOUTPUT;
      p.resErrCodes[KLFFP_DATAREADFAIL] = KLFERR_GSPOSTPROC_OUTPUTREADFAIL;

      p.argv << settings.gsexec;
      // The bad joke is that in gs' manpage '-dNOCACHE' is described as a debugging option.
      // It is NOT. It outlines the fonts to paths. It cost me a few hours trying to understand
      // what's going on ... :(
      p.argv << "-dNOCACHE";
      p.argv << "-dNOPAUSE" << "-dSAFER" << "-dEPSCrop" << "-sDEVICE=pswrite";
      p.argv << "-sOutputFile="+QDir::toNativeSeparators(fnProcessedEps)
	     << "-q" << "-dBATCH" << "-";
      
      ok = p.run(bboxepsdata, fnProcessedEps, &res.epsdata, &res);
      if (!ok) {
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

    // run 'gs' to get PNG data
    KLFFilterProgram p(QLatin1String("gs (PNG)"), &settings);
    p.resErrCodes[KLFFP_NOSTART] = KLFERR_GSPNG_NORUN;
    p.resErrCodes[KLFFP_NOEXIT] = KLFERR_GSPNG_NONORMALEXIT;
    p.resErrCodes[KLFFP_NOSUCCESSEXIT] = KLFERR_PROGERR_GSPNG;
    p.resErrCodes[KLFFP_NODATA] = KLFERR_GSPNG_NOOUTPUT;
    p.resErrCodes[KLFFP_DATAREADFAIL] = KLFERR_GSPNG_OUTPUTREADFAIL;

    p.argv << settings.gsexec
	   << "-dNOPAUSE" << "-dSAFER" << "-dTextAlphaBits=4" << "-dGraphicsAlphaBits=4"
	   << "-r"+QString::number(in.dpi) << "-dEPSCrop";
    if (qAlpha(in.bg_color) > 0) { // we're forcing a background color
      p.argv << "-sDEVICE=png16m";
    } else {
      p.argv << "-sDEVICE=pngalpha";
    }
    p.argv << "-sOutputFile="+QDir::toNativeSeparators(fnRawPng) << "-q" << "-dBATCH" << "-";

    ok = p.run(bboxepsdata, fnRawPng, &res.pngdata_raw, &res);
    if (!ok) {
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
    QString boolstr[2] = { QLatin1String("true"), QLatin1String("false") } ;

    // store some meta-information into result
    res.result.setText("AppVersion", QString::fromLatin1("KLatexFormula " KLF_VERSION_STRING));
    res.result.setText("Application",
			   QObject::tr("Created with KLatexFormula version %1", "KLFBackend::saveOutputToFile"));
    res.result.setText("Software", QString::fromLatin1("KLatexFormula " KLF_VERSION_STRING));
    res.result.setText("InputLatex", in.latex);
    res.result.setText("InputMathMode", in.mathmode);
    res.result.setText("InputPreamble", in.preamble);
    res.result.setText("InputFgColor", QString("rgb(%1, %2, %3)").arg(qRed(in.fg_color))
			   .arg(qGreen(in.fg_color)).arg(qBlue(in.fg_color)));
    res.result.setText("InputBgColor", QString("rgba(%1, %2, %3, %4)").arg(qRed(in.bg_color))
			   .arg(qGreen(in.bg_color)).arg(qBlue(in.bg_color))
			   .arg(qAlpha(in.bg_color)));
    res.result.setText("InputDPI", QString::number(in.dpi));
    res.result.setText("SettingsTBorderOffset", QString::number(settings.tborderoffset));
    res.result.setText("SettingsRBorderOffset", QString::number(settings.rborderoffset));
    res.result.setText("SettingsBBorderOffset", QString::number(settings.bborderoffset));
    res.result.setText("SettingsLBorderOffset", QString::number(settings.lborderoffset));
    res.result.setText("SettingsOutlineFonts", boolstr[(int)settings.outlineFonts]);
    res.result.setText("SettingsCalcEpsBoundingBox", boolstr[(int)settings.calcEpsBoundingBox]);
    res.result.setText("SettingsWantPDF", boolstr[(int)settings.wantPDF]);
    res.result.setText("SettingsWantSVG", boolstr[(int)settings.wantSVG]);
    
    klfDbg("prepared QImage.") ;
    
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

  if (!has_userscript_output(us_outputs, "pdf") && !our_skipfmts.contains("pdf")) {

    ASSERT_HAVE_FORMATS_FOR("pdf") ;

    // run 'gs' to get PDF data
    KLFFilterProgram p(QLatin1String("gs (PDF)"), &settings);
    p.resErrCodes[KLFFP_NOSTART] = KLFERR_GSPDF_NORUN;
    p.resErrCodes[KLFFP_NOEXIT] = KLFERR_GSPDF_NONORMALEXIT;
    p.resErrCodes[KLFFP_NOSUCCESSEXIT] = KLFERR_PROGERR_GSPDF;
    p.resErrCodes[KLFFP_NODATA] = KLFERR_GSPDF_NOOUTPUT;
    p.resErrCodes[KLFFP_DATAREADFAIL] = KLFERR_GSPDF_OUTPUTREADFAIL;

    p.argv << settings.gsexec
	   << "-dNOPAUSE" << "-dSAFER" << "-sDEVICE=pdfwrite"
	   << "-sOutputFile="+QDir::toNativeSeparators(fnPdf)
	   << "-q" << "-dBATCH" << "-";

    // input: res.epsdata is the processed EPS file, or the raw EPS + bbox/page correction if no post-processing
    ok = p.run(res.epsdata, fnPdf, &res.pdfdata, &res);
    if (!ok) {
      return res;
    }
  }

  if (settings.wantSVG) {

    if (!has_userscript_output(us_outputs, "svg-gs") &&
	!our_skipfmts.contains("svg-gs")) {

      ASSERT_HAVE_FORMATS_FOR("svg-gs") ;

      // run 'gs' to get SVG (raw from gs)
      if (!gsInfo[settings.gsexec].availdevices.contains("svg")) {
	// not OK to get SVG...
	klfWarning("ghostscript cannot create SVG");
	res.status = KLFERR_GSSVG_NOSVG;
	res.errorstr = QObject::tr("This ghostscript (%1) cannot generate SVG.", "KLFBackend").arg(settings.gsexec);
	return res;
      }

      KLFFilterProgram p(QLatin1String("gs (SVG)"), &settings);
      p.resErrCodes[KLFFP_NOSTART] = KLFERR_GSSVG_NORUN;
      p.resErrCodes[KLFFP_NOEXIT] = KLFERR_GSSVG_NONORMALEXIT;
      p.resErrCodes[KLFFP_NOSUCCESSEXIT] = KLFERR_PROGERR_GSSVG;
      p.resErrCodes[KLFFP_NODATA] = KLFERR_GSSVG_NOOUTPUT;
      p.resErrCodes[KLFFP_DATAREADFAIL] = KLFERR_GSSVG_OUTPUTREADFAIL;
      
      p.argv << settings.gsexec;
      // unconditionally outline fonts, otherwise output is horrible
      p.argv << "-dNOCACHE" << "-dNOPAUSE" << "-dSAFER" << "-dEPSCrop" << "-sDEVICE=svg"
	     << "-sOutputFile="+QDir::toNativeSeparators(fnGsSvg)
	     << "-q" << "-dBATCH" << "-";

      // input: res.epsdata is the processed EPS file, or the raw EPS file if no post-processing
      ok = p.run(bboxepsdata, fnGsSvg, &gssvgdata, &res);
      if (!ok) {
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
				  KLFBackend::klfOutput * resError, const KLFBackend::klfSettings& settings)
{
  KLF_DEBUG_BLOCK(KLF_FUNC_NAME) ;
  // find correct bounding box of EPS file, using ghostscript

  int i;

  KLFFilterProgram p(QLatin1String("GhostScript (bbox)"), &settings);
  p.resErrCodes[KLFFP_NOSTART] = KLFERR_GSBBOX_NORUN;
  p.resErrCodes[KLFFP_NOEXIT] = KLFERR_GSBBOX_NONORMALEXIT;
  p.resErrCodes[KLFFP_NOSUCCESSEXIT] = KLFERR_PROGERR_GSBBOX;
  p.resErrCodes[KLFFP_NODATA] = KLFERR_GSBBOX_NOOUTPUT;
  // p.resErrCodes[KLFFP_DATAREADFAIL]  unused (used only for reading output files)

  p.outputStdout = true;
  p.outputStderr = true;
  
  QByteArray bboxdata;

  p.argv << settings.gsexec << "-dNOPAUSE" << "-dSAFER" << "-sDEVICE=bbox" << "-q" << "-dBATCH"
	 << (epsFile.isEmpty() ? QString::fromLatin1("-") : epsFile);

  bool ok = p.run(epsData /*stdin*/, QString() /*no output file*/, &bboxdata/*collect stdout*/, resError);
  if (!ok) {
    // resError is already set
    return false;
  }
  
  klfDbg("gs provided output:\n"<<bboxdata<<"\n");

  // parse gs' bbox data
  QRegExp rx_gsbbox("%%HiResBoundingBox\\s*:\\s+" D_RX "\\s+" D_RX "\\s+" D_RX "\\s+" D_RX "");
  i = rx_gsbbox.rx_indexin(QString::fromLatin1(bboxdata));
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

static bool read_eps_bbox(const QByteArray& epsdata, klfbbox *bbox, KLFBackend::klfOutput * resError)
{
  KLF_DEBUG_BLOCK(KLF_FUNC_NAME) ;

  static const char * bboxtag = "%%BoundingBox:";
  static const int bboxtaglen = strlen(bboxtag);

  // Read dvips' bounding box.
  QBuffer buf(const_cast<QByteArray*>(&epsdata));
  bool r = buf.open(QIODevice::ReadOnly | QIODevice::Text);
  if (!r) {
    klfWarning("What's going on!!?! can't open buffer for reading? Will Fail!!!") ;
  }

  QString nobboxerrstr =
    QObject::tr("DVIPS did not provide parsable %%BoundingBox: in its output!", "KLFBackend");

  char linebuffer[512];
  int n, i;
  while ((n = buf.readLine(linebuffer, 512)) > 0) {
    if (!strncmp(linebuffer, bboxtag, bboxtaglen)) {
      // got bounding-box.
      // parse bbox values
      QRegExp rx_bbvalues("" D_RX "\\s+" D_RX "\\s+" D_RX "\\s+" D_RX "");
      i = rx_bbvalues.rx_indexin(QString::fromLatin1(linebuffer + bboxtaglen));
      if (i < 0) {
	resError->status = KLFERR_DVIPS_OUTPUTNOBBOX;
	resError->errorstr = nobboxerrstr;
	return false;
      }
      bbox->x1 = rx_bbvalues.cap(1).toDouble();
      bbox->y1 = rx_bbvalues.cap(2).toDouble();
      bbox->x2 = rx_bbvalues.cap(3).toDouble();
      bbox->y2 = rx_bbvalues.cap(4).toDouble();
      return true;
    }
  }

  resError->status = KLFERR_DVIPS_OUTPUTNOBBOX;
  resError->errorstr = nobboxerrstr;
  return false;
}
static void correct_eps_bbox(const QByteArray& rawepsdata, const klfbbox& bbox_corrected,
			     const klfbbox& bbox_orig, QByteArray * epsdatacorrected)
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

  int wi = (int)(bbox_corrected.x2 + 0.99999) ;
  int hi = (int)(bbox_corrected.y2 + 0.99999) ;
  char buffer[1024];
  int buffer_len;
  // recall that '%%' in printf is replaced by a single '%'...
  snprintf(buffer, sizeof(buffer)-1,
	   "%%%%BoundingBox: 0 0 %d %d%s"
	   "%%%%HiResBoundingBox: 0 0 %s %s%s",
	   wi, hi, nl,
	   klfFmtDoubleCC(bbox_corrected.x2, 'g', 6), klfFmtDoubleCC(bbox_corrected.y2, 'g', 6), nl);
  buffer_len = strlen(buffer);

  char buffer2[1024];
  int buffer2_len;
  snprintf(buffer2, sizeof(buffer2),
	   "%s"
	   "%%%%Page 1 1%s"
	   "%%%%PageBoundingBox 0 0 %d %d%s"
	   "<< /PageSize [%d %d] >> setpagedevice%s"
	   "%s %s translate%s",
	   nl,
	   nl,
	   wi, hi, nl,
	   wi, hi, nl,
	   klfFmtDoubleCC(offx, 'f'), klfFmtDoubleCC(offy, 'f'), nl);
  buffer2_len = strlen(buffer2);

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
  klfDebugf(("New eps bbox is [0 0 %.6g %.6g] with translate [%.6g %.6g].",
	     bbox_corrected.x2, bbox_corrected.y2, offx, offy));

  *epsdatacorrected = neweps;
}


static void replace_svg_width_or_height(QByteArray *svgdata, const char * attreq, double val)
{
  QByteArray & svgdataref = * svgdata;

  int i = ba_indexOf(svgdataref, attreq);
  int j = i;
  while (j < (int)svgdataref.size() && (!isspace(svgdataref[j]) && svgdataref[j] != '>'))
    ++j;

  char buffer[1024];
  snprintf(buffer, sizeof(buffer)-1, "%s'%s'", attreq, klfFmtDoubleCC(val, 'f', 3));

  ba_replace(svgdata, i, j-i, buffer);
}
   



static void cleanup(QString tempfname)
{
  // remove any file that has this basename...
  QFileInfo fi(tempfname);
  QDir dir = fi.dir();
  QString pattern = fi.baseName() + "*";

  QStringList l = dir.dir_entry_list(pattern);

  int k;
  for (k = 0; k < (int)l.size(); ++k) {
    QString f = dir.filePath(l[k]);
    QFile::remove(f);
  }

}


KLF_EXPORT bool operator==(const KLFBackend::klfInput& a, const KLFBackend::klfInput& b)
{
  return a.latex == b.latex &&
    a.mathmode == b.mathmode &&
    a.preamble == b.preamble &&
    a.fg_color == b.fg_color &&
    a.bg_color == b.bg_color &&
    a.dpi == b.dpi &&
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



QStringList KLFBackend::availableSaveFormats(const klfOutput& klfoutput)
{
  QStringList formats;
  if (!klfoutput.pngdata.isEmpty())
    formats << "PNG";
  if (!klfoutput.epsdata.isEmpty())
    formats << "PS" << "EPS";
  if (!klfoutput.dvidata.isEmpty())
    formats << "DVI";
  if (!klfoutput.pdfdata.isEmpty())
    formats << "PDF";
  if (!klfoutput.svgdata.isEmpty())
    formats << "SVG";
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
  QString format = fmt.s_trimmed().s_toUpper();

  // now choose correct data source and write to fout
  if (format == "PNG") {
    device->dev_write(klfoutput.pngdata);
  } else if (format == "EPS" || format == "PS") {
    device->dev_write(klfoutput.epsdata);
  } else if (format == "DVI") {
    device->dev_write(klfoutput.dvidata);
  } else if (format == "PDF") {
    if (klfoutput.pdfdata.isEmpty()) {
      QString error = QObject::tr("PDF format is not available!\n",
				  "KLFBackend::saveOutputToFile");
      qWarning("%s", qPrintable(error));
      if (errorStringPtr != NULL)
	errorStringPtr->operator=(error);
      return false;
    }
    device->dev_write(klfoutput.pdfdata);
  } else if (format == "SVG") {
    if (klfoutput.svgdata.isEmpty()) {
      QString error = QObject::tr("SVG format is not available!\n",
				  "KLFBackend::saveOutputToFile");
      qWarning("%s", qPrintable(error));
      if (errorStringPtr != NULL)
	errorStringPtr->operator=(error);
      return false;
    }
    device->dev_write(klfoutput.svgdata);
 } else {
    bool res = klfoutput.result.save(device, format.s_toLatin1());
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
    if ( ! fi.fi_suffix().isEmpty() )
      format = fi.fi_suffix();
  }
  if (format.isEmpty())
    format = QLatin1String("PNG");
  format = format.s_trimmed().s_toUpper();
  // got format. choose output now and prepare write
  QFile fout;
  if (fileName.isEmpty() || fileName == "-") {
    if ( ! fout.f_open_fp(stdout) ) {
      QString error = QObject::tr("Unable to open stderr for write! Error: %1\n",
				  "KLFBackend::saveOutputToFile").arg(fout.f_error());
      qWarning("%s", qPrintable(error));
      if (errorStringPtr != NULL)
	*errorStringPtr = error;
      return false;
    }
  } else {
    fout.f_setFileName(fileName);
    if ( ! fout.open(dev_WRITEONLY) ) {
      QString error = QObject::tr("Unable to write to file `%1'! Error: %2\n",
				  "KLFBackend::saveOutputToFile")
	.arg(fileName).arg(fout.f_error());
      qWarning("%s", qPrintable(error));
      if (errorStringPtr != NULL)
	*errorStringPtr = error;
      return false;
    }
  }

  return saveOutputToDevice(klfoutput, &fout, format, errorStringPtr);
}


bool KLFBackend::detectSettings(klfSettings *settings, const QString& extraPath)
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
#ifdef KLFBACKEND_QT4
  settings->tempdir = QDir::fromNativeSeparators(QDir::tempPath());
#else
# if defined(Q_OS_UNIX) || defined(Q_OS_LINUX) || defined(Q_OS_DARWIN) || defined(Q_OS_MACX)
  settings->tempdir = "/tmp";
# elif defined(Q_OS_WIN32)
  settings->tempdir = getenv("TEMP");
# else
  settings->tempdir = QString();
# endif
#endif

  // sensible defaults
  settings->lborderoffset = 1;
  settings->tborderoffset = 1;
  settings->rborderoffset = 1;
  settings->bborderoffset = 1;
  
  settings->epstopdfexec = QString(); // obsolete, no longer used

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

  klf_detect_execenv(settings);

  if (settings->gsexec.length()) {
    initGsInfo(settings);
    if (!gsInfo.contains(settings->gsexec)) {
      klfWarning("Cannot get 'gs' devices information with "<<(settings->gsexec+" --version/--help"));
    } else if (gsInfo[settings->gsexec].availdevices.contains("svg")) {
      settings->wantSVG = true;
    }
  }

  bool result_failure =
    settings->tempdir.isEmpty() || settings->latexexec.isEmpty() || settings->dvipsexec.isEmpty() ||
    settings->gsexec.isEmpty();

  return !result_failure;
}


/** \brief detects any additional settings to environment variables
 *
 * Detects whether the given values of latex, dvips, gs and epstopdf in the
 * given (initialized) settings \c settings need extra environment set,
 * and sets the \c execenv member of \c settings accordingly.
 *
 * Note that the environment settings already existing in \c settings->execenv are
 * kept; only those variables for which new values are detected are updated, or if
 * new declarations are needed they are appended.
 *
 * \note KLFBackend::detectSettings() already calls this function, you don't
 *   have to call this function manually in that case.
 *
 * \return TRUE (success) or FALSE (failure). Currently there is no reason
 * for failure, and returns always TRUE (as of 3.2.1).
 */
KLF_EXPORT bool klf_detect_execenv(KLFBackend::klfSettings *settings)
{
  // detect mgs.exe as ghostscript and setup its environment properly
  QFileInfo gsfi(settings->gsexec);
  if (gsfi.fileName() == "mgs.exe") {
    QString mgsenv = QString("MIKTEX_GS_LIB=")
      + QDir::toNativeSeparators(gsfi.fi_absolutePath()+"/../../ghostscript/base")
      + ";"
      + QDir::toNativeSeparators(gsfi.fi_absolutePath()+"/../../fonts");
    klf_append_replace_env_var(& settings->execenv, "MIKTEX_GS_LIB", mgsenv);
    klfDbg("Adjusting environment for mgs.exe: `"+mgsenv+"'") ;
  }

#ifdef Q_WS_MAC
  // make sure that epstopdf's path is in PATH because it wants to all gs
  // (eg fink distributions)
  if (!settings->epstopdfexec.isEmpty()) {
    QFileInfo epstopdf_fi(settings->epstopdfexec);
    QString execenvpath = QString("PATH=%1:$PATH").arg(epstopdf_fi.fi_absolutePath());
    klf_append_replace_env_var(& settings->execenv, "PATH", execenvpath);
  }
#endif

  return true;
}




// static 
void initGsInfo(const KLFBackend::klfSettings *settings)
{
  if (gsInfo.contains(settings->gsexec)) // info already cached
    return;

  QString gsver;
  { // test 'gs' version, to see if we can provide SVG data
    KLFFilterProgram p(QLatin1String("gs (test version)"), settings);
    //    p.resErrCodes[KLFFP_NOSTART] = ;
    //     p.resErrCodes[KLFFP_NOEXIT] = ;
    //     p.resErrCodes[KLFFP_NOSUCCESSEXIT] = ;
    //     p.resErrCodes[KLFFP_NODATA] = ;
    //     p.resErrCodes[KLFFP_DATAREADFAIL] = ;
    
    p.execEnviron = settings->execenv;
    
    p.argv << settings->gsexec << "--version";
    
    QByteArray ba_gsver;
    bool ok = p.run(QString(), &ba_gsver, NULL);
    if (ok) {
      gsver = QString::fromLatin1(ba_gsver);
      klfDbg("gs version text (untrimmed): "<<gsver) ;

      gsver = gsver.s_trimmed();
    }
  }

  QString gshelp;
  KLFStringSet availdevices;
  { // test 'gs' version, to see if we can provide SVG data
    KLFFilterProgram p(QLatin1String("gs (query help)"), settings);
    //    p.resErrCodes[KLFFP_NOSTART] = ;
    //     p.resErrCodes[KLFFP_NOEXIT] = ;
    //     p.resErrCodes[KLFFP_NOSUCCESSEXIT] = ;
    //     p.resErrCodes[KLFFP_NODATA] = ;
    //     p.resErrCodes[KLFFP_DATAREADFAIL] = ;
    
    p.execEnviron = settings->execenv;
    klf_append_replace_env_var(&p.execEnviron, QLatin1String("LANG"), QLatin1String("LANG=en_US.UTF-8"));
    
    p.argv << settings->gsexec << "--help";
    
    QByteArray ba_gshelp;
    bool ok = p.run(QString(), &ba_gshelp, NULL);
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

  GsInfo i;
  i.version = gsver;
  i.help = gshelp;
  i.availdevices = availdevices;

  gsInfo[settings->gsexec] = i;
}


static int read_spec_section(const QString& str, int fromindex, const QRegExp& seprx, QString * extractedPart)
{
  int i = fromindex;
  bool in_quote = false;

  QString s;

  while (i < str.length() && (in_quote || seprx.indexIn(str, i) != i)) {
    if (str[i] == '\\') {
      s.append(str[i]);
      if (i+1 < str.length())
	s.append(str[i+1]);
      i += 2; // skip next char, too. The actual escaping will be done with klfEscapedToData()
      continue;
    }
    if (str[i] == '"') {
      in_quote = !in_quote;
      ++i;
      continue;
    }
    s.append(str[i]);
    ++i;
  }

  *extractedPart = QString::fromLocal8Bit(klfEscapedToData(s.toLocal8Bit()));

  return i; // the position of the last char separator
}

struct KLFUserScriptInfo::Private
{
  Private()
  {
    refcount = 0;
    settings = NULL;
    scriptInfoError = KLFERR_NOERROR;
  }

  int refcount;
  inline int ref() { return ++refcount; }
  inline int deref() { return --refcount; }

  KLFBackend::klfSettings *settings;

  QString fname;
  int scriptInfoError;
  QString scriptInfoErrorString;

  QString category;
  QString name;
  QString author;
  QString version;
  QString license;
  QString klfMinVersion;
  QString klfMaxVersion;

  QStringList spitsOut;
  QStringList skipFormats;

  QList<KLFUserScriptInfo::Param> paramList;

  void query_script_info()
  {
    scriptInfoError = 0;
    scriptInfoErrorString = QString();

    QByteArray scriptinfo;
    //    bool want_full_template = true;
    { // Query Script Info phase
      KLFFilterProgram p(QObject::tr("User Script (ScriptInfo)"), settings);
      p.resErrCodes[KLFFP_NOSTART] = KLFERR_USERSCRIPT_NORUN;
      p.resErrCodes[KLFFP_NOEXIT] = KLFERR_USERSCRIPT_NONORMALEXIT;
      p.resErrCodes[KLFFP_NOSUCCESSEXIT] = KLFERR_PROGERR_USERSCRIPT;
      p.resErrCodes[KLFFP_NODATA] = KLFERR_USERSCRIPT_NOSCRIPTINFO;
      p.resErrCodes[KLFFP_DATAREADFAIL] = KLFERR_USERSCRIPT_NOSCRIPTINFO;
      
      p.argv << fname << "--scriptinfo";
      
      KLFBackend::klfOutput res; // for error messages

      bool ok = p.run(QString(), &scriptinfo, &res);
      if (!ok) {
	scriptInfoError = res.status;
	scriptInfoErrorString = res.errorstr;
	return;
      }
    }
    scriptinfo.replace("\r\n", "\n");
    if (scriptinfo.isEmpty()) {
      scriptinfo = "ScriptInfo\n";
    }
    klfDbg("got scriptinfo="<<scriptinfo) ;
    // parse scriptinfo
    if (!scriptinfo.startsWith("ScriptInfo\n")) {
      scriptInfoError = KLFERR_USERSCRIPT_INVALIDSCRIPTINFO;
      scriptInfoErrorString = QObject::tr("User script did not provide valid --scriptinfo output.");
      qWarning()<<KLF_FUNC_NAME<<": User script did not provide valid --scriptinfo (missing header line).";
      return;
    }
    QList<QByteArray> lines = scriptinfo.split('\n');
    lines.removeAt(0); // skip of course the 'ScriptInfo\n' line !
    int k;
    for (k = 0; k < lines.size(); ++k) {
      QString line = QString::fromLocal8Bit(lines[k]);
      if (line.trimmed().isEmpty())
	continue;
      //  Key: Value   or
      //  Key[specifiers]: Value
      //    Key is [-A-Za-z0-9_.]+
      //    specifiers (optional) is [-A-Za-z0-9_.,]*, intendend to be a list of comma-separated options
      //    value is anything until end of line. Use 'base64' specifier if you need to have newlines in the value
      //        and encode the value as base64.
      QRegExp rx("([-A-Za-z0-9_.]+)(?:\\[([-A-Za-z0-9_.,]*)\\])?:\\s*(.*)");
      QRegExp boolrxtrue("(t(rue)?|y(es)?|on|1)");
      if (!rx.exactMatch(line)) {
	qWarning()<<KLF_FUNC_NAME<<": User script did not provide valid --scriptinfo.\nCannot parse line: "<<line;
	scriptInfoError = KLFERR_USERSCRIPT_INVALIDSCRIPTINFO;
	scriptInfoErrorString = QObject::tr("User script provided invalid --scriptinfo output.", "KLFBackend");
	return;
      }
      QString key = rx.cap(1);
      QString specstr = rx.cap(2);
      QStringList specs = specstr.split(',');
      QString val = rx.cap(3).trimmed();
      if (val.isEmpty())
	val = QString(); // empty value is null string
      if (specs.contains(QLatin1String("base64"))) {
	val = QString::fromLocal8Bit(QByteArray::fromBase64(val.toLatin1()));
      }
      if (key == QLatin1String("Category")) {
	category = val;
	klfDbg("Read category: "<<category) ;
      } else if (key == QLatin1String("Name")) {
	name = val;
	klfDbg("Read name: "<<name) ;
      } else if (key == QLatin1String("Author") || key == QLatin1String("Authors")) {
	if (!author.isEmpty())
	  author += ", " + val;
	else
	  author = val;
	klfDbg("Read (cumulated) author: "<<author) ;
      } else if (key == QLatin1String("Version")) {
	version = val;
	klfDbg("Read version: "<<version) ;
      } else if (key == QLatin1String("License")) {
	license = val;
      } else if (key == QLatin1String("KLFMinVersion")) {
	klfMinVersion = val;
	klfDbg("Read klfMinVersion: "<<klfMinVersion) ;
      } else if (key == QLatin1String("KLFMaxVersion")) {
	klfMaxVersion = val;
	klfDbg("Read klfMaxVersion: "<<klfMaxVersion) ;
      } else if (key == QLatin1String("SpitsOut")) {
	spitsOut = val.split(QRegExp("(,|\\s+)"));
      } else if (key == QLatin1String("SkipFormats")) {
	skipFormats = val.split(QRegExp("(,|\\s+)"));
      } else if (key == QLatin1String("Param")) {
	// parse a paramter request specification, eg.
	// Param: USE_PDF;bool;"Use PDF?";"Generate PDF; or rather prefer old-fashioned PS?"
	int k;
	KLFUserScriptInfo::Param param;
	QRegExp seprx(";");
	// read name
	k = read_spec_section(val, 0, seprx, &param.name);
	// now read type
	QString typstring;
	k = read_spec_section(val, k+1, seprx, &typstring);
	// read title
	k = read_spec_section(val, k+1, seprx, &param.title);
	// and description
	k = read_spec_section(val, k+1, seprx, &param.description);

	// decode type
	QString typname;
	int j;
	QRegExp enumsep(":");
	j = read_spec_section(typstring, 0, enumsep, &typname);
	typname = typname.trimmed().toLower();
	if (typname == "enum") {
	  param.type = KLFUserScriptInfo::Param::Enum;
	  while (j < typstring.length()) {
	    QString enumstr;
	    j = read_spec_section(typstring, j+1, enumsep, &enumstr);
	    if (!enumstr.isEmpty())
	      param.type_enums << enumstr;
	  }
	} else if (typname == "bool") {
	  param.type = KLFUserScriptInfo::Param::Bool;
	} else if (typname == "int") {
	  param.type = KLFUserScriptInfo::Param::Int;
	} else {
	  if (typname != "string") {
	    klfWarning("interpreting unknown type name "<<typname<<" as 'string' type") ;
	  }
	  param.type = KLFUserScriptInfo::Param::String;
	}

	// and add this to our param list
	paramList << param;
      } else {
	klfDbg("Unknown userscript info key: "<<key<<", in line:\n"<<line);
	qWarning()<<KLF_FUNC_NAME<<": Ignoring unknown user script info key "<<key<<".";
      }
    }
  }


  static QMap<QString,KLFRefPtr<Private> > userScriptInfoCache;
  
private:
  /* no copy constructor */
  Private(const Private& other) { }
};


// static
QMap<QString,KLFRefPtr<KLFUserScriptInfo::Private> > KLFUserScriptInfo::Private::userScriptInfoCache;

// static
void KLFUserScriptInfo::clearCacheAll()
{
  // will decrease the refcounts if needed automatically (KLFRefPtr)
  Private::userScriptInfoCache.clear();
}


KLFUserScriptInfo::KLFUserScriptInfo(const QString& scriptFileName, KLFBackend::klfSettings * settings)
{
  QString normalizedfn = QFileInfo(scriptFileName).canonicalFilePath();
  if (Private::userScriptInfoCache.contains(normalizedfn)) {
    d = Private::userScriptInfoCache[normalizedfn];
  } else {
    d = new KLFUserScriptInfo::Private;

    d()->settings = settings;
    d()->fname = scriptFileName;

    KLF_ASSERT_NOT_NULL(settings, "Given NULL settings pointer! The KLFUserScript will not be initialized!", ; ) ;
    
    if (d()->settings != NULL) {
      d()->query_script_info();
      Private::userScriptInfoCache[normalizedfn] = d;
    }
  }
}

KLFUserScriptInfo::KLFUserScriptInfo(const KLFUserScriptInfo& copy)
{
  // will increase the refcount (thanks to KLFRefPtr)
  d = copy.d;
}

KLFUserScriptInfo::~KLFUserScriptInfo()
{
  d = NULL; // will delete the data if refcount reaches zero (see KLFRefPtr)
}

QString KLFUserScriptInfo::fileName() const
{
  return d()->fname;
}

int KLFUserScriptInfo::scriptInfoError() const
{
  return d()->scriptInfoError;
}
QString KLFUserScriptInfo::scriptInfoErrorString() const
{
  return d()->scriptInfoErrorString;
}

KLF_DEFINE_PROPERTY_GET(KLFUserScriptInfo, QString, category) ;

KLF_DEFINE_PROPERTY_GET(KLFUserScriptInfo, QString, name) ;

KLF_DEFINE_PROPERTY_GET(KLFUserScriptInfo, QString, author) ;

KLF_DEFINE_PROPERTY_GET(KLFUserScriptInfo, QString, version) ;

KLF_DEFINE_PROPERTY_GET(KLFUserScriptInfo, QString, license) ;

KLF_DEFINE_PROPERTY_GET(KLFUserScriptInfo, QString, klfMinVersion) ;

KLF_DEFINE_PROPERTY_GET(KLFUserScriptInfo, QString, klfMaxVersion) ;

KLF_DEFINE_PROPERTY_GET(KLFUserScriptInfo, QStringList, spitsOut) ;

KLF_DEFINE_PROPERTY_GET(KLFUserScriptInfo, QStringList, skipFormats) ;

KLF_DEFINE_PROPERTY_GET(KLFUserScriptInfo, QList<KLFUserScriptInfo::Param>, paramList) ;

