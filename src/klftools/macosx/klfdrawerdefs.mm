// COPIED CODE FROM Qt 4.7.3 source: src/gui/kernel/qt_mac_p.h;qwidget_mac.mm
// -- and then adapted when porting to Qt5

#include <QtCore>
#include <QtGui>
#include <QtWidgets>

#ifdef __OBJC__
#include <Cocoa/Cocoa.h>
#include <objc/runtime.h>
#endif

#include <CoreServices/CoreServices.h>

#include <klfdefs.h>


// ------------------------------------------
// DEFINITIONS FOR DRAWER STUFF


static bool contains_view(NSView * view, NSView * lookingForView, int levels = 4)
{
  klfDbg("("<<levels<<") Looking for view in "<<view) ;

  if (view == lookingForView) {
    return true;
  }

  NSArray * subviews = [view subviews];
  for (NSView *subview in subviews) {
    if (lookingForView == subview) {
      return true;
    }
  }
  if (levels > 0) {
    for (NSView * subview in subviews) {
      bool trysub = contains_view(subview, lookingForView, levels - 1);
      if (trysub) {
        return true;
      }
    }
  }
  return false;
}
static NSDrawer * klf_qt_mac_drawer_for(const QWidget *widget)
{
  KLF_DEBUG_BLOCK(KLF_FUNC_NAME) ;

  NSView *widgetView = reinterpret_cast<NSView *>(widget->window()->winId());
  NSArray *windows = [NSApp windows];
  for (NSWindow *window in windows) {
    NSArray *drawers = [window drawers];
    for (NSDrawer *drawer in drawers) {
      klfDbg("investigating drawer "<< drawer << "...") ;
      if (contains_view([drawer contentView], widgetView)) {
        klfDbg("found drawer. ptr="<<drawer) ;
        return drawer;
      }
    }
  }

  klfDbg("not found.") ;
  return 0;
}


static bool klf_qt_mac_is_macdrawer(const QWidget *w)
{
    return (w && w->parentWidget() && w->windowType() == Qt::Drawer);
}

bool klf_qt_mac_set_drawer_preferred_edge(QWidget *w, Qt::DockWidgetArea where)
{
  KLF_DEBUG_BLOCK(KLF_FUNC_NAME) ;

  if(!klf_qt_mac_is_macdrawer(w)) {
    klfDbg("Not a drawer.") ;
    return false;
  }

  NSDrawer *drawer = klf_qt_mac_drawer_for(w);
  if (!drawer) {
    klfDbg("Didn't find the drawer for "<<w<<" !!") ;
    return false;
  }
  NSRectEdge edge;
  if (where & Qt::LeftDockWidgetArea) {
    edge = NSMinXEdge;
  } else if (where & Qt::RightDockWidgetArea) {
    edge = NSMaxXEdge;
  } else if (where & Qt::TopDockWidgetArea) {
    edge = NSMaxYEdge;
  } else if (where & Qt::BottomDockWidgetArea) {
    edge = NSMinYEdge;
  } else {
    klfWarning("Where is not a valid edge (One of Qt::DockWidgetArea): "<<where);
    return false;
  }
  
  NSRectEdge curPrefEdge = [drawer preferredEdge];
  if (edge == curPrefEdge) { //no-op
    klfDbg("Already the right edge. preferredEdge="<<curPrefEdge);
    return false;
  }
  
  if (w->isVisible()) {
    [drawer close];
    [drawer setPreferredEdge:edge];
    [drawer openOnEdge:edge];
  } else {
    [drawer setPreferredEdge:edge];
  }
  return true;
}


void klf_qt_mac_close_drawer_and_act(QWidget *w, QObject *obj, const char *member, int timeout_ms)
{
  KLF_DEBUG_BLOCK(KLF_FUNC_NAME) ;

  Q_UNUSED(timeout_ms) ; // actually not used.


  if(!klf_qt_mac_is_macdrawer(w)) {
    klfDbg("Not a drawer.") ;
    return;
  }
  
  NSDrawer *drawer = klf_qt_mac_drawer_for(w);
  if (!drawer) {
    qWarning()<<KLF_FUNC_NAME<<": Can't find drawer for widget "<<w;
    return;
  }
  
  [drawer close];
  while ([drawer state] != NSDrawerClosedState) {
    qApp->processEvents();
  }
  if (obj != NULL && member != NULL) {
    QMetaObject::invokeMethod(obj, member);
  }
}


bool klf_qt_mac_drawer_is_still_animating(QWidget *w)
{
  KLF_DEBUG_BLOCK(KLF_FUNC_NAME) ;

  if(!klf_qt_mac_is_macdrawer(w)) {
    klfDbg("Not a drawer.") ;
    return false;
  }
  
  NSDrawer *drawer = klf_qt_mac_drawer_for(w);
  if (!drawer) {
    qWarning()<<KLF_FUNC_NAME<<": Can't find drawer for widget "<<w;
    return false;
  }

  return
    [drawer state] != NSDrawerClosedState &&
    [drawer state] != NSDrawerOpenState ;

}

