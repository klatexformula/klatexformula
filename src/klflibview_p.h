/***************************************************************************
 *   file klflibview_p.h
 *   This file is part of the KLatexFormula Project.
 *   Copyright (C) 2011 by Philippe Faist
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


/** \file
 * This header contains (in principle private) auxiliary classes for
 * library routines defined in klflibview.cpp */


#ifndef KLFLIBVIEW_P_H
#define KLFLIBVIEW_P_H

#include <math.h> // abs()

#include <QApplication>
#include <QStringList>
#include <QAbstractItemView>
#include <QTreeView>
#include <QListView>
#include <QMimeData>
#include <QDrag>
#include <QDragEnterEvent>
#include <QDragMoveEvent>
#include <QDropEvent>
#include <QWidget>
#include <QFileInfo>
#include <QFileDialog>
#include <QDir>
#include <QDesktopServices>
#include <QDirModel>
#include <QCompleter>

#include <klfiteratorsearchable.h>

#include "klflib.h"
#include "klflibview.h"
#include "klfconfig.h"

#include <ui_klfliblocalfilewidget.h>

class KLFLibDefTreeView;


/** \internal */
inline QPointF sizeToPointF(const QSizeF& s) { return QPointF(s.width(), s.height()); }
/** \internal */
inline QSizeF pointToSizeF(const QPointF& p) { return QSizeF(p.x(), p.y()); }


// -----------------------------


/** \internal */
class KLFLibModelCache
{
public:
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

  enum ItemKind {
    EntryKind = KLFLibModel::EntryKind,
    CategoryLabelKind = KLFLibModel::CategoryLabelKind
  };

  struct NodeId {
    NodeId(ItemKind k = ItemKind(EntryKind), IndexType i = -1) : kind(k), index(i)  { }
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
  struct Node {
    Node() : allocated(false) { }
    Node(ItemKind k) : allocated(true), kind(k), parent(NodeId()), children(QList<NodeId>()),
		       allChildrenFetched(false)  { }
    Node(const Node& other) : allocated(other.allocated), kind(other.kind), parent(other.parent),
			      children(other.children), allChildrenFetched(other.allChildrenFetched)  { }
    virtual ~Node() { }
    //! Whether this node is used. when false, this Node object is just unused memory space in the cache list
    bool allocated;
    ItemKind kind;
    NodeId parent;
    QList<NodeId> children;
    /** \brief TRUE if all the children of this node have been fetched and stored into \c children,
     * FALSE if possibly there may be more children, we may need to query the resource. */
    bool allChildrenFetched;
  };
  struct EntryNode : public Node {
    EntryNode() : Node(EntryKind), entryid(-1), minimalist(false), entry()
    {
      allChildrenFetched = true; // no children to fetch
    }
    EntryNode(const EntryNode& copy)
      : Node(copy), entryid(copy.entryid), minimalist(copy.minimalist), entry(copy.entry) { }

    inline bool entryIsValid() const  { return allocated && parent != NodeId() && entryid >= 0; }

    KLFLib::entryId entryid;
    /** if TRUE, 'entry' only holds category/tags/datetime/latex/previewsize, no pixmap, no style. */
    bool minimalist;
    KLFLibEntry entry;
  };
  struct CategoryLabelNode : public Node {
    CategoryLabelNode() : Node(CategoryLabelKind), categoryLabel(), fullCategoryPath()  { }
    CategoryLabelNode(const CategoryLabelNode& copy)
      : Node(copy), categoryLabel(copy.categoryLabel), fullCategoryPath(copy.fullCategoryPath) { }
    //! The last element in \ref fullCategoryPath eg. "General Relativity"
    QString categoryLabel;
    //! The full category path of this category eg. "Physics/General Relativity"
    QString fullCategoryPath;
  };

  template<class N>
  class NodeCache : public QList<N> {
  public:
    NodeCache() : QList<N>(), pContainsNonAllocated(false) { }

    inline bool isAllocated(IndexType i) { return QList<N>::at(i).allocated; }

    IndexType insertNewNode(const N& n) {
      IndexType insertPos = QList<N>::size();
      if (pContainsNonAllocated) {
	// walk the cache, and find an invalid node
	for (insertPos = 0; insertPos < QList<N>::size() && QList<N>::at(insertPos).allocated; ++insertPos)
	  ;
      }
      if (insertPos == QList<N>::size()) {
	pContainsNonAllocated = false;
	this->append(n);
	return insertPos;
      }
      QList<N>::operator[](insertPos) = n;
      return insertPos;
    }

    /** \warning: you must check manually before calling this function that \c nid is right kind! */
    inline void unlinkNode(const NodeId& nid) { unlinkNode(nid.index); }
    void unlinkNode(IndexType index) {
      N& node = QList<N>::operator[](index);
      node.allocated = false; // render invalid
      pContainsNonAllocated = true;
    }

    /** \warning: you must check manually before calling this function that \c nid is right kind! */
    inline N takeNode(const NodeId& nid) { return takeNode(nid.index); }
    N takeNode(IndexType index) {
      if (index < 0 || index >= QList<N>::size()) {
	qWarning()<<KLF_FUNC_NAME<<": invalid index="<<index;
	return N();
      }
      N node = QList<N>::at(index);
      unlinkNode(index);
      return node;
    }
  private:
    bool pContainsNonAllocated;
  };

