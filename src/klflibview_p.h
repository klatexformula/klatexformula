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


class KLFLibDefTreeView;


inline QPointF sizeToPointF(const QSizeF& s) { return QPointF(s.width(), s.height()); }
inline QSizeF pointToSizeF(const QPointF& p) { return QSizeF(p.x(), p.y()); }

static QImage transparentify_image(const QImage& img, qreal factor)
{
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


/** \internal */
class KLFLibDefTreeView : public QTreeView
{
public:
  KLFLibDefTreeView(QWidget *parent) : QTreeView(parent) { }

protected:
  void startDrag(Qt::DropActions supportedActions);
};

/** \internal */
class KLFLibDefListView : public QListView
{
public:
  KLFLibDefListView(QWidget *parent) : QListView(parent) { }
  
protected:
  void startDrag(Qt::DropActions supportedActions);
};





// -------------------------------

void KLFLibDefTreeView::startDrag(Qt::DropActions supportedActions)
{
  // largely based on Qt's QAbstractItemView source in src/gui/itemviews/qabstractitemview.cpp
  KLFLibModel *model = (KLFLibModel*)this->model();
  if (model == NULL)
    return;
  QModelIndexList indexes = selectedIndexes();
  if (indexes.count() > 0) {
    QMimeData *data = model->mimeData(indexes);
    if (data == NULL)
      return;
    QDrag *drag = new QDrag(this);
    drag->setMimeData(data);
    QPixmap pix = QPixmap::fromImage(model->dragImage(indexes));
    drag->setPixmap(pix);
    drag->setHotSpot(QPoint(0,0));
    Qt::DropAction defaultDropAction = Qt::IgnoreAction;
    
    if (supportedActions & Qt::CopyAction && dragDropMode() != QAbstractItemView::InternalMove)
      defaultDropAction = Qt::CopyAction;
    
    drag->exec(supportedActions, defaultDropAction);
  }
}

void KLFLibDefListView::startDrag(Qt::DropActions supportedActions)
{
  // largely based on Qt's QAbstractItemView source in src/gui/itemviews/qabstractitemview.cpp
  QAbstractItemModel *model = this->model();
  if (model == NULL)
    return;
  QModelIndexList indexes = selectedIndexes();
  if (indexes.count() > 0) {
    QMimeData *data = model->mimeData(indexes);
    if (data == NULL)
      return;
    QDrag *drag = new QDrag(this);
    drag->setMimeData(data);
    Qt::DropAction defaultDropAction = Qt::IgnoreAction;
    
    if (supportedActions & Qt::CopyAction && dragDropMode() != QAbstractItemView::InternalMove)
      defaultDropAction = Qt::CopyAction;
    
    drag->exec(supportedActions, defaultDropAction);
  }
}



#endif
