/***************************************************************************
 *   file main.cpp
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
/* $Id$ */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <getopt.h>

#include <signal.h>

#include <QApplication>
#include <QDebug>
#include <QTranslator>
#include <QFileInfo>
#include <QDir>
#include <QResource>
#include <QProcess>
#include <QPluginLoader>
#include <QMessageBox>

#include <klfbackend.h>

#include "klfmain.h"
#include "klfconfig.h"
#include "klfmainwin.h"
#include "klfdbus.h"
#include "klfpluginiface.h"

// Program Exit Error Codes
#define EXIT_ERR_FILEINPUT 100
#define EXIT_ERR_FILESAVE 101
#define EXIT_ERR_OPT 102


// COMMAND-LINE-OPTION SPECIFIC DEFINITIONS
// using getopt.h library (specifically getopt_long())

// flags
int opt_interactive = -1; // -1: not specified, 0: NOT interactive, 1: interactive
char *opt_input = NULL;
char *opt_latexinput = NULL;
char *opt_output = NULL;
char *opt_format = NULL;
char *opt_fgcolor = NULL;
char *opt_bgcolor = NULL;
int opt_dpi = -1;
char *opt_mathmode = NULL;
char *opt_preamble = NULL;
bool opt_quiet = false;

int opt_lborderoffset = -1;
int opt_tborderoffset = -1;
int opt_rborderoffset = -1;
int opt_bborderoffset = -1;

char *opt_tempdir;
char *opt_latex;
char *opt_dvips;
char *opt_gs;
char *opt_epstopdf;

bool opt_help_requested = false;
bool opt_version_requested = false;

char **klf_args;

int qt_argc;
char *qt_argv[1024];

// We will occasionally need to strdup some strings to keep persistent copies. Save these copies
// in this array, so that we can free() them in main_exit().
char *opt_strdup_free_list[64] = { NULL };
int opt_strdup_free_list_n = 0;


static struct { bool has_error; int retcode; } opt_error;

// option identifiers
enum {
  // if you change short options here, be sure to change the short option list below.
  OPT_INTERACTIVE = 'I',
  OPT_INPUT = 'i',
  OPT_LATEXINPUT = 'l',
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

  OPT_QTOPT = 'Q',

  OPT_LBORDEROFFSET = 128,
  OPT_TBORDEROFFSET,
  OPT_RBORDEROFFSET,
  OPT_BBORDEROFFSET,
  OPT_TEMPDIR,
  OPT_LATEX,
  OPT_DVIPS,
  OPT_GS,
  OPT_EPSTOPDF

};

