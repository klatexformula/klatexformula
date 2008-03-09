/***************************************************************************
 *   file klflibrary.cpp
 *   This file is part of the KLatexFormula Project.
 *   Copyright (C) 2007 by Philippe Faist
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

#include <qtooltip.h>
#include <qtabwidget.h>
#include <qtooltip.h>
#include <qpushbutton.h>
#include <qtoolbutton.h>
#include <qlabel.h>
#include <qtoolbox.h>
#include <qcombobox.h>
#include <qlayout.h>
#include <qiconset.h>
#include <qtimer.h>
#include <qregexp.h>

#include <klineedit.h>
#include <klocale.h>
#include <kstandarddirs.h>
#include <kmessagebox.h>

#include "klfconfig.h"
#include "klfdata.h"
#include "klfmainwin.h"
#include "klflibrary.h"




void dump_library_list(const KLFData::KLFLibraryList& list, const char *comment)
{
  uint k;
  printf("\n----------- LIBRARY RESOURCE [ %s ] ---------------\n", comment);
  for (k = 0; k < list.size(); ++k) {
    printf("#%03d: CAT='%s' TAGS='%s' LATEX='%s'\n", k, list[k].category.latin1(), list[k].tags.latin1(), list[k].latex.latin1());
  }
  printf("---------------------------------------------------\n");
}



KLFLibraryListCategoryViewItem::KLFLibraryListCategoryViewItem(QListView *parent, QString category)
  : QListViewItem(parent, "") , _category(category), _issubcategory(false)
{
   setup_category_item();
}
KLFLibraryListCategoryViewItem::KLFLibraryListCategoryViewItem(QString subcategory, KLFLibraryListCategoryViewItem *parent)
  : QListViewItem(parent, "") , _category(subcategory), _issubcategory(true)
{
   setup_category_item();
}

void KLFLibraryListCategoryViewItem::setup_category_item()
{
  QString c;
  if (_issubcategory) {
    c = _category != "" ? (_category.section('/', -1)) : "[ No Category ]";
  } else {
    c = _category != "" ? _category : "[ No Category ]";
  }
  setText(0, c);
}

KLFLibraryListCategoryViewItem::~KLFLibraryListCategoryViewItem()
{
}

KLFLibraryListViewItem::KLFLibraryListViewItem(QListViewItem *v, KLFData::KLFLibraryItem item, int index)
  : QListViewItem(v)
{
  _ind = index;
  updateLibraryItem(item);
}
KLFLibraryListViewItem::KLFLibraryListViewItem(QListView *v, KLFData::KLFLibraryItem item, int index)
  : QListViewItem(v)
{
  _ind = index;
  updateLibraryItem(item);
}

KLFLibraryListViewItem::~KLFLibraryListViewItem()
{
}

int KLFLibraryListViewItem::compare(QListViewItem *i, int /*col*/, bool ascending) const
{
  if (i->rtti() != KLFLibraryListViewItem::RTTI)
    return 1; // we come after any other item type (ie. categories)
  KLFLibraryListViewItem *item = (KLFLibraryListViewItem*) i;
  int k = ascending ? -1 : 1; // negative if ascending
  return k*(_ind - item->_ind);
}

void KLFLibraryListViewItem::updateLibraryItem(const KLFData::KLFLibraryItem& item)
{
  _item = item;
  setPixmap(0, _item.preview);
  QString l = _item.latex;
  l.replace("\n", "\t");
  setText(1, l);
}



QValueList<QListViewItem*> get_selected_items(QListView *lview, int givenRTTI)
{
  QValueList<QListViewItem*> selected_list;
  // get selection. For that, iterate all items and check their selection state.
  QListViewItemIterator iter(lview);

  while (iter.current()) {
    if (givenRTTI >= 0 && iter.current()->rtti() != givenRTTI) {
      ++iter;
      continue;
    }
    if (iter.current()->isSelected()) {
      selected_list.append(iter.current());
      ++iter;
    } else {
      ++iter;
    }
  }
  return selected_list;
}


// ------------------------------------------------------------------------

KLFLibraryListManager::KLFLibraryListManager(QListView *lview, KLFLibraryBrowser *parent, KLFData::KLFLibraryResource myresource,
					     KLFData::KLFLibrary *wholelibrary, KLFData::KLFLibraryResourceList *reslstptr, uint flags)
  : QObject(parent, "librarylistmanager"), _parent(parent), _listView(lview), _fulllibraryptr(wholelibrary),
	    _reslistptr(reslstptr), _myresource(myresource), _flags(flags), _am_libchg_originator(false), _dont_emit_libchg(false)
{
  _libItems = & ( _fulllibraryptr->operator[](myresource) );

  _search_str = QString("");
  _search_k = 0;


  QString msg = i18n("Right click for context menu, double-click to restore LaTeX formula.");
  QToolTip::add(_listView, msg);
  QToolTip::add(_listView->viewport(), msg);
  _listView->setSorting(0, true); // first column in ascending order


  connect(_listView, SIGNAL(contextMenuRequested(QListViewItem *, const QPoint&, int)),
	  this, SLOT(slotContextMenu(QListViewItem*, const QPoint&, int)));

  connect(_listView, SIGNAL(currentChanged(QListViewItem*)), this, SIGNAL(selectionChanged()));
  connect(_listView, SIGNAL(selectionChanged()), this, SIGNAL(selectionChanged()));
  connect(_listView, SIGNAL(doubleClicked(QListViewItem*, const QPoint&, int)),
	  this, SLOT(slotRestoreAll(QListViewItem*)));
  connect(_listView, SIGNAL(returnPressed(QListViewItem*)),
	  this, SLOT(slotRestoreAll(QListViewItem*)));

  connect(parent, SIGNAL(libraryChanged()), this, SLOT(slotCompleteRefresh()));
}

