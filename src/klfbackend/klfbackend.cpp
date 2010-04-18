/***************************************************************************
 *   file klfbackend.cpp
 *   This file is part of the KLatexFormula Project.
 *   Copyright (C) 2007 by Philippe Faist
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
#include <sys/time.h>

#include <qregexp.h>
#include <qfile.h>
#include <qdatetime.h>
#include <qtextstream.h>
#include <qdir.h>

#ifdef KLFBACKEND_QT4
#include <qdebug.h>
#endif

#include "klfblockprocess.h"
#include "klfbackend.h"


// declared in klfdefs.h

void __klf_debug_time_print(QString str)
{
  struct timeval tv;
  gettimeofday(&tv, NULL);
  qDebug("%03ld.%06ld : %s", tv.tv_sec % 1000, tv.tv_usec,
#ifdef KLFBACKEND_QT4
	 qPrintable(str)
#else
	 str.local8Bit().data()
#endif

	 );
}

#ifdef KLFBACKEND_QT4
#define NATIVESEPARATORS(x) QDir::toNativeSeparators(x)
#else
#define NATIVESEPARATORS(x) QDir::convertSeparators(x)
#endif


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
    return QObject::tr("<p><b>%1</b> reported an error (exit status %2). No Output was generated.</p>", "KLFBackend")
	.arg(progname).arg(exitstatus);
  if (stderrstr.isEmpty())
    return QObject::tr("<p><b>%1</b> reported an error (exit status %2). Here is full stdout output:</p>\n"
		       "<pre>\n%3</pre>", "KLFBackend")
      .arg(progname).arg(exitstatus).arg(stdouthtml);
  if (stdoutstr.isEmpty())
    return QObject::tr("<p><b>%1</b> reported an error (exit status %2). Here is full stderr output:</p>\n"
		       "<pre>\n%3</pre>", "KLFBackend")
      .arg(progname).arg(exitstatus).arg(stderrhtml);
  
  return QObject::tr("<p><b>%1</b> reported an error (exit status %2). Here is full stderr output:</p>\n"
		     "<pre>\n%3</pre><p>And here is full stdout output:</p><pre>\n%4</pre>", "KLFBackend")
    .arg(progname).arg(exitstatus).arg(stderrhtml).arg(stdouthtml);
}


/** Internal.
 * \internal */
struct cleanup_caller {
  QString tempfname;
  cleanup_caller(QString fn) : tempfname(fn) { }
  ~cleanup_caller() {
    KLFBackend::cleanup(tempfname);
  }
};

