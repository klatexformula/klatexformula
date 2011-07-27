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
    sideWidgetParentConsistency = false;
  }

  bool sideWidgetParentConsistency;
};


KLFSideWidgetManagerBase::KLFSideWidgetManagerBase(QWidget *parentWidget, QWidget *sideWidget, bool consistency)
  : QObject(parentWidget)
{
  KLF_DEBUG_BLOCK(KLF_FUNC_NAME) ;

  KLF_INIT_PRIVATE(KLFSideWidgetManagerBase) ;

  d->sideWidgetParentConsistency = consistency;

  // IMPORTANT: see dox doc of this constructor.
  pSideWidget = NULL;
  pParentWidget = NULL;
  //  pSideWidget = sideWidget;
  //  pParentWidget = parentWidget;

  Q_UNUSED(sideWidget);
}

KLFSideWidgetManagerBase::~KLFSideWidgetManagerBase()
{
  KLF_DELETE_PRIVATE ;
}

void KLFSideWidgetManagerBase::setOurParentWidget(QWidget *p)
{
  KLF_DEBUG_BLOCK(KLF_FUNC_NAME) ;

  if (pParentWidget == p) {
    klfDbg("no-op.") ;
    return;
  }

  klfDbg("old="<<pParentWidget<<", new parentWidget="<<p) ;
  QWidget *oldpw = pParentWidget;
  pParentWidget = p;
  if (pSideWidget != NULL && d->sideWidgetParentConsistency) {
    pSideWidget->setParent(pParentWidget);
  }
  newParentWidgetSet(oldpw, pParentWidget);
}

void KLFSideWidgetManagerBase::setSideWidget(QWidget *sideWidget)
{
  KLF_DEBUG_BLOCK(KLF_FUNC_NAME) ;

  if (pSideWidget == sideWidget) {
    klfDbg("no-op.") ;
    return;
  }

  klfDbg("old="<<pSideWidget<<", new sideWidget="<<sideWidget) ;
  QWidget *oldw = pSideWidget;

  pSideWidget = sideWidget;

  if (d->sideWidgetParentConsistency) {
    if (pSideWidget->parentWidget() != pParentWidget) {
      klfDbg("Adjusting side widget's parent to satisfy parent consistency...") ;
      pSideWidget->setParent(pParentWidget);
      pSideWidget->show(); // subclasses may assume that this object is shown.
    }
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
};


KLFShowHideSideWidgetManager::KLFShowHideSideWidgetManager(QWidget *parentWidget, QWidget *sideWidget)
  : KLFSideWidgetManagerBase(parentWidget, sideWidget, true)
{
  KLF_INIT_PRIVATE(KLFShowHideSideWidgetManager) ;

  setOurParentWidget(parentWidget);
  setSideWidget(sideWidget);
}

KLFShowHideSideWidgetManager::~KLFShowHideSideWidgetManager()
{
  KLF_DELETE_PRIVATE ;
}

// protected
void KLFShowHideSideWidgetManager::newSideWidgetSet(QWidget *oldw, QWidget *neww)
{
  KLF_ASSERT_CONDITION(ourParentWidget() == neww->parentWidget(),
		   "Adding a widget that is not a child of our 'parent widget' ! Correcting parent.",
		   setOurParentWidget(neww->parentWidget()); ) ;

  bool shown = false;

  if (oldw != NULL) {
    shown = oldw->isVisible();
    oldw->removeEventFilter(this);
    oldw->hide();
  }
  if (neww != NULL) {
    neww->setVisible(shown);
    neww->installEventFilter(this);
  }
  d->msize = QSize();
}


// protected
void KLFShowHideSideWidgetManager::newParentWidgetSet(QWidget */*oldParent*/, QWidget *pw)
{
  KLF_DEBUG_BLOCK(KLF_FUNC_NAME) ;

  if (d->oldParent != NULL)
    d->oldParent->removeEventFilter(this);
  if (pw == NULL) {
    d->msize = QSize(-1, -1);
    return;
  }
  d->msize = pw->size();
  pw->installEventFilter(this);
}

bool KLFShowHideSideWidgetManager::sideWidgetVisible() const
{
  KLF_ASSERT_NOT_NULL(sideWidget(), "Side Widget is NULL!", return false; ) ;
  return sideWidget()->isVisible();
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

  if (show == sideWidgetVisible())
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

  klfDbg("sideWidget is "<<sideWidget()) ;
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
    dlayout = NULL;
    dsize = QSize();
    openEdge = Qt::RightDockWidgetArea;
  }

  QWidget *dwidget;
  QBoxLayout *dlayout;

  QSize dsize;

  Qt::DockWidgetArea openEdge;
};

