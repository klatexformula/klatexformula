/***************************************************************************
 *   file klfconfig.cpp
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

#include <iostream>

#include <QApplication>
#include <QMessageBox>
#include <QObject>
#include <QDir>
#include <QTextStream>
#include <QFont>
#include <QFontDatabase>
#include <QMap>
#include <QString>
#include <QListView> // icon view flow
#include <QLocale>
#include <QDesktopServices> // "My Documents" or "Documents" directory

#include <klfmainwin.h>

#include <klfutil.h>
#include "klfmain.h"
#include "klfconfig.h"


static const char * __klf_fallback_share_dir =
#if defined(Q_OS_WIN32) || defined(Q_OS_WIN64)  // windows
	"..";   // note: program is in a bin/ directory by default (this is for nsis-installer)
#elif defined(Q_OS_MAC) || defined(Q_OS_DARWIN) // Mac OS X
	"../Resources";
#else  // unix-like system
	"../share/klatexformula";
#endif


static const char * __klf_share_dir =
#ifdef KLF_SHARE_DIR  // defined by the build system
	KLF_SHARE_DIR;
#else
        NULL;
#endif

static QString __klf_share_dir_cached;

KLF_EXPORT QString klf_share_dir_abspath()
{
  if (!__klf_share_dir_cached.isEmpty())
    return __klf_share_dir_cached;

  klfDbg(klfFmtCC("cmake-share-dir=%s; fallback-share-dir=%s\n", __klf_share_dir,
		  __klf_fallback_share_dir)) ;

  QString sharedir;
  if (__klf_share_dir != NULL)
    sharedir = QLatin1String(__klf_share_dir);
  else
    sharedir = QLatin1String(__klf_fallback_share_dir);

  __klf_share_dir_cached = klfPrefixedPath(sharedir); // prefixed by app-dir-path
  klfDbg("share dir is "<<__klf_share_dir_cached) ;
  return __klf_share_dir_cached;
}



// global variable to access our config
// remember to initialize it in main() in main.cpp !
KLFConfig klfconfig;



/*void settings_write_QTextCharFormat(QSettings& s, const QString& basename,
  const QTextCharFormat& charfmt)
  {
  s.setValue(basename+"_charformat", charfmt);
  }
*/
static QTextCharFormat settings_read_QTextCharFormat(QSettings& s, const QString& basename,
						     const QTextCharFormat& dflt)
{
  QVariant val = s.value(basename+"_charformat", dflt);
  QTextFormat tf = val.value<QTextFormat>();
  return tf.toCharFormat();
}
/*
 template<class T>
 void settings_write_list(QSettings& s, const QString& basename, const QList<T>& list)
 {
 QList<QVariant> l;
 int k;
 for (k = 0; k < list.size(); ++k)
 l.append(QVariant(list[k]));
 s.setValue(basename+"_list", l);
 }
*/

template<class T>
static QList<T> settings_read_list(QSettings& s, const QString& basename, const QList<T>& dflt)
{
  QList<QVariant> l = s.value(basename+"_list", QList<QVariant>()).toList();
  if (l.size() == 0)
    return dflt;
  QList<T> list;
  int k;
  for (k = 0; k < l.size(); ++k)
    list.append(l[k].value<T>());
  return list;
}


// -----------------------------------------------------


KLFConfig::KLFConfig()
{
}


#define KLFCONFIG_TEST_FIXED_FONT(found_fcode, fdb, fcode, f, fps)	\
  if (!found_fcode && fdb.isFixedPitch(f)) {				\
    fcode = QFont(f, fps);						\
    found_fcode = true;							\
  }

