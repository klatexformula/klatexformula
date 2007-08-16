/***************************************************************************
 *   file klfbackend.cpp
 *   This file is part of the KLatexFormula Project.
 *   Copyright (C) 2007 by Philippe Faist
 *   philippe.faist@bluewin.ch
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

#include <stdlib.h>

//#include <iostream>

#include <qregexp.h>
#include <qfile.h>
#include <qdatetime.h>
#include <qtextstream.h>

#include "klfblockprocess.h"
#include "klfbackend.h"

/** This may change in the future to provide translated error strings */
#define TRANSLATE(x) QString(x)


KLFBackend::KLFBackend()
{
}


// Utility function
QString progErrorMsg(QString progname, int exitstatus, QString stderr, QString stdout)
{
  if (stderr.isEmpty())
    return TRANSLATE("<p><b>%1</b> reported an error (exit status %2). Here is full stdout output:</p>\n"
		     "<pre>\n%3</pre>")
      .arg(progname).arg(exitstatus).arg(stdout);
  if (stdout.isEmpty())
    return TRANSLATE("<p><b>%1</b> reported an error (exit status %2). Here is full stderr output:</p>\n"
		     "<pre>\n%3</pre>")
      .arg(progname).arg(exitstatus).arg(stderr);

  return TRANSLATE("<p><b>%1</b> reported an error (exit status %2). Here is full stderr output:</p>\n"
		   "<pre>\n%3</pre><p>And here is full stdout output:</p><pre>\n%4</pre>")
    .arg(progname).arg(exitstatus).arg(stderr).arg(stdout);
}


KLFBackend::klfOutput KLFBackend::getLatexFormula(const klfInput& in, const klfSettings& settings)
{
  klfOutput res;
  res.status = 0;
  res.errorstr = QString();
  res.result = QImage();
  res.pngdata = QByteArray();
  res.epsdata = QByteArray();
  res.pdfdata = QByteArray();

  // PROCEDURE:
  // - generate LaTeX-file (documentclass klatexformula.cls)
  // - latex --> get DVI file
  // - dvips -E file.dvi it to get an EPS file
  // - Run gs:	gs -dNOPAUSE -dSAFER -dEPSCrop -r600 -dTextAlphaBits=4 -dGraphicsAlphaBits=4
  //              -sDEVICE=pngalpha|png16m -sOutputFile=xxxxxx.png -q -dBATCH xxxxxx.eps   to eventually get PNG data
  // - if epstopdfexec is not empty, run epstopdf and get PDF file.

  QString tempfname = settings.tempdir + "/klatexformulatmp2-"
    + QDateTime::currentDateTime().toString("hh-mm-ss");


#ifdef KLFBACKEND_QT4
  QString latexsimplified = in.latex.trimmed();
#else
  QString latexsimplified = in.latex.stripWhiteSpace();
#endif
  if (latexsimplified.isEmpty()) {
    res.errorstr = TRANSLATE("You must specify a LaTeX formula!");
    res.status = -1;
    return res;
  }

  QString latexin;
  if (in.mathmode.contains("...") == 0) {
    res.status = -2;
    res.errorstr = TRANSLATE("The math mode string doesn't contain '...'!");
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
      res.status = -3;
      res.errorstr = TRANSLATE("Can't open file for writing: '%1'!").arg(tempfname+".tex");
      return res;
    }
    QTextStream stream(&file);
    stream << "\\documentclass{klatexformula}\n"
	   << "\\usepackage[dvips]{color}\n"
	   << in.preamble << "\n"
	   << "\\begin{document}\n"
	   << QString("\\definecolor{klffgcolor}{rgb}{%1,%2,%3}\n").arg(qRed(in.fg_color)/255.0)
      .arg(qGreen(in.fg_color)/255.0).arg(qBlue(in.fg_color)/255.0)
	   << QString("\\definecolor{klfbgcolor}{rgb}{%1,%2,%3}\n").arg(qRed(in.bg_color)/255.0)
      .arg(qGreen(in.bg_color)/255.0).arg(qBlue(in.bg_color)/255.0)
	   << ( (qAlpha(in.bg_color)>0) ? "\\pagecolor{klfbgcolor}\n" : "" )
	   << "{\\color{klffgcolor} " << latexin << " }\n"
	   << "\\end{document}\n";
  }

  { // execute latex
    // we need klatexformula.cls, which is in path settings.klfclspath

    KLFBlockProcess proc;
    QStringList args;
    QStringList env;
    extern char **environ;

    proc.setWorkingDirectory(settings.tempdir);
    bool texinputsset = false;
    for (int i = 0; environ && environ[i] != NULL; ++i) {
      if (!strncmp(environ[i], "TEXINPUTS=", strlen("TEXINPUTS="))) {
	QString s = environ[i];
	s.insert(strlen("TEXINPUTS="), settings.klfclspath+":");
	texinputsset = true;
	env << s;
      } else {
	env << environ[i];
      }
    }
    if (!texinputsset) {
      env << "TEXINPUTS="+settings.klfclspath+":.:";
    }
    args << settings.latexexec << tempfname+".tex";

    /* printf("DEBUG: New environment:\n");
       for (uint jkl = 0; jkl < env.size(); ++jkl) {
       printf("DEBUG:\t%s\n", env[jkl].ascii());
       }
    */

    bool r = proc.startProcess(args, env);


    if (!r) {
      res.status = -4;
      res.errorstr = TRANSLATE("Unable to start Latex!");
      return res;
    }
    if (!proc.normalExit()) {
      res.status = -5;
      res.errorstr = TRANSLATE("Latex was killed!");
      return res;
    }
    if (proc.exitStatus() != 0) {
      cleanup(tempfname);
      res.status = 1;
      res.errorstr = progErrorMsg("LaTeX", proc.exitStatus(), proc.readStderrString(), proc.readStdoutString());
      return res;
    }

    if (!QFile::exists(tempfname + ".dvi")) {
      res.status = -6;
      res.errorstr = TRANSLATE("DVI file didn't appear after having called Latex!");
      return res;
    }

  } // end of 'latex' block

  { // execute dvips -E

    KLFBlockProcess proc;
    QStringList args;
    args << settings.dvipsexec << "-E" << (tempfname+".dvi") << "-o" << (tempfname+".eps");

    bool r = proc.startProcess(args);

    if ( ! r ) {
      res.status = -7;
      res.errorstr = TRANSLATE("Unable to start dvips!\n");
      return res;
    }
    if ( !proc.normalExit() ) {
      res.status = -8;
      res.errorstr = TRANSLATE("Dvips was mercilessly killed!\n");
      return res;
    }
    if ( proc.exitStatus() != 0) {
      res.status = 2;
      res.errorstr = progErrorMsg("dvips", proc.exitStatus(), proc.readStderrString(), proc.readStdoutString());
      return res;
    }
    if (!QFile::exists(tempfname + ".eps")) {
      res.status = -9;
      res.errorstr = TRANSLATE("EPS file didn't appear after dvips call!\n");
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
      res.status = -10;
      res.errorstr = TRANSLATE("Can't read file '%1'!\n").arg(tempfname+".eps");
      return res;
    }
    QByteArray epscontent = epsfile.readAll();
#ifdef KLFBACKEND_QT4
    int i = epscontent.indexOf("%%BoundingBox: ");
    if ( i == -1 ) {
      res.status = -11;
      res.errorstr = TRANSLATE("File '%1' does not contain line \"%%BoundingBox: ... \" !").arg(tempfname+".eps");
    }
    int ax, ay, bx, by;
    char temp[250];
    int k = i;
    i += strlen("%%BoundingBox:");
    int n = sscanf(epscontent.constData()+i, "%d %d %d %d", &ax, &ay, &bx, &by);
    if ( n != 4 ) {
      res.status = -12;
      res.errorstr = TRANSLATE("file %1: Line %%BoundingBox: can't read values!\n").arg(tempfname+".eps");
      return res;
    }
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
      res.status = -11;
      res.errorstr = TRANSLATE("File '%1' does not contain line \"%%BoundingBox: ... \" !").arg(tempfname+".eps");
      return res;
    }
    int ax, ay, bx, by;
    char temp[250];
    i += strlen("%%BoundingBox:");
    int n = sscanf((const char*)epscontent_s+i, "%d %d %d %d", &ax, &ay, &bx, &by);
    if ( n != 4 ) {
      res.status = -12;
      res.errorstr = TRANSLATE("file %1: Line %%BoundingBox: can't read values!\n").arg(tempfname+".eps");
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
      res.status = -13;
      res.errorstr = TRANSLATE("Can't write to file '%1'!\n").arg(tempfname+"-good.eps");
      return res;
    }
