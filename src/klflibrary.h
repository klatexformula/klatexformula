/***************************************************************************
 *   file klflibrary.h
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

#ifndef KLFLIBRARY_H
#define KLFLIBRARY_H

#include <qstring.h>
#include <qlistview.h>
#include <qtabwidget.h>

#include <kpopupmenu.h>

#include <klfdata.h>
#include <klflibrarybrowserui.h>

class KLFLibraryListCategoryViewItem : public QListViewItem
{
public:
  enum { RTTI = 1500 };
  /** A Root Category when showing in single-depth tree or in multi-depth tree. In either case, we need the
   * full category string here. */
  KLFLibraryListCategoryViewItem(QListView *parent, QString category);
  /** Subcategory when showing in multi-depth tree view: subcategory must contain full category path (ie.
   * "Physics/Relativity/Minkowski World") */
  KLFLibraryListCategoryViewItem(QString subcategory, KLFLibraryListCategoryViewItem *parent);
  virtual ~KLFLibraryListCategoryViewItem();

  virtual int rtti() const { return RTTI; }

  QString category() const { return _category; }

protected:
  QString _category;
};

class KLFLibraryListViewItem : public QListViewItem
{
public:
  enum { RTTI = 1501 };
  KLFLibraryListViewItem(QListViewItem *parent, KLFData::KLFLibraryItem item, int ind);
  KLFLibraryListViewItem(QListView *parent, KLFData::KLFLibraryItem item, int ind);
  virtual ~KLFLibraryListViewItem();

  virtual int rtti() const { return RTTI; }

  KLFData::KLFLibraryItem libraryItem() const { return _item; }
  int index() const { return _ind; }

  void updateLibraryItem(const KLFData::KLFLibraryItem& item);
  void updateIndex(int newindex) { _ind = newindex; }

protected:
  KLFData::KLFLibraryItem _item;
  int _ind;
};


QValueList<QListViewItem*> get_selected_items(QListView *lview, int givenRTTI = -1);


// ------------------------------------------------------------------------


class KLFLibraryBrowser;

class KLFLibraryListManager : public QObject
{
  Q_OBJECT
public:
  enum Flags { NoFlags = 0x00,
    ShowCategories = 0x01,
    ShowCategoriesDeepTree = 0x02,
    DefaultFlags = ShowCategories|ShowCategoriesDeepTree
  };
  KLFLibraryListManager(QListView *lview, KLFLibraryBrowser *parent, KLFData::KLFLibraryResource myresource,
			KLFData::KLFLibrary *wholelibrary, KLFData::KLFLibraryResourceList *resptr,
			uint flags = DefaultFlags);
  virtual ~KLFLibraryListManager();

  KLFData::KLFLibraryResource myResource() const { return _myresource; }

  bool canRestore() const;
  bool canDelete() const;

  bool getSelectedInfo_hasselection() const;
  bool getSelectedInfo_hasmuliple() const;
  bool getSelectedInfo_singleselection() const;
  QPixmap getSelectedInfo_preview() const;
  QString getSelectedInfo_latex() const;
  QString getSelectedInfo_category(int *canedit) const;
  QString getSelectedInfo_tags(int *canedit) const;
  QString getSelectedInfo_style() const;

  bool updateSelectedInfo_category(const QString& newcat);
  bool updateSelectedInfo_tags(const QString& tags);
  enum { UpdateCategory = 0x01, UpdateTags = 0x02 };
  bool updateSelectedInfo(uint which, const QString& category, const QString& tags);

signals:
  void restoreFromLibrary(KLFData::KLFLibraryItem h, bool restorestyle);
  void selectionChanged();
//  void libraryChanged();

public slots:

  void slotContextMenu(QListViewItem *item, const QPoint &p, int column);
  void slotRestoreAll(QListViewItem *i = 0);
  void slotRestoreLatex(QListViewItem *i = 0);
  void slotDelete(QListViewItem *i = 0);
  void slotCopyToResource(int indexInList);
  void slotMoveToResource(int indexInList);
  // search
  bool slotSearchFind(const QString& text);
  bool slotSearchFindNext();

  // complete refresh, typically connected to libraryChanged() of parent object
  void slotCompleteRefresh();

protected:
  KLFLibraryBrowser *_parent;
  QListView *_listView;

  KLFData::KLFLibrary *_fulllibraryptr;
  KLFData::KLFLibraryResourceList *_reslistptr;

  KLFData::KLFLibraryResource _myresource;
  /** KLFData::... library Items in current resource */
  KLFData::KLFLibraryList *_libItems;

  uint _flags;

  QValueList<KLFLibraryListCategoryViewItem*> mCategoryItems;

  bool is_duplicate_of_previous(uint indexInList);

  KLFLibraryListCategoryViewItem *itemForCategory(const QString& category, bool createifneeded);
  KLFLibraryListViewItem *itemForId(uint id);

  uint _search_k; // we're focusing on the k-th history item (decreases at each F3 key press [reverse search, remember!])
  QString _search_str; // the previous search string

  bool _am_libchg_originator; // flag to know if WE are the one who emitted the libraryChanged() signal
  bool _dont_emit_libchg; // internal flag for slotMoveToResource() to tell slotCopyToResource()/slotDelete() NOT to emit libraryChanged()
};




// ------------------------------------------------------------------------

class KLFMainWin;

class KLFLibraryBrowser : public KLFLibraryBrowserUI
{
  Q_OBJECT
public:
  KLFLibraryBrowser(KLFData::KLFLibrary *wholelist, KLFData::KLFLibraryResourceList *resourcelist, KLFMainWin *parent);
  virtual ~KLFLibraryBrowser();

  bool eventFilter(QObject *obj, QEvent *e);

  void emitLibraryChanged();

signals:
  void libraryChanged();
  void restoreFromLibrary(KLFData::KLFLibraryItem j, bool restorestyle);
  void refreshLibraryBrowserShownState(bool shown);

public slots:
  void addToHistory(KLFData::KLFLibraryItem j);

  void slotDisplayTaggedOnly();
  void slotDisplayTaggedOnly(bool display);
  void slotDisplayNoDuplicates();
  void slotDisplayNoDuplicates(bool display);
  void slotClose();
  void slotRefreshButtonsEnabled();
  void slotTabResourcesSelected(QWidget *selected);
  void slotCompleteRefresh();
  // preview
//  void slotCategoryEdited(const QString& newtext);
//  void slotTagsEdited(const QString& newtags);
  void slotUpdateEditedCategory();
  void slotUpdateEditedTags();
  void slotRefreshPreview();
  // button actions
  void slotRestoreAll(QListViewItem *i = 0);
  void slotRestoreLatex(QListViewItem *i = 0);
  void slotDelete(QListViewItem *i = 0);
  // search
  void slotSearchClear();
  void slotSearchFind(const QString& text);
  void slotSearchFindNext();

protected slots:
  // internal use
  void resetLneSearchColor();


protected:
  KLFData::KLFLibrary *_libptr;
  KLFData::KLFLibraryResourceList *_libresptr;

  QValueList<KLFLibraryListManager*> mLists;
  KLFLibraryListManager *_currentList;

  QTabWidget *tabResources;

  bool _allowrestore;
  bool _allowdelete;

  KPopupMenu *mRestoreMenu;
  KPopupMenu *mConfigMenu;

  // search bar default bg color
  QColor _dflt_lineedit_bgcol;

  void reject();
  void closeEvent(QCloseEvent *e);

  void setupResourcesListsAndTabs();
};


#endif
