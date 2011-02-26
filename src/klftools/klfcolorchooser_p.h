/***************************************************************************
 *   file klfcolorchooser_p.h
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

#ifndef KLFCOLORCHOOSER_P_H
#define KLFCOLORCHOOSER_P_H

#include "klfcolorchooser.h"

#define MAX_RECENT_COLORS 128


class KLF_EXPORT KLFColorList : public QObject
{
  Q_OBJECT

  Q_PROPERTY(int maxSize READ maxSize WRITE setMaxSize)
public:
  KLFColorList(int maxsize) : QObject(qApp) { _maxsize = maxsize; }

  int maxSize() const { return _maxsize; }

  QList<QColor> list;

signals:
  void listChanged();

public slots:
  void setMaxSize(int maxsize) { _maxsize = maxsize; }
  void addColor(const QColor& color);
  void removeColor(const QColor& color);
  void notifyListChanged() { emit listChanged(); }

private:
  int _maxsize;
};


#endif