  typedef NodeCache<EntryNode> EntryCache;
  typedef NodeCache<CategoryLabelNode> CategoryLabelCache;

  EntryNode pInvalidEntryNode; // not initialized, not used except for return values
  //                              of functions aborting their call eg. getNodeRef()


  KLFLibModelCache(KLFLibModel * model)
    : pModel(model)
  {
    pIsFetchingMore = false;

    pLastSortPropId = KLFLibEntry::DateTime;
    pLastSortOrder = Qt::DescendingOrder;
  }

  virtual ~KLFLibModelCache() { }

  KLFLibModel *pModel;

  void rebuildCache();

  /** If row is negative, it will be looked up automatically.
   */
  QModelIndex createIndexFromId(NodeId nodeid, int row, int column);

  /** Returns an invalid ID upon invalid index. */
  NodeId getNodeForIndex(const QModelIndex& index);
  /** Returns empty node for invalid indexes, not root nodem. */
  Node getNode(NodeId nodeid);
  Node& getNodeRef(NodeId nodeid);
  EntryNode& getEntryNodeRef(NodeId nodeid, bool requireNotMinimalist = false);
  CategoryLabelNode& getCategoryLabelNodeRef(NodeId nodeid);

  /** get the row of \c nodeid in its parent.  */
  int getNodeRow(NodeId nodeid);

  /** define here what prop ids are stored in minimalist entries.
   * Warning: some functions may make some assumptions on what minimalist entries have, and these
   * properties must make sense here (eg. PreviewSize for delegate's sizeHint(), Category for
   * creating the tree, etc.) */
  static inline QList<int> minimalistEntryPropIds() {
    return QList<int>() << KLFLibEntry::Category << KLFLibEntry::Tags
			<< KLFLibEntry::DateTime << KLFLibEntry::Latex
			<< KLFLibEntry::PreviewSize;
  }

  /** Updates \c count entry nodes in tree after (and including \c nodeId), if they
   * are marked as "minimalist" (see \ref EntryNode)
   *
   * If count is -1, uses pModel->fetchBatchCount(). */
  void ensureNotMinimalist(NodeId nodeId, int count = -1);

  bool canFetchMore(NodeId parentId);
  void fetchMore(NodeId parentId, int batchCount = -1);

  void updateData(const QList<KLFLib::entryId>& entryIdList, int modifyType);

  /**
   * if \c isRebuildingCache is set, then items are just appended to the category childs (as they
   * are inserted in the right order), and calls to cacheFindCategoryLabel will set
   * \c newlyCreatedAllChildrenFetched parameter to FALSE.
   *
   * emits QAbstractItemModel-appropriate beginInsertRows()/endInsertRows() if \c isRebuildingCache is
   * FALSE. Those signals are also emitted (if \c isRebuildingCache is false) when category labels are
   * created to fit the node.
   *
   * The entry-node \c e must not be yet in the entry cache.
   */
  void treeInsertEntry(const EntryNode& e, bool isRebuildingCache = false);

  /** emits QAbstractItemModel-appropriate signals and updates indexes if \c notifyQtApi is true
   *
   * This function sets the entryId of the removed entry to -1 so that it cannot be re-found in a
   * future search. */
  EntryNode treeTakeEntry(const NodeId& e, bool notifyQtApi = true);

  /** emits QAbstractItemModel-appropriate signals and updates indexes if \c notifyQtApi is true.
   *
   * If \c newlyCreatedAreChildrenFetched is TRUE, then any newly created CategoryLabelNode will have its
   * \c allChildrenFetched flag set to TRUE (eg. when updating data, a new entry was changed category to
   * a yet-inexistant category). If the parameter is FALSE, then any newly created CategoryLabelNode will
   * have its \c allChildrenFetched flag set to FALSE (eg. when rebuilding the cache).
   */
  IndexType cacheFindCategoryLabel(QStringList catelements, bool createIfNotExists = false,
				   bool notifyQtApi = false, bool newlyCreatedAreChildrenFetched = true);

  class KLF_EXPORT KLFLibModelSorter
  {
  public:
    KLFLibModelSorter(KLFLibModelCache *c, KLFLibEntrySorter *es, bool groupcategories)
      : cache(c), entrysorter(es), groupCategories(groupcategories)
    {
    }
    /** Returns TRUE if a<b, FALSE otherwise. */
    bool operator()(const NodeId& a, const NodeId& b);

    KLFLibEntrySorter *entrySorter() { return entrysorter; }

  private:
    KLFLibModelCache *cache;
    KLFLibEntrySorter *entrysorter;
    bool groupCategories;
  };

  /** If node is a category label, then \c propId is ignored. */
  QString nodeValue(NodeId node, int propId = KLFLibEntry::Latex);

  /** returns TRUE if the node \c nodeId matches the search query defined by \c searchString and
   * case-sensitivity \c cs. */
  bool searchNodeMatches(const NodeId& nodeId, const QString& searchString, Qt::CaseSensitivity cs);

  /** Remembers the given sort parameters, but does NOT update anything. */
  void setSortingBy(int propId, Qt::SortOrder order)
  {
    pLastSortPropId = propId;
    pLastSortOrder = order;
  }

