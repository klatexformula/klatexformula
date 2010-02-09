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

  pUi->lblPreview->setFixedSize(klfconfig.UI.labelOutputFixedSize);

  // preview and latexpreview should be as small as possible (while
  // still respecting their minimum sizes...)
  pUi->splitEntryEditor->setSizes(QList<int>() << 100 << 1000);

  // connect signals & stuff.......... TODO ...........
}
KLFLibEntryEditor::~KLFLibEntryEditor()
{
  delete pUi;
}


void KLFLibEntryEditor::displayEntry(const KLFLibEntry& entry)
{
  displayEntries(QList<KLFLibEntry>() << entry);
}

void KLFLibEntryEditor::displayEntries(const QList<KLFLibEntry>& entrylist)
{
  // ...... TODO ........... : UPDATE WHETHER BUTTONS ARE ENABLED

  if (entrylist.size() == 0) {
    pUi->lblPreview->setPixmap(QPixmap(":/pics/nopreview.png"));
    pUi->txtPreviewLatex->setText(tr("[ No Item Selected ]"));
    pUi->cbxCategory->setEditText(tr("[ No Item Selected ]"));
    pUi->cbxTags->setEditText(tr("[ No Item Selected ]"));
    pUi->lblStylePreview->setText(tr("[ No Item Selected ]"));
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
    pUi->lblStylePreview->setText(prettyPrintStyle(e.style()));
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
  if ( allsamestyle )
    pUi->lblStylePreview->setText(prettyPrintStyle(style));
  else
    pUi->lblStylePreview->setText(tr("[ Different Styles ]"));
}


