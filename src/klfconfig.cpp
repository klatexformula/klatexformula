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


KLFConfig::KLFConfig()
{
}


void KLFConfig::loadDefaults()
{
  homeConfigDir = QDir::homePath() + "/.klatexformula";
  homeConfigSettingsFile = homeConfigDir + "/config";
  
  SyntaxHighlighter.configFlags = 0x05;
  SyntaxHighlighter.colorKeyword = QColor(0, 0, 128);
  SyntaxHighlighter.colorComment = QColor(128, 0, 0);
  SyntaxHighlighter.colorParenMatch = QColor(0, 128, 128);
  SyntaxHighlighter.colorParenMismatch = QColor(255, 0, 255);
  SyntaxHighlighter.colorLonelyParen = QColor(255, 0, 255);

  UI.latexEditFont = QApplication::font();
  UI.previewTooltipMaxSize = QSize(500, 350);
  UI.labelOutputFixedSize = QSize(280, 100);
  UI.lastSaveDir = QDir::homePath();

  BackendSettings.tempDir = QDir::tempPath();
  BackendSettings.execLatex = "latex";
  BackendSettings.execDvips = "dvips";
  BackendSettings.execGs = "gs";
  BackendSettings.execEpstopdf = "epstopdf";
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

  s.beginGroup("SyntaxHighlighter");
  SyntaxHighlighter.configFlags = s.value("configflags", SyntaxHighlighter.configFlags).toUInt();
  SyntaxHighlighter.colorKeyword = s.value("keyword", SyntaxHighlighter.colorKeyword).value<QColor>();
  SyntaxHighlighter.colorComment = s.value("comment", SyntaxHighlighter.colorComment).value<QColor>();
  SyntaxHighlighter.colorParenMatch = s.value("parenmatch", SyntaxHighlighter.colorParenMatch).value<QColor>();
  SyntaxHighlighter.colorParenMismatch = s.value("parenmismatch", SyntaxHighlighter.colorParenMismatch).value<QColor>();
  SyntaxHighlighter.colorLonelyParen = s.value("lonelyparen", SyntaxHighlighter.colorLonelyParen).value<QColor>();
  s.endGroup();

  s.beginGroup("UI");
  UI.latexEditFont = s.value("latexeditfont", UI.latexEditFont).value<QFont>();
  UI.previewTooltipMaxSize = s.value("previewtooltipmaxsize", UI.previewTooltipMaxSize).toSize();
  UI.labelOutputFixedSize = s.value("lbloutputfixedsize", UI.labelOutputFixedSize ).toSize();
  UI.lastSaveDir = s.value("lastsavedir", UI.lastSaveDir).toString();
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

  s.beginGroup("SyntaxHighlighting");
  s.setValue("configflags", SyntaxHighlighter.configFlags);
  s.setValue("keyword", SyntaxHighlighter.colorKeyword);
  s.setValue("comment", SyntaxHighlighter.colorComment);
  s.setValue("parenmatch", SyntaxHighlighter.colorParenMatch);
  s.setValue("parenmismatch", SyntaxHighlighter.colorParenMismatch);
  s.setValue("lonelyparen", SyntaxHighlighter.colorLonelyParen);
  s.endGroup();

  s.beginGroup("UI");
  s.setValue("latexeditfont", UI.latexEditFont);
  s.setValue("previewtooltipmaxsize", UI.previewTooltipMaxSize);
  s.setValue("lbloutputfixedsize", UI.labelOutputFixedSize);
  s.setValue("lastSaveDir", UI.lastSaveDir);
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

  return 0;
}
