/***************************************************************************
 *   file klfpobjeditwidget.cpp
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


#include <QAbstractItemModel>
#include <QItemDelegate>

#include <klfdebug.h>

#include "klfpobj.h"
#include "klfdatautil.h"

#include "klfpobjeditwidget.h"
#include "klfpobjeditwidget_p.h"



// -----------------------------------------------


KLFPObjModel::KLFPObjModel(KLFAbstractPropertizedObject *pobj, QObject *parent)
  : QAbstractItemModel(parent)
{
  KLF_DEBUG_TIME_BLOCK(KLF_FUNC_NAME) ;

  KLF_INIT_PRIVATE(KLFPObjModel) ;

  setPObj(pobj);
}

KLFPObjModel::~KLFPObjModel()
{
  KLF_DEBUG_TIME_BLOCK(KLF_FUNC_NAME) ;

  KLF_DELETE_PRIVATE ;
}

void KLFPObjModel::setPObj(KLFAbstractPropertizedObject *pobj)
{
  d->pObj = pobj;
  if (d->pObj != NULL)
    d->propertyNames = d->pObj->propertyNameList();
  else
    d->propertyNames = QStringList();
  reset();
}

KLFAbstractPropertizedObject * KLFPObjModel::pObj()
{
  return d->pObj;
}

QVariant KLFPObjModel::data(const QModelIndex& index, int role) const
{
  KLF_DEBUG_TIME_BLOCK(KLF_FUNC_NAME) ;
  klfDbg( "\tindex="<<index<<"; role="<<role ) ;

  if (!index.isValid()) {
    klfDbg("Invalid index.") ;
    return QVariant();
  }

  int n = index.row();

  if (index.column() < 0 || index.column() > 1 ||
      n < 0 || n >= d->propertyNames.count()) {
    // out of range
    klfWarning("Index out of range: "<<index) ;
    return QVariant();
  }

  if (index.column() == 0) {
    // property name
    if (role == Qt::ToolTipRole || role == Qt::DisplayRole) {
      // current contents
      return d->propertyNames[n];
    }

  } else {
    // property value

    if (role == Qt::ToolTipRole || role == Qt::DisplayRole) {
      return QVariant(klfSaveVariantToText(d->pObj->property(d->propertyNames[n])));
    }
    if ( role == Qt::EditRole) {
      // current contents
      return d->pObj->property(d->propertyNames[n]);
    }
  }

  // by default
  return QVariant();
}

Qt::ItemFlags KLFPObjModel::flags(const QModelIndex& index) const
{
  KLF_DEBUG_TIME_BLOCK(KLF_FUNC_NAME) ;
  klfDbg( "\tindex="<<index ) ;

  if (!index.isValid()) {
    klfDbg("Invalid index.") ;
    return 0;
  }

  int n = index.row();

  if (index.column() < 0 || index.column() > 1 ||
      n < 0 || n >= d->propertyNames.count()) {
    // out of range
    klfWarning("Index out of range: "<<index) ;
    return 0;
  }

  if (index.column() == 0) {
    // property name
    return Qt::ItemIsSelectable | Qt::ItemIsEnabled;
  } else {
    // property value
    return Qt::ItemIsSelectable | Qt::ItemIsEnabled  | Qt::ItemIsEditable;
  }

  return 0;
}

bool KLFPObjModel::hasChildren(const QModelIndex &parent) const
{
  if (!parent.isValid())
    return true;
  return false;
}

QVariant KLFPObjModel::headerData(int section, Qt::Orientation orientation, int role) const
{
  if (orientation == Qt::Horizontal) {
    if (role == Qt::SizeHintRole) {
      if (section == 0) { // property name
	return QSize(100,30);
      } else { // property value
	return QSize(100,30);
      }
    }
    if (role == Qt::DisplayRole) {
      if (section == 0) {
	return tr("Property");
      } else {
	return tr("Value");
      }
    }
  }
  return QVariant();
}

bool KLFPObjModel::hasIndex(int row, int column, const QModelIndex &parent) const
{
  if (column < 0 || column > 1)
    return false;
  if (row < 0 || row >= d->propertyNames.size())
    return false;
  if (parent.isValid())
    return false;
  return true;
}

QModelIndex KLFPObjModel::index(int row, int column, const QModelIndex &parent) const
{
  if (!hasIndex(row, column, parent))
    return QModelIndex();

  return createIndex(row, column);
}

QModelIndex KLFPObjModel::parent(const QModelIndex &/*index*/) const
{
  return QModelIndex();
}

int KLFPObjModel::rowCount(const QModelIndex &parent) const
{
  if (parent.isValid())
    return 0;
  return d->propertyNames.size();
}

int KLFPObjModel::columnCount(const QModelIndex &parent) const
{
  if (parent.isValid())
    return 0;
  return 2;
}

bool KLFPObjModel::canFetchMore(const QModelIndex& /*parent*/) const
{
  return false;
}

void KLFPObjModel::fetchMore(const QModelIndex& /*parent*/)
{
  return;
}

void KLFPObjModel::updateData()
{
  setPObj(d->pObj);
}


// -------------------------------------------------


struct KLFPObjEditWidgetPrivate {
  KLF_PRIVATE_HEAD(KLFPObjEditWidget) {
    model = NULL;
    delegate = NULL;
  }

  KLFPObjModel * model;
  QItemDelegate * delegate;
};


KLFPObjEditWidget::KLFPObjEditWidget(KLFAbstractPropertizedObject *pobj, QWidget *parent)
  : QTreeView(parent)
{
  KLF_DEBUG_TIME_BLOCK(KLF_FUNC_NAME) ;

  KLF_INIT_PRIVATE(KLFPObjEditWidget) ;

  d->model = new KLFPObjModel(pobj, this);
  d->delegate = new QItemDelegate(this);

  setModel(d->model);
  setItemDelegate(d->delegate);
}

KLFPObjEditWidget::KLFPObjEditWidget(QWidget *parent)
  : QTreeView(parent)
{
  KLF_DEBUG_TIME_BLOCK(KLF_FUNC_NAME) ;

  KLF_INIT_PRIVATE(KLFPObjEditWidget) ;

  d->model = new KLFPObjModel(NULL, this);
  d->delegate = new QItemDelegate(this);

  setModel(d->model);
  setItemDelegate(d->delegate);
}


KLFPObjEditWidget::~KLFPObjEditWidget()
{
  KLF_DEBUG_TIME_BLOCK(KLF_FUNC_NAME) ;

  KLF_DELETE_PRIVATE ;
}

void KLFPObjEditWidget::setPObj(KLFAbstractPropertizedObject *pobj)
{
  d->model->setPObj(pobj);
}






