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

#ifndef KLFSTYLEMANAGER_H
#define KLFSTYLEMANAGER_H

#include <QWidget>
#include <QMenu>
#include <QListWidget>

#include <klfdata.h>
#include <ui_klfstylemanagerui.h>


class KLFStyleManager : public QWidget, private Ui::KLFStyleManagerUI
{
  Q_OBJECT
public:
  KLFStyleManager(KLFData::KLFStyleList *ptr, QWidget *parent);
  ~KLFStyleManager();

signals:

  void refreshStyles();

public slots:

  void stylesChanged();

  void slotDelete();
  void slotMoveUp();
  void slotMoveDown();
  void slotRename();

  void refreshActionsEnabledState();
  void showActionsContextMenu(const QPoint& pos);

private:
  KLFData::KLFStyleList *_styptr;

  QMenu *mActionsPopup;

  QAction *actPopupDelete;
  QAction *actPopupMoveUp;
  QAction *actPopupMoveDown;
  QAction *actPopupRename;


  QPoint _drag_init_pos;
  QListWidgetItem *_drag_item;
  /*  QListBoxItem *mDropIndicatorItem; */
};

#endif