KLFLibraryListManager::~KLFLibraryListManager()
{
}



bool KLFLibraryListManager::canRestore() const
{
  return (_listView->currentItem() != 0 && _listView->currentItem()->rtti() == KLFLibraryListViewItem::RTTI);
}
bool KLFLibraryListManager::canDelete() const
{
  QListViewItemIterator it(_listView);
  while (it.current()) {
    if (it.current()->isSelected() && it.current()->rtti() == KLFLibraryListViewItem::RTTI) {
      return true;
    }
    ++it;
  }
  return false;
}


bool KLFLibraryListManager::getSelectedInfo_hasselection() const
{
  return get_selected_items(_listView, KLFLibraryListViewItem::RTTI).size() > 0;
}
bool KLFLibraryListManager::getSelectedInfo_hasmuliple() const
{
  return get_selected_items(_listView, KLFLibraryListViewItem::RTTI).size() > 1;
}
bool KLFLibraryListManager::getSelectedInfo_singleselection() const
{
  return get_selected_items(_listView, KLFLibraryListViewItem::RTTI).size() == 1;
}
QPixmap KLFLibraryListManager::getSelectedInfo_preview() const
{
  QValueList<QListViewItem*> list = get_selected_items(_listView, KLFLibraryListViewItem::RTTI);
  if (list.size() == 1) {
    return ((KLFLibraryListViewItem*)list[0])->libraryItem().preview;
  }
  return QPixmap(locate("appdata", "pics/nopreview.png"));
}
QString KLFLibraryListManager::getSelectedInfo_latex() const
{
  QValueList<QListViewItem*> list = get_selected_items(_listView, KLFLibraryListViewItem::RTTI);
  if (list.size() == 1) {
    return ((KLFLibraryListViewItem*)list[0])->libraryItem().latex;
  }
  return list.size() == 0 ? i18n("[ No Item Selected ]") : i18n("[ Multiple Items Selected ]");
}
QString KLFLibraryListManager::getSelectedInfo_category(int *canedit) const
{
  QValueList<QListViewItem*> list = get_selected_items(_listView, KLFLibraryListViewItem::RTTI);
  QString cat = "";
  QString ret = cat;
  *canedit = 0;
  uint k;
  for (k = 0; k < list.size(); ++k) {
    QString thiscat = ((KLFLibraryListViewItem*)list[k])->libraryItem().category;
    if (k == 0)
      ret = cat = thiscat;
    else if ( thiscat != cat ) {
      ret = "";
    }
  }
  if (list.size() > 0)
    *canedit = 1;
  return ret;
}
QString KLFLibraryListManager::getSelectedInfo_tags(int *canedit) const
{
  QValueList<QListViewItem*> list = get_selected_items(_listView, KLFLibraryListViewItem::RTTI);
  *canedit = 0;
  if (list.size() == 1) {
    *canedit = 1;
    return ((KLFLibraryListViewItem*)list[0])->libraryItem().tags;
  }
  return list.size() == 0 ? i18n("[ No Item Selected ]") : i18n("[ Multiple Items Selected ]");
}
QString KLFLibraryListManager::getSelectedInfo_style() const
{
  QValueList<QListViewItem*> list = get_selected_items(_listView, KLFLibraryListViewItem::RTTI);
  if (list.size() == 0) {
    return i18n("[ No Items Selected ]");
  }
  KLFData::KLFStyle sty;
  uint k;
  for (k = 0; k < list.size(); ++k) {
    if (k == 0) {
      sty = ((KLFLibraryListViewItem*)list[0])->libraryItem().style;
    }
    if ( ! ( ((KLFLibraryListViewItem*)list[0])->libraryItem().style == sty ) ) {
      return i18n("[ Different Styles ]");
    }
  }
  return KLFData::prettyPrintStyle(sty);
}
bool KLFLibraryListManager::updateSelectedInfo_category(const QString& newcat)
{
  return updateSelectedInfo(UpdateCategory, newcat, QString::null);
}
bool KLFLibraryListManager::updateSelectedInfo_tags(const QString& tags)
{
  return updateSelectedInfo(UpdateTags, QString::null, tags);
}
bool KLFLibraryListManager::updateSelectedInfo(uint which, const QString& category, const QString& tags)
{
  //  dump_library_list(*_libItems, "update:before");
  QValueList<QListViewItem*> list = get_selected_items(_listView, KLFLibraryListViewItem::RTTI);
  for (uint k = 0; k < list.size(); ++k) {
    QString l;
    KLFData::KLFLibraryItem &lref = _libItems->operator[](  ((KLFLibraryListViewItem*)list[k])->index()  );

    QString newcat, newtags;
    if (which & UpdateCategory) {
      newcat = category;
    } else {
      newcat = lref.category;
    }
    if (which & UpdateTags) {
      newtags = tags;
    } else {
      newtags = lref.tags;
    }
    if (!newcat.isEmpty())
      l = "%: "+newcat+"\n";
    if (!newtags.isEmpty())
      l += "% "+newtags+"\n";

    l += KLFData::stripCategoryTagsFromLatex(lref.latex);

    lref.latex = l;
    lref.category = newcat;
    lref.tags = newtags;

    KLFLibraryListCategoryViewItem *citem = 0;
    if (_flags & ShowCategories) {
      citem = itemForCategory(lref.category, true);
    }
    KLFLibraryListViewItem *item = itemForId(lref.id);
    item->updateLibraryItem(lref);

    if (citem != 0 && citem != item->parent() && item->parent() != 0) {
      item->parent()->takeItem(item);
      citem->insertItem(item);
    }

    _listView->ensureItemVisible(item);
  }
  //  dump_library_list(*_libItems, "updatecategory:after");
  //  _parent->emitLibraryChanged();
  //..
  //  KMessageBox::information(_parent, i18n("New Category: %1 !").arg(newcat));
  
  return true;
}

