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
#include <QEvent>
#include <QDragMoveEvent>

#include "klflibview.h"

/** \internal */
class KLFLibBrowserViewContainer : public QStackedWidget
{
  Q_OBJECT
public:
  KLFLibBrowserViewContainer(KLFLibResourceEngine *resource, QTabWidget *parent)
    : QStackedWidget(parent), pResource(resource)
  {
    if (pResource == NULL) {
      qWarning()<<"KLFLibBrowserViewContainer: NULL RESOURCE! Expect Imminent Crash!";
    }
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

  void setResourceRoleFlags(uint flags) {
    pResFlags = flags;
  }
  uint resourceRoleFlags() const { return pResFlags; }

  QVariantMap saveGuiState() const {
    QVariantMap v;
    QVariantList vlist; // will hold QVariantMap's of each view's state
    QString curViewTypeIdent;
    for (QMap<QString,int>::const_iterator cit = pOpenViewTypeIdents.begin();
	 cit != pOpenViewTypeIdents.end(); ++cit) {
      KLFAbstractLibView *view = qobject_cast<KLFAbstractLibView*>(widget(*cit));
      QString vti = cit.key(); // view type identifier
      if (view == currentWidget()) // this is current view -> save it in more global setting
	curViewTypeIdent = vti;
      QVariantMap vstate;
      vstate["ViewTypeIdentifier"] = vti;
      vstate["ViewState"] = QVariant::fromValue<QVariantMap>(view->saveGuiState());
      vlist << QVariant::fromValue<QVariantMap>(vstate);
    }
    v["StateList"] = QVariant::fromValue<QVariantList>(vlist);
    v["CurrentViewTypeIdentifier"] = QVariant::fromValue<QString>(curViewTypeIdent);
    return v;
  }
  void loadGuiState(const QVariantMap& v) {
    const QVariantList vlist = v["StateList"].toList(); // will hold QVariantMap's
    const QString curvti = v["CurrentViewTypeIdentifier"].toString();
    int k;
    for (k = 0; k < vlist.size(); ++k) {
      const QVariantMap vstate = vlist[k].toMap();
      QString vti = vstate["ViewTypeIdentifier"].toString();
      bool res = openView(vti);
      if (!res) {
	qWarning()<<"Can't open view of type "<<vti<<" for resource "<<url()<<"!";
	continue;
      }
      KLFAbstractLibView *view
	= qobject_cast<KLFAbstractLibView*>(widget(pOpenViewTypeIdents.value(vti)));
      if (view == NULL)
	continue;
      view->restoreGuiState(vstate["ViewState"].toMap());
    }
    
    // raise current view ...
    if (!curvti.isEmpty())
      openView(curvti);
  }

public slots:
  bool openView(const QString& viewTypeIdent) {
    // see if we already have this view type open and ready
    if (pOpenViewTypeIdents.contains(viewTypeIdent)) {
      setCurrentIndex(pOpenViewTypeIdents[viewTypeIdent]);
      return true;
    }
    // otherwise, instanciate view
    KLFLibViewFactory *factory =
	KLFLibViewFactory::findFactoryFor(viewTypeIdent);
    KLFAbstractLibView *v = factory->createLibView(viewTypeIdent, this, pResource);
    if (v == NULL) {
      qWarning() << "The factory can't open a view of type "<<viewTypeIdent<<"!";
      return false;
    }
    v->setContextMenuPolicy(Qt::CustomContextMenu);
    // get informed about selection changes
    connect(v, SIGNAL(entriesSelected(const KLFLibEntryList& )),
	    this, SLOT(slotEntriesSelected(const KLFLibEntryList& )));
    // and of new category suggestions
    connect(v, SIGNAL(moreCategorySuggestions(const QStringList&)),
	    this, SLOT(slotMoreCategorySuggestions(const QStringList&)));
    // and of restore actions
    connect(v, SIGNAL(requestRestore(const KLFLibEntry&, uint)),
	    this, SLOT(slotRequestRestore(const KLFLibEntry&, uint)));
    connect(v, SIGNAL(requestRestoreStyle(const KLFStyle&)),
	    this, SLOT(slotRequestRestoreStyle(const KLFStyle&)));
    // and of after-change refresh
    connect(v, SIGNAL(resourceDataChanged(const QList<KLFLib::entryId>&)),
	    this, SLOT(slotResourceDataChanged(const QList<KLFLib::entryId>&)));
    // and of context menu request
    connect(v, SIGNAL(customContextMenuRequested(const QPoint&)),
	    this, SIGNAL(viewContextMenuRequested(const QPoint&)));
    qDebug()<<"connected signals.";
    pResource->setViewType(viewTypeIdent);
    qDebug()<<"set view type.";

    int index = addWidget(v);
    pOpenViewTypeIdents[viewTypeIdent] = index;
    qDebug()<<"added widget, about to raise";
    setCurrentIndex(index);
    qDebug()<<"Added view.";
    return true;
  }


signals:
  void viewContextMenuRequested(const QPoint& pos);

  void requestRestore(const KLFLibEntry& entry, uint restoreflags = KLFLib::RestoreLatexAndStyle);
  void requestRestoreStyle(const KLFStyle& style);

  void resourceDataChanged(const QList<KLFLib::entryId>& entryIdList);

  void entriesSelected(const KLFLibEntryList& entries);
  void moreCategorySuggestions(const QStringList& categorylist);

protected slots:
  void slotRequestRestore(const KLFLibEntry& entry, uint restoreflags = KLFLib::RestoreLatexAndStyle) {
    if (sender() == view())
      emit requestRestore(entry, restoreflags);
  }
  void slotRequestRestoreStyle(const KLFStyle& style) {
    if (sender() == view())
      emit requestRestoreStyle(style);
  }
  void slotResourceDataChanged(const QList<KLFLib::entryId>& entryIdList) {
    if (sender() == view())
      emit resourceDataChanged(entryIdList);
  }
  void slotEntriesSelected(const KLFLibEntryList& entries) {
    if (sender() == view())
      emit entriesSelected(entries);
  }
  void slotMoreCategorySuggestions(const QStringList& categorylist) {
    if (sender() == view())
      emit moreCategorySuggestions(categorylist);
  }

  void slotCurrentChanged(int /*index*/) {
    KLFAbstractLibView *v = view();
    if (v == NULL) return;
    emit entriesSelected(v->selectedEntries());
  }

protected:
  QStringList pOkViewTypeIdents;
  KLFLibResourceEngine *pResource;

  /** Stores the view type identifier with the index in widget stack for each open view */
  QMap<QString,int> pOpenViewTypeIdents;

  uint pResFlags;
};


// ------------------------


/** \internal */
class KLFLibBrowserTabWidget : public QTabWidget
{
  Q_OBJECT
public:
  KLFLibBrowserTabWidget(QWidget *parent) : QTabWidget(parent)
  {
    setUsesScrollButtons(false);
    pTabBar = new QTabBar(this);
    pTabBar->setAcceptDrops(true);
    pTabBar->installEventFilter(this);
    setTabBar(pTabBar);

    setProperty("normalColor", QVariant::fromValue(QColor(0,0,0)));
    setProperty("readOnlyColor", QVariant::fromValue(QColor(80,80,120)));
  }
  virtual ~KLFLibBrowserTabWidget() { }