void KLFConfig::loadDefaults()
{
  KLF_DEBUG_TIME_BLOCK(KLF_FUNC_NAME) ;

  homeConfigDir = QDir::homePath() + "/.klatexformula";
  globalShareDir = klf_share_dir_abspath();
  homeConfigSettingsFile = homeConfigDir + "/klatexformula.conf";
  homeConfigSettingsFileIni = homeConfigDir + "/config";
  homeConfigDirRCCResources = homeConfigDir + "/rccresources";
  homeConfigDirPlugins = homeConfigDir + "/plugins";
  homeConfigDirPluginData = homeConfigDir + "/plugindata";
  homeConfigDirI18n = homeConfigDir + "/i18n";

  //debug: QMessageBox::information(0, "", QString("global share dir=")+globalShareDir);

  if (qApp->inherits("QApplication")) { // and not QCoreApplication...

    QFontDatabase fdb;
    QFont f = QApplication::font();

#ifdef Q_WS_X11
    // setting pixel size avoids bug with Qt/X11 of fonts having their metrics badly calculated (...?)
    f.setPixelSize(15);
#endif

    defaultStdFont = f;

    QFont cmuappfont = f;
    if (fdb.families().contains("CMU Sans Serif")) {
      // CMU Sans Serif is available ;-)
      int ps = QFontInfo(f).pointSize();
      cmuappfont = QFont("CMU Sans Serif", ps);
      // ideal height of the string "MX" in pixels. This value was CAREFULLY ADJUSTED.
      // please change it only if you feel sure. (fonts have to look nice on most platforms)
      int fIdealHeight = 15;
      // start with a little bit smaller font
      ps -= 2;
      const int cutoff = 16; //just a cutoff to be sure
      // and increase font size up to something "ideal"
      while (ps < cutoff && QFontMetrics(cmuappfont).size(Qt::TextSingleLine, "MX").height() < fIdealHeight) {
	cmuappfont.setPointSize(++ps);
      }
      if (ps >= cutoff)
	ps = 10; // the default
    }

    QFont fcode;
    bool found_fcode = false;
    int ps = 11;
    KLFCONFIG_TEST_FIXED_FONT(found_fcode, fdb, fcode, "Courier 10 Pitch", ps);
    KLFCONFIG_TEST_FIXED_FONT(found_fcode, fdb, fcode, "ETL Fixed", ps);
    KLFCONFIG_TEST_FIXED_FONT(found_fcode, fdb, fcode, "Courier New", ps);
    KLFCONFIG_TEST_FIXED_FONT(found_fcode, fdb, fcode, "Efont Fixed", ps);
    KLFCONFIG_TEST_FIXED_FONT(found_fcode, fdb, fcode, "Adobe Courier", ps);
    KLFCONFIG_TEST_FIXED_FONT(found_fcode, fdb, fcode, "Courier", ps);
    KLFCONFIG_TEST_FIXED_FONT(found_fcode, fdb, fcode, "Misc Fixed", ps);
    KLFCONFIG_TEST_FIXED_FONT(found_fcode, fdb, fcode, "Monospace", ps);
    if ( ! found_fcode )
      fcode = f;
    // guess good font size for code font
    QFont fp1 = f, fp2 = f;
    qreal ps1 = 5, ps2 = 20; // measurement points
    int idealHeight = 18; // the ideal height of the string "MX" in pixels
    fp1.setPointSize((int)(ps1+0.5f));
    fp2.setPointSize((int)(ps2+0.5f));
    qreal h1 = QFontMetrics(fp1).size(Qt::TextSingleLine, "MX").height();
    qreal h2 = QFontMetrics(fp2).size(Qt::TextSingleLine, "MX").height();
    // linear interpolation for a height of \c idealHeight pixels
    ps = (int)(ps1 + (idealHeight-h1)*(ps2-ps1)/(h2-h1));
    // and set that font size
    fcode.setPointSize(ps);
    QFont fcodeMain = fcode;
    fcodeMain.setPointSize(ps+1);

    // by default, this is first run!
    Core.thisVersionMajFirstRun = true;
    Core.thisVersionMajMinFirstRun = true;
    Core.thisVersionMajMinRelFirstRun = true;
    Core.thisVersionExactFirstRun = true;

    Core.libraryFileName = "library.klf.db";
    Core.libraryLibScheme = "klf+sqlite";

    defaultCMUFont = cmuappfont;
    defaultTTFont = fcode;

    UI.locale = QLocale::system().name();
    klfDbg("System locale: "<<QLocale::system().name());
#ifdef KLF_NO_CMU_FONT
    UI.applicationFont = defaultStdFont;
#else
    UI.applicationFont = cmuappfont;
#endif
    UI.latexEditFont = fcodeMain;
    UI.preambleEditFont = fcode;
    UI.previewTooltipMaxSize = QSize(800, 600);
    UI.labelOutputFixedSize = QSize(280, 80);
    UI.lastSaveDir = QDir::homePath();
    UI.symbolsPerLine = 6;
    UI.userColorList = QList<QColor>();
    UI.userColorList.append(QColor(0,0,0));
    UI.userColorList.append(QColor(255,255,255));
    UI.userColorList.append(QColor(170,0,0));
    UI.userColorList.append(QColor(0,0,128));
    UI.userColorList.append(QColor(0,0,255));
    UI.userColorList.append(QColor(0,85,0));
    UI.userColorList.append(QColor(255,85,0));
    UI.userColorList.append(QColor(0,255,255));
    UI.userColorList.append(QColor(85,0,127));
    UI.userColorList.append(QColor(128,255,255));
    UI.colorChooseWidgetRecent = QList<QColor>();
    UI.colorChooseWidgetCustom = QList<QColor>();
    UI.maxUserColors = 12;
    UI.enableToolTipPreview = false;
    UI.enableRealTimePreview = true;
    UI.autosaveLibraryMin = 5;
    UI.showHintPopups = true;
    UI.clearLatexOnly = false;
    UI.copyExportProfile = "default";
    UI.dragExportProfile = "default";
    UI.glowEffect = false;
    UI.glowEffectColor = QColor(128, 255, 128, 12);
    UI.glowEffectRadius = 4;
    UI.customMathModes = QStringList();

    SyntaxHighlighter.configFlags = 0x05;
    SyntaxHighlighter.fmtKeyword = QTextCharFormat();
    SyntaxHighlighter.fmtKeyword.setForeground(QColor(0, 0, 128));
    SyntaxHighlighter.fmtComment = QTextCharFormat();
    SyntaxHighlighter.fmtComment.setForeground(QColor(180, 0, 0));
    SyntaxHighlighter.fmtComment.setFontItalic(true);
    SyntaxHighlighter.fmtParenMatch = QTextCharFormat();
    SyntaxHighlighter.fmtParenMatch.setBackground(QColor(180, 238, 180));
    SyntaxHighlighter.fmtParenMismatch = QTextCharFormat();
    SyntaxHighlighter.fmtParenMismatch.setBackground(QColor(255, 20, 147));
    SyntaxHighlighter.fmtLonelyParen = QTextCharFormat();
    SyntaxHighlighter.fmtLonelyParen.setForeground(QColor(255, 0, 255));
    SyntaxHighlighter.fmtLonelyParen.setFontWeight(QFont::Bold);
  }

  // invalid value, by convention ".". if the config is not read from file, then settings will
  // be detected in detectMissingSettings()
  BackendSettings.tempDir = ".";
  BackendSettings.execLatex = ".";
  BackendSettings.execDvips = ".";
  BackendSettings.execGs = ".";
  BackendSettings.execEpstopdf = ".";
  BackendSettings.execenv = QStringList();

  BackendSettings.lborderoffset = 0;
  BackendSettings.tborderoffset = 0;
  BackendSettings.rborderoffset = 0;
  BackendSettings.bborderoffset = 0;
  BackendSettings.outlineFonts = true;

  LibraryBrowser.colorFound = QColor(128, 255, 128);
  LibraryBrowser.colorNotFound = QColor(255, 128, 128);
  LibraryBrowser.restoreURLs = false;
  LibraryBrowser.confirmClose = true;
  LibraryBrowser.groupSubCategories = true;
  LibraryBrowser.iconViewFlow = QListView::TopToBottom;
  LibraryBrowser.historyTagCopyToArchive = true;
  LibraryBrowser.lastFileDialogPath = // "My Documents":
    QDesktopServices::storageLocation(QDesktopServices::DocumentsLocation);
  LibraryBrowser.treePreviewSizePercent = 75;
  LibraryBrowser.listPreviewSizePercent = 75;
  LibraryBrowser.iconPreviewSizePercent = 100;

  Plugins.pluginConfig = QMap< QString, QMap<QString,QVariant> >();
}