void KLFLibraryListManager::slotContextMenu(QListViewItem */*item*/, const QPoint &p, int /*column*/)
{
  KPopupMenu *menu = new KPopupMenu(_parent);
  menu->setCheckable(true);
  menu->insertTitle(i18n("Actions"));
  int idr1 = menu->insertItem(QIconSet(locate("appdata", "pics/restoreall.png")), i18n("Restore latex formula and style"),
			      this, SLOT(slotRestoreAll()));
  int idr2 = menu->insertItem(QIconSet(locate("appdata", "pics/restore.png")), i18n("Restore latex formula only"),
			      this, SLOT(slotRestoreLatex()));
  menu->insertSeparator();
  int iddel = menu->insertItem(QIconSet(locate("appdata", "pics/delete.png")), i18n("Delete from library"),
			       this, SLOT(slotDelete()));
  menu->insertSeparator();

  KPopupMenu *copytomenu = new KPopupMenu;
  copytomenu->insertTitle(i18n("Copy To Resource:"));
  uint k;
  for (k = 0; k < _reslistptr->size(); ++k) {
    if (_reslistptr->operator[](k).id == _myresource.id)
      continue;
    int idk = copytomenu->insertItem(QIconSet(locate("appdata", "pics/copytores.png")), _reslistptr->operator[](k).name,
				     this, SLOT(slotCopyToResource(int)));
    copytomenu->setItemParameter(idk, k);
  }
  int idcp = menu->insertItem(i18n("Copy to ..."), copytomenu);

  KPopupMenu *movetomenu = new KPopupMenu;
  movetomenu->insertTitle(i18n("Move To Resource:"));
  for (k = 0; k < _reslistptr->size(); ++k) {
    if (_reslistptr->operator[](k).id == _myresource.id)
      continue;
    int idk = movetomenu->insertItem(QIconSet(locate("appdata", "pics/movetores.png")), _reslistptr->operator[](k).name,
				     this, SLOT(slotMoveToResource(int)));
    movetomenu->setItemParameter(idk, k);
  }
  int idmv = menu->insertItem(i18n("Move to ..."), movetomenu);

  // Needed for when user pops up a menu without selection (ie. very short list, free white space under)
  bool canre = canRestore(), candel = canDelete();
  menu->setItemEnabled(idr1, canre);
  menu->setItemEnabled(idr2, canre);
  menu->setItemEnabled(iddel, candel);
  menu->setItemEnabled(idcp, canre);
  menu->setItemEnabled(idmv, canre && candel);

  menu->popup(p);
}


void KLFLibraryListManager::slotRestoreAll(QListViewItem *it)
{
  if (it == 0)
    it = _listView->currentItem();
  if (it == 0 || it->rtti() != KLFLibraryListViewItem::RTTI)
    return;

  emit restoreFromLibrary( ((KLFLibraryListViewItem*)it)->libraryItem(), true);
}

void KLFLibraryListManager::slotRestoreLatex(QListViewItem *it)
{
  if (it == 0)
    it = _listView->currentItem();
  if (it == 0 || it->rtti() != KLFLibraryListViewItem::RTTI)
    return;

  emit restoreFromLibrary( ((KLFLibraryListViewItem*)it)->libraryItem(), false);
}

