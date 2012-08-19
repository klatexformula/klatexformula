/***************************************************************************
 *   file klfitemviewsearchtarget.h
 *   This file is part of the KLatexFormula Project.
 *   Copyright (C) 2012 by Philippe Faist
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


#ifndef KLFITEMVIEWSEARCHTARGET_H
#define KLFITEMVIEWSEARCHTARGET_H

#include <QAbstractItemModel>
#include <QAbstractItemView>

#include <klfdefs.h>
#include <klfsearchbar.h>
#include <klfiteratorsearchable.h>


class KLFItemViewSearchTargetPrivate;

//! A search target (for KLFSearchBar) for standard item views
/** Add search functionality to standard item views. Any item view you may have (QTreeView/QListView etc.)
 * may be added search functionality with KLFSearchBar, using this class as search target.
 *
 * For basic usage, you should not need to interact more with this class than simply instantiating it and
 * feeding it to KLFSearchBar as the search target with KLFSearchBar::setSearchTarget().
 *
 * Matches are displayed in highlighted red font, and current found item is displayed as selected.
 *
 * Minimal example:
 * \code
 *   QTableView * view = ...;
 *   KLFSearchBar * searchBar = new KLFSearchBar(...);
 *   KLFItemViewSearchTarget * searchTarget = new KLFItemViewSearchTarget(view, this);
 *   searchBar->setSearchTarget(searchTarget);
 * \endcode
 *
 * \todo Currently, to highlight search matches this class replaces the delegate with a custom delegate used
 *   to highlight search matches, thus relying on the fact that the default delegate is close to a standard
 *   QItemDelegate. TODO: define some "proxy" delegate, and just extend the delegate that is already set on
 *   the view.
 */
class KLFItemViewSearchTarget : public QObject, public KLFIteratorSearchable<QModelIndex>
{
  Q_OBJECT
public:
  KLFItemViewSearchTarget(QAbstractItemView * view, QObject *parent = NULL);
  virtual ~KLFItemViewSearchTarget();

  QAbstractItemView * view() ;
  QList<int> searchColumns() ;

  virtual QModelIndex searchIterAdvance(const QModelIndex &pos, bool forward);
  virtual QModelIndex searchIterBegin();
  virtual QModelIndex searchIterEnd();

  virtual bool searchIterMatches(const QModelIndex &pos, const QString &queryString);
  virtual void searchPerformed(const QModelIndex& resultMatchPosition, bool found,
			       const QString& queryString);
  virtual void searchAborted();
  virtual void searchReinitialized();
  virtual void searchMoveToIterPos(const QModelIndex& pos);

  void setSearchView(QAbstractItemView *view);
  void setSearchColumns(const QList<int>& columnList);
private:
  KLF_DECLARE_PRIVATE(KLFItemViewSearchTarget) ;
};






#endif
