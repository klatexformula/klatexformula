/***************************************************************************
 *   file klflibview_p.h
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


/** \file
 * This header contains (in principle private) auxiliary classes for
 * library routines defined in klflibview.cpp */


#ifndef KLFLIBVIEW_P_H
#define KLFLIBVIEW_P_H

#include <math.h> // abs()

#include <QApplication>
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

#include <ui_klfliblocalfilewidget.h>

#include "klflib.h"
#include "klflibview.h"


class KLFLibDefTreeView;


/** \internal */
inline QPointF sizeToPointF(const QSizeF& s) { return QPointF(s.width(), s.height()); }
/** \internal */
inline QSizeF pointToSizeF(const QPointF& p) { return QSizeF(p.x(), p.y()); }


// /** \internal */
// static QColor image_average_color(const QImage& img)
// {
//   // img should not be transparent
// 
//   QColor avgc;
//   QColor c;
//   int x, y, N = 0;
//   for (x = 0; x < img.width(); ++x) {
//     for (y = 0; y < img.height(); ++y) {
//       c = QColor(img.pixel(x, y));
//       avgc.setHsvF( (avgc.hueF()*N + c.hueF())/(N+1),
// 		    (avgc.saturationF()*N + c.saturationF())/(N+1),
// 		    (avgc.valueF()*N + c.valueF())/(N+1) );
//       ++N;
//     }
//   }
//   return avgc;
// }

// ---

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
    Node(ItemKind k) : kind(k), parent(NodeId()), children(QList<NodeId>()), numDisplayFetched(0) { }
    Node(const Node& other) : kind(other.kind), parent(other.parent), children(other.children),
			      numDisplayFetched(other.numDisplayFetched) { }
    virtual ~Node() { }
    ItemKind kind;
    NodeId parent;
    QList<NodeId> children;
    int numDisplayFetched;
  };
  struct EntryNode : public Node {
    EntryNode() : Node(EntryKind), entryid(-1), minimalist(false), entry() { }
    KLFLib::entryId entryid;
    /** if TRUE, 'entry' only holds category/tags/datetime/latex/previewsize, no pixmap, no style. */
    bool minimalist;
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

  EntryNode pInvalidEntryNode; // not initialized, not used except for return values
  //                              of functions aborting their call eg. getNodeRef()


  KLFLibModelCache(KLFLibModel * model)
    : pModel(model)
  {
    pCategoryLabelCacheContainsInvalid = false;

    pIsFetchingMore = false;

    pLastSortPropId = KLFLibEntry::DateTime;
    pLastSortOrder = Qt::DescendingOrder;
  }

  virtual ~KLFLibModelCache() { }

  KLFLibModel *pModel;

  void rebuildCache();

  /** If row is negative, it will be looked up automatically. */
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
   * If count is -1, uses fetchBatchCount(). */
  void ensureNotMinimalist(NodeId nodeId, int count = -1);

  bool canFetchMore(NodeId parentId);
  void fetchMore(NodeId parentId, int batchCount);

  void updateData(const QList<KLFLib::entryId>& entryIdList, int modifyType);

  /** emits QAbstractItemModel-appropriate LAYOUT CHANGES SIGNALS if \c notifyQtApi is true. IT ALWAYS
   * EMITS APPROPRIATE SIGNALS FOR SUB-CATEGORIES THAT ARE CREATED TO FIT THE ITEM. */
  void treeInsertEntry(const NodeId& e, bool notifyQtApi = true);
  /** emits QAbstractItemModel-appropriate signals and updates indexes if \c notifyQtApi is true
   *
   * This function sets the entryId of the removed entry to -1 so that it cannot be re-found in a
   * future search. */
  void treeRemoveEntry(const NodeId& e, bool notifyQtApi = true);

  /** emits QAbstractItemModel-appropriate signals and updates indexes if \c notifyQtApi is true */
  IndexType cacheFindCategoryLabel(QStringList catelements, bool createIfNotExists = false,
				   bool notifyQtApi = false);

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

  QString nodeValue(NodeId node, int propId);

  /** Sort a category's children */
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
  bool pCategoryLabelCacheContainsInvalid;

  QStringList pCatListCache;

  bool pIsFetchingMore;

  int pLastSortPropId;
  Qt::SortOrder pLastSortOrder;
};












// ---

/** \internal */
class KLFLibDefViewCommon
{
public:
  KLFLibDefViewCommon(KLFLibDefaultView *dview)
    : pDView(dview), pViewType(dview->viewType()) { }

  virtual void moveSelectedIconsBy(const QPoint& delta) = 0;

