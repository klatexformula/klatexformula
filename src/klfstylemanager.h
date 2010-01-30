/***************************************************************************
 *   file klfstylemanager.h
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
/* $Id$ */

#ifndef KLFSTYLEMANAGER_H
#define KLFSTYLEMANAGER_H

#include <QWidget>
#include <QMenu>
#include <QListWidget>
#include <QStringListModel>

#include <klfdata.h>
#include <ui_klfstylemanagerui.h>


class KLFStyleListModel : public QStringListModel
{
  Q_OBJECT
public:
  KLFStyleListModel(QObject *parent = 0) : QStringListModel(parent) { }
  virtual ~KLFStyleListModel() { }

  virtual Qt::ItemFlags flags(const QModelIndex& index) const;

  virtual QString styleName(int row) const;
  virtual void setStyleName(int row, const QString& newname);

  Qt::DropActions supportedDropActions() const;
  QStringList mimeTypes() const;
  QMimeData *mimeData(const QModelIndexList& indexes) const;
  bool dropMimeData(const QMimeData *data, Qt::DropAction action, int row, int column, const QModelIndex &parent);

signals:
  void internalMoveCompleted(int prevrow, int newrow);
};


class KLFStyleManager : public QWidget, private Ui::KLFStyleManagerUI
{
  Q_OBJECT
public:
  KLFStyleManager(KLFStyleList *ptr, QWidget *parent);
  ~KLFStyleManager();

signals:
  void refreshStyles();

public slots:

  void slotRefresh();

  void slotDelete();
  void slotMoveUp();
  void slotMoveDown();
  void slotRename();

  void refreshActionsEnabledState();
  void showActionsContextMenu(const QPoint& pos);

protected slots:
  void slotModelMoveCompleted(int previouspos, int newpos);

private:
  KLFStyleList *_styptr;

  QMenu *mActionsPopup;

  QAction *actPopupDelete;
  QAction *actPopupMoveUp;
  QAction *actPopupMoveDown;
  QAction *actPopupRename;

  KLFStyleListModel *mStyleListModel;

  QPoint _drag_init_pos;
  QListWidgetItem *_drag_item;

  int currentRow();
};

#endif

