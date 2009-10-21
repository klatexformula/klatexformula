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

#include <stdio.h>

#include <QTabWidget>
#include <QPushButton>
#include <QLabel>
#include <QToolBox>
#include <QComboBox>
#include <QGridLayout>
#include <QIcon>
#include <QTimer>
#include <QRegExp>
#include <QFile>
#include <QDataStream>
#include <QLineEdit>
#include <QMessageBox>
#include <QFileDialog>
#include <QInputDialog>
#include <QCloseEvent>
#include <QKeyEvent>
#include <QDebug>

#include "klfconfig.h"
#include "klfdata.h"
#include "klfmainwin.h"
#include "klflibrary.h"


// KLatexFormula version
extern const char version[];
extern const int version_maj, version_min;



void dump_library_list(const KLFData::KLFLibraryList& list, const char *comment)
{
  int k;
  printf("\n----------- LIBRARY RESOURCE [ %s ] ---------------\n", comment);
  for (k = 0; k < list.size(); ++k) {
    printf("#%03d: CAT='%s' TAGS='%s' LATEX='%s'\n", k, list[k].category.toLocal8Bit().data(), list[k].tags.toLocal8Bit().data(),
	   list[k].latex.toLocal8Bit().data());
  }
  printf("---------------------------------------------------\n");
}



KLFLibraryListCategoryViewItem::KLFLibraryListCategoryViewItem(QTreeWidget *parent, QString category)
  : QTreeWidgetItem(parent, QStringList(""), Type) , _category(category), _issubcategory(false)
{
   setup_category_item();
}
KLFLibraryListCategoryViewItem::KLFLibraryListCategoryViewItem(QString subcategory, KLFLibraryListCategoryViewItem *parent)
  : QTreeWidgetItem(parent, QStringList(""), Type) , _category(subcategory), _issubcategory(true)
{
   setup_category_item();
}

void KLFLibraryListCategoryViewItem::setup_category_item()
{
  QString c;
  if (_issubcategory) {
    c = _category != "" ? (_category.section('/', -1)) : QObject::tr("[ No Category ]");
  } else {
    c = _category != "" ? _category : QObject::tr("[ No Category ]");
  }
  setText(0, c);
}

KLFLibraryListCategoryViewItem::~KLFLibraryListCategoryViewItem()
{
}

KLFLibraryListViewItem::KLFLibraryListViewItem(QTreeWidgetItem *v, const KLFData::KLFLibraryItem& item, int index)
  : QTreeWidgetItem(v, Type)
{
  _ind = index;
  updateLibraryItem(item);
}
KLFLibraryListViewItem::KLFLibraryListViewItem(QTreeWidget *v, const KLFData::KLFLibraryItem& item, int index)
  : QTreeWidgetItem(v, Type)
{
  _ind = index;
  updateLibraryItem(item);
}

KLFLibraryListViewItem::KLFLibraryListViewItem(const KLFData::KLFLibraryItem& item, int index)
  : QTreeWidgetItem(Type)
{
  _ind = index;
  updateLibraryItem(item);
}

KLFLibraryListViewItem::~KLFLibraryListViewItem()
{
}

QSize KLFLibraryListViewItem::sizeHint(int column) const
{
  if (column == 0)
    return _item.preview.size();
  return QTreeWidgetItem::sizeHint(column);
}

// int KLFLibraryListViewItem::compare(QListViewItem *i, int /*col*/, bool ascending) const
// {
//   if (i->rtti() != KLFLibraryListViewItem::RTTI)
//     return 1; // we come after any other item type (ie. categories)
//   KLFLibraryListViewItem *item = (KLFLibraryListViewItem*) i;
//   int k = ascending ? -1 : 1; // negative if ascending
//   return k*(_ind - item->_ind);
// }

void KLFLibraryListViewItem::updateLibraryItem(const KLFData::KLFLibraryItem& item)
{
  _item = item;
  setIcon(0, _item.preview);
  QString l = _item.latex;
  l.replace("\n", "\t");
  setText(1, l);
}



QList<QTreeWidgetItem*> get_selected_items(QTreeWidget *w, int givenType)
{
  QList<QTreeWidgetItem*> l = w->selectedItems();
  if (givenType < 0)
    return l;

  QList<QTreeWidgetItem*> filtered;
  for (int i = 0; i < l.size(); ++i) {
    if (l[i]->type() == givenType)
      filtered.append(l[i]);
  }
  return filtered;
//   QValueList<QListViewItem*> selected_list;
//   // get selection. For that, iterate all items and check their selection state.
//   QListViewItemIterator iter(lview);

//   while (iter.current()) {
//     if (givenRTTI >= 0 && iter.current()->rtti() != givenRTTI) {
//       ++iter;
//       continue;
//     }
//     if (iter.current()->isSelected()) {
//       selected_list.append(iter.current());
//       ++iter;
//     } else {
//       ++iter;
//     }
//   }
//   return selected_list;
}



// ------------------------------------------------------------------------

