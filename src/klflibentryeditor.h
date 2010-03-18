/***************************************************************************
 *   file klflibentryeditor.h
 *   This file is part of the KLatexFormula Project.
 *   Copyright (C) 2010 by Philippe Faist
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

#ifndef KLFLIBENTRYEDITOR_H
#define KLFLIBENTRYEDITOR_H

#include <QWidget>
#include <QComboBox>

#include <klflatexsyntaxhighlighter.h>
#include <klflib.h>


namespace Ui { class KLFLibEntryEditor; }

class KLFLibEntryEditor : public QWidget
{
  Q_OBJECT
public:
  KLFLibEntryEditor(QWidget *parent = NULL);
  virtual ~KLFLibEntryEditor();

  void addCategorySuggestions(const QStringList& categorylist);

  virtual bool eventFilter(QObject *object, QEvent *event);

signals:

  void categoryChanged(const QString& newCategory);
  void tagsChanged(const QString& newTags);
  void restoreStyle(const KLFStyle& style);

public slots:

  void displayEntry(const KLFLibEntry& entry);

  /** Handles the selection of multiple items properly by displaying
   * [Multiple items selected] whenever needed. */
  void displayEntries(const QList<KLFLibEntry>& entries);

protected slots:

  void slotUpdateFromCbx(QComboBox *cbx);
  void slotUpdateCategory();
  void slotUpdateTags();
  void slotRestoreStyle();

  void slotCbxSaveCurrentCompletion(QComboBox *cbx);
  void slotCbxCleanUpCompletions(QComboBox *cbx);

private:
  Ui::KLFLibEntryEditor *pUi;

  KLFStyle pCurrentStyle;

  KLFLatexSyntaxHighlighter *pHighlighter;

  //  void updateEditText(QComboBox *editWidget, const QString& newText);
};







#endif
