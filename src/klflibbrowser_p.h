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

/** \internal */
class KLFLibBrowserViewContainer : public QStackedWidget
{
  Q_OBJECT
public:
  KLFLibBrowserViewContainer(KLFLibResourceEngine *resource, QTabWidget *parent)
    : QStackedWidget(parent), pResource(resource)
  {
    // find OK view type identifiers
    QStringList allViewTypeIdents = KLFLibViewFactory::allSupportedViewTypeIdentifiers();
    int k;
    for (k = 0; k < allViewTypeIdents.size(); ++k) {
      KLFLibViewFactory *factory =
	KLFLibViewFactory::findFactoryFor(allViewTypeIdents[k]);
      if (factory->canCreateLibView(allViewTypeIdents[k], pResource))
	pOkViewTypeIdents << allViewTypeIdents[k];
    }

    connect(this, SIGNAL(currentChanged(int)), this, SLOT(slotCurrentChanged(int)));
  }
  virtual ~KLFLibBrowserViewContainer()
  {
  }

  QUrl url() const { return pResource->url(); }
  KLFLibResourceEngine * resourceEngine() { return pResource; }

  KLFAbstractLibView * view() { return qobject_cast<KLFAbstractLibView*>(currentWidget()); }

  
  QStringList supportedViewTypeIdentifiers() const { return pOkViewTypeIdents; }

public slots:
  bool openView(const QString& viewTypeIdent) {
    if (pOpenViewTypeIdents.contains(viewTypeIdent)) {
      setCurrentIndex(pOpenViewTypeIdents[viewTypeIdent]);
      return true;
    }
    // instanciate view
    KLFLibViewFactory *factory =
	KLFLibViewFactory::findFactoryFor(viewTypeIdent);
    KLFAbstractLibView *v = factory->createLibView(viewTypeIdent, this, pResource);
    if (v == NULL) {
      return false;
    }
    v->setContextMenuPolicy(Qt::CustomContextMenu);
    // get informed about selection changes
    connect(v, SIGNAL(entriesSelected(const KLFLibEntryList& )),
	    this, SIGNAL(entriesSelected(const KLFLibEntryList& )));
    // and of new category suggestions
    connect(v, SIGNAL(moreCategorySuggestions(const QStringList&)),
	    this, SIGNAL(moreCategorySuggestions(const QStringList&)));
    // and of restore actions
    connect(v, SIGNAL(requestRestore(const KLFLibEntry&, uint)),
	    this, SIGNAL(requestRestore(const KLFLibEntry&, uint)));
    connect(v, SIGNAL(requestRestoreStyle(const KLFStyle&)),
	    this, SIGNAL(requestRestoreStyle(const KLFStyle&)));
    connect(v, SIGNAL(customContextMenuRequested(const QPoint&)),
	    this, SIGNAL(viewContextMenuRequested(const QPoint&)));

    int index = addWidget(v);
    pOpenViewTypeIdents[viewTypeIdent] = index;
    setCurrentIndex(index);
    return true;
  }


signals:
  void viewContextMenuRequested(const QPoint& pos);

  void requestRestore(const KLFLibEntry& entry, uint restoreflags = KLFLib::RestoreLatexAndStyle);
  void requestRestoreStyle(const KLFStyle& style);

  void entriesSelected(const KLFLibEntryList& entries);
  void moreCategorySuggestions(const QStringList& categorylist);

protected slots:
  void slotCurrentChanged(int /*index*/) {
    KLFAbstractLibView *v = view();
    if (v == NULL) return;
    emit entriesSelected(v->selectedEntries());
  }

protected:
  QStringList pOkViewTypeIdents;
  KLFLibResourceEngine *pResource;

  QMap<QString,int> pOpenViewTypeIdents;
};


// ---

/** \internal */
class KLFLibBrowserTabWidget : public QTabWidget
{
  Q_OBJECT
public:
  KLFLibBrowserTabWidget(QWidget *parent) : QTabWidget(parent)
  {
    setUsesScrollButtons(false);
  }
  virtual ~KLFLibBrowserTabWidget() { }

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
