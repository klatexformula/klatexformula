
#include <QtCore>
#include <QtGui>
#include <QtWidgets>
#include <QMacNativeWidget>

#include <Cocoa/Cocoa.h>
#include <objc/runtime.h>
#include <CoreServices/CoreServices.h>

#include <klfdefs.h>

#include <klfsidewidget.h>
#include "klfsidewidget_p.h"



inline NSRectEdge nsEdgeForQtEdge(Qt::DockWidgetArea edge)
{
  switch (edge) {
  case Qt::LeftDockWidgetArea:     return NSMinXEdge;
  case Qt::RightDockWidgetArea:    return NSMaxXEdge;
  case Qt::TopDockWidgetArea:      return NSMinYEdge;
  case Qt::BottomDockWidgetArea:   return NSMaxYEdge;
  default:
    qWarning()<<KLF_FUNC_NAME<<": Invalid Qt::DockWidgetArea edge: " << edge << "\n";
  }
  return NSMaxXEdge; // by default
}



struct KLFDrawerSideWidgetManagerPrivate
{
  KLF_PRIVATE_HEAD(KLFDrawerSideWidgetManager)
  {
    nsdrawer = NULL;
    sideWidget = NULL;
    nativeWidget = NULL;
    layout = NULL;
    openEdge = Qt::RightDockWidgetArea;
  }

  NSDrawer * nsdrawer;
  QWidget * sideWidget;
  QMacNativeWidget * nativeWidget;
  QLayout * layout;

  Qt::DockWidgetArea openEdge;
};

KLFDrawerSideWidgetManager::KLFDrawerSideWidgetManager(QWidget *parentWidget, QWidget *sideWidget, QObject *parent)
  : KLFSideWidgetManagerBase(parentWidget, sideWidget, false, parent)
{
  KLF_DEBUG_BLOCK(KLF_FUNC_NAME) ;
  klfDbg("parentW=" << parentWidget << ", sidewidget=" << sideWidget << ", parent=" << parent) ;

  KLF_INIT_PRIVATE(KLFDrawerSideWidgetManager) ;

  NSView * nswinview = reinterpret_cast<NSView*>(parentWidget->window()->winId());

  QSize sideWidgetSizeHint = sideWidget->sizeHint();
  NSSize csz;
  csz.width = sideWidgetSizeHint.width();
  csz.height = sideWidgetSizeHint.height();
  d->nsdrawer = [[NSDrawer alloc] initWithContentSize:csz preferredEdge:NSRectEdgeMaxX];
  [d->nsdrawer setParentWindow:[nswinview window]];
  NSView *contentView = [d->nsdrawer contentView];

  // inspired by http://doc.qt.io/qt-5/qmacnativewidget.html

  d->nativeWidget = new QMacNativeWidget(contentView);
  d->nativeWidget->move(0, 0);
  d->nativeWidget->setPalette(QPalette(Qt::red));
  d->nativeWidget->setAutoFillBackground(true);
  d->nativeWidget->setMinimumSize(sideWidget->minimumSizeHint());
  d->layout = new QVBoxLayout();
  d->sideWidget = sideWidget;
  d->sideWidget->setParent(d->nativeWidget);
  d->sideWidget->setAttribute(Qt::WA_LayoutUsesWidgetRect);
  d->layout->addWidget(sideWidget);
  d->nativeWidget->setLayout(d->layout);

  NSView *nativeWidgetView = d->nativeWidget->nativeView();
  [contentView setAutoresizesSubviews:YES];
  [nativeWidgetView setAutoresizingMask:NSViewWidthSizable | NSViewHeightSizable];
  [nativeWidgetView setAutoresizesSubviews:YES];
  NSView *sideWidgetView = reinterpret_cast<NSView *>(d->sideWidget->winId());
  [sideWidgetView setAutoresizingMask:NSViewWidthSizable | NSViewHeightSizable];
    
  // Add the nativeWidget to the window.
  [contentView addSubview:nativeWidgetView positioned:NSWindowAbove relativeTo:nil];
  d->nativeWidget->show();
  d->sideWidget->show();

  setOurParentWidget(parentWidget);
  setSideWidget(sideWidget);
}

KLFDrawerSideWidgetManager::~KLFDrawerSideWidgetManager()
{
  KLF_DEBUG_BLOCK(KLF_FUNC_NAME) ;

  KLF_DELETE_PRIVATE ;
}

// protected
void KLFDrawerSideWidgetManager::newSideWidgetSet(QWidget *oldw, QWidget *neww)
{
  if (oldw != NULL) {
    d->layout->removeWidget(oldw);
  }
  if (neww != NULL) {
    d->sideWidget = neww;
    d->sideWidget->setParent(d->nativeWidget);
    d->sideWidget->setAttribute(Qt::WA_LayoutUsesWidgetRect);
    d->layout->addWidget(neww);
    NSView *sideWidgetView = reinterpret_cast<NSView *>(d->sideWidget->winId());
    [sideWidgetView setAutoresizingMask:NSViewWidthSizable];
    d->sideWidget->show();
  }
}

// protected
void KLFDrawerSideWidgetManager::newParentWidgetSet(QWidget */*oldp*/, QWidget *newWidget)
{
  if (d->sideWidget->parentWidget() != newWidget) {
    d->sideWidget->setParent(newWidget);
  }
}

bool KLFDrawerSideWidgetManager::sideWidgetVisible() const
{
  NSInteger s = [d->nsdrawer state];
  return (s == NSDrawerOpenState || s == NSDrawerOpeningState);
}

void KLFDrawerSideWidgetManager::showSideWidget(bool show)
{
  KLF_DEBUG_BLOCK(KLF_FUNC_NAME) ;

  if (show == sideWidgetVisible()) {
    klfDbg("show == sideWidgetVisible() == " << show);
    return;
  }

  klfDbg("show=" << show << "; sideWidgetVisible()=" << sideWidgetVisible());

  if (show) {
    [d->nsdrawer openOnEdge:nsEdgeForQtEdge(d->openEdge)];
  } else {
    [d->nsdrawer close];
  }

  emit sideWidgetShown(show);
}

bool KLFDrawerSideWidgetManager::showHideIsAnimating()
{
  NSInteger s = [d->nsdrawer state];
  return (s == NSDrawerOpeningState || s == NSDrawerClosingState);
}


KLF_DEFINE_PROPERTY_GET(KLFDrawerSideWidgetManager, Qt::DockWidgetArea, openEdge) ;

void KLFDrawerSideWidgetManager::setOpenEdge(Qt::DockWidgetArea edge)
{
  d->openEdge = edge;
}



