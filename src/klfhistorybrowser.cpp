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
#include <qtoolbutton.h>
#include <qregexp.h>
#include <qtooltip.h>
#include <qlistview.h>
#include <qheader.h>

#include <kapplication.h>
#include <kpopupmenu.h>
#include <kmessagebox.h>
#include <klocale.h>
#include <klineedit.h>
#include <kstandarddirs.h>
#include <kconfig.h>

#include "klfhistorybrowser.h"


#define CONFIGMENU_DISPLAYTAGGEDONLY_ID 9336 /* some arbitrary number */

KLFHistoryListViewItem::KLFHistoryListViewItem(QListView *v, KLFData::KLFHistoryItem h, int index) : QListViewItem(v)
{
  _h = h;
  _ind = index;

  setPixmap(0, _h.preview);
  QString l = _h.latex;
  l.replace("\n", "\t");
  setText(1, l);
  //  setText(2, _h.datetime.toString("yyyy-MM-dd hh:mm:ss")); // this is a cheat to sort the list correctly.
}

KLFHistoryListViewItem::~KLFHistoryListViewItem()
{
}





// ------------------------------------------------------------




KLFHistoryBrowser::KLFHistoryBrowser(KLFData::KLFHistoryList *ptr, QWidget* parent)
  : KLFHistoryBrowserUI(parent, 0, false)
{
  _histptr = ptr;

  _search_str = QString("");
  _search_k = 0;

  KConfig *cfg = kapp->config();

  cfg->setGroup("HistoryBrowser");
  _displaytaggedonly = cfg->readBoolEntry("displaytaggedonly", false);

  historyChanged();


  setModal(false);
  
  btnRestoreAll->setIconSet(QPixmap(locate("appdata", "pics/restoreall.png")));
  btnRestoreLatex->setIconSet(QPixmap(locate("appdata", "pics/restore.png")));
  btnClose->setIconSet(QPixmap(locate("appdata", "pics/closehide.png")));
  btnDelete->setIconSet(QPixmap(locate("appdata", "pics/delete.png")));
  btnConfig->setIconSet(QPixmap(locate("appdata", "pics/settings.png")));
  btnConfig->setText("");
  
  btnSearchClear->setPixmap(QPixmap(locate("appdata", "pics/clearright.png")));

  QString msg = i18n("Right click for context menu, double-click to restore LaTeX formula.");
  QToolTip::add(lstHistory, msg);
  QToolTip::add(lstHistory->viewport(), msg);

  lstHistory->setSorting(-1);
  lstHistory->setColumnWidth(2, 0);
  //  lstHistory->header()->setResizeEnabled(false, 2);

  _allowrestore = _allowdelete = false;

  mConfigMenu = new KPopupMenu(this);
  mConfigMenu->setCheckable(true);
  mConfigMenu->insertTitle(i18n("Configure display"), 1000, 0);
  mConfigMenu->insertItem(i18n("Only display tagged items"), this, SLOT(slotDisplayTaggedOnly()), 0, CONFIGMENU_DISPLAYTAGGEDONLY_ID);
  mConfigMenu->setItemChecked(CONFIGMENU_DISPLAYTAGGEDONLY_ID, _displaytaggedonly);

  btnConfig->setPopup(mConfigMenu);

  connect(lstHistory, SIGNAL(contextMenuRequested(QListViewItem *, const QPoint&, int)),
	  this, SLOT(slotContextMenu(QListViewItem*, const QPoint&, int)));
  connect(btnRestoreAll, SIGNAL(clicked()), this, SLOT(slotRestoreAll()));
  connect(btnRestoreLatex, SIGNAL(clicked()), this, SLOT(slotRestoreLatex()));
  connect(btnDelete, SIGNAL(clicked()), this, SLOT(slotDelete()));
  connect(btnClose, SIGNAL(clicked()), this, SLOT(slotClose()));

  connect(lstHistory, SIGNAL(currentChanged(QListViewItem*)), this, SLOT(slotRefreshButtonsEnabled()));
  connect(lstHistory, SIGNAL(selectionChanged()), this, SLOT(slotRefreshButtonsEnabled()));
  connect(lstHistory, SIGNAL(doubleClicked(QListViewItem*, const QPoint&, int)),
	  this, SLOT(slotRestoreAll(QListViewItem*)));
  connect(lstHistory, SIGNAL(returnPressed(QListViewItem*)),
	  this, SLOT(slotRestoreAll(QListViewItem*)));

  connect(btnSearchClear, SIGNAL(clicked()), this, SLOT(slotSearchClear()));
  connect(lneSearch, SIGNAL(textChanged(const QString&)), this, SLOT(slotSearchFind(const QString&)));
  connect(lneSearch, SIGNAL(lostFocus()), this, SLOT(resetLneSearchColor()));

  lneSearch->installEventFilter(this);

  _dflt_lineedit_bgcol = lneSearch->paletteBackgroundColor();
}

KLFHistoryBrowser::~KLFHistoryBrowser()
{
}


void KLFHistoryBrowser::reject()
{
  // this is called when "ESC" is pressed.
  // ignore event. We don't want to quit unless user selects "Close" and accept(),
  // possibly the user selects "X" on the window title bar. In this last case, it works even
  // with this reimplementation of reject() (Qt calls hide()).
}

