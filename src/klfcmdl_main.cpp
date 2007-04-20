/* klfcmdl_main.cpp for klatexformula_cmdl executable
 * Command-line tool for KLatexFormula */

#include <stdio.h>
#include <string.h>

#include <qcstring.h>
#include <qfile.h>
#include <qimage.h>
#include <qcolor.h>
#include <qtextstream.h>
#include <qfileinfo.h>

#include <kapplication.h>
#include <kaboutdata.h>
#include <kcmdlineargs.h>
#include <klocale.h>
#include <kstandarddirs.h>

#include <klfbackend.h>

static const char description[] =
    I18N_NOOP("KLatexFormula Command-line interface -- Easily get an image from a LaTeX equation");

static const char version[] = "2.0.0alpha5";


static KCmdLineOptions options[] =
{
  { "i", 0, 0 },
  { "input <file|->", I18N_NOOP( "An input file to parse, '-' for standard input." ), "-" },
  { "o", 0, 0 },
  { "output <file|->", I18N_NOOP( "A File to write to or '-' for standard output. Format is guessed by extention and defaults to PNG." ), "-" },
  { "f", 0, 0 },
  { "fgcolor <color>", I18N_NOOP( "A foreground color. Format #FFFFFF. (Don't forget to escape \\# from the shell!)" ), "#000000" },
  { "b", 0, 0 },
  { "bgcolor <color|->", I18N_NOOP( "A background color. Format #FFFFFF or '-' for transparent." ), "-" },
  { "X", 0, 0 },
  { "dpi <number>", I18N_NOOP( "DPI (dots per inch) pixelisation to use." ), "1200" },
  { "m", 0, 0 },
  { "mathmode <arg>", I18N_NOOP( "Use given math mode. <arg> must contain '...' to be replaced by the latex string." ), " \\[ ... \\] " },
  { "p", 0, 0 },
  { "preamble <arg>", I18N_NOOP( "Some arbitrary LaTeX code to be inserted after \\documentclass{} and before \\begin{document}. "
  "This may typically be \\usepackage{xyz} directives." ), "\\usepackage{amssymb,amsmath}" },

  { "q", 0, 0 },
  { "quiet", I18N_NOOP( "Print as little as possible on standard error. Error messages won't be printed. "
			"Errors will be reported with exit status." ), 0 },

  { "tempdir <dir>", I18N_NOOP( "A writable temporary directory. If empty, a suitable directory (like /tmp) will be used." ), "/tmp" },
  { "latex <path>", I18N_NOOP( "Path and file name to 'latex' executable. If 'latex' is installed normally and is accessible in $PATH, "
			       "you don't need to specify any path here." ), "latex" },
  { "dvips <path>", I18N_NOOP( "Path and file name to 'dvips' executable. If 'dvips' is installed normally and is accessible in $PATH, "
			       "you don't need to specify any path here." ), "dvips" },
  { "gs <path>", I18N_NOOP( "Path and file name to 'gs' executable. If 'gs' is installed normally and is accessible in $PATH, "
			    "you don't need to specify any path here." ), "gs" },
  { "epstopdf <path>", I18N_NOOP( "Path and file name to 'epstopdf' executable. If 'epstopdf' is installed normally and is accessible "
				  "in $PATH, you don't need to specify any path here. If this argument is set to empty, PDF will not "
				  "be available." ), "epstopdf" },
  { "lborderoffset <number>", I18N_NOOP( "A left border offset for image (Postscript points). Default value recommended." ), "1" },
  { "tborderoffset <number>", I18N_NOOP( "A top border offset for image (Postscript points). Default value recommended." ), "1" },
  { "rborderoffset <number>", I18N_NOOP( "A right border offset for image (Postscript points). Default value recommended." ), "3" },
  { "bborderoffset <number>", I18N_NOOP( "A bottom border offset for image (Postscript points). Default value recommended." ), "1" },

  KCmdLineLastOption
};



