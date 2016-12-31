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


#include <QApplication>
#include <QtGlobal>
#include <QLayout>
#include <QEvent>
#include <QResizeEvent>
#include <QStackedLayout>
#include <QPointer>
#include <QTime>
#include <QTimer>

#include <klfdefs.h>

#include "klfsidewidget.h"

#include "klfsidewidget_p.h"



struct KLFSideWidgetManagerBasePrivate
{
  KLF_PRIVATE_HEAD(KLFSideWidgetManagerBase)
  {
    sideWidgetParentConsistency = false;
  }

  QPointer<QWidget> pSideWidget;
  QPointer<QWidget> pParentWidget;

  bool sideWidgetParentConsistency;
};


KLFSideWidgetManagerBase::KLFSideWidgetManagerBase(QWidget *parentWidget, QWidget *sideWidget,
						   bool consistency, QObject *parent)
  : QObject(parent)
{
  KLF_DEBUG_BLOCK(KLF_FUNC_NAME) ;

  KLF_INIT_PRIVATE(KLFSideWidgetManagerBase) ;

  d->sideWidgetParentConsistency = consistency;

  // IMPORTANT: see dox doc of this constructor.
  d->pSideWidget = NULL;
  d->pParentWidget = NULL;

  Q_UNUSED(parentWidget);
  Q_UNUSED(sideWidget);
}

KLFSideWidgetManagerBase::~KLFSideWidgetManagerBase()
{
  KLF_DEBUG_BLOCK(KLF_FUNC_NAME) ;

  // with QPointer should be OK...  //  This has to be done manually by caller to avoid crashes at application/widget-destruction time
  setSideWidget(NULL);

  KLF_DELETE_PRIVATE ;
}

QWidget * KLFSideWidgetManagerBase::sideWidget() const
{
  return d->pSideWidget;
}
QWidget * KLFSideWidgetManagerBase::ourParentWidget() const
{
  return d->pParentWidget;
}

void KLFSideWidgetManagerBase::setOurParentWidget(QWidget *p)
{
  KLF_DEBUG_BLOCK(KLF_FUNC_NAME) ;

  if (d->pParentWidget == p) {
    klfDbg("no-op.") ;
    return;
  }

  klfDbg("old="<<d->pParentWidget<<", new parentWidget="<<p) ;
  QWidget *oldpw = d->pParentWidget;
  d->pParentWidget = p;
  if (d->pSideWidget != NULL && d->sideWidgetParentConsistency) {
    d->pSideWidget->setParent(d->pParentWidget);
  }
  newParentWidgetSet(oldpw, d->pParentWidget);
}

void KLFSideWidgetManagerBase::setSideWidget(QWidget *sideWidget)
{
  KLF_DEBUG_BLOCK(KLF_FUNC_NAME) ;

  if (d->pSideWidget == sideWidget) {
    klfDbg("no-op.") ;
    return;
  }

  klfDbg("old="<<(void*)d->pSideWidget<<", new sideWidget="<<(void*)sideWidget) ;
  klfDbg("old="<<d->pSideWidget<<", new sideWidget="<<sideWidget) ;
  QWidget *oldw = d->pSideWidget;

  d->pSideWidget = sideWidget;

  if (d->pSideWidget != NULL && d->sideWidgetParentConsistency) {
    if (d->pSideWidget->parentWidget() != d->pParentWidget) {
      klfDbg("Adjusting side widget's parent to satisfy parent consistency...") ;
      d->pSideWidget->setParent(d->pParentWidget);
      d->pSideWidget->show(); // subclasses may assume that this object is shown.
    }
  }

  klfDbg("about to call virtual method.") ;
  newSideWidgetSet(oldw, d->pSideWidget);
}


void KLFSideWidgetManagerBase::waitForShowHideActionFinished(int timeout_ms)
{
  if (!showHideIsAnimating())
    return; // we're not animated, we're immediately done.

  QTime tm;

  tm.start();

  //  connect(this, SIGNAL(sideWidgetShown(bool)), this, SLOT(slotSideWidgetShown(bool)));

  // Don't reinitialize to false here, since slotSideWidgetShownActionFinished() could have
  // already been called.
  //  d->actionFinishedSignalReceived = false; 
  while (showHideIsAnimating()) {
    qApp->processEvents();
    if (tm.elapsed() > timeout_ms) {
      klfDbg("timeout while waiting for action-finished signal. timeout_ms="<<timeout_ms) ;
      break;
    }
  }

  klfDbg("finished.");
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
  // msie is only used when the side widget is visible
  QSize msize;

  Qt::Orientation orientation;
  int calcSpacing;
};


