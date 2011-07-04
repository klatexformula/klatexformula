/***************************************************************************
 *   file main.cpp
 *   This file is part of the KLatexFormula Project.
 *   Copyright (C) 2011 by Philippe Faist
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
#include <QLibraryInfo>
#include <QMetaType>
#include <QClipboard>
#include <QFontDatabase>

#include <klfbackend.h>

#include <klfutil.h>
#include <klfcolorchooser.h>
#include "klflib.h"
#include "klflibdbengine.h"
#include "klfliblegacyengine.h"
#include "klflibview.h"
#include "klfmain.h"
#include "klfconfig.h"
#include "klfmainwin.h"
#include "klfdbus.h"
#include "klfpluginiface.h"

/** \file
 * \brief main() function for klatexformula [NOT part of klfapp]
 *
 * <span style="font-size: 14pt; font-weight: bold">NOTE: If this file appears
 * in the klfapp library documentation, it is to be noted that all definitions
 * contained in this file are compiled SEPARATELY into klatexformula itself,
 * and NOT into the klfapp library. In other words, the functions defined here
 * are NOT available in the klfapp API.</span>
 *
 * The documentation for this file is just provided for convenient source
 * browsing. It is not meant to be part of a public API.
 */


// Name of the environment variable to check for paths to extra resources
#ifndef KLF_RESOURCES_ENVNAM
#define KLF_RESOURCES_ENVNAM "KLF_RESOURCES"
#endif


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
int opt_paste = -1; // -1: don't paste, 1: paste clipboard, 2: paste selection
bool opt_noeval = false;
bool opt_base64arg = false;
char *opt_output = NULL;
char *opt_format = NULL;
char *opt_fgcolor = NULL;
char *opt_bgcolor = NULL;
int opt_dpi = -1;
char *opt_mathmode = NULL;
char *opt_preamble = NULL;
bool opt_quiet = false;
char *opt_redirect_debug = NULL;
bool opt_daemonize = false;
bool opt_dbus_export_mainwin = false;
bool opt_skip_plugins = false;

int opt_calcepsbbox = -1;
int opt_outlinefonts = -1;
QString opt_lborderoffset = QString();
QString opt_tborderoffset = QString();
QString opt_rborderoffset = QString();
QString opt_bborderoffset = QString();

char *opt_tempdir;
char *opt_latex;
char *opt_dvips;
char *opt_gs;
char *opt_epstopdf;

bool opt_help_requested = false;
FILE * opt_help_fp = stderr;
bool opt_version_requested = false;
FILE * opt_version_fp = stderr;
char *opt_version_format = (char*)"KLatexFormula: Version %k using Qt %q\n";

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
  OPT_PASTE_CLIPBOARD = 'P',
  OPT_PASTE_SELECTION = 'S',
  OPT_NOEVAL = 'n',
  OPT_BASE64ARG = 'B',
  OPT_OUTPUT = 'o',
  OPT_FORMAT = 'F',
  OPT_FGCOLOR = 'f',
  OPT_BGCOLOR = 'b',
  OPT_DPI = 'X',
  OPT_MATHMODE = 'm',
  OPT_PREAMBLE = 'p',
  OPT_QUIET = 'q',
  OPT_DAEMONIZE = 'd',

  OPT_HELP = 'h',
  OPT_VERSION = 'V',

  OPT_QTOPT = 'Q',

  OPT_CALCEPSBBOX = 127,
  OPT_NOCALCEPSBBOX,
  OPT_OUTLINEFONTS,
  OPT_NOOUTLINEFONTS,
  OPT_LBORDEROFFSET,
  OPT_TBORDEROFFSET,
  OPT_RBORDEROFFSET,
  OPT_BBORDEROFFSET,
  OPT_BORDEROFFSETS,
  OPT_TEMPDIR,
  OPT_LATEX,
  OPT_DVIPS,
  OPT_GS,
  OPT_EPSTOPDF,

  OPT_DBUS_EXPORT_MAINWIN,
  OPT_SKIP_PLUGINS,
  OPT_REDIRECT_DEBUG
};

/** A List of command-line options klatexformula accepts.
 *
 * NOTE: Remember to forward any NEW OPTIONS to the daemonized process when the new option is
 * used with --daemonize (!) (search for \c opt_daemonize).
 */
static struct option klfcmdl_optlist[] = {
  { "interactive", 0, NULL, OPT_INTERACTIVE },
  { "input", 1, NULL, OPT_INPUT },
  { "latexinput", 1, NULL, OPT_LATEXINPUT },
  { "paste-clipboard", 0, NULL, OPT_PASTE_CLIPBOARD },
  { "paste-selection", 0, NULL, OPT_PASTE_SELECTION },
  { "noeval", 0, NULL, OPT_NOEVAL },
  { "base64arg", 0, NULL, OPT_BASE64ARG },
  { "output", 1, NULL, OPT_OUTPUT },
  { "format", 1, NULL, OPT_FORMAT },
  { "fgcolor", 1, NULL, OPT_FGCOLOR },
  { "bgcolor", 1, NULL, OPT_BGCOLOR },
  { "dpi", 1, NULL, OPT_DPI },
  { "mathmode", 1, NULL, OPT_MATHMODE },
  { "preamble", 1, NULL, OPT_PREAMBLE },
  { "quiet", 2 /*optional arg*/, NULL, OPT_QUIET },
  { "redirect-debug", 1, NULL, OPT_REDIRECT_DEBUG },
  { "daemonize", 0, NULL, OPT_DAEMONIZE },
  { "dbus-export-mainwin", 0, NULL, OPT_DBUS_EXPORT_MAINWIN },
  { "skip-plugins", 2, NULL, OPT_SKIP_PLUGINS },
  // -----
  { "calcepsbbox", 2, NULL, OPT_CALCEPSBBOX },
  { "nocalcepsbbox", 2, NULL, OPT_NOCALCEPSBBOX },
  { "outlinefonts", 2, NULL, OPT_OUTLINEFONTS },
  { "nooutlinefonts", 0, NULL, OPT_NOOUTLINEFONTS },
  { "lborderoffset", 1, NULL, OPT_LBORDEROFFSET },
  { "tborderoffset", 1, NULL, OPT_TBORDEROFFSET },
  { "rborderoffset", 1, NULL, OPT_RBORDEROFFSET },
  { "bborderoffset", 1, NULL, OPT_BBORDEROFFSET },
  { "borderoffsets", 1, NULL, OPT_BORDEROFFSETS },
  // -----
  { "tempdir", 1, NULL, OPT_TEMPDIR },
  { "latex", 1, NULL, OPT_LATEX },
  { "dvips", 1, NULL, OPT_DVIPS },
  { "gs", 1, NULL, OPT_GS },
  { "epstopdf", 1, NULL, OPT_EPSTOPDF },
  // -----
  { "help", 2, NULL, OPT_HELP },
  { "version", 2, NULL, OPT_VERSION },
  // -----
  { "qtoption", 1, NULL, OPT_QTOPT },
  // ---- end of option list ----
  {0, 0, 0, 0}
};



