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

#include <QAbstractItemView>
#include <QTreeView>
#include <QListView>
#include <QMimeData>
#include <QDrag>

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


static void klf_common_start_drag(QAbstractItemView *v, Qt::DropActions supportedActions);

/** \internal */
class KLFLibDefTreeView : public QTreeView
{
  Q_OBJECT
public:
  KLFLibDefTreeView(KLFLibDefaultView *parent) : QTreeView(parent), pDView(parent) { }

  void simulateEvent(QEvent *e) {
    event(e);
  }

protected:
  virtual void startDrag(Qt::DropActions supportedActions) {
    klf_common_start_drag(this, supportedActions);
  }

  KLFLibDefaultView *pDView;

  friend void klf_common_start_drag(QAbstractItemView *v, Qt::DropActions supportedActions);
};

/** \internal */
class KLFLibDefListView : public QListView
{
  Q_OBJECT
public:
  KLFLibDefListView(KLFLibDefaultView *parent) : QListView(parent), pDView(parent) { }

  void simulateEvent(QEvent *e) {
    event(e);
  }

  void moveSelectedBy(const QPoint& delta) {
    int k;
    QModelIndexList sel = selectionModel()->selectedIndexes();
    for (k = 0; k < sel.size(); ++k) {
      QPoint newp = rectForIndex(sel[k]).topLeft() + delta;
      if (newp.x() < 0) newp.setX(0);
      if (newp.y() < 0) newp.setY(0);
      setPositionForIndex(newp, sel[k]);
    }
  }
  
protected:
  virtual void startDrag(Qt::DropActions supportedActions) {
    klf_common_start_drag(this, supportedActions);
  }

  KLFLibDefaultView *pDView;

  friend void klf_common_start_drag(QAbstractItemView *v, Qt::DropActions supportedActions);
};



// -------------------------------

static void klf_common_start_drag(QAbstractItemView *v, Qt::DropActions supportedActions)
{
  // largely based on Qt's QAbstractItemView source in src/gui/itemviews/qabstractitemview.cpp
  KLFLibModel *model = (KLFLibModel*)v->model();
  if (model == NULL)
    return;

  QModelIndexList indexes;
  // the following inheritance test and cast is needed for the frend attriubte
  // to be taken into account (yes, we're not a friend of QAbstractItemView)
  if (v->inherits("KLFLibDefListView"))
    indexes = qobject_cast<KLFLibDefListView*>(v)->selectedIndexes();
  else if (v->inherits("KLFLibDefTreeView"))
    indexes = qobject_cast<KLFLibDefTreeView*>(v)->selectedIndexes();


  if (v->inherits("KLFLibDefListView")) {
    KLFLibDefListView *lv = qobject_cast<KLFLibDefListView*>(v);
    if (lv->pDView->viewType() == KLFLibDefaultView::IconView) {
      // icon view -> move icons around
      qDebug()<<"Internal DRAG!";
      /// \bug ......BUG/TODO........ WARNING: NOT in offical Qt API !
      lv->internalDrag(supportedActions);
      return;
    }
  }

  qDebug() << "Normal DRAG";

  /*  // DEBUG:
      if (indexes.count() > 0)
      if (v->inherits("KLFLibDefListView")) {
      qDebug()<<"Got First index' rect: "
      <<qobject_cast<KLFLibDefListView*>(v)->rectForIndex(indexes[0]);
      qobject_cast<KLFLibDefListView*>(v)->setPositionForIndex(QPoint(200,100), indexes[0]);
      }
  */

  if (indexes.count() > 0) {
    QMimeData *data = model->mimeData(indexes);
    if (data == NULL)
      return;
    QDrag *drag = new QDrag(v);
    drag->setMimeData(data);
    QPixmap pix = QPixmap::fromImage(model->dragImage(indexes));
    drag->setPixmap(pix);
    drag->setHotSpot(QPoint(0,0));
    Qt::DropAction defaultDropAction = Qt::IgnoreAction;
    
    if (supportedActions & Qt::CopyAction && v->dragDropMode() != QAbstractItemView::InternalMove)
      defaultDropAction = Qt::CopyAction;
    
    drag->exec(supportedActions, defaultDropAction);
  }
}



#endif
