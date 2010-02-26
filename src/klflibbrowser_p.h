/***************************************************************************
 *   file klflibbrowser_p.h
 *   This file is part of the KLatexFormula Project.
 *   Copyright (C) 2010 by Philippe Faist
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

/** \file
 * This header contains (in principle _private_) definitions for klflibbrowser.cpp
 */


#ifndef KLFLIBBROWSER_P_H
#define KLFLIBBROWSER_P_H

#include <QWidget>
#include <QStackedWidget>
#include <QTabBar>
#include <QTabWidget>

#include "klflibview.h"


class KLFLibBrowserViewContainer : public QStackedWidget
{
  Q_OBJECT
public:
  KLFLibBrowserViewContainer(KLFLibResourceEngine *resource, QTabWidget *parent)
    : QStackedWidget(parent), pResource(resource)
  {
    // find OK view type identifiers
    QStringList allViewTypeIdents = KLFAbstractLibViewFactory::allSupportedViewTypeIdentifiers();
    int k;
    for (k = 0; k < allViewTypeIdents.size(); ++k) {
      KLFAbstractLibViewFactory *factory =
	KLFAbstractLibViewFactory::findFactoryFor(allViewTypeIdents[k]);
      if (factory->canCreateLibView(allViewTypeIdents[k], pResource))
	pOkViewTypeIdents << allViewTypeIdents[k];
    }
  }
  virtual ~KLFLibBrowserViewContainer()
  {
  }

  KLFAbstractLibView * view() { return qobject_cast<KLFAbstractLibView*>(currentWidget()); }
  
  QStringList supportedViewTypeIdentifiers() const { return pOkViewTypeIdents; }

protected:
  QStringList pOkViewTypeIdents;
  KLFLibResourceEngine *pResource;
};


// ---


class KLFLibBrowserTabWidget : public QTabWidget
{
  Q_OBJECT
public:
  KLFLibBrowserTabWidget(QWidget *parent) : QTabWidget(parent)
  {
    setUsesScrollButtons(false);
  }
  virtual ~KLFLibBrowserTabWidget() { }

  /** Returns the current KLFAbstractLibView widget at index \c index.
   * The tab widget at the given index MUST be a KLFLibBrowserViewContainer.
   * \code
   *  KLFAbstractLibView *view = tabs->viewWidget(index);
   *  // equivalent in idea to
   *  KLFAbstractLibView *view = ((KLFLibBrowserViewContainer*)tabs->widget(index))->view();
   * \endcode
   * \param index the tab index. If index is -1, returns NULL. If index is
   *   -2, returns the current widget
   * \note Returns NULL if the respective tab widget is not a KLFLibBrowserViewContainer. */
  KLFAbstractLibView *viewWidget(int index = -2) {
    QWidget *w = NULL;
    if (index == -1) return NULL;
    if (index == -2)
      w = currentWidget();
    else
      w = widget(index);
    KLFLibBrowserViewContainer *v = qobject_cast<KLFLibBrowserViewContainer*>(w);
    if (v == NULL)
      return NULL;
    return v->view();
  }

  /** Returns the tab index at position \c pos relative to tab widget. */
  int getTabAtPoint(const QPoint& pos) {
    QTabBar *tbar = tabBar();
    QPoint tabbarpos = tbar->mapFromGlobal(mapToGlobal(pos));
    return tbar->tabAt(tabbarpos);
  }

  /** Returns the rectangle, relative to tab widget, occupied by tab in the tab bar. */
  QRect getTabRect(int tab) {
    QTabBar *tbar = tabBar();
    QRect tabbarrect = tbar->tabRect(tab);
    return QRect(mapFromGlobal(tbar->mapToGlobal(tabbarrect.topLeft())),
		 tabbarrect.size());
  }

protected:
};


#endif
