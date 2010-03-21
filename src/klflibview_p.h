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

#include <QApplication>
#include <QAbstractItemView>
#include <QTreeView>
#include <QListView>
#include <QMimeData>
#include <QDrag>
#include <QDragEnterEvent>
#include <QDragMoveEvent>
#include <QDropEvent>

#include "klflib.h"
#include "klflibview.h"

class KLFLibDefTreeView;


inline QPointF sizeToPointF(const QSizeF& s) { return QPointF(s.width(), s.height()); }
inline QSizeF pointToSizeF(const QPointF& p) { return QSizeF(p.x(), p.y()); }

static QImage transparentify_image(const QImage& img, qreal factor)
{
  // set the image opacity to factor (by multiplying each alpha value by factor)
  QImage img2 = img.convertToFormat(QImage::Format_ARGB32_Premultiplied);
  int k, j;
  for (k = 0; k < img.height(); ++k) {
    for (j = 0; j < img.width(); ++j) {
      QRgb c = img2.pixel(j, k);
      img2.setPixel(j, k, qRgba(qRed(c),qGreen(c),qBlue(c),(int)(factor*qAlpha(c))));
    }
  }
  return img2;
}

static QImage autocrop_image(const QImage& img, int alpha_threshold = 0)
{
  // crop transparent borders
  int x, y;
  int min_x = -1, max_x = -1, min_y = -1, max_y = -1;
  // so first find borders
  for (x = 0; x < img.width(); ++x) {
    for (y = 0; y < img.height(); ++y) {
      if (qAlpha(img.pixel(x,y)) - alpha_threshold > 0) {
	// opaque pixel: include it.
	if (min_x == -1 || min_x > x) min_x = x;
	if (max_x == -1 || max_x < x) max_x = x;
	if (min_y == -1 || min_y > y) min_y = y;
	if (max_y == -1 || max_y < y) max_y = y;
      }
    }
  }
  return img.copy(QRect(QPoint(min_x, min_y), QPoint(max_x, max_y)));
}


//static void klf_common_start_drag(QAbstractItemView *v, Qt::DropActions supportedActions);


class KLFLibDefViewCommon
{
public:
  KLFLibDefViewCommon(KLFLibDefaultView *dview)
    : pDView(dview), pViewType(dview->viewType()) { }

  virtual void moveSelectedIconsBy(const QPoint& delta) = 0;

  /** \warning Caller eventFilter() must ensure not to recurse with fake events ! */  
  virtual bool evDragEnter(QDragEnterEvent *de, const QPoint& pos) {
    uint fl = pModel->dropFlags(de);
    qDebug()<<"KLFLibDefViewCommon::evDragEnter: drop flags are "<<fl<<"; this viewtype="<<pViewType;
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
      qDebug()<<"Accepted drag enter event issued at pos="<<pos;
      // and FAKE a QDragMoveEvent to the item view.
      QDragEnterEvent fakeevent(de->pos(), de->dropAction(), de->mimeData(), de->mouseButtons(),
				de->keyboardModifiers());
      qApp->sendEvent(thisView()->viewport(), &fakeevent);
    }
    return true;
  }

  /** \warning Caller eventFilter() must ensure not to recurse with fake events ! */  
  virtual bool evDragMove(QDragMoveEvent *de, const QPoint& pos) {
    uint fl = pModel->dropFlags(de);
    qDebug()<<"KLFLibDefViewCommon::evDragMove: flags are "<<fl<<"; pos is "<<pos;
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
      qDebug()<<"Delta is "<<delta;
      // and fake a QDragLeaveEvent
      QDragLeaveEvent fakeevent;
      qApp->sendEvent(thisView()->viewport(), &fakeevent);
      // and manually move all selected indexes
      moveSelectedIconsBy(delta);
      thisView()->viewport()->update();
    } else {
      // and FAKE a QDropEvent to the item view.
      qDebug()<<"Drop event at position="<<de->pos();
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
      qDebug()<<"Internal DRAG, move icons around...";
      // if icon positions are locked then abort
      if ( ! pDView->canMoveIcons() )
	return;
      /// \bug ......BUG/TODO........ WARNING: QListView::internalDrag() is NOT in offical Qt API !
      commonInternalDrag(supportedActions);
      return;
    }
    
    qDebug() << "Normal DRAG...";
    /*  // DEBUG:
	if (indexes.count() > 0)
	if (v->inherits("KLFLibDefListView")) {
	qDebug()<<"Got First index' rect: "
	<<qobject_cast<KLFLibDefListView*>(v)->rectForIndex(indexes[0]);
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

protected:
  KLFLibModel *pModel;
  KLFLibDefaultView *pDView;
  KLFLibDefaultView::ViewType pViewType;
  QPoint mousePressedContentsPos;

  virtual QModelIndexList commonSelectedIndexes() const = 0;
  virtual void commonInternalDrag(Qt::DropActions a) = 0;
  virtual QAbstractItemView *thisView() = 0;

  /** Returns contents position */
  virtual QPoint eventPos(QObject *object, QDragEnterEvent *event, int horoffset, int veroffset) {
    if (object == thisView()->viewport())
      return event->pos() + QPoint(horoffset, veroffset);
    if (object == thisView())
      return thisView()->viewport()->mapFromGlobal(thisView()->mapToGlobal(event->pos()))
	+ QPoint(horoffset, veroffset);
    return event->pos() + QPoint(horoffset, veroffset);
  }

  bool setTheModel(QAbstractItemModel *m) {
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
    : QTreeView(parent), KLFLibDefViewCommon(parent), pInEventFilter(false)
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


protected:
  virtual QModelIndexList commonSelectedIndexes() const { return selectedIndexes(); }
  virtual void commonInternalDrag(Qt::DropActions) {  }
  virtual QAbstractItemView *thisView() { return this; }

  virtual QPoint eventPos(QObject *object, QDragEnterEvent *event) {
    return KLFLibDefViewCommon::eventPos(object, event, horizontalOffset(), verticalOffset());
  }

  virtual void startDrag(Qt::DropActions supportedActions) {
    commonStartDrag(supportedActions);
  }

  bool pInEventFilter;
};