static struct option klfcmdl_optlist[] = {
  { "interactive", 0, NULL, OPT_INTERACTIVE },
  { "input", 1, NULL, OPT_INPUT },
  { "latexinput", 1, NULL, OPT_LATEXINPUT },
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
  // -----
  { "qtoption", 1, NULL, OPT_QTOPT },
  // ---- end of option list ----
  {0, 0, 0, 0}
};
// list of short options
static char klfcmdl_optstring[] = "Ii:l:o:F:f:b:X:m:p:qhVQ";
// help text
static struct { const char *source; const char *comment; }  klfopt_helptext =
  QT_TRANSLATE_NOOP3(// context
		     "QObject",
		     // source text
		     "\n"
		     "KLatexFormula by Philippe Faist\n"
		     "\n"
		     "Usage: klatexformula\n"
		     "           Opens klatexformula Graphical User Interface (GUI)\n"
		     "       klatexformula [OPTIONS]\n"
		     "           Performs actions required by [OPTIONS], and exits\n"
		     "       klatexformula --interactive|-I [OPTIONS]\n"
		     "           Opens the GUI and performs actions required by [OPTIONS]\n"
		     "       klatexformula filename1.klf [ filename2.klf ... ]\n"
		     "           Opens the GUI and imports the given .klf file(s) into the library.\n"
		     "\n"
		     "       If additional filename arguments are passed to the command line, they are\n"
		     "       interpreted as .klf files to load into the library in separate resources\n"
		     "       (only in interactive mode).\n"
		     "\n"
		     "OPTIONS may be one or many of:\n"
		     "  -I|--interactive\n"
		     "      Runs KLatexFormula in interactive mode with a full-featured graphical user\n"
		     "      interface. This option is on by default, except if --input or --latexinput\n"
		     "      is given.\n"
		     "  -i|--input <file|->\n"
		     "      Specifies a file to read latex input from.\n"
		     "  -l|--latexinput <expr>\n"
		     "      Specifies the LaTeX code of an equation to render.\n"
		     "  -o|--output <file|->\n"
		     "      Specifies to write the output image (obtained from equation given by --input\n"
		     "      or --latexinput) to <file> or standard output.\n"
		     "  -F|--format <format>\n"
		     "      Specifies the format the output should be written in. By default, the format\n"
		     "      is guessed from file name extention and defaults to PNG.\n"
		     "  -f|--fgcolor <'#xxxxxx'>\n"
		     "      Specifies a color (in web #RRGGBB hex format) to use for foreground color.\n"
		     "      Don't forget to escape the '#' to prevent the shell from interpreting it as\n"
		     "      a comment.\n"
		     "  -b|--bgcolor <-|'#xxxxxx'>\n"
		     "      Specifies a color (in web #RRGGBB hex format, or '-' for transparent) to\n"
		     "      use as background color (defaults to transparent)\n"
		     "  -X|--dpi <N>\n"
		     "      Use N dots per inch (DPI) when converting latex output to image. Defaults to\n"
		     "      1200 (high-resolution image).\n"
		     "  -m|--mathmode <expression containing '...'>\n"
		     "      Specifies which LaTeX math mode to use, if any. The argument to this option\n"
		     "      is any string containing \"...\", which will be replaced by the equation\n"
		     "      itself. Defaults to \"\\[ ... \\]\"\n"
		     "  -p|--preamble <LaTeX code>\n"
		     "      Any LaTeX code that will be inserted before \\begin{document}. Useful for\n"
		     "      including custom packages with \\usepackage{...}.\n"
		     "  -q|--quiet\n"
		     "      Disable console output of warnings and errors.\n"
		     "\n"
		     "  --lborderoffset <N>\n"
		     "  --tborderoffset <N>\n"
		     "  --rborderoffset <N>\n"
		     "  --bborderoffset <N>\n"
		     "      Include a margin of N postscript points on left, top, right, or bottom margin\n"
		     "      respectively.\n"
		     "  --tempdir </path/to/temp/dir>\n"
		     "      Specify the directory in which KLatexFormula will write temporary files.\n"
		     "      Defaults to a system-specific temporary directory like \"/tmp/\".\n"
		     "  --latex <latex executable>\n"
		     "  --dvips <dvips executable>\n"
		     "  --gs <gs executable>\n"
		     "  --epstopdf <epstopdf executable>\n"
		     "      Specifiy the executable for latex, dvips, gs or epstopdf. By default, they\n"
		     "      are searched for in $PATH and/or in common system directories.\n"
		     "\n"
		     "  -h|--help\n"
		     "      Display this help text and exit.\n"
		     "  -V|--version\n"
		     "      Display KLatexFormula version information and exit.\n"
		     "\n"
		     "  -Q|--qtoption <qt-option>\n"
		     "      Specify a Qt-Specific option. For example, to launch KLatexFormula in\n"
		     "      Plastique GUI style, use:\n"
		     "        klatexformula  --qtoption='-style=Plastique'\n"
		     "      Note that if <qt-option> begins with a '-', then it must be appended to the\n"
		     "      long '--qtoption=' syntax with the equal sign.\n"
		     "\n"
		     "\n"
		     "NOTES\n"
		     "  * When run in interactive mode, the newly evaluated equation is appended to\n"
		     "    KLatexFormula's history.\n"
		     "  * When not run in interactive mode, no X11 server is needed.\n"
		     "  * Additional translation files and/or data can be provided to klatexformula by specifying\n"
		     "    a list of Qt rcc files or directories containing such files to import in the KLF_RESOURCES\n"
		     "    environment variable. Separate the file names with ':' on unix/mac, or ';' on windows. The\n"
		     "    default paths can be included with an empty section ('::') or leading or trailing ':'.\n"
		     "  * Please report any bugs and malfunctions to the author.\n"
		     "\n"
		     "Have a lot of fun!\n"
		     "\n",
		     // comment
		     "Command-line help instructions")
  ;


// EXTERNAL INTERACTION STUFF:

#define KLF_RESOURCES_ENVNAM "KLF_RESOURCES"
#if defined(Q_OS_WIN32) || defined(Q_OS_WIN64)
const char * PATH_ENVVAR_SEP =  ";";
const char * klfresources_default_rel = "/rccresources/";
#define KLF_DLL_EXT "*.dll"
#else
# if defined(Q_WS_MAC)
#define KLF_DLL_EXT "*.dylib"
const char * PATH_ENVVAR_SEP =  ":";
const char * klfresources_default_rel = "/../Resources/rccresources/";
# else // unix-like system
#define KLF_DLL_EXT "*.so"
const char * PATH_ENVVAR_SEP = ":";
const char * klfresources_default_rel = "/../share/klatexformula/rccresources/";
# endif
#endif


// TRAP SIGINT SIGNAL AND EXIT GRACEFULLY

void signal_act(int sig)
{
  if (sig == SIGINT) {
    fprintf(stderr, "Interrupt\n");
    if (qApp != NULL) {
      qApp->quit();
    } else {
      ::exit(128);
    }
  }
}


// DEBUG, WARNING AND FATAL MESSAGES HANDLER

// in the future I may add some option to redirect debug output to file...
static FILE *klf_qt_msg_fp = NULL;

void klf_qt_message(QtMsgType type, const char *msg)
{
  if (opt_quiet)
    return;

  FILE *fout = stderr;
  if (klf_qt_msg_fp != NULL)  fout = klf_qt_msg_fp;

  switch (type) {
  case QtDebugMsg:
    fprintf(fout, "Debug: %s\n", msg);
    break;
  case QtWarningMsg:
    fprintf(fout, "Warning: %s\n", msg);
    break;
  case QtCriticalMsg:
    fprintf(fout, "Critical: %s\n", msg);
    break;
  case QtFatalMsg:
    fprintf(fout, "Fatal: %s\n", msg);
    abort();
  default:
    fprintf(fout, "?????: %s\n", msg);
    break;
  }
}





