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

#include <klfmainwin.h>

#include "klfconfig.h"

// global variable to access our config
// remember to initialize it in main.cpp !
KLFConfig klfconfig;


// negative limit means "no limit"
static QStringList __search_find_test(const QString& root, const QStringList& pathlist, int level, int limit)
{
  if (limit == 0)
    return QStringList();

  QStringList newpathlist = pathlist;
  // our level: levelpathlist contains items in pathlist from 0 to level-1 inclusive.
  QStringList levelpathlist;
  int k;
  for (k = 0; k < level; ++k) { levelpathlist << newpathlist[k]; }
  // the dir/file at our level:
  QString flpath = root+levelpathlist.join("/");
  QFileInfo flinfo(flpath);
  if (flinfo.isDir()) {
    QDir d(flpath);
    QStringList entries = d.entryList(QStringList()<<pathlist[level]);
    QStringList hitlist;
    for (k = 0; k < entries.size(); ++k) {
      newpathlist[level] = entries[k];
      hitlist << __search_find_test(root, newpathlist, level+1, limit - hitlist.size());
      if (limit >= 0 && hitlist.size() >= limit) // reached limit
	break;
    }
    return hitlist;
  }
  if (flinfo.exists()) {
    return QStringList() << QDir::toNativeSeparators(root+pathlist.join("/"));
  }
  return QStringList();
}

// returns at most limit results matching wildcard_expression (which is given as absolute path with wildcards)
QStringList search_find(const QString& wildcard_expression, int limit)
{
  QString expr = QDir::fromNativeSeparators(wildcard_expression);
  QStringList pathlist = expr.split("/", QString::SkipEmptyParts);
  QString root = "/";
  static QRegExp driveregexp("^[A-Za-z]:$");
  if (driveregexp.exactMatch(pathlist[0])) {
    // WIN System with X: drive letter
    root = pathlist[0]+"/";
    pathlist.pop_front();
  }
  return __search_find_test(root, pathlist, 0, limit);
}

// smart search PATH that will interpret wildcards in PATH+extra_path and return the first matching executable
QString search_path(const QString& prog, const QString& extra_path)
{
  static const QString PATH = getenv("PATH");
#if defined(Q_OS_WIN32) || defined(Q_OS_WIN64)
  static const char pathsep = ';';
#else
  static const char pathsep = ':';
#endif
  QString path = PATH;
  if (!extra_path.isEmpty())
    path += pathsep + extra_path;

  const QStringList paths = path.split(pathsep, QString::KeepEmptyParts);
  QString test;
  int k, j;
  for (k = 0; k < paths.size(); ++k) {
    QStringList hits = search_find(paths[k]+"/"+prog);
    for (j = 0; j < hits.size(); ++j) {
      if ( QFileInfo(hits[j]).isExecutable() ) {
	return hits[j];
      }
    }
  }
  return QString::null;
}



void settings_write_QTextCharFormat(QSettings& s, const QString& basename, const QTextCharFormat& charfmt)
{
  s.setValue(basename+"_charformat", charfmt);
}
QTextCharFormat settings_read_QTextCharFormat(QSettings& s, const QString& basename, const QTextCharFormat& dflt)
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




KLFConfig::KLFConfig()
{
}


#define KLFCONFIG_TEST_FIXED_FONT(found_fcode, fdb, fcode, f)		\
  if (!found_fcode && fdb.isFixedPitch(f)) {				\
    fcode = QFont(f, 11);						\
    found_fcode = true;							\
  }