/** \internal */
class KLFLibDefListView : public QListView, public KLFLibDefViewCommon
{
  Q_OBJECT
public:
  KLFLibDefListView(KLFLibDefaultView *parent)
    : QListView(parent), KLFLibDefViewCommon(parent), pWantRelayout(true), pInEventFilter(false),
      pHasBeenPolished(false), pSavingIconPositions(false)
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
      qDebug()<<"KLFLibDefListView::eventFilter: Polish! iconpos="<<pDelayedSetIconPositions;
      if (!pDelayedSetIconPositions.isEmpty()) {
	// QListView wants to have its own doItemsLayout() function called first, before we start
	// fiddling with item positions.
	/** \bug .......... BUG/TODO/WARNING: QListView::doItemsLayout() is NOT in official API! */
	QListView::doItemsLayout();
	loadIconPositions(pDelayedSetIconPositions, true);
      } else {
	if (pWantRelayout)
	  forceRelayout(true); // relayout if wanted
      }
      pHasBeenPolished = true;
      eat = false;
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
    // walk all indexes in view
    QModelIndex it = pModel->walkNextIndex(QModelIndex());
    while (it.isValid()) {
      KLFLib::entryId eid = pModel->entryIdForIndex(it);
      if (eid != -1) {
	iconPositions[eid] = iconPosition(it);
      }
      it = pModel->walkNextIndex(it);
    }
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