KLFShowHideSideWidgetManager::KLFShowHideSideWidgetManager(QWidget *parentWidget, QWidget *sideWidget,
							   QObject *parent)
  : KLFSideWidgetManagerBase(parentWidget, sideWidget, true, parent)
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
  KLF_DEBUG_BLOCK(KLF_FUNC_NAME) ;

  //bool preserveshown = false;
  //bool shown = false;

  if (oldw != NULL) {
    klfDbg("old="<<oldw) ;
    //preserveshown = true;
    //shown = oldw->isVisible();
    oldw->removeEventFilter(this);
    oldw->hide();
  }
  if (neww != NULL) {
    KLF_ASSERT_CONDITION(ourParentWidget() == neww->parentWidget(),
		       "Adding a widget that is not a child of our 'parent widget' ! Correcting parent.",
		       setOurParentWidget(neww->parentWidget()); ) ;
    klfDbg("new="<<neww) ;
    //     if (neww->isVisible()) {
    //       d->msize = neww->parentWidget()->size();
    //       klfDbg("parent size is "<<d->msize);
    //       if (d->orientation & Qt::Horizontal)
    // 	d->msize.setWidth(d->msize.width() - d->calcSpacing + neww->width());
    //       if (d->orientation & Qt::Vertical)
    // 	d->msize.setHeight(d->msize.height() - d->calcSpacing + neww->height());
    //       klfDbg("initialized size to "<<d->msize) ;
    //     } else {
    //       // if widget is about to be shown in initialization process, we will recieve a resizeEvent.
    //       // will calculate proper size there.
    //       d->msize = QSize();
    //     }
    if (neww->parentWidget()->layout() != NULL) {
      setCalcSpacing(neww->parentWidget()->layout()->spacing());
    }
    neww->hide(); //unconditionally hide!
    //    showSideWidget(false);
    //    if (preserveshown && shown)
    //      showSideWidget(true);
    neww->installEventFilter(this);
    emit sideWidgetShown(false);
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
	klfDbg("resize event, new size="<<re->size()<<", old size="<<re->oldSize()
	       <<"; sidewidget->isvisible="<<sideWidget()->isVisible()) ;
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
  
  return KLFSideWidgetManagerBase::eventFilter(obj, event);
}

bool KLFShowHideSideWidgetManager::event(QEvent *event)
{
  return KLFSideWidgetManagerBase::event(event);
}


void KLFShowHideSideWidgetManager::showSideWidget(bool show)
{
  KLF_DEBUG_BLOCK(KLF_FUNC_NAME) ;
  
  KLF_ASSERT_NOT_NULL(sideWidget(), "Side Widget is NULL!", return; ) ;

  klfDbg("show="<<show<<", sideWidgetVisible()="<<sideWidgetVisible()) ;

  if (show == sideWidgetVisible())
    return;

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
  klfDbg("newSize is "<<newSize<<"; d->msize is "<<d->msize) ;
  if (newSize.isValid()) {
    QMetaObject::invokeMethod(this, "resizeParentWidget", Qt::QueuedConnection, Q_ARG(QSize, newSize));
  }
  // will probably(?) emit sideWidgetShown _after_ we resized, which is possibly a more desirable behavior (?)
  QMetaObject::invokeMethod(this, "sideWidgetShown", Qt::QueuedConnection, Q_ARG(bool, show));
}


void KLFShowHideSideWidgetManager::resizeParentWidget(const QSize& size)
{
  KLF_DEBUG_BLOCK(KLF_FUNC_NAME) ;
  klfDbg("size="<<size) ;
  QWidget * sw = sideWidget();
  KLF_ASSERT_NOT_NULL(sw, "Side Widget is NULL!", return; ) ;
  KLF_ASSERT_NOT_NULL(sw->parentWidget(), "Side Widget is NULL!", return; ) ;

  QWidget *window = sw->window();
  KLF_ASSERT_NOT_NULL(window, "hey, side-widget->window() is NULL!", return; ) ;
  QSize diffsize = size - sw->parentWidget()->size();
  QSize winsize = window->size() + diffsize;
  klfDbg("resizing window to "<<winsize) ;
  window->setFixedSize(winsize);
  window->setFixedSize(QSize(QWIDGETSIZE_MAX, QWIDGETSIZE_MAX));
  d->infunction = false;
}


