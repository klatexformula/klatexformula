/***************************************************************************
 *   file klflibentryeditor.cpp
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


#include <QWidget>
#include <QComboBox>
#include <QEvent>
#include <QKeyEvent>
#include <QPixmap>

#include "klfdata.h"
#include "klfconfig.h"
#include "klflib.h"

#include <ui_klflibentryeditor.h>
#include "klflibentryeditor.h"





KLFLibEntryEditor::KLFLibEntryEditor(QWidget *parent)
  : QWidget(parent)
{
  pUi = new Ui::KLFLibEntryEditor;
  pUi->setupUi(this);
  setAutoFillBackground(false);

  pCurrentStyle = KLFStyle();

  pUi->lblPreview->setFixedSize(klfconfig.UI.labelOutputFixedSize);

  pUi->cbxCategory->setInsertPolicy(QComboBox::InsertAlphabetically);
  pUi->cbxCategory->setDuplicatesEnabled(false);
  pUi->cbxTags->setInsertPolicy(QComboBox::InsertAlphabetically);
  pUi->cbxTags->setDuplicatesEnabled(false);

  pUi->cbxCategory->installEventFilter(this);
  pUi->cbxTags->installEventFilter(this);

  pUi->cbxCategory->addItem("");
  pUi->cbxTags->addItem("");

  connect(pUi->cbxCategory, SIGNAL(activated(int)),
	  this, SLOT(slotUpdateCategory()));
  connect(pUi->cbxTags, SIGNAL(activated(int)),
	  this, SLOT(slotUpdateTags()));

  // preview and latexpreview should be as small as possible (while
  // still respecting their minimum sizes...)
  pUi->splitEntryEditor->setSizes(QList<int>() << 100 << 1000);

  connect(pUi->btnUpdateCategory, SIGNAL(clicked()),
	  this, SLOT(slotUpdateCategory()));
  connect(pUi->btnUpdateTags, SIGNAL(clicked()),
	  this, SLOT(slotUpdateTags()));


  // setup latex preview text browser with syntax highlighter

  pUi->txtPreviewLatex->setFont(klfconfig.UI.preambleEditFont);

  pHighlighter = new KLFLatexSyntaxHighlighter(pUi->txtPreviewLatex, this);
  connect(pUi->txtPreviewLatex, SIGNAL(cursorPositionChanged()),
	  pHighlighter, SLOT(refreshAll()));

  // --

  connect(pUi->btnRestoreStyle, SIGNAL(clicked()), this, SLOT(slotRestoreStyle()));

}
KLFLibEntryEditor::~KLFLibEntryEditor()
{
  delete pUi;
}

bool KLFLibEntryEditor::eventFilter(QObject *object, QEvent *event)
{
  if (object == pUi->cbxCategory || object == pUi->cbxTags) {
    QComboBox *cbx = qobject_cast<QComboBox*>(object);
    if (event->type() == QEvent::KeyPress) {
      QKeyEvent *ke = (QKeyEvent*) event;
      if (ke->key() == Qt::Key_Return || ke->key() == Qt::Key_Enter) {
	slotUpdateFromCbx(cbx);
	return true;
      }
    }
  }

  return QWidget::eventFilter(object, event);
}

void KLFLibEntryEditor::addCategorySuggestions(const QStringList& categorylist)
{
  pUi->cbxCategory->addItems(categorylist);
  slotCbxCleanUpCompletions(pUi->cbxCategory);
}


void KLFLibEntryEditor::displayEntry(const KLFLibEntry& entry)
{
  displayEntries(QList<KLFLibEntry>() << entry);
}

void KLFLibEntryEditor::displayEntries(const QList<KLFLibEntry>& entrylist)
{
  if (entrylist.size() == 0) {
    pUi->lblPreview->setPixmap(QPixmap(":/pics/nopreview.png"));
    pUi->txtPreviewLatex->setText(tr("[ No Item Selected ]"));
    pUi->cbxCategory->setEditText(tr("[ No Item Selected ]"));
    pUi->cbxTags->setEditText(tr("[ No Item Selected ]"));
    pUi->lblStylePreview->setText(tr("[ No Item Selected ]"));
    pUi->cbxCategory->setEnabled(false);
    pUi->btnUpdateCategory->setEnabled(false);
    pUi->cbxTags->setEnabled(false);
    pUi->btnUpdateTags->setEnabled(false);
    pUi->btnRestoreStyle->setEnabled(false);
    pCurrentStyle = KLFStyle();
    return;
  }
  if (entrylist.size() == 1) {
    KLFLibEntry e = entrylist[0];
    pUi->lblPreview->setPixmap(QPixmap::fromImage(e.preview().scaled(pUi->lblPreview->size(),
								     Qt::KeepAspectRatio,
								     Qt::SmoothTransformation)));
    pUi->txtPreviewLatex->setText(e.latex());
    pUi->cbxCategory->setEditText(e.category());
    pUi->cbxTags->setEditText(e.tags());
    pCurrentStyle = e.style();
    pUi->lblStylePreview->setText(prettyPrintStyle(pCurrentStyle));
    pUi->cbxCategory->setEnabled(true);
    pUi->btnUpdateCategory->setEnabled(true);
    pUi->cbxTags->setEnabled(true);
    pUi->btnUpdateTags->setEnabled(true);
    pUi->btnRestoreStyle->setEnabled(true);
    return;
  }
  // multiple items selected
  pUi->lblPreview->setPixmap(QPixmap(":/pics/nopreview.png"));
  pUi->txtPreviewLatex->setText(tr("[ %1 Items Selected ]").arg(entrylist.size()));
  pUi->cbxTags->setEditText(tr("[ Multiple Items Selected ]"));
  // if all elements have same category and style, display them, otherwise set
  // the respective field empty
  QString cat;
  bool allsamestyle = true;
  KLFStyle style;
  int k;
  for (k = 0; k < entrylist.size(); ++k) {
    QString thiscat = entrylist[k].category();
    KLFStyle thisstyle = entrylist[k].style();
    if (k == 0) {
      cat = thiscat;
      style = thisstyle;
      allsamestyle = true;
      continue;
    }
    if ( !cat.isEmpty() && thiscat != cat ) {
      cat = "";
    }
    if ( allsamestyle && !(style == thisstyle) ) {
      allsamestyle = false;
    }
  }
  pUi->cbxCategory->setEditText(cat);
  if ( allsamestyle ) {
    pCurrentStyle = style;
    pUi->lblStylePreview->setText(prettyPrintStyle(style));
    pUi->btnRestoreStyle->setEnabled(true);
  } else {
    pCurrentStyle = KLFStyle();
    pUi->lblStylePreview->setText(tr("[ Different Styles ]"));
    pUi->btnRestoreStyle->setEnabled(false);
  }

  pUi->cbxCategory->setEnabled(true);
  pUi->btnUpdateCategory->setEnabled(true);
  pUi->cbxTags->setEnabled(false);
  pUi->btnUpdateTags->setEnabled(false);
}

void KLFLibEntryEditor::slotUpdateFromCbx(QComboBox *cbx)
{
  if (cbx == pUi->cbxCategory)
    slotUpdateCategory();
  else if (cbx == pUi->cbxTags)
    slotUpdateTags();
  else
    qWarning("KLFLibEntryEditor::slotUpdateFromCbx: Couldn't find combo box=%p", (void*)cbx);
}

void KLFLibEntryEditor::slotUpdateCategory()
{
  slotCbxSaveCurrentCompletion(pUi->cbxCategory);
  emit categoryChanged(pUi->cbxCategory->currentText());
}
void KLFLibEntryEditor::slotUpdateTags()
{
  slotCbxSaveCurrentCompletion(pUi->cbxTags);
  emit tagsChanged(pUi->cbxTags->currentText());
}

void KLFLibEntryEditor::slotRestoreStyle()
{
  emit restoreStyle(pCurrentStyle);
}

void KLFLibEntryEditor::slotCbxSaveCurrentCompletion(QComboBox *cbx)
{
  cbx->addItem(cbx->currentText());
  slotCbxCleanUpCompletions(cbx);
}

void KLFLibEntryEditor::slotCbxCleanUpCompletions(QComboBox *cbx)
{
  cbx->blockSignals(true);
  QString bkp_edittext = cbx->currentText();

  QStringList items;
  QStringList uitems;
  int k;
  for (k = 0; k < cbx->count(); ++k) {
    items << cbx->itemText(k);
  }
  items.sort();
  // unique items now
  for (k = 0; k < items.size(); ++k) {
    if ( ! uitems.contains(items[k]) )
      uitems << items[k];
  }
  // remove all items
  while (cbx->count())
    cbx->removeItem(0);
  cbx->addItems(uitems);

  cbx->setEditText(bkp_edittext);
  cbx->blockSignals(false);
}


/*
void KLFLibEntryEditor::updateEditText(QComboBox *editWidget, const QString& newText)
{
  qDebug()<<"updateEditText("<<newtext<<")!";
  // small utility function that updates text if it isn't already the same text
  if (editWidget->currentText() != newText)
    editWidget->setEditText(newText);
}
*/
