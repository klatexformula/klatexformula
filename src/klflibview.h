/***************************************************************************
 *   file klflibview.h
 *   This file is part of the KLatexFormula Project.
 *   Copyright (C) 2010 by Philippe Faist
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
/* $Id$ */

#ifndef KLFLIBVIEW_H
#define KLFLIBVIEW_H

#include <QAbstractItemModel>
#include <QAbstractItemDelegate>
#include <QMimeData>
#include <QEvent>
#include <QWidget>
#include <QDialog>
#include <QAbstractButton>
#include <QTreeView>
#include <QItemSelectionModel>
#include <QTextDocument>
#include <QTextCharFormat>
#include <QStandardItemModel>
#include <QListView>

#include <klfdefs.h>
#include <klflib.h>



namespace KLFLib {
  enum RestoreMode {
    RestoreLatex = 0x0001,
    RestoreStyle = 0x0002,

    RestoreLatexAndStyle = RestoreLatex|RestoreStyle,
    RestoreAll = 0xFFFF
  };
};


//! A view widget to display a library resource's contents
/** A base API for a widget that will display a KLFLibResourceEngine's contents.
 * For example one could create a QTreeView and display the contents in there
 * (that's what KLFLibDefaultView does...).
 *
 * \note This class subclasses QWidget because it makes uses of signals and slots.
 *   Before editing this file to change that, think before removing that inheritance
 *   or muting it to a QObject (subclasses will eventually want to inherit QWidget),
 *   no multiple QObject inheritance please!
 *
 * \note Subclasses should emit the \ref resourceDataChanged() signal AFTER they
 *   refresh after a resource data change.
 *
 * Note: The operationStartReportingProgress() signal is purposed for when long update
 * operations are necessary. It is to be used by subclasses exactly like in
 * KLFLibResourceEngine.
 *
 * \warning The design of this class with a \ref setResourceEngine() function makes it
 *   unsafe to blindly assume a non-NULL resource engine pointer at all times. Check
 *   the pointer before using it! See \ref validResourceEngine().
 */
class KLF_EXPORT KLFAbstractLibView : public QWidget
{
  Q_OBJECT
public:
  KLFAbstractLibView(QWidget *parent);
  virtual ~KLFAbstractLibView() { }

  virtual KLFLibResourceEngine * resourceEngine() const { return pResourceEngine; }

  //! Display Resource URL. <i>NOT exactly like KLFLibResourceEngine::url() !</i>
  /** Returns an URL describing exactly what is shown to user, based on the resource
   * engine's URL.
   *
   * \warning This does not simply return <tt>resourceEngine()->url()</tt>. It returns
   *   in the URL exactly the information that is displayed to user. For example, a
   *   view that can handle displaying all sub-resources will NOT include the default
   *   sub-resource in its URL, however a view displaying only the default sub-resource
   *   will include a "klfDefaultSubResource" query item to distinguish it from another
   *   view acting on the same base URL but displaying a different sub-resource.
   */
  virtual QUrl url() const = 0;

  //! Compare this resource view's URL to another URL
  /** Compares the URL of the resource we are viewing with the URL \c other, and
   * returns an OR-ed combination of enum \ref KlfUrlCompareFlag values of URL-comparision
   * tests that have turned out to be true (see \ref KlfUrlCompareFlag for a list
   * of tests).
   *
   * This function is supposed to answer to the following questions, each a condition
   * as whether to return a URL comparation flag or not:
   * - flag \c KlfUrlCompareEqual: does this view show the same information as a view
   *   whose URL would be \c other ?
   * - flag \c KlfUrlCompareLessSpecific: does this view show _MORE_ information than
   *   the \c other view ("less specific information" <=> "shows more") ?
   * - flag \c KLFUrlCompareMoreSpecific: does this view show _LESS_ information than
   *   the \c other view ?
   * - flag \c KLFUrlComapreBaseEqual: does this view show the same resource as the
   *   \c other view, but possibly does not the same specific information, eg. not the
   *   same sub-resource.
   *
   * The \c interestFlags is a binary OR'ed value of KlfUrlCompareFlag values of tests
   * to be performed. Any flag that is set in \c interestFlags indicates that the return
   * value of this function, when binary-AND'ed with that flag, is the result (T or F)
   * of the test the flag stands for. However, if a flag is not set in \c interestFlags,
   * its state in the return value by this function is undefined.
   *
   * See also klfUrlCompare().
   */
  virtual uint compareUrlTo(const QUrl& other, uint interestFlags = 0xFFFFFFFF) const = 0;


  //! Returns TRUE if a non-\c NULL resource engine has been set
  inline bool validResourceEngine() const { return pResourceEngine != NULL; }

  virtual void setResourceEngine(KLFLibResourceEngine *resource);

  /** Subclasses should return a list of entries that have been selected by the user. */
  virtual KLFLibEntryList selectedEntries() const = 0;

