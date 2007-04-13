/***************************************************************************
 *   file klfhistorybrowser.h
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

#ifndef KLFHISTORYBROWSER_H
#define KLFHISTORYBROWSER_H


#include <qwidget.h>
#include <qlistview.h>
#include <qevent.h>

#include <kpopupmenu.h>

#include <klfdata.h>
#include <klfhistorybrowserui.h>




class KLFHistoryListViewItem : public QListViewItem
{
public:
  KLFHistoryListViewItem(QListView *parent, KLFData::KLFHistoryItem h, int ind);
  virtual ~KLFHistoryListViewItem();

  KLFData::KLFHistoryItem historyItem() const { return _h; }
  int index() const { return _ind; }

protected:
  KLFData::KLFHistoryItem _h;
  int _ind;
};


// -------------------------------------------

class KLFHistoryBrowser : public KLFHistoryBrowserUI
{
  Q_OBJECT
public:
  KLFHistoryBrowser(KLFData::KLFHistoryList *histptr, QWidget *parent);
  ~KLFHistoryBrowser();

  bool eventFilter(QObject *obj, QEvent *e);

signals:
  void restoreFromHistory(KLFData::KLFHistoryItem h, bool restorestyle);
  void refreshHistoryBrowserShownState(bool shown);

public slots:

  // typically called by mainwin
  void historyChanged();
  void addToHistory(KLFData::KLFHistoryItem h);

  // typically called by UI
  void slotContextMenu(QListViewItem *item, const QPoint &p, int column);
  void slotRestoreAll(QListViewItem *i = 0);
  void slotRestoreLatex(QListViewItem *i = 0);
  void slotDelete(QListViewItem *i = 0);
  void slotDisplayTaggedOnly();
  void slotDisplayTaggedOnly(bool display);
  void slotClose();
  void slotRefreshButtonsEnabled();
  // search
  void slotSearchClear();
  void slotSearchFind(const QString& text);
  void slotSearchFindNext();

protected slots:

  void resetLneSearchColor();

protected:

  void reject();
  void closeEvent(QCloseEvent *e);

private:

  bool _allowrestore;
  bool _allowdelete;

  bool _displaytaggedonly;

  KLFData::KLFHistoryList *_histptr;

  KLFHistoryListViewItem *itemForId(uint id);

  KPopupMenu *mConfigMenu;

  uint _search_k; // we're focusing on the k-th history item (decreases at each F3 key press [reverse search, remember!])
  QString _search_str; // the previous search string

  QColor _dflt_lineedit_bgcol;
};

#endif