  /** Sort a category's children -- DON'T USE---OBSOLETE, does not handle fetch-mores */
  void sortCategory(NodeId category, KLFLibModelSorter *sorter, bool rootCall = true);

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

  /** Returns a list of all valid entryIds in the index list. ORDER of result is not garanteed and invalid
   * indexes are ignored, as opposed to entryIdForIndexList().  */
  QList<KLFLib::entryId> entryIdList(const QModelIndexList& indexlist);

  /** Returns a list of same size as \c indexlist in which each entry is exactly the entryId of the
   * corresponding index in \c indexlist, setting -1 if it isn't an entry index. */
  QList<KLFLib::entryId> entryIdForIndexList(const QModelIndexList& indexlist);
  QModelIndexList findEntryIdList(const QList<KLFLib::entryId>& eidlist);

  NodeId findEntryId(KLFLib::entryId eId);

  QStringList categoryListCache() { return pCatListCache; }

  void fullDump();
  void dumpNodeTree(NodeId node, int indent = 0);

private:
  EntryCache pEntryCache;
  CategoryLabelCache pCategoryLabelCache;

  QStringList pCatListCache;

  /** remember this category in the category list cache (that is useful only to
   * suggest known categories to user at given occasions) */
  void insertCategoryStringInSuggestionCache(const QString& category)
  {
    insertCategoryStringInSuggestionCache(category.split('/', QString::SkipEmptyParts));
  }
  /** \note catelements should be obtained with
   * <tt>categorystring.split('/', QString::SkipEmptyParts)</tt> */
  void insertCategoryStringInSuggestionCache(const QStringList& catelements)
  {
    // walk decrementally, and break once we fall back in the list of known categories
    for (int kl = catelements.size()-1; kl >= 0; --kl) {
      QString c = QStringList(catelements.mid(0,kl+1)).join("/");
      if (pCatListCache.contains(c))
	break;
      pCatListCache.insert(qLowerBound(pCatListCache.begin(), pCatListCache.end(), c), c);
    }
  }

  bool pIsFetchingMore;

  int pLastSortPropId;
  Qt::SortOrder pLastSortOrder;

  KLF_DEBUG_DECLARE_ASSIGNABLE_REF_INSTANCE() ;
};


// -----------------------------------------


class KLFLibViewSearchable : public KLFIteratorSearchable<QModelIndex>
{
  KLFLibDefaultView *v;
  inline KLFLibModel *m() { return v->pModel; }

public:
  KLFLibViewSearchable(KLFLibDefaultView *view)
    : v(view)
  {
  }

  SearchIterator searchIterBegin()
  {
    KLF_DEBUG_BLOCK(KLF_FUNC_NAME) ;
    KLF_ASSERT_NOT_NULL(m(), "Model is NULL!", return QModelIndex() ) ;
    return m()->walkNextIndex(QModelIndex());
  }
  SearchIterator searchIterEnd()
  {
    KLF_DEBUG_BLOCK(KLF_FUNC_NAME) ;
    return QModelIndex();
  }
  SearchIterator searchIterAdvance(const SearchIterator& pos, bool forward)
  {
    KLF_DEBUG_BLOCK(KLF_FUNC_NAME) ;
    KLF_ASSERT_NOT_NULL(m(), "Model is NULL!", return QModelIndex() ) ;
    return forward ? m()->walkNextIndex(pos) : m()->walkPrevIndex(pos);
  }

  virtual SearchIterator searchIterStartFrom(bool /*forward*/)
  {
    KLF_DEBUG_BLOCK(KLF_FUNC_NAME) ;
    KLF_ASSERT_NOT_NULL(m(), "Model is NULL!", return QModelIndex() ) ;

    return v->currentVisibleIndex(true); // always first visible index
  }

  virtual bool searchIterMatches(const SearchIterator& pos, const QString& queryString)
  {
    KLF_DEBUG_BLOCK(KLF_FUNC_NAME) ;
    KLF_ASSERT_NOT_NULL(m(), "Model is NULL!", return false ) ;
    KLF_ASSERT_NOT_NULL(m()->pCache, "Model Cache is NULL!", return false ) ;

    Qt::CaseSensitivity cs = Qt::CaseInsensitive;
    if (queryString.contains(QRegExp("[A-Z]")))
      cs = Qt::CaseSensitive;
    KLFLibModelCache::NodeId n = m()->pCache->getNodeForIndex(pos);
    return KLF_DEBUG_TEE( m()->pCache->searchNodeMatches(n, queryString, cs) ) ;
  }

  virtual void searchMoveToIterPos(const SearchIterator& pos)
  {
    KLF_DEBUG_BLOCK(KLF_FUNC_NAME) ;
    v->pDelegate->setSearchIndex(pos);
    if ( ! pos.isValid() ) {
      //      v->pView->scrollToTop();
      //      // unselect all
      //      v->pView->selectionModel()->setCurrentIndex(QModelIndex(), QItemSelectionModel::Clear);
      return;
    } else {
      v->pView->scrollTo(pos, QAbstractItemView::EnsureVisible);
    }
    if (v->pViewType == KLFLibDefaultView::CategoryTreeView) {
      // if tree view, expand item
      qobject_cast<QTreeView*>(v->pView)->expand(pos);
    }
    // select the item
    v->pView->selectionModel()->setCurrentIndex(pos,
						QItemSelectionModel::ClearAndSelect|
						QItemSelectionModel::Rows);
    v->updateDisplay();
  }

