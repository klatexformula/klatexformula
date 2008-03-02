/***************************************************************************
 *   file klfconfig.h
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

#ifndef KLFCONFIG_H
#define KLFCONFIG_H

#include <qstring.h>
#include <qfont.h>
#include <qsize.h>
#include <qcolor.h>

#include <kconfig.h>

class KLFConfig {
public:

  // this doesn't do anything. It actually leaves every entry with undefined values.
  // This is why it's important to call loadDefaults() quickly after building an instance
  // of KLFConfig. readFromConfig() isn't enough, beacause it assumes the default values
  // are already stored in the current fields.
  KLFConfig();

  struct {

    unsigned int configFlags;
    QColor colorKeyword;
    QColor colorComment;
    QColor colorParenMatch;
    QColor colorParenMismatch;
    QColor colorLonelyParen;

  } SyntaxHighlighter;

  struct {

    QFont latexEditFont;
    QSize previewTooltipMaxSize;
    QSize labelOutputFixedSize;

  } Appearance;

  struct {

    QString tempDir;
    QString execLatex;
    QString execDvips;
    QString execGs;
    QString execEpstopdf;
    int lborderoffset;
    int tborderoffset;
    int rborderoffset;
    int bborderoffset;

  } BackendSettings;

  struct {

    bool displayTaggedOnly;

  } HistoryBrowser;

  // call loadDefaults() before anything, at the beginning.
  void loadDefaults();
  // cfg defaults to kapp->config()
  int readFromConfig(KConfig *cfg = 0);

  int writeToConfig(KConfig *cfg = 0);

};

#endif