void KLFConfig::detectMissingSettings()
{
  int neededsettings = 
    !BackendSettings.tempDir.compare(".")      << 0 |
    !BackendSettings.execLatex.compare(".")    << 1 |
    !BackendSettings.execDvips.compare(".")    << 2 |
    !BackendSettings.execGs.compare(".")       << 3 |
    !BackendSettings.execEpstopdf.compare(".") << 4 ;

  if (neededsettings) {
    KLFBackend::klfSettings defaultsettings;
    KLFBackend::detectSettings(&defaultsettings);
    if (neededsettings & (1<<0))
      BackendSettings.tempDir = defaultsettings.tempdir;
    if (neededsettings & (1<<1))
      BackendSettings.execLatex = defaultsettings.latexexec;
    if (neededsettings & (1<<2))
      BackendSettings.execDvips = defaultsettings.dvipsexec;
    if (neededsettings & (1<<3))
      BackendSettings.execGs = defaultsettings.gsexec;
    if (neededsettings & (1<<4))
      BackendSettings.execEpstopdf = defaultsettings.epstopdfexec;
    BackendSettings.execenv << defaultsettings.execenv;
  }

}



int KLFConfig::ensureHomeConfigDir()
{
  if ( !klfEnsureDir(homeConfigDir) )
    return -1;
  if ( !klfEnsureDir(homeConfigDirRCCResources) )
    return -1;
  if ( !klfEnsureDir(homeConfigDirPlugins) )
    return -1;
  if ( !klfEnsureDir(homeConfigDirPluginData) )
    return -1;
  if ( !klfEnsureDir(homeConfigDirI18n) )
    return -1;

  return 0;
}


bool KLFConfig::checkExePaths()
{
  return QFileInfo(BackendSettings.execLatex).isExecutable() &&
    QFileInfo(BackendSettings.execDvips).isExecutable() &&
    QFileInfo(BackendSettings.execGs).isExecutable() &&
    ( BackendSettings.execEpstopdf.isEmpty() ||
      QFileInfo(BackendSettings.execEpstopdf).isExecutable()) ;
}




int KLFConfig::readFromConfig()
{
  klfDbgT(" reading config") ;

  ensureHomeConfigDir();

  if (QFile::exists(homeConfigSettingsFile)) {
    return readFromConfig_v2();
  }
  if (QFile::exists(homeConfigSettingsFileIni)) {
    return readFromConfig_v1();
  }
  return -1;
}

