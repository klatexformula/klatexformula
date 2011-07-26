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
#include <QVBoxLayout>

#include <klfdefs.h>

#include "klfsidewidget.h"




struct KLFSideWidgetManagerBasePrivate
{
  KLF_PRIVATE_HEAD(KLFSideWidgetManagerBase)
  {
  }

  // for future use....
};


KLFSideWidgetManagerBase::KLFSideWidgetManagerBase(QWidget *parentWidget, QWidget *sideWidget)
  : QObject(parentWidget)
{
  KLF_INIT_PRIVATE(KLFSideWidgetManagerBase) ;

  pSideWidget = NULL;
  pParentWidget = NULL;
  setOurParentWidget(parentWidget);
  setSideWidget(sideWidget);
}

KLFSideWidgetManagerBase::~KLFSideWidgetManagerBase()
{
  KLF_DELETE_PRIVATE ;
}

void KLFSideWidgetManagerBase::setOurParentWidget(QWidget *p)
{
  QWidget *oldpw = pParentWidget;
  pParentWidget = p;
  newParentWidgetSet(oldpw, pParentWidget);
}

void KLFSideWidgetManagerBase::setSideWidget(QWidget *sideWidget)
{
  QWidget *oldw = pSideWidget;
  pSideWidget = sideWidget;
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
};


KLFShowHideSideWidgetManager::KLFShowHideSideWidgetManager(QWidget *parentWidget, QWidget *sideWidget)
  : KLFSideWidgetManagerBase(parentWidget, sideWidget)
{
  KLF_INIT_PRIVATE(KLFShowHideSideWidgetManager) ;
}

KLFShowHideSideWidgetManager::~KLFShowHideSideWidgetManager()
{
  KLF_DELETE_PRIVATE ;
}

// protected
void KLFShowHideSideWidgetManager::newSideWidgetSet(QWidget *oldw, QWidget *neww)
{
  KLF_ASSERT_CONDITION(ourParentWidget() == neww,
		   "Adding a widget that is not a child of our 'parent widget' ! Correcting parent.",
		   setOurParentWidget(neww); ) ;

  if (oldw != NULL) {
    oldw->removeEventFilter(this);
    oldw->hide();
  }
  if (neww != NULL) {
    neww->hide();
    neww->installEventFilter(this);
  }
}

// protected
void KLFShowHideSideWidgetManager::newParentWidgetSet(QWidget *oldParent, QWidget *pw)
{
  if (d->oldParent != NULL)
    d->oldParent->removeEventFilter(this);
  if (pw == NULL) {
    d->msize = QSize(-1, -1);
    return;
  }
  d->msize = pw->size();
  pw->installEventFilter(this);
}


bool KLFShowHideSideWidgetManager::eventFilter(QObject *obj, QEvent *event)
{
  if (sideWidget() != NULL) {
    KLF_ASSERT_CONDITION(ourParentWidget() == sideWidget()->parentWidget(),
		     "We have a side widget that is not a child of our 'parent widget' ! Correcting parent.",
		     setOurParentWidget(sideWidget()->parentWidget()); ) ;
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
	setOurParentWidget(pw);
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
  
  KLF_ASSERT_NOT_NULL(sideWidget(), "Side Widget is NULL!", return; ) ;

  if (show == sideWidget()->isVisible())
    return;

  klfDbg("show="<<show) ;
  QWidget *pw = sideWidget()->parentWidget();

  KLF_ASSERT_NOT_NULL(pw, "Parent Widget of Side Widget is NULL!", return; ) ;
  KLF_ASSERT_CONDITION(ourParentWidget() == sideWidget()->parentWidget(),
		   "We have a side widget that is not a child of our 'parent widget' ! Correcting parent.",
		   setOurParentWidget(sideWidget()->parentWidget()); ) ;

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


void KLFShowHideSideWidgetManager::resizeParentWidget(const QSize& size)
{
  QWidget * sw = sideWidget();
  KLF_ASSERT_NOT_NULL(sw, "Side Widget is NULL!", return; ) ;
  KLF_ASSERT_NOT_NULL(sw->parentWidget(), "Side Widget is NULL!", return; ) ;

  sw->parentWidget()->resize(size);
  d->infunction = false;
}


KLF_DEFINE_PROPERTY_GETSET(KLFShowHideSideWidgetManager, Qt::Orientation, orientation, Orientation) ;

KLF_DEFINE_PROPERTY_GETSET(KLFShowHideSideWidgetManager, int, calcSpacing, CalcSpacing) ;


// ------------------




struct KLFDrawerSideWidgetManagerPrivate
{
  KLF_PRIVATE_HEAD(KLFDrawerSideWidgetManager)
  {
    dwidget = NULL;
  }

  QWidget *dwidget;
  QBoxLayout *dlayout;
};

KLFDrawerSideWidgetManager::KLFDrawerSideWidgetManager(QWidget *pw, QWidget *sideWidget)
  : KLFSideWidgetManagerBase(pw, sideWidget)
{
  KLF_INIT_PRIVATE(KLFDrawerSideWidgetManager) ;

  d->dwidget = new QWidget(ourParentWidget(), Qt::Drawer);
  d->dlayout = new QVBoxLayout(d->dwidget);
  d->dlayout->setContentsMargins(0,0,0,0);
  d->dlayout->setSpacing(0);
}

KLFDrawerSideWidgetManager::~KLFDrawerSideWidgetManager()
{
  KLF_DELETE_PRIVATE ;
}

// protected
void KLFDrawerSideWidgetManager::newSideWidgetSet(QWidget *oldw, QWidget *neww)
{
  if (oldw != NULL) {
    d->dlayout->removeWidget(oldw);
    //    oldw->hide(); // setParent() automatically hides the widget
    oldw->setParent(NULL);
  }
  if (neww != NULL) {
    neww->setParent(d->dwidget);
    d->dlayout->addWidget(neww);
    neww->show();
  }
}

// protected
void KLFDrawerSideWidgetManager::newParentWidgetSet(QWidget *, QWidget *newWidget)
{
  d->dwidget->setParent(newWidget);
}


bool KLFDrawerSideWidgetManager::eventFilter(QObject *obj, QEvent *event)
{
  return QObject::eventFilter(obj, event);
}

bool KLFDrawerSideWidgetManager::event(QEvent *event)
{
  return QObject::event(event);
}


void KLFDrawerSideWidgetManager::showSideWidget(bool show)
{
#ifdef Q_WS_MAC
  //  extern bool klf_qt_mac_set_drawer_preferred_edge(QWidget *w, Qt::DockWidgetArea where);
#endif

  KLF_DEBUG_BLOCK(KLF_FUNC_NAME) ;

  KLF_ASSERT_NOT_NULL(sideWidget(), "Side Widget is NULL!", return; ) ;
  
  if (show == sideWidget()->isVisible())
    return;

  klfDbg("show="<<show) ;

#ifdef Q_WS_MAC
  //  if (show)
  //    klf_qt_mac_set_drawer_preferred_edge(mMacDetailsDrawer, Qt::RightDockWidgetArea);
#endif

  d->dwidget->setVisible(show);
}



