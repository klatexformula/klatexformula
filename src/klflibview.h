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

#include <klfdefs.h>
#include <klflib.h>

namespace KLFLib {
  enum RestoreMode {
    RestoreLatex = 0x0001,
    RestoreStyle = 0x0002,

    RestoreLatexAndStyle = RestoreLatex|RestoreStyle
  };
};

//! A view widget to display a library resource's contents
/** A base API for a widget that will display a KLFLibResourceEngine's contents.
 * For example one could create a QTreeView and display the contents in there
 * (that's what KLFLibDefaultView does...).
 *
 * \note Subclasses should emit the \ref resourceDataChanged() signal AFTER they
 *   refresh after a resource data change.
 */
class KLF_EXPORT KLFAbstractLibView : public QWidget
{
  Q_OBJECT
public:
  KLFAbstractLibView(QWidget *parent);
  virtual ~KLFAbstractLibView() { }

  virtual KLFLibResourceEngine * resourceEngine() const { return pResourceEngine; }

  virtual void setResourceEngine(KLFLibResourceEngine *resource);

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

  /** Restores the state described in \c state (which was previously returned
   * by \ref saveGuiState()) */
  virtual bool restoreGuiState(const QVariantMap& state) = 0;

signals:
  void requestRestore(const KLFLibEntry& entry, uint restoreflags = KLFLib::RestoreLatexAndStyle);
  void requestRestoreStyle(const KLFStyle& style);

  /** Subclasses must emit this signal AFTER they have refreshed. */
  void resourceDataChanged(const QList<KLFLib::entryId>& entryIdList);

  void entriesSelected(const KLFLibEntryList& entries);
  /** Subclasses should emit this signal with lists of categories they come accross,
   * so that the editor can suggest these as completions upon editing category */
  void moreCategorySuggestions(const QStringList& categorylist);

public slots:
  virtual void updateView();
  virtual void updateResourceView() = 0;
  virtual void updateResourceProp(int propId) = 0;
  virtual void updateResourceData(const QList<KLFLib::entryId>& entryIdList) = 0;
  virtual bool writeEntryProperty(int property, const QVariant& value) = 0;
  inline bool writeEntryCategory(const QString& category)
  { return writeEntryProperty(KLFLibEntry::Category, category); }
  inline bool writeEntryTags(const QString& tags)
  { return writeEntryProperty(KLFLibEntry::Tags, tags); }
  virtual bool deleteSelected(bool requireConfirm = true) = 0;
  virtual bool insertEntries(const KLFLibEntryList& entries) = 0;
  inline bool insertEntry(const KLFLibEntry& entry)
    /* */ { return insertEntries(KLFLibEntryList() << entry); }

  virtual bool searchFind(const QString& queryString, bool forward = true) = 0;
  virtual bool searchFindNext(bool forward) = 0;
  virtual void searchAbort() = 0;

  virtual void restore(uint restoreFlags = KLFLib::RestoreLatexAndStyle) = 0;
  void restoreWithStyle() { restore(KLFLib::RestoreLatexAndStyle); }
  void restoreLatexOnly() { restore(KLFLib::RestoreLatex); }