template<class T>
static void klf_config_read(QSettings &s, const QString& baseName, T *target,
			      const char * listOrMapType = NULL)
{
  //  qDebug("klf_config_read<...>(%s)", qPrintable(baseName));
  QVariant defVal = QVariant::fromValue<T>(*target);
  QVariant valstrv = s.value(baseName, QVariant());
  //  klfDbg( "\tRead value "<<valstr ) ;
  if (valstrv.isNull()) {
    // no such entry in config
    klfDbg("No entry "<<baseName<<" in config.") ;
    return;
  }
  QString valstr = valstrv.toString();
  QVariant val = klfLoadVariantFromText(valstr.toLatin1(), defVal.typeName(), listOrMapType);
  if (val.isValid())
    *target = val.value<T>();
}
template<>
void klf_config_read<QTextCharFormat>(QSettings &s, const QString& baseName,
					QTextCharFormat *target,
					const char * listOrMapType)
{
  qDebug("klf_config_read<QTextCharFormat>(%s)", qPrintable(baseName));
  QTextFormat fmt = *target;
  klf_config_read(s, baseName, &fmt);
  *target = fmt.toCharFormat();
}

template<class T>
static void klf_config_read_list(QSettings &s, const QString& baseName, QList<T> *target)
{
  QVariantList vlist = klfListToVariantList(*target);
  klf_config_read(s, baseName, &vlist, QVariant::fromValue<T>(T()).typeName());
  *target = klfVariantListToList<T>(vlist);
}


template<class T>
static void klf_config_write(QSettings &s, const QString& baseName, const T * value)
{
  QVariant val = QVariant::fromValue<T>(*value);
  QByteArray datastr = klfSaveVariantToText(val);
  s.setValue(baseName, QVariant::fromValue<QString>(QString::fromLocal8Bit(datastr)));
}

template<class T>
static void klf_config_write_list(QSettings &s, const QString& baseName, const QList<T> * target)
{
  QVariantList vlist = klfListToVariantList(*target);
  klf_config_write(s, baseName, &vlist);
}

static QString firstRunConfigKey(int N)
{
  QString s = QString("versionFirstRun-%1_").arg(N);
  if (N >= 4)
    return s + QLatin1String(KLF_VERSION_STRING);

  if (N-- > 0)
    s += QString("%1").arg(KLF_VERSION_MAJ);
  if (N-- > 0)
    s += QString(".%1").arg(KLF_VERSION_MIN);
  if (N-- > 0)
    s += QString(".%1").arg(KLF_VERSION_REL);
  return s;
}