// UTILITY FUNCTIONS


void main_parse_options(int argc, char *argv[]);

/** Free some memory we have persistently allocated */
void main_cleanup()
{
  // free strdup()'ed strings
  while (--opt_strdup_free_list_n >= 0)
    free(opt_strdup_free_list[opt_strdup_free_list_n]);
}

/** Perform clean-up and ::exit() */
void main_exit(int code)
{
  main_cleanup();
  exit(code);
}

/** Determine from where to get input (direct input, from file, stdin) and read latex code;
 * return the latex code as QString */
QString main_get_input(char *input, char *latexinput)
{
  if (latexinput != NULL && strlen(latexinput) != 0) {
    if (input != NULL && strlen(input) != 0) {
      if ( ! opt_quiet ) {
	fprintf(stderr, "%s", QObject::tr("Warning: Ignoring --input when --latexinput is given.\n")
		.toLocal8Bit().constData());
      }
    }
    return QString::fromLocal8Bit(latexinput);
  }
  if (input != NULL && strlen(input) != 0) {
    QString fname = QString::fromLocal8Bit(input);
    QFile f;
    if ( fname == "-" ) {
      if ( ! f.open(stdin, QIODevice::ReadOnly) ) {
	if ( ! opt_quiet ) {
	  fprintf(stderr, "%s", QObject::tr("Error: Can't read standard input (!)\n")
		  .toLocal8Bit().constData());
	}
	main_exit(EXIT_ERR_FILEINPUT);
      }
    } else {
      f.setFileName(fname);
      if ( ! f.open(QIODevice::ReadOnly) ) {
	if ( ! opt_quiet ) {
	  fprintf(stderr, "%s", QObject::tr("Error: Can't read input file `%1'.\n").arg(fname)
		  .toLocal8Bit().constData());
	}
	main_exit(EXIT_ERR_FILEINPUT);
      }
    }
    // now file is opened properly.
    QByteArray contents = f.readAll();
    // contents is assumed to be local 8 bit encoding.
    return QString::fromLocal8Bit(contents);
  }
  // neither input nor latexinput have any contents
  return QString::null;
}

/** Saves a klfbackend result (KLFBackend::klfOutput) to a file or stdout with given format.
 * format is guessed if not provided, and defaults to PNG. */
void main_save(KLFBackend::klfOutput klfoutput, const QString& f_output, QString format)
{
  KLFBackend::saveOutputToFile(klfoutput, f_output, format);
}

void main_load_extra_resources()
{
  // this function is called with running Q[Core]Application and klfconfig all set up.

  QStringList env = QProcess::systemEnvironment();
  QRegExp rgx("^" KLF_RESOURCES_ENVNAM "=");
  QStringList klf_resources_l = env.filter(rgx);
  QString klf_resources = QString::null;
  if (klf_resources_l.size() > 0) {
    klf_resources = klf_resources_l[0].replace(rgx, "");
  }

  bool klfsettings_can_import = false;

  // Find global system-wide klatexformula rccresources dir
  QString defaultrccpath = QCoreApplication::applicationDirPath() + klfresources_default_rel
    + PATH_ENVVAR_SEP + klfconfig.homeConfigDirRCCResources;
  QString rccfilepath;
  if ( klf_resources.isNull() ) {
    rccfilepath = "";
  } else {
    rccfilepath = klf_resources;
  }
  //  printf("DEBUG: Rcc file list is \"%s\"\n", rccfilepath.toLocal8Bit().constData());
  QStringList defaultsplitrccpath = defaultrccpath.split(PATH_ENVVAR_SEP, QString::SkipEmptyParts);
  QStringList rccfiles = rccfilepath.split(PATH_ENVVAR_SEP, QString::KeepEmptyParts);
  int j, k;
  for (QStringList::iterator it = rccfiles.begin(); it != rccfiles.end(); ++it) {
    if ((*it).isEmpty()) {
      // empty split section: meaning that we want default paths at this point
      it = rccfiles.erase(it, it+1);
      for (j = 0; j < defaultsplitrccpath.size(); ++j) {
	it = rccfiles.insert(it, defaultsplitrccpath[j]) + 1;
      }
      // having the default paths added, it is safe for klfsettings to import add-ons to ~/.klf.../rccresources/
      klfsettings_can_import = true;
      --it; // we already point to the next entry, compensate the ++it in for
    }
  }
  QStringList rccfilesToLoad;
  for (j = 0; j < rccfiles.size(); ++j) {
    QFileInfo fi(rccfiles[j]);
    if (fi.isDir()) {
      QDir dir(rccfiles[j]);
      QFileInfoList files = dir.entryInfoList(QStringList()<<"*.rcc", QDir::Files);
      for (k = 0; k < files.size(); ++k) {
	rccfilesToLoad << files[k].absoluteFilePath();
      }
    } else if (fi.isFile() && fi.suffix() == "rcc") {
      rccfilesToLoad << fi.absoluteFilePath();
    }
  }
  for (j = 0; j < rccfilesToLoad.size(); ++j) {
    KLFAddOnInfo addoninfo(rccfilesToLoad[j]);
    bool res = QResource::registerResource(addoninfo.fpath);
    if ( res ) {
      // resource registered.
      klf_addons.append(addoninfo);
    } else {
      if ( ! opt_quiet )
	fprintf(stderr, "Failed to register resource `%s'.\n", rccfiles[j].toLocal8Bit().constData());
    }
  }

  // set the global "can-import" flag
  klf_addons_canimport = klfsettings_can_import;
}

