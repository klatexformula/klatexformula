/***************************************************************************
 *   file klfflowlayout.cpp
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


#include <math.h>

#include <QHash>
#include <QWidget>
#include <QEvent>
#include <QResizeEvent>
#include <QMoveEvent>
#include <QApplication>
#include <QStyle>

#include <klfdefs.h>

#include "klfflowlayout.h"



struct KLFFlowLayoutItem : public QLayoutItem
{
  KLFFlowLayoutItem(QLayoutItem *li, Qt::Alignment a = 0)
    : QLayoutItem(a), item(li), hstretch(0), vstretch(0)
  {
    KLF_ASSERT_NOT_NULL(item, "item is NULL! Expect crash!", ;) ;
  }
  virtual ~KLFFlowLayoutItem();

  virtual Qt::Orientations expandingDirections() const
  {
    return item->expandingDirections();
  }
  virtual QRect geometry() const
  {
    klfDbg("geometry") ;
    return item->geometry();
  }
  virtual bool hasHeightForWidth() const
  {
    return item->hasHeightForWidth();
  }
  virtual int heightForWidth(int w) const
  {
    return item->heightForWidth(w);
  }
  virtual void invalidate()
  {
    item->invalidate();
  }
  virtual bool isEmpty() const
  {
    return item->isEmpty();
  }
  virtual QLayout *layout()
  {
    return item->layout();
  }
  virtual QSize maximumSize() const
  {
    return item->maximumSize();
  }
  virtual int minimumHeightForWidth(int w) const
  {
    return item->minimumHeightForWidth(w);
  }
  virtual QSize minimumSize() const
  {
    return item->minimumSize();
  }
  virtual void setGeometry(const QRect& r)
  {
    klfDbg("item/widget="<<item->widget()<<"; setGeom: "<<r) ;
    item->setGeometry(r);
  }
  virtual QSize sizeHint() const
  {
    return item->sizeHint();
  }
  virtual QSpacerItem * spacerItem()
  {
    return item->spacerItem();
  }
  virtual QWidget * widget()
  {
    //    klfDbg("widget="<<item->widget()) ;
    return item->widget();
  }

  // ---

  QLayoutItem *item;

  int hstretch;
  int vstretch;
};


KLFFlowLayoutItem::~KLFFlowLayoutItem()
{
}

struct KLFFlowLayoutPrivate
{
  /** Back-pointer to the main object */
  KLFFlowLayout * K;
  /** Constructor */
  KLFFlowLayoutPrivate(KLFFlowLayout *f)
  {
    K = f;
    mainLayout = new QBoxLayout(QBoxLayout::TopToBottom, NULL);
    mainLayout->setContentsMargins(0,0,0,0);
    hspc = vspc = -1;
    flush = KLFFlowLayout::NoFlush;
    geom = effectiveGeom = QRect(0, 0, 640, 480); // arbitrary... (?)
    marginsSize = QSize(0,0);
    hfw_w = -1;
    hfw_h = -1;
    min_size = QSize(0,0);

    dirty = true;
  }

  /** Keep a list of all items */
  QList<KLFFlowLayoutItem*> items;

  /** Do we need to recalculate the layout? */
  bool dirty;
  /** Our sub-layouts, one horizontal box per line. */
  QList<QBoxLayout*> layoutLines;

  QBoxLayout *mainLayout;

  int hspc;
  int vspc;
  KLFFlowLayout::Flush flush;

  QRect geom;
  QRect effectiveGeom;
  QSize marginsSize;
  int hfw_w;
  int hfw_h;
  QSize min_size;
  QSize size_hint;
  QSize max_size;

  // -------


  typedef QList<KLFFlowLayoutItem*> ItemLine;

  void calc()
  {
    if (!dirty)
      return;

    doLayout();
  }

  void removeItemFromSubLayouts(KLFFlowLayoutItem* fi)
  {
    int k, n;
    QLayoutItem *item;
    for (k = 0; k < layoutLines.size(); ++k) {
      n = 0;
      while ((item = layoutLines[k]->itemAt(n)) != NULL) {
	if (item == fi) {
	  layoutLines[k]->takeAt(n);
	  return;
	}
	n++;
      }
    }
  }

  void clean()
  {
    int k;
    for (k = 0; k < layoutLines.size(); ++k) {
      //      klfDbg("removing line #"<<k) ;
      // empty each layout line
      QLayoutItem *item;
      while ((item = layoutLines[k]->takeAt(0)) != NULL) {
	KLFFlowLayoutItem *fi = dynamic_cast<KLFFlowLayoutItem*>(item);
	if (fi == NULL) {
	  // this is not a KLFFlowLayoutItem -> delete it, we don't need it
	  delete item;
	}
      }
      mainLayout->removeItem(layoutLines[k]);
      delete layoutLines[k];
      layoutLines[k] = NULL;
    }
    layoutLines.clear();
    dirty = true;
  }

  struct CalcData
  {
    QSize minsize;
    QSize sizehint;
    QSize maxsize;
    int height;
  };

  QList<ItemLine> calcLines(CalcData *data = NULL)
  {
    QList<ItemLine> lines;

    int maxminwidth = 0;
    int summinwidth = 0;
    int maxwidth = 0;
    int maxminheight = 0;
    int summinheight = 0;
    int maxheight = 0;
    int shwidth = 0;

    ItemLine line;

    QStyle *style = NULL;
    QWidget *pwidget = K->parentWidget();
    if (pwidget != NULL)
      style = pwidget->style();
    if (style == NULL)
      style = qApp->style();

    int x = 0;
    int fitwidth = effectiveGeom.width();
    int height = 0;
    int thislheight = 0;

    KLFFlowLayoutItem *item;
    KLFFlowLayoutItem *prevItem = NULL;
    int k;
    for (k = 0; k < items.size(); ++k) {
      item = items[k];

      QSize mins = item->minimumSize();
      QSize maxs = item->maximumSize();
      QSize sh = item->sizeHint();

      if (item->isEmpty())
	continue; // skip empty items

      QSizePolicy::ControlTypes t = QSizePolicy::DefaultType;
      if (prevItem != NULL)
	t = prevItem->controlTypes();

      int spaceX = hspc;
      if (spaceX == -1)
	spaceX = style->combinedLayoutSpacing(t, item->controlTypes(), Qt::Horizontal);

      int spaceY = vspc;
      if (spaceY == -1)
	spaceY = style->combinedLayoutSpacing(t, item->controlTypes(), Qt::Vertical);

      if (x + sh.width() > fitwidth) {
	lines << line; // flush this line
	shwidth = qMax(shwidth, x);
	height += spaceY + thislheight;
	thislheight = 0;
	line.clear(); // clear line buffer

	x = 0;
      }
      int nextX = x + sh.width() + spaceX;

      line << item;
      x = nextX;
      maxminwidth = qMax(mins.width(), maxminwidth);
      maxminheight = qMax(mins.height(), maxminheight);
      summinwidth += mins.width();
      summinheight += mins.height();
      if (maxwidth < (int)0x7fffffff)
	maxwidth += maxs.width() + spaceX;
      if (maxheight < (int)0x7fffffff)
	maxheight += maxs.height() + spaceY;
      thislheight = qMax(item->sizeHint().height(), thislheight);
      prevItem = item;
    }

    // and flush last line
    height += thislheight;
    lines << line;

    if (data != NULL) {
      data->height = height;

      //       int area = qMax(summinwidth * maxminheight, maxminwidth * summinheight);
      //       qreal ratio = (qreal)effectiveGeom.width() / effectiveGeom.height();
      //       int minx = (int)sqrt(area / ratio);
      //       data->minsize = QSize(minx, area/minx);
      //--
      //      if (lines.count() <= 3)
      data->minsize = QSize(maxminwidth, height);
      data->sizehint = QSize(shwidth, height);
      //      else
      //	data->minsize = QSize(maxminwidth, (int)(height*0.50));

      data->maxsize = QSize(maxwidth, maxheight);
    }

    return lines;
  }

  void doLayout()
  {
    KLF_DEBUG_TIME_BLOCK(KLF_FUNC_NAME) ;

    if (!geom.isValid()) {
      klfDbg("geom is not valid yet, returning.") ;
      return;
    }

    // remove all items from all our sub-layouts and redistribute them...
    clean();

    int left, top, right, bottom;
    K->getContentsMargins(&left, &top, &right, &bottom);
    effectiveGeom = geom.adjusted(+left, +top, -right, -bottom);
    marginsSize = QSize(left+right, top+bottom);

    klfDbg("geom = "<<geom<<"; mainlayout/setgeom, effGeom="<<effectiveGeom) ;
    
    CalcData sizedat;

    QList<ItemLine> lines = calcLines(&sizedat);

    klfDbg("calculated lines. minsize="<<sizedat.minsize<<"; maxsize="<<sizedat.maxsize<<", height="
	   <<sizedat.height) ;

    int k, n, i;
    int linevstretch;
    for (k = 0; k < lines.size(); ++k) {
      //      klfDbg("adding line #"<<k<<", with "<<lines[k].size()<<" items in the line.") ;
      QBoxLayout *linelyt = new QBoxLayout(QBoxLayout::LeftToRight, NULL);
      linelyt->setSpacing(hspc);
      linevstretch = 0;
      if (flush == KLFFlowLayout::FlushEnd) {
	// start with a spacer
	linelyt->addStretch(0x7fffffff);
      }
      for (n = 0; n < lines[k].size(); ++n) {
	//	klfDbg("adding item ["<<k<<"]["<<n<<"], item="<<lines[k][n]->item) ;
	Qt::Alignment a = lines[k][n]->alignment();
	if (flush == KLFFlowLayout::NoFlush) {
	  // remove horizontal alignemnt flags
	  a = (a & ~Qt::AlignHorizontal_Mask);
	} else if (flush == KLFFlowLayout::FlushSparse) {
	  a |= Qt::AlignLeft;
	}
	lines[k][n]->item->setAlignment(a); // correct the alignment flags
	linelyt->addItem(lines[k][n]);
	for (i = 0; i < linelyt->count() && linelyt->itemAt(i) != lines[k][n]; ++i)
	  ;
	if (i < linelyt->count())
	  linelyt->setStretch(i, lines[k][n]->hstretch);
	linevstretch = qMax(linevstretch, lines[k][n]->vstretch);
      }
      if (flush == KLFFlowLayout::FlushBegin) {
	// end with a spacer
	linelyt->addStretch(0x7fffffff);
      }
      mainLayout->addLayout(linelyt, linevstretch);
      layoutLines << linelyt;
    }

    hfw_w = geom.width();
    hfw_h = sizedat.height;

    min_size = sizedat.minsize;
    size_hint = sizedat.sizehint;
    max_size = sizedat.maxsize;

    //    QWidget *pwidget = K->parentWidget();
    //    if (pwidget != NULL)
    //      pwidget->updateGeometry();
    //    mainLayout->invalidate();
    //    mainLayout->update();
    //    mainLayout->activate();
    //    if (pwidget != NULL && K->sizeConstraint() == QLayout::SetDefaultConstraint)
    //      pwidget->setMinimumSize(QSize());
    if (K->sizeConstraint() == QLayout::SetDefaultConstraint)
      K->setSizeConstraint(QLayout::SetMinimumSize);
    K->update();

    dirty = false;
  }
};