int KLFConfig::readFromConfig_v2()
{
  KLF_DEBUG_TIME_BLOCK(KLF_FUNC_NAME) ;

  QSettings s(homeConfigSettingsFile, QSettings::IniFormat);

  qDebug("Reading base configuration");

  s.beginGroup("Core");
  klf_config_read(s, firstRunConfigKey(1), &Core.thisVersionMajFirstRun);
  klf_config_read(s, firstRunConfigKey(2), &Core.thisVersionMajMinFirstRun);
  klf_config_read(s, firstRunConfigKey(3), &Core.thisVersionMajMinRelFirstRun);
  klf_config_read(s, firstRunConfigKey(4), &Core.thisVersionExactFirstRun);
  klf_config_read(s, "libraryfilename", &Core.libraryFileName);
  klf_config_read(s, "librarylibscheme", &Core.libraryLibScheme);
  s.endGroup();

  s.beginGroup("UI");
  klf_config_read(s, "locale", &UI.locale);
  klf_config_read(s, "applicationfont", &UI.applicationFont);
  klf_config_read(s, "latexeditfont", &UI.latexEditFont);
  klf_config_read(s, "preambleeditfont", &UI.preambleEditFont);
  klf_config_read(s, "previewtooltipmaxsize", &UI.previewTooltipMaxSize);
  klf_config_read(s, "lbloutputfixedsize", &UI.labelOutputFixedSize);
  klf_config_read(s, "lastsavedir", &UI.lastSaveDir);
  klf_config_read(s, "symbolsperline", &UI.symbolsPerLine);
  klf_config_read_list(s, "usercolorlist", &UI.userColorList);
  klf_config_read_list(s, "colorchoosewidgetrecent", &UI.colorChooseWidgetRecent);
  klf_config_read_list(s, "colorchoosewidgetcustom", &UI.colorChooseWidgetCustom);
  klf_config_read(s, "maxusercolors", &UI.maxUserColors);
  klf_config_read(s, "enabletooltippreview", &UI.enableToolTipPreview);
  klf_config_read(s, "enablerealtimepreview", &UI.enableRealTimePreview);
  klf_config_read(s, "autosavelibrarymin", &UI.autosaveLibraryMin);
  klf_config_read(s, "showhintpopups", &UI.showHintPopups);
  klf_config_read(s, "clearlatexonly", &UI.clearLatexOnly);
  klf_config_read(s, "copyexportprofile", &UI.copyExportProfile);
  klf_config_read(s, "dragexportprofile", &UI.dragExportProfile);
  klf_config_read(s, "gloweffect", &UI.glowEffect);
  klf_config_read(s, "gloweffectcolor", &UI.glowEffectColor);
  klfDbg("Read glow effect color from config: color="<<UI.glowEffectColor
	 <<", alpha="<<UI.glowEffectColor.alpha());
  klf_config_read(s, "gloweffectradius", &UI.glowEffectRadius);
  klf_config_read(s, "custommathmodes", &UI.customMathModes);
  s.endGroup();

  s.beginGroup("SyntaxHighlighter");
  klf_config_read(s, "configflags", &SyntaxHighlighter.configFlags);
  klf_config_read<QTextCharFormat>(s, "keyword", &SyntaxHighlighter.fmtKeyword);
  klf_config_read<QTextCharFormat>(s, "comment", &SyntaxHighlighter.fmtComment);
  klf_config_read<QTextCharFormat>(s, "parenmatch", &SyntaxHighlighter.fmtParenMatch);
  klf_config_read<QTextCharFormat>(s, "parenmismatch", &SyntaxHighlighter.fmtParenMismatch);
  klf_config_read<QTextCharFormat>(s, "lonelyparen", &SyntaxHighlighter.fmtLonelyParen);
  s.endGroup();

  s.beginGroup("BackendSettings");
  klf_config_read(s, "tempdir", &BackendSettings.tempDir);
  klf_config_read(s, "latexexec", &BackendSettings.execLatex);
  klf_config_read(s, "dvipsexec", &BackendSettings.execDvips);
  klf_config_read(s, "gsexec", &BackendSettings.execGs);
  klf_config_read(s, "epstopdfexec", &BackendSettings.execEpstopdf);
  klf_config_read(s, "execenv", &BackendSettings.execenv);
  klf_config_read(s, "lborderoffset", &BackendSettings.lborderoffset);
  klf_config_read(s, "tborderoffset", &BackendSettings.tborderoffset);
  klf_config_read(s, "rborderoffset", &BackendSettings.rborderoffset);
  klf_config_read(s, "bborderoffset", &BackendSettings.bborderoffset);
  klf_config_read(s, "outlinefonts", &BackendSettings.outlineFonts);
  s.endGroup();

  s.beginGroup("LibraryBrowser");
  klf_config_read(s, "colorfound", &LibraryBrowser.colorFound);
  klf_config_read(s, "colornotfound", &LibraryBrowser.colorNotFound);
  klf_config_read(s, "restoreurls", &LibraryBrowser.restoreURLs);
  klf_config_read(s, "confirmclose", &LibraryBrowser.confirmClose);
  klf_config_read(s, "groupsubcategories", &LibraryBrowser.groupSubCategories);
  klf_config_read(s, "iconviewflow", &LibraryBrowser.iconViewFlow);
  klf_config_read(s, "historytagcopytoarchive", &LibraryBrowser.historyTagCopyToArchive);
  klf_config_read(s, "lastfiledialogpath", &LibraryBrowser.lastFileDialogPath);
  klf_config_read(s, "treepreviewsizepercent", &LibraryBrowser.treePreviewSizePercent);
  klf_config_read(s, "listpreviewsizepercent", &LibraryBrowser.listPreviewSizePercent);
  klf_config_read(s, "iconpreviewsizepercent", &LibraryBrowser.iconPreviewSizePercent);
  s.endGroup();

  // Special treatment for Plugins.pluginConfig
  // for reading, we cannot rely on klf_plugins since we are called before plugins are loaded!
  int k, j;
  QDir plugindatadir = QDir(homeConfigDirPluginData);
  QStringList plugindirs = plugindatadir.entryList(QDir::Dirs);
  for (k = 0; k < plugindirs.size(); ++k) {
    if (plugindirs[k] == "." || plugindirs[k] == "..")
      continue;
    qDebug("Reading config for plugin %s", qPrintable(plugindirs[k]));
    QString fn = plugindatadir.absoluteFilePath(plugindirs[k])+"/"+plugindirs[k]+".conf";
    if ( ! QFile::exists(fn) ) {
      qDebug("\tskipping plugin %s since the file %s does not exist.",
	     qPrintable(plugindirs[k]), qPrintable(fn));
      continue;
    }
    QSettings psettings(fn, QSettings::IniFormat);
    QVariantMap pconfmap;
    QStringList keys = psettings.allKeys();
    for (j = 0; j < keys.size(); ++j) {
      pconfmap[keys[j]] = psettings.value(keys[j]);
    }
    Plugins.pluginConfig[plugindirs[k]] = pconfmap;
  }

  // POST-CONFIG-READ SETUP

  // forbid empty locale
  if (klfconfig.UI.locale.isEmpty())
    klfconfig.UI.locale = "en_US";
  // set Qt default locale to ours
  QLocale::setDefault(klfconfig.UI.locale);

  return 0;
}


