/***************************************************************************
 *   file cmdl_main.cpp
 *   This file is part of the KLatexFormula Project.
 *   Copyright (C) 2009 by Philippe Faist
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
/* $Id$ */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <getopt.h>

#include <QString>
#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QColor>

#include <klfbackend.h>


bool klf_option_quiet = false;

void klfprint(QString s)
{
  if (!klf_option_quiet)
    printf("%s", s.toLocal8Bit().constData());
}
void klfprinterr(QString s)
{
  if (!klf_option_quiet)
    fprintf(stderr, "%s", s.toLocal8Bit().constData());
}



enum {
  // if you change short options here, be sure to change the short option list below.
  OPT_INPUT = 'i',
  OPT_OUTPUT = 'o',
  OPT_FORMAT = 'F',
  OPT_FGCOLOR = 'f',
  OPT_BGCOLOR = 'b',
  OPT_DPI = 'X',
  OPT_MATHMODE = 'm',
  OPT_PREAMBLE = 'p',
  OPT_QUIET = 'q',

  OPT_HELP = 'h',
  OPT_VERSION = 'V',

  OPT_LBORDEROFFSET = 128,
  OPT_TBORDEROFFSET,
  OPT_RBORDEROFFSET,
  OPT_BBORDEROFFSET,
  OPT_TEMPDIR,
  OPT_LATEX,
  OPT_DVIPS,
  OPT_GS,
  OPT_EPSTOPDF,

};

static struct option klfcmdl_optlist[] = {
  { "input", 1, NULL, OPT_INPUT },
  { "output", 1, NULL, OPT_OUTPUT },
  { "format", 1, NULL, OPT_FORMAT },
  { "fgcolor", 1, NULL, OPT_FGCOLOR },
  { "bgcolor", 1, NULL, OPT_BGCOLOR },
  { "dpi", 1, NULL, OPT_DPI },
  { "mathmode", 1, NULL, OPT_MATHMODE },
  { "preamble", 1, NULL, OPT_PREAMBLE },
  { "quiet", 1, NULL, OPT_QUIET },
  // -----
  { "lborderoffset", 1, NULL, OPT_LBORDEROFFSET },
  { "tborderoffset", 1, NULL, OPT_TBORDEROFFSET },
  { "rborderoffset", 1, NULL, OPT_RBORDEROFFSET },
  { "bborderoffset", 1, NULL, OPT_BBORDEROFFSET },
  // -----
  { "tempdir", 1, NULL, OPT_TEMPDIR },
  { "latex", 1, NULL, OPT_LATEX },
  { "dvips", 1, NULL, OPT_DVIPS },
  { "gs", 1, NULL, OPT_GS },
  { "epstopdf", 1, NULL, OPT_EPSTOPDF },
  // -----
  { "help", 0, NULL, OPT_HELP },
  { "version", 0, NULL, OPT_VERSION },

  // ---- end of options
  {0, 0, 0, 0}
};
// list of short options
static char klfcmdl_optstring[] = "i:o:f:b:X:m:p:q:hV";
// help text
static struct { const char *source; const char *comment; }  klfopt_helptext =
  QT_TRANSLATE_NOOP3("QObject", "\n"
		     "KLatexFormula by Philippe Faist\n"
		     "Usage: klatexformula_cmdl [OPTIONS]\n"
		     "\n"
		     "Available Options:\n"
		     "  -i,--input <file>  ....\n"
		     " ...........\n"
		     "\n"
		     "Have a lot of fun!\n"
		     "\n", "klf_cmdl")
  ;
static QString klfopt_version = KLF_VERSION_STRING;


// --------------------------------------------------------------------

static int fake_argc = 1;
static char *fake_argv[2] = { (char*)"", NULL };

