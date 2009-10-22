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

#include <QString>
#include <QListWidget>
#include <QTabWidget>
#include <QGridLayout>
#include <QMenu>
#include <QPalette>
#include <QTreeWidget>
#include <QTreeWidgetItem>


#include <klfdata.h>
#include <ui_klflibrarybrowserui.h>


class KLFLibraryListCategoryViewItem : public QTreeWidgetItem
{
public:
  enum { Type = 1500 };
  /** A Root Category when showing in single-depth tree or in multi-depth tree. In either case, we need the
   * full category string here. */
  KLFLibraryListCategoryViewItem(QTreeWidget *parent, QString category);
  /** Subcategory when showing in multi-depth tree view: subcategory must contain full category path (ie.
   * "Physics/Relativity/Minkowski World") */
  KLFLibraryListCategoryViewItem(QString subcategory, KLFLibraryListCategoryViewItem *parent);

  virtual ~KLFLibraryListCategoryViewItem();

  QString category() const { return _category; }

protected:
  QString _category;
  bool _issubcategory;
private:
  void setup_category_item();
};

class KLFLibraryListViewItem : public QTreeWidgetItem
{
public:
  enum { Type = 1501 };

  KLFLibraryListViewItem(QTreeWidgetItem *parent, const KLFData::KLFLibraryItem& item, int ind);
  KLFLibraryListViewItem(QTreeWidget *parent, const KLFData::KLFLibraryItem& item, int ind);
  KLFLibraryListViewItem(const KLFData::KLFLibraryItem& item, int ind);
  virtual ~KLFLibraryListViewItem();

  virtual QSize sizeHint(int column) const;

  KLFData::KLFLibraryItem libraryItem() const { return _item; }
  int index() const { return _ind; }

  void updateLibraryItem(const KLFData::KLFLibraryItem& item);
  void updateIndex(int newindex) { _ind = newindex; }

protected:
  KLFData::KLFLibraryItem _item;
  int _ind;
};


QList<QTreeWidgetItem*> get_selected_items(QTreeWidget *w, int givenType);


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
  KLFLibraryListManager(QTreeWidget *lview, KLFLibraryBrowser *parent, KLFData::KLFLibraryResource myresource,
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

  KLFData::KLFLibraryList getSelectedLibraryItems() const;


signals:
  void restoreFromLibrary(KLFData::KLFLibraryItem h, bool restorestyle);
  void selectionChanged();

public slots:

  void slotContextMenu(const QPoint &p);
  void slotRestoreAll();
  void slotRestoreLatex();
  void slotDelete();
  // dont ask confirmation and update library at end:
  void slotDelete(const QList<uint>& toDeleteIds);
  void slotCopyToResource(bool move = false);
  void slotMoveToResource();
  void slotCopyMoveToResource(uint resourceId, bool move = false);
  // search
  bool slotSearchFind(const QString& text);
  bool slotSearchFindNext(int direction = 1);
  bool slotSearchFindPrev();
  void slotSearchAbort();

  // complete refresh, typically connected to libraryChanged() of parent object
  void slotCompleteRefresh();

protected:
  KLFLibraryBrowser *_parent;
  QTreeWidget *_listView;

  KLFData::KLFLibrary *_fulllibraryptr;
  KLFData::KLFLibraryResourceList *_reslistptr;

  KLFData::KLFLibraryResource _myresource;
  /** KLFData::... library Items in current resource */
  KLFData::KLFLibraryList *_libItems;

  uint _flags;

  QList<KLFLibraryListCategoryViewItem*> mCategoryItems;

  bool is_duplicate_of_previous(uint indexInList);

  KLFLibraryListCategoryViewItem *itemForCategory(const QString& category, bool createifneeded = true);
  KLFLibraryListViewItem *itemForId(uint id);

  int _search_k; // we're focusing on the k-th history item (decreases at each F3 key press [reverse search, remember!])
  QString _search_str; // the previous search string

  bool _am_libchg_originator; // flag to know if WE are the one who emitted the libraryChanged() signal
  bool _dont_emit_libchg; // internal flag for slotMoveToResource() to tell slotCopyToResource()/slotDelete() NOT to emit libraryChanged()
};




