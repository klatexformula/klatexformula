/***************************************************************************
 *   file klfstylemanager.cpp
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

#include <string.h>

#include <QApplication>
#include <QList>
#include <QPushButton>
#include <QMessageBox>
#include <QInputDialog>
#include <QDrag>
#include <QMimeData>

#include "klfstylemanager.h"



KLFStyleManager::KLFStyleManager(KLFData::KLFStyleList *stydata, QWidget *parent)
  : QWidget(parent, Qt::Dialog), KLFStyleManagerUI()
{
  setupUi(this);

  _styptr = stydata;

  _drag_item = 0;
  _drag_init_pos = QPoint(-1,-1);
  /*  mDropIndicatorItem = 0; */

  mActionsPopup = new QMenu(this);

  actPopupDelete = mActionsPopup->addAction(tr("Delete Style"), this, SLOT(slotDelete()));
  actPopupMoveUp = mActionsPopup->addAction(tr("Move up"), this, SLOT(slotMoveUp()));
  actPopupMoveDown = mActionsPopup->addAction(tr("Move down"), this, SLOT(slotMoveDown()));
  actPopupRename = mActionsPopup->addAction(tr("Rename style"), this, SLOT(slotRename()));

  btnActions->setMenu(mActionsPopup);

  // populate style list
  stylesChanged();

  // and set menu items enabled or not
  refreshActionsEnabledState();

  connect(btnClose, SIGNAL(clicked()), this, SLOT(hide()));

  connect(lstStyles, SIGNAL(currentItemChanged(QListWidgetItem *, QListWidgetItem *)),
	  this, SLOT(refreshActionsEnabledState()));
  connect(lstStyles, SIGNAL(itemSelectionChanged()), this, SLOT(refreshActionsEnabledState()));
  connect(lstStyles, SIGNAL(customContextMenuRequested(const QPoint&)),
	  this, SLOT(showActionsContextMenu(const QPoint&)));
}

KLFStyleManager::~KLFStyleManager()
{
}

void KLFStyleManager::refreshActionsEnabledState()
{
  QList<QListWidgetItem*> selection = lstStyles->selectedItems();
  if (selection.size() > 0) {
    lstStyles->setCurrentItem(selection[0]);
  }
  QListWidgetItem *cur = lstStyles->currentItem();
  if (cur != 0) {
    actPopupDelete->setEnabled(true);
    actPopupRename->setEnabled(true);
    int row = lstStyles->row(cur);
    actPopupMoveUp->setEnabled(row > 0);
    actPopupMoveDown->setEnabled(row < lstStyles->count());
  } else {
    actPopupDelete->setEnabled(false);
    actPopupRename->setEnabled(false);
    actPopupMoveUp->setEnabled(false);
    actPopupMoveDown->setEnabled(false);
  }
}

void KLFStyleManager::showActionsContextMenu(const QPoint& pos)
{
  mActionsPopup->exec(pos);
}

void KLFStyleManager::slotDelete()
{
  QListWidgetItem *cur = lstStyles->currentItem();
  if ( cur != 0 &&
       QMessageBox::question(this, tr("Erase style?"), tr("Are you sure you want to erase selected style?"),
			     QMessageBox::Yes|QMessageBox::No, QMessageBox::No) == QMessageBox::Yes ) {
    _styptr->removeAt(lstStyles->row(cur));
    delete cur;
  }

  emit refreshStyles();
  refreshActionsEnabledState();
}

void KLFStyleManager::slotRename()
{
  QListWidgetItem *cur = lstStyles->currentItem();
  if (cur == 0)
    return;

  int ind = lstStyles->row(cur);

  QString newname = QInputDialog::getText(this, tr("Rename style"), tr("Enter new style name:"), QLineEdit::Normal,
					  _styptr->at(ind).name);

  if (!newname.isEmpty()) {
    _styptr->operator[](ind).name = newname;
    cur->setText(newname);
  }

  emit refreshStyles();
  refreshActionsEnabledState();
}

void KLFStyleManager::slotMoveUp()
{
  QListWidgetItem *cur = lstStyles->currentItem();
  if (cur == 0)
    return;

  int ind = lstStyles->row(cur);
  if (ind < 1)
    return;

  KLFData::KLFStyle s = _styptr->at(ind);
  _styptr->operator[](ind) = _styptr->at(ind-1);
  _styptr->operator[](ind-1) = s;

  lstStyles->item(ind-1)->setText(_styptr->at(ind-1).name);
  lstStyles->item(ind)->setText(_styptr->at(ind).name);
  lstStyles->setCurrentItem(lstStyles->item(ind-1), QItemSelectionModel::Select);

  emit refreshStyles();
  refreshActionsEnabledState();
}

void KLFStyleManager::slotMoveDown()
{
  QListWidgetItem *cur = lstStyles->currentItem();
  if (cur == 0)
    return;

  int ind = lstStyles->row(cur);
  if (ind >= lstStyles->count()-1)
    return;

  KLFData::KLFStyle s = _styptr->at(ind);
  _styptr->operator[](ind) = _styptr->at(ind+1);
  _styptr->operator[](ind+1) = s;

  lstStyles->item(ind+1)->setText(_styptr->at(ind+1).name);
  lstStyles->item(ind)->setText(_styptr->at(ind).name);
  lstStyles->setCurrentItem(lstStyles->item(ind+1), QItemSelectionModel::Select);

  emit refreshStyles();
  refreshActionsEnabledState();
}



void KLFStyleManager::stylesChanged()
{
  lstStyles->clear();
  for (int i = 0; i < _styptr->size(); ++i) {
    lstStyles->addItem(_styptr->at(i).name);
  }
}