int main(int argc, char *argv[])
{
  QString f_input;
  QString f_output;
  QString format;
  KLFBackend::klfInput input;
  KLFBackend::klfSettings settings;
  KLFBackend::klfOutput output;

  // fill default values :
  input.latex = "";
  input.mathmode = "\\[ ... \\]";
  input.preamble = "";
  input.fg_color = qRgb(0,0,0);
  input.bg_color = qRgba(255,255,255,0);
  input.dpi = 1200;
  settings.tempdir = QDir::tempPath();
  settings.latexexec = "latex";
  settings.dvipsexec = "dvips";
  settings.gsexec = "gs";
  settings.epstopdfexec = "epstopdf";
  settings.lborderoffset = 1;
  settings.tborderoffset = 1;
  settings.rborderoffset = 1;
  settings.bborderoffset = 1;

  fake_argv[0] = strdup(argv[0]);
  QCoreApplication app(fake_argc, fake_argv); // needed before printing stuff for translations.
  // load possible translations... TODO

  // ---------------------------
  // argument processing
  // ---------------------------
  int c;
  QColor color; // temp color object
  for (;;) {
    
    c = getopt_long(argc, argv, klfcmdl_optstring, klfcmdl_optlist, NULL);
    if (c == -1)
      break;

    QString arg = QString::fromLocal8Bit(optarg);

    switch (c) {
    case OPT_INPUT:
      f_input = arg;
      break;
    case OPT_OUTPUT:
      f_output = arg;
      break;
    case OPT_FORMAT:
      format = arg;
      break;
    case OPT_FGCOLOR:
      color.setNamedColor(arg);
      input.fg_color = color.rgb();
      break;
    case OPT_BGCOLOR:
      if ( arg == "-" ) {
	// transparent
	input.bg_color = qRgba(255, 255, 255, 0); // white transparent
      } else {
	// solid color
	color.setNamedColor(arg);
	input.bg_color = color.rgb();
      }
      break;
    case OPT_DPI:
      input.dpi = arg.toInt();
      break;
    case OPT_MATHMODE:
      input.mathmode = arg;
      break;
    case OPT_PREAMBLE:
      input.preamble = arg;
      break;
    case OPT_QUIET:
      klf_option_quiet = true;
      break;
    case OPT_LBORDEROFFSET:
      settings.lborderoffset = arg.toInt();
      break;
    case OPT_TBORDEROFFSET:
      settings.tborderoffset = arg.toInt();
      break;
    case OPT_RBORDEROFFSET:
      settings.rborderoffset = arg.toInt();
      break;
    case OPT_BBORDEROFFSET:
      settings.bborderoffset = arg.toInt();
      break;
    case OPT_TEMPDIR:
      settings.tempdir = arg;
      break;
    case OPT_LATEX:
      settings.latexexec = arg;
      break;
    case OPT_DVIPS:
      settings.dvipsexec = arg;
      break;
    case OPT_GS:
      settings.gsexec = arg;
      break;
    case OPT_EPSTOPDF:
      settings.epstopdfexec = arg;
      break;
    case OPT_HELP:
      klfprinterr(QObject::tr(klfopt_helptext.source, klfopt_helptext.comment));
      exit(0);
    case OPT_VERSION:
      klfprinterr(QObject::tr("KLatexFormula: Version %1 using Qt %2\n", "klf_cmdl").arg(klfopt_version).arg(qVersion()));
      exit(0);
    default:
      klfprinterr(QObject::tr("ERROR: Unable to parse options (returned octal 0%1) !!??\n", "klf_cmdl").arg(c, 0, 8));
      exit(255);
    }
  }

  bool wants_read_from_stdin = false;
  QFile f;
  bool res;
  if ( ! f_input.isEmpty() ) {
    if (optind < argc) {
      // got unexpected input
      klfprinterr(QObject::tr("Got unexpected argument (-i was used) : %1\n", "klf_cmdl").arg(QString::fromLocal8Bit(argv[optind])));
      exit(255);
    }
    // read from the -i given file
    if (f_input == "-") {
      wants_read_from_stdin = true;
    } else {
      f.setFileName(f_input);
      res = f.open(QIODevice::ReadOnly);
      if ( ! res ) {
	klfprinterr(QObject::tr("Unable to open file `%1'!\n", "klf_cmdl").arg(f_input));
	exit(255);
      }
      QByteArray contents = f.readAll();
      // contents is assumed to be local 8 bit encoding.
      input.latex = QString::fromLocal8Bit(contents);
    }
  } else if (optind < argc) {
    // contents is taken from additional argument
    if (argc - optind > 1) {
      // got 2 extra arguments or more... error
      klfprinterr(QObject::tr("Got unexpected extra argument `%1'!\n").arg(argv[optind+1]));
      exit(255);
    }
    input.latex = QString::fromLocal8Bit(argv[optind]);
  } else {
    wants_read_from_stdin = true;
  }
  // if we want to read from stdin:
  if (wants_read_from_stdin) {
    // read from standard input...
    res = f.open(stdin, QIODevice::ReadOnly);
    if ( ! res ) {
      klfprinterr(QObject::tr("Unable to read on standard input!\n", "klf_cmdl").arg(f_input));
      exit(255);
    }
    QByteArray contents = f.readAll();
    // contents is assumed to be local 8 bit encoding.
    input.latex = QString::fromLocal8Bit(contents);
  }


  // ----------------------------------------------------------------------
  // NOW WE HAVE COLLECTED ALL THE INFORMATION NEEDED TO RUN KLFBACKEND
  // ----------------------------------------------------------------------

  output = KLFBackend::getLatexFormula(input, settings);

  if (output.status != 0) {
    // an error occurred
    klfprinterr(output.errorstr);
    exit(output.status);
  }

  // -----------------------------
  // do something with result
  // -----------------------------

  // determine format first
  if (format.isEmpty()) {
    QFileInfo fi(f_output);
    if (!fi.suffix().isEmpty())
      format = fi.suffix();
    else
      format = "PNG";
  }
  format = format.trimmed().toUpper();
  // got format. choose output now and prepare write
  QFile fout;
  if (f_output.isEmpty() || f_output == "-") {
    if ( ! fout.open(stdout, QIODevice::WriteOnly) ) {
      klfprinterr(QObject::tr("Unable to open stderr for write! Error: %1\n").arg(fout.error()));
      exit(255);
    }
  } else {
    fout.setFileName(f_output);
    if ( ! fout.open(QIODevice::WriteOnly) ) {
      klfprinterr(QObject::tr("Unable to write to file `%1'! Error: %2\n").arg(f_input).arg(fout.error()));
      exit(255);
    }
  }
  // now choose correct data source and write to fout
  if (format == "PNG") {
    fout.write(output.pngdata);
  } else if (format == "EPS") {
    fout.write(output.epsdata);
  } else if (format == "PDF") {
    if (output.pdfdata.isEmpty()) {
      klfprinterr(QObject::tr("PDF format is not available!\n"));
      exit(255);
    }
    fout.write(output.pdfdata);
  } else {
    bool res = output.result.save(&fout, format.toLatin1());
    if ( ! res ) {
      klfprinterr(QObject::tr("Unable to save image to file! (is format `%1' supported?)\n").arg(format));
      exit(255);
    }
  }

  return 0;
}