  virtual void searchPerformed(const SearchIterator& resultMatchPosition, bool found, const QString& queryString)
  {
    KLF_DEBUG_BLOCK(KLF_FUNC_NAME) ;
    Q_UNUSED(resultMatchPosition) ;
    Q_UNUSED(found) ;
    v->pDelegate->setSearchString(queryString);
  }
  virtual void searchAborted()
  {
    KLF_DEBUG_BLOCK(KLF_FUNC_NAME) ;
    v->pDelegate->setSearchString(QString());
    v->pDelegate->setSearchIndex(QModelIndex());
    v->updateDisplay(); // repaint widget to update search underline
    KLFIteratorSearchable<QModelIndex>::searchAborted();
  }

  virtual void searchReinitialized()
  {
    KLF_DEBUG_BLOCK(KLF_FUNC_NAME) ;
    v->pDelegate->setSearchString(QString());
    v->pDelegate->setSearchIndex(QModelIndex());
    v->updateDisplay();
    KLFIteratorSearchable<QModelIndex>::searchReinitialized();
  }


  KLF_DEBUG_DECLARE_REF_INSTANCE("libview-searchable") ;
};






// -----------------------------------------


/** \internal */
class KLFLibDefViewCommon
{
public:
  KLFLibDefViewCommon(KLFLibDefaultView *dview)
    : pDView(dview), pViewType(dview->viewType())
  {
  }
  virtual ~KLFLibDefViewCommon() { }

  //  virtual void moveSelectedIconsBy(const QPoint& delta) = 0;

  /** \warning Caller eventFilter() must ensure not to recurse with fake events ! */  
  virtual bool evDragEnter(QDragEnterEvent *de, const QPoint& pos) {
    uint fl = pModel->dropFlags(de, thisView());
    klfDbg( "KLFLibDefViewCommon::evDragEnter: drop flags are "<<fl<<"; this viewtype="<<pViewType ) ;
    // decide whether to show drop indicator or not.
    bool showdropindic = (fl & KLFLibModel::DropWillCategorize);
    thisView()->setDropIndicatorShown(showdropindic);
    // decide whether to accept the drop or to ignore it
    if ( !(fl & KLFLibModel::DropWillAccept) && pViewType != KLFLibDefaultView::IconView ) {
      // pViewType != IconView  <=  don't ignore if in icon view (where user can
      // move the equations freely...by drag&drop)
      de->ignore();
      qDebug("Ignored drag enter event.");
    } else {
      mousePressedContentsPos = pos;
      de->accept();
      klfDbg( "Accepted drag enter event issued at pos="<<pos ) ;
      // and FAKE a QDragMoveEvent to the item view.
      QDragEnterEvent fakeevent(de->pos(), de->dropAction(), de->mimeData(), de->mouseButtons(),
				de->keyboardModifiers());
      qApp->sendEvent(thisView()->viewport(), &fakeevent);
    }
    return true;
  }

  /** \warning Caller eventFilter() must ensure not to recurse with fake events ! */  
  virtual bool evDragMove(QDragMoveEvent *de, const QPoint& pos) {
    Q_UNUSED(pos) ;
    uint fl = pModel->dropFlags(de, thisView());
    klfDbg( "KLFLibDefViewCommon::evDragMove: flags are "<<fl<<"; pos is "<<pos ) ;
    // decide whether to accept the drop or to ignore it
    if ( !(fl & KLFLibModel::DropWillAccept) && pViewType != KLFLibDefaultView::IconView ) {
      // pView != IconView  <=   don't ignore if in icon view (user can move the equations
      //    freely...by drag&drop)
      de->ignore();
    } else {
      // check proposed actions
      if ( (fl & KLFLibModel::DropWillMove) || pViewType == KLFLibDefaultView::IconView ) {
	de->setDropAction(Qt::MoveAction);
      } else {
	de->setDropAction(Qt::CopyAction);
      }
      de->accept();
      // and FAKE a QDragMoveEvent to the item view.
      QDragMoveEvent fakeevent(de->pos(), de->dropAction(), de->mimeData(), de->mouseButtons(),
			       de->keyboardModifiers());
      qApp->sendEvent(thisView()->viewport(), &fakeevent);
    }
    return true;
  }

  /** \warning Caller eventFilter() must ensure not to recurse with fake events ! */
  virtual bool evDrop(QDropEvent *de, const QPoint& pos) {
    if ( pViewType == KLFLibDefaultView::IconView && de->source() == thisView() ) {
      // internal move -> send event directly
      //	qobject_cast<KLFLibDefListView*>(pView)->simulateEvent(event);
      //	qApp->sendEvent(object, event);
      // move the objects ourselves because of bug (?) in Qt's handling?
      QPoint delta = pos - mousePressedContentsPos;
      Q_UNUSED( delta ) ;
      klfDbg( "Delta is "<<delta ) ;
      // and fake a QDragLeaveEvent
      QDragLeaveEvent fakeevent;
      qApp->sendEvent(thisView()->viewport(), &fakeevent);
      // and manually move all selected indexes
      //      moveSelectedIconsBy(delta); .... IF we decide to sometime re-try supporting manual icon positioning...
      thisView()->viewport()->update();
    } else {
      // and FAKE a QDropEvent to the item view.
      klfDbg( "Drop event at position="<<de->pos() ) ;
      QDropEvent fakeevent(de->pos(), de->dropAction(), de->mimeData(), de->mouseButtons(),
			   de->keyboardModifiers());
      qApp->sendEvent(thisView()->viewport(), &fakeevent);
    }
    return true;
  }