KLF_DEFINE_PROPERTY_GETSET(KLFShowHideSideWidgetManager, Qt::Orientation, orientation, Orientation) ;

KLF_DEFINE_PROPERTY_GETSET(KLFShowHideSideWidgetManager, int, calcSpacing, CalcSpacing) ;


// ------------------




struct KLFContainerSideWidgetManagerPrivate
{
  KLF_PRIVATE_HEAD(KLFContainerSideWidgetManager)
  {
    isdestroying = false;
    dwidget = NULL;
    dlayout = NULL;
    init_pw = init_sw = NULL;
    saved_pw = NULL;
    want_restore_saved = true;
  }

  bool isdestroying;
  QPointer<QWidget> dwidget;
  QStackedLayout *dlayout;

  QWidget *init_pw;
  QWidget *init_sw;

  QPointer<QWidget> saved_pw;
  bool want_restore_saved;


  void restore_saved_parent(QWidget *oldw)
  {
    KLF_DEBUG_BLOCK(KLF_FUNC_NAME) ;
    KLF_ASSERT_NOT_NULL(oldw, "oldw is NULL!", return; ) ;
    klfDbg("oldw="<<(void*)oldw<<"; saved_pw="<<(void*)saved_pw) ;
    klfDbg("oldw="<<oldw<<"; saved_pw="<<saved_pw) ;

    oldw->setParent(saved_pw);

    klfDbg("set parent.") ;

    if (saved_pw != NULL && saved_pw->layout() != NULL) {
      klfDbg("About to reinsert into layout "<<saved_pw->layout()) ;
      saved_pw->layout()->addWidget(oldw);
    }

  }
};


KLFContainerSideWidgetManager::KLFContainerSideWidgetManager(QWidget *parentWidget, QWidget *sideWidget,
							     QObject *parent)
  : KLFSideWidgetManagerBase(parentWidget, sideWidget, false, parent)
{
  KLF_INIT_PRIVATE(KLFContainerSideWidgetManager) ;

  klfDbg("parentWidget="<<parentWidget<<", sideWidget="<<sideWidget) ;
  connect(parentWidget, SIGNAL(destroyed(QObject*)), this, SLOT(aWidgetDestroyed(QObject*)));
  d->init_pw = parentWidget;
  d->init_sw = sideWidget;
}

void KLFContainerSideWidgetManager::init()
{
  d->dwidget = createContainerWidget(d->init_pw);
  connect(d->dwidget, SIGNAL(destroyed(QObject*)), this, SLOT(aWidgetDestroyed(QObject*)));

  d->dwidget->installEventFilter(this); // intercept close events

  KLF_ASSERT_NOT_NULL(d->dwidget, "Created Container Widget is NULL!", return; ) ;

  d->dlayout = new QStackedLayout(d->dwidget);
  d->dlayout->setContentsMargins(0,0,0,0);
  d->dlayout->setSpacing(0);

  d->dwidget->hide();

  setOurParentWidget(d->init_pw);
  setSideWidget(d->init_sw);
}


QWidget * KLFContainerSideWidgetManager::containerWidget() const
{
  return d->dwidget;
}

KLFContainerSideWidgetManager::~KLFContainerSideWidgetManager()
{
  KLF_DEBUG_BLOCK(KLF_FUNC_NAME) ;

  d->isdestroying = true;

  if (d->dwidget != NULL) {
    // make sure we do not destroy the sideWidget while destroying d->dwidget !
    d->restore_saved_parent(sideWidget());
  }

  klfDbg("d->dwidget="<<(void*)d->dwidget) ;
  klfDbg("d->dwidget="<<d->dwidget) ;

// d->dwidget should get deleted automatically when its parent widget dies.  Doing this
// explicitly on the other hand, and in the destructor, may cause a segfault
//
//  if (d->dwidget != NULL) {
//    klfDbg("deleting...") ;
//    delete d->dwidget;
//    klfDbg("...done") ;
//  }
  KLF_DELETE_PRIVATE ;
}

