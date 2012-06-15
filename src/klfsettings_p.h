/***************************************************************************
 *   file klfsettings_p.h
 *   This file is part of the KLatexFormula Project.
 *   Copyright (C) 2012 by Philippe Faist
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

/** \file
 * This header contains (in principle private) auxiliary classes for
 * library routines defined in ****.cpp */

#ifndef KLFSETTINGS_P_H
#define KLFSETTINGS_P_H



#include <QDialog>
#include <QStandardItemModel>
#include <QStyledItemDelegate>
#include <QItemEditorFactory>
#include <QStandardItemEditorCreator>
#include <QMessageBox>
#include <QLineEdit>

#include <klfcolorchooser.h>
#include "klfconfig.h"
#include "klfsettings.h"
#include "klfadvancedconfigeditor.h"

struct KLFSettingsPrivate
{
  KLF_PRIVATE_HEAD(KLFSettings)
  {
    mainWin = NULL;
    pUserSetDefaultAppFont = false;
    pluginstuffloaded = false;
#ifdef KLF_EXPERIMENTAL
    advancedConfigEditor = NULL;
#endif
  }


  KLFMainWin *mainWin;

  bool pUserSetDefaultAppFont;

  QMap<QString,QPushButton*> pFontButtons;
  QMap<QString,QAction*> pFontBasePresetActions;
  QList<QAction*> pFontSetActions;

  struct TextFormatEnsemble {
    TextFormatEnsemble(KLFConfigProp<QTextCharFormat> *format,
		       KLFColorChooser *foreground, KLFColorChooser *background,
		       QCheckBox *chkBold, QCheckBox *chkItalic)
      : fmt(format), fg(foreground), bg(background), chkB(chkBold), chkI(chkItalic) { }
    KLFConfigProp<QTextCharFormat> *fmt;
    KLFColorChooser *fg;
    KLFColorChooser *bg;
    QCheckBox *chkB;
    QCheckBox *chkI;
  };
  QList<TextFormatEnsemble> textformats;

  bool pluginstuffloaded;
  QMap<QString,QWidget*> pluginConfigWidgets;
  QMap<QString,QTreeWidgetItem*> pluginListItems;

#ifdef KLF_EXPERIMENTAL
  KLFAdvancedConfigEditor *advancedConfigEditor;
#endif

  bool setDefaultFor(const QString& progname, const QString& guessprog, bool required,
		     KLFPathChooser *destination);
};






#endif
