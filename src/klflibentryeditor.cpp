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
#include <QLineEdit>
#include <QEvent>
#include <QKeyEvent>
#include <QPixmap>

//#include "klfdata.h"
#include "klfconfig.h"
#include "klflib.h"

#include <ui_klflibentryeditor.h>
#include "klflibentryeditor.h"





KLFLibEntryEditor::KLFLibEntryEditor(QWidget *parent)
  : QWidget(parent), pInputEnabled(true)
{
  u = new Ui::KLFLibEntryEditor;
  u->setupUi(this);
  setAutoFillBackground(false);

  pCurrentStyle = KLFStyle();

  u->lblPreview->setFixedSize(klfconfig.UI.labelOutputFixedSize);

  u->cbxCategory->setInsertPolicy(QComboBox::InsertAlphabetically);
  u->cbxCategory->setDuplicatesEnabled(false);
  u->cbxTags->setInsertPolicy(QComboBox::InsertAlphabetically);
  u->cbxTags->setDuplicatesEnabled(false);

  u->cbxCategory->installEventFilter(this);
  u->cbxTags->installEventFilter(this);

  u->cbxCategory->addItem("");
  u->cbxTags->addItem("");

  connect(u->cbxCategory, SIGNAL(activated(int)),
	  this, SLOT(slotUpdateCategory()));
  connect(u->cbxTags, SIGNAL(activated(int)),
	  this, SLOT(slotUpdateTags()));

  // preview and latexpreview should be as small as possible (while
  // still respecting their minimum sizes...)
  u->splitEntryEditor->setSizes(QList<int>() << 100 << 1000);

  connect(u->btnUpdateCategory, SIGNAL(clicked()),
	  this, SLOT(slotUpdateCategory()));
  connect(u->btnUpdateTags, SIGNAL(clicked()),
	  this, SLOT(slotUpdateTags()));


  // setup latex preview text browser with syntax highlighter

  u->txtPreviewLatex->setFont(klfconfig.UI.preambleEditFont);

  pHighlighter = new KLFLatexSyntaxHighlighter(u->txtPreviewLatex, this);
  connect(u->txtPreviewLatex, SIGNAL(cursorPositionChanged()),
	  pHighlighter, SLOT(refreshAll()));

  // --

  connect(u->btnRestoreStyle, SIGNAL(clicked()), this, SLOT(slotRestoreStyle()));

}
void KLFLibEntryEditor::retranslateUi(bool alsoBaseUi)
{
  if (alsoBaseUi)
    u->retranslateUi(this);
}
KLFLibEntryEditor::~KLFLibEntryEditor()
{
  delete u;
}

bool KLFLibEntryEditor::eventFilter(QObject *object, QEvent *event)
{
  if (object == u->cbxCategory || object == u->cbxTags) {
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
  u->cbxCategory->addItems(categorylist);
  slotCbxCleanUpCompletions(u->cbxCategory);
}


void KLFLibEntryEditor::displayEntry(const KLFLibEntry& entry)
{
  displayEntries(QList<KLFLibEntry>() << entry);
}

void KLFLibEntryEditor::displayEntries(const QList<KLFLibEntry>& entrylist)
{
  u->cbxCategory->lineEdit()->setReadOnly(!pInputEnabled);
  u->cbxTags->lineEdit()->setReadOnly(!pInputEnabled);
  if (entrylist.size() == 0) {
    u->lblPreview->setPixmap(QPixmap(":/pics/nopreview.png"));
    u->txtPreviewLatex->setText(tr("[ No Item Selected ]"));
    u->cbxCategory->setEditText(tr("[ No Item Selected ]"));
    u->cbxTags->setEditText(tr("[ No Item Selected ]"));
    u->lblStylePreview->setText(tr("[ No Item Selected ]"));
    u->cbxCategory->setEnabled(false);
    u->btnUpdateCategory->setEnabled(false);
    u->cbxTags->setEnabled(false);
    u->btnUpdateTags->setEnabled(false);
    u->btnRestoreStyle->setEnabled(false);
    pCurrentStyle = KLFStyle();
    return;
  }
  if (entrylist.size() == 1) {
    KLFLibEntry e = entrylist[0];
    u->lblPreview->setPixmap(QPixmap::fromImage(e.preview().scaled(u->lblPreview->size(),
								     Qt::KeepAspectRatio,
								     Qt::SmoothTransformation)));
    u->txtPreviewLatex->setText(e.latex());
    u->cbxCategory->setEditText(e.category());
    u->cbxTags->setEditText(e.tags());
    pCurrentStyle = e.style();
    u->lblStylePreview->setText(prettyPrintStyle(pCurrentStyle));
    u->cbxCategory->setEnabled(true);
    u->btnUpdateCategory->setEnabled(pInputEnabled && true);
    u->cbxTags->setEnabled(true);
    u->btnUpdateTags->setEnabled(pInputEnabled && true);
    u->btnRestoreStyle->setEnabled(true); // NOT pInputEnabled && : not true input
    return;
  }
  // multiple items selected
  u->lblPreview->setPixmap(QPixmap(":/pics/nopreview.png"));
  u->txtPreviewLatex->setText(tr("[ %n Items Selected ]", 0, entrylist.size()));
  u->cbxTags->setEditText(tr("[ Multiple Items Selected ]"));
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
  u->cbxCategory->setEditText(cat);
  if ( allsamestyle ) {
    pCurrentStyle = style;
    u->lblStylePreview->setText(prettyPrintStyle(style));
    u->btnRestoreStyle->setEnabled(true); // NOT pInputEnabled && : not true input
  } else {
    pCurrentStyle = KLFStyle();
    u->lblStylePreview->setText(tr("[ Different Styles ]"));
    u->btnRestoreStyle->setEnabled(false);
  }

  u->cbxCategory->setEnabled(pInputEnabled && true);
  u->btnUpdateCategory->setEnabled(pInputEnabled && true);
  u->cbxTags->setEnabled(pInputEnabled && false);
  u->btnUpdateTags->setEnabled(pInputEnabled && false);
}

void KLFLibEntryEditor::setInputEnabled(bool enabled)
{
  pInputEnabled = enabled;
}



void KLFLibEntryEditor::slotUpdateFromCbx(QComboBox *cbx)
{
  if (cbx == u->cbxCategory)
    slotUpdateCategory();
  else if (cbx == u->cbxTags)
    slotUpdateTags();
  else
    qWarning("KLFLibEntryEditor::slotUpdateFromCbx: Couldn't find combo box=%p", (void*)cbx);
}

void KLFLibEntryEditor::slotUpdateCategory()
{
  slotCbxSaveCurrentCompletion(u->cbxCategory);
  emit categoryChanged(u->cbxCategory->currentText());
}
void KLFLibEntryEditor::slotUpdateTags()
{
  slotCbxSaveCurrentCompletion(u->cbxTags);
  emit tagsChanged(u->cbxTags->currentText());
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
  klfDbg( "updateEditText("<<newtext<<")!" ) ;
  // small utility function that updates text if it isn't already the same text
  if (editWidget->currentText() != newText)
    editWidget->setEditText(newText);
}
*/
