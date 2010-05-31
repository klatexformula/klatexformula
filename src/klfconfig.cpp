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
#include <QLocale>

#include <klfmainwin.h>

#include "klfutil.h"
#include "klfmain.h"
#include "klfconfig.h"

// global variable to access our config
// remember to initialize it in main() in main.cpp !
KLFConfig klfconfig;


void settings_write_QTextCharFormat(QSettings& s, const QString& basename, const QTextCharFormat& charfmt)
{
  s.setValue(basename+"_charformat", charfmt);
}
QTextCharFormat settings_read_QTextCharFormat(QSettings& s, const QString& basename,
					      const QTextCharFormat& dflt)
{
  QVariant val = s.value(basename+"_charformat", dflt);
  QTextFormat tf = val.value<QTextFormat>();
  return tf.toCharFormat();
}

template<class T>
void settings_write_list(QSettings& s, const QString& basename, const QList<T>& list)
{
  QList<QVariant> l;
  int k;
  for (k = 0; k < list.size(); ++k)
    l.append(QVariant(list[k]));
  s.setValue(basename+"_list", l);
}

template<class T>
QList<T> settings_read_list(QSettings& s, const QString& basename, const QList<T>& dflt)
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
  homeConfigDir = QDir::homePath() + "/.klatexformula";
  homeConfigSettingsFile = homeConfigDir + "/klatexformula.conf";
  homeConfigSettingsFileIni = homeConfigDir + "/config";
  homeConfigDirRCCResources = homeConfigDir + "/rccresources";
  homeConfigDirPlugins = homeConfigDir + "/plugins";
  homeConfigDirPluginData = homeConfigDir + "/plugindata";
  homeConfigDirI18n = homeConfigDir + "/i18n";

  if (qApp->inherits("QApplication")) { // and not QCoreApplication...
    QFont f = QApplication::font();
#ifdef Q_WS_X11
    // setting pixel size avoids X11 bug of fonts having their metrics badly calculated
    f.setPixelSize(15);
#endif

    QFontDatabase fdb;
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
    qreal ps1 = 10, ps2 = 20; // measurement
    int idealHeight = 7;
    fp1.setPointSize((int)(ps1+0.5f));
    fp2.setPointSize((int)(ps2+0.5f));
    qreal h1 = QFontMetrics(fp1).xHeight();
    qreal h2 = QFontMetrics(fp2).xHeight();
    // linear interpolation for a height of \c idealHeight pixels
    ps = (int)(ps1 + (idealHeight-h1)*(ps2-ps1)/(h2-h1));
    // and set that font size
    fcode.setPointSize(ps);

    UI.locale = QLocale::system().name();
    UI.applicationFont = f;
    UI.latexEditFont = fcode;
    UI.preambleEditFont = fcode;
    UI.previewTooltipMaxSize = QSize(800, 600);
    UI.labelOutputFixedSize = QSize(280, 90);
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

  loadDefaultBackendPaths(this);
  BackendSettings.lborderoffset = 1;
  BackendSettings.tborderoffset = 1;
  BackendSettings.rborderoffset = 1;
  BackendSettings.bborderoffset = 1;

  LibraryBrowser.displayTaggedOnly = false;
  LibraryBrowser.displayNoDuplicates = false;
  LibraryBrowser.colorFound = QColor(128, 255, 128);
  LibraryBrowser.colorNotFound = QColor(255, 128, 128);

  Plugins.pluginConfig = QMap< QString, QMap<QString,QVariant> >();
}



#if defined(Q_OS_WIN32) || defined(Q_OS_WIN64)
#  define PROG_LATEX "latex.exe"
#  define PROG_DVIPS "dvips.exe"
#  define PROG_GS "gswin32c.exe"
#  define PROG_EPSTOPDF "epstopdf.exe"
static QString standard_extra_paths
/* */      = "C:\\Program Files\\MiKTeX*\\miktex\\bin;C:\\Program Files\\gs\\gs*\\bin";
#elif defined(Q_WS_MAC)
#  define PROG_LATEX "latex"
#  define PROG_DVIPS "dvips"
#  define PROG_GS "gs"
#  define PROG_EPSTOPDF "epstopdf"
static QString standard_extra_paths = "/usr/texbin:/usr/local/bin:/sw/bin:/sw/usr/bin";
#else
#  define PROG_LATEX "latex"
#  define PROG_DVIPS "dvips"
#  define PROG_GS "gs"
#  define PROG_EPSTOPDF "epstopdf"
static QString standard_extra_paths = "";
#endif