void KLFLibraryListManager::slotDelete(QListViewItem *it)
{
  if (it == 0) {
    uint k;
    // get selection
    QValueList<QListViewItem*> list = get_selected_items(_listView, KLFLibraryListViewItem::RTTI);

    if (!_dont_emit_libchg) { // not only don't emit, but also don't ask questions
      int r = KMessageBox::questionYesNo(_parent, i18n("Are you sure you want to delete %1 selected item(s) from library?").arg(list.size()),
					i18n("delete from Library?"));
      if (r != KMessageBox::Yes)
	return;
    }

//DEBUG    printf("libitems->size=%d\n", _libItems->size());
    // delete items.
    for (k = 0; k < list.size(); ++k) {
      // find item with good ID
      // can't use index() because we're deleting the items and indexes change!
      uint j;
      for (j = 0; j < _libItems->size() && _libItems->operator[](j).id != ((KLFLibraryListViewItem*)list[k])->libraryItem().id; ++j);
      if (j >= _libItems->size()) {
	fprintf(stderr, "ERROR: NASTY: Can't find index for deletion!!! item-id=%d\n", _libItems->operator[](j).id);
      } else {
//DEBUG	printf("Erasing Item k=%d which reports an index of %d and is position j=%d in libItems\n", k, ((KLFLibraryListViewItem*)list[k])->index(), j);
	_libItems->erase(_libItems->at(j));
	delete list[k];
      }
    }
//DEBUG    printf("libitems->size=%d\n", _libItems->size());

    // we need to re-update the index() for each item
    for (k = 0; k < _libItems->size(); ++k) {
      KLFLibraryListViewItem *itm = itemForId(_libItems->operator[](k).id);
      if (itm != 0) {
	itm->updateIndex(k);
      }
    }

  } else {
    _libItems->erase(_libItems->at( ((KLFLibraryListViewItem*)it)->index() ));
    delete it;
  }
  if (!_dont_emit_libchg) {
    _am_libchg_originator = true;
    _parent->emitLibraryChanged();
    _am_libchg_originator = false;
  }
}

void KLFLibraryListManager::slotCopyToResource(int ind)
{
  QValueList<QListViewItem*> selected = get_selected_items(_listView, KLFLibraryListViewItem::RTTI);

  KLFData::KLFLibraryResource new_resource = _reslistptr->operator[](ind);

  for (uint i = 0; i < selected.size(); ++i) {
    uint k;
    ulong id = ((KLFLibraryListViewItem*)selected[i])->libraryItem().id;
    for (k = 0; k < _libItems->size() && _libItems->operator[](k).id != id; ++k);
    if (k == _libItems->size()) {
      fprintf(stderr, "ERROR: NASTY: Can't find index for deletion!!!\n");
    } else {
      _fulllibraryptr->operator[](new_resource).append(_libItems->operator[](k));
    }
  }
  if (!_dont_emit_libchg) {
    _am_libchg_originator = true;
    _parent->emitLibraryChanged();
    _am_libchg_originator = false;
  }
}
void KLFLibraryListManager::slotMoveToResource(int ind)
{
  _dont_emit_libchg = true;
  slotCopyToResource(ind);
  slotDelete(); // _dont_emit_libchg = true  also tells slotDelete to forget about asking questions to user like "Are you sure?"
  _dont_emit_libchg = false;
  _parent->emitLibraryChanged();
}

bool KLFLibraryListManager::slotSearchFind(const QString& s)
{
  _search_str = s;
  _search_k = _libItems->size();
  return slotSearchFindNext();
}
bool KLFLibraryListManager::slotSearchFindNext()
{
  QString s = _search_str;

  if (s.isEmpty())
    return false;

  // search _libItems for occurence
  bool found = false;
  int k;
  for (k = _search_k-1; !found && k >= 0; --k) { // reverse search, find most recent first
    int i = _libItems->operator[](k).latex.find(s, 0, false); // case insensitive search
    if (i != -1) {
      KLFLibraryListViewItem *item = itemForId(_libItems->operator[](k).id);
      if (item != 0) {
	// we found our item
	// 'item' points to our item
	_listView->ensureItemVisible(item);
	QListView::SelectionMode selmode = _listView->selectionMode();
	_listView->setSelectionMode(QListView::Single);
	_listView->setSelected(item, true);
	_listView->setSelectionMode(selmode);
	_listView->setCurrentItem(item);
	found = true;
	break;
      }
    }
  }
  if (found) {
    _search_k = k;
  } else {
    _search_k = _libItems->size(); // restart from end next search

    KLFLibraryListViewItem *item = (KLFLibraryListViewItem*) _listView->firstChild();
    if (item) {
      // small tweaks that work ok
      QListView::SelectionMode selmode = _listView->selectionMode();
      _listView->setSelectionMode(QListView::Single);
      _listView->setSelected(item, true);
      _listView->setSelectionMode(selmode);
      _listView->ensureItemVisible(item);
      _listView->setSelected(item, false);
      _listView->setCurrentItem(_listView->lastItem());
    } else {
      // this shouldn't happen, but anyway, just in case
      _listView->clearSelection();
    }
  }
  return found;
}