int KLFConfig::writeToConfig()
{
  ensureHomeConfigDir();
  QSettings s(homeConfigSettingsFile, QSettings::IniFormat);

  bool thisVersionFirstRunFalse = false;

  s.beginGroup("Core");
  klf_config_write(s, firstRunConfigKey(1), &thisVersionFirstRunFalse);
  klf_config_write(s, firstRunConfigKey(2), &thisVersionFirstRunFalse);
  klf_config_write(s, firstRunConfigKey(3), &thisVersionFirstRunFalse);
  klf_config_write(s, firstRunConfigKey(4), &thisVersionFirstRunFalse);
  klf_config_write(s, "libraryfilename", &Core.libraryFileName);
  klf_config_write(s, "librarylibscheme", &Core.libraryLibScheme);
  s.endGroup();

  s.beginGroup("UI");
  klf_config_write(s, "locale", &UI.locale);
  klf_config_write(s, "applicationfont", &UI.applicationFont);
  klf_config_write(s, "latexeditfont", &UI.latexEditFont);
  klf_config_write(s, "preambleeditfont", &UI.preambleEditFont);
  klf_config_write(s, "previewtooltipmaxsize", &UI.previewTooltipMaxSize);
  klf_config_write(s, "lbloutputfixedsize", &UI.labelOutputFixedSize);
  klf_config_write(s, "lastsavedir", &UI.lastSaveDir);
  klf_config_write(s, "symbolsperline", &UI.symbolsPerLine);
  klf_config_write_list(s, "usercolorlist", &UI.userColorList);
  klf_config_write_list(s, "colorchoosewidgetrecent", &UI.colorChooseWidgetRecent);
  klf_config_write_list(s, "colorchoosewidgetcustom", &UI.colorChooseWidgetCustom);
  klf_config_write(s, "maxusercolors", &UI.maxUserColors);
  klf_config_write(s, "enabletooltippreview", &UI.enableToolTipPreview);
  klf_config_write(s, "enablerealtimepreview", &UI.enableRealTimePreview);
  klf_config_write(s, "autosavelibrarymin", &UI.autosaveLibraryMin);
  klf_config_write(s, "showhintpopups", &UI.showHintPopups);
  klf_config_write(s, "clearlatexonly", &UI.clearLatexOnly);
  klf_config_write(s, "copyexportprofile", &UI.copyExportProfile);
  klf_config_write(s, "dragexportprofile", &UI.dragExportProfile);
  klf_config_write(s, "gloweffect", &UI.glowEffect);
  klf_config_write(s, "gloweffectcolor", &UI.glowEffectColor);
  klf_config_write(s, "gloweffectradius", &UI.glowEffectRadius);
  klf_config_write(s, "custommathmodes", &UI.customMathModes);
  s.endGroup();

  s.beginGroup("SyntaxHighlighter");
  klf_config_write(s, "configflags", &SyntaxHighlighter.configFlags);
  klf_config_write<QTextFormat>(s, "keyword", &SyntaxHighlighter.fmtKeyword);
  klf_config_write<QTextFormat>(s, "comment", &SyntaxHighlighter.fmtComment);
  klf_config_write<QTextFormat>(s, "parenmatch", &SyntaxHighlighter.fmtParenMatch);
  klf_config_write<QTextFormat>(s, "parenmismatch", &SyntaxHighlighter.fmtParenMismatch);
  klf_config_write<QTextFormat>(s, "lonelyparen", &SyntaxHighlighter.fmtLonelyParen);
  s.endGroup();

  s.beginGroup("BackendSettings");
  klf_config_write(s, "tempdir", &BackendSettings.tempDir);
  klf_config_write(s, "latexexec", &BackendSettings.execLatex);
  klf_config_write(s, "dvipsexec", &BackendSettings.execDvips);
  klf_config_write(s, "gsexec", &BackendSettings.execGs);
  klf_config_write(s, "epstopdfexec", &BackendSettings.execEpstopdf);
  klf_config_write(s, "execenv", &BackendSettings.execenv);
  klf_config_write(s, "lborderoffset", &BackendSettings.lborderoffset);
  klf_config_write(s, "tborderoffset", &BackendSettings.tborderoffset);
  klf_config_write(s, "rborderoffset", &BackendSettings.rborderoffset);
  klf_config_write(s, "bborderoffset", &BackendSettings.bborderoffset); 
  klf_config_write(s, "outlinefonts", &BackendSettings.outlineFonts);
  s.endGroup();

  s.beginGroup("LibraryBrowser");
  klf_config_write(s, "colorfound", &LibraryBrowser.colorFound);
  klf_config_write(s, "colornotfound", &LibraryBrowser.colorNotFound);
  klf_config_write(s, "restoreurls", &LibraryBrowser.restoreURLs);
  klf_config_write(s, "confirmclose", &LibraryBrowser.confirmClose);
  klf_config_write(s, "groupsubcategories", &LibraryBrowser.groupSubCategories);
  klf_config_write(s, "iconviewflow", &LibraryBrowser.iconViewFlow);
  klf_config_write(s, "historytagcopytoarchive", &LibraryBrowser.historyTagCopyToArchive);
  klf_config_write(s, "lastfiledialogpath", &LibraryBrowser.lastFileDialogPath);
  klf_config_write(s, "treepreviewsizepercent", &LibraryBrowser.treePreviewSizePercent);
  klf_config_write(s, "listpreviewsizepercent", &LibraryBrowser.listPreviewSizePercent);
  klf_config_write(s, "iconpreviewsizepercent", &LibraryBrowser.iconPreviewSizePercent);
  s.endGroup();

  // Special treatment for Plugins.pluginConfig
  int k;
  for (k = 0; k < klf_plugins.size(); ++k) {
    QString fn = homeConfigDirPluginData+"/"+klf_plugins[k].name+"/"+klf_plugins[k].name+".conf";
    QSettings psettings(fn, QSettings::IniFormat);
    QVariantMap pconfmap = Plugins.pluginConfig[klf_plugins[k].name];
    QVariantMap::const_iterator it;
    for (it = pconfmap.begin(); it != pconfmap.end(); ++it) {
      psettings.setValue(it.key(), it.value());
    }
    psettings.sync();
  }

  s.sync();
  return 0;
}




