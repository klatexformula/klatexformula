/***************************************************************************
 *   file klfflowlayout.h
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

#ifndef KLFFLOWLAYOUT_H
#define KLFFLOWLAYOUT_H

#include <QLayout>
#include <QWidget>
#include <QLayoutItem>


class KLFFlowLayoutPrivate;


/** \brief A Layout that places widgets left to right, top to bottom.
 *
 */
class KLFFlowLayout : public QLayout
{
  Q_OBJECT

  Q_ENUMS(Flush)
  Q_PROPERTY(Flush flush READ flush WRITE setFlush) ;
  Q_PROPERTY(int horizontalSpacing READ horizontalSpacing WRITE setHorizontalSpacing) ;
  Q_PROPERTY(int verticalSpacing READ verticalSpacing WRITE setVerticalSpacing) ;

public:
  /** How to deal with too much space: */
  enum Flush {
    NoFlush = 0, //!< Give the extra space to the widgets to stretch, don't flush.
    FlushSparse, //!< Distribute the extra space inbetween the widgets to fill the line
    FlushBegin, //!< Leave all extra space at end of line
    FlushEnd //!< Leave all extra space at beginning of line
  };

  KLFFlowLayout(QWidget *parent, int margin = -1, int hspacing = -1, int vspacing = -1);
  virtual ~KLFFlowLayout();

  /** Add a QLayoutItem to the layout. Ownership of the object is taken by the layout: it will be deleted
   * by the layout. */
  virtual void addItem(QLayoutItem *item)
  { addItem(item, 0, 0); }
  virtual void addItem(QLayoutItem *item, int hstretch, int vstretch);
  virtual void addLayout(QLayout *l, int hstretch = 0, int vstretch = 0);
  virtual void addWidget(QWidget *w, int hstretch = 0, int vstretch = 0, Qt::Alignment align = 0);
  int horizontalSpacing() const;
  int verticalSpacing() const;
  Flush flush() const;
  virtual int count() const;
  virtual QLayoutItem *itemAt(int index) const;
  virtual QLayoutItem *takeAt(int index);
  virtual Qt::Orientations expandingDirections() const;
  virtual bool hasHeightForWidth() const;
  virtual int heightForWidth(int width) const;
  virtual QSize minimumSize() const;
  virtual QSize maximumSize() const;
  virtual QSize sizeHint() const;

  void setGeometry(const QRect &rect);

  virtual void invalidate();

  virtual bool event(QEvent *event);
  virtual bool eventFilter(QObject *obj, QEvent *event);
  
public slots:
  void clearAll(bool deleteItems = true);
  void setHorizontalSpacing(int spacing);
  void setVerticalSpacing(int spacing);
  void setFlush(Flush f);

private:

  KLFFlowLayoutPrivate *d;
};







#endif