  /** Subclasses may add items to the context menu by returning them in this function.
   * \param pos is the position relative to widget where the menu was requested.
   *
   * \note The view itself does not handle context menus. This function is provided
   *   for whichever class uses this view to add these actions when that class creates
   *   a context menu.
   *
   * The default implementation returns an empty list. */
  virtual QList<QAction*> addContextMenuActions(const QPoint& pos);

  /** Saves the current GUI state (eg. column widths and order, etc.) */
  virtual QVariantMap saveGuiState() const = 0;

  /** Restores the state described in \c state (which was previously, possibly in another
   * session, returned by \ref saveGuiState()) */
  virtual bool restoreGuiState(const QVariantMap& state) = 0;

  /** Subclasses should reimplement this function to return a list of all known categories
   * to suggest to the user, eg. in a completion list for an editable combo box to edit
   * categories. */
  virtual QStringList getCategorySuggestions() = 0;

signals:
  /** Is emitted (by subclasses) when a latex entry is selected to be restored (eg. the entry was
   * double-clicked).
   *
   * \param entry is the data to restore
   * \param restoreflags provides information on which part of \c entry to restore. See the
   *   possible flags in \ref KLFLib::RestoreMode.
   */
  void requestRestore(const KLFLibEntry& entry, uint restoreflags = KLFLib::RestoreLatexAndStyle);
  /** Is emitted (by subclasses) when the view wants the main application (or the framework that uses
   * this class) to restore the given style \c style. No latex is to be restored.
   */
  void requestRestoreStyle(const KLFStyle& style);

  /** Subclasses must emit this signal AFTER they have refreshed. */
  void resourceDataChanged(const QList<KLFLib::entryId>& entryIdList);

  /** Emitted when the selection has changed, eg. user selected or deselected some entries. */
  void entriesSelected(const KLFLibEntryList& entries);

  /** Subclasses should emit this signal with lists of categories they come accross,
   * so that the editor can suggest these as completions upon editing category */
  void moreCategorySuggestions(const QStringList& categorylist);

  /** Emitted by subclasses to announce the beginning of a long operation during which progress
   * will be reported through the given \c progressReporter.
   *
   * Should be used in the same way as KLFLibResourceEngine::operationStartReportingProgress().
   */
  void operationStartReportingProgress(KLFProgressReporter *progressReporter, const QString& descriptiveText);

public slots:
  virtual void updateResourceEngine() = 0;
  virtual void updateResourceProp(int propId) = 0;
  virtual void updateResourceData(const QString& subres, int modifyType,
				  const QList<KLFLib::entryId>& entryIdList) = 0;
  /** Default implementation calls updateResourceEngine() */
  virtual void updateResourceDefaultSubResourceChanged(const QString& newSubResource);
  virtual bool writeEntryProperty(int property, const QVariant& value) = 0;
  /** Provides a reasonable default implementation that should suit for most purposes. */
  virtual bool writeEntryCategory(const QString& category)
  { return writeEntryProperty(KLFLibEntry::Category, category); }
  /** Provides a reasonable default implementation that should suit for most purposes. */
  virtual bool writeEntryTags(const QString& tags)
  { return writeEntryProperty(KLFLibEntry::Tags, tags); }
  virtual bool deleteSelected(bool requireConfirm = true) = 0;
  virtual bool insertEntries(const KLFLibEntryList& entries) = 0;
  /** Provides a reasonable default implementation that should suit for most purposes. */
  virtual bool insertEntry(const KLFLibEntry& entry)
  { return insertEntries(KLFLibEntryList() << entry); }

  /** This function should instruct the view to find the first occurence of the
   * string \c queryString, searching from top of the list if \c forward is TRUE, or reverse
   * from end of list of FALSE.
   *
   * The reimplementation should call from time to time
   * \code qApp->processEvents() \endcode
   * to keep the GUI from freezing in long resources.
   *
   * \note If the reimplementation implements the above suggestion, note that the slot
   *   \ref searchAbort() may be called during that time! It is best to take that into
   *   account and provide a means to stop the search if that is the case.
   */
  virtual bool searchFind(const QString& queryString, bool forward = true) = 0;
  /** This function should instruct the view to find the next occurence of the query string
   * given by a previous call to \ref searchFind(). The search must be performed in the
   * direction given by \c forward (see \ref searchFind()).
   *
   * It is up to the sub-class to remember the query string and the current match location.
   *
   * This function should also call the applications's processEvents() to keep the GUI from
   * freezing. The instructions are the same as for \ref searchFind().
   */
  virtual bool searchFindNext(bool forward) = 0;
  virtual void searchAbort() = 0;

  /** Collects the necessary information and emits \ref requestRestore() */
  virtual void restore(uint restoreFlags = KLFLib::RestoreLatexAndStyle) = 0;
  /** Provides a reasonable default implementation that should suit for most purposes. */
  virtual void restoreWithStyle() { restore(KLFLib::RestoreLatexAndStyle); }
  /** Provides a reasonable default implementation that should suit for most purposes. */
  virtual void restoreLatexOnly() { restore(KLFLib::RestoreLatex); }