KLFLibraryListManager::KLFLibraryListManager(QTreeWidget *lview, KLFLibraryBrowser *parent, KLFData::KLFLibraryResource myresource,
					     KLFData::KLFLibrary *wholelibrary, KLFData::KLFLibraryResourceList *reslstptr, uint flags)
  : QObject(parent), _parent(parent), _listView(lview), _fulllibraryptr(wholelibrary),
    _reslistptr(reslstptr), _myresource(myresource), _flags(flags), _am_libchg_originator(false), _dont_emit_libchg(false)
{
  setObjectName("librarylistmanager");

  _libItems = & ( _fulllibraryptr->operator[](myresource) );

  _search_str = QString("");
  _search_k = 0;

  _listView->setHeaderLabels(QStringList() << tr("Preview") << tr("Latex Code"));
  _listView->setColumnWidth(0, 320);
  _listView->setColumnWidth(1, 800);
  _listView->setIconSize(klfconfig.UI.labelOutputFixedSize);
  _listView->setSelectionMode(QTreeWidget::ExtendedSelection);
  _listView->setSelectionBehavior(QAbstractItemView::SelectRows);
  _listView->setContextMenuPolicy(Qt::CustomContextMenu);
  _listView->setAllColumnsShowFocus(true);
  _listView->setSortingEnabled(false); // no sorting
  _listView->setAutoScroll(false);
  _listView->setDragDropMode(QAbstractItemView::DragOnly);
  _listView->setEditTriggers(QAbstractItemView::NoEditTriggers);
  _listView->setHorizontalScrollMode(QAbstractItemView::ScrollPerPixel);
  _listView->setVerticalScrollMode(QAbstractItemView::ScrollPerPixel);

  connect(_listView, SIGNAL(customContextMenuRequested(const QPoint&)),
	  this, SLOT(slotContextMenu(const QPoint&)));

  connect(_listView, SIGNAL(currentItemChanged(QTreeWidgetItem*,QTreeWidgetItem*)), this, SIGNAL(selectionChanged()));
  connect(_listView, SIGNAL(itemSelectionChanged()), this, SIGNAL(selectionChanged()));
  connect(_listView, SIGNAL(itemDoubleClicked(QTreeWidgetItem*, int)),
	  this, SLOT(slotRestoreAll()));
  //  connect(_listView, SIGNAL(itemActivated(QTreeWidgetItem *, int)),
  //          this, SLOT(slotRestoreAll(QTreeWidgetItem*)));

  connect(parent, SIGNAL(libraryChanged()), this, SLOT(slotCompleteRefresh()));
}

KLFLibraryListManager::~KLFLibraryListManager()
{
}



bool KLFLibraryListManager::canRestore() const
{
  return (_listView->currentItem() != 0 && _listView->currentItem()->type() == KLFLibraryListViewItem::Type);
}
bool KLFLibraryListManager::canDelete() const
{
  QTreeWidgetItemIterator it(_listView);
  while (*it) {
    if ((*it)->isSelected() && (*it)->type() == KLFLibraryListViewItem::Type) {
      return true;
    }
    ++it;
  }
  return false;
}


