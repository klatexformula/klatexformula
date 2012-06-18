/***************************************************************************
 *   file klfitemviewsearchtarget.cpp
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

#include <QList>
#include <QMap>
#include <QAbstractItemView>
#include <QAbstractItemModel>

#include "klfitemviewsearchtarget.h"


struct KLFItemViewSearchTargetPrivate {
  KLF_PRIVATE_HEAD(KLFItemViewSearchTarget)
  {
    view = NULL;
  }

  QAbstractItemView * view;

  QList<int> columnlist;
  QMap<int,int> nextcolumn;
  QMap<int,int> prevcolumn;

  int next_valid_column(int c, bool forward = true)
  {
    if (columnlist.isEmpty())
      return -1;

    while (c >= 0 && c < columnlist.last() && !columnlist.contains(c))
      c = c + (forward ? +1 : -1);
    if (c >= columnlist.last())
      c = -1;
    return c;
  }

  QModelIndex advance_iter_helper(const QModelIndex& pos, bool explorechildren = true)
  {
    KLF_ASSERT_NOT_NULL(view, "View is NULL!", return QModelIndex(); );
    QAbstractItemModel *model = view->model();
    KLF_ASSERT_NOT_NULL(model, "View's model is NULL!", return QModelIndex(); );
    
    if (columnlist.isEmpty())
      return QModelIndex();

    if (explorechildren && model->hasChildren(pos)) {
      // explore children
      return model->index(0, columnlist[0], pos);
    }
    // no children, move to next column
    int nextcol = nextcolumn.value(next_valid_column(pos.column()), -1);
    if (nextcol >= 0) {
      // valid next column to move to
      return model->index(pos.row(), nextcol, pos.parent());
    }
    // we're at last column, try next row
    if (pos.row() < model->rowCount(pos.parent())) {
      return pos.sibling(pos.row()+1, columnlist[0]);
    }
    // otherwise, we're at the end of (sub-?)list
    // try parent
    if (pos.parent() != QModelIndex()) {
      return advance_iter_helper(pos.parent(), false); // don't re-iterate to children...
    }
    // we seem to be at the end of the list...
    return QModelIndex();
  }

  QModelIndex last_child_index(const QModelIndex& pos)
  {
    KLF_ASSERT_NOT_NULL(view, "View is NULL!", return QModelIndex(); );
    QAbstractItemModel *model = view->model();
    KLF_ASSERT_NOT_NULL(model, "View's model is NULL!", return QModelIndex(); );
    
    if (!model->hasChildren(pos))
      return pos;

    // children: return last node of last child.
    return last_child_index(model->index(model->rowCount(pos)-1, columnlist.last(), pos));
  }

  QModelIndex advance_iter_back_helper(const QModelIndex& pos)
  {
    KLF_ASSERT_NOT_NULL(view, "View is NULL!", return QModelIndex(); );
    QAbstractItemModel *model = view->model();
    KLF_ASSERT_NOT_NULL(model, "View's model is NULL!", return QModelIndex(); );

    // try to go to previous column
    int prevcol = prevcolumn.value(next_valid_column(pos.column(), false), -1);
    if (prevcol >= 0) {
      return last_child_index(pos.sibling(pos.row(), prevcol));
    }
    if (pos.row() >= 1) {
      // there is a previous sibling
      return last_child_index(pos.sibling(pos.row()-1, columnlist.last()));
    }
    // there is no previous sibling: so we can return the parent
    return pos.parent();
  }
};


KLFItemViewSearchTarget::KLFItemViewSearchTarget(QAbstractItemView *view, QObject *parent)
  : QObject(parent), KLFIteratorSearchable<QModelIndex>()
{
  KLF_INIT_PRIVATE(KLFItemViewSearchTarget) ;
  setSearchView(view);
}
KLFItemViewSearchTarget::~KLFItemViewSearchTarget()
{
  KLF_DELETE_PRIVATE ;
}


QAbstractItemView * KLFItemViewSearchTarget::view()
{
  return d->view;
}
QList<int> KLFItemViewSearchTarget::searchColumns()
{
  return d->columnlist;
}




QModelIndex KLFItemViewSearchTarget::searchIterAdvance(const QModelIndex& pos, bool forward)
{
  if (forward)
    return d->advance_iter_helper(pos);
  // backwards search
  return d->advance_iter_back_helper(pos);
}
QModelIndex KLFItemViewSearchTarget::searchIterBegin()
{
  return d->advance_iter_helper(QModelIndex());
}
QModelIndex KLFItemViewSearchTarget::searchIterEnd()
{
  return QModelIndex();
}
bool KLFItemViewSearchTarget::searchIterMatches(const QModelIndex &pos, const QString &queryString)
{
  KLF_DEBUG_BLOCK(KLF_FUNC_NAME) ;

  bool has_uppercase = queryString.contains(QRegExp("[A-Z]"));
  if (pos.data().toString().contains(queryString, has_uppercase ? Qt::CaseSensitive : Qt::CaseInsensitive))
    return true;
  return false;
}
void KLFItemViewSearchTarget::searchPerformed(const QModelIndex& resultMatchPosition, bool found,
					      const QString& queryString)
{
  KLF_DEBUG_BLOCK(KLF_FUNC_NAME) ;

  //   KLF_ASSERT_NOT_NULL(d->view, "View is NULL!", return ; );
  //   QAbstractItemModel * model = d->view->model();
  //   KLF_ASSERT_NOT_NULL(model, "View has NULL model!!!", return ; ) ;

  /* * \warning When search results are displayed, some item properties are set in the model itself;
   * this is seen by views/delegates as edits! */
  /** \todo highlight all matches, leave info for the delegate etc. ..... */

  //  model->setData(resultMatchPosition, QColor(128,255,255,128), Qt::BackgroundRole);
}
void KLFItemViewSearchTarget::searchMoveToIterPos(const QModelIndex& pos)
{
  KLF_DEBUG_BLOCK(KLF_FUNC_NAME) ;

  KLF_ASSERT_NOT_NULL(d->view, "View is NULL!", return ; );
  d->view->setCurrentIndex(pos);
  d->view->selectionModel()->select(pos, QItemSelectionModel::ClearAndSelect);
}