int main(int argc, char **argv)
{
  KAboutData about("klatexformula", I18N_NOOP("KLatexFormula Cmdline interface"), version, description,
		   KAboutData::License_GPL, "(C) 2005-2007 Philippe Faist", 0, 0, "philippe.faist@bluewin.ch");
  about.addAuthor( "Philippe Faist", 0, "philippe.faist@bluewin.ch" );
  KCmdLineArgs::init(argc, argv, &about);
  KCmdLineArgs::addCmdLineOptions( options );

  KApplication a(false, false); // non-GUI application

  KCmdLineArgs *args = KCmdLineArgs::parsedArgs();

  bool quiet = true;

  if ( ! args->isSet("quiet") ) {
    quiet = false;
    fprintf(stderr, "KLatexFormula Command-line interface Version %s by Philippe Faist (c) 2005-2007\n"
	    "Licensed under the terms of the GNU Public License GPL\n\n",
	    version);
  }

  QCString input = args->getOption("input");
  QCString output = args->getOption("output");
  KLFBackend::klfInput klfin;
  klfin.fg_color = QColor((const char*)args->getOption("fgcolor")).rgb();
  QCString bgcols = args->getOption("bgcolor");
  klfin.bg_color = !strcmp(bgcols, "-") ? qRgba(255, 255, 255, 0) : QColor((const char*)bgcols).rgb();
  klfin.dpi = args->getOption("dpi").toUInt();
  klfin.mathmode = args->getOption("mathmode");
  klfin.preamble = args->getOption("preamble");

  if (!strcmp(input, "-")) {
    // read stdin
    QString s;
    int c;
    while ( (c = getchar()) != EOF )
      s += c;
    klfin.latex = s;
  } else {
    QFile fin(input);
    fin.open(IO_ReadOnly);
    QTextStream stream(&fin);
    klfin.latex = stream.read(); // gulp everything
  }

  KLFBackend::klfSettings settings;
  settings.tempdir = args->getOption("tempdir");
  settings.klfclspath = QFileInfo(locate("appdata", "klatexformula.cls")).dirPath(true);
  settings.latexexec = args->getOption("latex");
  settings.dvipsexec = args->getOption("dvips");
  settings.gsexec = args->getOption("gs");
  settings.epstopdfexec = args->getOption("epstopdf");
  settings.lborderoffset = args->getOption("lborderoffset").toUInt();
  settings.tborderoffset = args->getOption("tborderoffset").toUInt();
  settings.rborderoffset = args->getOption("rborderoffset").toUInt();
  settings.bborderoffset = args->getOption("bborderoffset").toUInt();

  args->clear();


  KLFBackend::klfOutput klfout;

  klfout = KLFBackend::getLatexFormula(klfin, settings);

  if (klfout.status != 0) {
    // an error occurred:
    if (!quiet)
      fprintf(stderr, "Error: %d: %s\n", klfout.status, klfout.errorstr.ascii());
    return klfout.status;
  }

  // no error, all OK.
  // save to file or stdout
  if (!strcmp(output, "-")) {
    for (uint k = 0; k < klfout.pngdata.size(); ++k) {
      putchar(klfout.pngdata[k]);
    }
  } else {

    QString format;
    int l = output.findRev('.');
    if (l == -1)
      format = "PNG";
    else
      format = output.mid(l+1).upper();

    QByteArray *dataptr;
    if (format == "PS" || format == "EPS") {
      dataptr = &klfout.epsdata;
    } else if (format == "PDF") {
      dataptr = &klfout.pdfdata;
    } else if (format == "PNG") {
      dataptr = &klfout.pngdata;
    } else {
      dataptr = 0;
    }
    
    if (dataptr) {
      if (dataptr->size() == 0) {
	if (!quiet)
	  fprintf(stderr, "Sorry, format `%s' is not available.\n", format.ascii());
	return 101;
      }
      QFile fsav(output);
      fsav.open(IO_WriteOnly | IO_Raw);
      fsav.writeBlock(*dataptr);
    } else {
      klfout.result.save(output, format);
    }

  }
  
  return 0;
}