  virtual void commonStartDrag(Qt::DropActions supportedActions) {
    // largely based on Qt's QAbstractItemView source in src/gui/itemviews/qabstractitemview.cpp
    QModelIndexList indexes = commonSelectedIndexes();

    if (pViewType == KLFLibDefaultView::IconView) {
      // icon view -> move icons around

      // this is not supported. This functionality was attempted and abandoned.
      return;
    }
    
    klfDbg(  "Normal DRAG..." ) ;
    /*  // DEBUG:
	if (indexes.count() > 0)
	if (v->inherits("KLFLibDefListView")) {
	klfDbg( "Got First index' rect: "
	<<qobject_cast<KLFLibDefListView*>(v)->rectForIndex(indexes[0]) ) ;
	qobject_cast<KLFLibDefListView*>(v)->setPositionForIndex(QPoint(200,100), indexes[0]);
	}
    */
    if (indexes.count() == 0)
      return;
    QMimeData *data = pModel->mimeData(indexes);
    if (data == NULL)
      return;
    QDrag *drag = new QDrag(thisView());
    drag->setMimeData(data);
    QPixmap pix = QPixmap::fromImage(pModel->dragImage(indexes));
    drag->setPixmap(pix);
    drag->setHotSpot(QPoint(0,0));
    Qt::DropAction defaultDropAction = Qt::IgnoreAction;
    
    if (supportedActions & Qt::CopyAction &&
	thisView()->dragDropMode() != QAbstractItemView::InternalMove)
      defaultDropAction = Qt::CopyAction;
    
    drag->exec(supportedActions, defaultDropAction);
  }


  QModelIndex curVisibleIndex(bool firstOrLast) const {
    /*
      int off_y = scrollOffset().y();
      klfDbg( "curVisibleIndex: offset y is "<<off_y ) ;
      QModelIndex it = QModelIndex();
      while ((it = pModel->walkNextIndex(it)).isValid()) {
      klfDbg( "\texploring item it="<<it<<"; bottom="<<thisConstView()->visualRect(it).bottom() ) ;
      if (thisConstView()->visualRect(it).bottom() >= 0) {
      // first index from the beginning, that is after our scroll offset.
      return it;
      }
      }
    */
    QModelIndex index;
    QPoint offset = scrollOffset();
    Q_UNUSED( offset ) ;
    klfDbg( " offset="<<offset ) ;
    int xStart, yStart;
    int xStep, yStep;
    if (firstOrLast) {
      xStep = 40;
      yStep = 40;
      xStart = xStep/2;
      yStart = yStep/2;
    } else {
      xStep = -40;
      yStep = -40;
      xStart = thisConstView()->width() - xStep/2;
      yStart = thisConstView()->height() - yStep/2;
    }
    int xpos, ypos;
    for (xpos = xStart; xpos > 0 && xpos < thisConstView()->width(); xpos += xStep) {
      for (ypos = yStart; ypos > 0 && ypos < thisConstView()->height(); ypos += yStep) {
	if ((index = thisConstView()->indexAt(QPoint(xpos,ypos))).isValid()) {
	  klfDbg( ": Found index = "<<index<<" at pos=("<<xpos<<","<<ypos<<"); "
		  <<" with offset "<<offset ) ;
	  return firstOrLast ? pModel->walkPrevIndex(index) : pModel->walkNextIndex(index);
	}
      }
    }
    return firstOrLast ? pModel->walkNextIndex(QModelIndex()) : pModel->walkPrevIndex(QModelIndex());
  }

  virtual void modelInitialized() { }

protected:
  KLFLibModel *pModel;
  KLFLibDefaultView *pDView;
  KLFLibDefaultView::ViewType pViewType;
  QPoint mousePressedContentsPos;

  virtual QModelIndexList commonSelectedIndexes() const = 0;
  //  virtual void commonInternalDrag(Qt::DropActions a) = 0;
  virtual QAbstractItemView *thisView() = 0;
  virtual const QAbstractItemView *thisConstView() const = 0;
  virtual QPoint scrollOffset() const = 0;

  /** Returns contents position */
  QPoint eventPosBase(QObject *object, QDragEnterEvent *event, int horoffset, int veroffset) {
    if (object == thisView()->viewport())
      return event->pos() + QPoint(horoffset, veroffset);
    if (object == thisView())
      return thisView()->viewport()->mapFromGlobal(thisView()->mapToGlobal(event->pos()))
	+ QPoint(horoffset, veroffset);
    return event->pos() + QPoint(horoffset, veroffset);
  }

