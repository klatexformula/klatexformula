// COPIED CODE FROM Qt 4.7.3 source: src/gui/kernel/qt_mac_p.h;qwidget_mac.mm

#include <QtCore>
#include <QtGui>

#include <qmacdefines_mac.h>

#ifdef __OBJC__
#include <Cocoa/Cocoa.h>
#ifdef QT_MAC_USE_COCOA
#include <objc/runtime.h>
#endif // QT_MAC_USE_COCOA
#endif

#include <CoreServices/CoreServices.h>

#include <Carbon/Carbon.h>


#include <klfdefs.h>


// ------------------------------------------
// DEFINITIONS FOR DRAWER STUFF


static OSWindowRef klf_qt_mac_window_for(OSViewRef view)
{
#ifdef QT_MAC_USE_COCOA
    if (view)
        return [view window];
    return 0;
#else
    return HIViewGetWindow(view);
#endif
}

#ifdef QT_MAC_USE_COCOA
static NSDrawer * klf_qt_mac_drawer_for(const QWidget *widget)
{
  KLF_DEBUG_BLOCK(KLF_FUNC_NAME) ;
  // This only goes one level below the content view so start with the window.
  // This works fine for straight Qt stuff, but runs into problems if we are
  // embedding, but if that's the case, they probably want to be using
  // NSDrawer directly.
  NSView *widgetView = reinterpret_cast<NSView *>(widget->window()->winId());
  NSArray *windows = [NSApp windows];
  for (NSWindow *window in windows) {
    NSArray *drawers = [window drawers];
    for (NSDrawer *drawer in drawers) {
      NSArray *views = [[drawer contentView] subviews];
      for (NSView *view in views) {
	if (view == widgetView) {
	  klfDbg("found drawer. ptr="<<drawer) ;
	  //	  [autoreleasepool release];
	  return drawer;
	}
      }
    }
  }

  klfDbg("not found.") ;
  //  [autoreleasepool release];
  return 0;
}
#endif


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
  
#if QT_MAC_USE_COCOA
  NSDrawer *drawer = klf_qt_mac_drawer_for(w);
  if (!drawer)
    return false;
  NSRectEdge edge;
  if (where & Qt::LeftDockWidgetArea)
    edge = NSMinXEdge;
  else if (where & Qt::RightDockWidgetArea)
    edge = NSMaxXEdge;
  else if (where & Qt::TopDockWidgetArea)
    edge = NSMaxYEdge;
  else if (where & Qt::BottomDockWidgetArea)
    edge = NSMinYEdge;
  else
    return false;
  
  if (edge == [drawer preferredEdge]) //no-op
    return false;
  
  if (w->isVisible()) {
    [drawer close];
    [drawer setPreferredEdge:edge];
    [drawer openOnEdge:edge];
  } else {
    [drawer setPreferredEdge:edge];
  }
#else
  OSWindowRef window = klf_qt_mac_window_for(w);
  OptionBits edge;
  if(where & Qt::LeftDockWidgetArea)
    edge = kWindowEdgeLeft;
  else if(where & Qt::RightDockWidgetArea)
    edge = kWindowEdgeRight;
  else if(where & Qt::TopDockWidgetArea)
    edge = kWindowEdgeTop;
  else if(where & Qt::BottomDockWidgetArea)
    edge = kWindowEdgeBottom;
  else
    return false;
  
  if(edge == GetDrawerPreferredEdge(window)) //no-op
    return false;
  
  //do it
  SetDrawerPreferredEdge(window, edge);
  if(w->isVisible()) {
    CloseDrawer(window, false);
    OpenDrawer(window, edge, true);
  }
#endif
  return true;
}


void klf_qt_mac_close_drawer_and_act(QWidget *w, QObject *obj, const char *member, int timeout_ms)
{
  KLF_DEBUG_BLOCK(KLF_FUNC_NAME) ;

  if(!klf_qt_mac_is_macdrawer(w)) {
    klfDbg("Not a drawer.") ;
    return;
  }
  
#if QT_MAC_USE_COCOA
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
#else
  OSWindowRef window = klf_qt_mac_window_for(w);
  if(w->isVisible()) {
    CloseDrawer(window, false);
  }
#endif
}


bool klf_qt_mac_drawer_is_still_animating(QWidget *w)
{
  KLF_DEBUG_BLOCK(KLF_FUNC_NAME) ;

  if(!klf_qt_mac_is_macdrawer(w)) {
    klfDbg("Not a drawer.") ;
    return false;
  }
  
#if QT_MAC_USE_COCOA
  NSDrawer *drawer = klf_qt_mac_drawer_for(w);
  if (!drawer) {
    qWarning()<<KLF_FUNC_NAME<<": Can't find drawer for widget "<<w;
    return false;
  }

  return
    [drawer state] != NSDrawerClosedState &&
    [drawer state] != NSDrawerOpenState ;

#else
  /** \todo ... CARBON IMPLEMENTATION .... */
  return false;
#endif
}

