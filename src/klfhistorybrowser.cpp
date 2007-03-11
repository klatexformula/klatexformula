/***************************************************************************
 *   file klfhistorybrowser.cpp
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

#include <qpushbutton.h>

#include <kpopupmenu.h>
#include <kmessagebox.h>
#include <klocale.h>

#include "klfhistorybrowser.h"


KLFHistoryListViewItem::KLFHistoryListViewItem(QListView *v, KLFData::KLFHistoryItem h, int index) : QListViewItem(v)
{
  _h = h;
  _ind = index;

  setText(0, _h.datetime.toString("yyyy-MM-dd hh:mm:ss"));
  setPixmap(1, _h.preview);
  setText(2, _h.latex);
}

KLFHistoryListViewItem::~KLFHistoryListViewItem()
{
}








KLFHistoryBrowser::KLFHistoryBrowser(KLFData::KLFHistoryList *ptr, QWidget* parent)
  : KLFHistoryBrowserUI(parent, 0, false)
{
  _histptr = ptr;
  historyChanged();

  setModal(false);

  //  setWFlags( (getWFlags() & ~WType_Dialog) );

  _allowrestore = _allowdelete = false;

  connect(lstHistory, SIGNAL(contextMenuRequested(QListViewItem *, const QPoint&, int)),
	  this, SLOT(slotContextMenu(QListViewItem*, const QPoint&, int)));
  connect(btnRestoreAll, SIGNAL(clicked()), this, SLOT(slotRestoreAll()));
  connect(btnRestoreLatex, SIGNAL(clicked()), this, SLOT(slotRestoreLatex()));
  connect(btnDelete, SIGNAL(clicked()), this, SLOT(slotDelete()));
  connect(btnClose, SIGNAL(clicked()), this, SLOT(slotClose()));

  connect(lstHistory, SIGNAL(currentChanged(QListViewItem*)), this, SLOT(slotRefreshButtonsEnabled()));
  connect(lstHistory, SIGNAL(selectionChanged()), this, SLOT(slotRefreshButtonsEnabled()));
}

KLFHistoryBrowser::~KLFHistoryBrowser()
{
}


void KLFHistoryBrowser::historyChanged()
{
  lstHistory->clear();
  for (uint i = 0; i < _histptr->size(); ++i) {
    (void) new KLFHistoryListViewItem(lstHistory, _histptr->operator[](i), i);
  }
}

void KLFHistoryBrowser::addToHistory(KLFData::KLFHistoryItem h)
{
  _histptr->append(h);
  (void) new KLFHistoryListViewItem(lstHistory, h, _histptr->size()-1);
}

void KLFHistoryBrowser::slotRefreshButtonsEnabled()
{
  if (lstHistory->currentItem() != 0) {
    _allowrestore = true;
  } else {
    _allowrestore = false;
  }

  _allowdelete = false;
  QListViewItemIterator it(lstHistory);
  while (it.current()) {
    if (it.current()->isSelected()) {
      _allowdelete = true;
      break;
    }
    ++it;
  }

  btnRestoreAll->setEnabled(_allowrestore);
  btnRestoreLatex->setEnabled(_allowrestore);
  btnDelete->setEnabled(_allowdelete);

}

void KLFHistoryBrowser::slotContextMenu(QListViewItem */*item*/, const QPoint &p, int /*column*/)
{
  KPopupMenu *menu = new KPopupMenu(this);
  menu->insertTitle(i18n("Actions"));
  int idr1 = menu->insertItem(i18n("Restore latex formula and style"), this, SLOT(slotRestoreAll()));
  int idr2 = menu->insertItem(i18n("Restore latex formula only"), this, SLOT(slotRestoreLatex()));
  menu->insertSeparator();
  int iddel = menu->insertItem(i18n("Delete from history"), this, SLOT(slotDelete()));

  menu->setItemEnabled(idr1, _allowrestore);
  menu->setItemEnabled(idr2, _allowrestore);
  menu->setItemEnabled(iddel, _allowdelete);

  menu->popup(p);
}


void KLFHistoryBrowser::slotRestoreAll(QListViewItem *it)
{
  if (it == 0)
    it = lstHistory->currentItem();
  if (it == 0)
    return;

  emit restoreFromHistory( ((KLFHistoryListViewItem*)it)->historyItem(), true);
}

void KLFHistoryBrowser::slotRestoreLatex(QListViewItem *it)
{
  if (it == 0)
    it = lstHistory->currentItem();
  if (it == 0)
    return;

  emit restoreFromHistory( ((KLFHistoryListViewItem*)it)->historyItem(), false);
}

void KLFHistoryBrowser::slotDelete(QListViewItem *it)
{
  if (it == 0) {
    int r = KMessageBox::questionYesNo(this, i18n("Are you sure you want to delete selected item(s) from history?"),
				       i18n("delete from history?"));
    if (r != KMessageBox::Yes)
      return;

    // get selection. For that, iterate all items and check their selection state.
    QListViewItemIterator it(lstHistory);

    while (it.current()) {
      if (it.current()->isSelected()) {
	uint k;
	ulong id = ((KLFHistoryListViewItem*)it.current())->historyItem().id;
	for (k = 0; k < _histptr->size() && _histptr->operator[](k).id != id; ++k);
	if (k == _histptr->size()) {
	  fprintf(stderr, "ERROR: NASTY: Can't find index for deletion!!!\n");
	  return;
	}
	_histptr->erase(_histptr->at(k));
	QListViewItem *ptr = it.current();
	++it;
	delete ptr;
      } else {
	++it;
      }
    }
  } else {
    _histptr->erase(_histptr->at( ((KLFHistoryListViewItem*)it)->index() ));
  }

  historyChanged();
}

void KLFHistoryBrowser::slotClose()
{
  hide();
  emit refreshHistoryBrowserShownState(false);
}

void KLFHistoryBrowser::closeEvent(QCloseEvent *e)
{
  //  hide();
  e->accept();
  emit refreshHistoryBrowserShownState(false);
}


#include "klfhistorybrowser.moc"