  /** \warning Caller eventFilter() must ensure not to recurse with fake events ! */  
  virtual bool evDragEnter(QDragEnterEvent *de, const QPoint& pos) {
    uint fl = pModel->dropFlags(de, thisView());
    klfDbg( "KLFLibDefViewCommon::evDragEnter: drop flags are "<<fl<<"; this viewtype="<<pViewType ) ;
    fprintf(stderr, "\t\tflags are %u", fl);
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
      klfDbg( "Delta is "<<delta ) ;
      // and fake a QDragLeaveEvent
      QDragLeaveEvent fakeevent;
      qApp->sendEvent(thisView()->viewport(), &fakeevent);
      // and manually move all selected indexes
      moveSelectedIconsBy(delta);
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
      klfDbg( "Internal DRAG, move icons around..." ) ;
      // if icon positions are locked then abort
      if ( ! pDView->canMoveIcons() )
	return;
      /// \bug ......BUG/TODO........ WARNING: QListView::internalDrag() is NOT in offical Qt API !
      commonInternalDrag(supportedActions);
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


  QModelIndex curVisibleIndex() const {
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
    klfDbg( " offset="<<offset ) ;
    int xStep = 40;
    int yStep = 40;
    int xpos, ypos;
    for (xpos = xStep/2; xpos < thisConstView()->width(); xpos += xStep) {
      for (ypos = yStep/2; ypos < thisConstView()->height(); ypos += yStep) {
	if ((index = thisConstView()->indexAt(QPoint(xpos,ypos))).isValid()) {
	  klfDbg( ": Found index = "<<index<<" at pos=("<<xpos<<","<<ypos<<"); "
		  <<" with offset "<<offset ) ;
	  return index;
	}
      }
    }
    return pModel->walkNextIndex(QModelIndex());
  }

  virtual void modelInitialized() { }

protected:
  KLFLibModel *pModel;
  KLFLibDefaultView *pDView;
  KLFLibDefaultView::ViewType pViewType;
  QPoint mousePressedContentsPos;

  virtual QModelIndexList commonSelectedIndexes() const = 0;
  virtual void commonInternalDrag(Qt::DropActions a) = 0;
  virtual QAbstractItemView *thisView() = 0;
  virtual const QAbstractItemView *thisConstView() const = 0;
  virtual QPoint scrollOffset() const = 0;

  /** Returns contents position */
  virtual QPoint eventPos(QObject *object, QDragEnterEvent *event, int horoffset, int veroffset) {
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

  virtual void moveSelectedIconsBy(const QPoint& /*delta*/) { }

public slots:

  virtual void selectAll() {
    pDView->slotSelectAll();
  }

protected:
  virtual QModelIndexList commonSelectedIndexes() const { return selectedIndexes(); }
  virtual void commonInternalDrag(Qt::DropActions) {  }
  virtual QAbstractItemView *thisView() { return this; }
  virtual const QAbstractItemView *thisConstView() const { return this; }
  virtual QPoint scrollOffset() const { return QPoint(horizontalOffset(), verticalOffset()); }

  virtual QPoint eventPos(QObject *object, QDragEnterEvent *event) {
    return KLFLibDefViewCommon::eventPos(object, event, horizontalOffset(), verticalOffset());
  }

  virtual void startDrag(Qt::DropActions supportedActions) {
    commonStartDrag(supportedActions);
  }

  bool inPaintEvent;
  virtual void paintEvent(QPaintEvent *event) {
    QTreeView::paintEvent(event);
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
      pInitialLayoutDone(false), pSavingIconPositions(false), pWantRelayout(true)
  {
    pDelayedSetIconPositions.clear();

    installEventFilter(this);
    viewport()->installEventFilter(this);
  }

  virtual bool eventFilter(QObject *object, QEvent *event) {
    if (pInEventFilter)
      return false;
    pInEventFilter = true;
    bool eat = false;
    if (object == this && event->type() == QEvent::Polish) {
      /*      klfDbg( "Polish! delayedseticonpos="<<pDelayedSetIconPositions ) ;
	      if (!pDelayedSetIconPositions.isEmpty()) {
	      // QListView wants to have its own doItemsLayout() function called first, before we start
	      // fiddling with item positions.
	      / ** \bug .......... BUG/TODO/WARNING: QListView::doItemsLayout() is NOT in official API! * /
	      QListView::doItemsLayout();
	      loadIconPositions(pDelayedSetIconPositions, true);
	      } else {
	      if (pWantRelayout)
	      forceRelayout(true); // relayout if wanted
	      }
	      pHasBeenPolished = true;
	      eat = false;
      */
    }
    if (event->type() == QEvent::DragEnter) {
      eat = evDragEnter((QDragEnterEvent*)event, eventPos(object, (QDragEnterEvent*)event));
    } else if (event->type() == QEvent::DragMove) {
      eat = evDragMove((QDragMoveEvent*)event, eventPos(object, (QDragEnterEvent*)event));
    } else if (event->type() == QEvent::Drop) {
      eat = evDrop((QDropEvent*)event, eventPos(object, (QDragEnterEvent*)event));
    }
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

  /** Returns the positions of all the icon positions and for each entry IDs to
   * which they refer.
   * */
  virtual QMap<KLFLib::entryId,QPoint> allIconPositions() const {
    QMap<KLFLib::entryId,QPoint> iconPositions;
    /*    // walk all indexes in view
	  QModelIndex it = pModel->walkNextIndex(QModelIndex());
	  while (it.isValid()) {
	  KLFLib::entryId eid = pModel->entryIdForIndex(it);
	  if (eid != -1) {
	  iconPositions[eid] = iconPosition(it);
	  }
	  it = pModel->walkNextIndex(it);
	  } */
    return iconPositions;
  }

  /** If we are in the process of saving icon positions (may be called from
   * for example an update method which was called because the entry was changed
   * when setting new icon position) */
  bool isSavingIconPositions() const { return pSavingIconPositions; }

  /** this function restores the positions of all the icons as described
   * in the \c iconPositions map, for example which has been obtained by a call
   * to \ref allIconPositions() some time earlier.
   *
   * This function does NOT save the new icon positions; this function is precisely meant
   * for usage from a "loadGuiState()" function or from an "update resource data ()" function.
   */
  virtual void loadIconPositions(const QMap<KLFLib::entryId,QPoint>& iconPositions,
				 bool forcenow = false) {
    if ( pSavingIconPositions )
      return;

    /*    if ( ! pHasBeenPolished && ! forcenow ) {
	  pDelayedSetIconPositions = iconPositions;
	  klfDbg( "KLFLibDefListView::loadIconPositions: delaying action!" ) ;
	  return;
    */
    /*	  klfDbg( "KLFLibDefListView::loadIconPositions: setting icon positions "<<iconPositions ) ;
	  QMap<KLFLib::entryId,QPoint>::const_iterator it;
	  for (it = iconPositions.begin(); it != iconPositions.end(); ++it) {
	  QModelIndex index = pModel->findEntryId(it.key());
	  if (!index.isValid())
	  continue;
	  QPoint pos = *it;
	  klfDbg( ": About to set single icon position.." ) ;
	  setIconPosition(index, pos, true);
	  klfDbg( "Set single icon position OK." ) ;
	  }
    */
    pDelayedSetIconPositions.clear();
  }


  virtual void moveSelectedIconsBy(const QPoint& delta) {
    /*    int k;
	  QModelIndexList sel = selectionModel()->selectedIndexes();
	  for (k = 0; k < sel.size(); ++k) {
	  QPoint newp = rectForIndex(sel[k]).topLeft() + delta;
	  if (newp.x() < 0) newp.setX(0);
	  if (newp.y() < 0) newp.setY(0);
	  setIconPosition(sel[k], newp);
	  }
    */
  }

  void forceRelayout() {

    scheduleDelayedItemsLayout(); // force a re-layout
    //    doItemsLayout(); // force a re-layout

    /*	  bool wr = pWantRelayout;
	  pWantRelayout = true;
	  doItemsLayout(); // force re-layout
	  if (!isPolishing && !wr) {
	  // if we didn't by default want a relayout (ie. icon positions were controlled)
	  saveIconPositions();
	  }
	  pWantRelayout = false;
    */
  }

  QPoint iconPosition(const QModelIndex& index) const {
    return rectForIndex(index).topLeft();
  }
  void setIconPosition(const QModelIndex& index, const QPoint& pos, bool dontSave = false) {
    if (index.column() > 0)
      return;
    //    if (rectForIndex(index).topLeft() == pos)
    //      return;
    /*
      klfDbg( "Functional setIconPosition("<<index<<","<<pos<<")" ) ;
      setPositionForIndex(pos, index);
      if (!dontSave)
      saveIconPosition(index);
      pWantRelayout = false;
    */
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
  virtual void commonInternalDrag(Qt::DropActions a) { internalDrag(a); }
  virtual QAbstractItemView *thisView() { return this; }
  virtual const QAbstractItemView *thisConstView() const { return this; }
  virtual QPoint scrollOffset() const { return QPoint(horizontalOffset(), verticalOffset()); }

  virtual QPoint eventPos(QObject *object, QDragEnterEvent *event) {
    return KLFLibDefViewCommon::eventPos(object, event, horizontalOffset(), verticalOffset());
  }

  bool inPaintEvent;
  virtual void paintEvent(QPaintEvent *event) {
    QListView::paintEvent(event);
  }

  bool pInEventFilter;
  bool pInitialLayoutDone;
  bool pSavingIconPositions;

  QMap<KLFLib::entryId, QPoint> pDelayedSetIconPositions;

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

  //   virtual void doItemsLayout() {
  //     QListView::doItemsLayout();
  //     / ** \bug ......BUG/TODO........ WARNING: QListView::doItemsLayout() is NOT in offical Qt API !
  //      * /
  //     /*    klfDbg( "doItemsLayout!" ) ;
  //     // Qt want its own doItemsLayout() to be called first (in case new indexes have appeared, or
  //     // old ones dissapeared)
  //     QMap<KLFLib::entryId,QPoint> bkpiconpos;
  //     if (!pWantRelayout) {
  //     bkpiconpos = allIconPositions(); // save current icon positions
  //       klfDbg( "Got backup icon positions: "<<bkpiconpos ) ;
  //       }
  //     // do qt's layout
  //     QListView::doItemsLayout();
  //     // but then we want to keep our layout if !pWantRelayout:
  //     if (!pWantRelayout)
  //       loadIconPositions(bkpiconpos);

  //     pWantRelayout = false;
  //     klfDbg( "doItemsLayout finished!" ) ;
  //     */
  //   } 
  
  bool pWantRelayout;

  void saveIconPositions() {
    /*    klfDbg( "saveIconPositions()" ) ;
    KLFLibModel *model = qobject_cast<KLFLibModel*>(this->model());
    // walk all indices, fill entryIdList
    QModelIndex index = model->walkNextIndex(QModelIndex());
    while (index.isValid()) {
      klfDbg( "Walking index "<<index<<"; our model is "<<((QAbstractItemModel*)model)
	      <<"item's model is"<<((QAbstractItemModel*)index.model()) ) ;
      saveIconPosition(index);
      index = model->walkNextIndex(index);
    }
    klfDbg( "Done saving all icon positions." ) ; */
  }
  void saveIconPosition(const QModelIndex& index) {
    if (index.column()>0)
      return;
    /*    klfDbg( "saveIconPosition()" ) ;
    KLFLibModel *model = qobject_cast<KLFLibModel*>(this->model());
    KLFLibEntry edummy = index.data(KLFLibModel::FullEntryItemRole).value<KLFLibEntry>();
    int propId = edummy.setEntryProperty("IconView_IconPosition", rectForIndex(index).topLeft());
    pSavingIconPositions = true;
    model->changeEntries(QModelIndexList() << index, propId, edummy.property(propId)); 
    pSavingIconPositions = false;
    klfDbg( "end of saveIconPosition()" ) ; */
  }
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
  }
  virtual ~KLFLibLocalFileOpenWidget() { }

  virtual bool isReadyToOpen() const { return pReadyToOpen; }

  virtual QString selectedFName() const { return txtFile->text(); }
  virtual QString selectedScheme() const {
    QString filename = selectedFName();
    QString fname = QFileInfo(filename).fileName();
    int k;
    for (k = 0; k < pFileTypes.size(); ++k) {
      // test for this file type
      QRegExp rgx(pFileTypes[k].filepattern, Qt::CaseInsensitive, QRegExp::Wildcard);
      if (rgx.exactMatch(fname))
	return pFileTypes[k].scheme;
    }
    // fall back to guessing the scheme with the file contents
    return KLFLibBasicWidgetFactory::guessLocalFileScheme(filename);
  }

  virtual void setUrl(const QUrl& url) {
    txtFile->setText(url.path());
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

    QStringList filters;
    QStringList fpatterns;
    int k;
    for (k = 0; k < pFileTypes.size(); ++k) {
      filters << pFileTypes[k].filter;
      fpatterns << pFileTypes[k].filepattern;
    }
    if (bt == BrowseOpen)
      filters.prepend(tr("All Known Files (%1)").arg(fpatterns.join(" "))); // only for opening
    filters << tr("All Files (*)");
    QString filter = filters.join(";;");
    QString title = tr("Select Library Resource File");
    QString name;
    if (bt == BrowseOpen) {
      name = QFileDialog::getOpenFileName(this, title, QDir::homePath(), filter, &selectedFilter);
    } else if (bt == BrowseSave) {
      name = QFileDialog::getSaveFileName(this, title, QDir::homePath(), filter, &selectedFilter);
    } else {
      qWarning()<<"KLFLibLocalFileOpenWidget::browseFileName: bad bt="<<bt;
      name = QFileDialog::getSaveFileName(this, title, QDir::homePath(), filter, &selectedFilter);
    }
    if ( name.isEmpty() )
      return false;

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

// ---

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






#endif