KLFDrawerSideWidgetManager::KLFDrawerSideWidgetManager(QWidget *pw, QWidget *sideWidget)
  : KLFSideWidgetManagerBase(pw, sideWidget)
{
  KLF_DEBUG_BLOCK(KLF_FUNC_NAME) ;
  klfDbg("parentW="<<pw<<", sidewidget="<<sideWidget) ;

  KLF_INIT_PRIVATE(KLFDrawerSideWidgetManager) ;

  d->dwidget = new QWidget(pw, Qt::Drawer);
  d->dlayout = new QVBoxLayout(d->dwidget);
  d->dlayout->setContentsMargins(0,0,0,0);
  d->dlayout->setSpacing(0);

  d->dwidget->hide();

  // needs to be called _after_ we created d->dwidget properly
  setOurParentWidget(pw);
  setSideWidget(sideWidget);
}

KLFDrawerSideWidgetManager::~KLFDrawerSideWidgetManager()
{
  KLF_DELETE_PRIVATE ;
}

// protected
void KLFDrawerSideWidgetManager::newSideWidgetSet(QWidget *oldw, QWidget *neww)
{
  klfDbg("new side widget: old="<<oldw<<", new="<<neww) ;
  if (oldw != NULL) {
    d->dlayout->removeWidget(oldw);
    //    oldw->hide(); // setParent() automatically hides the widget
    oldw->setParent(NULL);
  }
  if (neww != NULL) {
    neww->setParent(NULL) ;
    neww->setParent(d->dwidget);
    d->dlayout->addWidget(neww);
    neww->show();
  }
  d->dsize = QSize();
}

// protected
void KLFDrawerSideWidgetManager::newParentWidgetSet(QWidget *, QWidget *newWidget)
{
  KLF_DEBUG_BLOCK(KLF_FUNC_NAME) ;

  if (d->dwidget->parentWidget() != newWidget)
    d->dwidget->setParent(newWidget);
}

bool KLFDrawerSideWidgetManager::sideWidgetVisible() const
{
  return d->dwidget->isVisible();
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
  extern bool klf_qt_mac_set_drawer_preferred_edge(QWidget *w, Qt::DockWidgetArea where);
#endif

  KLF_DEBUG_BLOCK(KLF_FUNC_NAME) ;

  KLF_ASSERT_NOT_NULL(sideWidget(), "Side Widget is NULL!", return; ) ;
  
  klfDbg("show="<<show<<", sideWidgetVisible="<<sideWidgetVisible()) ;

  if (show == sideWidgetVisible())
    return;

  if (sideWidgetVisible() && !show) {
    klfDbg("about to maybe save size. d->openEdge="<<d->openEdge<<", _klf_openEdge="
	   <<d->dwidget->property("_klf_openEdge")) ;
    if (d->dwidget->property("_klf_openEdge").toInt() == d->openEdge) {
      d->dsize = d->dwidget->size();
    } else {
      d->dsize = QSize();
    }
  }
  if (show) {
    klfDbg("Will adjust widget size, d->dsize="<<d->dsize) ;
    if (d->dsize.isValid())
      d->dwidget->resize(d->dsize);
    //    else
    //      d->dwidget->resize(d->dwidget->minimumSizeHint());
  }
    
#ifdef Q_WS_MAC
  if (show) {
    klf_qt_mac_set_drawer_preferred_edge(d->dwidget, d->openEdge);
    d->dwidget->setProperty("_klf_openEdge", QVariant((int)d->openEdge));
  }
#endif

  d->dwidget->setVisible(show);
}



KLF_DEFINE_PROPERTY_GET(KLFDrawerSideWidgetManager, Qt::DockWidgetArea, openEdge, OpenEdge) ;