void KLFLibraryListManager::slotCompleteRefresh()
{
  if (_am_libchg_originator)
    return;

  // memorize open/closed tree status
  uint i;
  QMap<QString,bool> cattreestatus;
  for (i = 0; i < mCategoryItems.size(); ++i) {
    cattreestatus[mCategoryItems[i]->category()] = mCategoryItems[i]->isOpen();
  }

  _listView->clear();
  mCategoryItems.clear();
  for (i = 0; i < _libItems->size(); ++i) {
    // display if [  !("dont display duplicates" AND isduplicate) AND [ "show all" OR "this one is tagged" ]  ]
    // [equivalent to] display if  [  (!"dont display duplicates" OR !isduplicate) AND [ "show all" OR "this one is tagged" ]  ]
    if ( (klfconfig.LibraryBrowser.displayNoDuplicates == false || is_duplicate_of_previous(i) == false) &&
	(klfconfig.LibraryBrowser.displayTaggedOnly == false || _libItems->operator[](i).tags != "")  ) {
      if (_flags & ShowCategories)
	(void) new KLFLibraryListViewItem(itemForCategory(_libItems->operator[](i).category, true), _libItems->operator[](i), i);
      else
	(void) new KLFLibraryListViewItem(_listView, _libItems->operator[](i), i);
    }
  }
  for (i = 0; i < mCategoryItems.size(); ++i) {
    QMap<QString,bool>::iterator mit;
    if ((mit = cattreestatus.find(mCategoryItems[i]->category())) != cattreestatus.end()) {
      mCategoryItems[i]->setOpen(mit.data());
    } else {
      // expand all categories by default if less than 6 categories
      bool showopen = (_flags & ShowCategoriesDeepTree) ? (mCategoryItems[i]->parent()==0) : (mCategoryItems.size() < 6);
      mCategoryItems[i]->setOpen(showopen);
    }
  }
  //   _listView->setSorting(0, true); // column zero ascending
  //   _listView->sort();

  //  dump_library_list(*_libItems, "slotCompleteRefresh() done");
}
bool KLFLibraryListManager::is_duplicate_of_previous(uint indexInList)
{
  uint k;
  for (k = 0; k < indexInList; ++k) {
    if (_libItems->operator[](k) == _libItems->operator[](indexInList))
      return true;
  }
  return false;
}

KLFLibraryListCategoryViewItem *KLFLibraryListManager::itemForCategory(const QString& cat, bool createifneeded)
{
  //   if (_flags & ShowCategories == 0) {
  //     fprintf(stderr, "ERROR: CALL TO itemForCategory(%s, %d) IN NO-CATEGORY MODE !\n", (const char*)cat.local8Bit(), createifneeded);
  //     return 0;
  //   }
  KLFLibraryListCategoryViewItem *parentitem, *c;
  for (uint k = 0; k < mCategoryItems.size(); ++k) {
    if (mCategoryItems[k]->category() == cat) {
      return mCategoryItems[k];
    }
  }
  if (createifneeded && _flags & ShowCategoriesDeepTree) {
    int k;
    if ((k = cat.find('/')) != -1) {
      parentitem = itemForCategory(cat.left(k));
      //      printf("Creating catitem with other catitem as parent, this category=%s, parentcat=%s\n", (const char*)cat.local8Bit(), (const char*)parentitem->category().local8Bit());
      c = new KLFLibraryListCategoryViewItem( cat, parentitem );
      mCategoryItems.append(c);
      return c;
    }
  }
  // Note: this is executed if EITHER _flags&ShowCategoriesDeepTree is false
  //          OR _flgas&ShowCategoriesDeepTree is true but cat.find('/')==-1
  if (createifneeded) {
    //    printf("Creating catitem with listview as parent cat=%s\n", (const char*)cat.local8Bit());
    c = new KLFLibraryListCategoryViewItem( _listView, cat );
    mCategoryItems.append(c);
    return c;
  } else {
    return 0;
  }
}


KLFLibraryListViewItem *KLFLibraryListManager::itemForId(uint reqid)
{
  QListViewItemIterator it(_listView);
  KLFLibraryListViewItem *item;
  while (it.current()) {
    if (it.current()->rtti() != KLFLibraryListViewItem::RTTI) {
      ++it;
      continue;
    }
    item = (KLFLibraryListViewItem *)it.current();
    if (item && item->libraryItem().id == reqid)
      return item;
    ++it;
  }
  return 0;
}



// ------------------------------------------------------------------------