  virtual bool setTheModel(QAbstractItemModel *m) {
    KLFLibModel *model = qobject_cast<KLFLibModel*>(m);
    if (model == NULL) {
      qWarning()<<"KLFLibDefViewCommon::setTheModel: model is NULL or not a KLFLibModel :"<<model<<" !";
      return false;
    }
    pModel = model;
    return true;
  }

  /*   virtual void preparePaintEvent(QPaintEvent *e) */
  /*   { */
  /*     KLF_DEBUG_TIME_BLOCK(KLF_FUNC_NAME) ; */
  /*     klfDbg("e->rect()="<<e->rect()); */
  /*     const QAbstractItemView *v = thisView(); */
  /*     QModelIndex idx; */
  /*     while ((idx = pModel->walkNextIndex(idx)).isValid()) { */
  /*       klfDbg("Testing for prefetching item at row="<<idx.row()<<" its rect="<<v->visualRect(idx)); */
  /*       if (e->rect().intersects(v->visualRect(idx))) { */
  /* 	pModel->prefetch(QModelIndexList()<<idx); */
  /*       } */
  /*     } */
  /*   } */

};


/** \internal */
class KLFLibDefTreeView : public QTreeView, public KLFLibDefViewCommon
{
  Q_OBJECT
public:
  KLFLibDefTreeView(KLFLibDefaultView *parent)
    : QTreeView(parent), KLFLibDefViewCommon(parent), inPaintEvent(false), pInEventFilter(false)
  {
    installEventFilter(this);
    viewport()->installEventFilter(this);
  }

  virtual bool eventFilter(QObject *object, QEvent *event) {
    if (pInEventFilter)
      return QTreeView::eventFilter(object, event);
    pInEventFilter = true;
    bool eat = false;
    if (event->type() == QEvent::DragEnter) {
      eat = evDragEnter((QDragEnterEvent*)event, eventPos(object, (QDragEnterEvent*)event));
    } else if (event->type() == QEvent::DragMove) {
      eat = evDragMove((QDragMoveEvent*)event, eventPos(object, (QDragEnterEvent*)event));
    } else if (event->type() == QEvent::Drop) {
      eat = evDrop((QDropEvent*)event, eventPos(object, (QDragEnterEvent*)event));
    }
    //    if (object == this && event->type() == QEvent::Paint)
    //      preparePaintEvent((QPaintEvent*)event);
    pInEventFilter = false;
    if (eat)
      return eat;
    return QTreeView::eventFilter(object, event);
  }
  virtual void setModel(QAbstractItemModel *model) {
    if ( setTheModel(model) ) {
      QTreeView::setModel(model);
    }
  }


public slots:

  virtual void selectAll() {
    pDView->slotSelectAll();
  }

  void ensureExpandedTo(const QModelIndexList& mil)
  {
    int k;
    for (k = 0; k < mil.size(); ++k) {
      QModelIndex idx = mil[k];
      if (!idx.isValid())
	continue;
      while (idx.parent().isValid()) {
	klfDbg("expanding index "<<idx.parent()) ;
	expand(idx.parent());
	idx = idx.parent();
      }
    }
  }


protected:
  QModelIndexList commonSelectedIndexes() const { return selectedIndexes(); }
  // void commonInternalDrag(Qt::DropActions) {  }
  QAbstractItemView *thisView() { return this; }
  const QAbstractItemView *thisConstView() const { return this; }
  QPoint scrollOffset() const { return QPoint(horizontalOffset(), verticalOffset()); }

  QPoint eventPos(QObject *object, QDragEnterEvent *event) {
    return eventPosBase(object, event, horizontalOffset(), verticalOffset());
  }

  void startDrag(Qt::DropActions supportedActions) {
    commonStartDrag(supportedActions);
  }

  bool inPaintEvent;
  virtual void paintEvent(QPaintEvent *event) {
    QTreeView::paintEvent(event);
  }

  // reimpl. from QTreeView for debugging a segfault in fetchMore()
  virtual void rowsInserted(const QModelIndex& parent, int start, int end)
  {
    KLF_DEBUG_TIME_BLOCK(KLF_FUNC_NAME) ;
    QTreeView::rowsInserted(parent, start, end);
  }

  bool pInEventFilter;
};

/** \internal */
class KLFLibDefListView : public QListView, public KLFLibDefViewCommon
{
  Q_OBJECT
public:
  KLFLibDefListView(KLFLibDefaultView *parent)
    : QListView(parent), KLFLibDefViewCommon(parent), inPaintEvent(false), pInEventFilter(false),
      pInitialLayoutDone(false), pWantRelayout(true)
  {
    installEventFilter(this);
    viewport()->installEventFilter(this);
  }

  virtual bool eventFilter(QObject *object, QEvent *event) {
    if (pInEventFilter)
      return false;
    pInEventFilter = true;
    bool eat = false;
    if (event->type() == QEvent::DragEnter) {
      eat = evDragEnter((QDragEnterEvent*)event, eventPos(object, (QDragEnterEvent*)event));
    } else if (event->type() == QEvent::DragMove) {
      eat = evDragMove((QDragMoveEvent*)event, eventPos(object, (QDragEnterEvent*)event));
    } else if (event->type() == QEvent::Drop) {
      eat = evDrop((QDropEvent*)event, eventPos(object, (QDragEnterEvent*)event));
    }
    //    if (object == this && event->type() == QEvent::Paint)
    //      preparePaintEvent((QPaintEvent*)event);
    pInEventFilter = false;
    if (eat)
      return eat;
    return QListView::eventFilter(object, event);
  }
  virtual void setModel(QAbstractItemModel *model) {
    if ( setTheModel(model) ) {
      QListView::setModel(model);
    }
  }


