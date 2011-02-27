/***************************************************************************
 *   file klflibdbengine_p.h
 *   This file is part of the KLatexFormula Project.
 *   Copyright (C) 2011 by Philippe Faist
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
 * library routines defined in klflibdbengine.cpp */


#ifndef KLFLIBDBENGINE_P_H
#define KLFLIBDBENGINE_P_H

#include <QObject>


/** \internal */
class KLFLibDBEnginePropertyChangeNotifier : public QObject
{
  Q_OBJECT
public:
  KLFLibDBEnginePropertyChangeNotifier(const QString& dbname, QObject *parent)
    : QObject(parent), pDBName(dbname), pRef(0) { }
  virtual ~KLFLibDBEnginePropertyChangeNotifier() { }

  void ref() { ++pRef; }
  //! Returns TRUE if the object needs to be deleted.
  bool deRef() { return !--pRef; }

  void notifyResourcePropertyChanged(int propId)
  {
    emit resourcePropertyChanged(propId);
  }

  void notifySubResourcePropertyChanged(const QString& subResource, int propId)
  {
    emit subResourcePropertyChanged(subResource, propId);
  }

signals:
  void resourcePropertyChanged(int propId);
  void subResourcePropertyChanged(const QString& subResource, int propId);

private:
  QString pDBName;
  int pRef;
};





#endif