  /** Called by the owner of the view. This function fetches category suggestions
   * (by calling the virtual getCategorySuggestions() reimplemented by subclasses)
   * and emits the signal moreCategorySuggestions().
   *
   * Subclasses need not reimplement this function. */
  virtual void wantMoreCategorySuggestions();

private:
  KLFLibResourceEngine *pResourceEngine;
};


// -----------------

class KLF_EXPORT KLFLibViewFactory : public QObject
{
  Q_OBJECT
public:
  KLFLibViewFactory(const QStringList& viewTypeIdentifiers, QObject *parent = NULL);
  virtual ~KLFLibViewFactory();

  /** A list of view type identifiers that this factory can create.
   *
   * Individual view widget types are identified by their "view type identifiers". They
   * are not meant to be human-readable (eg. "LibModel+CategoryTree" or whatever)  */
  virtual QStringList viewTypeIdentifiers() { return pViewTypeIdentifiers; }

  /** A translated string to be shown to user (in a choice box for ex.) for
   * the given view widget type. (eg. <tt>tr("Tree View")</tt>) */
  virtual QString viewTypeTitle(const QString& viewTypeIdent) const = 0;

  /** \returns Whether this factory can create the given view widget for the given engine.
   *
   * This function may return false, for example if this widget factory creates a specialized
   * kind of widget that can only work with a given engine. */
  virtual bool canCreateLibView(const QString& viewTypeIdent, KLFLibResourceEngine *engine) = 0;

  /** Create a library view with the given widget \c parent. The view should reflect the contents
   * given by the resource engine \c resourceEngine . */
  virtual KLFAbstractLibView * createLibView(const QString& viewTypeIdent, QWidget *parent,
					     KLFLibResourceEngine *resourceEngine) = 0;

  /** Returns the default view type identifier. Create this view if you don't have any idea
   * which view you prefer.
   *
   * This actually returns the first view type identifier of the first registered factory. */
  static QString defaultViewTypeIdentifier();

  /** Returns the factory that can handle the URL scheme \c urlScheme, or NULL if no such
   * factory exists (ie. has been registered). */
  static KLFLibViewFactory *findFactoryFor(const QString& viewTypeIdentifier);

  /** Returns a combined list of all view type identifiers that the installed factories support.
   * ie. returns a list of all view type idents. we're capable of creating. */
  static QStringList allSupportedViewTypeIdentifiers();
  /** Returns the full list of installed factories. */
  static QList<KLFLibViewFactory*> allFactories() { return pRegisteredFactories; }

private:
  QStringList pViewTypeIdentifiers;

  static void registerFactory(KLFLibViewFactory *factory);
  static void unRegisterFactory(KLFLibViewFactory *factory);

  static QList<KLFLibViewFactory*> pRegisteredFactories;

};



// -----------------

class KLFLibModelCache;

/** \brief Model for Item-Views displaying a library resource's contents
 *
 * The Model can morph into different forms, for simulating various common & useful
 * displays (chronological list (history), category/tags tree (archive), maybe icons
 * in the future, ...).
 *
 */
class KLF_EXPORT KLFLibModel : public QAbstractItemModel
{
  Q_OBJECT
public:
  enum FlavorFlag {
    LinearList = 0x0001,
    CategoryTree = 0x0002,
    DisplayTypeMask = 0x000f,

    GroupSubCategories = 0x1000
  };

  KLFLibModel(KLFLibResourceEngine *resource, uint flavorFlags = LinearList|GroupSubCategories,
	      QObject *parent = NULL);
  virtual ~KLFLibModel();

  enum ItemKind { EntryKind, CategoryLabelKind };
  enum {
    ItemKindItemRole = Qt::UserRole+768, // = 800 in Qt 4.4
    EntryContentsTypeItemRole,
    EntryIdItemRole,
    FullEntryItemRole,
    CategoryLabelItemRole,
    FullCategoryPathItemRole
  };

  /** For example use
   * \code
   *  model->data(index, KLFLibModel::entryItemRole(KLFLibEntry::Latex)).toString()
   * \endcode
   * to get LaTeX string for model index \c index.
   */
  static inline int entryItemRole(int propertyId) { return (Qt::UserRole+788) + propertyId; } // = 820+propId
  /** inverse operation of \ref entryItemRole */
  static inline int entryPropIdForItemRole(int role)  { return role - (Qt::UserRole+788); } // = role - 820
  
  virtual void setResource(KLFLibResourceEngine *resource);

  virtual KLFLibResourceEngine * resource() { return pResource; }

  virtual QUrl url() const;

  /** sets the flavor flags given by \c flags. Only flags masked by \c modify_mask
   * are affected. Examples:
   * \code
   *  // Display type set to LinearList. GroupSubCategories is unchanged.
   *  m->setFlavorFlags(KLFLibModel::LinearList, KLFLibModel::DisplayTypeMask);
   *  // Set, and respectively unset the group sub-categories flag (no change to other flags)
   *  m->setFlavorFlags(KLFLibModel::GroupSubCategories, KLFLibModel::GroupSubCategories);
   *  m->setFlavorFlags(0, KLFLibModel::GroupSubCategories);
   * \endcode  */
  virtual void setFlavorFlags(uint flags, uint modify_mask = 0xffffffff);
  virtual uint flavorFlags() const;
  inline uint displayType() const { return flavorFlags() & DisplayTypeMask; }

