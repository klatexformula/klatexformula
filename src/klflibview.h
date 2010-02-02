/***************************************************************************
 *   file klflibview.h
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

#ifndef KLFLIBVIEW_H
#define KLFLIBVIEW_H

#include <QAbstractItemModel>

#include <klflib.h>


/** \brief Model for Item-Views displaying a library resource's contents
 *
 * The Model can morph into different forms, for simulating various common & useful
 * displays (chronological list (history), category/tags tree (archive), maybe icons
 * in the future, ...).
 *
 */
class KLFLibModel : public QAbstractItemModel {
  Q_OBJECT
public:
  KLFLibModel(KLFLibResourceEngine *resource, QObject *parent = NULL);
  virtual ~KLFLibModel();

  /** For example use
   * \code
   *  model->data(index, model->entryItemRole(KLFLibEntry::Latex)).toString()
   * \endcode
   * to get LaTeX string for model index \c index.
   */
  static inline int entryItemRole(int propertyId) { return Qt::UserRole+800+propertyId; }
  
  void setResource(KLFLibResourceEngine *resource);

  enum FlavorFlags {
    LinearList = 0x0001,
    CategoryTree = 0x0002,
    DisplayTypeMask = 0x000f,

    SortByDate = 0x0100,
    SortByLatex = 0x0200,
    SortByTags = 0x0400,
    SortMask = 0x0f00
  };
  void setFlavorFlags(uint flags, uint modify_mask = 0xffffffff);
  uint flavorFlags() const;


  QVariant data(const QModelIndex& index, int role) const;
  Qt::ItemFlags flags(const QModelIndex& index) const;
  QVariant headerData(int section, Qt::Orientation orientation,
		      int role = Qt::DisplayRole) const;
  QModelIndex index(int row, int column,
		    const QModelIndex &parent = QModelIndex()) const;
  QModelIndex parent(const QModelIndex &index) const;
  int rowCount(const QModelIndex &parent = QModelIndex()) const;
  int columnCount(const QModelIndex &parent = QModelIndex()) const;


private:

  typedef QList<KLFLibResourceEngine::KLFLibEntryWithId> EntryCache;

  KLFLibResourceEngine *pResource;

  unsigned int pFlavorFlags;
  EntryCache pEntryCache;

  void updateEntryCacheSetupModel();

  int cacheFindEntry(KLFLibResourceEngine::entryId id) const;
};



#endif