KLFBackend::klfOutput KLFBackend::getLatexFormula(const klfInput& in, const klfSettings& settings)
{
  // ALLOW ONLY ONE RUNNING getLatexFormula() AT A TIME 
  QMutexLocker mutexlocker(&__mutex);

  klf_debug_time_print("KLFBackend::getLatexFormula() called.\n");
  
  klfOutput res;
  res.status = KLFERR_NOERROR;
  res.errorstr = QString();
  res.result = QImage();
  res.pngdata = QByteArray();
  res.epsdata = QByteArray();
  res.pdfdata = QByteArray();

  // PROCEDURE:
  // - generate LaTeX-file
  // - latex --> get DVI file
  // - dvips -E file.dvi it to get an EPS file
  // - Run gs:	gs -dNOPAUSE -dSAFER -dEPSCrop -r600 -dTextAlphaBits=4 -dGraphicsAlphaBits=4
  //              -sDEVICE=pngalpha|png16m -sOutputFile=xxxxxx.png -q -dBATCH xxxxxx.eps   to eventually get PNG data
  // - if epstopdfexec is not empty, run epstopdf and get PDF file.

  QString tempfname = settings.tempdir + "/klatexformulatmp2-"
    + QDateTime::currentDateTime().toString("hh-mm-ss");

  // upon destruction (on stack) of this object, cleanup() will be
  // automatically called as wanted
  cleanup_caller cleanupcallerinstance(tempfname);

#ifdef KLFBACKEND_QT4
  QString latexsimplified = in.latex.trimmed();
#else
  QString latexsimplified = in.latex.stripWhiteSpace();
#endif
  if (latexsimplified.isEmpty()) {
    res.errorstr = QObject::tr("You must specify a LaTeX formula!", "KLFBackend");
    res.status = KLFERR_MISSINGLATEXFORMULA;
    return res;
  }

  QString latexin;
  if (in.mathmode.contains("...") == 0) {
    res.status = KLFERR_MISSINGMATHMODETHREEDOTS;
    res.errorstr = QObject::tr("The math mode string doesn't contain '...'!", "KLFBackend");
    return res;
  }
  latexin = in.mathmode;
  latexin.replace("...", in.latex);

  {
    QFile file(tempfname+".tex");
#ifdef KLFBACKEND_QT4
    bool r = file.open(QIODevice::WriteOnly);
#else
    bool r = file.open(IO_WriteOnly);
#endif
    if ( ! r ) {
      res.status = KLFERR_TEXWRITEFAIL;
      res.errorstr = QObject::tr("Can't open file for writing: '%1'!", "KLFBackend").arg(tempfname+".tex");
      return res;
    }
    QTextStream stream(&file);
    stream << "\\documentclass{article}\n"
	   << "\\usepackage[dvips]{color}\n"
	   << in.preamble << "\n"
	   << "\\begin{document}\n"
	   << "\\thispagestyle{empty}\n"
	   << QString("\\definecolor{klffgcolor}{rgb}{%1,%2,%3}\n").arg(qRed(in.fg_color)/255.0)
      .arg(qGreen(in.fg_color)/255.0).arg(qBlue(in.fg_color)/255.0)
	   << QString("\\definecolor{klfbgcolor}{rgb}{%1,%2,%3}\n").arg(qRed(in.bg_color)/255.0)
      .arg(qGreen(in.bg_color)/255.0).arg(qBlue(in.bg_color)/255.0)
	   << ( (qAlpha(in.bg_color)>0) ? "\\pagecolor{klfbgcolor}\n" : "" )
	   << "{\\color{klffgcolor} " << latexin << " }\n"
	   << "\\end{document}\n";
  }

  { // execute latex

    KLFBlockProcess proc;
    QStringList args;
    QStringList env;

    proc.setWorkingDirectory(settings.tempdir);

    args << settings.latexexec << NATIVESEPARATORS(tempfname+".tex");

    klf_debug_time_print("KLFBackend::getLatexFormula: about to exec latex...\n");
    bool r = proc.startProcess(args, env);
    klf_debug_time_print("KLFBackend::getLatexFormula: latex returned.\n");

    if (!r) {
      res.status = KLFERR_NOLATEXPROG;
      res.errorstr = QObject::tr("Unable to start Latex program %1!", "KLFBackend").arg(settings.latexexec);
      return res;
    }
    if (!proc.normalExit()) {
      res.status = KLFERR_LATEXNONORMALEXIT;
      res.errorstr = QObject::tr("Latex was killed!", "KLFBackend");
      return res;
    }
    if (proc.exitStatus() != 0) {
      res.status = KLFERR_PROGERR_LATEX;
      res.errorstr = progErrorMsg("LaTeX", proc.exitStatus(), proc.readStderrString(), proc.readStdoutString());
      return res;
    }

    if (!QFile::exists(tempfname + ".dvi")) {
      res.status = KLFERR_NODVIFILE;
      res.errorstr = QObject::tr("DVI file didn't appear after having called Latex!", "KLFBackend");
      return res;
    }

  } // end of 'latex' block

  { // execute dvips -E

    KLFBlockProcess proc;
    QStringList args;
    args << settings.dvipsexec << "-E" << NATIVESEPARATORS(tempfname+".dvi")
         << "-o" << NATIVESEPARATORS(tempfname+".eps");

    klf_debug_time_print("KLFBackend::getLatexFormula: about to dvips...\n");
    bool r = proc.startProcess(args);
    klf_debug_time_print("KLFBackend::getLatexFormula: dvips returned.\n");

    if ( ! r ) {
      res.status = KLFERR_NODVIPSPROG;
      res.errorstr = QObject::tr("Unable to start dvips!\n", "KLFBackend");
      return res;
    }
    if ( !proc.normalExit() ) {
      res.status = KLFERR_DVIPSNONORMALEXIT;
      res.errorstr = QObject::tr("Dvips was mercilessly killed!\n", "KLFBackend");
      return res;
    }
    if ( proc.exitStatus() != 0) {
      res.status = KLFERR_PROGERR_DVIPS;
      res.errorstr = progErrorMsg("dvips", proc.exitStatus(), proc.readStderrString(), proc.readStdoutString());
      return res;
    }
    if (!QFile::exists(tempfname + ".eps")) {
      res.status = KLFERR_NOEPSFILE;
      res.errorstr = QObject::tr("EPS file didn't appear after dvips call!\n", "KLFBackend");
      return res;
    }

    // add some space on bounding-box to avoid some too tight bounding box bugs
    // read eps file
    QFile epsfile(tempfname+".eps");
#ifdef KLFBACKEND_QT4
    r = epsfile.open(QIODevice::ReadOnly);
#else
    r = epsfile.open(IO_ReadOnly);
#endif
    if ( ! r ) {
      res.status = KLFERR_EPSREADFAIL;
      res.errorstr = QObject::tr("Can't read file '%1'!\n", "KLFBackend").arg(tempfname+".eps");
      return res;
    }
    QByteArray epscontent = epsfile.readAll();
#ifdef KLFBACKEND_QT4
    int i = epscontent.indexOf("%%BoundingBox: ");
    if ( i == -1 ) {
      res.status = KLFERR_NOEPSBBOX -11;
      res.errorstr = QObject::tr("File '%1' does not contain line \"%%BoundingBox: ... \" !", "KLFBackend")
	.arg(tempfname+".eps");
    }
    int ax, ay, bx, by;
    char temp[250];
    int k = i;
    i += strlen("%%BoundingBox:");
    int n = sscanf(epscontent.constData()+i, "%d %d %d %d", &ax, &ay, &bx, &by);
    if ( n != 4 ) {
      res.status = KLFERR_BADEPSBBOX;
      res.errorstr = QObject::tr("file %1: Line %%BoundingBox: can't read values!\n", "KLFBackend")
	.arg(tempfname+".eps");
      return res;
    }
    // Don't forget: '%' in printf has special meaning (!)
    sprintf(temp, "%%%%BoundingBox: %d %d %d %d", ax-settings.lborderoffset, ay-settings.bborderoffset,
	    bx+settings.rborderoffset, by+settings.tborderoffset); // grow bbox by settings.?borderoffset points
    QString chunk = QString::fromLocal8Bit(epscontent.constData()+k);
    QRegExp rx("^%%BoundingBox: [0-9]+ [0-9]+ [0-9]+ [0-9]+");
    (void) rx.indexIn(chunk);
    int l = rx.matchedLength();
    epscontent.replace(k, l, temp);
    res.epsdata = epscontent;
#else
    QCString epscontent_s(epscontent.data(), epscontent.size());
    // process file data and transform it
    int i = epscontent_s.find("%%BoundingBox: ");
    if ( i == -1 ) {
      res.status = KLFERR_NOEPSBBOX;
      res.errorstr = QObject::tr("File '%1' does not contain line \"%%BoundingBox: ... \" !", "KLFBackend")
	.arg(tempfname+".eps");
      return res;
    }
    int ax, ay, bx, by;
    char temp[250];
    i += strlen("%%BoundingBox:");
    int n = sscanf((const char*)epscontent_s+i, "%d %d %d %d", &ax, &ay, &bx, &by);
    if ( n != 4 ) {
      res.status = KLFERR_BADEPSBBOX;
      res.errorstr = QObject::tr("file %1: Line %%BoundingBox: can't read values!\n", "KLFBackend")
	.arg(tempfname+".eps");
      return res;
    }
    sprintf(temp, "%%%%BoundingBox: %d %d %d %d", ax-settings.lborderoffset, ay-settings.bborderoffset,
	    bx+settings.rborderoffset, by+settings.tborderoffset); // grow bbox by settings.?borderoffset points
    epscontent_s.replace(QRegExp("%%BoundingBox: [0-9]+ [0-9]+ [0-9]+ [0-9]+"), temp);
    res.epsdata.duplicate(epscontent_s.data(), epscontent_s.length());
#endif
    // write content back to second file
    QFile epsgoodfile(tempfname+"-good.eps");
#ifdef KLFBACKEND_QT4
    r = epsgoodfile.open(QIODevice::WriteOnly);
#else
    r = epsgoodfile.open(IO_WriteOnly);
#endif
    if ( ! r ) {
      res.status = KLFERR_EPSWRITEFAIL;
      res.errorstr = QObject::tr("Can't write to file '%1'!\n", "KLFBackend")
	.arg(tempfname+"-good.eps");
      return res;
    }
#ifdef KLFBACKEND_QT4
    epsgoodfile.write(res.epsdata);
#else
    epsgoodfile.writeBlock(res.epsdata);
#endif
    // res.epsdata is now set.

    klf_debug_time_print("KLFBackend::getLatexFormula: eps bbox set.\n");    

  } // end of block "make EPS"

  { // run 'gs' to get png
    KLFBlockProcess proc;
    QStringList args;
    args << settings.gsexec << "-dNOPAUSE" << "-dSAFER" << "-dEPSCrop" << "-r"+QString::number(in.dpi)
	 << "-dTextAlphaBits=4" << "-dGraphicsAlphaBits=4";
    if (qAlpha(in.bg_color) > 0) { // we're forcing a background color
      args << "-sDEVICE=png16m";
    } else {
      args << "-sDEVICE=pngalpha";
    }
    args << "-sOutputFile="+NATIVESEPARATORS(tempfname+".png") << "-q" << "-dBATCH"
         << NATIVESEPARATORS(tempfname+"-good.eps");

    klf_debug_time_print("KLFBackend::getLatexFormula: about to gs...\n");    
    bool r = proc.startProcess(args);
    klf_debug_time_print("KLFBackend::getLatexFormula: gs returned.\n");    
  
    if ( ! r ) {
      res.status = KLFERR_NOGSPROG;
      res.errorstr = QObject::tr("Unable to start gs!\n", "KLFBackend");
      return res;
    }
    if ( !proc.normalExit() ) {
      res.status = KLFERR_GSNONORMALEXIT;
      res.errorstr = QObject::tr("gs died abnormally!\n", "KLFBackend");
      return res;
    }
    if ( proc.exitStatus() != 0) {
      res.status = KLFERR_PROGERR_GS;
      res.errorstr = progErrorMsg("gs", proc.exitStatus(), proc.readStderrString(), proc.readStdoutString());
      return res;
    }
    if (!QFile::exists(tempfname + ".png")) {
      res.status = KLFERR_NOPNGFILE;
      res.errorstr = QObject::tr("PNG file didn't appear after call to gs!\n", "KLFBackend");
      return res;
    }

    // get and save PNG to memory
    QFile pngfile(tempfname+".png");
#ifdef KLFBACKEND_QT4
    r = pngfile.open(QIODevice::ReadOnly);
#else
    r = pngfile.open(IO_ReadOnly);
#endif
    if ( ! r ) {
      res.status = KLFERR_PNGREADFAIL;
      res.errorstr = QObject::tr("Unable to read file %1!\n", "KLFBackend")
	.arg(tempfname+".png");
      return res;
    }
    res.pngdata = pngfile.readAll();
    pngfile.close();
    // res.pngdata is now set.
    res.result.loadFromData(res.pngdata, "PNG");

  }

  if (!settings.epstopdfexec.isEmpty()) {
    // if we have epstopdf functionality, then we'll take advantage of it to generate pdf:
    KLFBlockProcess proc;
    QStringList args;
    args << settings.epstopdfexec << (tempfname+"-good.eps") << ("--outfile="+NATIVESEPARATORS(tempfname+".pdf"));

    klf_debug_time_print("KLFBackend::getLatexFormula: about to epstopdf...\n");    
    bool r = proc.startProcess(args);
    klf_debug_time_print("KLFBackend::getLatexFormula: epstopdf returned.\n");    

    if ( ! r ) {
      res.status = KLFERR_NOEPSTOPDFPROG;
      res.errorstr = QObject::tr("Unable to start epstopdf!\n", "KLFBackend");
      return res;
    }
    if ( !proc.normalExit() ) {
      res.status = KLFERR_EPSTOPDFNONORMALEXIT;
      res.errorstr = QObject::tr("epstopdf died nastily!\n", "KLFBackend");
      return res;
    }
    if ( proc.exitStatus() != 0) {
      res.status = KLFERR_PROGERR_EPSTOPDF;
      res.errorstr = progErrorMsg("epstopdf", proc.exitStatus(), proc.readStderrString(), proc.readStdoutString());
      return res;
    }
    if (!QFile::exists(tempfname + ".pdf")) {
      res.status = KLFERR_NOPDFFILE;
      res.errorstr = QObject::tr("PDF file didn't appear after call to epstopdf!\n", "KLFBackend");
      return res;
    }

    // get and save PDF to memory
    QFile pdffile(tempfname+".pdf");
#ifdef KLFBACKEND_QT4
    r = pdffile.open(QIODevice::ReadOnly);
#else
    r = pdffile.open(IO_ReadOnly);
#endif
    if ( ! r ) {
      res.status = KLFERR_PDFREADFAIL;
      res.errorstr = QObject::tr("Unable to read file %1!\n", "KLFBackend")
	.arg(tempfname+".pdf");
      return res;
    }
    res.pdfdata = pdffile.readAll();

  }

  klf_debug_time_print("KLFBackend::getLatexFormula: end of function.\n");    

  return res;
}