  /** Called by the owner of the view. This function fetches category suggestions
   * (by calling the virtual getCategorySuggestions() reimplemented by subclasses)
   * and emits the signal moreCategorySuggestions().
   *
   * Subclasses need not reimplement this function. */
  virtual void wantMoreCategorySuggestions();

protected:
  virtual QStringList getCategorySuggestions() = 0;

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
    ItemKindItemRole = Qt::UserRole+800,
    EntryContentsTypeItemRole,
    EntryIdItemRole,
    FullEntryItemRole,
    CategoryLabelItemRole,
    FullCategoryPathItemRole,
  };

  /** For example use
   * \code
   *  model->data(index, KLFLibModel::entryItemRole(KLFLibEntry::Latex)).toString()
   * \endcode
   * to get LaTeX string for model index \c index.
   */
  static inline int entryItemRole(int propertyId) { return Qt::UserRole+810+propertyId; }
  
  virtual void setResource(KLFLibResourceEngine *resource);

  virtual KLFLibResourceEngine * resource() { return pResource; }

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

  virtual QVariant data(const QModelIndex& index, int role) const;
  virtual Qt::ItemFlags flags(const QModelIndex& index) const;
  virtual bool hasChildren(const QModelIndex &parent) const;
  virtual QVariant headerData(int section, Qt::Orientation orientation,
			      int role = Qt::DisplayRole) const;
  virtual bool hasIndex(int row, int column,
			const QModelIndex &parent = QModelIndex()) const;
  virtual QModelIndex index(int row, int column,
			    const QModelIndex &parent = QModelIndex()) const;
  virtual QModelIndex parent(const QModelIndex &index) const;
  virtual int rowCount(const QModelIndex &parent = QModelIndex()) const;
  virtual int columnCount(const QModelIndex &parent = QModelIndex()) const;

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

  virtual void sort(int column, Qt::SortOrder order = Qt::AscendingOrder);

  virtual int entryColumnContentsPropertyId(int column) const;
  virtual int columnForEntryPropertyId(int entryPropertyId) const;

  virtual bool isDesendantOf(const QModelIndex& child, const QModelIndex& ancestor);

  virtual QStringList categoryList() const;

  virtual void updateData(const QList<KLFLib::entryId>& entryIdList);

  //! Call repeatedly to walk all indexes (once each exactly, first column only)
  virtual QModelIndex walkNextIndex(const QModelIndex& cur);
  //! Call repeatedly to walk all indexes in model in reverse order
  virtual QModelIndex walkPrevIndex(const QModelIndex& cur);

  virtual KLFLib::entryId entryIdForIndex(const QModelIndex& index) const;
  virtual QModelIndex findEntryId(KLFLib::entryId eid) const;

public slots:

  virtual QModelIndex searchFind(const QString& queryString, const QModelIndex& fromIndex
				 = QModelIndex(), bool forward = true);
  virtual QModelIndex searchFindNext(bool forward);

  virtual bool changeEntries(const QModelIndexList& items, int property, const QVariant& value);
  virtual bool insertEntries(const KLFLibEntryList& entries);
  virtual bool deleteEntries(const QModelIndexList& items);