void KLFConfig::loadDefaults()
{
  homeConfigDir = QDir::homePath() + "/.klatexformula";
  homeConfigSettingsFile = homeConfigDir + "/config";

  if (qApp->inherits("QApplication")) {
    QFont f = QApplication::font();
#ifdef Q_WS_X11
    f.setPixelSize(15); // setting pixel size avoids X11 bug of fonts having their metrics badly calculated
#endif

    QFontDatabase fdb;
    QFont fcode;
    bool found_fcode = false;
    KLFCONFIG_TEST_FIXED_FONT(found_fcode, fdb, fcode, "Courier 10 Pitch");
    KLFCONFIG_TEST_FIXED_FONT(found_fcode, fdb, fcode, "ETL Fixed");
    KLFCONFIG_TEST_FIXED_FONT(found_fcode, fdb, fcode, "Courier New");
    KLFCONFIG_TEST_FIXED_FONT(found_fcode, fdb, fcode, "Efont Fixed");
    KLFCONFIG_TEST_FIXED_FONT(found_fcode, fdb, fcode, "Adobe Courier");
    KLFCONFIG_TEST_FIXED_FONT(found_fcode, fdb, fcode, "Courier");
    KLFCONFIG_TEST_FIXED_FONT(found_fcode, fdb, fcode, "Misc Fixed");
    KLFCONFIG_TEST_FIXED_FONT(found_fcode, fdb, fcode, "Monospace");
    if ( ! found_fcode )
      fcode = f;

    UI.applicationFont = f;
    UI.latexEditFont = fcode;
    UI.preambleEditFont = fcode;
    UI.previewTooltipMaxSize = QSize(500, 350);
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
    UI.enableToolTipPreview = true;
    UI.enableRealTimePreview = true;

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
static QString standard_extra_paths = "C:\\Program Files\\MiKTeX*\\miktex\\bin;C:\\Program Files\\gs\\gs*\\bin";
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
  c->BackendSettings.execLatex = search_path(PROG_LATEX, standard_extra_paths);
  if (!allowempty && c->BackendSettings.execLatex.isNull()) c->BackendSettings.execLatex = PROG_LATEX;
  c->BackendSettings.execDvips = search_path(PROG_DVIPS, standard_extra_paths);
  if (!allowempty && c->BackendSettings.execDvips.isNull()) c->BackendSettings.execDvips = PROG_DVIPS;
  c->BackendSettings.execGs = search_path(PROG_GS, standard_extra_paths);
  if (!allowempty && c->BackendSettings.execGs.isNull()) c->BackendSettings.execGs = PROG_GS;
  c->BackendSettings.execEpstopdf = search_path(PROG_EPSTOPDF, standard_extra_paths);
  if (!allowempty && c->BackendSettings.execEpstopdf.isNull()) c->BackendSettings.execEpstopdf = "";
}

int KLFConfig::ensureHomeConfigDir()
{
  if ( ! QDir(homeConfigDir).exists() ) {
    bool r = QDir("/").mkpath(homeConfigDir);
    if ( ! r ) {
      QMessageBox::critical(0, QObject::tr("Error"),
			    QObject::tr("Can't make local config directory `%1' !").arg(homeConfigDir));
      return -1;
    }
  }
  if ( ! QDir(homeConfigDir + "/rccresources").exists() ) {
    bool r = QDir("/").mkpath(homeConfigDir + "/rccresources");
    if ( ! r ) {
      QMessageBox::critical(0, QObject::tr("Error"),
			    QObject::tr("Can't make local config directory `%1' !").arg(homeConfigDir));
      return -1;
    }
  }
  return 0;
}

int KLFConfig::readFromConfig()
{
  ensureHomeConfigDir();
  QSettings s(homeConfigSettingsFile, QSettings::IniFormat);

  s.beginGroup("UI");
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
  s.setValue("applicationfont", UI.applicationFont);
  s.setValue("latexeditfont", UI.latexEditFont);
  s.setValue("preambleeditfont", UI.preambleEditFont);
  s.setValue("previewtooltipmaxsize", UI.previewTooltipMaxSize);
  s.setValue("lbloutputfixedsize", UI.labelOutputFixedSize);
  s.setValue("lastSaveDir", UI.lastSaveDir);
  s.setValue("symbolsperline", UI.symbolsPerLine);
  settings_write_list(s, "usercolorlist", UI.userColorList);
  settings_write_list(s, "colorchoosewidgetrecent", UI.colorChooseWidgetRecent);
  settings_write_list(s, "colorchoosewidgetcustom", UI.colorChooseWidgetCustom);
  s.setValue("maxusercolors", UI.maxUserColors);
  s.setValue("enabletooltippreview", UI.enableToolTipPreview);
  s.setValue("enablerealtimepreview", UI.enableRealTimePreview);
  s.endGroup();

  s.beginGroup("SyntaxHighlighter");
  s.setValue("configflags", SyntaxHighlighter.configFlags);
  settings_write_QTextCharFormat(s, "keyword", SyntaxHighlighter.fmtKeyword);
  settings_write_QTextCharFormat(s, "comment", SyntaxHighlighter.fmtComment);
  settings_write_QTextCharFormat(s, "parenmatch", SyntaxHighlighter.fmtParenMatch);
  settings_write_QTextCharFormat(s, "parenmismatch", SyntaxHighlighter.fmtParenMismatch);
  settings_write_QTextCharFormat(s, "lonelyparen", SyntaxHighlighter.fmtLonelyParen);
  s.endGroup();

  s.beginGroup("BackendSettings");
  s.setValue("tempdir", BackendSettings.tempDir);
  s.setValue("latexexec", BackendSettings.execLatex);
  s.setValue("dvipsexec", BackendSettings.execDvips);
  s.setValue("gsexec", BackendSettings.execGs);
  s.setValue("epstopdfexec", BackendSettings.execEpstopdf);
  s.setValue("lborderoffset", BackendSettings.lborderoffset);
  s.setValue("tborderoffset", BackendSettings.tborderoffset);
  s.setValue("rborderoffset", BackendSettings.rborderoffset);
  s.setValue("bborderoffset", BackendSettings.bborderoffset);
  s.endGroup();

  s.beginGroup("LibraryBrowser");
  s.setValue("displaytaggedonly", LibraryBrowser.displayTaggedOnly);
  s.setValue("displaynoduplicates", LibraryBrowser.displayNoDuplicates);
  s.setValue("colorfound", LibraryBrowser.colorFound);
  s.setValue("colornotfound", LibraryBrowser.colorNotFound);
  s.endGroup();

  // Special treatment for Plugins.pluginConfig
  QMap< QString, QMap<QString,QVariant> >::const_iterator it;
  for (it = Plugins.pluginConfig.begin(); it != Plugins.pluginConfig.end(); ++it) {
    s.beginGroup( QString("Plugins/Config/%1").arg(it.key()) );
    QMap<QString,QVariant> thispluginconfig = it.value();
    QMap<QString,QVariant>::const_iterator pcit;
    for (pcit = thispluginconfig.begin(); pcit != thispluginconfig.end(); ++pcit) {
      s.setValue(pcit.key(), pcit.value());
    }
    s.endGroup();
  }

  s.sync();

  return 0;
}



KLFPluginConfigAccess::KLFPluginConfigAccess(KLFConfig *configObject, const QString& pluginName,
					     uint accessMode)
{
  _config = configObject;
  _pluginname = pluginName;
  _amode = accessMode;
}




QVariant KLFPluginConfigAccess::readValue(const QString& key)
{
  if ( (_amode & Read) == 0 ) {
    fprintf(stderr, "KLFPluginConfigAccess::readValue: Warning: Read mode not set!\n");
    return QVariant();
  }
  if ( ! _config->Plugins.pluginConfig[_pluginname].contains(key) )
    return QVariant();

  return _config->Plugins.pluginConfig[_pluginname][key];
}

void KLFPluginConfigAccess::writeValue(const QString& key, const QVariant& value)
{

  if ( (_amode & Write) == 0 ) {
    fprintf(stderr, "KLFPluginConfigAccess::writeValue: Warning: Write mode not set!\n");
    return;
  }
  _config->Plugins.pluginConfig[_pluginname][key] = value;
}