struct KLFI18nFile
{
  QString fpath;
  QString name;
  QString locale;
  // how specific the locale is (e.g. ""->0 , "fr"->1, "fr_CH"->2 )
  int locale_specificity;

  KLFI18nFile(QDir d, QString fn) {
    int firstunderscore = fn.indexOf('_');
    int endbasename = fn.endsWith(".qm") ? fn.length() - 3 : fn.length() ;
    if (firstunderscore == -1)
      firstunderscore = endbasename; // no locale part if no underscore
    // ---
    fpath = d.absoluteFilePath(fn);
    name = fn.mid(0, firstunderscore);
    locale = fn.mid(firstunderscore+1, endbasename-(firstunderscore+1));
    locale_specificity = (locale.split('_', QString::SkipEmptyParts)).size() ;
  }
};

void main_load_translations(QCoreApplication *app)
{
  // load all translations. Translations are files found in the form
  //   :/i18n/<name>_<locale>.qm   or   homeconfig/i18n/<name>_<locale>.qm
  //

  // we will find all possible .qm files and store them in this structure for easy access
  // structure is indexed by name, then locale specificity
  QMap<QString, QMap<int, QList<KLFI18nFile> > > i18nFiles;
  // a list of names. this is redundant for  i18nFiles.keys()
  QSet<QString> names;

  QStringList i18ndirlist = QStringList() << ":/i18n" << klfconfig.homeConfigDirI18n ;
  int j, k;
  for (j = 0; j < i18ndirlist.size(); ++j) {
    // explore this directory; we expect a list of *.qm files
    QDir i18ndir(i18ndirlist[j]);
    QStringList files = i18ndir.entryList(QStringList() << QString::fromLatin1("*.qm"), QDir::Files);
    for (k = 0; k < files.size(); ++k) {
      KLFI18nFile i18nfile(i18ndir, files[k]);
      //      qDebug("Found i18n file %s (name=%s,locale=%s,lc-spcif.=%d)", qPrintable(i18nfile.fpath),
      //	     qPrintable(i18nfile.name), qPrintable(i18nfile.locale), i18nfile.locale_specificity);
      i18nFiles[i18nfile.name][i18nfile.locale_specificity] << i18nfile;
      names << i18nfile.name;
    }
  }

  // get locale
  QString lc = QLocale::system().name();
  QStringList lcparts = lc.split("_");

  //  qDebug("Required locale is %s", qPrintable(lc));

  // a list of translation files to load (absolute paths)
  QStringList translationsToLoad;

  // now, load a suitable translator for each encountered name.
  for (QSet<QString>::const_iterator it = names.begin(); it != names.end(); ++it) {
    QString name = *it;
    QMap< int, QList<KLFI18nFile> > translations = i18nFiles[name];
    int specificity = lcparts.size();  // start with maximum specificity for required locale
    while (specificity >= 0) {
      // try to load a translation matching this specificity and locale
      QString testlocale = QStringList(lcparts.mid(0, specificity)).join("_");
      //      qDebug("Testing locale string %s...", qPrintable(testlocale));
      // search list:
      QList<KLFI18nFile> list = translations[specificity];
      for (j = 0; j < list.size(); ++j) {
	if (list[j].locale == testlocale) {
	  //	  qDebug("Found translation file.");
	  // got matching translation file ! Load it !
	  translationsToLoad << list[j].fpath;
	  // and stop searching translation files for this name (break while condition);
	  specificity = -1;
	}
      }
      // If we didn't find a suitable translation, try less specific locale name
      specificity--;
    }
  }
  // now we have a full list of translation files to load stored in  translationsToLoad .

  // Load Translations:
  for (j = 0; j < translationsToLoad.size(); ++j) {
    // load this translator
    qDebug("Loading translator %s for %s", qPrintable(translationsToLoad[j]), qPrintable(lc));
    QTranslator *translator = new QTranslator(app);
    QFileInfo fi(translationsToLoad[j]);
    //    qDebug("translator->load(\"%s\", \"%s\", \"_\", \"%s\")", qPrintable(fi.completeBaseName()),
    //	   qPrintable(fi.absolutePath()),  qPrintable("."+fi.suffix()));
    bool res = translator->load(fi.completeBaseName(), fi.absolutePath(), "_", "."+fi.suffix());
    if ( res ) {
      app->installTranslator(translator);
    } else {
      qWarning("Failed to load translator %s.\n", qPrintable(translationsToLoad[j]));
    }
  }
}