enum { CONFIGMENU_DISPLAYTAGGEDONLY_ID = 1840, CONFIGMENU_NODUPLICATES_ID = 1841 };
  
  
KLFLibraryBrowser::KLFLibraryBrowser(KLFData::KLFLibrary *wholelistptr, KLFData::KLFLibraryResourceList *reslistptr, KLFMainWin *parent)
  : KLFLibraryBrowserUI(0, 0, Qt::WStyle_Customize|Qt::WStyle_DialogBorder|Qt::WStyle_Title
			|Qt::WStyle_SysMenu|Qt::WStyle_Minimize)
{

  btnRestore->setIconSet(QPixmap(locate("appdata", "pics/restore.png")));
  btnClose->setIconSet(QPixmap(locate("appdata", "pics/closehide.png")));
  btnDelete->setIconSet(QPixmap(locate("appdata", "pics/delete.png")));
  btnConfig->setIconSet(QPixmap(locate("appdata", "pics/settings.png")));
  btnFindNext->setIconSet(QPixmap(locate("appdata", "pics/findnext.png")));
  btnConfig->setText("");

  btnSearchClear->setPixmap(QPixmap(locate("appdata", "pics/clearright.png")));

  _libptr = wholelistptr;
  _libresptr = reslistptr;

  QVBoxLayout *lyt = new QVBoxLayout(frmList);
  lyt->setAutoAdd(true);
  // setup tab bar and mLists
  tabResources = 0;
  setupResourcesListsAndTabs();


  _allowrestore = _allowdelete = false;

  mRestoreMenu = new KPopupMenu(this);
  mRestoreMenu->insertItem(QIconSet(locate("appdata", "pics/restoreall.png")), i18n("Restore Formula with Style"), this, SLOT(slotRestoreAll()));
  mRestoreMenu->insertItem(QIconSet(locate("appdata", "pics/restore.png")), i18n("Restore Formula Only"), this, SLOT(slotRestoreLatex()));
  btnRestore->setPopup(mRestoreMenu);

  mConfigMenu = new KPopupMenu(this);
  mConfigMenu->setCheckable(true);
  mConfigMenu->insertTitle(i18n("Configure display"), 1000, 0);
  mConfigMenu->insertItem(i18n("Only display tagged items"), this, SLOT(slotDisplayTaggedOnly()), 0, CONFIGMENU_DISPLAYTAGGEDONLY_ID);
  mConfigMenu->setItemChecked(CONFIGMENU_DISPLAYTAGGEDONLY_ID, klfconfig.LibraryBrowser.displayTaggedOnly);
  mConfigMenu->insertItem(i18n("Don't display duplicate items"), this, SLOT(slotDisplayNoDuplicates()), 0, CONFIGMENU_NODUPLICATES_ID);
  mConfigMenu->setItemChecked(CONFIGMENU_NODUPLICATES_ID, klfconfig.LibraryBrowser.displayTaggedOnly);
  btnConfig->setPopup(mConfigMenu);

  mConfigMenu->setItemChecked(CONFIGMENU_DISPLAYTAGGEDONLY_ID, klfconfig.LibraryBrowser.displayTaggedOnly);
  mConfigMenu->setItemChecked(CONFIGMENU_NODUPLICATES_ID, klfconfig.LibraryBrowser.displayNoDuplicates);
  
  // syntax highlighter for latex preview
  KLFLatexSyntaxHighlighter *syntax = new KLFLatexSyntaxHighlighter(txtPreviewLatex, this);
  QTimer *synttimer = new QTimer(this);
  synttimer->start(500);
  connect(synttimer, SIGNAL(timeout()), syntax, SLOT(refreshAll()));

  connect(btnDelete, SIGNAL(clicked()), this, SLOT(slotDelete()));
  connect(btnClose, SIGNAL(clicked()), this, SLOT(slotClose()));

  connect(btnSearchClear, SIGNAL(clicked()), this, SLOT(slotSearchClear()));
  connect(lneSearch, SIGNAL(textChanged(const QString&)), this, SLOT(slotSearchFind(const QString&)));
  connect(lneSearch, SIGNAL(lostFocus()), this, SLOT(resetLneSearchColor()));
  connect(btnFindNext, SIGNAL(clicked()), this, SLOT(slotSearchFindNext()));
  
//   connect(cbxCategory, SIGNAL(textChanged(const QString&)), this, SLOT(slotCategoryEdited(const QString&)));
//   connect(cbxTags, SIGNAL(textChanged(const QString&)), this, SLOT(slotTagsEdited(const QString&)));
  connect(btnUpdateCategory, SIGNAL(clicked()), this, SLOT(slotUpdateEditedCategory()));
  connect(cbxCategory->lineEdit(), SIGNAL(returnPressed()), this, SLOT(slotUpdateEditedCategory()));
  connect(btnUpdateTags, SIGNAL(clicked()), this, SLOT(slotUpdateEditedTags()));
  connect(cbxTags->lineEdit(), SIGNAL(returnPressed()), this, SLOT(slotUpdateEditedTags()));
  
  connect(parent, SIGNAL(libraryAllChanged()), this, SLOT(slotCompleteRefresh()));
  
  

  lneSearch->installEventFilter(this);

  _dflt_lineedit_bgcol = lneSearch->paletteBackgroundColor();

  slotRefreshButtonsEnabled();
  slotRefreshPreview();
}
KLFLibraryBrowser::~KLFLibraryBrowser()
{
}
void KLFLibraryBrowser::setupResourcesListsAndTabs()
{
  mLists.clear();

  if (tabResources)
    delete tabResources;

  tabResources = new QTabWidget(frmList);
  tabResources->setTabPosition(QTabWidget::Top);
  tabResources->setTabShape(QTabWidget::Triangular);

  if (_libresptr->size() == 0) {
    fprintf(stderr, "KLFLibraryBrowser: RESOURCE LIST IS EMPTY! EXPECT A CRASH AT ANY MOMENT !!\n");
  }

  uint k, flags;
  for (k = 0; k < _libresptr->size(); ++k) {
    QListView *lstview = new QListView(tabResources);
    lstview->addColumn(i18n("Preview"), 320);
    lstview->addColumn(i18n("Latex Code"), 800);
    lstview->setName(QString::number(_libresptr->operator[](k).id));
    lstview->setRootIsDecorated(true);
    lstview->setSelectionMode(QListView::Extended);
    tabResources->addTab(lstview, _libresptr->operator[](k).name);

    // the default flags
    flags = KLFLibraryListManager::ShowCategories | KLFLibraryListManager::ShowCategoriesDeepTree;
    // specific flags for specific resources
    if (_libresptr->operator[](k).id == KLFData::LibResource_History) {
      flags = KLFLibraryListManager::NoFlags;
    } else if (_libresptr->operator[](k).id == KLFData::LibResource_Archive) {
      flags = KLFLibraryListManager::ShowCategories | KLFLibraryListManager::ShowCategoriesDeepTree;
    }
    KLFLibraryListManager *manager = new KLFLibraryListManager(lstview, this, _libresptr->operator[](k), _libptr, _libresptr, flags);

//    connect(manager, SIGNAL(libraryChanged()), this, SIGNAL(libraryChanged()));
    connect(manager, SIGNAL(restoreFromLibrary(KLFData::KLFLibraryItem, bool)),
	    this, SIGNAL(restoreFromLibrary(KLFData::KLFLibraryItem, bool)));
    connect(manager, SIGNAL(selectionChanged()), this, SLOT(slotRefreshButtonsEnabled()));
    connect(manager, SIGNAL(selectionChanged()), this, SLOT(slotRefreshPreview()));

    mLists.append(manager);
  }
  emitLibraryChanged();

  connect(tabResources, SIGNAL(currentChanged(QWidget *)), this, SLOT(slotTabResourcesSelected(QWidget *)));
  _currentList = 0;
  tabResources->setCurrentPage(0);
}