  void forceRelayout() {
    scheduleDelayedItemsLayout(); // force a re-layout
  }
  
  virtual void modelInitialized() {
    klfDbg("scheduling an items layout...");
    if (isVisible()) {
      scheduleDelayedItemsLayout();
      pInitialLayoutDone = true;
    } else {
      pInitialLayoutDone = false;
    }
  }

protected:
  virtual QModelIndexList commonSelectedIndexes() const { return selectedIndexes(); }
  //  virtual void commonInternalDrag(Qt::DropActions a) { internalDrag(a); }
  virtual QAbstractItemView *thisView() { return this; }
  virtual const QAbstractItemView *thisConstView() const { return this; }
  virtual QPoint scrollOffset() const { return QPoint(horizontalOffset(), verticalOffset()); }

  QPoint eventPos(QObject *object, QDragEnterEvent *event) {
    return eventPosBase(object, event, horizontalOffset(), verticalOffset());
  }

  bool inPaintEvent;
  virtual void paintEvent(QPaintEvent *event) {
    QListView::paintEvent(event);
  }

  bool pInEventFilter;
  bool pInitialLayoutDone;

  virtual void startDrag(Qt::DropActions supportedActions) {
    commonStartDrag(supportedActions);
  }

  virtual void showEvent(QShowEvent *event) {
    if (!event->spontaneous()) {
      klfDbg("Show event! initialLayoutDone="<<pInitialLayoutDone<<"; size hint="<<sizeHint());
      if (!pInitialLayoutDone) {
	scheduleDelayedItemsLayout();
	pInitialLayoutDone = true;
      }
    }
    QListView::showEvent(event);
  }
  
  bool pWantRelayout;

};







// ----------------------------------------------------------------------









/** \internal */
class KLFLibLocalFileOpenWidget : public QWidget, protected Ui::KLFLibLocalFileWidget
{
  Q_OBJECT

  Q_PROPERTY(bool readyToOpen READ isReadyToOpen)
public:
  typedef KLFLibBasicWidgetFactory::LocalFileType LocalFileType;

  KLFLibLocalFileOpenWidget(QWidget *parent,
			    const QList<LocalFileType>& fileTypes)
    : QWidget(parent)
  {
    pReadyToOpen = false;
    pReadyToOpenUrl = QUrl();
    pFileTypes = fileTypes;
    setupUi(this);
    
    QStringList namefilters;
    int k;
    for (k = 0; k < pFileTypes.size(); ++k)
      namefilters << pFileTypes[k].filepattern;
    QDirModel *dirModel = new QDirModel(namefilters,
					QDir::AllEntries|QDir::AllDirs|QDir::NoDotAndDotDot,
					QDir::DirsFirst|QDir::IgnoreCase, this);

    QCompleter *fileNameCompleter = new QCompleter(this);
    fileNameCompleter->setModel(dirModel);
    txtFile->setCompleter(fileNameCompleter);

    setUrl(QUrl());
  }
  virtual ~KLFLibLocalFileOpenWidget() { }

  virtual bool isReadyToOpen() const { return pReadyToOpen; }

  virtual QString selectedFName() const {
    return QDir::fromNativeSeparators(txtFile->text());
  }
  virtual QString selectedScheme() const
  {
    QString filename = selectedFName();
    LocalFileType ft = fileTypeForFileName(filename);
    if (!ft.scheme.isEmpty())
      return ft.scheme;
    // fall back to guessing the scheme with the file contents
    return KLFLibBasicWidgetFactory::guessLocalFileScheme(filename);
  }

  LocalFileType fileTypeForFileName(const QString& filename) const
  {
    QString fname = QFileInfo(filename).fileName();
    int k;
    for (k = 0; k < pFileTypes.size(); ++k) {
      // test for this file type
      QRegExp rgx(pFileTypes[k].filepattern, Qt::CaseInsensitive, QRegExp::Wildcard);
      if (rgx.exactMatch(fname))
	return pFileTypes[k];
    }
    if (!pFileTypes.size())
      return LocalFileType();
    return pFileTypes[0];
  }

  virtual void setUrl(const QUrl& url) {
    if (url.isValid() && url.path().length()>0)
      txtFile->setText(QDir::toNativeSeparators(url.path()));
    else
      txtFile->setText(QDir::toNativeSeparators(klfconfig.LibraryBrowser.lastFileDialogPath+"/"));
  }
  virtual QUrl url() const {
    QString fname = selectedFName();
    QString scheme = selectedScheme();
    if (scheme.isEmpty())
      return QUrl(); // invalid file type...

    QUrl url = QUrl::fromLocalFile(fname);
    url.setScheme(scheme);
    return url;
  }

signals:
  void readyToOpen(bool ready);

public slots:
  void setReadyToOpen(bool ready, const QUrl& url) {
    if (ready != pReadyToOpen || url != pReadyToOpenUrl) {
      // ready to open state changes
      pReadyToOpen = ready;
      pReadyToOpenUrl = url;
      emit readyToOpen(ready);
    }
  }

protected:
  QList<LocalFileType> pFileTypes;
  bool pReadyToOpen;
  QUrl pReadyToOpenUrl;
  enum BrowseType { BrowseOpen, BrowseSave };