void main_load_plugins(QApplication *app, KLFMainWin *mainWin)
{
  // first step: copy all resource-located plugin libraries to our local config
  // directory because we can only load filesystem-located plugins.
  QDir resplugdir(":/plugins");
  QStringList resplugins = resplugdir.entryList(QStringList() << KLF_DLL_EXT, QDir::Files);
  int k;
  for (k = 0; k < resplugins.size(); ++k) {
    QString resfn = resplugdir.absoluteFilePath(resplugins[k]);
    QString locfn = klfconfig.homeConfigDirPlugins + "/" + resplugins[k];
    if ( ! QFile::exists( locfn ) ) {
      // copy plugin to local plugin dir
      bool res = QFile::copy( resfn , locfn );
      if ( ! res ) {
	qWarning("Unable to copy plugin to local directory!");
      } else {
	QFile::setPermissions(locfn, QFile::ReadOwner|QFile::WriteOwner|QFile::ExeOwner|
			      QFile::ReadUser|QFile::WriteUser|QFile::ExeUser|
			      QFile::ReadGroup|QFile::ExeGroup|QFile::ReadOther|QFile::ExeOther);
	qDebug("Copied plugin %s to local directory %s.", qPrintable(resfn), qPrintable(locfn));
      }
    }
  }

  int i, j;
  QStringList pluginsdirs;
  pluginsdirs << klfconfig.homeConfigDirPlugins ;
  for (i = 0; i < pluginsdirs.size(); ++i) {
    if ( ! QFileInfo(pluginsdirs[i]).isDir() )
      continue;

    QDir thisplugdir(pluginsdirs[i]);
    QStringList plugins = thisplugdir.entryList(QStringList() << KLF_DLL_EXT, QDir::Files);
    KLFPluginGenericInterface * pluginInstance;
    for (j = 0; j < plugins.size(); ++j) {
      QString pluginpath = thisplugdir.absoluteFilePath(plugins[j]);
      QPluginLoader pluginLoader(pluginpath, app);
      QObject *pluginInstObject = pluginLoader.instance();
      if (pluginInstObject) {
	pluginInstance = qobject_cast<KLFPluginGenericInterface *>(pluginInstObject);
	if (pluginInstance) {
	  // plugin file successfully loaded.
	  QString nm = pluginInstance->pluginName();
	  qDebug("Successfully loaded plugin library %s (%s)", qPrintable(nm),
		 qPrintable(pluginInstance->pluginDescription()));

	  if ( ! klfconfig.Plugins.pluginConfig.contains(nm) ) {
	    // create default plugin configuration if non-existant
	    klfconfig.Plugins.pluginConfig[nm] = QMap<QString, QVariant>();
	    // ask plugin whether it's supposed to be loaded by default
	    klfconfig.Plugins.pluginConfig[nm]["__loadenabled"] = pluginInstance->pluginDefaultLoadEnable();
	  }

	  KLFPluginInfo pluginInfo;
	  pluginInfo.name = nm;
	  pluginInfo.title = pluginInstance->pluginTitle();
	  pluginInfo.description = pluginInstance->pluginDescription();
	  pluginInfo.author = pluginInstance->pluginAuthor();
	  pluginInfo.fpath = pluginpath;
	  pluginInfo.instance = NULL;

	  // if we are configured to load this plugin, load it.
	  if ( klfconfig.Plugins.pluginConfig[nm]["__loadenabled"].toBool() ) {
	    KLFPluginConfigAccess * c
	      = new KLFPluginConfigAccess(klfconfig.getPluginConfigAccess(nm, KLFPluginConfigAccess::ReadWrite));
	    pluginInstance->initialize(app, mainWin, c);
	    pluginInfo.instance = pluginInstance;
	    qDebug("\tPlugin %s loaded.", qPrintable(nm));
	  } else {
	    // if we aren't configured to load it, then discard it, but keep info with NULL instance,
	    // so that user can configure to load or not this plugin in the settings dialog.
	    pluginInfo.instance = NULL;
	    delete pluginInstance;
	    qDebug("\tPlugin %s NOT loaded.", qPrintable(nm));
	  }

	  klf_plugins.push_back(pluginInfo);
	}
      } else {
	QMessageBox::warning(0, "", QString("Plugin load error: %1\n").arg(pluginLoader.errorString()));
      }
    }
  }
}




// OUR MAIN FUNCTION