  virtual void prefetch(const QModelIndexList& index) const;
  virtual QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const;
  virtual Qt::ItemFlags flags(const QModelIndex& index) const;
  virtual bool hasChildren(const QModelIndex &parent = QModelIndex()) const;
  virtual QVariant headerData(int section, Qt::Orientation orientation,
			      int role = Qt::DisplayRole) const;
  virtual bool hasIndex(int row, int column,
			const QModelIndex &parent = QModelIndex()) const;
  virtual QModelIndex index(int row, int column,
			    const QModelIndex &parent = QModelIndex()) const;
  virtual QModelIndex parent(const QModelIndex &index) const;
  virtual int rowCount(const QModelIndex &parent = QModelIndex()) const;
  virtual int columnCount(const QModelIndex &parent = QModelIndex()) const;

  virtual bool canFetchMore(const QModelIndex& parent) const;
  virtual void fetchMore(const QModelIndex& parent);

  virtual Qt::DropActions supportedDropActions() const;

  virtual QStringList mimeTypes() const;
  virtual QMimeData *mimeData(const QModelIndexList& indexes) const;

  virtual bool dropMimeData(const QMimeData *data, Qt::DropAction action, int row,
			    int column, const QModelIndex& parent);

  enum { DropWillAccept = 0x0001,
	 DropWillCategorize = 0x0002,
	 DropWillMove = 0x0004 };
  virtual uint dropFlags(QDragMoveEvent *event, QAbstractItemView *view);

  virtual QImage dragImage(const QModelIndexList& indexes);

  virtual int entryColumnContentsPropertyId(int column) const;
  virtual int columnForEntryPropertyId(int entryPropertyId) const;

  virtual bool isDesendantOf(const QModelIndex& child, const QModelIndex& ancestor);

  virtual QStringList categoryList() const;

  virtual void updateData(const QList<KLFLib::entryId>& entryIdList, int modifyType);

  //! Call repeatedly to walk all indexes (once each exactly, first column only)
  virtual QModelIndex walkNextIndex(const QModelIndex& cur);
  //! Call repeatedly to walk all indexes in model in reverse order
  virtual QModelIndex walkPrevIndex(const QModelIndex& cur);

  virtual KLFLib::entryId entryIdForIndex(const QModelIndex& index) const;
  virtual QModelIndex findEntryId(KLFLib::entryId eid) const;
  virtual QList<KLFLib::entryId> entryIdForIndexList(const QModelIndexList& indexlist) const;
  virtual QModelIndexList findEntryIdList(const QList<KLFLib::entryId>& eidlist) const;

  virtual int fetchBatchCount() const { return pFetchBatchCount; }


  //! notify the model that the entrySorter() settings were changed, and we need to re-sort.
  virtual void redoSort();

  //! change the entrySorter accordingly and re-sort the model.
  virtual void sort(int column, Qt::SortOrder order = Qt::AscendingOrder);

  //! The current KLFLibEntrySorter that sorts our items
  virtual KLFLibEntrySorter * entrySorter() { return pEntrySorter; }

  /** \warning The model will take ownership of the sorter and deleted in the destructor. */
  virtual void setEntrySorter(KLFLibEntrySorter *entrySorter);

signals:
  /** Announces the beginning of a long operation  (used for updates in updateData()) */
  void operationStartReportingProgress(KLFProgressReporter *progressReporter, const QString& descriptiveText);

public slots:

  virtual QModelIndex searchFind(const QString& queryString, const QModelIndex& fromIndex
				 = QModelIndex(), bool forward = true);
  virtual QModelIndex searchFindNext(bool forward);
  virtual void searchAbort();

  virtual bool changeEntries(const QModelIndexList& items, int property, const QVariant& value);
  virtual bool insertEntries(const KLFLibEntryList& entries);
  virtual bool deleteEntries(const QModelIndexList& items);

  virtual void completeRefresh();

  /** how many items to fetch at a time when fetching preview and style (non-minimalist) */
  virtual void setFetchBatchCount(int count) { pFetchBatchCount = count; }

private:

  friend class KLFLibModelCache;

  KLFLibResourceEngine *pResource;

  unsigned int pFlavorFlags;

  int pFetchBatchCount;

  mutable KLFLibModelCache *pCache;

  KLFLibEntrySorter *pEntrySorter;

  struct PersistentId {
    int kind;
    KLFLib::entryId entry_id;
    QString categorylabel_fullpath;
    int column;
  };
  QList<PersistentId> persistentIdList(const QModelIndexList& persistentindexlist);
  QModelIndexList newPersistentIndexList(const QList<PersistentId>& persistentidlist);