  virtual bool checkIsReadyToOpen(const QString& text = QString()) const
  {
    QString t = text.isNull() ? txtFile->text() : text;
    return QFileInfo(t).isFile();
  }


protected slots:
  virtual bool browseFileName(BrowseType bt)
  {
    static QString selectedFilter;

    QString path = txtFile->text();
    if (path.isEmpty())
      path = klfconfig.LibraryBrowser.lastFileDialogPath;
    QString pathfilter;
    if (!QFileInfo(path).isDir()) {
      LocalFileType pathft = fileTypeForFileName(path);
      if (!pathft.filter.isEmpty())
	pathfilter = pathft.filter;
    }

    QStringList filters;
    QStringList fpatterns;
    int k;
    int pathfilterN = -1;
    for (k = 0; k < pFileTypes.size(); ++k) {
      filters << pFileTypes[k].filter;
      fpatterns << pFileTypes[k].filepattern;
      if (pFileTypes[k].filter == pathfilter)
	pathfilterN = k; // remember this one
    }
    if (bt == BrowseOpen) {
      // only when opening
      // NOTE: this doesn't work for save when we want to preserve 'path's extension, because
      //   Q(K?)FileDialog foolishly feels obliged to automatically force the first extension
      //   in the patterns list.
      if (pathfilterN >= 0)
	filters.removeAt(pathfilterN);
      filters.prepend(tr("All Known Files (%1)").arg(fpatterns.join(" ")));
      if (pathfilterN >= 0)
	filters.prepend(pathfilter);
    } else {
      if (pathfilterN >= 0) {
	filters.removeAt(pathfilterN);
	filters.prepend(pathfilter);
      }
    }
    filters.append(tr("All Files (*)"));
    QString filter = filters.join(";;");
    QString title = tr("Select Library Resource File");
    QString name;
    if (bt == BrowseOpen) {
      name = QFileDialog::getOpenFileName(this, title, path, filter, &selectedFilter);
    } else if (bt == BrowseSave) {
      name = QFileDialog::getSaveFileName(this, title, path, filter, &selectedFilter);
    } else {
      qWarning()<<"KLFLibLocalFileOpenWidget::browseFileName: bad bt="<<bt;
      name = QFileDialog::getSaveFileName(this, title, path, filter, &selectedFilter);
    }
    if ( name.isEmpty() )
      return false;

    klfconfig.LibraryBrowser.lastFileDialogPath = QFileInfo(name).absolutePath();

    txtFile->setText(name); // this will call on_txtFile_textChanged()
    return true;
  }
  virtual void on_btnBrowse_clicked()
  {
    browseFileName(BrowseOpen);
  }
  virtual void on_txtFile_textChanged(const QString& text)
  {
    setReadyToOpen(checkIsReadyToOpen(text), url());
  }
};

// --------------------------------------

/** \internal */
class KLFLibLocalFileCreateWidget : public KLFLibLocalFileOpenWidget
{
  Q_OBJECT

  Q_PROPERTY(bool readyToCreate READ isReadyToCreate WRITE setReadyToCreate)
public:
  KLFLibLocalFileCreateWidget(QWidget *parent,
			    const QList<LocalFileType>& fileTypes)
    : KLFLibLocalFileOpenWidget(parent, fileTypes)
  {
    pConfirmedOverwrite = false;
    pReadyToCreate = false;
  }
  virtual ~KLFLibLocalFileCreateWidget() { }

  QString fileName() const {
    return txtFile->text();
  }

  virtual bool isReadyToCreate() const { return pReadyToCreate; }

  virtual bool confirmedOverwrite() const { return pConfirmedOverwrite; }

signals:
  void readyToCreate(bool ready);

public slots:
  void setReadyToCreate(bool ready) {
    if (ready != pReadyToCreate) {
      pReadyToCreate = ready;
      emit readyToCreate(pReadyToCreate);
    }
  }

protected slots:
  virtual void on_btnBrowse_clicked()
  {
    if ( browseFileName(BrowseSave) )
      pConfirmedOverwrite = true;
  }
  virtual void on_txtFile_textChanged(const QString& text)
  {
    pConfirmedOverwrite = false;
    setReadyToCreate(QFileInfo(text).absoluteDir().exists());
  }

protected:
  bool pConfirmedOverwrite;
  bool pReadyToCreate;
};



#ifdef KLF_DEBUG
#include <QDebug>
KLF_EXPORT  QDebug& operator<<(QDebug& dbg, const KLFLibModelCache::NodeId& n);
KLF_EXPORT  QDebug& operator<<(QDebug& dbg, const KLFLibModelCache::Node& n);
KLF_EXPORT  QDebug& operator<<(QDebug& dbg, const KLFLibModelCache::EntryNode& en);
KLF_EXPORT  QDebug& operator<<(QDebug& dbg, const KLFLibModelCache::CategoryLabelNode& cn);
#endif

#endif