void KLFLibraryBrowser::slotTabResourcesSelected(QWidget *cur)
{
  uint k;
  _currentList = 0;
  for (k = 0; k < mLists.size(); ++k) {
    if (QString::number(mLists[k]->myResource().id) == cur->name()) {
      _currentList = mLists[k];
    }
  }
  slotRefreshButtonsEnabled();
  slotRefreshPreview();
}

void KLFLibraryBrowser::reject()
{
  // this is called when "ESC" is pressed.
  // ignore event. We don't want to quit unless user selects "Close" and accept(),
  // possibly the user selects "X" on the window title bar. In this last case, it works even
  // with this reimplementation of reject() (Qt calls hide()).
}

void KLFLibraryBrowser::closeEvent(QCloseEvent *e)
{
  e->accept();
  emit refreshLibraryBrowserShownState(false);
}

void KLFLibraryBrowser::slotClose()
{
  hide();
  emit refreshLibraryBrowserShownState(false);
}

bool KLFLibraryBrowser::eventFilter(QObject *obj, QEvent *ev)
{
  if (obj == lneSearch) {
    if (ev->type() == QEvent::KeyPress) {
      QKeyEvent *kev = (QKeyEvent*)ev;
      if (_currentList) {
	if (kev->key() == Key_F3) { // find next
	  slotSearchFindNext();
	  return true;
	}
	if (kev->key() == Key_Escape) { // abort search

	  _currentList->slotSearchFind(QString("\024\015\077\073\017__badstr__THIS_###_WONT_BE FOUND\033\025\074\057"));
	  slotSearchFindNext();
	  lneSearch->setText(""); // reset search
	  resetLneSearchColor();
	}
      }
    }
  }
  return KLFLibraryBrowserUI::eventFilter(obj, ev);
}

void KLFLibraryBrowser::emitLibraryChanged()
{
  emit libraryChanged();
  //   uint k;
  //   for (k = 0; k < mLists.size(); ++k) {
  //     mLists[k]->slotCompleteRefresh();
  //   }
}


void KLFLibraryBrowser::addToHistory(KLFData::KLFLibraryItem h)
{
  _libptr->operator[](_libresptr->operator[](0)).append(h);
  emitLibraryChanged();
}

void KLFLibraryBrowser::slotCompleteRefresh()
{
  setupResourcesListsAndTabs();
  emitLibraryChanged();
}

void KLFLibraryBrowser::slotUpdateEditedCategory()
{
  QString c = cbxCategory->currentText();
  // purify c
  c = c.stripWhiteSpace();
  if (!c.isEmpty()) {
    QStringList sections = QStringList::split("/", c, false); // no empty sections
    uint k;
    for (k = 0; k < sections.size(); ++k) {
      //      printf("Section %d: '%s'\n", k, (const char*)sections[k].local8Bit());
      sections[k] = sections[k].simplifyWhiteSpace();
      if (sections[k].isEmpty()) {
	sections.erase(sections.at(k));
	--k;
      }
    }
    c = sections.join("/");
      //     c = c.replace(QRegExp("/+"), "/");
      //     if (c[0] == '/') c = c.mid(1);
      //     if (c[c.length()-1] == '/') c = c.left(c.length()-1);
  } else {
    // c is empty
    c = QString::null; // force to being NULL
  }
  _currentList->updateSelectedInfo_category(c);
  slotRefreshPreview();
}

void KLFLibraryBrowser::slotUpdateEditedTags()
{
  // tags don't need to be purified.
  _currentList->updateSelectedInfo_tags(cbxTags->currentText());
  slotRefreshPreview();
}

