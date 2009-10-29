/***************************************************************************
 *   file 
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

#ifndef KLFSETTINGS_H
#define KLFSETTINGS_H

#include <QDialog>
#include <QTextCharFormat>

#include <klfbackend.h>

#include <ui_klfsettingsui.h>

class KLFLatexSyntaxHighlighter;
class KLFMainWin;

class KLFSettings : public QDialog, private Ui::KLFSettingsUI
{
  Q_OBJECT

public:
  KLFSettings(KLFMainWin* parent = 0);
  ~KLFSettings();

public slots:

  void reset();
  void show();

  void apply();

  void setDefaultPaths();
  void importExtensionFile();

protected:

protected slots:

  void initPluginControls();

  virtual void accept();
  virtual void slotChangeFont();

private:

  KLFMainWin *_mainwin;

  struct TextFormatEnsemble {
    TextFormatEnsemble(QTextCharFormat *format,
		       KLFColorChooser *foreground, KLFColorChooser *background,
		       QCheckBox *chkBold, QCheckBox *chkItalic)
      : fmt(format), fg(foreground), bg(background), chkB(chkBold), chkI(chkItalic) { }
    QTextCharFormat *fmt;
    KLFColorChooser *fg;
    KLFColorChooser *bg;
    QCheckBox *chkB;
    QCheckBox *chkI;
  };
  QList<TextFormatEnsemble> _textformats;

  bool _pluginstuffloaded;
  QMap<QString,QWidget*> mPluginConfigWidgets;

  bool setDefaultFor(const QString& progname, const QString& guessprog, bool required,
		     KLFPathChooser *destination);
};

#endif