#ifdef KLFBACKEND_QT4
    epsgoodfile.write(res.epsdata);
#else
    epsgoodfile.writeBlock(res.epsdata);
#endif
    // res.epsdata is now set.

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
    args << "-sOutputFile="+tempfname+".png" << "-q" << "-dBATCH" << (tempfname+"-good.eps");

    bool r = proc.startProcess(args);
  
    if ( ! r ) {
      res.status = -7;
      res.errorstr = TRANSLATE("Unable to start gs!\n");
      return res;
    }
    if ( !proc.normalExit() ) {
      res.status = -8;
      res.errorstr = TRANSLATE("gs died abnormally!\n");
      return res;
    }
    if ( proc.exitStatus() != 0) {
      res.status = 3;
      res.errorstr = progErrorMsg("gs", proc.exitStatus(), proc.readStderrString(), proc.readStdoutString());
      return res;
    }
    if (!QFile::exists(tempfname + ".png")) {
      res.status = -9;
      res.errorstr = TRANSLATE("PNG file didn't appear after call to gs!\n");
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
      res.status = -10;
      res.errorstr = TRANSLATE("Unable to read file %1!\n").arg(tempfname+".png");
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
    args << settings.epstopdfexec << (tempfname+"-good.eps") << "-o" << (tempfname+".pdf");

    bool r = proc.startProcess(args);

    if ( ! r ) {
      res.status = -11;
      res.errorstr = TRANSLATE("Unable to start epstopdf!\n");
      return res;
    }
    if ( !proc.normalExit() ) {
      res.status = -12;
      res.errorstr = TRANSLATE("epstopdf died nastily!\n");
      return res;
    }
    if ( proc.exitStatus() != 0) {
      res.status = 4;
      res.errorstr = progErrorMsg("epstopdf", proc.exitStatus(), proc.readStderrString(), proc.readStdoutString());
      return res;
    }
    if (!QFile::exists(tempfname + ".pdf")) {
      res.status = -13;
      res.errorstr = TRANSLATE("PDF file didn't appear after call to epstopdf!\n");
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
      res.status = -14;
      res.errorstr = TRANSLATE("Unable to read file %1!\n").arg(tempfname+".pdf");
      return res;
    }
    res.pdfdata = pdffile.readAll();

  }

  // clean up our mess
  cleanup(tempfname);


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

