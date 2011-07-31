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

#include <qapplication.h>
#include <qregexp.h>
#include <qfile.h>
#include <qdatetime.h>
#include <qtextstream.h>
#include <qbuffer.h>
#include <qdir.h>


#include "klfblockprocess.h"
#include "klfbackend.h"

// write Qt 3/4 compatible code
#include "klfqt34common.h"


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
QString progErrorMsg(QString progname, int exitstatus, QString stderrstr, QString stdoutstr)
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
    KLFBackend::cleanup(tempfname);
  }
};

QString __klf_expand_env_vars(const QString& envexpr)
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

void __klf_append_replace_env_var(QStringList *list, const QString& var, const QString& line)
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
    KLFBlockProcess proc;

    exitCode = 0;
    exitStatus = 0;

    KLF_ASSERT_CONDITION(argv.size() > 0, "argv array is empty! No program is given!", return false; ) ;
    KLF_ASSERT_NOT_NULL(outdata, "Please provide a valid QByteArray outdata pointer!", return false; ) ;

    proc.setWorkingDirectory(programCwd);

    qDebug("%s: %s:  about to exec %s...", KLF_FUNC_NAME, KLF_SHORT_TIME, qPrintable(progTitle)) ;
    qDebug("\t%s", qPrintable(argv.join(" ")));
    bool r = proc.startProcess(argv, indata, execEnviron);
    qDebug("%s: %s:  %s returned.", KLF_FUNC_NAME, KLF_SHORT_TIME, qPrintable(progTitle)) ;

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

    if ( ! outFileName.isEmpty() ) {
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
      r = outfile.open(dev_READONLY);
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
      qDebug("KLFBackend/%s: Read file `%s', got data, length=%d", KLF_FUNC_NAME, qPrintable(outfile.f_filename()),
	     outdata->size());
    } else {
      // output file name is empty, read standard output
      *outdata = QByteArray();
      if (outputStdout) {
	QByteArray stdoutdata = proc.getAllStdout();
	ba_cat(outdata, stdoutdata);
      }
      if (outputStderr) {
	QByteArray stderrdata = proc.getAllStderr();
	ba_cat(outdata, stderrdata);
      }

      if (outdata->isEmpty()) {
	// no data
	QString stderrstr = (!outputStderr) ? ("\n"+proc.readStderrString()) : QString();
	qDebug("%s: %s did not provide any data.%s", KLF_FUNC_NAME, qPrintable(progTitle), qPrintable(stderrstr));
	if (resError != NULL) {
	  resError->status = resErrCodes[KLFFP_NODATA];
	  resError->errorstr = QObject::tr("Program %1 did not provide any output data.", "KLFBackend")
	    .arg(progTitle) + stderrstr;
	}
	return false;
      }
    }

    qDebug("%s: %s was successfully run and output retrieved.", KLF_FUNC_NAME, qPrintable(progTitle));

    // all OK
    exitStatus = 0;
    exitCode = 0;
    return true;
  }
};


#define D_RX "([0-9eE.-]+)"


static void replace_svg_width_or_height(QByteArray *svgdata, const char * attr, double val);
    