// TRAP SOME SIGNALS TO EXIT GRACEFULLY

void signal_act(int sig)
{
  FILE *ftty = NULL;
#ifdef Q_OS_LINUX
  ftty = fopen("/dev/tty", "w");
#endif
  if (ftty == NULL)
    ftty = stderr;

  if (sig == SIGINT) {
    fprintf(ftty, "Interrupt\n");
    if (ftty != stderr)  fprintf(stderr, "*** Interrupt\n");

    static long last_sigint_time = 0;
    long curtime;
    time(&curtime);
    bool isInsisted = (curtime - last_sigint_time <= 2); // re-pressed Ctrl-C after less than 2 secs
    if (!isInsisted && qApp != NULL) {
      qApp->quit();
      last_sigint_time = curtime;
    } else {
      fprintf(ftty, "Exiting\n");
      if (ftty != stderr)  fprintf(stderr, "*** Exiting\n");
      ::exit(128);
    }
  }
  if (sig == SIGSEGV) {
    fprintf(ftty, "Segmentation Fault :-(\n");
    if (ftty != stderr)  fprintf(stderr, "** Segmentation Fault :-( **\n");

    qApp->exit(127);

    // next time SIGSEGV is sent, use default handler (exit and dump core)
    signal(SIGSEGV, SIG_DFL);
  }
}


// DEBUG, WARNING AND FATAL MESSAGES HANDLER

// redirect deboug output to this file (if non-NULL) instead of stderr
static FILE *klf_qt_msg_fp = NULL;

// in case we want to print messages directly into terminal
static FILE *klf_fp_tty = NULL;
static bool klf_fp_tty_failed = false;

void klf_qt_message(QtMsgType type, const char *msg)
{
  if (opt_quiet)
    return;

  FILE *fout = stderr;
  if (klf_qt_msg_fp != NULL)  fout = klf_qt_msg_fp;

#ifdef Q_OS_LINUX
  if (klf_fp_tty == NULL && !klf_fp_tty_failed)
    if ( !(klf_fp_tty = fopen("/dev/tty", "w")) )
      klf_fp_tty_failed = true;
#else
  Q_UNUSED(klf_fp_tty_failed) ;
#endif

  switch (type) {
  case QtDebugMsg:
    // only with debugging enabled
#ifdef KLF_DEBUG
    fprintf(fout, "D: %s\n", msg);
    fflush(fout);
#endif
    break;
  case QtWarningMsg:
    fprintf(fout, "Warning: %s\n", msg);
    fflush(fout);
#ifdef KLF_DEBUG
    // in debug mode, also print warning messages to TTY (because they get lost in the debug messages!)
    if (klf_fp_tty) fprintf(klf_fp_tty, "Warning: %s\n", msg);
#endif

#if defined Q_WS_WIN && defined KLF_DEBUG
#  define   SAFECOUNTER_NUM   10
    // only show dialog after having created a QApplication
    if (qApp != NULL && qApp->inherits("QApplication")) {
      static int safecounter = SAFECOUNTER_NUM;
      if (safecounter-- >= 0) {
	if (!QString::fromLocal8Bit(msg).startsWith("MNG error")) { // ignore these "MNG" errors...
	  QMessageBox::warning(0, "Warning",
			       QString("KLatexFormula System Warning:\n%1")
			       .arg(QString::fromLocal8Bit(msg)));
	}
      }
      if (safecounter == -1) {
	QMessageBox::information(0, "Information",
				 QString("Shown %1 system warnings. Will stop displaying them.").arg(SAFECOUNTER_NUM));
	safecounter = -2;
      }
      if (safecounter < -2) safecounter = -2;
    }
#endif
    break;
  case QtCriticalMsg:
    fprintf(fout, "Error: %s\n", msg);
    fflush(fout);
#ifdef Q_WS_WIN
    if (qApp != NULL && qApp->inherits("QApplication")) {
      QMessageBox::critical(0, QObject::tr("Error", "[[KLF's Qt Message Handler: dialog title]]"),
			    QObject::tr("KLatexFormula System Error:\n%1",
					"[[KLF's Qt Message Handler: dialog text]]")
			    .arg(QString::fromLocal8Bit(msg)));
    }
#endif
    break;
  case QtFatalMsg:
    fprintf(fout, "Fatal: %s\n", msg);
    fflush(fout);
#ifdef Q_WS_WIN
    if (qApp != NULL && qApp->inherits("QApplication")) {
      QMessageBox::critical(0, QObject::tr("FATAL ERROR",
					   "[[KLF's Qt Message Handler: dialog title]]"),
			    QObject::tr("KLatexFormula System FATAL ERROR:\n%1",
					"[[KLF's Qt Message Handler: dialog text]]")
			    .arg(QString::fromLocal8Bit(msg)));
    }
#endif
    ::exit(255);
  default:
    fprintf(fout, "?????: %s\n", msg);
    fflush(fout);
    break;
  }
}





// UTILITY FUNCTIONS


void main_parse_options(int argc, char *argv[]);