bool KLFHistoryBrowser::eventFilter(QObject *obj, QEvent *ev)
{
  if (obj == lneSearch) {
    if (ev->type() == QEvent::KeyPress) {
      QKeyEvent *kev = (QKeyEvent*)ev;
      if (kev->key() == Key_F3) { // find next
	slotSearchFindNext();
	return true;
      }
      if (kev->key() == Key_Escape) { // abort search
	_search_str = QString("\024\015\077\073\017__badstr__THIS_###_WONT_BE FOUND\033\025\074\057");
	_search_k = 0;
	slotSearchFindNext();
	lneSearch->setText(""); // reset search
	resetLneSearchColor();
      }
    }
  }
  return KLFHistoryBrowserUI::eventFilter(obj, ev);
}


// small utility function to determine if our latex code is "tagged with keyword"
bool is_tagged(QString latex)
{
  return latex.find(QRegExp("^\\s*\\%\\s*\\S+")) != -1 ;
}

void KLFHistoryBrowser::historyChanged()
{
  lstHistory->clear();
  for (uint i = 0; i < _histptr->size(); ++i) {
    // display if [ "show all" OR "this one is tagged" ]
    if (_displaytaggedonly == false || is_tagged(_histptr->operator[](i).latex)) {
      (void) new KLFHistoryListViewItem(lstHistory, _histptr->operator[](i), i);
    }
  }

}

void KLFHistoryBrowser::addToHistory(KLFData::KLFHistoryItem h)
{
  _histptr->append(h);
  if (_displaytaggedonly == false || is_tagged(h.latex)) {
    (void) new KLFHistoryListViewItem(lstHistory, h, _histptr->size()-1);
  }
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
  menu->setCheckable(true);
  menu->insertTitle(i18n("Actions"));
  int idr1 = menu->insertItem(QIconSet(locate("appdata", "pics/restoreall.png")), i18n("Restore latex formula and style"),
			      this, SLOT(slotRestoreAll()));
  int idr2 = menu->insertItem(QIconSet(locate("appdata", "pics/restore.png")), i18n("Restore latex formula only"),
			      this, SLOT(slotRestoreLatex()));
  menu->insertSeparator();
  int iddel = menu->insertItem(QIconSet(locate("appdata", "pics/delete.png")), i18n("Delete from history"),
			       this, SLOT(slotDelete()));
  menu->insertSeparator();


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

void KLFHistoryBrowser::slotDisplayTaggedOnly()
{
  slotDisplayTaggedOnly(!_displaytaggedonly);
}
void KLFHistoryBrowser::slotDisplayTaggedOnly(bool display)
{
  _displaytaggedonly = display;
  mConfigMenu->setItemChecked(CONFIGMENU_DISPLAYTAGGEDONLY_ID, display);
  historyChanged(); // force a complete refresh
  // save setting
  KConfig *cfg = kapp->config();
  cfg->setGroup("HistoryBrowser");
  cfg->writeEntry("displaytaggedonly", display);
}

void KLFHistoryBrowser::slotClose()
{
  hide();
  emit refreshHistoryBrowserShownState(false);
}

void KLFHistoryBrowser::closeEvent(QCloseEvent *e)
{
  e->accept();
  emit refreshHistoryBrowserShownState(false);
}


void KLFHistoryBrowser::slotSearchClear()
{
  lneSearch->setText("");
  lneSearch->setFocus();
}

void KLFHistoryBrowser::slotSearchFind(const QString& s)
{
  _search_str = s;
  _search_k = _histptr->size();
  slotSearchFindNext();
}
void KLFHistoryBrowser::slotSearchFindNext()
{
  QString s = _search_str;

  if (s.isEmpty()) {
    resetLneSearchColor();
    return;
  }

  // search _histptr for occurence
  bool found = false;
  int k;
  for (k = _search_k-1 /*_histptr->size()-1*/; !found && k >= 0; --k) { // reverse search, find most recent first
    int i = _histptr->operator[](k).latex.find(s, 0, false); // case insensitive search
    if (i != -1) {
      KLFHistoryListViewItem *item = itemForId(_histptr->operator[](k).id);
      if (item != 0) {
	// we found our item
	// 'item' points to our item
	lstHistory->ensureItemVisible(item);
	QListView::SelectionMode selmode = lstHistory->selectionMode();
	lstHistory->setSelectionMode(QListView::Single);
	lstHistory->setSelected(item, true);
	lstHistory->setSelectionMode(selmode);
	lstHistory->setCurrentItem(item);
	found = true;
	break;
      }
    }
  }
  if (found) {
    lneSearch->setPaletteBackgroundColor(QColor(128, 255, 128));
    _search_k = k;
  } else {
    _search_k = _histptr->size(); // restart from end

    KLFHistoryListViewItem *item = (KLFHistoryListViewItem*) lstHistory->firstChild();
    if (item) {
      // small tweaks that work ok
      QListView::SelectionMode selmode = lstHistory->selectionMode();
      lstHistory->setSelectionMode(QListView::Single);
      lstHistory->setSelected(item, true);
      lstHistory->setSelectionMode(selmode);
      lstHistory->ensureItemVisible(item);
      lstHistory->setSelected(item, false);
      lstHistory->setCurrentItem(lstHistory->lastItem());
    } else {
      // this shouldn't happen, but anyway, just in case
      lstHistory->clearSelection();
    }
    lneSearch->setPaletteBackgroundColor(QColor(255,128,128));
  }
}

void KLFHistoryBrowser::resetLneSearchColor()
{
  lneSearch->setPaletteBackgroundColor(_dflt_lineedit_bgcol);
}


KLFHistoryListViewItem *KLFHistoryBrowser::itemForId(uint reqid)
{
  QListViewItemIterator it(lstHistory);
  KLFHistoryListViewItem *item;
  while (it.current()) {
    item = (KLFHistoryListViewItem *)it.current();
    if (item && item->historyItem().id == reqid)
	  return item;
    ++it;
  }
  return 0;
}


#include "klfhistorybrowser.moc"
