/***************************************************************************
 *   file klflibview.cpp
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

#include <QDebug>
#include <QAbstractItemModel>
#include <QModelIndex>

#include "klflibview.h"


KLFLibModel::KLFLibModel(KLFLibResourceEngine *engine, QObject *parent)
  : QAbstractItemModel(parent)
{
  setResource(engine);
}

KLFLibModel::~KLFLibModel()
{
}

void KLFLibModel::setResource(KLFLibResourceEngine *resource)
{
  pResource = resource;
  updateEntryCacheSetupModel();
}


void KLFLibModel::setFlavorFlags(uint flags, uint modify_mask)
{
  // ...
}
uint KLFLibModel::flavorFlags() const
{
  return pFlavorFlags;
}

QVariant KLFLibModel::data(const QModelIndex& index, int role) const
{
  KLFLibResourceEngine::entryId id = index.internalId();

  // index not valid
  int ind = cacheFindEntry(id);
  if ( ! index.isValid()  ||  ind == -1 )
    return QVariant();

  KLFLibEntry entry = pEntryCache[ind].entry;

  if (role == Qt::DisplayRole)
    role = entryItemRole(KLFLibEntry::Latex);

  if (role == entryItemRole(KLFLibEntry::Latex))
    return entry.latex();
  if (role == entryItemRole(KLFLibEntry::DateTime))
    return entry.dateTime();
  if (role == entryItemRole(KLFLibEntry::Preview))
    return entry.preview();
  if (role == entryItemRole(KLFLibEntry::Category))
    return entry.category();
  if (role == entryItemRole(KLFLibEntry::Tags))
    return entry.tags();
  if (role == entryItemRole(KLFLibEntry::Style))
    return QVariant::fromValue(entry.style());

  // default
  return QVariant();
}
Qt::ItemFlags KLFLibModel::flags(const QModelIndex& index) const
{
  return Qt::ItemIsSelectable | Qt::ItemIsEnabled;
}
QVariant KLFLibModel::headerData(int section, Qt::Orientation orientation, int role) const
{
  if (orientation == Qt::Horizontal)
    return QString("Library Entry...");
  return QString("hdr>");
}
QModelIndex KLFLibModel::index(int row, int column,
			       const QModelIndex &parent) const
{
  if (row >= 0 && row < pEntryCache.size()) {
    return createIndex(row, 0, pEntryCache[row].id);
  }
  return QModelIndex();
}
QModelIndex KLFLibModel::parent(const QModelIndex &index) const
{
  return QModelIndex();
}
int KLFLibModel::rowCount(const QModelIndex &parent) const
{
  return pEntryCache.size();
}
int KLFLibModel::columnCount(const QModelIndex &parent) const
{
  return 1;
}


void KLFLibModel::updateEntryCacheSetupModel()
{
  pEntryCache = pResource->allEntries();
}

int KLFLibModel::cacheFindEntry(KLFLibResourceEngine::entryId id) const
{
  int k;
  for (k = 0; k < pEntryCache.size(); ++k)
    if (pEntryCache[k].id == id)
      return k;
  return -1;
}


#include <QMessageBox>
#include <QSqlDatabase>
#include <QListView>

void klf___temp___test_newlib()
{
  QSqlDatabase db = QSqlDatabase::addDatabase("QSQLITE");
  db.setDatabaseName("/home/philippe/temp/klf_sqlite_test");
  if (!db.open()) {
    QMessageBox::critical(0, "Cannot open database",
			  "Unable to establish a database connection.",
			  QMessageBox::Ok);
    return;
  }
  
  KLFLibDBEngine *engine = new KLFLibDBEngine(db);
  KLFLibModel *model = new KLFLibModel(engine);

  QListView *view = new QListView(0);
  view->setModel(model);
  view->show();
}



