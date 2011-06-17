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


Q_GUI_EXPORT OSWindowRef qt_mac_window_for(OSViewRef view)
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
static NSDrawer *qt_mac_drawer_for(const QWidget *widget)
{
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
                if (view == widgetView)
                    return drawer;
            }
        }
    }
    return 0;
}
#endif


bool qt_mac_is_macdrawer(const QWidget *w)
{
    return (w && w->parentWidget() && w->windowType() == Qt::Drawer);
}

bool qt_mac_set_drawer_preferred_edge(QWidget *w, Qt::DockWidgetArea where) //users of Qt for Mac OS X can use this..
{
    if(!qt_mac_is_macdrawer(w))
        return false;

#if QT_MAC_USE_COCOA
    NSDrawer *drawer = qt_mac_drawer_for(w);
    if (!drawer)
        return false;
	NSRectEdge	edge;
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
	    [drawer openOnEdge:edge];
	}
	[drawer setPreferredEdge:edge];
#else
    OSWindowRef window = qt_mac_window_for(w);
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