int main(int argc, char **argv)
{
  // first thing : setup version_maj/min/release correctly
  sscanf(version, "%d.%d.%d", &version_maj, &version_min, &version_release);

  qInstallMsgHandler(klf_qt_message);

  // signal acting -- catch SIGINT to exit gracefully
  signal(SIGINT, signal_act);

  // parse command-line options
  main_parse_options(argc, argv);

  // error handling
  if (opt_error.has_error) {
    fprintf(stderr, "Error while parsing command-line arguments.\n");
    main_exit(EXIT_ERR_OPT);
  }

  if ( opt_interactive ) {
    QApplication app(qt_argc, qt_argv);

    qRegisterMetaType< QImage >("QImage");

#if defined(KLF_USE_DBUS)
    // see if an instance of KLatexFormula is running...
    KLFDBusAppInterface *iface
      = new KLFDBusAppInterface("org.klatexformula.KLatexFormula", "/MainApplication",
				QDBusConnection::sessionBus(), &app);
    if (iface->isValid()) {
      iface->raiseWindow();
      // load everything via DBus
      QString latex = main_get_input(opt_input, opt_latexinput);
      if ( ! latex.isNull() )
	iface->setInputData("latex", latex);
      if ( opt_fgcolor != NULL )
	iface->setInputData("fgcolor", opt_fgcolor);
      if ( opt_bgcolor != NULL )
	iface->setInputData("bgcolor", opt_bgcolor);
      if ( opt_dpi > 0 )
	iface->setInputData("dpi", QString::null, opt_dpi);
      if (opt_mathmode != NULL)
	iface->setInputData("mathmode", QString::fromLocal8Bit(opt_mathmode));
      if (opt_preamble != NULL)
	iface->setInputData("preamble", QString::fromLocal8Bit(opt_preamble));
      if (opt_lborderoffset != -1)
	iface->setAlterSetting_i(KLFMainWin::altersetting_LBorderOffset, opt_lborderoffset);
      if (opt_tborderoffset != -1)
	iface->setAlterSetting_i(KLFMainWin::altersetting_TBorderOffset, opt_tborderoffset);
      if (opt_rborderoffset != -1)
	iface->setAlterSetting_i(KLFMainWin::altersetting_RBorderOffset, opt_rborderoffset);
      if (opt_bborderoffset != -1)
	iface->setAlterSetting_i(KLFMainWin::altersetting_BBorderOffset, opt_bborderoffset);
      if (opt_tempdir != NULL)
	iface->setAlterSetting_s(KLFMainWin::altersetting_TempDir, QString::fromLocal8Bit(opt_tempdir));
      if (opt_latex != NULL)
	iface->setAlterSetting_s(KLFMainWin::altersetting_Latex, QString::fromLocal8Bit(opt_latex));
      if (opt_dvips != NULL)
	iface->setAlterSetting_s(KLFMainWin::altersetting_Dvips, QString::fromLocal8Bit(opt_dvips));
      if (opt_gs != NULL)
	iface->setAlterSetting_s(KLFMainWin::altersetting_Gs, QString::fromLocal8Bit(opt_gs));
      if (opt_epstopdf != NULL)
	iface->setAlterSetting_s(KLFMainWin::altersetting_Epstopdf, QString::fromLocal8Bit(opt_epstopdf));
      // will actually save only if output is non empty.
      iface->evaluateAndSave(QString::fromLocal8Bit(opt_output), QString::fromLocal8Bit(opt_format));
      // and import KLF files if wanted
      QStringList flist;
      for (int k = 0; klf_args[k] != NULL; ++k)
	flist << QString::fromLocal8Bit(klf_args[k]);
      iface->importCmdlKLFFiles(flist);
      main_cleanup();
      return 0;
    }
#endif

    if ( ! opt_quiet )
      fprintf(stderr, "KLatexFormula Version %s by Philippe Faist (c) 2005-2009\n"
	      "Licensed under the terms of the GNU Public License GPL\n\n",
	      version);
  
    // now load default config
    klfconfig.loadDefaults(); // must be called before 'readFromConfig'
    klfconfig.readFromConfig();

    main_load_extra_resources();

    main_load_translations(&app);

    app.setFont(klfconfig.UI.applicationFont);

    KLFColorChooser::setColorList(klfconfig.UI.userColorList);

    KLFColorChooseWidget::setRecentCustomColors(klfconfig.UI.colorChooseWidgetRecent,
						klfconfig.UI.colorChooseWidgetCustom);

    KLFMainWin mainWin;
    mainWin.show();

    main_load_plugins(&app, &mainWin);

#if defined(KLF_USE_DBUS)
    new KLFDBusAppAdaptor(&app, &mainWin);
    QDBusConnection dbusconn = QDBusConnection::sessionBus();
    dbusconn.registerService("org.klatexformula.KLatexFormula");
    dbusconn.registerObject("/MainApplication", &app);
    dbusconn.registerObject("/MainWindow/KLFMainWin", &mainWin, QDBusConnection::ExportAllContents
			    | QDBusConnection::ExportChildObjects);
#endif

    // parse command-line given actions

    QString latex = main_get_input(opt_input, opt_latexinput);
    if ( ! latex.isNull() )
      mainWin.slotSetLatex(latex);

    if ( opt_fgcolor != NULL ) {
      mainWin.slotSetFgColor(QString::fromLocal8Bit(opt_fgcolor));
    }
    if ( opt_bgcolor != NULL ) {
      mainWin.slotSetBgColor(QString::fromLocal8Bit(opt_bgcolor));
    }
    if ( opt_dpi > 0 ) {
      mainWin.slotSetDPI(opt_dpi);
    }
    if (opt_mathmode != NULL) {
      mainWin.slotSetMathMode(QString::fromLocal8Bit(opt_mathmode));
    }
    if (opt_preamble != NULL) {
      mainWin.slotSetPreamble(QString::fromLocal8Bit(opt_preamble));
    }
    if (opt_lborderoffset != -1)
      mainWin.alterSetting(KLFMainWin::altersetting_LBorderOffset, opt_lborderoffset);
    if (opt_tborderoffset != -1)
      mainWin.alterSetting(KLFMainWin::altersetting_TBorderOffset, opt_tborderoffset);
    if (opt_rborderoffset != -1)
      mainWin.alterSetting(KLFMainWin::altersetting_RBorderOffset, opt_rborderoffset);
    if (opt_bborderoffset != -1)
      mainWin.alterSetting(KLFMainWin::altersetting_BBorderOffset, opt_bborderoffset);
    if (opt_tempdir != NULL)
      mainWin.alterSetting(KLFMainWin::altersetting_TempDir, QString::fromLocal8Bit(opt_tempdir));
    if (opt_latex != NULL)
      mainWin.alterSetting(KLFMainWin::altersetting_Latex, QString::fromLocal8Bit(opt_latex));
    if (opt_dvips != NULL)
      mainWin.alterSetting(KLFMainWin::altersetting_Dvips, QString::fromLocal8Bit(opt_dvips));
    if (opt_gs != NULL)
      mainWin.alterSetting(KLFMainWin::altersetting_Gs, QString::fromLocal8Bit(opt_gs));
    if (opt_epstopdf != NULL)
      mainWin.alterSetting(KLFMainWin::altersetting_Epstopdf, QString::fromLocal8Bit(opt_epstopdf));

    // will actually save only if output is non empty.
    mainWin.slotEvaluateAndSave(QString::fromLocal8Bit(opt_output),
				QString::fromLocal8Bit(opt_format));


    // IMPORT .klf files passed as arguments
    QStringList flist;
    for (int k = 0; klf_args[k] != NULL; ++k)
      flist << QString::fromLocal8Bit(klf_args[k]);
    mainWin.importCmdlKLFFiles(flist);

    int r = app.exec();
    main_cleanup();
    // and exit.
    // DO NOT CALL ::exit() as this prevents KLFMainWin's destructor from being called.
    return r;

  } else {

    // NON-INTERACTIVE (BATCH MODE, no X11)
    QCoreApplication app(qt_argc, qt_argv);

    qRegisterMetaType< QImage >("QImage");

    // now load default config (for default paths etc.)
    klfconfig.loadDefaults(); // must be called before 'readFromConfig'
    klfconfig.readFromConfig();

    main_load_extra_resources();

    main_load_translations(&app);

    // show version number ?
    if ( opt_version_requested ) {
      fprintf(stderr, "KLatexFormula: Version %s using Qt %s\n", KLF_VERSION_STRING, qVersion());
      main_exit(0);
    }

    // show program help ?
    if ( opt_help_requested ) {
      fprintf(stderr, "%s", QObject::tr(klfopt_helptext.source, klfopt_helptext.comment)
	      .toLocal8Bit().constData());
      main_exit(0);
    }

    if ( ! opt_quiet )
      fprintf(stderr, "KLatexFormula Version %s by Philippe Faist (c) 2005-2009\n"
	      "Licensed under the terms of the GNU Public License GPL\n\n",
	      version);
  
    if ( klf_args[0] != NULL && ! opt_quiet ) {
      fprintf(stderr, "Warning: ignoring extra command-line arguments\n");
    }

    // now process required actions.
    KLFBackend::klfInput input;
    KLFBackend::klfSettings settings;
    KLFBackend::klfOutput klfoutput;

    if ( (opt_input == NULL || !strlen(opt_input)) &&
	 (opt_latexinput == NULL || !strlen(opt_latexinput)) ) {
      // read from stdin by default
      opt_input = strdup("-");
      opt_strdup_free_list[opt_strdup_free_list_n++] = opt_input;
    }

    input.latex = main_get_input(opt_input, opt_latexinput);

    if (opt_mathmode != NULL) {
      input.mathmode = QString::fromLocal8Bit(opt_mathmode);
    } else {
      input.mathmode = "\\[ ... \\]";
    }

    if (opt_preamble != NULL) {
      input.preamble = QString::fromLocal8Bit(opt_preamble);
    } else {
      input.preamble = "";
    }

    if ( ! opt_fgcolor ) {
      opt_fgcolor = strdup("#000000");
      opt_strdup_free_list[opt_strdup_free_list_n++] = opt_fgcolor;
    }
    QColor fgcolor;
    fgcolor.setNamedColor(opt_fgcolor);
    input.fg_color = fgcolor.rgb();
    if ( ! opt_bgcolor ) {
      opt_bgcolor = strdup("-");
      opt_strdup_free_list[opt_strdup_free_list_n++] = opt_bgcolor;
    }
    QColor bgcolor;
    if (!strcmp(opt_bgcolor, "-"))
      bgcolor.setRgb(255, 255, 255, 0); // white transparent
    else
      bgcolor.setNamedColor(opt_bgcolor);
    input.bg_color = bgcolor.rgba();

    input.dpi = (opt_dpi > 0) ? opt_dpi : 1200;

    settings.lborderoffset = settings.tborderoffset
      = settings.rborderoffset = settings.bborderoffset = 1;
    if (opt_lborderoffset != -1)
      settings.lborderoffset = opt_lborderoffset;
    if (opt_tborderoffset != -1)
      settings.tborderoffset = opt_tborderoffset;
    if (opt_rborderoffset != -1)
      settings.rborderoffset = opt_rborderoffset;
    if (opt_bborderoffset != -1)
      settings.bborderoffset = opt_bborderoffset;
    settings.latexexec = klfconfig.BackendSettings.execLatex;
    settings.dvipsexec = klfconfig.BackendSettings.execDvips;
    settings.gsexec = klfconfig.BackendSettings.execGs;
    settings.epstopdfexec = klfconfig.BackendSettings.execEpstopdf;
    settings.tempdir = klfconfig.BackendSettings.tempDir;

    if (opt_tempdir != NULL)
      settings.tempdir = QString::fromLocal8Bit(opt_tempdir);
    if (opt_latex != NULL)
      settings.latexexec = QString::fromLocal8Bit(opt_latex);
    if (opt_dvips != NULL)
      settings.dvipsexec = QString::fromLocal8Bit(opt_dvips);
    if (opt_gs != NULL)
      settings.gsexec = QString::fromLocal8Bit(opt_gs);
    if (opt_epstopdf != NULL)
      settings.epstopdfexec = QString::fromLocal8Bit(opt_epstopdf);

    klfoutput = KLFBackend::getLatexFormula(input, settings);

    if (klfoutput.status != 0) {
      // error occurred

      if ( ! opt_quiet )
	fprintf(stderr, "%s\n", klfoutput.errorstr.toLocal8Bit().constData());

      main_exit(klfoutput.status);
    }

    QString output = QString::fromLocal8Bit(opt_output);
    QString format = QString::fromLocal8Bit(opt_format).trimmed().toUpper();
    main_save(klfoutput, output, format);

    main_exit( 0 );
  }

  main_exit( 0 );
}