bool KLFLibraryListManager::getSelectedInfo_hasselection() const
{
  return get_selected_items(_listView, KLFLibraryListViewItem::Type).size() > 0;
}
bool KLFLibraryListManager::getSelectedInfo_hasmuliple() const
{
  return get_selected_items(_listView, KLFLibraryListViewItem::Type).size() > 1;
}
bool KLFLibraryListManager::getSelectedInfo_singleselection() const
{
  return get_selected_items(_listView, KLFLibraryListViewItem::Type).size() == 1;
}
QPixmap KLFLibraryListManager::getSelectedInfo_preview() const
{
  QList<QTreeWidgetItem*> list = get_selected_items(_listView, KLFLibraryListViewItem::Type);
  if (list.size() == 1) {
    return ((KLFLibraryListViewItem*)list[0])->libraryItem().preview;
  }
  return QPixmap(":/pics/nopreview.png");
}
QString KLFLibraryListManager::getSelectedInfo_latex() const
{
  QList<QTreeWidgetItem*> list = get_selected_items(_listView, KLFLibraryListViewItem::Type);
  if (list.size() == 1) {
    return ((KLFLibraryListViewItem*)list[0])->libraryItem().latex;
  }
  return list.size() == 0 ? tr("[ No Item Selected ]") : tr("[ Multiple Items Selected ]");
}
QString KLFLibraryListManager::getSelectedInfo_category(int *canedit) const
{
  QList<QTreeWidgetItem*> list = get_selected_items(_listView, KLFLibraryListViewItem::Type);
  QString cat = "";
  QString ret = cat;
  *canedit = 0;
  int k;
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
  QList<QTreeWidgetItem*> list = get_selected_items(_listView, KLFLibraryListViewItem::Type);
  *canedit = 0;
  if (list.size() == 1) {
    *canedit = 1;
    return ((KLFLibraryListViewItem*)list[0])->libraryItem().tags;
  }
  return list.size() == 0 ? tr("[ No Item Selected ]") : tr("[ Multiple Items Selected ]");
}
QString KLFLibraryListManager::getSelectedInfo_style() const
{
  QList<QTreeWidgetItem*> list = get_selected_items(_listView, KLFLibraryListViewItem::Type);
  if (list.size() == 0) {
    return tr("[ No Items Selected ]");
  }
  KLFData::KLFStyle sty;
  int k;
  for (k = 0; k < list.size(); ++k) {
    KLFData::KLFStyle thisstyle = ((KLFLibraryListViewItem*)list[k])->libraryItem().style;
    if (k == 0) {
      sty = thisstyle;
    }
    if ( ! ( thisstyle == sty ) ) {
      return tr("[ Different Styles ]");
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
  QList<QTreeWidgetItem*> list = get_selected_items(_listView, KLFLibraryListViewItem::Type);
  for (int k = 0; k < list.size(); ++k) {
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

    QTreeWidgetItem *p = item->parent();
    if (citem != 0 && citem != p && p != 0) {
      p->takeChild(p->indexOfChild(item));
      citem->addChild(item);
    }

    _listView->scrollToItem(item);
  }
  //  dump_library_list(*_libItems, "updatecategory:after");
  //  _parent->emitLibraryChanged();
  //..
  //  KMessageBox::information(_parent, i18n("New Category: %1 !").arg(newcat));

  return true;
}

KLFData::KLFLibraryList KLFLibraryListManager::getSelectedLibraryItems() const
{
  QList<QTreeWidgetItem*> list = get_selected_items(_listView, KLFLibraryListViewItem::Type);
  KLFData::KLFLibraryList liblist;
  int k;
  for (k = 0; k < list.size(); ++k) {
    liblist.append(((KLFLibraryListViewItem*)list[k])->libraryItem());
  }
  return liblist;
}

void KLFLibraryListManager::slotContextMenu(const QPoint& pos)
{
  QMenu *menu = new QMenu(_parent);
  QAction *a1 = menu->addAction(QIcon(":/pics/restoreall.png"), tr("Restore latex formula and style"),
				this, SLOT(slotRestoreAll()));
  QAction *a2 = menu->addAction(QIcon(":/pics/restore.png"), tr("Restore latex formula only"),
				this, SLOT(slotRestoreLatex()));
  menu->addSeparator();
  QAction *adel = menu->addAction(QIcon(":/pics/delete.png"), tr("Delete from library"),
				  this, SLOT(slotDelete()));
  menu->addSeparator();

  QMenu *copytomenu = new QMenu;
  QMenu *movetomenu = new QMenu;
  int k;
  QAction *acopy, *amove;
  for (k = 0; k < _reslistptr->size(); ++k) {
    if (_reslistptr->at(k).id == _myresource.id)
      continue;
    acopy = copytomenu->addAction(_reslistptr->at(k).name, this, SLOT(slotCopyToResource()));
    acopy->setProperty("resourceId", (uint)_reslistptr->at(k).id);
    amove = movetomenu->addAction(_reslistptr->at(k).name, this, SLOT(slotMoveToResource()));
    amove->setProperty("resourceId", (uint)_reslistptr->at(k).id);
  }
  QAction *acopyto = menu->addMenu(copytomenu);
  acopyto->setText(tr("Copy to"));
  acopyto->setIcon(QIcon(":/pics/copy.png"));
  QAction *amoveto = menu->addMenu(movetomenu);
  amoveto->setText(tr("Move to"));
  amoveto->setIcon(QIcon(":/pics/move.png"));

  menu->addSeparator();

  /*QAction *arefresh =*/ menu->addAction(QIcon(":/pics/refresh.png"), tr("Refresh"),
					  this, SLOT(slotCompleteRefresh()));

  // Needed for when user pops up a menu without selection (ie. short list, free white space under)
  bool canre = canRestore(), candel = canDelete();
  a1->setEnabled(canre);
  a2->setEnabled(canre);
  adel->setEnabled(candel);
  acopyto->setEnabled(canre);
  amoveto->setEnabled(canre);

  menu->popup(_listView->mapToGlobal(pos));
}


void KLFLibraryListManager::slotRestoreAll()
{
  QTreeWidgetItem *it = _listView->currentItem();
  if (it == 0 || it->type() != KLFLibraryListViewItem::Type)
    return;

  emit restoreFromLibrary( ((KLFLibraryListViewItem*)it)->libraryItem(), true);
}

void KLFLibraryListManager::slotRestoreLatex()
{
  QTreeWidgetItem *it = _listView->currentItem();
  if (it == 0 || it->type() != KLFLibraryListViewItem::Type)
    return;

  emit restoreFromLibrary( ((KLFLibraryListViewItem*)it)->libraryItem(), false);
}

void KLFLibraryListManager::slotDelete()
{
  int k;
  // get selection
  QList<QTreeWidgetItem*> list = get_selected_items(_listView, KLFLibraryListViewItem::Type);
  
  if (!_dont_emit_libchg) { // not only don't emit, but also don't ask questions
    int r = QMessageBox::question(_parent, tr("delete from Library?"),
				  tr("Are you sure you want to delete %1 selected item(s) from library?").arg(list.size()),
				  QMessageBox::Yes|QMessageBox::No, QMessageBox::No);
    if (r != QMessageBox::Yes)
      return;
  }

  QList<uint> deleteIds;

  for (k = 0; k < list.size(); ++k) {
    deleteIds.append((dynamic_cast<KLFLibraryListViewItem*>(list[k]))->libraryItem().id);
  }

  slotDelete(deleteIds);
}

void KLFLibraryListManager::slotDelete(const QList<uint>& deleteIds)
{
  int k, j;
  // delete items.
  KLFLibraryListViewItem *itm;
  for (k = 0; k < deleteIds.size(); ++k) {
    // find item with good ID
    itm = itemForId(deleteIds[k]);
    if (itm == 0) {
      fprintf(stderr, "ERROR: Can't find item with id `%u'!\n", (uint)deleteIds[k]);
      continue;
    }
    for (j = 0; j < _libItems->size() && _libItems->at(j).id != deleteIds[k]; ++j) ;
    if (j >= _libItems->size()) {
      fprintf(stderr, "ERROR: Can't determine index of library item with id `%u'!\n", (uint)deleteIds[k]);
      continue;
    }
    itm->setHidden(true);
    _libItems->removeAt(j);
  }
  // we need to re-update the index() for each item
  for (k = 0; k < _libItems->size(); ++k) {
    KLFLibraryListViewItem *itm = itemForId(_libItems->at(k).id);
    if (itm != 0) {
      itm->updateIndex(k);
    }
  }
  
  if (!_dont_emit_libchg) {
    _am_libchg_originator = true;
    _parent->emitLibraryChanged();
    _am_libchg_originator = false;
  }
}

void KLFLibraryListManager::slotCopyToResource(bool move)
{
  QVariant v = sender()->property("resourceId");
  if ( ! v.isValid() || ! v.canConvert(QVariant::UInt) )
    return;
  uint rid = v.toUInt();
  slotCopyMoveToResource(rid, move);
}
void KLFLibraryListManager::slotMoveToResource()
{
  slotCopyToResource(true);
}
void KLFLibraryListManager::slotCopyMoveToResource(uint rid, bool move)
{
  int ind;
  for (ind = 0; ind < _reslistptr->size() && _reslistptr->at(ind).id != rid; ++ind) ;
  if (ind >= _reslistptr->size()) {
    fprintf(stderr, "ERROR: Can't find resource id `%d'!\n", rid);
    return;
  }

  QList<QTreeWidgetItem*> selected = get_selected_items(_listView, KLFLibraryListViewItem::Type);

  KLFData::KLFLibraryResource new_resource = _reslistptr->at(ind);

  QList<uint> deleteIds;

  for (int i = 0; i < selected.size(); ++i) {
    int k;
    ulong id = ((KLFLibraryListViewItem*)selected[i])->libraryItem().id;
    for (k = 0; k < _libItems->size() && _libItems->at(k).id != id; ++k) ;
    if (k == _libItems->size()) {
      fprintf(stderr, "ERROR: NASTY: Can't find index!!!\n");
    } else {
      _fulllibraryptr->operator[](new_resource).append(_libItems->at(k));
      if (move) {
	// if a move is requested, add this id to the list to delete
	deleteIds.append(id);
      }
    }
  }

  if (move) {
    slotDelete(deleteIds);
  } else {
    if (!_dont_emit_libchg) {
      _am_libchg_originator = true;
      _parent->emitLibraryChanged();
      _am_libchg_originator = false;
    }
  }
}
bool KLFLibraryListManager::slotSearchFind(const QString& s)
{
  _search_str = s;
  _search_k = _libItems->size();
  return slotSearchFindNext();
}
bool KLFLibraryListManager::slotSearchFindNext(int direction)
{
  QString s = _search_str;

  if (s.isEmpty())
    return false;

  if (direction == 0)
    direction = 1;

  // search _libItems for occurence
  bool found = false;
  int k;
  for (k = _search_k-direction; !found && k >= 0 && k < _libItems->size(); k -= direction) { // search in given direction
    int i = _libItems->at(k).latex.indexOf(s, 0, Qt::CaseInsensitive); // case insensitive search
    if (i != -1) {
      KLFLibraryListViewItem *item = itemForId(_libItems->at(k).id);
      if (item != 0) {
	// we found our item
	// 'item' points to our item
	_listView->scrollToItem(item);
	_listView->setCurrentItem(item, QItemSelectionModel::ClearAndSelect);
	found = true;
	break;
      }
    }
  }
  if (found) {
    _search_k = k;
  } else {
    if (direction > 0)
      _search_k = _libItems->size(); // restart from end next search
    else
      _search_k = -1; // restart from beginning if reverse search is on

//     // unselect all items
//     QList<QTreeWidgetItem*> l = _listView->selectedItems();
//     for (int j = 0; j < l.size(); ++j) {
//       l[j]->setSelected(false);
//     }
    
    KLFLibraryListViewItem *item = (KLFLibraryListViewItem*) _listView->topLevelItem(0);
    if (item) {
      // small tweaks that work ok
      _listView->scrollToItem(item);
      _listView->setCurrentItem(item, QItemSelectionModel::Clear);
      item->setSelected(false);
    }
  }
  return found;
}
bool KLFLibraryListManager::slotSearchFindPrev()
{
  return slotSearchFindNext(-1);
}
void KLFLibraryListManager::slotSearchAbort()
{
  // show top of list, no selection
  KLFLibraryListViewItem *item = (KLFLibraryListViewItem*) _listView->topLevelItem(0);
  if (item) {
    // small tweaks that work ok
    _listView->scrollToItem(item);
    _listView->setCurrentItem(item, QItemSelectionModel::Clear);
    item->setSelected(false);
  }
  _search_k = _libItems->size();
}

void KLFLibraryListManager::slotCompleteRefresh()
{
  if (_am_libchg_originator)
    return;

  // memorize open/closed tree status
  int i;
  QMap<QString,bool> cattreestatus;
  for (i = 0; i < mCategoryItems.size(); ++i) {
    cattreestatus[mCategoryItems[i]->category()] = mCategoryItems[i]->isExpanded();
  }

  _listView->clear();
  mCategoryItems.clear();
  KLFLibraryListViewItem *itm;
  for (i = 0; i < _libItems->size(); ++i) {
    // display if [  !("dont display duplicates" AND isduplicate) AND [ "show all" OR "this one is tagged" ]  ]
    // [equivalent to] display if  [  (!"dont display duplicates" OR !isduplicate) AND [ "show all" OR "this one is tagged" ]  ]
    if ( (klfconfig.LibraryBrowser.displayNoDuplicates == false || is_duplicate_of_previous(i) == false) &&
	 (klfconfig.LibraryBrowser.displayTaggedOnly == false || _libItems->operator[](i).tags != "")  ) {
      itm = new KLFLibraryListViewItem(_libItems->operator[](i), i);
      if (_flags & ShowCategories) {
	QTreeWidgetItem *catitem = itemForCategory(_libItems->operator[](i).category, true);
	if (catitem != 0) {
	  catitem->insertChild(0, itm);
	} else {
	  _listView->insertTopLevelItem(0, itm);
	}
      } else {
	_listView->insertTopLevelItem(0, itm);
      }
    }
  }
  for (i = 0; i < mCategoryItems.size(); ++i) {
    QMap<QString,bool>::iterator mit;
    if ((mit = cattreestatus.find(mCategoryItems[i]->category())) != cattreestatus.end()) {
      mCategoryItems[i]->setExpanded(*mit);
    } else {
      // expand all categories by default if less than 6 categories
      bool showopen = (_flags & ShowCategoriesDeepTree) ? (mCategoryItems[i]->parent()==0) : (mCategoryItems.size() < 6);
      mCategoryItems[i]->setExpanded(showopen);
    }
  }

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
  for (int k = 0; k < mCategoryItems.size(); ++k) {
    if (mCategoryItems[k]->category() == cat) {
      return mCategoryItems[k];
    }
  }
  if (createifneeded && _flags & ShowCategoriesDeepTree) {
    int k;
    if ((k = cat.indexOf('/')) != -1) {
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
  QTreeWidgetItemIterator it(_listView);
  KLFLibraryListViewItem *item;
  while (*it) {
    if ((*it)->type() != KLFLibraryListViewItem::Type) {
      ++it;
      continue;
    }
  item = dynamic_cast<KLFLibraryListViewItem*>(*it);
  if (item && item->libraryItem().id == reqid)
      return item;
    ++it;
  }
  return 0;
}



// ------------------------------------------------------------------------


KLFLibraryBrowser::KLFLibraryBrowser(KLFData::KLFLibrary *wholelistptr, KLFData::KLFLibraryResourceList *reslistptr, KLFMainWin *parent)
  : QWidget(/*parent*/ 0 , /*Qt::Tool*/ 0), KLFLibraryBrowserUI()
{
  setupUi(this);

  _libptr = wholelistptr;
  _libresptr = reslistptr;

  // setup tab bar and mLists
  tabResources = 0;
  mFrameResourcesLayout = 0;
  btnManageResources = 0;
  mManageResourcesMenu = 0;
  mImportExportMenu = mConfigMenu = mRestoreMenu = 0;
  setupResourcesListsAndTabs();

  _allowrestore = _allowdelete = false;

  mRestoreMenu = new QMenu(this);
  mRestoreMenu->addAction(QIcon(":/pics/restoreall.png"), tr("Restore Formula with Style"), this, SLOT(slotRestoreAll()));
  mRestoreMenu->addAction(QIcon(":/pics/restore.png"), tr("Restore Formula Only"), this, SLOT(slotRestoreLatex()));
  btnRestore->setMenu(mRestoreMenu);

  mConfigMenu = new QMenu(this);
  CONFIGMENU_DISPLAYTAGGEDONLY =
    mConfigMenu->addAction(tr("Only display tagged items"), this, SLOT(slotDisplayTaggedOnly()));
  CONFIGMENU_DISPLAYTAGGEDONLY->setCheckable(true);
  CONFIGMENU_DISPLAYTAGGEDONLY->setChecked(klfconfig.LibraryBrowser.displayTaggedOnly);
  CONFIGMENU_NODUPLICATES =
    mConfigMenu->addAction(tr("Don't display duplicate items"), this, SLOT(slotDisplayNoDuplicates()));
  CONFIGMENU_NODUPLICATES->setCheckable(true);
  CONFIGMENU_NODUPLICATES->setChecked(klfconfig.LibraryBrowser.displayTaggedOnly);
  btnConfig->setMenu(mConfigMenu);

  mImportExportMenu = new QMenu(this);
  IMPORTEXPORTMENU_IMPORT =
    mImportExportMenu->addAction(tr("Import ..."), this, SLOT(slotImport()));
  IMPORTEXPORTMENU_IMPORTTOCURRENTRESOURCE =
    mImportExportMenu->addAction(tr("Import Into Current Resource ..."), this, SLOT(slotImportToCurrentResource()));
  mImportExportMenu->addSeparator();
  IMPORTEXPORTMENU_EXPORT_LIBRARY =
    mImportExportMenu->addAction(tr("Export Whole Library ..."), this, SLOT(slotExportLibrary()));
  IMPORTEXPORTMENU_EXPORT_RESOURCE =
    mImportExportMenu->addAction(tr("Export Resource [] ..."), this, SLOT(slotExportResource()));
  IMPORTEXPORTMENU_EXPORT_SELECTION =
    mImportExportMenu->addAction(tr("Export Current Selection ..."), this, SLOT(slotExportSelection()));
  btnImportExport->setMenu(mImportExportMenu);
  

  // syntax highlighter for latex preview
  KLFLatexSyntaxHighlighter *syntax = new KLFLatexSyntaxHighlighter(txtPreviewLatex, this);
  QTimer *synttimer = new QTimer(this);
  synttimer->start(500);
  connect(synttimer, SIGNAL(timeout()), syntax, SLOT(refreshAll()));

  txtPreviewLatex->setFont(klfconfig.UI.preambleEditFont);

  connect(btnDelete, SIGNAL(clicked()), this, SLOT(slotDelete()));
  connect(btnClose, SIGNAL(clicked()), this, SLOT(slotClose()));

  connect(btnSearchClear, SIGNAL(clicked()), this, SLOT(slotSearchClear()));
  connect(txtSearch, SIGNAL(textChanged(const QString&)), this, SLOT(slotSearchFind(const QString&)));
  connect(txtSearch, SIGNAL(lostFocus()), this, SLOT(resetLneSearchColor()));
  connect(btnFindNext, SIGNAL(clicked()), this, SLOT(slotSearchFindNext()));
  connect(btnFindPrev, SIGNAL(clicked()), this, SLOT(slotSearchFindPrev()));
  
  connect(btnUpdateCategory, SIGNAL(clicked()), this, SLOT(slotUpdateEditedCategory()));
  connect(cbxCategory->lineEdit(), SIGNAL(returnPressed()), btnUpdateCategory, SLOT(animateClick()));
  connect(btnUpdateTags, SIGNAL(clicked()), this, SLOT(slotUpdateEditedTags()));
  connect(cbxTags->lineEdit(), SIGNAL(returnPressed()), btnUpdateTags, SLOT(animateClick()));
  
  connect(parent, SIGNAL(libraryAllChanged()), this, SLOT(slotCompleteRefresh()));

  txtSearch->installEventFilter(this);

  _dflt_lineedit_pal = txtSearch->palette();

  slotSelectionChanged(); // toggle by the way refreshbuttonsenabled() and refreshpreview()
}
KLFLibraryBrowser::~KLFLibraryBrowser()
{
}
void KLFLibraryBrowser::setupResourcesListsAndTabs()
{
  // clear our lists
  int k;
  for (k = 0; k < mLists.size(); ++k)
    delete mLists[k];
  mLists.clear();

  if (mFrameResourcesLayout) {
    delete mFrameResourcesLayout;
  }
  if (btnManageResources)
    delete btnManageResources;
  if (tabResources) {
    delete tabResources;
  }

  mFrameResourcesLayout = new QGridLayout(frmList);
  mFrameResourcesLayout->setMargin(2);

  tabResources = new QTabWidget(frmList);

  mFrameResourcesLayout->addWidget(tabResources);

  if (mManageResourcesMenu == 0) {
    // create menu if needed
    mManageResourcesMenu = new QMenu(this);
    MANAGERESOURCESMENU_ADD =
      mManageResourcesMenu->addAction(tr("Add Resource ..."), this, SLOT(slotAddResource()));
    MANAGERESOURCESMENU_RENAME =
      mManageResourcesMenu->addAction(tr("Rename Resource ..."), this, SLOT(slotRenameResource()));
    MANAGERESOURCESMENU_DELETE =
      mManageResourcesMenu->addAction(tr("Delete Resource"), this, SLOT(slotDeleteResource()));
  }

  btnManageResources = new QPushButton(tr("Resources"), tabResources);
  QFont ff = btnManageResources->font();
  ff.setPointSize(ff.pointSize()-1);
  btnManageResources->setFont(ff);
  btnManageResources->setMenu(mManageResourcesMenu);
  QPalette pal = btnManageResources->palette();
  pal.setColor(QPalette::Button, QColor(128, 150, 150));
  tabResources->setCornerWidget(btnManageResources, Qt::TopRightCorner);

  if (_libresptr->size() == 0) {
    fprintf(stderr, "KLFLibraryBrowser: RESOURCE LIST IS EMPTY! EXPECT A CRASH AT ANY MOMENT !!\n");
  }

  uint flags;
  for (k = 0; k < _libresptr->size(); ++k) {
    QTreeWidget *lstview = new QTreeWidget(tabResources);
    lstview->setProperty("resourceId", (unsigned int)_libresptr->operator[](k).id);
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

    connect(manager, SIGNAL(restoreFromLibrary(KLFData::KLFLibraryItem, bool)),
	    this, SIGNAL(restoreFromLibrary(KLFData::KLFLibraryItem, bool)));
    connect(manager, SIGNAL(selectionChanged()), this, SLOT(slotSelectionChanged()));

    mLists.append(manager);
  }

  // it seems like objects created after the initial show() have to be shown manually...
  tabResources->show();

  emitLibraryChanged();

  connect(tabResources, SIGNAL(currentChanged(int)), this, SLOT(slotTabResourcesSelected(int)));
  _currentList = 0;
  tabResources->setCurrentIndex(0);
  slotTabResourcesSelected(0);
}


void KLFLibraryBrowser::slotTabResourcesSelected(int selected)
{
  QWidget *cur = tabResources->widget(selected);
  int k;
  _currentList = 0;
  for (k = 0; k < mLists.size(); ++k) {
    if (mLists[k]->myResource().id == cur->property("resourceId").toUInt()) {
      _currentList = mLists[k];
    }
  }

  if (mImportExportMenu) {
    IMPORTEXPORTMENU_IMPORTTOCURRENTRESOURCE->setEnabled(_currentList != 0);
    IMPORTEXPORTMENU_EXPORT_SELECTION->setEnabled(_currentList != 0 && _currentList->getSelectedInfo_hasselection());
    IMPORTEXPORTMENU_EXPORT_RESOURCE->setEnabled(_currentList != 0);
    IMPORTEXPORTMENU_EXPORT_RESOURCE->setText(tr("Export Resource [ %1 ] ...")
					      .arg(_currentList ? _currentList->myResource().name : tr("<none>")));
  }
  if (mManageResourcesMenu) {
    if (_currentList) {
      int id = _currentList->myResource().id;
      MANAGERESOURCESMENU_DELETE->setEnabled(KLFData::LibResourceUSERMIN <= id && id <= KLFData::LibResourceUSERMAX);
    } else {
      MANAGERESOURCESMENU_DELETE->setEnabled(false);
    }
  }

  slotRefreshButtonsEnabled();
  slotRefreshPreview();
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
  if (obj == txtSearch) {
    if (ev->type() == QEvent::KeyPress) {
      QKeyEvent *kev = (QKeyEvent*)ev;
      if (_currentList) {
	if (kev->key() == Qt::Key_F3) { // find next
	  if (QApplication::keyboardModifiers() & Qt::ShiftModifier)
	    slotSearchFindPrev();
	  else
	    slotSearchFindNext();
	  return true;
	}
	if (kev->key() == Qt::Key_Escape) { // abort search
	  _currentList->slotSearchAbort();
	  txtSearch->setText(""); // reset search
	  resetLneSearchColor();
	}
      }
    }
  }
  return QWidget::eventFilter(obj, ev);
}

void KLFLibraryBrowser::emitLibraryChanged()
{
  emit libraryChanged();
  //   int k;
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
  // Now by this we mean a COMPLETE refresh !
  setupResourcesListsAndTabs();
  emitLibraryChanged();
}

void KLFLibraryBrowser::slotSelectionChanged()
{
  slotRefreshButtonsEnabled();
  slotRefreshPreview();
  if (mImportExportMenu)
    IMPORTEXPORTMENU_EXPORT_SELECTION->setEnabled(_currentList != 0 && _currentList->getSelectedInfo_hasselection());
}

void KLFLibraryBrowser::slotUpdateEditedCategory()
{
  QString c = cbxCategory->currentText();
  // purify c
  c = c.trimmed();
  if (!c.isEmpty()) {
    QStringList sections = c.split("/", QString::SkipEmptyParts); // no empty sections
    int k;
    for (k = 0; k < sections.size(); ++k) {
      //      printf("Section %d: '%s'\n", k, (const char*)sections[k].local8Bit());
      sections[k] = sections[k].simplified();
      if (sections[k].isEmpty()) {
	sections.removeAt(k);
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
    lblPreview->setPixmap(QPixmap(":/pics/nopixmap.png"));
    txtPreviewLatex->setText("");
    cbxCategory->setEditText("");
    cbxTags->setEditText("");
    lblStylePreview->setText("");
    wPreview->setCurrentIndex(0);

    wPreview->setEnabled(false);
    return;
  }
  // populate comboboxes
  KLFData::KLFLibraryResource curres = _currentList->myResource();
  QStringList categories;
  int j;
  int k;
  for (j = 0; j < _libresptr->size(); ++j) {
    KLFData::KLFLibraryList l = _libptr->operator[](_libresptr->operator[](j));
    for (k = 0; k < l.size(); ++k) {
      QString c = l[k].category;
      if (!c.isEmpty() && categories.indexOf(c) == -1)
	categories.append(c);
    }
  }
  categories.sort();
  cbxCategory->clear();
  cbxCategory->addItems(categories);
  // show the previews
  int can_edit;
  wPreview->setEnabled(true);
  lblPreview->setPixmap(_currentList->getSelectedInfo_preview());
  txtPreviewLatex->setText(_currentList->getSelectedInfo_latex());
  txtPreviewLatex->setEnabled(_currentList->getSelectedInfo_hasselection());
  cbxCategory->setEditText(_currentList->getSelectedInfo_category(&can_edit));
  cbxCategory->setEnabled(can_edit);
  cbxTags->setEditText(_currentList->getSelectedInfo_tags(&can_edit));
  cbxTags->setEnabled(can_edit);
  lblStylePreview->setText(_currentList->getSelectedInfo_style());
}


void KLFLibraryBrowser::slotRestoreAll(QTreeWidgetItem *)
{
  if (!_currentList)
    return;
  _currentList->slotRestoreAll();
}
void KLFLibraryBrowser::slotRestoreLatex(QTreeWidgetItem *)
{
  if (!_currentList)
    return;
  _currentList->slotRestoreLatex();
}
void KLFLibraryBrowser::slotDelete(QTreeWidgetItem *)
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
  CONFIGMENU_DISPLAYTAGGEDONLY->setChecked(display);
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
  CONFIGMENU_NODUPLICATES->setChecked(display);
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
  txtSearch->setText("");
  txtSearch->setFocus();
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
  QPalette pal = txtSearch->palette();
  if (res) {
    pal.setColor(QPalette::Base, klfconfig.LibraryBrowser.colorFound);
  } else {
    pal.setColor(QPalette::Base, klfconfig.LibraryBrowser.colorNotFound);
  }
  txtSearch->setPalette(pal);
}
void KLFLibraryBrowser::slotSearchFindNext(int direction)
{
  QString s = txtSearch->text();

  if (s.isEmpty()) {
    resetLneSearchColor();
    return;
  }

  if (!_currentList)
    return;
  bool res = _currentList->slotSearchFindNext(direction);

  QPalette pal = txtSearch->palette();
  if (res) {
    pal.setColor(QPalette::Base, klfconfig.LibraryBrowser.colorFound);
  } else {
    pal.setColor(QPalette::Base, klfconfig.LibraryBrowser.colorNotFound);
  }
  txtSearch->setPalette(pal);
}
void KLFLibraryBrowser::slotSearchFindPrev()
{
  slotSearchFindNext(-1);
}

void KLFLibraryBrowser::resetLneSearchColor()
{
  txtSearch->setPalette(_dflt_lineedit_pal);
}


void KLFLibraryBrowser::slotImport(bool keepResources)
{
  if (_currentList == 0 && ! keepResources) {
    fprintf(stderr, "ERROR: Can't insert into current resource: currentList is zero\n");
    return;
  }

  KLFData::KLFLibrary imp_lib = KLFData::KLFLibrary();
  KLFData::KLFLibraryResourceList imp_reslist = KLFData::KLFLibraryResourceList();

  QString fname = QFileDialog::getOpenFileName(this, tr("Import Library Resource"), QString(),
					       tr("KLatexFormula Library Files (*.klf);;All Files (*)"));
  if (fname.isEmpty())
    return;

  QFile fimp(fname);
  if ( ! fimp.open(QIODevice::ReadOnly) ) {
    QMessageBox::critical(this, tr("Error"), tr("Unable to open library file %1!").arg(fname));
  } else {
    QDataStream stream(&fimp);
    QString s1;
    stream >> s1;
    if (s1 != "KLATEXFORMULA_LIBRARY_EXPORT") {
      QMessageBox::critical(this, tr("Error"), tr("Error: Library file `%1' is incorrect or corrupt!\n").arg(fname));
    } else {
      qint16 vmaj, vmin;
      stream >> vmaj >> vmin;

      // ATTENTION: vmaj and vmin correspond to the compatibility version.
      // for example in KLF 3 we write vmaj=2 and vmin=1 because klf3 is
      // compatible with KLF 2.1 format.

      if (vmaj > version_maj || (vmaj == version_maj && vmin > version_min)) {
	QMessageBox::information(this, tr("Import Library Items"),
				 tr("This library file was created by a more recent version of KLatexFormula.\n"
				    "The process of library importing may fail."));
      }

      // Qt3-compatible stream input
      stream.setVersion(QDataStream::Qt_3_3);

      stream >> imp_reslist >> imp_lib;
    } // format corrupt
  } // open file ok

  KLFData::KLFLibraryResource curres;
  if (!keepResources)
    curres = _currentList->myResource();

  int k, j, l;
  for (k = 0; k < imp_reslist.size(); ++k) {
    KLFData::KLFLibraryResource res = imp_reslist[k];
    if (keepResources) { // we're keeping resources
      if (KLFData::LibResourceUSERMIN <= res.id && res.id <= KLFData::LibResourceUSERMAX) {
	// this is a USER RESOURCE
	//int l;
	for (l = 0; l < _libresptr->size() && ! resources_equal_for_import(_libresptr->operator[](l), res) ; ++l)
	  ;
	// if l >= size => not found
	if (l >= _libresptr->size()) {
	  // resource doesn't exist yet -> insert resource with a free ID
	  uint maxid = KLFData::LibResourceUSERMIN;
	  for (l = 0; l < _libresptr->size(); ++l) {
	    uint thisID = _libresptr->operator[](l).id;
	    if (thisID < KLFData::LibResourceUSERMIN || thisID > KLFData::LibResourceUSERMAX)
	      continue;
	    if (l == 0 || maxid < thisID)
	      maxid = thisID;
	  }
	  res.id = maxid+1;

	  if (res.id >= KLFData::LibResourceUSERMIN && res.id <= KLFData::LibResourceUSERMAX) {
	    // and insert resource
	    _libresptr->append(res);
	  } else {
	    fprintf(stderr, "ERROR: Can't find good resource ID!\n");
	    continue;
	  }
	} else {
	  // USER RESOURCE, but already existing
	  // found exact or near match (fine-tuning of matches done in resources_equal_for_import)
	  // make sure res is the exact local resource
	  res = _libresptr->operator[](l);
	}
      } else {
	// system resource (ie. Not user resource)
	// ignore name and use same ID (for translation issues)
	for (l = 0; l < _libresptr->size() && _libresptr->operator[](l).id != res.id; ++l)
	  ;
	if (l >= _libresptr->size()) {
	  // resource doesn't exist, create it
	  _libresptr->append(res);
	  // and keep res as it is
	} else {
	  // using system resource, make sure res is the same as ours
	  res = _libresptr->operator[](l); // has same ID, adapting to local name.
	}
      }
    } else {
      // we're not keeping resources
      res = curres;
    }
    for (j = 0; j < imp_lib[imp_reslist[k]].size(); ++j) {
      KLFData::KLFLibraryItem item = imp_lib[imp_reslist[k]][j];
      // readjust item id
      item.id = KLFData::KLFLibraryItem::MaxId++;
      _libptr->operator[](res).append(item);
    }
  }
  // and do a complete refresh (resources may have changed)
  slotCompleteRefresh();
}
void KLFLibraryBrowser::slotImportToCurrentResource()
{
  slotImport(false); // import to current resource
}
void KLFLibraryBrowser::slotExportLibrary()
{
  slotExportSpecific(*_libresptr, *_libptr);
}
void KLFLibraryBrowser::slotExportResource()
{
  if (!_currentList) {
    fprintf(stderr, "Hey! No resource selected! What are you expecting to export!!?!\n");
    return;
  }

  // the resource we're saving
  KLFData::KLFLibraryResource res = _currentList->myResource();

  // make KLFData::KLFLibraryResourceList for Resources with names and ids
  KLFData::KLFLibraryResourceList reslist;
  reslist.append(res);
  KLFData::KLFLibrary lib;
  lib[res] = _libptr->operator[](res); // create single resource and fill it

  slotExportSpecific(reslist, lib);
}
void KLFLibraryBrowser::slotExportSelection()
{
  if (!_currentList) {
    fprintf(stderr, "Hey ! No resource selected ! Where do you think you can get a selection from ?\n");
    return;
  }

  // the current resource we're saving items from
  KLFData::KLFLibraryResource res = _currentList->myResource();

  KLFData::KLFLibrary lib;
  KLFData::KLFLibraryResourceList reslist;
  reslist.append(res);

  lib[res] = _currentList->getSelectedLibraryItems();

  slotExportSpecific(reslist, lib);
}

void KLFLibraryBrowser::slotExportSpecific(KLFData::KLFLibraryResourceList reslist, KLFData::KLFLibrary library)
{
  QString fname = QFileDialog::getSaveFileName(this, tr("Export Library Resource"), QString(),
					       tr("KLatexFormula Library Files (*.klf);;All Files (*)"));
  if (fname.isEmpty())
    return;

  //   if (QFile::exists(fname)) {
  //     int res = QMessageBox::questionYesNoCancel(this, i18n("Specified file exists. Overwrite?"), i18n("Overwrite?"));
  //     if (res == KMessageBox::No) {
  //       slotExportResource(); // recurse
  //       // and quit
  //       return;
  //     }
  //     if (res == KMessageBox::Cancel) {
  //       // quit directly
  //       return;
  //     }
  //     // if res == KMessageBox::Yes, we may continue ...
  //   }

  QFile fsav(fname);
  if ( ! fsav.open(QIODevice::WriteOnly) ) {
    QMessageBox::critical(this, tr("Error !"), tr("Error: Can't write to file `%1'!").arg(fname));
    return;
  }

  QDataStream stream(&fsav);

  stream.setVersion(QDataStream::Qt_3_3);
  // WE'RE WRITING KLF 2.1 compatible output, so we write as being version 2.1
  stream << QString("KLATEXFORMULA_LIBRARY_EXPORT") << (qint16)2 << (qint16)1
	 << reslist << library;
}

void KLFLibraryBrowser::slotAddResource()
{
  bool ok;
  QString rname = QInputDialog::getText(this, tr("New Resource"), tr("Please Enter New Resource Name"), QLineEdit::Normal,
					tr("New Resource"), &ok);
  rname = rname.trimmed();
  if ( ! ok || rname.isEmpty() )
    return;

  quint32 ourid = KLFData::LibResourceUSERMIN; // determine a good ID:
  int k;
  for (k = 0; ourid < KLFData::LibResourceUSERMAX && k < _libresptr->size(); ++k) {
    if (ourid == _libresptr->operator[](k).id) {
      ourid++;
      k = 0; // restart a complete check from first item
    }
  }
  if (ourid == KLFData::LibResourceUSERMAX) {
    fprintf(stderr, "ERROR: Can't find a good ID !\n");
    return;
  }

  KLFData::KLFLibraryResource res = { ourid, rname };

  _libresptr->append(res);
  _libptr->operator[](res) = KLFData::KLFLibraryList();

  // and do a thorough refresh
  slotCompleteRefresh();
}
void KLFLibraryBrowser::slotRenameResource()
{
  // no current resource selected
  if (!_currentList)
    return;

  bool ok;
  QString rname = QInputDialog::getText(this, tr("Rename Resource"), tr("Please Enter New Resource Name"), QLineEdit::Normal,
					_currentList->myResource().name, &ok);
  rname = rname.simplified();
  if ( ! ok || rname.isEmpty() )
    return;

  KLFData::KLFLibraryResource res = _currentList->myResource();
  KLFData::KLFLibraryResource newres = res;
  newres.name = rname;

  int it = _libresptr->indexOf(res);
  KLFData::KLFLibrary::iterator libit = _libptr->find(res);
  if (it == -1 || libit == _libptr->end()) {
    fprintf(stderr, "ERROR: Can't find items in library !\n");
    return;
  }
  _libresptr->operator[](it) = newres;
  KLFData::KLFLibraryList l = *libit;
  _libptr->erase(libit);
  _libptr->operator[](newres) = l;

  // and do a thorough refresh
  slotCompleteRefresh();
}
void KLFLibraryBrowser::slotDeleteResource()
{
  if ( ! _currentList )
    return;
  KLFData::KLFLibraryResource r = _currentList->myResource();
  if (r.id < KLFData::LibResourceUSERMIN || r.id > KLFData::LibResourceUSERMAX)
    return;

  int k;
  for (k = 0; k < _libresptr->size() && _libresptr->operator[](k).id != r.id; ++k)
    ;
  if (k >= _libresptr->size())
    return;

  if (QMessageBox::question(this, tr("Delete Resource"),
			    tr("<qt>Are you sure you want to delete resource <b>%1</b> with all its contents?</qt>").arg(r.name),
			    QMessageBox::Yes|QMessageBox::No, QMessageBox::No)
      == QMessageBox::Yes) {

    _libresptr->removeAt(k);
    _libptr->remove(r);

    // and do complete refresh
    slotCompleteRefresh();

  }
}