  void startLayoutChange(bool withQtLayoutChangedSignal = true);
  void endLayoutChange(bool withQtLayoutChangedSignal = true);

  QModelIndexList pLytChgIndexes;
  QList<PersistentId> pLytChgIds;

  void updateCacheSetupModel();

  QString pSearchString;
  QModelIndex pSearchCurNode;
  bool pSearchAborted;

  bool dropCanInternal(const QMimeData *data);
};



// -----------------


class KLF_EXPORT KLFLibViewDelegate : public QAbstractItemDelegate
{
  Q_OBJECT
public:
  /** Create a view delegate for displaying a KLFLibModel.
   * \param parent the (QObject-)parent of this object.
   */
  KLFLibViewDelegate(QObject *parent);
  virtual ~KLFLibViewDelegate();

  virtual QWidget *createEditor(QWidget *parent, const QStyleOptionViewItem& option,
				const QModelIndex& index) const;
  virtual bool editorEvent(QEvent *event,QAbstractItemModel *model, const QStyleOptionViewItem& option,
			   const QModelIndex& index);
  virtual void paint(QPainter *painter, const QStyleOptionViewItem& option, const QModelIndex& index) const;
  virtual void setEditorData(QWidget *editor, const QModelIndex& index) const;
  virtual void setModelData(QWidget *editor, QAbstractItemModel *model, const QModelIndex& index) const;
  virtual QSize sizeHint(const QStyleOptionViewItem& option, const QModelIndex& index) const;
  virtual void updateEditorGeometry(QWidget *editor, const QStyleOptionViewItem& option,
				    const QModelIndex& index) const;

  virtual void setSearchString(const QString& s) { pSearchString = s; }
  virtual void setSearchIndex(const QModelIndex& index) { pSearchIndex = index; }
  virtual void setSelectionModel(QItemSelectionModel *sm) { pSelModel = sm; }
  /** If the delegate paints items in a QTreeView, then pass a pointer to it here to display nice
   * selection markings under non-expanded tree items. Pass \c NULL to unset any previously set
   * QTreeView pointer.
   *
   * By default, the internal tree view pointer is set to NULL. */
  virtual void setTheTreeView(QTreeView *theTreeView) { pTheTreeView = theTreeView; }
  //  virtual void setIndexExpanded(const QModelIndex& index, bool isexpanded) {
  //    pExpandedIndexes[QPersistentModelIndex(index)] = isexpanded;
  //  }

  virtual bool autoBackgroundItems() const { return pAutoBackgroundItems; }
  virtual void setAutoBackgroundItems(bool autoBgItems) { pAutoBackgroundItems = autoBgItems; }
  virtual QColor autoBackgroundColor() const { return pAutoBackgroundColor; }
  virtual void setAutoBackgroundColor(const QColor& autoBgColor) { pAutoBackgroundColor = autoBgColor; }

protected:
  struct PaintPrivate {
    QPainter *p;
    QBrush background;
    const QStyleOptionViewItem *option;
    bool isselected;
    QRect innerRectText;
    QRect innerRectImage;
  };

  virtual void paintEntry(PaintPrivate *p, const QModelIndex& index) const;
  virtual void paintCategoryLabel(PaintPrivate *p, const QModelIndex& index) const;

  enum { PTF_HighlightSearch = 0x0001,
	 PTF_HighlightSearchCurrent = 0x0002,
	 PTF_SelUnderline = 0x0004,
	 PTF_ForceRichTextRender = 0x0008
  };
  virtual void paintText(PaintPrivate *p, const QString& text, uint flags = PTF_HighlightSearch) const;

  virtual bool indexHasSelectedDescendant(const QModelIndex& parent) const;
  virtual bool selectionIntersectsIndexChildren(const QItemSelection& selection,
						const QModelIndex& parent) const;
  /** implements the core of \ref indexHasSelectedDescendant. use that instead. */
  virtual bool func_indexHasSelectedDescendant(const QModelIndex& parent, const QTime& timer,
					       int timeLimitMs) const;

private:
  QString pSearchString;
  QModelIndex pSearchIndex;
  QItemSelectionModel *pSelModel;
  QTreeView *pTheTreeView; //!< warning: this is possibly NULL! see \ref setTheTreeView()

  bool pAutoBackgroundItems;
  QColor pAutoBackgroundColor;

  //  QMap<QPersistentModelIndex, bool> pExpandedIndexes;

  struct ColorRegion {
    ColorRegion(QTextCharFormat f = QTextCharFormat(), int s = -1, int l = 0)
      : fmt(f), start(s), len(l) { }
    QTextCharFormat fmt; int start; int len;
    bool operator<(const ColorRegion& other) const {
      return start < other.start;
    }
  };
  friend QDebug& operator<<(QDebug&, const ColorRegion&);
};

// -----------------

/** An implementation of the KLFAbstractLibView viwer to view library resource contents in
 * so-called Category, List or Icon view modes.
 */