void KLFLibraryBrowser::slotRefreshPreview()
{
  if (!_currentList) {
    lblPreview->setPixmap(locate("appdata", "pics/nopixmap.png"));
    txtPreviewLatex->setText("");
    cbxCategory->setCurrentText("");
    cbxTags->setCurrentText("");
    lblStylePreview->setText("");
    tbxPreview->setCurrentIndex(0);

    frmPreview->setEnabled(false);
    return;
  }
  // populate comboboxes
  KLFData::KLFLibraryResource curres = _currentList->myResource();
  QStringList categories;
  uint j;
  uint k;
  for (j = 0; j < _libresptr->size(); ++j) {
    KLFData::KLFLibraryList l = _libptr->operator[](_libresptr->operator[](j));
    for (k = 0; k < l.size(); ++k) {
      QString c = l[k].category;
      if (!c.isEmpty() && categories.find(c) == categories.end())
	categories.append(c);
    }
  }
  categories.sort();
  cbxCategory->clear();
  cbxCategory->insertStringList(categories);
  // show the previews
  int can_edit;
  frmPreview->setEnabled(true);
  lblPreview->setPixmap(_currentList->getSelectedInfo_preview());
  txtPreviewLatex->setText(_currentList->getSelectedInfo_latex());
  txtPreviewLatex->setEnabled(_currentList->getSelectedInfo_hasselection());
  cbxCategory->setCurrentText(_currentList->getSelectedInfo_category(&can_edit));
  cbxCategory->setEnabled(can_edit);
  cbxTags->setCurrentText(_currentList->getSelectedInfo_tags(&can_edit));
  cbxTags->setEnabled(can_edit);
  lblStylePreview->setText(_currentList->getSelectedInfo_style());
}


void KLFLibraryBrowser::slotRestoreAll(QListViewItem *)
{
  if (!_currentList)
    return;
  _currentList->slotRestoreAll();
}
void KLFLibraryBrowser::slotRestoreLatex(QListViewItem *)
{
  if (!_currentList)
    return;
  _currentList->slotRestoreLatex();
}
void KLFLibraryBrowser::slotDelete(QListViewItem *)
{
  if (!_currentList)
    return;
  _currentList->slotDelete();
}


void KLFLibraryBrowser::slotDisplayTaggedOnly()
{
  slotDisplayTaggedOnly( ! klfconfig.LibraryBrowser.displayTaggedOnly );
}
void KLFLibraryBrowser::slotDisplayTaggedOnly(bool display)
{
  klfconfig.LibraryBrowser.displayTaggedOnly = display;
  klfconfig.writeToConfig();
  mConfigMenu->setItemChecked(CONFIGMENU_DISPLAYTAGGEDONLY_ID, display);
  emitLibraryChanged();
}
void KLFLibraryBrowser::slotDisplayNoDuplicates()
{
  slotDisplayNoDuplicates( ! klfconfig.LibraryBrowser.displayNoDuplicates );
}
void KLFLibraryBrowser::slotDisplayNoDuplicates(bool display)
{
  klfconfig.LibraryBrowser.displayNoDuplicates = display;
  klfconfig.writeToConfig();
  mConfigMenu->setItemChecked(CONFIGMENU_NODUPLICATES_ID, display);
  emitLibraryChanged();
}


void KLFLibraryBrowser::slotRefreshButtonsEnabled()
{
  if (!_currentList) {
    _allowrestore = _allowdelete = false;
  } else {
    _allowrestore = _currentList->canRestore();
    _allowdelete = _currentList->canDelete();
  }

  btnRestore->setEnabled(_allowrestore);
  btnDelete->setEnabled(_allowdelete);
}


void KLFLibraryBrowser::slotSearchClear()
{
  lneSearch->setText("");
  lneSearch->setFocus();
}

void KLFLibraryBrowser::slotSearchFind(const QString& s)
{
  if (!_currentList)
    return;
  bool res = _currentList->slotSearchFind(s);
  if (s.isEmpty()) {
    resetLneSearchColor();
    return;
  }
  if (res) {
    lneSearch->setPaletteBackgroundColor(klfconfig.LibraryBrowser.colorFound /*QColor(128, 255, 128)*/);
  } else {
    lneSearch->setPaletteBackgroundColor(klfconfig.LibraryBrowser.colorNotFound /*QColor(255,128,128)*/);
  }
}
void KLFLibraryBrowser::slotSearchFindNext()
{
  QString s = lneSearch->text();

  if (s.isEmpty()) {
    resetLneSearchColor();
    return;
  }

  if (!_currentList)
    return;
  bool res = _currentList->slotSearchFindNext();
  if (res) {
    lneSearch->setPaletteBackgroundColor(klfconfig.LibraryBrowser.colorFound /*QColor(128, 255, 128)*/);
  } else {
    lneSearch->setPaletteBackgroundColor(klfconfig.LibraryBrowser.colorNotFound /*QColor(255,128,128)*/);
  }
}

void KLFLibraryBrowser::resetLneSearchColor()
{
  lneSearch->setPaletteBackgroundColor(_dflt_lineedit_bgcol);
}




#include "klflibrary.moc"
