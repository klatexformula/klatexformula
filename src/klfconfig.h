/***************************************************************************
 *   file klfconfig.h
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

#ifndef KLFCONFIG_H
#define KLFCONFIG_H

#include <QString>
#include <QFont>
#include <QSize>
#include <QColor>
#include <QSettings>
#include <QTextCharFormat>


class KLFConfig {
public:

  // this doesn't do anything. It actually leaves every entry with undefined values.
  // This is why it's important to call loadDefaults() quickly after building an instance
  // of KLFConfig. readFromConfig() isn't enough, beacause it assumes the default values
  // are already stored in the current fields.
  KLFConfig();

  QString homeConfigDir;
  QString homeConfigSettingsFile;

  struct {

    QFont latexEditFont;
    QFont preambleEditFont;
    QSize previewTooltipMaxSize;
    QSize labelOutputFixedSize;
    QString lastSaveDir;
    int symbolsPerLine;
    QList<QColor> userColorList;
    int maxUserColors;

  } UI;

  struct {

    unsigned int configFlags;
    QTextCharFormat fmtKeyword;
    QTextCharFormat fmtComment;
    QTextCharFormat fmtParenMatch;
    QTextCharFormat fmtParenMismatch;
    QTextCharFormat fmtLonelyParen;

  } SyntaxHighlighter;

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
    bool displayNoDuplicates;
    QColor colorFound;
    QColor colorNotFound;

  } LibraryBrowser;

  // call loadDefaults() before anything, at the beginning.
  void loadDefaults();
  // cfg defaults to kapp->config()
  int readFromConfig();

  int ensureHomeConfigDir();

  int writeToConfig();

};

// utility function
QString search_path(const QString& prog);



// defined in main.cpp
extern KLFConfig klfconfig;



#endif