// ------------------------------------------------------------------------

class KLFMainWin;

class KLFLibraryBrowser : public QWidget, private Ui::KLFLibraryBrowserUI
{
  Q_OBJECT
public:
  KLFLibraryBrowser(KLFData::KLFLibrary *wholelist, KLFData::KLFLibraryResourceList *resourcelist, KLFMainWin *parent);
  virtual ~KLFLibraryBrowser();

  bool eventFilter(QObject *obj, QEvent *e);

  void emitLibraryChanged();

  bool importLibraryFileSeparateResources(const QString& fname, const QString& baseresource);

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
  void slotTabResourcesSelected(int selected);
  // Now by this we mean a COMPLETE refresh (tab widget is recreated, ...) !
  void slotCompleteRefresh();
  // when selection has changed
  void slotSelectionChanged();
  // preview
  void slotUpdateEditedCategory();
  void slotUpdateEditedTags();
  void slotRefreshPreview();
  // button actions
  void slotRestoreAll(QTreeWidgetItem *i = 0);
  void slotRestoreLatex(QTreeWidgetItem *i = 0);
  void slotDelete(QTreeWidgetItem *i = 0);
  // search
  void slotSearchClear();
  void slotSearchFind(const QString& text);
  void slotSearchFindNext(int direction = 1);
  void slotSearchFindPrev();
  // import/export actions
  // will restore each item into its given resource in file if keepResources is true
  void slotImport(bool keepResources = true);
  void slotImportToCurrentResource();
  void slotImportToSeparateResources();
  void slotExportLibrary();
  void slotExportResource();
  void slotExportSelection();
  //   internal use mostly, prompt user for filename:
  void slotExportSpecific(KLFData::KLFLibraryResourceList reslist, KLFData::KLFLibrary library);
  // resource management
  void slotAddResource();
  void slotRenameResource();
  void slotDeleteResource();

protected slots:
  // internal use
  void resetLneSearchColor();


protected:
  KLFData::KLFLibrary *_libptr;
  KLFData::KLFLibraryResourceList *_libresptr;

  QList<KLFLibraryListManager*> mLists;
  KLFLibraryListManager *_currentList;

  QGridLayout *mFrameResourcesLayout;
  QTabWidget *tabResources;
  QPushButton *btnManageResources;
  QMenu *mManageResourcesMenu;

  bool _allowrestore;
  bool _allowdelete;

  QMenu *mRestoreMenu;
  QMenu *mConfigMenu;

  QMenu *mImportExportMenu;

  // search bar default palette
  QPalette _dflt_lineedit_pal;

  void closeEvent(QCloseEvent *e);

  void setupResourcesListsAndTabs();

  QAction *CONFIGMENU_DISPLAYTAGGEDONLY;
  QAction *CONFIGMENU_NODUPLICATES;
  QAction *IMPORTEXPORTMENU_IMPORT;
  QAction *IMPORTEXPORTMENU_IMPORTTOCURRENTRESOURCE;
  QAction *IMPORTEXPORTMENU_EXPORT_LIBRARY;
  QAction *IMPORTEXPORTMENU_EXPORT_RESOURCE;
  QAction *IMPORTEXPORTMENU_EXPORT_SELECTION;
  QAction *IMPORTEXPORTMENU_IMPORTTOSEPARATERESOURCE;
  QAction *MANAGERESOURCESMENU_ADD;
  QAction *MANAGERESOURCESMENU_DELETE;
  QAction *MANAGERESOURCESMENU_RENAME;

  quint32 getNewResourceId();
  bool loadLibraryFile(const QString& fname, KLFData::KLFLibraryResourceList *res, KLFData::KLFLibrary *lib);
};


#endif
