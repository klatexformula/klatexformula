/***************************************************************************
 *   file klfstylemanager.cpp
 *   This file is part of the KLatexFormula Project.
 *   Copyright (C) 2011 by Philippe Faist
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
/* $Id$ */

#include <stdio.h>
#include <string.h>

#include <QApplication>
#include <QList>
#include <QPushButton>
#include <QMessageBox>
#include <QInputDialog>
#include <QDrag>
#include <QMimeData>

#include <ui_klfstylemanager.h>

#include "klfstylemanager.h"



Qt::ItemFlags KLFStyleListModel::flags(const QModelIndex& index) const
{
  if (!index.isValid() || index.row() >= rowCount() || index.model() != this)
      return Qt::ItemIsDropEnabled; // we allow drops outside the items
  return Qt::ItemIsSelectable | Qt::ItemIsEnabled | Qt::ItemIsDragEnabled;
}

QString KLFStyleListModel::styleName(int row) const
{
  return data(index(row), Qt::DisplayRole).toString();
}

void KLFStyleListModel::setStyleName(int row, const QString& newname)
{
  setData(index(row), newname, Qt::DisplayRole);
}


Qt::DropActions KLFStyleListModel::supportedDropActions() const
{
  return Qt::CopyAction;
}

QStringList KLFStyleListModel::mimeTypes() const
{
  QStringList types;
  types << "application/x-klf-stylename";
  return types;
}

QMimeData *KLFStyleListModel::mimeData(const QModelIndexList& indexes) const
{
  QMimeData *mimeData = new QMimeData();

  // can only drag ONE stylename
  if (indexes.size() > 1 || indexes.size() <= 0) {
    return mimeData;
  }

  QByteArray encodedData;
  QDataStream stream(&encodedData, QIODevice::WriteOnly);
  stream << styleName(indexes[0].row());
  mimeData->setData("application/x-klf-stylename", encodedData);
  return mimeData;
}

bool KLFStyleListModel::dropMimeData(const QMimeData *mdata, Qt::DropAction action, int row, int column,
				     const QModelIndex &parent)
{
  if (action == Qt::IgnoreAction)
    return true;

  if (parent.isValid())
    return false;

  if (!mdata->hasFormat("application/x-klf-stylename"))
    return false;

  if (column > 0)
    return false;

  if (row == -1)
    row = rowCount();

  QByteArray encodedData = mdata->data("application/x-klf-stylename");
  QDataStream stream(&encodedData, QIODevice::ReadOnly);
  QString newItem;

  stream >> newItem;

  // find style already existant in this list
  int k;
  for (k = 0; k < rowCount() && styleName(k) != newItem; ++k)
    ;
  if (k >= rowCount()) {
    fprintf(stderr, "WARNING: Ignoring drop of style named `%s' which was not already in list!\n",
	    newItem.toLocal8Bit().constData());
    return false;
  }
  // remove row at position k
  removeRows(k, 1);
  if (row > k)
    --row;
  // and insert our text at the right position
  insertRows(row, 1);
  setStyleName(row, newItem);

  emit internalMoveCompleted(k, row);

  return true;
}


// ------------------



KLFStyleManager::KLFStyleManager(KLFStyleList *stydata, QWidget *parent)
  : QWidget(parent, 
#ifdef KLF_WS_MAC
	    Qt::Sheet
#else
	    Qt::Dialog
#endif
	    )
{
  u = new Ui::KLFStyleManager;
  u->setupUi(this);
  setObjectName("KLFStyleManager");

  _styptr = stydata;

  _drag_item = 0;
  _drag_init_pos = QPoint(-1,-1);
  /*  mDropIndicatorItem = 0; */

  mActionsPopup = new QMenu(this);

  /** \todo ............ DYNAMIC LANGUAGE CHANGE STRINGS .................. */

  actPopupDelete = mActionsPopup->addAction("", this, SLOT(slotDelete()));
  actPopupMoveUp = mActionsPopup->addAction("", this, SLOT(slotMoveUp()));
  actPopupMoveDown = mActionsPopup->addAction("", this, SLOT(slotMoveDown()));
  actPopupRename = mActionsPopup->addAction("", this, SLOT(slotRename()));
  u->btnActions->setMenu(mActionsPopup);

  mStyleListModel = new KLFStyleListModel(this);
  u->lstStyles->setModel(mStyleListModel);

  // populate style list
  slotRefresh();

  // and set menu items enabled or not
  refreshActionsEnabledState();

  connect(u->btnClose, SIGNAL(clicked()), this, SLOT(hide()));

  u->lstStyles->installEventFilter(this);
  connect(u->lstStyles, SIGNAL(customContextMenuRequested(const QPoint&)),
	  this, SLOT(showActionsContextMenu(const QPoint&)));
  connect(mStyleListModel, SIGNAL(internalMoveCompleted(int, int)),
	  this, SLOT(slotModelMoveCompleted(int, int)));
  connect(u->lstStyles->selectionModel(),
	  SIGNAL(selectionChanged(const QItemSelection&, const QItemSelection&)),
	  this, SLOT(refreshActionsEnabledState()));

  retranslateUi(false);
}