KLFPluginConfigAccess KLFConfig::getPluginConfigAccess(const QString& name)
{
  return KLFPluginConfigAccess(this, name);
}


// --------------------------------------




KLFPluginConfigAccess::KLFPluginConfigAccess()
{
  _config = NULL;
  _pluginname = QString::null;
}
KLFPluginConfigAccess::KLFPluginConfigAccess(const KLFPluginConfigAccess& other)
  : _config(other._config), _pluginname(other._pluginname)
{
}
KLFPluginConfigAccess::~KLFPluginConfigAccess()
{
}

KLFPluginConfigAccess::KLFPluginConfigAccess(KLFConfig *configObject, const QString& pluginName)
{
  _config = configObject;
  _pluginname = pluginName;
}



QString KLFPluginConfigAccess::homeConfigDir() const
{
  if ( _config == NULL ) {
    qWarning("KLFPluginConfigAccess::homeConfigDir: Invalid Config Pointer!\n");
    return QString();
  }

  return _config->homeConfigDir;
}

QString KLFPluginConfigAccess::globalShareDir() const
{
  if ( _config == NULL ) {
    qWarning("KLFPluginConfigAccess::homeConfigDir: Invalid Config Pointer!\n");
    return QString();
  }

  return _config->globalShareDir;
}

QString KLFPluginConfigAccess::tempDir() const
{
  if ( _config == NULL ) {
    qWarning("KLFPluginConfigAccess::tempDir: Invalid Config Pointer!\n");
    return QString();
  }

  return _config->BackendSettings.tempDir;
}

QString KLFPluginConfigAccess::homeConfigPluginDataDir(bool createIfNeeded) const
{
  if ( _config == NULL ) {
    qWarning("KLFPluginConfigAccess::homeConfigPluginDataDir: Invalid Config Pointer!\n");
    return QString();
  }

  QString d = _config->homeConfigDirPluginData + "/" + _pluginname;
  if ( createIfNeeded && ! klfEnsureDir(d) ) {
    qWarning("KLFPluginConfigAccess::homeConfigPluginDataDir: Can't create directory: `%s'",
	     qPrintable(d));
    return QString();
  }
  return d;
}

QVariant KLFPluginConfigAccess::readValue(const QString& key) const
{
  if ( _config == NULL ) {
    qWarning("KLFPluginConfigAccess::readValue: Invalid Config Pointer!\n");
    return QVariant();
  }

  if ( ! _config->Plugins.pluginConfig[_pluginname].contains(key) )
    return QVariant();

  return _config->Plugins.pluginConfig[_pluginname][key];
}

QVariant KLFPluginConfigAccess::makeDefaultValue(const QString& key, const QVariant& defaultValue)
{
  if ( _config == NULL ) {
    qWarning("KLFPluginConfigAccess::makeDefaultValue: Invalid Config Pointer!\n");
    return QVariant();
  }

  if (_config->Plugins.pluginConfig[_pluginname].contains(key))
    return _config->Plugins.pluginConfig[_pluginname][key];

  // assign the value into the plugin config, and return it
  return ( _config->Plugins.pluginConfig[_pluginname][key] = defaultValue );
}
void KLFPluginConfigAccess::writeValue(const QString& key, const QVariant& value)
{
  if ( _config == NULL ) {
    qWarning("KLFPluginConfigAccess::writeValue: Invalid Config Pointer!\n");
    return;
  }

  _config->Plugins.pluginConfig[_pluginname][key] = value;
}








// ----------------------









// KEPT FOR COMPATIBILITY WITH OLDER VERSIONS