class KLF_EXPORT KLFLibDefaultView : public KLFAbstractLibView
{
  Q_OBJECT
  Q_PROPERTY(bool autoBackgroundItems READ autoBackgroundItems WRITE setAutoBackgroundItems) ;
  Q_PROPERTY(QColor autoBackgroundColor READ autoBackgroundColor WRITE setAutoBackgroundColor) ;

  Q_PROPERTY(QListView::Flow iconViewFlow READ iconViewFlow WRITE setIconViewFlow) ;

public:
  enum ViewType { CategoryTreeView, ListTreeView, IconView };
  KLFLibDefaultView(QWidget *parent, ViewType viewtype = CategoryTreeView);
  virtual ~KLFLibDefaultView();

  virtual QUrl url() const;
  virtual uint compareUrlTo(const QUrl& other, uint interestFlags = 0xFFFFFFFF) const;

  bool groupSubCategories() const { return pGroupSubCategories; }

  virtual bool event(QEvent *e);
  virtual bool eventFilter(QObject *o, QEvent *e);

  virtual KLFLibEntryList selectedEntries() const;

  ViewType viewType() const { return pViewType; }

  virtual QList<QAction*> addContextMenuActions(const QPoint& pos);

  /** Returns TRUE if this view type is an IconView and that we can move icons around
   * (ie. resource property lockediconpositions FALSE and resource not locked if we
   * store the positions in the resource data) */
  virtual bool canMoveIcons() const;

  virtual QVariantMap saveGuiState() const;
  virtual bool restoreGuiState(const QVariantMap& state);

  //! The first index that is currently visible in the current scrolling position
  virtual QModelIndex currentVisibleIndex() const;

  bool autoBackgroundItems() const { return pDelegate->autoBackgroundItems(); }
  QColor autoBackgroundColor() const { return pDelegate->autoBackgroundColor(); }

  QListView::Flow iconViewFlow() const;

  virtual QStringList getCategorySuggestions();

public slots:
  virtual bool writeEntryProperty(int property, const QVariant& value);
  virtual bool deleteSelected(bool requireConfirmation = true);
  virtual bool insertEntries(const KLFLibEntryList& entries);

  virtual bool searchFind(const QString& queryString, bool forward = true);
  virtual bool searchFindNext(bool forward);
  virtual void searchAbort();

  virtual void restore(uint restoreflags = KLFLib::RestoreLatexAndStyle);

  virtual void showColumns(int propIdColumn, bool show);
  virtual void sortBy(int propIdColumn, Qt::SortOrder sortorder);

  /** Selects all children of \c parent (by default a QModelIndex(), so this function selects
   * all items). rootCall is internal and should always be set to TRUE for regular use. */
  virtual void slotSelectAll(const QModelIndex& parent = QModelIndex(), bool rootCall = true);
  virtual void slotRelayoutIcons();

  /** Inoperational. Icon positioning by user was abandoned for now. */
  virtual void slotLockIconPositions(bool locked);

  void setAutoBackgroundItems(bool on) { pDelegate->setAutoBackgroundItems(on); }
  void setAutoBackgroundColor(const QColor& c) { pDelegate->setAutoBackgroundColor(c); }

  /** Has no effect if view type is not icon view. */
  void setIconViewFlow(QListView::Flow flow);

  /** \warning This function takes effect upon the next change of resource engine, ie the next
   * call of \ref KLFAbstractLibView::setResourceEngine() */
  void setGroupSubCategories(bool yesOrNo) { pGroupSubCategories = yesOrNo; }

protected:
  virtual void updateResourceEngine();
  virtual void updateResourceProp(int propId);
  virtual void updateResourceData(const QString& subRes, int modifyType,
				  const QList<KLFLib::entryId>& entryIdList);
  virtual void updateResourceOwnData(const QList<KLFLib::entryId>& entryIdList);

  virtual void showEvent(QShowEvent *event);

protected slots:
  void slotViewSelectionChanged(const QItemSelection& selected, const QItemSelection& deselected);
  QItemSelection fixSelection(const QModelIndexList& selected);
  void slotViewItemClicked(const QModelIndex& index);
  void slotEntryDoubleClicked(const QModelIndex& index);

  //  void slotExpanded(const QModelIndex& index);
  //  void slotCollapsed(const QModelIndex& index);

  void slotShowColumnSenderAction(bool showCol);

  // called from model
  void slotResourceDataChanged(const QModelIndex& topLeft, const QModelIndex& botRight);

private:
  ViewType pViewType;
  QAbstractItemView *pView;
  KLFLibViewDelegate *pDelegate;
  KLFLibModel *pModel;

  bool pGroupSubCategories;

  QList<QAction*> pCommonActions;
  QList<QAction*> pShowColumnActions;
  QAction *pIconViewRelayoutAction;
  QAction *pIconViewLockAction;
  QList<QAction*> pIconViewActions;

  bool pEventFilterNoRecurse;

  QModelIndexList selectedEntryIndexes() const;

private slots:
  void searchFound(const QModelIndex& i);
};

// -----------------

