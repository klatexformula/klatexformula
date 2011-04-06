/***************************************************************************
 *   file klflibentryeditor.cpp
 *   This file is part of the KLatexFormula Project.
 *   Copyright (C) 2011 by Philippe Faist
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

#include <klfdisplaylabel.h>

#include "klfconfig.h"
#include "klflib.h"
#include "klflatexedit.h"

#include <ui_klflibentryeditor.h>
#include "klflibentryeditor.h"





KLFLibEntryEditor::KLFLibEntryEditor(QWidget *parent)
  : QWidget(parent), pInputEnabled(true)
{
  u = new Ui::KLFLibEntryEditor;
  u->setupUi(this);
  setAutoFillBackground(false);

  pCurrentStyle = KLFStyle();

  u->lblPreview->setLabelFixedSize(klfconfig.UI.labelOutputFixedSize);

  u->cbxCategory->setInsertPolicy(QComboBox::InsertAlphabetically);
  u->cbxCategory->setDuplicatesEnabled(false);
  u->cbxTags->setInsertPolicy(QComboBox::InsertAlphabetically);
  u->cbxTags->setDuplicatesEnabled(false);

  u->cbxCategory->installEventFilter(this);
  u->cbxTags->installEventFilter(this);

  u->cbxCategory->addItem("");
  u->cbxTags->addItem("");

  // do NOT automatically apply changes, rather enable the 'apply changes' button
  //  connect(u->cbxCategory, SIGNAL(activated(int)), this, SLOT(slotApplyChanges()));
  //  connect(u->cbxTags, SIGNAL(activated(int)), this, SLOT(slotApplyChanges()));
  connect(u->cbxCategory, SIGNAL(activated(int)), this, SLOT(slotModified()));
  connect(u->cbxCategory, SIGNAL(editTextChanged(const QString&)), this, SLOT(slotModified()));
  connect(u->cbxTags, SIGNAL(activated(int)), this, SLOT(slotModified()));
  connect(u->cbxTags, SIGNAL(editTextChanged(const QString&)), this, SLOT(slotModified()));

  // preview and latexpreview should be as small as possible (while
  // still respecting their minimum sizes...)
  u->splitEntryEditor->setSizes(QList<int>() << 100 << 1000);

  // setup latex preview / preamble preview text browser
  u->txtPreviewLatex->setFont(klfconfig.UI.preambleEditFont);
  u->txtStyPreamble->setFont(klfconfig.UI.preambleEditFont);
  u->txtStyPreamble->setHeightHintLines(4);

  KLF_CONNECT_CONFIG_SH_LATEXEDIT(u->txtPreviewLatex) ;
  KLF_CONNECT_CONFIG_SH_LATEXEDIT(u->txtStyPreamble) ;
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
  // refresh the display label's glow effect
  /** \bug .... find a better way to synchronize config values like this. ....... */
  u->lblPreview->setGlowEffect(klfconfig.UI.glowEffect);
  u->lblPreview->setGlowEffectColor(klfconfig.UI.glowEffectColor);
  u->lblPreview->setGlowEffectRadius(klfconfig.UI.glowEffectRadius);

  u->cbxCategory->lineEdit()->setReadOnly(!pInputEnabled);
  u->cbxTags->lineEdit()->setReadOnly(!pInputEnabled);
  if (entrylist.size() == 0) {
    u->lblPreview->display(QImage(":/pics/nopreview.png"), QImage(), false);
    u->txtPreviewLatex->setText(tr("[ No Item Selected ]"));
    u->cbxCategory->setEditText(tr("[ No Item Selected ]"));
    u->cbxTags->setEditText(tr("[ No Item Selected ]"));
    //    u->lblStylePreview->setText(tr("[ No Item Selected ]"));
    u->cbxCategory->setEnabled(false);
    u->cbxTags->setEnabled(false);
    u->btnApplyChanges->setEnabled(false);
    u->btnRestoreStyle->setEnabled(false);
    pCurrentStyle = KLFStyle();
    displayStyle(false, KLFStyle());
    u->lblStyMathMode->setText(tr("[ No Item Selected ]"));
    u->txtStyPreamble->setPlainText(tr("[ No Item Selected ]"));
    slotModified(false);
    return;
  }
  if (entrylist.size() == 1) {
    KLFLibEntry e = entrylist[0];
    QImage img = e.preview();
    u->lblPreview->display(img, img, true);
    u->txtPreviewLatex->setText(e.latex());
    u->cbxCategory->setEditText(e.category());
    u->cbxTags->setEditText(e.tags());
    pCurrentStyle = e.style();
    //    u->lblStylePreview->setText(prettyPrintStyle(pCurrentStyle));
    u->cbxCategory->setEnabled(true);
    u->cbxTags->setEnabled(true);
    u->btnApplyChanges->setEnabled(pInputEnabled && true);
    u->btnRestoreStyle->setEnabled(true); // NOT pInputEnabled && : not true input
    displayStyle(true, pCurrentStyle);
    slotModified(false);
    return;
  }
  // multiple items selected
  u->lblPreview->display(QImage(":/pics/nopreview.png"), QImage(), false);
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
    displayStyle(true, pCurrentStyle);
    u->btnRestoreStyle->setEnabled(true); // NOT pInputEnabled && : not true input
  } else {
    pCurrentStyle = KLFStyle();
    displayStyle(false, KLFStyle());
    u->lblStyMathMode->setText(tr("[ Different Styles ]"));
    u->txtStyPreamble->setPlainText(tr("[ Different Styles ]"));
    u->btnRestoreStyle->setEnabled(false);
  }

  u->cbxCategory->setEnabled(pInputEnabled && true);
  u->cbxTags->setEnabled(pInputEnabled && false);
  u->btnApplyChanges->setEnabled(pInputEnabled && true);
  slotModified(false);
}

