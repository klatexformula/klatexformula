/***************************************************************************
 *   file klfmarginsselector.h
 *   This file is part of the KLatexFormula Project.
 *   Copyright (C) 2019 by Philippe Faist
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


#ifndef KLFMARGINSSELECTOR_H
#define KLFMARGINSSELECTOR_H


#include <Qt>
#include <QWidget>
#include <QPushButton>

#include <klfdefs.h>


struct KLFMarginsSelectorPrivate;

class KLF_EXPORT KLFMarginsSelector : public QPushButton
{
  Q_OBJECT
public:
  KLFMarginsSelector(QWidget * parent);
  ~KLFMarginsSelector();

  qreal topMargin();
  qreal rightMargin();
  qreal bottomMargin();
  qreal leftMargin();

public slots:
  void setMargins(qreal t, qreal r, qreal b, qreal l);

private:
  KLF_DECLARE_PRIVATE(KLFMarginsSelector) ;
};



#endif