void KLFConfig::loadDefaultBackendPaths(KLFConfig *c, bool allowempty)
{
  c->BackendSettings.tempDir = QDir::fromNativeSeparators(QDir::tempPath());
  c->BackendSettings.execLatex = klfSearchPath(PROG_LATEX, standard_extra_paths);
  if (!allowempty && c->BackendSettings.execLatex.isNull()) c->BackendSettings.execLatex = PROG_LATEX;
  c->BackendSettings.execDvips = klfSearchPath(PROG_DVIPS, standard_extra_paths);
  if (!allowempty && c->BackendSettings.execDvips.isNull()) c->BackendSettings.execDvips = PROG_DVIPS;
  c->BackendSettings.execGs = klfSearchPath(PROG_GS, standard_extra_paths);
  if (!allowempty && c->BackendSettings.execGs.isNull()) c->BackendSettings.execGs = PROG_GS;
  c->BackendSettings.execEpstopdf = klfSearchPath(PROG_EPSTOPDF, standard_extra_paths);
  if (!allowempty && c->BackendSettings.execEpstopdf.isNull()) c->BackendSettings.execEpstopdf = "";
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

int KLFConfig::readFromConfig()
{
  ensureHomeConfigDir();

  if (QFile::exists(homeConfigSettingsFile)) {
    return readFromConfig_v2();
  }
  if (QFile::exists(homeConfigSettingsFileIni)) {
    return readFromConfig_ini();
  }
}

template<class T>
static void klf_settings_read(QSettings &s, const QString& baseName, T *target,
			      const char * listOrMapType = NULL)
{
  qDebug("klf_settings_read<...>(%s)", qPrintable(baseName));
  QVariant defVal = QVariant::fromValue<T>(*target);
  QVariant valstr = s.value(baseName, defVal);
  qDebug()<<"\tRead value "<<valstr;
  QVariant val = klfLoadVariantFromText(valstr.toString().toLatin1(), defVal.typeName(), listOrMapType);
  if (val.isValid())
    *target = val.value<T>();
}
template<>
void klf_settings_read<QTextCharFormat>(QSettings &s, const QString& baseName,
					QTextCharFormat *target,
					const char * listOrMapType)
{
  qDebug("klf_settings_read<QTextCharFormat>(%s)", qPrintable(baseName));
  QTextFormat fmt = *target;
  klf_settings_read(s, baseName, &fmt);
  *target = fmt.toCharFormat();
}

template<class T>
static void klf_settings_read_list(QSettings &s, const QString& baseName, QList<T> *target)
{
  QVariantList vlist = klfListToVariantList(*target);
  klf_settings_read(s, baseName, &vlist, QVariant::fromValue<T>(T()).typeName());
  *target = klfVariantListToList<T>(vlist);
}


template<class T>
static void klf_settings_write(QSettings &s, const QString& baseName, const T * value)
{
  QVariant val = QVariant::fromValue<T>(*value);
  QByteArray datastr = klfSaveVariantToText(val);
  s.setValue(baseName, QVariant::fromValue<QString>(QString::fromLocal8Bit(datastr)));
}

template<class T>
static void klf_settings_write_list(QSettings &s, const QString& baseName, const QList<T> * target)
{
  QVariantList vlist = klfListToVariantList(*target);
  klf_settings_write(s, baseName, &vlist);
}

int KLFConfig::readFromConfig_v2()
{
  QSettings s(homeConfigSettingsFile, QSettings::IniFormat);

  qDebug("Reading base configuration");

  s.beginGroup("UI");
  klf_settings_read(s, "locale", &UI.locale);
  klf_settings_read(s, "applicationfont", &UI.applicationFont);
  klf_settings_read(s, "latexeditfont", &UI.latexEditFont);
  klf_settings_read(s, "preambleeditfont", &UI.preambleEditFont);
  klf_settings_read(s, "previewtooltipmaxsize", &UI.previewTooltipMaxSize);
  klf_settings_read(s, "lbloutputfixedsize", &UI.labelOutputFixedSize);
  klf_settings_read(s, "lastsavedir", &UI.lastSaveDir);
  klf_settings_read(s, "symbolsperline", &UI.symbolsPerLine);
  klf_settings_read_list(s, "usercolorlist", &UI.userColorList);
  klf_settings_read_list(s, "colorchoosewidgetrecent", &UI.colorChooseWidgetRecent);
  klf_settings_read_list(s, "colorchoosewidgetcustom", &UI.colorChooseWidgetCustom);
  klf_settings_read(s, "maxusercolors", &UI.maxUserColors);
  klf_settings_read(s, "enabletooltippreview", &UI.enableToolTipPreview);
  klf_settings_read(s, "enablerealtimepreview", &UI.enableRealTimePreview);
  klf_settings_read(s, "autosavelibrarymin", &UI.autosaveLibraryMin);
  s.endGroup();

  s.beginGroup("SyntaxHighlighter");
  klf_settings_read(s, "configflags", &SyntaxHighlighter.configFlags);
  klf_settings_read<QTextCharFormat>(s, "keyword", &SyntaxHighlighter.fmtKeyword);
  klf_settings_read<QTextCharFormat>(s, "comment", &SyntaxHighlighter.fmtComment);
  klf_settings_read<QTextCharFormat>(s, "parenmatch", &SyntaxHighlighter.fmtParenMatch);
  klf_settings_read<QTextCharFormat>(s, "parenmismatch", &SyntaxHighlighter.fmtParenMismatch);
  klf_settings_read<QTextCharFormat>(s, "lonelyparen", &SyntaxHighlighter.fmtLonelyParen);
  s.endGroup();

  s.beginGroup("BackendSettings");
  klf_settings_read(s, "tempdir", &BackendSettings.tempDir);
  klf_settings_read(s, "latexexec", &BackendSettings.execLatex);
  klf_settings_read(s, "dvipsexec", &BackendSettings.execDvips);
  klf_settings_read(s, "gsexec", &BackendSettings.execGs);
  klf_settings_read(s, "epstopdfexec", &BackendSettings.execEpstopdf);
  klf_settings_read(s, "lborderoffset", &BackendSettings.lborderoffset);
  klf_settings_read(s, "tborderoffset", &BackendSettings.tborderoffset);
  klf_settings_read(s, "rborderoffset", &BackendSettings.rborderoffset);
  klf_settings_read(s, "bborderoffset", &BackendSettings.bborderoffset);
  s.endGroup();

  s.beginGroup("LibraryBrowser");
  klf_settings_read(s, "displaytaggedonly", &LibraryBrowser.displayTaggedOnly);
  klf_settings_read(s, "displaynoduplicates", &LibraryBrowser.displayNoDuplicates);
  klf_settings_read(s, "colorfound", &LibraryBrowser.colorFound);
  klf_settings_read(s, "colornotfound", &LibraryBrowser.colorNotFound);
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

}

int KLFConfig::readFromConfig_ini()
{
  QSettings s(homeConfigSettingsFileIni, QSettings::IniFormat);

  s.beginGroup("UI");
  UI.locale = s.value("locale", UI.locale).toString();
  UI.applicationFont = s.value("applicationfont", UI.applicationFont).value<QFont>();
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
  LibraryBrowser.displayTaggedOnly = s.value("displaytaggedonly", LibraryBrowser.displayTaggedOnly).toBool();
  LibraryBrowser.displayNoDuplicates = s.value("displaynoduplicates", LibraryBrowser.displayNoDuplicates).toBool();
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


int KLFConfig::writeToConfig()
{
  ensureHomeConfigDir();
  QSettings s(homeConfigSettingsFile, QSettings::IniFormat);

  s.beginGroup("UI");
  klf_settings_write(s, "locale", &UI.locale);
  klf_settings_write(s, "applicationfont", &UI.applicationFont);
  klf_settings_write(s, "latexeditfont", &UI.latexEditFont);
  klf_settings_write(s, "preambleeditfont", &UI.preambleEditFont);
  klf_settings_write(s, "previewtooltipmaxsize", &UI.previewTooltipMaxSize);
  klf_settings_write(s, "lbloutputfixedsize", &UI.labelOutputFixedSize);
  klf_settings_write(s, "lastsavedir", &UI.lastSaveDir);
  klf_settings_write(s, "symbolsperline", &UI.symbolsPerLine);
  klf_settings_write_list(s, "usercolorlist", &UI.userColorList);
  klf_settings_write_list(s, "colorchoosewidgetrecent", &UI.colorChooseWidgetRecent);
  klf_settings_write_list(s, "colorchoosewidgetcustom", &UI.colorChooseWidgetCustom);
  klf_settings_write(s, "maxusercolors", &UI.maxUserColors);
  klf_settings_write(s, "enabletooltippreview", &UI.enableToolTipPreview);
  klf_settings_write(s, "enablerealtimepreview", &UI.enableRealTimePreview);
  klf_settings_write(s, "autosavelibrarymin", &UI.autosaveLibraryMin);
  s.endGroup();

  s.beginGroup("SyntaxHighlighter");
  klf_settings_write(s, "configflags", &SyntaxHighlighter.configFlags);
  klf_settings_write<QTextFormat>(s, "keyword", &SyntaxHighlighter.fmtKeyword);
  klf_settings_write<QTextFormat>(s, "comment", &SyntaxHighlighter.fmtComment);
  klf_settings_write<QTextFormat>(s, "parenmatch", &SyntaxHighlighter.fmtParenMatch);
  klf_settings_write<QTextFormat>(s, "parenmismatch", &SyntaxHighlighter.fmtParenMismatch);
  klf_settings_write<QTextFormat>(s, "lonelyparen", &SyntaxHighlighter.fmtLonelyParen);
  s.endGroup();

  s.beginGroup("BackendSettings");
  klf_settings_write(s, "tempdir", &BackendSettings.tempDir);
  klf_settings_write(s, "latexexec", &BackendSettings.execLatex);
  klf_settings_write(s, "dvipsexec", &BackendSettings.execDvips);
  klf_settings_write(s, "gsexec", &BackendSettings.execGs);
  klf_settings_write(s, "epstopdfexec", &BackendSettings.execEpstopdf);
  klf_settings_write(s, "lborderoffset", &BackendSettings.lborderoffset);
  klf_settings_write(s, "tborderoffset", &BackendSettings.tborderoffset);
  klf_settings_write(s, "rborderoffset", &BackendSettings.rborderoffset);
  klf_settings_write(s, "bborderoffset", &BackendSettings.bborderoffset);
  s.endGroup();

  s.beginGroup("LibraryBrowser");
  klf_settings_write(s, "displaytaggedonly", &LibraryBrowser.displayTaggedOnly);
  klf_settings_write(s, "displaynoduplicates", &LibraryBrowser.displayNoDuplicates);
  klf_settings_write(s, "colorfound", &LibraryBrowser.colorFound);
  klf_settings_write(s, "colornotfound", &LibraryBrowser.colorNotFound);
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




KLFPluginConfigAccess::KLFPluginConfigAccess()
{
  _config = NULL;
  _pluginname = QString::null;
  _amode = 0;
}

KLFPluginConfigAccess::KLFPluginConfigAccess(KLFConfig *configObject, const QString& pluginName,
					     uint accessMode)
{
  _config = configObject;
  _pluginname = pluginName;
  _amode = accessMode;
}



QString KLFPluginConfigAccess::homeConfigDir() const
{
  if ( _config == NULL ) {
    qWarning("KLFPluginConfigAccess::homeConfigDir: Invalid Config Pointer!\n");
    return QString();
  }

  return _config->homeConfigDir;
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

QVariant KLFPluginConfigAccess::readValue(const QString& key)
{
  if ( _config == NULL ) {
    qWarning("KLFPluginConfigAccess::readValue: Invalid Config Pointer!\n");
    return QVariant();
  }
  if ( (_amode & Read) == 0 ) {
    qWarning("KLFPluginConfigAccess::readValue: Warning: Read mode not set!\n");
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
  if ( (_amode & Write) == 0 ) {
    qWarning("KLFPluginConfigAccess::makeDefaultValue: Warning: Write mode not set!\n");
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
  if ( (_amode & Write) == 0 ) {
    qWarning("KLFPluginConfigAccess::writeValue: Warning: Write mode not set!\n");
    return;
  }
  _config->Plugins.pluginConfig[_pluginname][key] = value;
}