KLFFlowLayout::KLFFlowLayout(QWidget *parent, int margin, int hspacing, int vspacing)
  : QLayout(parent)
{
  d = new KLFFlowLayoutPrivate(this);
  addChildLayout(d->mainLayout);
  setContentsMargins(margin, margin, margin, margin);
  setSpacing(-1);
  d->hspc = hspacing;
  d->vspc = vspacing;
  d->mainLayout->setSpacing(d->vspc);
}

KLFFlowLayout::~KLFFlowLayout()
{
  delete d->mainLayout;
  delete d;
}

bool KLFFlowLayout::event(QEvent *event)
{
  //   if (event->type() == QEvent::ParentChange) {
  //     d->dirty = true;
  //     d->mainLayout->setParent(parentWidget());
  //     // don't eat the event, propagate it to parent layout object
  //   }

  return QLayout::event(event);
}

bool KLFFlowLayout::eventFilter(QObject */*obj*/, QEvent */*event*/)
{
  /*
  QWidget * w = qobject_cast<QWidget*>(obj);
  if (w != NULL && w->parentWidget() == parentWidget()) {
    if (event->type() == QEvent::Resize) {
      klfDbg("child "<<w<<" got new size "<<((QResizeEvent*)event)->size()) ;
    }
    if (event->type() == QEvent::Move) {
      klfDbg("child "<<w<<" moved to "<<((QMoveEvent*)event)->pos()) ;
    }
  }
  */
  return false;
}