bool KLFContainerSideWidgetManager::eventFilter(QObject *obj, QEvent *event)
{
  if (obj == d->dwidget) {
    if (event->type() == QEvent::Close) {
      klfDbg("intercepting close event.") ;
      // close button clicked, eg. in floating window; make sure to hide the widget
      // appropriately, emitting sideWidgetShown(bool) too, or saving its geometry etc.
      showSideWidget(false);
      return true;
    }
  }
  return KLFSideWidgetManagerBase::eventFilter(obj, event);
}

bool KLFContainerSideWidgetManager::sideWidgetVisible() const
{
  KLF_ASSERT_NOT_NULL(d->dwidget, "Container Widget is NULL! Did you forget to call init()?", return false; ) ;
  return d->dwidget->isVisible();
}

void KLFContainerSideWidgetManager::showSideWidget(bool show)
{
  KLF_ASSERT_NOT_NULL(d->dwidget, "Container Widget is NULL! Did you forget to call init()?", return; ) ;

  // and actually show/hide the container widget
  d->dwidget->setVisible(show);
  d->dwidget->setFocus();
  emit sideWidgetShown(show);
}

// protected
void KLFContainerSideWidgetManager::newSideWidgetSet(QWidget *oldw, QWidget *neww)
{
  KLF_DEBUG_BLOCK(KLF_FUNC_NAME) ;

  if (d->isdestroying)
    return; // destructor has its own special treatment

  KLF_ASSERT_NOT_NULL(d->dwidget, "Container Widget is NULL! Did you forget to call init()?", return; ) ;

  klfDbg("new side widget: old="<<oldw<<", new="<<neww<<"; want restore saved="<<d->want_restore_saved) ;
  if (oldw != NULL && d->want_restore_saved) {
    d->dlayout->removeWidget(oldw);
    //    oldw->hide(); // setParent() automatically hides the widget
    d->restore_saved_parent(oldw);
  }
  if (d->saved_pw != NULL) {
    klfDbg("Disconnecting the saved parent widget.") ;
    disconnect(d->saved_pw, SIGNAL(destroyed()), this, 0);
    d->saved_pw = NULL;
  }
  if (neww != NULL) {
    d->saved_pw = neww->parentWidget(); // save its parent widget so that we can restore it
    if (d->saved_pw != NULL) {
      bool connected = connect(d->saved_pw, SIGNAL(destroyed(QObject*)), this, SLOT(aWidgetDestroyed(QObject*)));
      Q_UNUSED(connected) ;
      klfDbg("saving pw : "<<d->saved_pw<<" and connected to destroyed(QObject*) signal?="<<connected) ;
    }
    d->want_restore_saved = true;
    neww->setParent(NULL) ;
    neww->setParent(d->dwidget);
    d->dlayout->addWidget(neww);
    neww->show();
    emit sideWidgetShown(d->dwidget->isVisible());
  }
}

// protected
void KLFContainerSideWidgetManager::newParentWidgetSet(QWidget *, QWidget *newWidget)
{
  if (d->dwidget->parentWidget() != newWidget)
    d->dwidget->setParent(newWidget);
}

// private slot
void KLFContainerSideWidgetManager::aWidgetDestroyed(QObject *w)
{
  KLF_DEBUG_BLOCK(KLF_FUNC_NAME) ;
  klfDbg("w="<<w) ;
  if (w == d->saved_pw) {
    klfDbg("saved parent "<<d->saved_pw<<" or our own parent = "<<parent()<<" was destroyed!") ;
    d->want_restore_saved = false;
    d->saved_pw = NULL;
  }
  // else if (w == d->dwidget) {
  //    d->dwidget = NULL;
  //  }
}








// ------------------



struct KLFFloatSideWidgetManagerPrivate
{
  KLF_PRIVATE_HEAD(KLFFloatSideWidgetManager)
  {
    dwgeom = QRect();
  }

  QRect dwgeom;
};


KLFFloatSideWidgetManager::KLFFloatSideWidgetManager(QWidget *parentWidget, QWidget *sideWidget, QObject *parent)
  : KLFContainerSideWidgetManager(parentWidget, sideWidget, parent)
{
  KLF_INIT_PRIVATE(KLFFloatSideWidgetManager) ;

  init();
}