void KLFBackend::cleanup(QString tempfname)
{
  if (QFile::exists(tempfname+".tex")) QFile::remove(tempfname+".tex");
  if (QFile::exists(tempfname+".dvi")) QFile::remove(tempfname+".dvi");
  if (QFile::exists(tempfname+".aux")) QFile::remove(tempfname+".aux");
  if (QFile::exists(tempfname+".log")) QFile::remove(tempfname+".log");
  if (QFile::exists(tempfname+".toc")) QFile::remove(tempfname+".toc");
  if (QFile::exists(tempfname+".eps")) QFile::remove(tempfname+".eps");
  if (QFile::exists(tempfname+"-good.eps")) QFile::remove(tempfname+"-good.eps");
  if (QFile::exists(tempfname+".png")) QFile::remove(tempfname+".png");
  if (QFile::exists(tempfname+".pdf")) QFile::remove(tempfname+".pdf");
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
    a.dpi == b.dpi;
}



#ifndef KLFBACKEND_QT4
#define suffix extension
#define WRITEONLY IO_WriteOnly
#define OPENSTDOUT WRITEONLY, stdout
#define qPrintable(x) (x).local8Bit().data()
#define trimmed stripWhiteSpace
#define toUpper upper
#define setFileName setName
#define error errorString
#define toLatin1 latin1
#define write writeBlock
#else
#define WRITEONLY QIODevice::WriteOnly
#define OPENSTDOUT stdout, WRITEONLY
#endif

