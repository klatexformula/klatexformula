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


#include <qapplication.h>

#include <kapplication.h>
#include <kstandarddirs.h>
#include <kconfig.h>
#include <kglobal.h>

#include "klfconfig.h"

// global variable to access our config
// remember to initialize it in main.cpp !
KLFConfig klfconfig;


KLFConfig::KLFConfig()
{
}


void KLFConfig::loadDefaults()
{
  SyntaxHighlighter.configFlags = 0x05;
  SyntaxHighlighter.colorKeyword = QColor(0, 0, 128);
  SyntaxHighlighter.colorComment = QColor(128, 0, 0);
  SyntaxHighlighter.colorParenMatch = QColor(0, 128, 128);
  SyntaxHighlighter.colorParenMismatch = QColor(255, 0, 255);
  SyntaxHighlighter.colorLonelyParen = QColor(255, 0, 255);

  Appearance.latexEditFont = QApplication::font();
  Appearance.previewTooltipMaxSize = QSize(500, 350);
  Appearance.labelOutputFixedSize = QSize(280, 80);

  BackendSettings.tempDir = KGlobal::instance()->dirs()->findResourceDir("tmp", "/");
  BackendSettings.execLatex = KStandardDirs::findExe("latex");
  BackendSettings.execDvips = KStandardDirs::findExe("dvips");
  BackendSettings.execGs = KStandardDirs::findExe("gs");
  BackendSettings.execEpstopdf = KStandardDirs::findExe("epstopdf");
  BackendSettings.lborderoffset = 1;
  BackendSettings.tborderoffset = 1;
  BackendSettings.rborderoffset = 1;
  BackendSettings.bborderoffset = 1;

  LibraryBrowser.displayTaggedOnly = false;
  LibraryBrowser.displayNoDuplicates = false;
  LibraryBrowser.colorFound = QColor(128, 255, 128);
  LibraryBrowser.colorNotFound = QColor(255, 128, 128);
}

int KLFConfig::readFromConfig(KConfig *cfg)
{
  if ( ! cfg )
    cfg = kapp->config();

  cfg->setGroup("SyntaxHighlighting"); // -ing. NOT an error.

  SyntaxHighlighter.configFlags = cfg->readUnsignedNumEntry("configflags", SyntaxHighlighter.configFlags);
  SyntaxHighlighter.colorKeyword = cfg->readColorEntry("keyword", & SyntaxHighlighter.colorKeyword);
  SyntaxHighlighter.colorComment = cfg->readColorEntry("comment", & SyntaxHighlighter.colorComment);
  SyntaxHighlighter.colorParenMatch = cfg->readColorEntry("parenmatch", & SyntaxHighlighter.colorParenMatch);
  SyntaxHighlighter.colorParenMismatch = cfg->readColorEntry("parenmismatch", & SyntaxHighlighter.colorParenMismatch);
  SyntaxHighlighter.colorLonelyParen = cfg->readColorEntry("lonelyparen", & SyntaxHighlighter.colorLonelyParen);


  cfg->setGroup("Appearance");

  Appearance.latexEditFont = cfg->readFontEntry("latexeditfont", & Appearance.latexEditFont);
  Appearance.previewTooltipMaxSize = cfg->readSizeEntry("previewtooltipmaxsize", & Appearance.previewTooltipMaxSize);
  QSize n(1, 1);
  Appearance.labelOutputFixedSize = cfg->readSizeEntry("lbloutputfixedsize", & n );
  if (Appearance.labelOutputFixedSize == n) {
    Appearance.labelOutputFixedSize =
      QSize( cfg->readNumEntry("lbloutputfixedwidth", 280) ,
	     cfg->readNumEntry("lbloutputfixedheight", 100) );
  }


  cfg->setGroup("BackendSettings");

  BackendSettings.tempDir = cfg->readEntry("tempdir", BackendSettings.tempDir);
  BackendSettings.execLatex = cfg->readEntry("latexexec", BackendSettings.execLatex);
  BackendSettings.execDvips = cfg->readEntry("dvipsexec", BackendSettings.execDvips);
  BackendSettings.execGs = cfg->readEntry("gsexec", BackendSettings.execGs);
  BackendSettings.execEpstopdf = cfg->readEntry("epstopdfexec", BackendSettings.execEpstopdf);
  BackendSettings.lborderoffset = cfg->readNumEntry("lborderoffset", BackendSettings.lborderoffset);
  BackendSettings.tborderoffset = cfg->readNumEntry("tborderoffset", BackendSettings.tborderoffset);
  BackendSettings.rborderoffset = cfg->readNumEntry("rborderoffset", BackendSettings.rborderoffset);
  BackendSettings.bborderoffset = cfg->readNumEntry("bborderoffset", BackendSettings.bborderoffset);


  cfg->setGroup("LibraryBrowser");

  LibraryBrowser.displayTaggedOnly = cfg->readBoolEntry("displaytaggedonly", LibraryBrowser.displayTaggedOnly);
  LibraryBrowser.displayNoDuplicates = cfg->readBoolEntry("displaynoduplicates", LibraryBrowser.displayNoDuplicates);
  LibraryBrowser.colorFound = cfg->readColorEntry("colorfound", & LibraryBrowser.colorFound);
  LibraryBrowser.colorNotFound = cfg->readColorEntry("colornotfound", & LibraryBrowser.colorNotFound);

  return 0;
}

int KLFConfig::writeToConfig(KConfig *cfg)
{
  if ( ! cfg )
    cfg = kapp->config();

  cfg->setGroup("SyntaxHighlighting");

  cfg->writeEntry("configflags", SyntaxHighlighter.configFlags);
  cfg->writeEntry("keyword", SyntaxHighlighter.colorKeyword);
  cfg->writeEntry("comment", SyntaxHighlighter.colorComment);
  cfg->writeEntry("parenmatch", SyntaxHighlighter.colorParenMatch);
  cfg->writeEntry("parenmismatch", SyntaxHighlighter.colorParenMismatch);
  cfg->writeEntry("lonelyparen", SyntaxHighlighter.colorLonelyParen);


  cfg->setGroup("Appearance");

  cfg->writeEntry("latexeditfont", Appearance.latexEditFont);
  cfg->writeEntry("previewtooltipmaxsize", Appearance.previewTooltipMaxSize);
  cfg->writeEntry("lbloutputfixedsize", Appearance.labelOutputFixedSize);


  cfg->setGroup("BackendSettings");

  cfg->writeEntry("tempdir", BackendSettings.tempDir);
  cfg->writeEntry("latexexec", BackendSettings.execLatex);
  cfg->writeEntry("dvipsexec", BackendSettings.execDvips);
  cfg->writeEntry("gsexec", BackendSettings.execGs);
  cfg->writeEntry("epstopdfexec", BackendSettings.execEpstopdf);
  cfg->writeEntry("lborderoffset", BackendSettings.lborderoffset);
  cfg->writeEntry("tborderoffset", BackendSettings.tborderoffset);
  cfg->writeEntry("rborderoffset", BackendSettings.rborderoffset);
  cfg->writeEntry("bborderoffset", BackendSettings.bborderoffset);


  cfg->setGroup("LibraryBrowser");

  cfg->writeEntry("displaytaggedonly", LibraryBrowser.displayTaggedOnly);
  cfg->writeEntry("displaynoduplicates", LibraryBrowser.displayNoDuplicates);
  cfg->writeEntry("colorfound", LibraryBrowser.colorFound);
  cfg->writeEntry("colornotfound", LibraryBrowser.colorNotFound);

  return 0;
}