KLFBackend::klfOutput KLFBackend::getLatexFormula(const klfInput& in, const klfSettings& usersettings)
{
  // ALLOW ONLY ONE RUNNING getLatexFormula() AT A TIME 
  QMutexLocker mutexlocker(&__mutex);

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
      QString newenvdef = __klf_expand_env_vars(settings.execenv[k]);
      __klf_append_replace_env_var(&execenv, varname, newenvdef);
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
  initGsVersion(&settings);
  if (!gsVersion.contains(settings.gsexec)) {
    res.status = KLFERR_NOGSVERSION;
    res.errorstr = QObject::tr("Can't query version of ghostscript located at `%1'.").arg(settings.gsexec);
    return res;
  }

  /** \todo .... query gs --help to see if we have SVG driver ... ! */

  klfDebugf(("%s: queried ghostscript version: %s", KLF_FUNC_NAME, qPrintable(gsVersion[settings.gsexec]))) ;

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

  QString tempfname = settings.tempdir + "/klatexformulatmp" KLF_VERSION_STRING "-"
    + QDateTime::currentDateTime().toString("hh-mm-ss");

  QString fnTex = tempfname + ".tex";
  QString fnDvi = tempfname + ".dvi";
  QString fnRawEps = tempfname + "-dvips.eps";
  QString fnProcessedEps = tempfname + "-dvips-bbcorr-gs.eps";
  QString fnFinalEps = fnProcessedEps;
  QString fnPng = tempfname + ".png";
  QString fnPdf = tempfname + ".pdf";
  QString fnRawSvg = tempfname + "-gssvg.svg";

  // upon destruction (on stack) of this object, cleanup() will be
  // automatically called as wanted
  cleanup_caller cleanupcallerinstance(tempfname);

  QString latexsimplified = in.latex.s_trimmed();
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

  { // prepare latex file
    QFile file(fnTex);
    bool r = file.open(dev_WRITEONLY);
    if ( ! r ) {
      res.status = KLFERR_TEXWRITEFAIL;
      res.errorstr = QObject::tr("Can't open file for writing: '%1'!", "KLFBackend")
	.arg(fnTex);
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

  if (in.wrapperScript.isEmpty()) {
    // execute latex
    KLFFilterProgram p(QLatin1String("LaTeX"), &settings);
    p.resErrCodes[KLFFP_NOSTART] = KLFERR_LATEX_NORUN;
    p.resErrCodes[KLFFP_NOEXIT] = KLFERR_LATEX_NONORMALEXIT;
    p.resErrCodes[KLFFP_NOSUCCESSEXIT] = KLFERR_PROGERR_LATEX;
    p.resErrCodes[KLFFP_NODATA] = KLFERR_LATEX_NOOUTPUT;
    p.resErrCodes[KLFFP_DATAREADFAIL] = KLFERR_LATEX_OUTPUTREADFAIL;

    p.argv << settings.latexexec << dir_native_separators(fnTex);

    ok = p.run(fnDvi, &res.dvidata, &res);
    if (!ok) {
      return res;
    }
  } // end of 'latex' block
  else {
    // user has provided us a wrapper script. Query it and use it
    QByteArray scriptinfo;
    QString scriptname;
    //    bool want_full_template = true;
    { // Query Script Info phase
      KLFFilterProgram p(QLatin1String("User-Wrapper-Script"), &settings);
      p.resErrCodes[KLFFP_NOSTART] = KLFERR_USERWRAPPERSCRIPT_NORUN;
      p.resErrCodes[KLFFP_NOEXIT] = KLFERR_USERWRAPPERSCRIPT_NONORMALEXIT;
      p.resErrCodes[KLFFP_NOSUCCESSEXIT] = KLFERR_PROGERR_USERWRAPPERSCRIPT;
      p.resErrCodes[KLFFP_NODATA] = KLFERR_USERWRAPPERSCRIPT_NOSCRIPTINFO;
      p.resErrCodes[KLFFP_DATAREADFAIL] = KLFERR_USERWRAPPERSCRIPT_NOSCRIPTINFO;
      
      p.argv << in.wrapperScript << "--scriptinfo";
      
      ok = p.run(QString(), &scriptinfo, &res);
      if (!ok) {
	return res;
      }
    }
    scriptinfo.replace("\r\n", "\n");
    if (scriptinfo.isEmpty()) {
      scriptinfo = "ScriptInfo\n";
    }
    // parse scriptinfo
    if (!scriptinfo.startsWith("ScriptInfo\n")) {
      qWarning()<<KLF_FUNC_NAME<<": User script did not provide valid --scriptinfo.";
      res.status = KLFERR_USERWRAPPERSCRIPT_INVALIDSCRIPTINFO;
      res.errorstr = QObject::tr("User wrapper script provided invalid --scriptinfo output.", "KLFBackend");
      return res;
    }
    QList<QByteArray> lines = scriptinfo.split('\n');
    lines.removeAt(0); // skip of course the 'ScriptInfo\n' line !
    int kkl;
    for (kkl = 0; kkl < lines.size(); ++kkl) {
      QString line = QString::fromLocal8Bit(lines[kkl]);
      if (line.trimmed().isEmpty())
	continue;
      QRegExp rx("(\\w+):\\s*(.*)");
      QRegExp boolrxtrue("(t(rue)?|y(es)?|on|1)");
      if (!rx.exactMatch(line)) {
	klfDbg("Buggy line: "<<line);
	qWarning()<<KLF_FUNC_NAME<<": User script did not provide valid --scriptinfo.";
	res.status = KLFERR_USERWRAPPERSCRIPT_INVALIDSCRIPTINFO;
	res.errorstr = QObject::tr("User wrapper script provided invalid --scriptinfo output.", "KLFBackend");
	return res;
      }
      QString key = rx.cap(1);
      QString val = rx.cap(2).trimmed();
      if (key == QLatin1String("Name")) {
	scriptname = val;
      } else {
	klfDbg("Unknown userscript info key: "<<key<<", in line:\n"<<line);
	qWarning()<<KLF_FUNC_NAME<<": Ignoring unknown user script info key "<<key<<".";
      }
    }
    // and run the script with the latex input
    QStringList addenv;
    addenv << "KLF_TEMPDIR="+settings.tempdir
	   << "KLF_LATEX="+settings.latexexec
	   << "KLF_DVIPS="+settings.dvipsexec
	   << "KLF_GS="+settings.gsexec
	   << "KLF_INPUT_LATEX=" + in.latex;
    { // now run the script
      KLFFilterProgram p(QLatin1String("User Wrapper Script"), &settings);
      p.resErrCodes[KLFFP_NOSTART] = KLFERR_USERWRAPPERSCRIPT_NORUN;
      p.resErrCodes[KLFFP_NOEXIT] = KLFERR_USERWRAPPERSCRIPT_NONORMALEXIT;
      p.resErrCodes[KLFFP_NOSUCCESSEXIT] = KLFERR_PROGERR_USERWRAPPERSCRIPT;
      p.resErrCodes[KLFFP_NODATA] = KLFERR_USERWRAPPERSCRIPT_NOOUTPUT;
      p.resErrCodes[KLFFP_DATAREADFAIL] = KLFERR_USERWRAPPERSCRIPT_OUTPUTREADFAIL;
      
      p.argv << in.wrapperScript << dir_native_separators(fnTex);
      
      ok = p.run(fnDvi, &res.dvidata, &res);
      if (!ok) {
	return res;
      }
    }
  }

  QByteArray rawepsdata;

  { // execute dvips -E
    KLFFilterProgram p(QLatin1String("dvips"), &settings);
    p.resErrCodes[KLFFP_NOSTART] = KLFERR_DVIPS_NORUN;
    p.resErrCodes[KLFFP_NOEXIT] = KLFERR_DVIPS_NONORMALEXIT;
    p.resErrCodes[KLFFP_NOSUCCESSEXIT] = KLFERR_PROGERR_DVIPS;
    p.resErrCodes[KLFFP_NODATA] = KLFERR_DVIPS_NOOUTPUT;
    p.resErrCodes[KLFFP_DATAREADFAIL] = KLFERR_DVIPS_OUTPUTREADFAIL;

    p.argv << settings.dvipsexec << "-E" << dir_native_separators(fnDvi) << "-o" << dir_native_separators(fnRawEps);

    ok = p.run(fnRawEps, &rawepsdata, &res);

    if (!ok) {
      return res;
    }
    qDebug("%s: read raw EPS; rawepsdata/length=%d", KLF_FUNC_NAME, rawepsdata.size());
  } // end of 'dvips' block

  // width and height of the (final) EPS bbox in postscript points
  double width_pt = 0, height_pt = 0;

  { // find correct bounding box of EPS file, using ghostscript, and modify the EPS file manually
    struct klfbbox {
      double x1, x2, y1, y2;
    }  bbox,  bbox_corrected;
    double offx, offy;

    int i;

    if (settings.calcEpsBoundingBox) {

      KLFFilterProgram p(QLatin1String("GhostScript (bbox)"), &settings);
      p.resErrCodes[KLFFP_NOSTART] = KLFERR_GSBBOX_NORUN;
      p.resErrCodes[KLFFP_NOEXIT] = KLFERR_GSBBOX_NONORMALEXIT;
      p.resErrCodes[KLFFP_NOSUCCESSEXIT] = KLFERR_PROGERR_GSBBOX;
      p.resErrCodes[KLFFP_NODATA] = KLFERR_GSBBOX_NOOUTPUT;
      // p.resErrCodes[KLFFP_DATAREADFAIL]  unused

      p.outputStdout = true;
      p.outputStderr = true;
    
      QByteArray bboxdata;

      p.argv << settings.gsexec << "-dNOPAUSE" << "-dSAFER" << "-sDEVICE=bbox" << "-q" << "-dBATCH"
	     << fnRawEps;

      ok = p.run(QString(), &bboxdata, &res);
      if (!ok) {
	return res;
      }

      qDebug("%s: gs provided output:\n%s\n", KLF_FUNC_NAME, (const char*)bboxdata);

      // parse gs' bbox data
      QRegExp rx_gsbbox("%%HiResBoundingBox\\s*:\\s+" D_RX "\\s+" D_RX "\\s+" D_RX "\\s+" D_RX "");
      i = rx_gsbbox.rx_indexin(QString::fromLatin1(bboxdata));
      if (i < 0) {
	res.status = KLFERR_GSBBOX_NOBBOX;
	res.errorstr = QObject::tr("Ghostscript did not provide parsable BBox output!", "KLFBackend");
	return res;
      }
      bbox.x1 = rx_gsbbox.cap(1).toDouble()  -  settings.lborderoffset;
      bbox.y1 = rx_gsbbox.cap(2).toDouble()  -  settings.bborderoffset;
      bbox.x2 = rx_gsbbox.cap(3).toDouble()  +  settings.rborderoffset;
      bbox.y2 = rx_gsbbox.cap(4).toDouble()  +  settings.tborderoffset;

    } else {
      qDebug("%s: reading EPS BoundingBox from dvips output", KLF_FUNC_NAME);

      // Read dvips' bounding box.
      buf_decl_ba(QBuffer buf, rawepsdata);
      bool r = buf.open(dev_READONLY | dev_TEXTTRANSLATE);
      if (!r) {
	qWarning("%s: %s: What's going on!!?! can't open buffer for reading?", KLF_FUNC_NAME, KLF_SHORT_TIME) ;
      }

      bool havebbox = false;

      QString nobboxerrstr =
	QObject::tr("DVIPS did not provide parsable %%BoundingBox: in its output!", "KLFBackend");

      char linebuffer[256];
      int n;
      while ((n = buf.readLine(linebuffer, 256)) > 0) {
	static const char * bboxtag = "%%BoundingBox:";
	static const int bboxtaglen = strlen(bboxtag);
	if (!strncmp(linebuffer, bboxtag, bboxtaglen)) {
	  // got bounding-box.
	  // parse bbox values
	  QRegExp rx_bbvalues("" D_RX "\\s+" D_RX "\\s+" D_RX "\\s+" D_RX "");
	  i = rx_bbvalues.rx_indexin(QString::fromLatin1(linebuffer + bboxtaglen));
	  if (i < 0) {
	    res.status = KLFERR_DVIPS_OUTPUTNOBBOX;
	    res.errorstr = nobboxerrstr;
	    return res;
	  }
	  bbox.x1 = rx_bbvalues.cap(1).toDouble()  -  settings.lborderoffset;
	  bbox.y1 = rx_bbvalues.cap(2).toDouble()  -  settings.bborderoffset;
	  bbox.x2 = rx_bbvalues.cap(3).toDouble()  +  settings.rborderoffset;
	  bbox.y2 = rx_bbvalues.cap(4).toDouble()  +  settings.tborderoffset;
	  havebbox = true;
	  break;
	}
      }
      if (!havebbox) {
	res.status = KLFERR_DVIPS_OUTPUTNOBBOX;
	res.errorstr = nobboxerrstr;
	return res;
      }
    }

    qDebug("%s: BoundingBox is %f %f %f %f", KLF_FUNC_NAME, bbox.x1, bbox.y1, bbox.x2, bbox.y2);

    offx = -bbox.x1;
    offy = -bbox.y1;
    width_pt = bbox.x2 - bbox.x1;
    height_pt = bbox.y2 - bbox.y1;
    bbox_corrected.x1 = 0;
    bbox_corrected.y1 = 0;
    bbox_corrected.x2 = width_pt;
    bbox_corrected.y2 = height_pt;

    klfDebugf(("%s: Corrected BoundingBox to %f %f %f %f", KLF_FUNC_NAME, bbox_corrected.x1, bbox_corrected.y1,
	       bbox_corrected.x2, bbox_corrected.y2)) ;

    // in raw EPS data, find '%%BoundingBox:'
    int len;
    char nl[] = "\0\0\0";
    i = ba_indexOf(rawepsdata, "%%BoundingBox:");
    if (i < 0) {
      i = 0;
      len = 0;
    } else {
      int j = i+14; // 14==strlen("%%BoundingBox:")
      while (j < (int)rawepsdata.size() && rawepsdata[j] != '\r' && rawepsdata[j] != '\n')
	++j;
      len = j-i;
      if (rawepsdata[j] == '\r' && j < (int)rawepsdata.size()-1 && rawepsdata[j+1] == '\n') {
	nl[0] = '\r', nl[1] = '\n';
      } else {
	nl[0] = rawepsdata[j];
      }
    }

#ifdef KLF_DEBUG
    {    // DUMP RAW EPS file
      fprintf(stderr, "Raw EPS (dvips output) file contents is:\n");
      for (int k = 0; k < (int)rawepsdata.size(); ++k)
	fputc(rawepsdata[k], stderr);
      fprintf(stderr, "\n");
    }
#endif

    // recall that '%%' in printf is replaced by a single '%'...
    int wi = (int)(bbox_corrected.x2 + 0.99999) ;
    int hi = (int)(bbox_corrected.y2 + 0.99999) ;
    char buffer[256];
    int buffer_len;
    snprintf(buffer, 255,
	     "%%%%BoundingBox: 0 0 %d %d%s"
	     "%%%%HiResBoundingBox: 0 0 %s %s%s",
	     wi, hi, nl, klfFmtDoubleCC(bbox_corrected.x2, 'g', 6), klfFmtDoubleCC(bbox_corrected.y2, 'g', 6), nl);
    buffer_len = strlen(buffer);

    char buffer2[256];
    int buffer2_len;
    snprintf(buffer2, 255,
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

    qDebug("%s: buffer is '%s', length=%d", KLF_FUNC_NAME, buffer, buffer_len);

    qDebug("%s: %s: rawepsdata has length=%d", KLF_FUNC_NAME, KLF_SHORT_TIME, rawepsdata.size());

    // and modify the raw EPS data, to replace "%%BoundingBox:" instruction by our stuff...
    ba_replace(&rawepsdata, i, len, buffer);

    const char * endsetupstr = "%%EndSetup";
    int i2 = ba_indexOf(rawepsdata, endsetupstr);
    if (i2 < 0)
      i2 = i + buffer_len; // add our info after modified %%BoundingBox'es instructions
    else
      i2 +=  strlen(endsetupstr);

    ba_replace(&rawepsdata, i2, 0, buffer2);

    qDebug("%s: %s: rawepsdata has now length=%d", KLF_FUNC_NAME, KLF_SHORT_TIME, rawepsdata.size());
    qDebug("%s: %s: New eps bbox is [0 0 %.6g %.6g] with translate [%.6g %.6g].", KLF_FUNC_NAME, KLF_SHORT_TIME,
	   bbox_corrected.x2, bbox_corrected.y2, offx, offy);

#ifdef KLF_DEBUG
    {    // DUMP EPS file
      fprintf(stderr, "Corrected BBox EPS file contents is:\n");
      for (int k = 0; k < (int)rawepsdata.size(); ++k)
	fputc(rawepsdata[k], stderr);
      fprintf(stderr, "\n");
    }
#endif

    res.epsdata_raw = rawepsdata; // [Qt3: shallow copy is OK]

  } // end of block "correct EPS BBox"

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
    // what's going on ...
    p.argv << "-dNOCACHE";
    p.argv << "-dNOPAUSE" << "-dSAFER" << "-dEPSCrop" << "-sDEVICE=pswrite";
    p.argv << "-sOutputFile="+dir_native_separators(fnProcessedEps)
	   << "-q" << "-dBATCH" << "-";

    ok = p.run(rawepsdata, fnProcessedEps, &res.epsdata, &res);
    if (!ok) {
      return res;
    }

    klfDebugf(("%s: res.epsdata has length=%d", KLF_FUNC_NAME, res.epsdata.size())) ;

  } else {
    // no post-processed EPS, copy raw (bbox-corrected) EPS data:
    res.epsdata.ba_assign(rawepsdata);
  }

  { // run 'gs' to get PNG data
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
    p.argv << "-sOutputFile="+dir_native_separators(fnPng) << "-q" << "-dBATCH" << "-";

    ok = p.run(rawepsdata, fnPng, &res.pngdata_raw, &res);
    if (!ok) {
      return res;
    }

    res.result.loadFromData(res.pngdata_raw, "PNG");

    QString boolstr[2] = { QLatin1String("true"), QLatin1String("false") } ;

    // store some meta-information into result
    res.result.img_settext("AppVersion", QString::fromLatin1("KLatexFormula " KLF_VERSION_STRING));
    res.result.img_settext("Application",
			   QObject::tr("Created with KLatexFormula version %1", "KLFBackend::saveOutputToFile"));
    res.result.img_settext("Software", QString::fromLatin1("KLatexFormula " KLF_VERSION_STRING));
    res.result.img_settext("InputLatex", in.latex);
    res.result.img_settext("InputMathMode", in.mathmode);
    res.result.img_settext("InputPreamble", in.preamble);
    res.result.img_settext("InputFgColor", QString("rgb(%1, %2, %3)").arg(qRed(in.fg_color))
			   .arg(qGreen(in.fg_color)).arg(qBlue(in.fg_color)));
    res.result.img_settext("InputBgColor", QString("rgba(%1, %2, %3, %4)").arg(qRed(in.bg_color))
			   .arg(qGreen(in.bg_color)).arg(qBlue(in.bg_color))
			   .arg(qAlpha(in.bg_color)));
    res.result.img_settext("InputDPI", QString::number(in.dpi));
    res.result.img_settext("SettingsTBorderOffset", QString::number(settings.tborderoffset));
    res.result.img_settext("SettingsRBorderOffset", QString::number(settings.rborderoffset));
    res.result.img_settext("SettingsBBorderOffset", QString::number(settings.bborderoffset));
    res.result.img_settext("SettingsLBorderOffset", QString::number(settings.lborderoffset));
    res.result.img_settext("SettingsOutlineFonts", boolstr[(int)settings.outlineFonts]);
    res.result.img_settext("SettingsCalcEpsBoundingBox", boolstr[(int)settings.calcEpsBoundingBox]);
    res.result.img_settext("SettingsWantPDF", boolstr[(int)settings.wantPDF]);
    res.result.img_settext("SettingsWantSVG", boolstr[(int)settings.wantSVG]);

    qDebug("%s: prepared QImage.", KLF_FUNC_NAME) ;
    
    { // create "final" PNG data
      buf_decl_ba(QBuffer buf, res.pngdata);
      buf.open(dev_WRITEONLY);

      bool r = res.result.save(&buf, "PNG");
      if (!r) {
	qWarning("%s: Error: Can't save \"final\" PNG data.", KLF_FUNC_NAME);
	res.pngdata.ba_assign(res.pngdata_raw);
      }
    }

    qDebug("%s: prepared final PNG data.", KLF_FUNC_NAME) ;
  }

  { // run 'gs' to get PDF data
    KLFFilterProgram p(QLatin1String("gs (PDF)"), &settings);
    p.resErrCodes[KLFFP_NOSTART] = KLFERR_GSPDF_NORUN;
    p.resErrCodes[KLFFP_NOEXIT] = KLFERR_GSPDF_NONORMALEXIT;
    p.resErrCodes[KLFFP_NOSUCCESSEXIT] = KLFERR_PROGERR_GSPDF;
    p.resErrCodes[KLFFP_NODATA] = KLFERR_GSPDF_NOOUTPUT;
    p.resErrCodes[KLFFP_DATAREADFAIL] = KLFERR_GSPDF_OUTPUTREADFAIL;

    p.argv << settings.gsexec
	   << "-dNOPAUSE" << "-dSAFER" << "-sDEVICE=pdfwrite"
	   << "-sOutputFile="+dir_native_separators(fnPdf)
	   << "-q" << "-dBATCH" << "-";

    // input: res.epsdata is the processed EPS file, or the raw EPS + bbox/page correction if no post-processing
    ok = p.run(res.epsdata, fnPdf, &res.pdfdata, &res);
    if (!ok) {
      return res;
    }
  }

  if (settings.wantSVG) {

    QString thisgsversion = gsVersion[settings.gsexec];

    if (klfVersionCompare(thisgsversion, "8.64") < 0) {
      // not OK to get SVG...
      qWarning("%s: ghostscript is too old (%s < 8.64). cannot create SVG", KLF_FUNC_NAME,
	       qPrintable(thisgsversion));
      res.status = KLFERR_GSSVG_TOOOLD;
      res.errorstr = QObject::tr("This ghostscript is too old (%1 < 8.64) to generate SVG.", "KLFBackend")
	.arg(thisgsversion);
      return res;
    }
      
    QByteArray rawsvgdata;

    { // run 'gs' to get SVG
      KLFFilterProgram p(QLatin1String("gs (SVG)"), &settings);
      p.resErrCodes[KLFFP_NOSTART] = KLFERR_GSSVG_NORUN;
      p.resErrCodes[KLFFP_NOEXIT] = KLFERR_GSSVG_NONORMALEXIT;
      p.resErrCodes[KLFFP_NOSUCCESSEXIT] = KLFERR_PROGERR_GSSVG;
      p.resErrCodes[KLFFP_NODATA] = KLFERR_GSSVG_NOOUTPUT;
      p.resErrCodes[KLFFP_DATAREADFAIL] = KLFERR_GSSVG_OUTPUTREADFAIL;
      
      p.argv << settings.gsexec;
      // unconditionally outline fonts, otherwise output is horrible
      p.argv << "-dNOCACHE" << "-dNOPAUSE" << "-dSAFER" << "-dEPSCrop" << "-sDEVICE=svg"
	     << "-sOutputFile="+dir_native_separators(fnRawSvg)
	     << "-q" << "-dBATCH" << "-";

      // input: res.epsdata is the processed EPS file, or the raw EPS file if no post-processing
      ok = p.run(rawepsdata, fnRawSvg, &rawsvgdata, &res);
      if (!ok) {
	return res;
      }
    }

    // and now re-touch SVG generated by ghostscript that is not very clean...
    // find the first occurences of width='' and height='' and set them to the
    // appropriate width and heights given by BBox read earlier

    klfDebugf(("%s: rawsvgdata/length=%d", KLF_FUNC_NAME, rawsvgdata.size())) ;

    replace_svg_width_or_height(&rawsvgdata, "width=", width_pt);
    replace_svg_width_or_height(&rawsvgdata, "height=", height_pt);

    klfDebugf(("%s: now, rawsvgdata/length=%d", KLF_FUNC_NAME, rawsvgdata.size())) ;

    res.svgdata = rawsvgdata;

  } // end if(wantSVG)

  qDebug("%s: %s:  end of function.", KLF_FUNC_NAME, KLF_SHORT_TIME) ;    

  return res;
}


static void replace_svg_width_or_height(QByteArray *svgdata, const char * attreq, double val)
{
  QByteArray & svgdataref = * svgdata;

  int i = ba_indexOf(svgdataref, attreq);
  int j = i;
  while (j < (int)svgdataref.size() && (!isspace(svgdataref[j]) && svgdataref[j] != '>'))
    ++j;

  char buffer[256];
  snprintf(buffer, 256, "%s'%s'", attreq, klfFmtDoubleCC(val, 'f', 3));

  ba_replace(svgdata, i, j-i, buffer);
}
   



void KLFBackend::cleanup(QString tempfname)
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

// static private mutex object
QMutex KLFBackend::__mutex;

KLF_EXPORT bool operator==(const KLFBackend::klfInput& a, const KLFBackend::klfInput& b)
{
  return a.latex == b.latex &&
    a.mathmode == b.mathmode &&
    a.preamble == b.preamble &&
    a.fg_color == b.fg_color &&
    a.bg_color == b.bg_color &&
    a.dpi == b.dpi &&
    a.bypassTemplate == b.bypassTemplate;
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



bool KLFBackend::saveOutputToDevice(const klfOutput& klfoutput, QIODevice *device,
				    const QString& fmt, QString *errorStringPtr)
{
  QString format = fmt.s_trimmed().s_toUpper();

  // now choose correct data source and write to fout
  if (format == "EPS" || format == "PS") {
    device->dev_write(klfoutput.epsdata);
  } else if (format == "PNG") {
    device->dev_write(klfoutput.pngdata);
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

  settings->wantSVG = false; // will be set to TRUE once we verify 'gs' version

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
    initGsVersion(settings);
    if (!gsVersion.contains(settings->gsexec)) {
      qWarning("%s: cannot get 'gs' version with `%s --version'.", KLF_FUNC_NAME, qPrintable(settings->gsexec));
    } else if (klfVersionCompare(gsVersion[settings->gsexec], "8.64") >= 0) {
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
      + dir_native_separators(gsfi.fi_absolutePath()+"/../../ghostscript/base")
      + ";"
      + dir_native_separators(gsfi.fi_absolutePath()+"/../../fonts");
    __klf_append_replace_env_var(& settings->execenv, "MIKTEX_GS_LIB", mgsenv);
    klfDbg("Adjusting environment for mgs.exe: `"+mgsenv+"'") ;
  }

#ifdef Q_WS_MAC
  // make sure that epstopdf's path is in PATH because it wants to all gs
  // (eg fink distributions)
  if (!settings->epstopdfexec.isEmpty()) {
    QFileInfo epstopdf_fi(settings->epstopdfexec);
    QString execenvpath = QString("PATH=%1:$PATH").arg(epstopdf_fi.fi_absolutePath());
    __klf_append_replace_env_var(& settings->execenv, "PATH", execenvpath);
  }
#endif

  return true;
}




QMap<QString,QString> KLFBackend::gsVersion = QMap<QString,QString>();

// static 
void KLFBackend::initGsVersion(const KLFBackend::klfSettings *settings)
{
  if (gsVersion.contains(settings->gsexec)) // value already cached
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
      gsver = gsver.s_trimmed();
      if (!gsver.isEmpty() && klfIsValidVersion(gsver)) {
	gsVersion[settings->gsexec] = gsver;
      }
    }
  }
}