bool KLFBackend::saveOutputToFile(const klfOutput& klfoutput, const QString& fileName, const QString& fmt)
{
  QString format = fmt;
  // determine format first
  if (format.isEmpty()) {
    QFileInfo fi(fileName);
    if ( ! fi.suffix().isEmpty() )
      format = fi.suffix();
    else
      format = "PNG";
  }
  format = format.trimmed().toUpper();
  // got format. choose output now and prepare write
  QFile fout;
  if (fileName.isEmpty() || fileName == "-") {
    if ( ! fout.open(OPENSTDOUT) ) {
      qWarning("%s", qPrintable(QObject::tr("Unable to open stderr for write! Error: %1\n",
					    "KLFBackend::saveOutputToFile")
				.arg(fout.error())));
      return false;
    }
  } else {
    fout.setFileName(fileName);
    if ( ! fout.open(WRITEONLY) ) {
      qWarning("%s", qPrintable(QObject::tr("Unable to write to file `%1'! Error: %2\n",
					    "KLFBackend::saveOutputToFile")
				.arg(fileName).arg(fout.error())));
      return false;
    }
  }
  // now choose correct data source and write to fout
  if (format == "PNG") {
    fout.write(klfoutput.pngdata);
  } else if (format == "EPS" || format == "PS") {
    fout.write(klfoutput.epsdata);
  } else if (format == "PDF") {
    if (klfoutput.pdfdata.isEmpty()) {
      qWarning("%s", qPrintable(QObject::tr("PDF format is not available!\n",
					    "KLFBackend::saveOutputToFile")));
      return false;
    }
    fout.write(klfoutput.pdfdata);
  } else {
    bool res = klfoutput.result.save(&fout, format.toLatin1());
    if ( ! res ) {
      qWarning("%s", qPrintable(QObject::tr("Unable to save image to file `%1' in format `%2'!\n",
					    "KLFBackend::saveOutputToFile")
				.arg(fileName).arg(format)));
      return false;
    }
  }

  return true;
}

// warning from here on: nasty macros are defined for the previous function.