void KLFDrawerSideWidgetManager::setOpenEdge(Qt::DockWidgetArea edge)
{
  if (edge == d->openEdge)
    return; // no-op

  d->openEdge = edge;
  d->dsize = QSize();
#ifdef Q_WS_MAC
  if (sideWidgetVisible()) {
    extern bool klf_qt_mac_set_drawer_preferred_edge(QWidget *w, Qt::DockWidgetArea edge);
    klf_qt_mac_set_drawer_preferred_edge(sideWidget(), edge);
  }
#endif
}



// --------------



struct KLFFloatSideWidgetManagerPrivate
{
  KLF_PRIVATE_HEAD(KLFFloatSideWidgetManager)
  {
    dwidget = NULL;
    dlayout = NULL;
  }

  QWidget *dwidget;
  QBoxLayout *dlayout;
};


KLFFloatSideWidgetManager::KLFFloatSideWidgetManager(QWidget *parentWidget, QWidget *sideWidget)
  : KLFSideWidgetManagerBase(parentWidget, sideWidget, false)
{
  KLF_INIT_PRIVATE(KLFFloatSideWidgetManager) ;

  d->dwidget = new QWidget(parentWidget, Qt::Tool);
  d->dlayout = new QVBoxLayout(d->dwidget);
  d->dlayout->setContentsMargins(0,0,0,0);
  d->dlayout->setSpacing(0);

  d->dwidget->hide();

  setOurParentWidget(parentWidget);
  setSideWidget(sideWidget);
}

KLFFloatSideWidgetManager::~KLFFloatSideWidgetManager()
{
  KLF_DELETE_PRIVATE ;
}

Qt::WindowFlags KLFFloatSideWidgetManager::wflags() const
{
  return d->dwidget->windowFlags();
}

bool KLFFloatSideWidgetManager::sideWidgetVisible() const
{
  return d->dwidget->isVisible();
}

void KLFFloatSideWidgetManager::setWFlags(Qt::WindowFlags wf)
{
  KLF_ASSERT_NOT_NULL(sideWidget(), "side widget is NULL!", return; ) ;
  wf |= Qt::Window; // make sure it is not a nested widget
  d->dwidget->setWindowFlags(wf);
}

void KLFFloatSideWidgetManager::showSideWidget(bool show)
{
  d->dwidget->setVisible(show);
}

// protected
void KLFFloatSideWidgetManager::newSideWidgetSet(QWidget *oldw, QWidget *neww)
{
  klfDbg("new side widget: old="<<oldw<<", new="<<neww) ;
  if (oldw != NULL) {
    d->dlayout->removeWidget(oldw);
    //    oldw->hide(); // setParent() automatically hides the widget
    oldw->setParent(NULL);
  }
  if (neww != NULL) {
    neww->setParent(NULL) ;
    neww->setParent(d->dwidget);
    d->dlayout->addWidget(neww);
    neww->show();
  }
}






// ---------------------------------------------

// static
KLFFactoryManager KLFSideWidgetManagerFactory::pFactoryManager;


KLFSideWidgetManagerFactory::KLFSideWidgetManagerFactory()
  : KLFFactoryBase(&pFactoryManager)
{
}
KLFSideWidgetManagerFactory::~KLFSideWidgetManagerFactory()
{
}

// static
KLFSideWidgetManagerFactory * KLFSideWidgetManagerFactory::findFactoryFor(const QString& managertype)
{
  return dynamic_cast<KLFSideWidgetManagerFactory*>(pFactoryManager.findFactoryFor(managertype));
}

QStringList KLFSideWidgetManagerFactory::supportedTypes() const
{
  return QStringList()
    << QLatin1String("ShowHide")
    << QLatin1String("Float")
#ifdef Q_WS_MAC
    << QLatin1String("Drawer")
#endif
    ;
}

KLFSideWidgetManagerBase *
/* */  KLFSideWidgetManagerFactory::createSideWidgetManager(const QString& type, QWidget *parentWidget,
						QWidget *sideWidget)
{
  if (type == QLatin1String("ShowHide")) {
    return new KLFShowHideSideWidgetManager(parentWidget, sideWidget);
  }
  if (type == QLatin1String("Float")) {
    return new KLFFloatSideWidgetManager(parentWidget, sideWidget);
  }
#ifdef Q_WS_MAC
  if (type == QLatin1String("Drawer")) {
    return new KLFDrawerSideWidgetManager(parentWidget, sideWidget);
  }
#endif

  qWarning()<<KLF_FUNC_NAME<<": Unknown side-widget-manager type "<<type;

  return NULL;
}