private:

  KLFLibResourceEngine *pResource;

  unsigned int pFlavorFlags;

  typedef qint32 IndexType;
  /** UIDType standing for \a UniversalIdType: on 32 bits:
   * <pre>
   *   mask shift for Node Kind:  30 bits
   *   mask for Node Kind:  0x c0000000
   *   mask for Node Index: 0x 3FFFFFFF
   * </pre>
   */
  typedef quint32 UIDType;
  static const quint8 UIDKindShift  = 30;
  static const UIDType UIDKindMask  = 0xC0000000;
  static const UIDType UIDIndexMask = 0x3FFFFFFF;
  static const UIDType UIDInvalid   = 0xFFFFFFFF;

  struct NodeId {
    NodeId(ItemKind k = EntryKind, IndexType i = -1) : kind(k), index(i)  { }
    bool valid() const { return index >= 0; }
    bool isRoot() const { return kind == CategoryLabelKind && index == 0; }
    ItemKind kind;
    IndexType index;
    bool operator==(const NodeId& other) const {
      return kind == other.kind && index == other.index;
    }
    bool operator!=(const NodeId& other) const {
      return !(operator==(other));
    }
    UIDType universalId() const {
      if ( index < 0 ) // invalid index
	return (~0x0); // 0xFFFFFFFF
      UIDType uid = (UIDType)index;
      uid |= (kind << UIDKindShift);
      return uid;
    }
    static NodeId fromUID(UIDType uid) {
      return NodeId((ItemKind)((uid&UIDKindMask)>>UIDKindShift), uid&UIDIndexMask);
    }
    static NodeId rootNode() { return NodeId(CategoryLabelKind, 0); }
  };
  struct PersistentId {
    ItemKind kind;
    KLFLib::entryId entry_id;
    QString categorylabel_fullpath;
    int column;
  };
  struct Node {
    Node(ItemKind k) : kind(k), parent(NodeId()), children(QList<NodeId>()) { }
    Node(const Node& other) : kind(other.kind), parent(other.parent), children(other.children) { }
    virtual ~Node() { }
    ItemKind kind;
    NodeId parent;
    QList<NodeId> children;
  };
  struct EntryNode : public Node {
    EntryNode() : Node(EntryKind), entryid(-1), entry() { }
    KLFLib::entryId entryid;
    KLFLibEntry entry;
  };
  struct CategoryLabelNode : public Node {
    CategoryLabelNode() : Node(CategoryLabelKind), categoryLabel(), fullCategoryPath()  { }
    //! The last element in \ref fullCategoryPath eg. "General Relativity"
    QString categoryLabel;
    //! The full category path of this category eg. "Physics/General Relativity"
    QString fullCategoryPath;
  };
  typedef QList<EntryNode> EntryCache;
  typedef QList<CategoryLabelNode> CategoryLabelCache;

  friend QDebug& operator<<(QDebug& dbg, const NodeId& en);
  friend QDebug& operator<<(QDebug& dbg, const EntryNode& en);
  friend QDebug& operator<<(QDebug& dbg, const CategoryLabelNode& cn);

  EntryCache pEntryCache;
  CategoryLabelCache pCategoryLabelCache;

  QStringList pCatListCache;

  /** If row is negative, it will be looked up automatically. */
  QModelIndex createIndexFromId(NodeId nodeid, int row, int column) const;
  //  /** If row is negative, it will be looked up automatically. */
  //  QModelIndex createIndexFromPtr( *node, int row, int column) const;
  /** Returns an invalid ID upon invalid index. */
  NodeId getNodeForIndex(const QModelIndex& index) const;
  Node getNode(NodeId nodeid) const;
  EntryNode getEntryNode(NodeId nodeid) const;
  CategoryLabelNode getCategoryLabelNode(NodeId nodeid) const;
  //  NodeId getNodeId(Node *node) const;
  /** get the row of \c nodeid in its parent.  */
  int getNodeRow(NodeId nodeid) const;
  //  int getNodeRow(Node * node) const; 

  QList<PersistentId> persistentIdList(const QModelIndexList& persistentindexlist);
  QModelIndexList newPersistentIndexList(const QList<PersistentId>& persistentidlist);

  void updateCacheSetupModel();

  IndexType cacheFindCategoryLabel(QStringList catelements, bool createIfNotExists = false);

  void dumpNodeTree(NodeId node, int indent = 0) const;

  class KLF_EXPORT KLFLibModelSorter
  {
  public:
    KLFLibModelSorter(KLFLibModel *m, int entry_prop, Qt::SortOrder sort_order, bool groupcategories)
      : model(m), entryProp(entry_prop), sortOrderFactor( (sort_order == Qt::AscendingOrder) ? -1 : 1),
	groupCategories(groupcategories) { }
    bool operator()(const NodeId& a, const NodeId& b);
  private:
    KLFLibModel *model;
    int entryProp;
    int sortOrderFactor;
    bool groupCategories;
  };
  QString nodeValue(NodeId node, int entryProperty);
  /** Sort a category's children */
  void sortCategory(NodeId category, int column, Qt::SortOrder order);

  int lastSortColumn;
  Qt::SortOrder lastSortOrder;

  /** Walks the whole tree returning all the nodes one after the other, in the following order:
   * if \c n has children, first child is returned; otherwise next sibling is returned.
   *
   * This function returns all nodes in the order they would be displayed in a tree view.
   *
   * Returns \c NULL after last node. Returns first node in tree if \c NULL is given as
   * paremeter. */
  NodeId nextNode(NodeId n);
  /** Same as \ref nextNode() but the walk is performed in the opposite direction.
   *
   * This function returns all nodes in the inverse order they would be displayed in a tree view. In
   * particular, it returns a parent node after having explored its children. */
  NodeId prevNode(NodeId n);
  /** Returns the last node in tree defined by node \c n.
   *
   * If \c n has children, returns last child of the last child of the last child etc. If \c n does
   * not have children, it is itself returned.
   *
   * If \c NULL is given, the root node is assumed. */
  NodeId lastNode(NodeId n);

  QList<KLFLib::entryId> entryIdList(const QModelIndexList& indexlist) const;
  
  QString pSearchString;
  NodeId pSearchCurNode;

  bool dropCanInternal(const QMimeData *data);
};



// -----------------


class KLF_EXPORT KLFLibViewDelegate : public QAbstractItemDelegate
{
  Q_OBJECT
public:
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
  virtual void setIndexExpanded(const QModelIndex& index, bool isexpanded) {
    pExpandedIndexes[QPersistentModelIndex(index)] = isexpanded;
  }

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