void KLFItemViewSearchTarget::setSearchView(QAbstractItemView *view)
{
  KLF_DEBUG_BLOCK(KLF_FUNC_NAME) ;

  d->view = view;

  if (d->columnlist.isEmpty()) {
    // empty list means all columns
    setSearchColumns(QList<int>());
  }
}

void KLFItemViewSearchTarget::setSearchColumns(const QList<int>& columnList)
{
  KLF_DEBUG_BLOCK(KLF_FUNC_NAME) ;

  KLF_ASSERT_NOT_NULL(d->view, "View is NULL!", return ; );
  QAbstractItemModel * model = d->view->model();
  KLF_ASSERT_NOT_NULL(model, "View has NULL model!!!", return ; ) ;

  // set-list conversion to remove duplicates, and sorted so it's nice :)
  d->columnlist = columnList.toSet().toList();
  qSort(d->columnlist);
  d->nextcolumn.clear();
  d->prevcolumn.clear();
  if (d->columnlist.isEmpty()) {
    for (int k = 0; k < model->columnCount(); ++k)
      d->columnlist << k;
  }
  if (d->columnlist.isEmpty()) // model has no columns, don't insist
    return;
  int k;

  for (k = 0; k < (d->columnlist.size()-1); ++k) {
    d->nextcolumn[d->columnlist[k]] = d->columnlist[k+1];
    d->prevcolumn[d->columnlist[k+1]] = d->columnlist[k];
  }
  d->nextcolumn[d->columnlist[k]] = -1;
  d->prevcolumn[d->columnlist[0]] = -1;
}