KLFFloatSideWidgetManager::~KLFFloatSideWidgetManager()
{
  KLF_DELETE_PRIVATE ;
}

Qt::WindowFlags KLFFloatSideWidgetManager::wflags() const
{
  return containerWidget()->windowFlags();
}

bool KLFFloatSideWidgetManager::sideWidgetVisible() const
{
  return containerWidget()->isVisible();
}

QWidget * KLFFloatSideWidgetManager::createContainerWidget(QWidget *pw)
{
  return  new QWidget(pw, Qt::Tool|Qt::CustomizeWindowHint|Qt::WindowTitleHint
		      |Qt::WindowSystemMenuHint
#if QT_VERSION >= 0x040500
		      |Qt::WindowCloseButtonHint
#endif
		      );
}

void KLFFloatSideWidgetManager::setWFlags(Qt::WindowFlags wf)
{
  KLF_ASSERT_NOT_NULL(sideWidget(), "side widget is NULL!", return; ) ;
  wf |= Qt::Window; // make sure it is not a nested widget
  containerWidget()->setWindowFlags(wf);
}

void KLFFloatSideWidgetManager::showSideWidget(bool show)
{
  QWidget *w = containerWidget();
  if (sideWidgetVisible()) {
    // save position and size
    d->dwgeom = w->geometry();
  }
  if (show && d->dwgeom.isValid()) {
    // set saved position and size
    w->setGeometry(d->dwgeom);
  }
  // this automatically emits sideWidgetShown
  KLFContainerSideWidgetManager::showSideWidget(show);
}