int KLFConfig::readFromConfig_v1()
{
  QSettings s(homeConfigSettingsFileIni, QSettings::IniFormat);

  s.beginGroup("UI");
  UI.locale = s.value("locale", UI.locale).toString();
  // ingnore KLF 3.1 app font setting, we have our nice CMU Sans Serif font ;-)
  //  UI.applicationFont = s.value("applicationfont", UI.applicationFont).value<QFont>();
  UI.latexEditFont = s.value("latexeditfont", UI.latexEditFont).value<QFont>();
  UI.preambleEditFont = s.value("preambleeditfont", UI.preambleEditFont).value<QFont>();
  UI.previewTooltipMaxSize = s.value("previewtooltipmaxsize", UI.previewTooltipMaxSize).toSize();
  UI.labelOutputFixedSize = s.value("lbloutputfixedsize", UI.labelOutputFixedSize ).toSize();
  UI.lastSaveDir = s.value("lastsavedir", UI.lastSaveDir).toString();
  UI.symbolsPerLine = s.value("symbolsperline", UI.symbolsPerLine).toInt();
  UI.userColorList = settings_read_list(s, "usercolorlist", UI.userColorList);
  UI.colorChooseWidgetRecent = settings_read_list(s, "colorchoosewidgetrecent", UI.colorChooseWidgetRecent);
  UI.colorChooseWidgetCustom = settings_read_list(s, "colorchoosewidgetcustom", UI.colorChooseWidgetCustom);
  UI.maxUserColors = s.value("maxusercolors", UI.maxUserColors).toInt();
  UI.enableToolTipPreview = s.value("enabletooltippreview", UI.enableToolTipPreview).toBool();
  UI.enableRealTimePreview = s.value("enablerealtimepreview", UI.enableRealTimePreview).toBool();
  UI.autosaveLibraryMin = s.value("autosavelibrarymin", UI.autosaveLibraryMin).toInt();
  s.endGroup();

  s.beginGroup("SyntaxHighlighter");
  SyntaxHighlighter.configFlags = s.value("configflags", SyntaxHighlighter.configFlags).toUInt();
  SyntaxHighlighter.fmtKeyword = settings_read_QTextCharFormat(s, "keyword", SyntaxHighlighter.fmtKeyword);
  SyntaxHighlighter.fmtComment = settings_read_QTextCharFormat(s, "comment", SyntaxHighlighter.fmtComment);
  SyntaxHighlighter.fmtParenMatch = settings_read_QTextCharFormat(s, "parenmatch", SyntaxHighlighter.fmtParenMatch);
  SyntaxHighlighter.fmtParenMismatch = settings_read_QTextCharFormat(s, "parenmismatch", SyntaxHighlighter.fmtParenMismatch);
  SyntaxHighlighter.fmtLonelyParen = settings_read_QTextCharFormat(s, "lonelyparen", SyntaxHighlighter.fmtLonelyParen);
  s.endGroup();

  s.beginGroup("BackendSettings");
  BackendSettings.tempDir = s.value("tempdir", BackendSettings.tempDir).toString();
  BackendSettings.execLatex = s.value("latexexec", BackendSettings.execLatex).toString();
  BackendSettings.execDvips = s.value("dvipsexec", BackendSettings.execDvips).toString();
  BackendSettings.execGs = s.value("gsexec", BackendSettings.execGs).toString();
  BackendSettings.execEpstopdf = s.value("epstopdfexec", BackendSettings.execEpstopdf).toString();
  BackendSettings.lborderoffset = s.value("lborderoffset", BackendSettings.lborderoffset).toInt();
  BackendSettings.tborderoffset = s.value("tborderoffset", BackendSettings.tborderoffset).toInt();
  BackendSettings.rborderoffset = s.value("rborderoffset", BackendSettings.rborderoffset).toInt();
  BackendSettings.bborderoffset = s.value("bborderoffset", BackendSettings.bborderoffset).toInt();
  s.endGroup();

  s.beginGroup("LibraryBrowser");
  LibraryBrowser.colorFound = s.value("colorfound", LibraryBrowser.colorFound).value<QColor>();
  LibraryBrowser.colorNotFound = s.value("colornotfound", LibraryBrowser.colorNotFound).value<QColor>();
  s.endGroup();

  // Special treatment for Plugins.pluginConfig
  s.beginGroup("Plugins/Config");
  QStringList pluginList = s.childGroups();
  s.endGroup();
  int j;
  for (j = 0; j < pluginList.size(); ++j) {
    QString name = pluginList[j];
    s.beginGroup( QString("Plugins/Config/%1").arg(name) );
    QMap<QString,QVariant> thispluginconfig;
    QStringList plconfkeys = s.childKeys();
    int k;
    for (k = 0; k < plconfkeys.size(); ++k) {
      thispluginconfig[plconfkeys[k]] = s.value(plconfkeys[k]);
    }
    klfconfig.Plugins.pluginConfig[name] = thispluginconfig;
    s.endGroup();
  }

  return 0;
}

