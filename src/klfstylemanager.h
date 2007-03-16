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

#include <qdragobject.h>
#include <qevent.h>
#include <qlistbox.h>

#include <kpopupmenu.h>

#include <klfdata.h>
#include <klfstylemanagerui.h>


class KLFStyleDrag : public QDragObject
{
  Q_OBJECT
public:
  KLFStyleDrag(KLFData::KLFStyle sty, QWidget *parent = 0);

  const char *format(int index) const;

  QByteArray encodedData(const char *format) const;

  static bool canDecode(const QMimeSource *source);
  static bool decode(const QMimeSource *source, KLFData::KLFStyle& sty);
private:
  KLFData::KLFStyle _sty;
};

class KLFStyleManager : public KLFStyleManagerUI
{
  Q_OBJECT
public:
  KLFStyleManager(KLFData::KLFStyleList *ptr, QWidget *parent = 0);
  ~KLFStyleManager();

  bool eventFilter(QObject *obj, QEvent *event);

signals:

  void refreshStyles();

public slots:

  void stylesChanged();

  void slotDelete();
  void slotMoveUp();
  void slotMoveDown();
  void slotRename();

  void refreshActionsEnabledState();
  void showActionsContextMenu(QListBoxItem *item, const QPoint& pos);

protected:

  enum { PopupIdDelete = 1001, PopupIdMoveUp, PopupIdMoveDown, PopupIdRename };

private:
  KLFData::KLFStyleList *_styptr;

  KPopupMenu *mActionsPopup;

  QPoint _drag_init_pos;
  QListBoxItem *_drag_item;
  /*  QListBoxItem *mDropIndicatorItem; */
};

#endif