/** Free some memory we have persistently allocated */
void main_cleanup()
{
  /** \bug ................ BUG IN WINDOWS HERE? .................... ???? */
  // FIXME: under windows, we have a proliferation of qt_temp.XXXXXX files
  //   in local plugin directory, what's going on?
  QDir pdir(klfconfig.homeConfigDirPlugins);
  QStringList qttempfiles = pdir.entryList(QStringList() << "qt_temp.??????", QDir::Files);
  foreach(QString s, qttempfiles) {
    QFile::remove(pdir.absoluteFilePath(s));
  }
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
 * return the latex code as QString.
 *
 * We can count on a QCoreApplication or QApplication running.
 */
QString main_get_input(char *input, char *latexinput, int paste)
{
  QString latex;
  if (latexinput != NULL && strlen(latexinput) != 0) {
    latex += QString::fromLocal8Bit(latexinput);
  }
  if (input != NULL && strlen(input) != 0) {
    QString fname = QString::fromLocal8Bit(input);
    QFile f;
    if ( fname == "-" ) {
      if ( ! f.open(stdin, QIODevice::ReadOnly) ) {
	qCritical("%s", qPrintable(QObject::tr("Can't read standard input (!)")));
	main_exit(EXIT_ERR_FILEINPUT);
      }
    } else {
      f.setFileName(fname);
      if ( ! f.open(QIODevice::ReadOnly) ) {
	qCritical("%s", qPrintable(QObject::tr("Can't read input file `%1'.").arg(fname)));
	main_exit(EXIT_ERR_FILEINPUT);
      }
    }
    // now file is opened properly.
    QByteArray contents = f.readAll();
    // contents is assumed to be local 8 bit encoding.
    latex += QString::fromLocal8Bit(contents);
  }
  if (paste >= 0) {
    if (!qApp->inherits("QApplication")) {
      qWarning("%s",
	       qPrintable(QObject::tr("--paste-{clipboard|selection} requires interactive mode. Ignoring option.")));
    } else {
      if (paste == 1)
	latex += QApplication::clipboard()->text();
      else
	latex += QApplication::clipboard()->text(QClipboard::Selection);
    }
  }

  return latex;
}

/** Saves a klfbackend result (KLFBackend::klfOutput) to a file or stdout with given format.
 * format is guessed if not provided, and defaults to PNG. */
void main_save(KLFBackend::klfOutput klfoutput, const QString& f_output, QString format)
{
  KLFBackend::saveOutputToFile(klfoutput, f_output, format);
}

void main_load_extra_resources()
{
  KLF_DEBUG_BLOCK(KLF_FUNC_NAME) ;

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
  QStringList defaultrccpaths;
#ifdef KLF_SHARE_RCCRESOURCES_DIR
  defaultrccpaths << klfPrefixedPath(KLF_SHARE_RCCRESOURCES_DIR); // prefixed by app-dir-path
#endif
  defaultrccpaths << klfconfig.globalShareDir+"/rccresources/";
  defaultrccpaths << klfconfig.homeConfigDirRCCResources;
  klfDbg("RCC search path is "<<defaultrccpaths.join(QString()+KLF_PATH_SEP)) ;
  QString rccfilepath;
  if ( klf_resources.isNull() ) {
    rccfilepath = "";
  } else {
    rccfilepath = klf_resources;
  }
  //  printf("DEBUG: Rcc file list is \"%s\"\n", rccfilepath.toLocal8Bit().constData());
  QStringList rccfiles = rccfilepath.split(KLF_PATH_SEP, QString::KeepEmptyParts);
  int j, k;
  for (QStringList::iterator it = rccfiles.begin(); it != rccfiles.end(); ++it) {
    if ((*it).isEmpty()) {
      // empty split section: meaning that we want default paths at this point
      it = rccfiles.erase(it, it+1);
      for (j = 0; j < defaultrccpaths.size(); ++j) {
	it = rccfiles.insert(it, defaultrccpaths[j]) + 1;
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
	QString f = files[k].canonicalFilePath();
	if (!rccfilesToLoad.contains(f))
	  rccfilesToLoad << f;
      }
    } else if (fi.isFile() && fi.suffix() == "rcc") {
      QString f = fi.canonicalFilePath();
      if (!rccfilesToLoad.contains(f))
	rccfilesToLoad << f;
    }
  }
  for (j = 0; j < rccfilesToLoad.size(); ++j) {
    KLFAddOnInfo addoninfo(rccfilesToLoad[j]);
    // resource registered.
    klf_addons.append(addoninfo);
    klfDbg("registered resource "<<addoninfo.fpath()<<".") ;
  }

  // set the global "can-import" flag
  klf_addons_canimport = klfsettings_can_import;

  void dumpDir(const QDir&, int = 0);
  klfDbg( "dump of :/ :" ) ;
  dumpDir(QDir(":/"));
}


void dumpDir(const QDir& d, int indent = 0)
{
  char sindent[] = "                                                               ";
  uint nindent = indent*2; // 2 spaces per indentation
  if (nindent < strlen(sindent))
    sindent[nindent] = '\0';

  QStringList dchildren = d.entryList(QDir::Dirs);

  int k;
  for (k = 0; k < dchildren.size(); ++k) {
    // skip system ":/trolltech"
    if (indent == 0 && dchildren[k] == "trolltech")
      continue;
    qDebug("%s%s/", sindent, qPrintable(dchildren[k]));
    dumpDir(QDir(d.absoluteFilePath(dchildren[k])), indent+1);
  }

  QStringList fchildren = d.entryList(QDir::Files);
  for (k = 0; k < fchildren.size(); ++k) {
    qDebug("%s%s", sindent, qPrintable(fchildren[k]));
  }
}

/** \internal */
class VersionCompareWithPrefixGreaterThan {
  int prefixLen;
public:
  /** only the length of prefix is important, the prefix itself is not checked. */
  VersionCompareWithPrefixGreaterThan(const QString& prefix) : prefixLen(prefix.length()) { }
  bool operator()(const QString& a, const QString& b) {
    return klfVersionCompare(a.mid(prefixLen), b.mid(prefixLen)) > 0;
  }
};

void main_load_plugins(QApplication *app, KLFMainWin *mainWin)
{
  KLF_DEBUG_BLOCK(KLF_FUNC_NAME) ;

  QStringList baseplugindirs =
    QStringList() << klfconfig.homeConfigDirPlugins << klfconfig.globalShareDir+"/plugins";

  klfDbg("base plugins dirs are "<<baseplugindirs) ;

  // first step: copy all resource-located plugin libraries to our local config
  // directory because we can only load filesystem-located plugins.
  int i, k, j;
  for (k = 0; k < klf_addons.size(); ++k) {
    QStringList pluginList = klf_addons[k].pluginList();
    for (j = 0; j < pluginList.size(); ++j) {
      KLFAddOnInfo::PluginSysInfo psinfo = klf_addons[k].pluginSysInfo(pluginList[j]);
      klfDbg( "Testing plugin psinfo="<<psinfo<<"\n\tTo our system: qtver="<<qVersion()
	      <<"; klfver="<<KLF_VERSION_STRING<<"; os="<<KLFSysInfo::osString()
	      <<"; arch="<<KLFSysInfo::arch() ) ;
      if ( psinfo.isCompatibleWithCurrentSystem() ) {
	// ok to install plugin
	QString resfn = klf_addons[k].rccmountroot() + "/plugins/" + pluginList[j];
	QString locsubdir = klf_addons[k].pluginLocalSubDirName(pluginList[j]);
	QString locfn = klfconfig.homeConfigDirPlugins + "/" + locsubdir + "/"
	  + QFileInfo(pluginList[j]).fileName();
	QDateTime installedplugin_dt = QFileInfo(locfn).lastModified();
	QDateTime resourceplugin_dt = QFileInfo(klf_addons[k].fpath()).lastModified();
	qDebug("Comparing resource datetime (%s) with installed plugin datetime (%s)",
	       qPrintable(resourceplugin_dt.toString()), qPrintable(installedplugin_dt.toString()));
	if (  ! QFile::exists( locfn ) ||
	      installedplugin_dt.isNull() || resourceplugin_dt.isNull() ||
	      ( resourceplugin_dt > installedplugin_dt )  ) {
	  // create path to that plugin dir
	  if (!locsubdir.isEmpty() &&
	      !QDir(klfconfig.homeConfigDirPlugins + "/plugins/" + locsubdir).exists())
	    QDir(klfconfig.homeConfigDirPlugins).mkpath(locsubdir);
	  // remove old version if exists
	  if (QFile::exists(locfn)) QFile::remove(locfn);
	  // copy plugin to local plugin dir
	  klfDbg( "\tcopy "<<resfn<<" to "<<locfn ) ;
	  bool res = QFile::copy( resfn , locfn );
	  if ( ! res ) {
	    qWarning("Unable to copy plugin '%s' to local directory!", qPrintable(pluginList[j]));
	  } else {
	    QFile::setPermissions(locfn, QFile::ReadOwner|QFile::WriteOwner|QFile::ExeOwner|
				  QFile::ReadUser|QFile::WriteUser|QFile::ExeUser|
				  QFile::ReadGroup|QFile::ExeGroup|QFile::ReadOther|QFile::ExeOther);
	    qDebug("Copied plugin %s to local directory %s.", qPrintable(resfn), qPrintable(locfn));
	  }
	}
      }
      // OK, plugin locally installed.
    }
  }

  // explore all base plugins dir, eg. /usr/share/klatexformula/plugins, and ~/.klatexformula/plugins/
  int n;
  for (n = 0; n < baseplugindirs.size(); ++n) {
    QString baseplugindir = baseplugindirs[n];
    klfDbg("exploring base plugin directory "<<baseplugindir) ;
    // build a list of plugin directories to search. We will need to load plugins in our version directory
    // + all prior version directories + root plugin path.
    QStringList pluginsdirs;
    // For each path in pluginsdirs, in this array (at same index pos) we have the relative path from baseplugindir.
    QStringList pluginsdirsbaserel;
    QDir pdir(baseplugindir);
    QStringList pdirlist = pdir.entryList(QStringList()<<"klf*", QDir::Dirs);
    // sort plugin dirs so that for a plugin existing in multiple versions, we load the one for the
    // most recent first, then ignore the others.
    qSort(pdirlist.begin(), pdirlist.end(), VersionCompareWithPrefixGreaterThan("klf"));
    for (i = 0; i < pdirlist.size(); ++i) {
      klfDbg( "maybe adding plugin dir"<<pdirlist[i]<<"; klfver="<<pdirlist[i].mid(3) ) ;
      if (klfVersionCompare(pdirlist[i].mid(3), KLF_VERSION_STRING) <= 0) { // Version OK
	pluginsdirs << pdir.absoluteFilePath(pdirlist[i]) ;
	pluginsdirsbaserel << pdirlist[i]+"/";
      }
    }
    pluginsdirs << klfconfig.homeConfigDirPlugins ;
    pluginsdirsbaserel << "" ;

    klfDbg( "pluginsdirs="<<pluginsdirs ) ;
    
    for (i = 0; i < pluginsdirs.size(); ++i) {
      if ( ! QFileInfo(pluginsdirs[i]).isDir() )
	continue;

      QDir thisplugdir(pluginsdirs[i]);
      QStringList plugins = thisplugdir.entryList(KLF_DLL_EXT_LIST, QDir::Files);
      KLFPluginGenericInterface * pluginInstance;
      for (j = 0; j < plugins.size(); ++j) {
	QString pluginfname = plugins[j];
	QString pluginfnamebaserel = pluginsdirsbaserel[i]+plugins[j];
	bool plugin_already_loaded = false;
	int k;
	for (k = 0; k < klf_plugins.size(); ++k) {
	  if (QFileInfo(klf_plugins[k].fname).fileName() == pluginfname) {
	    klfDbg( "Rejecting loading of plugin "<<pluginfname<<" in dir "<<pluginsdirs[i]
		    <<"; already loaded." ) ;
	    plugin_already_loaded = true;
	    break;
	  }
	}
	if (plugin_already_loaded)
	  continue;
	QString pluginpath = thisplugdir.absoluteFilePath(pluginfname);
	QPluginLoader pluginLoader(pluginpath, app);
	bool loaded = pluginLoader.load();
	if (!loaded) {
	  klfDbg("QPluginLoader failed to load plugin "<<pluginpath<<". Skipping.");
	  continue;
	}
	QObject *pluginInstObject = pluginLoader.instance();
	if (pluginInstObject == NULL) {
	  klfDbg("QPluginLoader failed to load plugin "<<pluginpath<<" (object is NULL). Skipping.");
	  continue;
	}
	pluginInstance = qobject_cast<KLFPluginGenericInterface *>(pluginInstObject);
	if (pluginInstance == NULL) {
	  klfDbg("QPluginLoader failed to load plugin "<<pluginpath<<" (instance is NULL). Skipping.");
	  continue;
	}
	// plugin file successfully loaded.
	QString nm = pluginInstance->pluginName();
	klfDbg("Successfully loaded plugin library "<<nm<<" ("<<qPrintable(pluginInstance->pluginDescription())
	       <<") from file "<<pluginfnamebaserel);

	if ( ! klfconfig.Plugins.pluginConfig.contains(nm) ) {
	  // create default plugin configuration if non-existant
	  klfconfig.Plugins.pluginConfig[nm] = QMap<QString, QVariant>();
	  // ask plugin whether it's supposed to be loaded by default
	  klfconfig.Plugins.pluginConfig[nm]["__loadenabled"] =
	    pluginInstance->pluginDefaultLoadEnable();
	}
	bool keepPlugin = true;

	// make sure this plugin wasn't already loaded (eg. in a different klf-version sub-dir)
	bool pluginRejected = false;
	for (k = 0; k < klf_plugins.size(); ++k) {
	  if (klf_plugins[k].name == nm) {
	    klfDbg( "Rejecting loading of plugin "<<nm<<" in "<<pluginfname<<"; already loaded." ) ;
	    pluginLoader.unload();
	    pluginRejected = true;
	    break;
	  }
	}
	if (pluginRejected)
	  continue;

	KLFPluginInfo pluginInfo;
	pluginInfo.name = nm;
	pluginInfo.title = pluginInstance->pluginTitle();
	pluginInfo.description = pluginInstance->pluginDescription();
	pluginInfo.author = pluginInstance->pluginAuthor();
	pluginInfo.fname = pluginfnamebaserel;
	pluginInfo.fpath = pluginpath;
	pluginInfo.instance = NULL;

	// if we are configured to load this plugin, load it.
	keepPlugin = keepPlugin && klfconfig.Plugins.pluginConfig[nm]["__loadenabled"].toBool();
	klfDbg("got plugin info. keeping plugin? "<<keepPlugin);
	if ( keepPlugin ) {
	  KLFPluginConfigAccess pgca = klfconfig.getPluginConfigAccess(nm);
	  KLFPluginConfigAccess * c = new KLFPluginConfigAccess(pgca);
	  klfDbg("prepared a configaccess "<<c);
	  pluginInstance->initialize(app, mainWin, c);
	  pluginInfo.instance = pluginInstance;
	  qDebug("\tPlugin %s loaded and initialized.", qPrintable(nm));
	} else {
	  // if we aren't configured to load it, then discard it, but keep info with NULL instance,
	  // so that user can configure to load or not this plugin in the settings dialog.
	  delete pluginInstance;
	  pluginInfo.instance = NULL;
	  qDebug("\tPlugin %s NOT loaded.", qPrintable(nm));
	}
	klf_plugins.push_back(pluginInfo);
      }
    }
  }
}




// function to set up the Q[Core]Application correctly
void main_setup_app(QCoreApplication *a)
{
  a->setApplicationName(QLatin1String("KLatexFormula"));
  a->setApplicationVersion(QLatin1String(KLF_VERSION_STRING));
  a->setOrganizationDomain(QLatin1String("klatexformula.org"));
  a->setOrganizationName(QLatin1String("KLatexFormula"));

#ifdef KLF_LIBKLFTOOLS_STATIC
  Q_INIT_RESOURCE(klftoolsres) ;
#endif
#ifdef KLF_LIBKLFAPP_STATIC
  Q_INIT_RESOURCE(klfres) ;
#endif

  // add [share dir]/qt-plugins to library path.
  // under windows, that is were plugins are packaged with the executable
  extern QString klf_share_dir_abspath();
  QCoreApplication::addLibraryPath(klf_share_dir_abspath()+"/qt-plugins");

  klfDbg("Library paths are:\n"<<qPrintable(QCoreApplication::libraryPaths().join("\n")));

  qRegisterMetaType< QImage >("QImage");
  qRegisterMetaType< KLFStyle >();
  qRegisterMetaTypeStreamOperators< KLFStyle >("KLFStyle");
  qRegisterMetaType< KLFLibEntry >();
  qRegisterMetaTypeStreamOperators< KLFLibEntry >("KLFLibEntry");
  qRegisterMetaType< KLFLibResourceEngine::KLFLibEntryWithId >();
  qRegisterMetaTypeStreamOperators< KLFLibResourceEngine::KLFLibEntryWithId >
    /* */  ("KLFLibResourceEngine::KLFLibEntryWithId");

  // for delayed calls in klflibview.cpp
  qRegisterMetaType< QItemSelection >("QItemSelection");
  qRegisterMetaType< QItemSelectionModel::SelectionFlags >("QItemSelectionModel::SelectionFlags");
}


// OUR MAIN FUNCTION

int main(int argc, char **argv)
{
  int k;
  klfDbgT("$$main()$$") ;

  qInstallMsgHandler(klf_qt_message);

  //  // DEBUG: command-line arguments
  //  for (int jjj = 0; jjj < argc; ++jjj)
  //    qDebug("arg: %s", argv[jjj]);

  // signal acting -- catch SIGINT to exit gracefully
  signal(SIGINT, signal_act);
  // signal acting -- catch SIGSEGV to attempt graceful exit
  signal(SIGSEGV, signal_act);

  klfDbg("about to parse options") ;

  // parse command-line options
  main_parse_options(argc, argv);

  klfDbg("options parsed.") ;

  // error handling
  if (opt_error.has_error) {
    qCritical("Error while parsing command-line arguments.");
    qCritical("Use --help to display command-line help.");
    main_exit(EXIT_ERR_OPT);
  }

  // redirect debug output if requested
  if (opt_redirect_debug != NULL) {
    // force the file name to end in .klfdebug to make sure we don't overwrite an important file
    char fname[1024];
    const char * SUFFIX = ".klfdebug";
    strcpy(fname, opt_redirect_debug);
    if (strncmp(fname+(strlen(fname)-strlen(SUFFIX)), SUFFIX, strlen(SUFFIX)) != 0) {
      // fname does not end with SUFFIX
      strcat(fname, SUFFIX);
    }
    // before performing the redirect...
    klfDbg("Redirecting debug output to file "<<QString::fromLocal8Bit(fname)) ;
    klf_qt_msg_fp = fopen(fname, "w");
    KLF_ASSERT_NOT_NULL( klf_qt_msg_fp, "debug output redirection failed." , /* no fail action */; ) ;
    if (klf_qt_msg_fp != NULL) {
      fprintf(klf_qt_msg_fp, "\n\n"
	      "-------------------------------------------------\n"
	      "  KLATEXFORMULA DEBUG OUTPUT\n"
	      "-------------------------------------------------\n"
	      "Started on %s\n\n",
	      qPrintable(QDateTime::currentDateTime().toString(Qt::DefaultLocaleLongDate)));
    }
  }

  if ( opt_interactive ) {
    // save the qt_argv options separately to pass them to daemonized process if needed, before
    // QApplication modifies the qt_argv array
    QStringList qtargvlist;
    for (k = 0; k < qt_argc && qt_argv[k] != NULL; ++k)
      qtargvlist << QString::fromLocal8Bit(qt_argv[k]);

    // Create the QApplication
#ifdef Q_WS_MAC
    // this is needed to avoid having default app font set right after window activation :(
    QApplication::setDesktopSettingsAware(false);
#endif
    QApplication app(qt_argc, qt_argv);

#ifdef Q_WS_MAC
    app.setFont(QFont("Lucida Grande", 13));

    extern void __klf_init_the_macpasteboardmime();
    __klf_init_the_macpasteboardmime();
    //    extern void qt_set_sequence_auto_mnemonic(bool b);
    //    qt_set_sequence_auto_mnemonic(true);
#endif

    // add our default application font(s) ;-)
    QFileInfoList appFontsInfoList = QDir(":/data/fonts/").entryInfoList(QStringList()<<"*.otf"<<"*.ttf");
    int k;
    for (k = 0; k < appFontsInfoList.size(); ++k) {
      QFontDatabase::addApplicationFont(appFontsInfoList[k].absoluteFilePath());
    }

    // main_get_input relies on a Q[Core]Application
    QString latexinput = main_get_input(opt_input, opt_latexinput, opt_paste);

    // see if we have to daemonize
    if ( opt_daemonize ) {
      // try to start detached process, with our arguments. This is preferred to feeding D-BUS input
      // to the new process, since we cannot be sure this system supports D-BUS, and we would have
      // to wait to see the new process appear, etc. and I really don't see the big advantage over
      // cmdl options here.
      QString progexe = QCoreApplication::applicationFilePath();
      QStringList args;
      args << "-I";
      if (!latexinput.isNull())
	args << "--latexinput="+latexinput;
      if (opt_noeval)
	args << "--noeval";
      if (opt_output != NULL)
	args << "--output="+QString::fromLocal8Bit(opt_output);
      if (opt_format != NULL)
	args << "--format="+QString::fromLocal8Bit(opt_format);
      if (opt_fgcolor != NULL)
	args << "--fgcolor="+QString::fromLocal8Bit(opt_fgcolor);
      if (opt_bgcolor != NULL)
	args << "--bgcolor="+QString::fromLocal8Bit(opt_bgcolor);
      if (opt_dpi >= 0)
	args << "--dpi="+QString::number(opt_dpi);
      if (opt_mathmode != NULL)
	args << "--mathmode="+QString::fromLocal8Bit(opt_mathmode);
      if (opt_preamble != NULL)
	args << "--preamble="+QString::fromLocal8Bit(opt_preamble);
      if (opt_quiet)
	args << "--quiet";
      if (opt_redirect_debug != NULL)
	args << "--redirect-debug="+QString::fromLocal8Bit(opt_redirect_debug);
      if (opt_calcepsbbox >= 0)
	args << "--calcepsbbox="+QString::fromLatin1(opt_calcepsbbox?"1":"0");
      if (opt_outlinefonts >= 0)
	args << "--outlinefonts="+QString::fromLatin1(opt_outlinefonts?"1":"0");
      const struct { char c; QString optval; } borderoffsets[] =
						{ {'t', opt_tborderoffset}, {'r', opt_rborderoffset},
						  {'b', opt_bborderoffset}, {'l', opt_lborderoffset},
						  {'\0', QString() } };
      for (k = 0; borderoffsets[k].c != 0; ++k)
	if (borderoffsets[k].optval.length())
	  args << (QString::fromLatin1("--")+QLatin1Char(borderoffsets[k].c)+"borderoffset="
		   +borderoffsets[k].optval) ;
      if (opt_tempdir != NULL)
	args << "--tempdir="+QString::fromLocal8Bit(opt_tempdir);
      if (opt_latex != NULL)
	args << "--latex="+QString::fromLocal8Bit(opt_latex);
      if (opt_dvips != NULL)
	args << "--dvips="+QString::fromLocal8Bit(opt_dvips);
      if (opt_gs != NULL)
	args << "--gs="+QString::fromLocal8Bit(opt_gs);
      if (opt_epstopdf != NULL)
	args << "--epstopdf="+QString::fromLocal8Bit(opt_epstopdf);
      for (k = 0; k < qtargvlist.size(); ++k)
	args << "--qtoption="+qtargvlist[k];
      // add additional args
      for (k = 0; klf_args[k] != NULL; ++k)
	args << QString::fromLocal8Bit(klf_args[k]);

      klfDbg("Prepared daemonized process' command-line: progexe="<<progexe<<"; args="<<args) ;

      // now launch the klatexformula 'daemon' process
      qint64 pid;
      bool result = QProcess::startDetached(progexe, args, QDir::currentPath(), &pid);
      if (result) { // Success
	if (!opt_quiet)
	  fprintf(stderr, "%s",
		  qPrintable(QObject::tr("KLatexFormula Daemon Process successfully launched with pid %1\n")
			     .arg(pid)));
	return 0;
      }
      qWarning()<<qPrintable(QObject::tr("Failed to launch daemon process. Not daemonizing."));
    }

    main_setup_app(&app);

#if defined(KLF_USE_DBUS)
    // see if an instance of KLatexFormula is running...
    KLFDBusAppInterface *iface
      = new KLFDBusAppInterface("org.klatexformula.KLatexFormula", "/MainApplication",
				QDBusConnection::sessionBus(), &app);
    if (iface->isValid()) {
      iface->raiseWindow();
      // load everything via DBus
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
      // load latex after preamble, so that the interface doesn't prompt to include missing packages
      if ( ! latexinput.isNull() )
	iface->setInputData("latex", latexinput);
      if (opt_calcepsbbox >= 0)
	iface->setAlterSetting_i(KLFMainWin::altersetting_CalcEpsBoundingBox, opt_calcepsbbox);
      if (opt_outlinefonts >= 0)
	iface->setAlterSetting_i(KLFMainWin::altersetting_OutlineFonts, opt_outlinefonts);
      if (opt_lborderoffset.length())
	iface->setAlterSetting_s(KLFMainWin::altersetting_LBorderOffset, opt_lborderoffset);
      if (opt_tborderoffset.length())
	iface->setAlterSetting_s(KLFMainWin::altersetting_TBorderOffset, opt_tborderoffset);
      if (opt_rborderoffset.length())
	iface->setAlterSetting_s(KLFMainWin::altersetting_RBorderOffset, opt_rborderoffset);
      if (opt_bborderoffset.length())
	iface->setAlterSetting_s(KLFMainWin::altersetting_BBorderOffset, opt_bborderoffset);
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
      if (!opt_noeval || opt_output) {
	iface->evaluateAndSave(QString::fromLocal8Bit(opt_output), QString::fromLocal8Bit(opt_format));
      }
      // and import KLF files if wanted
      QStringList flist;
      for (int k = 0; klf_args[k] != NULL; ++k)
	flist << QString::fromLocal8Bit(klf_args[k]);
      iface->openFiles(flist);
      main_cleanup();
      return 0;
    }
#endif

    if ( ! opt_quiet )
      fprintf(stderr, "KLatexFormula Version %s by Philippe Faist (c) 2005-2011\n"
	      "Licensed under the terms of the GNU Public License GPL\n\n",
	      KLF_VERSION_STRING);

    klfDbgT("$$About to load config$$");
  
    // now load default config
    klfconfig.loadDefaults(); // must be called before 'readFromConfig'
    klfconfig.readFromConfig();
    klfconfig.detectMissingSettings();

    klfDbgT("$$About to main_load_extra_resources$$");
    main_load_extra_resources();

    klfDbgT("$$About to main_reload_translations$$");
    klf_reload_translations(&app, klfconfig.UI.locale);

    KLFColorChooser::setUserMaxColors(klfconfig.UI.maxUserColors);
    KLFColorChooser::setColorList(klfconfig.UI.userColorList);
    KLFColorChooseWidget::setRecentCustomColors(klfconfig.UI.colorChooseWidgetRecent,
						klfconfig.UI.colorChooseWidgetCustom);

    klfDbgT("$$About to create lib factories$$");

    // initialize and register some library resource engine + view factories
    (void)new KLFLibBasicWidgetFactory(qApp);
    (void)new KLFLibDBEngineFactory(qApp);
    (void)new KLFLibLegacyEngineFactory(qApp);
    (void)new KLFLibDefaultViewFactory(qApp);

    klfDbgT( "$$START LOADING$$" ) ;

    KLFMainWin mainWin;

    if (!klfconfig.UI.useSystemAppFont)
      app.setFont(klfconfig.UI.applicationFont);

    mainWin.refreshWindowSizes();

    if (!opt_skip_plugins)
      main_load_plugins(&app, &mainWin);

    mainWin.show();

    mainWin.startupFinished();

    klfDbgT( "$$END LOADING$$" ) ;

#if defined(KLF_USE_DBUS)
    new KLFDBusAppAdaptor(&app, &mainWin);
    QDBusConnection dbusconn = QDBusConnection::sessionBus();
    dbusconn.registerService("org.klatexformula.KLatexFormula");
    dbusconn.registerObject("/MainApplication", &app);
    if (opt_dbus_export_mainwin)
      dbusconn.registerObject("/MainWindow/KLFMainWin", &mainWin, QDBusConnection::ExportAllContents
			      | QDBusConnection::ExportChildObjects);
#endif

    // parse command-line given actions

    // consistency check warning
    if (opt_output && latexinput.isEmpty()) {
      qWarning("%s", qPrintable(QObject::tr("Can't use --output without any input")));
    }

    if ( ! latexinput.isNull() )
      mainWin.slotSetLatex(latexinput);

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
      qDebug("opt_preamble != NULL, gui mode, preamble=%s", opt_preamble);
      mainWin.slotSetPreamble(QString::fromLocal8Bit(opt_preamble));
    }
    if (opt_calcepsbbox >= 0)
      mainWin.alterSetting(KLFMainWin::altersetting_CalcEpsBoundingBox, opt_calcepsbbox);
    if (opt_outlinefonts >= 0)
      mainWin.alterSetting(KLFMainWin::altersetting_OutlineFonts, opt_outlinefonts);
    if (opt_lborderoffset.length())
      mainWin.alterSetting(KLFMainWin::altersetting_LBorderOffset, opt_lborderoffset);
    if (opt_tborderoffset.length())
      mainWin.alterSetting(KLFMainWin::altersetting_TBorderOffset, opt_tborderoffset);
    if (opt_rborderoffset.length())
      mainWin.alterSetting(KLFMainWin::altersetting_RBorderOffset, opt_rborderoffset);
    if (opt_bborderoffset.length())
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

    if (!opt_noeval && opt_output) {
      // will actually save only if output is non empty.
      mainWin.slotEvaluateAndSave(QString::fromLocal8Bit(opt_output),
				  QString::fromLocal8Bit(opt_format));
    }


    // IMPORT .klf (or other) files passed as arguments
    QStringList flist;
    for (int k = 0; klf_args[k] != NULL; ++k)
      flist << QString::fromLocal8Bit(klf_args[k]);

    QMetaObject::invokeMethod(&mainWin, "openFiles", Qt::QueuedConnection, Q_ARG(QStringList, flist));

    app.setQuitOnLastWindowClosed(false);
    int r = app.exec();
    main_cleanup();
    klfDbg("application has quit; we have cleaned up main(), ready to return. code="<<r) ;
    // and exit.
    // DO NOT CALL ::exit() as this prevents KLFMainWin's destructor from being called.
    // This includes not calling main_exit().
    return r;

  } else {
    // NON-INTERACTIVE (BATCH MODE, no X11)

    // Create the QCoreApplication
    QCoreApplication app(qt_argc, qt_argv);

    // main_get_input relies on a Q[Core]Application
    QString latexinput = main_get_input(opt_input, opt_latexinput, opt_paste);

    main_setup_app(&app);

    // now load default config
    klfconfig.loadDefaults(); // must be called before 'readFromConfig'
    klfconfig.readFromConfig();
    klfconfig.detectMissingSettings();

    main_load_extra_resources();

    klf_reload_translations(&app, klfconfig.UI.locale);

    // show version number ?
    if ( opt_version_requested ) {
      /* Remember: the format here should NOT change from one version to another, so that it
       * can be parsed eg. by scripts if needed. */
      QString version_string = QString::fromLocal8Bit(opt_version_format);
      version_string.replace(QLatin1String("%k"), QLatin1String(KLF_VERSION_STRING));
      version_string.replace(QLatin1String("%q"), QLatin1String(qVersion()));
      version_string.replace(QLatin1String("%%"), QLatin1String("%"));
      fprintf(opt_version_fp, "%s\n", qPrintable(version_string));
      main_exit(0);
    }

    // show program help ?
    if ( opt_help_requested ) {
      QFile cmdlHelpFile(klfFindTranslatedDataFile(":/data/cmdl-help", ".txt"));
      if (!cmdlHelpFile.open(QIODevice::ReadOnly)) {
	qWarning()<<KLF_FUNC_NAME<<": Can't access command-line-help file :/data/cmdl-help.txt!";
	main_exit(-1);
      }
      QString helpData = QString::fromUtf8(cmdlHelpFile.readAll());
      fprintf(opt_help_fp, "%s", helpData.toLocal8Bit().constData());
      main_exit(0);
    }

    if ( ! opt_quiet )
      fprintf(stderr, "KLatexFormula Version %s by Philippe Faist (c) 2005-2011\n"
	      "Licensed under the terms of the GNU Public License GPL\n\n",
	      KLF_VERSION_STRING);

    if ( opt_daemonize ) {
      qWarning()<<qPrintable(QObject::tr("The option --daemonize can only be used in interactive mode."));
    }
  
    // warn for ignored arguments
    for (int kl = 0; klf_args[kl] != NULL; ++kl)
      qWarning()<<qPrintable(QObject::tr("[Non-Interactive Mode] Ignoring additional command-line argument: %1")
			     .arg(klf_args[kl]));


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

    input.latex = latexinput;

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

    settings.calcEpsBoundingBox = true;
    if (opt_calcepsbbox >= 0)
      settings.calcEpsBoundingBox = (bool)opt_calcepsbbox;
    settings.outlineFonts = true;
    if (opt_outlinefonts >= 0)
      settings.outlineFonts = (bool)opt_outlinefonts;
    settings.lborderoffset = settings.tborderoffset
      = settings.rborderoffset = settings.bborderoffset = 1;
    if (opt_lborderoffset.length())
      settings.lborderoffset = opt_lborderoffset.toDouble();
    if (opt_tborderoffset.length())
      settings.tborderoffset = opt_tborderoffset.toDouble();
    if (opt_rborderoffset.length())
      settings.rborderoffset = opt_rborderoffset.toDouble();
    if (opt_bborderoffset.length())
      settings.bborderoffset = opt_bborderoffset.toDouble();
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

FILE *main_msg_get_fp_arg(const char *arg)
{
  FILE *fp = NULL;
  if (arg != NULL) {
    if (arg[0] == '&') { // file descriptor number
      int fd = atoi(&arg[1]);
      if (fd > 0)
	fp = fdopen(fd, "a");
      if (fd <= 0 || fp == NULL) {
	qWarning("Failed to open file descriptor %d.", fd);
	return stderr;
      }
      return fp;
    }
    if (!strcmp(arg, "-")) { // stdout
      return stdout;
    }
    // file name
    fp = fopen(arg, "a");
    if (fp == NULL) {
      qWarning("Failed to open file `%s' to print help message.", arg);
      return stderr;
    }
    return fp;
  }
  return stderr;
}

static bool __klf_parse_bool_arg(const char * arg, bool defaultvalue)
{
  if (arg == NULL)
    return defaultvalue;

  QRegExp booltruerx = QRegExp("^\\s*on|y(es)?|1|t(rue)?\\s*", Qt::CaseInsensitive);
  QRegExp boolfalserx = QRegExp("^\\s*off|n(o)?|0|f(alse)?\\s*", Qt::CaseInsensitive);

  if ( booltruerx.exactMatch(arg) )
    return true;
  if ( boolfalserx.exactMatch(arg) )
    return false;

  qWarning("%s", qPrintable(QObject::tr("Can't parse boolean argument: `%1'").arg(arg)));
  opt_error.has_error = true;
  opt_error.retcode = -1;

  return defaultvalue;
}

#define D_RX "([0-9eE.-]+)"
#define SEP "[ \t,;]+"

static void klf_warn_parse_double(const QString& s)
{
  bool ok;
  (void)s.toDouble(&ok);
  if (!ok) {
    qWarning("%s", qPrintable(QObject::tr("Can't parse number argument: `%1'").arg(s)));
  }
}

static void klf_read_borderoffsets(const char * arg)
{
  if (arg == NULL)
    return;

  QRegExp rx("" D_RX "(?:" SEP D_RX "(?:" SEP D_RX "(?:" SEP D_RX ")?)?)?");

  if (rx.indexIn(QString::fromLocal8Bit(arg)) < 0) {
    qWarning("%s", qPrintable(QObject::tr("Expected --borderoffsets=L[,T[,R[,B]]]")));
    return;
  }

  QString l, t, r, b;
  l = rx.cap(1);
  t = rx.cap(2);
  r = rx.cap(3);
  b = rx.cap(4);
  klf_warn_parse_double(l);
  opt_lborderoffset = l;

  if (t.isEmpty()) {
    opt_tborderoffset = opt_rborderoffset = opt_bborderoffset = l;
    return;
  }
  // t is set
  klf_warn_parse_double(t);
  opt_tborderoffset = t;

  if (r.isEmpty()) {
    opt_rborderoffset = opt_lborderoffset;
    opt_bborderoffset = opt_tborderoffset;
    return;
  }
  // r is set
  klf_warn_parse_double(r);
  opt_rborderoffset = r;

  if (b.isEmpty()) {
    opt_bborderoffset = opt_tborderoffset;
    return;
  }
  // b is set
  opt_bborderoffset = b;

  return;
}


void main_parse_options(int argc, char *argv[])
{
  // argument processing
  int c;
  char *arg = NULL;

  // prepare fake command-line options as will be seen by Q[Core]Application
  qt_argc = 1;
  qt_argv[0] = argv[0];
  qt_argv[1] = NULL;

  // build getopt_long short option list
  char klfcmdl_optstring[1024];
  int k, j;
  for (k = 0, j = 0; klfcmdl_optlist[k].name != NULL; ++k) {
    if (klfcmdl_optlist[k].val < 127) { // has short option char
      klfcmdl_optstring[j++] = klfcmdl_optlist[k].val;
      if (klfcmdl_optlist[k].has_arg)
	klfcmdl_optstring[j++] = ':';
    }
  }
  klfcmdl_optstring[j] = '\0'; // terminate C string

  // loop for each option
  for (;;) {
    // get an option from command line
    c = getopt_long(argc, argv, klfcmdl_optstring, klfcmdl_optlist, NULL);
    if (c == -1)
      break;

    arg = NULL;
    if (optarg != NULL) {
      if (opt_base64arg) {
	// this argument is to be decoded from base64
	//
	// note that here QByteArray can function without a Q[Core]Application
	// (officially? this is just suggested by the fact that they mention it
	//  explicitely for QString: 
	//  http://doc.trolltech.com/4.4/qcoreapplication.html#details)
	QByteArray decoded = QByteArray::fromBase64(optarg);
	arg = strdup(decoded.constData());
      } else {
	arg = strdup(optarg);
      }
      opt_strdup_free_list[opt_strdup_free_list_n++] = arg;
    }
    // immediately reset this flag, as it applies to the argument of the next option
    // only (which we have just retrieved and possibly decoded)
    opt_base64arg = false;

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
    case OPT_PASTE_CLIPBOARD:
      if (opt_interactive <= 0) {
	if (opt_interactive == 0)
	  qWarning("%s", qPrintable(QObject::tr("--paste-clipboard requires interactive mode. Switching.")));
	opt_interactive = 1;
      }
      opt_paste = 1;
      break;
    case OPT_PASTE_SELECTION:
      if (opt_interactive <= 0) {
	if (opt_interactive == 0)
	  qWarning("%s", qPrintable(QObject::tr("--paste-selection requires interactive mode. Switching.")));
	opt_interactive = 1;
      }
      opt_paste = 2;
      break;
    case OPT_NOEVAL:
      opt_noeval = true;
      break;
    case OPT_BASE64ARG:
      opt_base64arg = true;
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
#if defined(Q_WS_MAC)
      // NASTY WORKAROUND FOR a mysterious -psn_**** option passed to the application
      // when opened using the apple 'open' command-line utility, and thus when the
      // application is launched via an icon..
      if ( !strncmp(arg, "sn_", 3) )
	break;
#endif
      opt_preamble = arg;
      break;
    case OPT_QUIET:
      opt_quiet = __klf_parse_bool_arg(arg, true);
      break;
    case OPT_REDIRECT_DEBUG:
      opt_redirect_debug = arg;
      break;
    case OPT_DAEMONIZE:
      opt_daemonize = true;
      break;
    case OPT_DBUS_EXPORT_MAINWIN:
      opt_dbus_export_mainwin = true;
      break;
    case OPT_SKIP_PLUGINS:
      // default value 'true' (default value if option is given)
      opt_skip_plugins = __klf_parse_bool_arg(arg, true);
      break;
    case OPT_CALCEPSBBOX:
      opt_calcepsbbox = __klf_parse_bool_arg(arg, true);
      break;
    case OPT_NOCALCEPSBBOX:
      opt_calcepsbbox = 0;
      break;
    case OPT_OUTLINEFONTS:
      opt_outlinefonts = __klf_parse_bool_arg(arg, true);
      break;
    case OPT_NOOUTLINEFONTS:
      opt_outlinefonts = 0;
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
    case OPT_BORDEROFFSETS:
      klf_read_borderoffsets(arg);
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
      opt_help_fp = main_msg_get_fp_arg(arg);
      opt_help_requested = true;
      break;
    case OPT_VERSION:
      if (arg != NULL) {
	char *colonptr = strchr(arg, ':');
	if (colonptr != NULL) {
	  *colonptr = '\0';
	  opt_version_format = colonptr+1;
	}
      }
      opt_version_fp = main_msg_get_fp_arg(arg);
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
  
  // Constistency checks
  if (opt_noeval && !opt_interactive) {
    qWarning("%s", qPrintable(QObject::tr("--noeval is relevant only in interactive mode.")));
    opt_noeval = false;
  }
  if (opt_noeval && opt_output) {
    qWarning("%s", qPrintable(QObject::tr("--noeval may not be used when --output is present.")));
    opt_noeval = false;
  }
  if (opt_interactive && opt_format && !opt_output) {
    qWarning("%s", qPrintable(QObject::tr("Ignoring --format without --output.")));
    opt_format = NULL;
  }

  return;
}