  virtual bool indexHasSelectedDescendant(const QModelIndex& index) const;

private:
  QString pSearchString;
  QModelIndex pSearchIndex;
  QItemSelectionModel *pSelModel;

  QMap<QPersistentModelIndex, bool> pExpandedIndexes;

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

class KLF_EXPORT KLFLibDefaultView : public KLFAbstractLibView
{
  Q_OBJECT
public:
  enum ViewType { CategoryTreeView, ListTreeView, IconView };
  KLFLibDefaultView(QWidget *parent, ViewType viewtype = CategoryTreeView);
  virtual ~KLFLibDefaultView();

  virtual bool event(QEvent *e);
  virtual bool eventFilter(QObject *o, QEvent *e);

  virtual KLFLibEntryList selectedEntries() const;

  virtual ViewType viewType() const { return pViewType; }

  virtual QList<QAction*> addContextMenuActions(const QPoint& pos);

  /** Returns TRUE if this view type is an IconView and that we can move icons around
   * (ie. resource property lockediconpositions FALSE and resource not locked if we
   * store the positions in the resource data) */
  virtual bool canMoveIcons() const;

  virtual QVariantMap saveGuiState() const;
  virtual bool restoreGuiState(const QVariantMap& state);

  //! The first index that is currently visible in the current scrolling position
  virtual QModelIndex currentVisibleIndex() const;

public slots:
  virtual bool writeEntryProperty(int property, const QVariant& value);
  virtual bool deleteSelected(bool requireConfirmation = true);
  virtual bool insertEntries(const KLFLibEntryList& entries);

  virtual bool searchFind(const QString& queryString, bool forward = true);
  virtual bool searchFindNext(bool forward);
  virtual void searchAbort();

  virtual void restore(uint restoreflags = KLFLib::RestoreLatexAndStyle);

  virtual void slotSelectAll(const QModelIndex& parent = QModelIndex());
  virtual void slotRelayoutIcons();
  virtual void slotLockIconPositions(bool locked);

protected:
  virtual void updateResourceView();
  virtual void updateResourceProp(int propId);
  virtual void updateResourceData(const QList<KLFLib::entryId>& entryIdList);
  virtual void updateResourceOwnData(const QList<KLFLib::entryId>& entryIdList);
  virtual QStringList getCategorySuggestions();

protected slots:
  void slotViewSelectionChanged(const QItemSelection& selected, const QItemSelection& deselected);
  QItemSelection fixSelection(const QModelIndexList& selected);
  void slotViewItemClicked(const QModelIndex& index);
  void slotEntryDoubleClicked(const QModelIndex& index);

  void slotExpanded(const QModelIndex& index);
  void slotCollapsed(const QModelIndex& index);

  void slotShowColumnSenderAction(bool showCol);

  // called from model
  void slotResourceDataChanged(const QModelIndex& topLeft, const QModelIndex& botRight);

private:
  ViewType pViewType;
  QAbstractItemView *pView;
  //  QListView *pListView;
  //  QTreeView *pTreeView;
  KLFLibViewDelegate *pDelegate;
  KLFLibModel *pModel;

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
};

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

  KLFLibCreateResourceDlg(QWidget *parent = 0);
  virtual ~KLFLibCreateResourceDlg();

  /** An additional parameter <tt>p["klfScheme"] = QString(...scheme...)</tt> is
   * set in the return value to reflect the chosen scheme. */
  virtual Parameters getCreateParameters() const;

  static KLFLibResourceEngine *createResource(QObject *resourceParent, QWidget *parent = 0);

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

class KLFLibResPropEditor : public QWidget
{
  Q_OBJECT
public:
  KLFLibResPropEditor(KLFLibResourceEngine *resource, QWidget *parent = 0);
  virtual ~KLFLibResPropEditor();

public slots:
  bool apply();

protected slots:
  void slotResourcePropertyChanged(int propId);
  void on_btnAdvanced_toggled(bool on);
  void advPropEdited(QStandardItem *item);

private:
  KLFLibResourceEngine *pResource;
  Ui::KLFLibResPropEditor *U;
  QStandardItemModel *pPropModel;
};

class KLFLibResPropEditorDlg : public QDialog
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


#endif