void KLFFlowLayout::addItem(QLayoutItem *item, int hstretch, int vstretch)
{
  invalidate();
  KLFFlowLayoutItem *fi = new KLFFlowLayoutItem(item, item->alignment());
  fi->hstretch = hstretch;
  fi->vstretch = vstretch;
  d->items << fi;
}
void KLFFlowLayout::addWidget(QWidget *w, int hstretch, int vstretch, Qt::Alignment align)
{
  addChildWidget(w);

  w->installEventFilter(this);

  QWidgetItem *wi = new QWidgetItem(w);
  wi->setAlignment(align);
  addItem(wi, hstretch, vstretch);
}
void KLFFlowLayout::addLayout(QLayout *l, int hstretch, int vstretch)
{
  addChildLayout(l);
  addItem(l, hstretch, vstretch);
}

int KLFFlowLayout::horizontalSpacing() const
{
  return d->hspc;
}
void KLFFlowLayout::setHorizontalSpacing(int spacing)
{
  invalidate();
  d->hspc = spacing;
}
int KLFFlowLayout::verticalSpacing() const
{
  return d->vspc;
}
void KLFFlowLayout::setVerticalSpacing(int spacing)
{
  invalidate();
  d->vspc = spacing;
  d->mainLayout->setSpacing(d->vspc);
}
KLFFlowLayout::Flush KLFFlowLayout::flush() const
{
  return d->flush;
}
void KLFFlowLayout::setFlush(Flush f)
{
  invalidate();
  d->flush = f;
}
int KLFFlowLayout::count() const
{
  return d->items.size();
}
QLayoutItem *KLFFlowLayout::itemAt(int index) const
{
  // ### this is not an error, it is documented in Qt API...
  //  KLF_ASSERT_CONDITION(index >= 0 && index < d->items.size(),
  //		       "index "<<index<<" out of bounds [0,"<<d->items.size()-1<<"] !",
  //		       return NULL; ) ;
  if (index < 0 || index >= d->items.size())
    return NULL;

  return d->items[index]->item;
}
QLayoutItem *KLFFlowLayout::takeAt(int index)
{
  // ### this is not an error, it is documented in Qt API...
  //  KLF_ASSERT_CONDITION(index >= 0 && index < d->items.size(),
  //		       "index "<<index<<" out of bounds [0,"<<d->items.size()-1<<"] !",
  //		       return NULL; ) ;
  if (index < 0 || index >= d->items.size())
    return NULL;

  KLFFlowLayoutItem *fi = d->items.takeAt(index);
  d->removeItemFromSubLayouts(fi);
  QLayoutItem *item = fi->item;
  delete fi;
  return item;
}