void KLFStyleManager::retranslateUi(bool alsoBaseUi)
{
  KLF_DEBUG_TIME_BLOCK(KLF_FUNC_NAME);
  if (alsoBaseUi)
    u->retranslateUi(this);

  actPopupDelete->setText(tr("Delete Style"));
  actPopupMoveUp->setText(tr("Move up"));
  actPopupMoveDown->setText(tr("Move down"));
  actPopupRename->setText(tr("Rename style"));
}

KLFStyleManager::~KLFStyleManager()
{
}

void KLFStyleManager::refreshActionsEnabledState()
{
  int curidx = currentRow();

  if (curidx != -1) {
    actPopupDelete->setEnabled(true);
    actPopupRename->setEnabled(true);
    actPopupMoveUp->setEnabled(curidx > 0);
    actPopupMoveDown->setEnabled(curidx < mStyleListModel->rowCount()-1);
  } else {
    actPopupDelete->setEnabled(false);
    actPopupRename->setEnabled(false);
    actPopupMoveUp->setEnabled(false);
    actPopupMoveDown->setEnabled(false);
  }
}

void KLFStyleManager::showActionsContextMenu(const QPoint& pos)
{
  mActionsPopup->exec(u->lstStyles->mapToGlobal(pos));
}


int KLFStyleManager::currentRow()
{
  QModelIndexList sel = u->lstStyles->selectionModel()->selectedRows();
  if (sel.size() == 0)
    return -1;
  if (sel.size() >= 2) {
    qWarning("Multiple style names selected! Expected Single Selection Policy!\n");
    return -1;
  }
  return sel[0].row();
}

void KLFStyleManager::slotDelete()
{
  int r = currentRow();
  if ( r == -1 )
    return;

  if ( QMessageBox::question(this, tr("Erase style?"), tr("Are you sure you want to erase selected style?"),
			     QMessageBox::Yes|QMessageBox::No, QMessageBox::No) == QMessageBox::Yes ) {
    _styptr->removeAt(r);
    mStyleListModel->removeRows(r, 1);
  }

  emit refreshStyles();
  refreshActionsEnabledState();
}

void KLFStyleManager::slotRename()
{
  int r = currentRow();
  if ( r == -1 )
    return;

  QString newname = QInputDialog::getText(this, tr("Rename style"), tr("Enter new style name:"), QLineEdit::Normal,
 					  _styptr->at(r).name);

  if ( ! newname.isEmpty() ) {
    _styptr->operator[](r).name = newname;
    mStyleListModel->setStyleName(r, newname);
  }

  emit refreshStyles();
  refreshActionsEnabledState();
}

void KLFStyleManager::slotMoveUp()
{
  int r = currentRow();
  if ( r < 1 || r >= mStyleListModel->rowCount() )
    return;

  QString s = mStyleListModel->styleName(r);
  mStyleListModel->setStyleName(r, mStyleListModel->styleName(r-1));
  mStyleListModel->setStyleName(r-1, s);
  slotModelMoveCompleted(r, r-1);

  emit refreshStyles();
  refreshActionsEnabledState();
}

void KLFStyleManager::slotMoveDown()
{
  int r = currentRow();
  if ( r < 0 || r > mStyleListModel->rowCount() - 1 )
    return;

  QString s = mStyleListModel->styleName(r);
  mStyleListModel->setStyleName(r, mStyleListModel->styleName(r+1));
  mStyleListModel->setStyleName(r+1, s);
  slotModelMoveCompleted(r, r+1);

  emit refreshStyles();
  refreshActionsEnabledState();
}


void KLFStyleManager::slotModelMoveCompleted(int prev, int newpos)
{
  KLFStyle sty = _styptr->takeAt(prev);
  _styptr->insert(newpos, sty);

  QModelIndex i = mStyleListModel->index(newpos);
  u->lstStyles->selectionModel()->select(i, QItemSelectionModel::ClearAndSelect);
  u->lstStyles->setCurrentIndex(i);

  emit refreshStyles();
  refreshActionsEnabledState();
}

void KLFStyleManager::slotRefresh()
{
  QStringList list;
  for (int i = 0; i < _styptr->size(); ++i) {
    list << _styptr->at(i).name;
  }
  mStyleListModel->setStringList(list);
}




