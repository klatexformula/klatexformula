/***************************************************************************
 *   file klfpobjeditwidget_p.h
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

/** \file
 * This header contains (in principle private) auxiliary classes for
 * library routines defined in ****.cpp */

#ifndef KLFPOBJEDITWIDGET_P_H
#define KLFPOBJEDITWIDGET_P_H

#include <QVariant>
#include <QAbstractItemModel>

#include "klfpobj.h"
#include "klfpobjeditwidget.h"

class KLFPObjModelPrivate;

class /*KLF_EXPORT*/ KLFPObjModel : public QAbstractItemModel
{
  Q_OBJECT
public:

  KLFPObjModel(KLFAbstractPropertizedObject *pobj, QObject *parent = NULL);
  virtual ~KLFPObjModel();

  virtual void setPObj(KLFAbstractPropertizedObject *pobj);

  virtual KLFAbstractPropertizedObject * pObj();

  virtual QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const;
  virtual Qt::ItemFlags flags(const QModelIndex& index) const;
  virtual bool hasChildren(const QModelIndex &parent = QModelIndex()) const;
  virtual QVariant headerData(int section, Qt::Orientation orientation,
			      int role = Qt::DisplayRole) const;
  virtual bool hasIndex(int row, int column,
			const QModelIndex &parent = QModelIndex()) const;
  virtual QModelIndex index(int row, int column,
			    const QModelIndex &parent = QModelIndex()) const;
  virtual QModelIndex parent(const QModelIndex &index) const;
  virtual int rowCount(const QModelIndex &parent = QModelIndex()) const;
  virtual int columnCount(const QModelIndex &parent = QModelIndex()) const;

  virtual bool canFetchMore(const QModelIndex& parent) const;
  virtual void fetchMore(const QModelIndex& parent);

public slots:
  void updateData();

private:
  
  KLF_DECLARE_PRIVATE(KLFPObjModel) ;
};



struct KLFPObjModelPrivate {
  KLF_PRIVATE_HEAD(KLFPObjModel) {
    pObj = NULL;
  }

  KLFAbstractPropertizedObject *pObj;

  QStringList propertyNames;
};










#endif

