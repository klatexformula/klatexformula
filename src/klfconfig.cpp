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


#include <QApplication>
#include <QMessageBox>
#include <QObject>
#include <QDir>

#include "klfconfig.h"

// global variable to access our config
// remember to initialize it in main.cpp !
KLFConfig klfconfig;



QString search_path(const QString& prog)
{
  static const QString PATH = getenv("PATH");
#if defined(Q_OS_WIN32) || defined(Q_OS_WIN64)
  static const char pathsep = ';';
#else
  static const char pathsep = ':';
#endif
  static const QStringList paths = PATH.split(pathsep, QString::KeepEmptyParts);
  QString test;
  int k;
  for (k = 0; k < paths.size(); ++k) {
    test = paths[k] + "/" + prog;
    if (QFileInfo(test).isExecutable()) {
      return test;
    }
  }
  return QString::null;
}



void settings_write_QTextCharFormat(QSettings& s, const QString& basename, const QTextCharFormat& charfmt)
{
  //   s.setValue(basename+"_font", charfmt.font());
  //   s.setValue(basename+"_bg", charfmt.color());
  //   "_fg";
  //   "_outline";
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


void KLFConfig::loadDefaults()
{
  homeConfigDir = QDir::homePath() + "/.klatexformula";
  homeConfigSettingsFile = homeConfigDir + "/config";
  
  UI.latexEditFont = QApplication::font();
  UI.preambleEditFont = QApplication::font();
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
  UI.maxUserColors = 12;

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

  BackendSettings.tempDir = QDir::tempPath();
  BackendSettings.execLatex = search_path("latex");
  if (BackendSettings.execLatex.isNull()) BackendSettings.execLatex = "latex";
  BackendSettings.execDvips = search_path("dvips");
  if (BackendSettings.execDvips.isNull()) BackendSettings.execDvips = "dvips";
  BackendSettings.execGs = search_path("gs");
  if (BackendSettings.execGs.isNull()) BackendSettings.execGs = "gs";
  BackendSettings.execEpstopdf = search_path("epstopdf");
  if (BackendSettings.execEpstopdf.isNull()) BackendSettings.execEpstopdf = "";
  BackendSettings.lborderoffset = 1;
  BackendSettings.tborderoffset = 1;
  BackendSettings.rborderoffset = 1;
  BackendSettings.bborderoffset = 1;

  LibraryBrowser.displayTaggedOnly = false;
  LibraryBrowser.displayNoDuplicates = false;
  LibraryBrowser.colorFound = QColor(128, 255, 128);
  LibraryBrowser.colorNotFound = QColor(255, 128, 128);
}


int KLFConfig::ensureHomeConfigDir()
{
  if ( QDir(homeConfigDir).exists() )
    return 0;

  bool r = QDir("/").mkpath(homeConfigDir);
  if ( ! r ) {
    QMessageBox::critical(0, QObject::tr("Error"),
			  QObject::tr("Can't make local config directory `%1' !").arg(homeConfigDir));
    return -1;
  }
  return 0;
}

int KLFConfig::readFromConfig()
{
  ensureHomeConfigDir();
  QSettings s(homeConfigSettingsFile, QSettings::IniFormat);

  s.beginGroup("UI");
  UI.latexEditFont = s.value("latexeditfont", UI.latexEditFont).value<QFont>();
  UI.preambleEditFont = s.value("preambleeditfont", UI.preambleEditFont).value<QFont>();
  UI.previewTooltipMaxSize = s.value("previewtooltipmaxsize", UI.previewTooltipMaxSize).toSize();
  UI.labelOutputFixedSize = s.value("lbloutputfixedsize", UI.labelOutputFixedSize ).toSize();
  UI.lastSaveDir = s.value("lastsavedir", UI.lastSaveDir).toString();
  UI.symbolsPerLine = s.value("symbolsperline", UI.symbolsPerLine).toInt();
  UI.userColorList = settings_read_list(s, "usercolorlist", UI.userColorList);
  UI.maxUserColors = s.value("maxusercolors", UI.maxUserColors).toInt();
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

  return 0;
}


int KLFConfig::writeToConfig()
{
  ensureHomeConfigDir();
  QSettings s(homeConfigSettingsFile, QSettings::IniFormat);

  s.beginGroup("UI");
  s.setValue("latexeditfont", UI.latexEditFont);
  s.setValue("preambleeditfont", UI.preambleEditFont);
  s.setValue("previewtooltipmaxsize", UI.previewTooltipMaxSize);
  s.setValue("lbloutputfixedsize", UI.labelOutputFixedSize);
  s.setValue("lastSaveDir", UI.lastSaveDir);
  s.setValue("symbolsperline", UI.symbolsPerLine);
  settings_write_list(s, "usercolorlist", UI.userColorList);
  s.setValue("maxusercolors", UI.maxUserColors);
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

  s.sync();

  return 0;
}
