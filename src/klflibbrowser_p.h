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
#include <QTabBar>
#include <QTabWidget>

/*
class KLFLibBrowserViewContainer : public QStackedWidget
{
  Q_OBJECT
public:
  KLFLibBrowserViewContainer(KLFLibResourceEngine *resource, QTabWidget *parent)
  {
  }
  virtual ~KLFLibBrowserViewContainer()
  {
  }

  

protected:

};*/


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