// protected
void KLFFloatSideWidgetManager::newSideWidgetSet(QWidget *oldw, QWidget *neww)
{
  KLF_DEBUG_BLOCK(KLF_FUNC_NAME) ;
  KLFContainerSideWidgetManager::newSideWidgetSet(oldw, neww);
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
QStringList KLFSideWidgetManagerFactory::allSupportedTypes()
{
  return pFactoryManager.allSupportedTypes();
}

// static
KLFSideWidgetManagerFactory * KLFSideWidgetManagerFactory::findFactoryFor(const QString& managertype)
{
  return dynamic_cast<KLFSideWidgetManagerFactory*>(pFactoryManager.findFactoryFor(managertype));
}

// static
KLFSideWidgetManagerBase *
/* */ KLFSideWidgetManagerFactory::findCreateSideWidgetManager(const QString& type, QWidget *pw,
							       QWidget *sw, QObject *parent)
{
  KLFSideWidgetManagerFactory * f = findFactoryFor(type);

  KLF_ASSERT_NOT_NULL(f, "Can't find factory for side widget manager type="<<type<<"!", return NULL; ) ;

  return f->createSideWidgetManager(type, pw, sw, parent);
}


QStringList KLFSideWidgetManagerFactory::supportedTypes() const
{
  return QStringList()
    << QLatin1String("ShowHide")
    << QLatin1String("Float")
#ifdef KLF_WS_MAC
    << QLatin1String("Drawer")
#endif
    ;
}

QString KLFSideWidgetManagerFactory::getTitleFor(const QString& type) const
{
  if (type == QLatin1String("ShowHide"))
    return QObject::tr("Expand/Shrink Window", "[[KLFSideWidgetManagerFactory]]");
  if (type == QLatin1String("Float"))
    return QObject::tr("Floating Tool Window", "[[KLFSideWidgetManagerFactory]]");
  if (type == QLatin1String("Drawer"))
    return QObject::tr("Side Drawer", "[[KLFSideWidgetManagerFactory]]");

  return QString();
}

KLFSideWidgetManagerBase *
/* */  KLFSideWidgetManagerFactory::createSideWidgetManager(const QString& type, QWidget *parentWidget,
							    QWidget *sideWidget, QObject *parent)
{
  if (type == QLatin1String("ShowHide")) {
    return new KLFShowHideSideWidgetManager(parentWidget, sideWidget, parent);
  }
  if (type == QLatin1String("Float")) {
    return new KLFFloatSideWidgetManager(parentWidget, sideWidget, parent);
  }
#ifdef KLF_WS_MAC
  if (type == QLatin1String("Drawer")) {
    return new KLFDrawerSideWidgetManager(parentWidget, sideWidget, parent);
  }
#endif

  qWarning()<<KLF_FUNC_NAME<<": Unknown side-widget-manager type "<<type;

  return NULL;
}


// an instance of the factory
KLFSideWidgetManagerFactory __klf_side_widget_manager_factory;


// ---------

struct KLFSideWidgetPrivate
{
  KLF_PRIVATE_HEAD(KLFSideWidget) {
    manager = NULL;
    swmtype = QString();
  }

  KLFSideWidgetManagerBase * manager;
  QString swmtype;
};


/*KLFSideWidget::KLFSideWidget(SideWidgetManager mtype, QWidget *parent)
  : QWidget(parent)
{
  KLF_INIT_PRIVATE(KLFSideWidget) ;
  setSideWidgetManager(mtype);
}
KLFSideWidget::KLFSideWidget(const QString& mtype, QWidget *parent)
{
  KLF_INIT_PRIVATE(KLFSideWidget) ;
  setSideWidgetManager(mtype);
  }*/

KLFSideWidget::KLFSideWidget(QWidget *parent)
  : QWidget(parent)
{
  KLF_INIT_PRIVATE(KLFSideWidget) ;

  _inqtdesigner = false;

}
KLFSideWidget::~KLFSideWidget()
{
  KLF_DEBUG_BLOCK(KLF_FUNC_NAME) ;

  delete d->manager;

  KLF_DELETE_PRIVATE ;
}

KLFSideWidgetManagerBase * KLFSideWidget::sideWidgetManager()
{
  return d->manager;
}

bool KLFSideWidget::sideWidgetVisible() const
{
  KLF_ASSERT_NOT_NULL(d->manager, "Manager is NULL!", return false) ;
  return d->manager->sideWidgetVisible();
}

QString KLFSideWidget::sideWidgetManagerType() const
{
  return d->swmtype;
}


void KLFSideWidget::setSideWidgetManager(SideWidgetManager mtype)
{
  QString s;
  switch (mtype) {
  case ShowHide: s = QLatin1String("ShowHide"); break;
  case Float: s = QLatin1String("Float"); break;
  case Drawer: s = QLatin1String("Drawer"); break;
  default: break;
  }

  KLF_ASSERT_CONDITION(!s.isEmpty(), "Invalid mtype: "<<mtype<<"!", return; ) ;
  setSideWidgetManager(s);
}
void KLFSideWidget::setSideWidgetManager(const QString& mtype)
{
  KLF_DEBUG_BLOCK(KLF_FUNC_NAME) ;

  if (_inqtdesigner) {
    klfDbg("We're in Qt Designer. DUMMY ACTION.") ;
    d->swmtype = mtype;
    return;
  }

  if (d->swmtype == mtype) {
    klfDbg("no-op");
    return;
  }

  if (d->manager != NULL) {
    d->manager->hideSideWidget();
    klfDbg("deleting current manager") ;
    //    d->manager->setSideWidget(NULL) ; // re-take the side widget
    delete d->manager;
    d->manager = NULL;
    d->swmtype = QString();
  }

  d->swmtype = mtype;
  d->manager = KLFSideWidgetManagerFactory::findCreateSideWidgetManager(mtype, parentWidget(), this, this);
  KLF_ASSERT_NOT_NULL(d->manager, "Factory returned NULL manager for type "<<mtype<<"!", return; ) ;

  connect(d->manager, SIGNAL(sideWidgetShown(bool)), this, SIGNAL(sideWidgetShown(bool)));

  emit sideWidgetManagerTypeChanged(mtype);
  emit sideWidgetShown(d->manager->sideWidgetVisible());
}

void KLFSideWidget::showSideWidget(bool show)
{
  if (_inqtdesigner)
    return;

  KLF_ASSERT_NOT_NULL(d->manager, "Manager is NULL! For debugging purposes, I'm creating a 'float' manager !",
		      setSideWidgetManager(Float); ) ;
  KLF_ASSERT_NOT_NULL(d->manager, "Manager is NULL!", return; ) ;
  d->manager->showSideWidget(show);
}

void KLFSideWidget::debug_unlock_qtdesigner()
{
  if (_inqtdesigner) {
    _inqtdesigner = false;
    setSideWidgetManager(sideWidgetManagerType());
  }
}
