/***************************************************************************
 *   file klfsidewidget.cpp
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


#include <QtGlobal>
#include <QLayout>
#include <QEvent>
#include <QResizeEvent>

#include <klfdefs.h>

#include "klfsidewidget.h"




struct KLFSideWidgetManagerPrivate
{
  KLF_PRIVATE_HEAD(KLFSideWidgetManager)
  {
  }

  // for future use....
};


KLFSideWidgetManager::KLFSideWidgetManager(QWidget *sideWidget, QObject *parent)
  : QObject(parent)
{
  KLF_INIT_PRIVATE(KLFSideWidgetManager) ;

  pSideWidget = NULL;
  setSideWidget(sideWidget);
}

KLFSideWidgetManager::~KLFSideWidgetManager()
{
  KLF_DELETE_PRIVATE ;
}

void KLFSideWidgetManager::setSideWidget(QWidget *sideWidget)
{
  QWidget *oldw = pSideWidget;

  if (pSideWidget != NULL) {
    pSideWidget->hide();
  }
  pSideWidget = sideWidget;
  if (pSideWidget != NULL) {
    pSideWidget->hide();
  }

  newSideWidgetSet(oldw, pSideWidget);
}



// ------------------




struct KLFShowHideSideWidgetManagerPrivate
{
  KLF_PRIVATE_HEAD(KLFShowHideSideWidgetManager)
  {
    infunction = false;
    oldParent = NULL;
    msize = QSize(0, 0);
    orientation = Qt::Horizontal;
    calcSpacing = 6;
  }

  bool infunction;
  QWidget *oldParent;
  QSize msize;

  Qt::Orientation orientation;
  int calcSpacing;

  // --

  void setupNewParent()
  {
    KLF_ASSERT_NOT_NULL(K->sideWidget(), "sideWidget is NULL!", return; ) ;
    QWidget * pw = K->sideWidget()->parentWidget();
    if (oldParent != NULL)
      oldParent->removeEventFilter(K);
    if (pw == NULL) {
      msize = QSize(-1, -1);
      return;
    }
    msize = pw->size();
    pw->installEventFilter(K);
  }
};


KLFShowHideSideWidgetManager::KLFShowHideSideWidgetManager(QWidget *sideWidget, QObject *parent)
  : KLFSideWidgetManager(sideWidget, parent)
{
  KLF_INIT_PRIVATE(KLFShowHideSideWidgetManager) ;

  if (parent != NULL)
    d->setupNewParent();
}

KLFShowHideSideWidgetManager::~KLFShowHideSideWidgetManager()
{
  KLF_DELETE_PRIVATE ;
}

// protected
void KLFShowHideSideWidgetManager::newSideWidgetSet(QWidget *oldw, QWidget *neww)
{
  if (oldw != NULL)
    oldw->removeEventFilter(this);
  if (neww != NULL)
    neww->installEventFilter(this);
}



bool KLFShowHideSideWidgetManager::eventFilter(QObject *obj, QEvent *event)
{
  if (sideWidget() != NULL) {
    QWidget * pw = sideWidget()->parentWidget();
    if (pw != NULL && obj == pw) {
      if (event->type() == QEvent::Resize) {
	QResizeEvent *re = (QResizeEvent*) event;
	klfDbg("resize event, new size="<<re->size()<<", old size="<<re->oldSize()) ;
	if (sideWidget()->isVisible() && !d->infunction) {
	  // only relevant if we are visible and not ourselves resizing the widget
	  klfDbg("Readjusting inner widget size") ;
	  d->msize += re->size() - re->oldSize();
	}
      }
    }
    if (obj == sideWidget()) {
      if (event->type() == QEvent::ParentAboutToChange) {
	d->oldParent = sideWidget()->parentWidget();
      } else if (event->type() == QEvent::ParentChange) {
	d->setupNewParent();
      }
    }
  }
  
  return QObject::eventFilter(obj, event);
}

bool KLFShowHideSideWidgetManager::event(QEvent *event)
{
  return QObject::event(event);
}


void KLFShowHideSideWidgetManager::showSideWidget(bool show)
{
  KLF_DEBUG_BLOCK(KLF_FUNC_NAME) ;
  
  if (show == sideWidget()->isVisible())
    return;

  klfDbg("show="<<show) ;
  QWidget *pw = sideWidget()->parentWidget();

  KLF_ASSERT_NOT_NULL(pw, "Parent Widget of Side Widget is NULL!", return; ) ;

  QSize newSize;
  if (show) {
    d->msize = pw->size();
    klfDbg("Store inner widget size as "<<d->msize) ;
    newSize = d->msize;
    if (d->orientation & Qt::Horizontal)
      newSize += QSize(d->calcSpacing + sideWidget()->sizeHint().width(), 0);
    if (d->orientation & Qt::Vertical)
      newSize += QSize(0, d->calcSpacing + sideWidget()->sizeHint().height());
  } else {
    newSize = d->msize;
  }
  sideWidget()->setVisible(show);

  d->infunction = true;
  klfDbg("newSize is "<<newSize) ;
  QMetaObject::invokeMethod(this, "resizeParentWidget", Qt::QueuedConnection, Q_ARG(QSize, newSize));
}

/*
void KLFShowHideSideWidgetManager::setVisible(bool on)
{
  KLF_DEBUG_BLOCK(KLF_FUNC_NAME) ;

  if (on == isVisible())
    return;

  klfDbg("on="<<on) ;
  QWidget *pw = parentWidget();
  QSize newSize;

  if (pw != NULL) {
    if (on) {
      d->msize = pw->size();
      klfDbg("Store inner widget size as "<<d->msize) ;
      newSize = d->msize;
      if (d->orientation & Qt::Horizontal)
	newSize += QSize(d->calcSpacing + sizeHint().width(), 0);
      if (d->orientation & Qt::Vertical)
	newSize += QSize(0, d->calcSpacing + sizeHint().height());
    } else {
      newSize = d->msize;
    }
  }
  klfDbg("newSize is "<<newSize) ;
  QWidget::setVisible(on);
  if (pw != NULL) {
    d->infunction = true;
    QMetaObject::invokeMethod(this, "resizeParentWidget", Qt::QueuedConnection, Q_ARG(QSize, newSize));
  }
}
*/

void KLFShowHideSideWidgetManager::resizeParentWidget(const QSize& size)
{
  QWidget * sw = sideWidget();
  KLF_ASSERT_NOT_NULL(sw, "Side Widget is NULL!", return; ) ;
  KLF_ASSERT_NOT_NULL(sw->parentWidget(), "Side Widget is NULL!", return; ) ;

  sw->parentWidget()->resize(size);
  d->infunction = false;
}



#undef KLF_DEFINE_PROPERTY_GET
#undef KLF_DEFINE_PROPERTY_GETSET

#define KLF_DEFINE_PROPERTY_GET(ClassName, type, prop, Prop)	\
  type ClassName::prop() const { return d->prop; }

#define KLF_DEFINE_PROPERTY_GETSET(ClassName, type, prop, Prop)	       \
  KLF_DEFINE_PROPERTY_GET(ClassName, type, prop, Prop)	       \
  void ClassName::set##Prop(const type& value) { d->prop = value; }


KLF_DEFINE_PROPERTY_GETSET(KLFShowHideSideWidgetManager, Qt::Orientation, orientation, Orientation) ;

KLF_DEFINE_PROPERTY_GETSET(KLFShowHideSideWidgetManager, int, calcSpacing, CalcSpacing) ;