class KLF_EXPORT KLFLibDefaultViewFactory : public KLFLibViewFactory
{
  Q_OBJECT
public:
  KLFLibDefaultViewFactory(QObject *parent = NULL);
  virtual ~KLFLibDefaultViewFactory() { }

  virtual QString viewTypeTitle(const QString& viewTypeIdent) const;

  virtual bool canCreateLibView(const QString& /*viewTypeIdent*/,
				KLFLibResourceEngine */*engine*/) { return true; }

  virtual KLFAbstractLibView * createLibView(const QString& viewTypeIdent, QWidget *parent,
					     KLFLibResourceEngine *resourceEngine);
};



// -----------------

namespace Ui {
  class KLFLibOpenResourceDlg;
  class KLFLibCreateResourceDlg;
  class KLFLibResPropEditor;
  class KLFLibNewSubResDlg;
};

/** \todo .......... TODO: ......... HANDLE sub-resources */
class KLF_EXPORT KLFLibOpenResourceDlg : public QDialog
{
  Q_OBJECT
public:
  KLFLibOpenResourceDlg(const QUrl& defaultlocation = QUrl(), QWidget *parent = 0);
  virtual ~KLFLibOpenResourceDlg();

  virtual QUrl url() const;

  static QUrl queryOpenResource(const QUrl& defaultlocation = QUrl(), QWidget *parent = 0);

protected slots:

  virtual void updateReadyToOpenFromSender(bool isready);
  virtual void updateReadyToOpen();

protected:
  virtual QUrl rawUrl() const;

private:
  Ui::KLFLibOpenResourceDlg *pUi;
  QAbstractButton *btnGo;
};

// --


class KLF_EXPORT KLFLibCreateResourceDlg : public QDialog
{
  Q_OBJECT
public:
  typedef KLFLibEngineFactory::Parameters Parameters;

  KLFLibCreateResourceDlg(const QString& defaultWtype, QWidget *parent = 0);
  virtual ~KLFLibCreateResourceDlg();

  virtual Parameters getCreateParameters() const;

  static KLFLibResourceEngine *createResource(const QString& defaultWtype, QObject *resourceParent,
					      QWidget *parent = 0);

public slots:

  virtual void accept();
  virtual void reject();

protected slots:

  virtual void updateReadyToCreateFromSender(bool isready);
  virtual void updateReadyToCreate();

private:
  Ui::KLFLibOpenResourceDlg *pUi;
  QAbstractButton *btnGo;

  Parameters pParam;
};


// --

class KLF_EXPORT KLFLibResPropEditor : public QWidget
{
  Q_OBJECT
public:
  KLFLibResPropEditor(KLFLibResourceEngine *resource, QWidget *parent = 0);
  virtual ~KLFLibResPropEditor();

public slots:
  bool apply();

protected slots:
  void slotResourcePropertyChanged(int propId);
  void slotSubResourcePropertyChanged(const QString& subResource, int propId);
  void on_btnAdvanced_toggled(bool on);
  void advPropEdited(QStandardItem *item);
  void advSubResPropEdited(QStandardItem *item);
  void on_cbxSubResource_currentIndexChanged(int newSubResItemIndex);

  void updateResourceProperties();
  void updateSubResourceProperties();
  void updateSubResources(const QString& curSubResource = QString());

private:
  KLFLibResourceEngine *pResource;
  bool pSuppSubRes;
  bool pSuppSubResProps;
  Ui::KLFLibResPropEditor *U;
  QStandardItemModel *pPropModel;
  QStandardItemModel *pSubResPropModel;

  QString curSubResource() const;
};

class KLF_EXPORT KLFLibResPropEditorDlg : public QDialog
{
  Q_OBJECT
public:
  KLFLibResPropEditorDlg(KLFLibResourceEngine *resource, QWidget *parent = 0);
  virtual ~KLFLibResPropEditorDlg();

public slots:
  void applyAndClose();
  void cancelAndClose();

private:
  KLFLibResPropEditor *pEditor;
};




class KLF_EXPORT KLFLibNewSubResDlg : public QDialog
{
  Q_OBJECT
public:
  KLFLibNewSubResDlg(KLFLibResourceEngine *resource, QWidget *parent = 0);
  virtual ~KLFLibNewSubResDlg();

  QString newSubResourceName() const;
  QString newSubResourceTitle() const;

  /** Prompt to create a sub-resource in resource \c resource. Then actually create the
   * sub-resource and return the name of the sub-resource that was created.
   *
   * Returns a null string in case of error or if the operation was canceled.
   */
  static QString createSubResourceIn(KLFLibResourceEngine *resource, QWidget *parent = 0);

private slots:
  void on_txtTitle_textChanged(const QString& text);
  void on_txtName_textChanged(const QString& text);

private:
  Ui::KLFLibNewSubResDlg *u;

  bool isAutoName;
};