// private
void KLFLibEntryEditor::displayStyle(bool valid, const KLFStyle& style)
{
  if (valid) {
    u->lblStyDPI->setText(QString::number(style.dpi));
    QPixmap pxfg(16, 16);
    pxfg.fill(QColor(style.fg_color));
    u->lblStyColFg->setPixmap(pxfg);
    if (qAlpha(style.bg_color)) {
      QPixmap pxbg(16, 16);
      pxbg.fill(QColor(style.bg_color));
      u->lblStyColBg->setPixmap(pxbg);
    } else {
      u->lblStyColBg->setPixmap(QPixmap(":pics/transparenticon16.png"));
    }
    u->lblStyMathMode->setText(style.mathmode);
    u->txtStyPreamble->setPlainText(style.preamble);
  } else {
    u->lblStyDPI->setText(QLatin1String("-"));
    u->lblStyColFg->setText(QString());
    u->lblStyColFg->setPixmap(QPixmap());
    u->lblStyColBg->setText(QString());
    u->lblStyColBg->setPixmap(QPixmap());
    u->lblStyMathMode->setText(QString());
    u->txtStyPreamble->setPlainText(QString());
  }
}


void KLFLibEntryEditor::setInputEnabled(bool enabled)
{
  pInputEnabled = enabled;
}

void KLFLibEntryEditor::slotModified(bool modif)
{
  pMetaInfoModified = modif;
  u->btnApplyChanges->setEnabled(pMetaInfoModified);
}


void KLFLibEntryEditor::slotUpdateFromCbx(QComboBox *cbx)
{
  // Apply all changes, this is more natural than applying only the changes
  // to the current cbx (and losing the changes to the other)
  if (cbx == u->cbxCategory)
    //    slotApplyChanges(true, false);
    slotApplyChanges();
  else if (cbx == u->cbxTags)
    //    slotApplyChanges(false, true);
    slotApplyChanges();
  else
    qWarning("KLFLibEntryEditor::slotUpdateFromCbx: Couldn't find combo box=%p", (void*)cbx);
}

void KLFLibEntryEditor::on_btnApplyChanges_clicked()
{
  slotApplyChanges(u->cbxCategory->isEnabled(), u->cbxTags->isEnabled());
}
void KLFLibEntryEditor::slotApplyChanges(bool cat, bool tags)
{
  klfDbg("category="<<cat<<" tags="<<tags) ;
  QMap<int,QVariant> data;
  if (cat && u->cbxCategory->isEnabled()) {
    slotCbxSaveCurrentCompletion(u->cbxCategory);
    data[KLFLibEntry::Category] = u->cbxCategory->currentText();
  }
  if (tags && u->cbxTags->isEnabled()) {
    slotCbxSaveCurrentCompletion(u->cbxTags);
    data[KLFLibEntry::Tags] = u->cbxTags->currentText();
  }
  klfDbg("data to update: "<<data) ;
  if (data.isEmpty())
    return;

  emit metaInfoChanged(data);
}

void KLFLibEntryEditor::on_btnRestoreStyle_clicked()
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