// PARSE COMMAND-LINE OPTIONS

void main_parse_options(int argc, char *argv[])
{
  // argument processing
  int c;
  char *arg = NULL;

  // prepare fake command-line options as will be seen by Q[Core]Application
  qt_argc = 1;
  qt_argv[0] = argv[0];
  qt_argv[1] = NULL;

  // loop for each option
  for (;;) {
    // get an option from command line
    c = getopt_long(argc, argv, klfcmdl_optstring, klfcmdl_optlist, NULL);
    if (c == -1)
      break;

    arg = NULL;
    if (optarg != NULL) {
      arg = strdup(optarg);
      opt_strdup_free_list[opt_strdup_free_list_n++] = arg;
    }

    switch (c) {
    case OPT_INTERACTIVE:
      opt_interactive = 1;
      break;
    case OPT_INPUT:
      if (opt_interactive == -1) opt_interactive = 0;
      opt_input = arg;
      break;
    case OPT_LATEXINPUT:
      if (opt_interactive == -1) opt_interactive = 0;
      opt_latexinput = arg;
      break;
    case OPT_OUTPUT:
      opt_output = arg;
      break;
    case OPT_FORMAT:
      opt_format = arg;
      break;
    case OPT_FGCOLOR:
      opt_fgcolor = arg;
      break;
    case OPT_BGCOLOR:
      opt_bgcolor = arg;
      break;
    case OPT_DPI:
      opt_dpi = atoi(arg);
      break;
    case OPT_MATHMODE:
      opt_mathmode = arg;
      break;
    case OPT_PREAMBLE:
      opt_preamble = arg;
      break;
    case OPT_QUIET:
      opt_quiet = true;
      break;
    case OPT_LBORDEROFFSET:
      opt_lborderoffset = atoi(arg);
      break;
    case OPT_TBORDEROFFSET:
      opt_tborderoffset = atoi(arg);
      break;
    case OPT_RBORDEROFFSET:
      opt_rborderoffset = atoi(arg);
      break;
    case OPT_BBORDEROFFSET:
      opt_bborderoffset = atoi(arg);
      break;
    case OPT_TEMPDIR:
      opt_tempdir = arg;
      break;
    case OPT_LATEX:
      opt_latex = arg;
      break;
    case OPT_DVIPS:
      opt_dvips = arg;
      break;
    case OPT_GS:
      opt_gs = arg;
      break;
    case OPT_EPSTOPDF:
      opt_epstopdf = arg;
      break;
    case OPT_HELP:
      opt_help_requested = true;
      break;
    case OPT_VERSION:
      opt_version_requested = true;
      break;
    case OPT_QTOPT:
      qt_argv[qt_argc] = arg;
      qt_argc++;
      break;
    default:
      opt_error.has_error = true;
      opt_error.retcode = c;
      return;
    }
  }

  qt_argv[qt_argc] = NULL;

  // possibly pointing on NULL if no extra arguments
  klf_args = & argv[optind];

  if (opt_help_requested || opt_version_requested || opt_error.has_error)
    opt_interactive = 0;

  if (opt_interactive == -1) {
    // interactive (open GUI) by default
    opt_interactive = 1;
  }

  return;
}