    if ( ! pHasBeenPolished && ! forcenow ) {
      pDelayedSetIconPositions = iconPositions;
      qDebug()<<"KLFLibDefListView::loadIconPositions: delaying action!";
      return;
    }
    qDebug()<<"KLFLibDefListView::loadIconPositions: setting icon positions "<<iconPositions;
    QMap<KLFLib::entryId,QPoint>::const_iterator it;
    for (it = iconPositions.begin(); it != iconPositions.end(); ++it) {
      QModelIndex index = pModel->findEntryId(it.key());
      if (!index.isValid())
	continue;
      QPoint pos = *it;
      qDebug()<<KLF_FUNC_NAME<<": About to set single icon position..";
      setIconPosition(index, pos, true);
      qDebug()<<"Set single icon position OK.";
    }
    pDelayedSetIconPositions.clear();
  }


  virtual void moveSelectedIconsBy(const QPoint& delta) {
    int k;
    QModelIndexList sel = selectionModel()->selectedIndexes();
    for (k = 0; k < sel.size(); ++k) {
      QPoint newp = rectForIndex(sel[k]).topLeft() + delta;
      if (newp.x() < 0) newp.setX(0);
      if (newp.y() < 0) newp.setY(0);
      setIconPosition(sel[k], newp);
    }
  }

  void forceRelayout(bool isPolishing = false) {
    qDebug()<<"KLFLibDefListView::forceRelayout: isPolishing="<<isPolishing<<"; pWantRelayout="
	    <<pWantRelayout<<"; hasbeenpolished="<<pHasBeenPolished;
    bool wr = pWantRelayout;
    pWantRelayout = true;
    doItemsLayout(); // force re-layout
    if (!isPolishing && !wr) {
      // if we didn't by default want a relayout (ie. icon positions were controlled)
      saveIconPositions();
    }
    pWantRelayout = false;
  }

  QPoint iconPosition(const QModelIndex& index) const {
    return rectForIndex(index).topLeft();
  }
  void setIconPosition(const QModelIndex& index, const QPoint& pos, bool dontSave = false) {
    if (index.column() > 0)
      return;
    //    if (rectForIndex(index).topLeft() == pos)
    //      return;
    qDebug()<<"Functional setIconPosition("<<index<<","<<pos<<")";
    setPositionForIndex(pos, index);
    if (!dontSave)
      saveIconPosition(index);
    pWantRelayout = false;
  }
  
protected:
  virtual QModelIndexList commonSelectedIndexes() const { return selectedIndexes(); }
  virtual void commonInternalDrag(Qt::DropActions a) { internalDrag(a); }
  virtual QAbstractItemView *thisView() { return this; }

  virtual QPoint eventPos(QObject *object, QDragEnterEvent *event) {
    return KLFLibDefViewCommon::eventPos(object, event, horizontalOffset(), verticalOffset());
  }

  bool pInEventFilter;
  bool pHasBeenPolished;
  bool pSavingIconPositions;

  QMap<KLFLib::entryId, QPoint> pDelayedSetIconPositions;

  virtual void startDrag(Qt::DropActions supportedActions) {
    commonStartDrag(supportedActions);
  }
  virtual void doItemsLayout() {
    /** \bug ......BUG/TODO........ WARNING: QListView::doItemsLayout() is NOT in offical Qt API ! */
    qDebug()<<"doItemsLayout!";
    // Qt want its own doItemsLayout() to be called first (in case new indexes have appeared, or
    // old ones dissapeared)
    QMap<KLFLib::entryId,QPoint> bkpiconpos;
    if (!pWantRelayout) {
      bkpiconpos = allIconPositions(); // save current icon positions
      qDebug()<<"Got backup icon positions: "<<bkpiconpos;
    }
    // do qt's layout
    QListView::doItemsLayout();
    // but then we want to keep our layout if !pWantRelayout:
    if (!pWantRelayout)
      loadIconPositions(bkpiconpos);

    pWantRelayout = false;
    qDebug()<<"doItemsLayout finished!";
  } 
  
  bool pWantRelayout;

  void saveIconPositions() {
    qDebug()<<"saveIconPositions()";
    KLFLibModel *model = qobject_cast<KLFLibModel*>(this->model());
    // walk all indices, fill entryIdList
    QModelIndex index = model->walkNextIndex(QModelIndex());
    while (index.isValid()) {
      qDebug()<<"Walking index "<<index<<"; our model is "<<((QAbstractItemModel*)model)
	      <<"item's model is"<<((QAbstractItemModel*)index.model());
      saveIconPosition(index);
      index = model->walkNextIndex(index);
    }
    qDebug()<<"Done saving all icon positions.";
  }
  void saveIconPosition(const QModelIndex& index) {
    if (index.column()>0)
      return;
    qDebug()<<"saveIconPosition()";
    KLFLibModel *model = qobject_cast<KLFLibModel*>(this->model());
    KLFLibEntry edummy = index.data(KLFLibModel::FullEntryItemRole).value<KLFLibEntry>();
    int propId = edummy.setEntryProperty("IconView_IconPosition", rectForIndex(index).topLeft());
    pSavingIconPositions = true;
    model->changeEntries(QModelIndexList() << index, propId, edummy.property(propId)); 
    pSavingIconPositions = false;
    qDebug()<<"end of saveIconPosition()";
  }
};





#endif