  /** Returns the tab index at position \c pos relative to tab widget. */
  int getTabAtPoint(const QPoint& pos) {
    QPoint tabbarpos = pTabBar->mapFromGlobal(mapToGlobal(pos));
    return pTabBar->tabAt(tabbarpos);
  }

  /** Returns the rectangle, relative to tab widget, occupied by tab in the tab bar. */
  QRect getTabRect(int tab) {
    QRect tabbarrect = pTabBar->tabRect(tab);
    return QRect(mapFromGlobal(pTabBar->mapToGlobal(tabbarrect.topLeft())),
		 tabbarrect.size());
  }

  bool eventFilter(QObject *object, QEvent *event)
  {
    if (object == pTabBar && event->type() == QEvent::DragEnter) {
      QDragEnterEvent *de = (QDragEnterEvent*)event;
      de->accept();
      return true;
    }
    if (object == pTabBar && event->type() == QEvent::DragMove) {
      // ignore event, but raise the underlying tab.
      QDragMoveEvent *de = (QDragMoveEvent*)event;
      QPoint pos = de->pos();
      de->ignore();
      int index = getTabAtPoint(pos);
      if (index >= 0)
	setCurrentIndex(index);
      return true;
    }
    return QTabWidget::eventFilter(object, event);
  }


public slots:

  void refreshTabReadOnly(int tabindex, bool readonly) {
    if (readonly)
      pTabBar->setTabTextColor(tabindex, property("readOnlyColor").value<QColor>());
    else
      pTabBar->setTabTextColor(tabindex, property("normalColor").value<QColor>());
  }

protected:

  void tabInserted(int index) {
    return QTabWidget::tabInserted(index);
  }

private:
  QTabBar *pTabBar;
};


#endif