Qt::Orientations KLFFlowLayout::expandingDirections() const
{
  d->calc();
  return  Qt::Horizontal  |  d->mainLayout->expandingDirections();
}
bool KLFFlowLayout::hasHeightForWidth() const
{
  return true;
}
int KLFFlowLayout::heightForWidth(int width) const
{
  if (d->hfw_w != width) {
    d->hfw_w = width;
    d->dirty = true;
    d->calc();
  }
  return d->hfw_h;
}
QSize KLFFlowLayout::minimumSize() const
{
  d->calc();
  //  QSize s = d->mainLayout->minimumSize() + d->marginsSize;
  QSize s = d->min_size + d->marginsSize;
  klfDbg("minimumSize is "<<s) ;
  return s;
}
QSize KLFFlowLayout::maximumSize() const
{
  d->calc();
  QSize s = d->max_size;
  klfDbg("maximumSize is "<<s) ;
  return s.expandedTo(QSize(200,200));
}
void KLFFlowLayout::setGeometry(const QRect &rect)
{
  KLF_DEBUG_BLOCK(KLF_FUNC_NAME) ;
  klfDbg("rect="<<rect) ;
  if (d->geom != rect) {
    invalidate();
    d->geom = rect;
  }
  QLayout::setGeometry(rect);
  d->calc();
  d->mainLayout->setGeometry(d->effectiveGeom);
}
QSize KLFFlowLayout::sizeHint() const
{
  d->calc();
  QSize s = d->size_hint + d->marginsSize;//d->mainLayout->sizeHint() + d->marginsSize;
  klfDbg("sizeHint is "<<s) ;
  return s;
}

void KLFFlowLayout::invalidate()
{
  //  klfDbg("invalidate") ;
  d->dirty = true;
  QLayout::invalidate();
  d->mainLayout->invalidate();
}