/** \brief Interface for guessing file schemes
 *
 * This class provides the basic interface for customizing known local file types, and
 * guessing their corresponding schemes.
 *
 * To add a scheme guesser, just reimplement this function and create an instance of it.
 * It will register automatically.
 *
 * To query [all guessers instances] the scheme to use for a filename, use
 * \ref KLFLibBasicWidgetFactory::guessLocalFileScheme().
 */
class KLF_EXPORT KLFLibLocalFileSchemeGuesser
{
public:
  KLFLibLocalFileSchemeGuesser();
  virtual ~KLFLibLocalFileSchemeGuesser();

  //! Guess the appropriate scheme for handling the given file
  /** Reimplentations of this function must guess what scheme fileName is to be opened
   * with.
   *
   * By \a scheme we mean the URL scheme, ie. the scheme that the correct subclass of
   * \ref KLFLibEngineFactory reports being capable of opening (eg. \c "klf+sqlite").
   *
   * In reimplementations of this function, first the filename extension should be checked. If
   * it is not known, then the file can be peeked into for magic headers.
   *
   * If the scheme cannot be guessed, then the reimplementation should return an empty string.
   *
   * \note the \c fileName does not necessarily exist. (keep that in mind before reporting
   *   an error that you can't open the file to read a magic header). In that case, a
   *   simple test should be performed on the file extension.
   */
  virtual QString guessScheme(const QString& fileName) const = 0;
};



//! Provides some basic UIs to access resources
/**
 * Provides the following widget types for opening/creating/saving resources:
 *  - Local file (\c "LocalFile"). Don't forget to add new file types with
 *    \ref addLocalFileType() (this can be done e.g. in other engine factories'
 *    constructor).
 *  - planned, not yet implemented: remote DB connection with hostname/user/pass
 *    information collecting (tentative name \c "RemoteHostUserPass").
 *
 * \note Sub-resources are handled in \ref KLFLibOpenResourceDlg.
 *
 * \todo TODO: remote connections to eg. DB ..........
 */
class KLF_EXPORT KLFLibBasicWidgetFactory : public KLFLibWidgetFactory
{
  Q_OBJECT
public:
  //! A known local file type for \c KLFLibBasicWidgetFactory-created widgets
  struct LocalFileType {
    QString scheme; //!< eg. \c "klf+sqlite"
    QString filepattern; //!< eg. \c "*.klf.db"
    QString filter; //!< eg. \c "Local Library Database File (*.klf.db)"
  };

  KLFLibBasicWidgetFactory(QObject *parent = NULL);
  virtual ~KLFLibBasicWidgetFactory();

  virtual QStringList supportedTypes() const;

  virtual QString widgetTypeTitle(const QString& wtype) const;

  virtual QWidget * createPromptUrlWidget(QWidget *parent, const QString& scheme,
					  QUrl defaultlocation = QUrl());
  virtual QUrl retrieveUrlFromWidget(const QString& scheme, QWidget *widget);

  virtual bool hasCreateWidget(const QString& /*wtype*/) const { return true; }

  /** See \ref KLFLibWidgetFactory.
   *
   * Default parameters that can be given in \c defaultparameters:
   * - \c "Url" (type QUrl): the URL to start with
   */
  virtual QWidget * createPromptCreateParametersWidget(QWidget *parent, const QString& scheme,
						       const Parameters& defaultparameters = Parameters());

  /** The parameters returned by this function depends on the \c wtype.
   *
   * <b>Widget-type \c "LocalFile"</b>
   * - \c "Filename" : the selected local file name
   * - \c "klfRetry", \c "klfScheme" as documented in
   *   \ref KLFLibWidgetFactory::retrieveCreateParametersFromWidget().
   */
  virtual Parameters retrieveCreateParametersFromWidget(const QString& wtype, QWidget *widget);


  /** This function should be called for example in KLFLibEngineFactory subclasses' constructor
   * to inform this widget factory of local file types that are known by the various engine
   * factories. This is then eg. used to provide a useful filter choice in file dialogs.
   */
  static void addLocalFileType(const LocalFileType& fileType);
  static QList<LocalFileType> localFileTypes();

  /** Queries all the instantiated KLFLibLocalFileSchemeGuesser objects to see if one can recognize
   * the file \c fileName. The first scheme match found is returned. An empty QString is returned
   * if no guesser succeeded to recognize \c fileName.
   */
  static QString guessLocalFileScheme(const QString& fileName);

protected:
  static QList<LocalFileType> pLocalFileTypes;
  static QList<KLFLibLocalFileSchemeGuesser*> pSchemeGuessers;

  friend class KLFLibLocalFileSchemeGuesser;

  /** This function adds a scheme guesser, ie. a functional sub-class of
   * \ref KLFLibLocalFileSchemeGuesser. The instance is NOT deleted after use.
   * \c schemeguesser could for example also sub-class QObject and set \c qApp as parent.
   */
  static void addLocalFileSchemeGuesser(KLFLibLocalFileSchemeGuesser *schemeguesser);

  static void removeLocalFileSchemeGuesser(KLFLibLocalFileSchemeGuesser *schemeguesser);

};





#endif
