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

#include "klflibview.h"


KLFLibModel::KLFLibModel(KLFLibResourceEngine *engine, QObject *parent)
  : QObject(parent)
{
  setResource(engine);
}

KLFLibModel::~KLFLibModel()
{
}

void KLFLibModel::setResource(KLFLibResourceEngine *resource)
{
  pResource = resource;
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
  KLFLibResourceEntry::entryId id = index.internalId();

  // index not valid
  if ( ! index.isValid()  ||  ! pEntryCache.contains(id) )
    return QVariant();

  KLFLibEntry entry = pEntryCache[id];

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
    return entry.style();

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
  if (pEntryCache.contains(row))
    return createIndex(row, 0, row);
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


void KLFLibModel::updateCacheListSetupModel()
{
  QList<KLFLibEntryWithId> all = pResource->allEntries();
  int k;
  // clear cache
  pEntryCache = EntryCache();
  // walk full list and populate cache
  for (k = 0; k < all.size(); ++k) {
    pEntryCache[all.id] = all.entry;
  }
}




