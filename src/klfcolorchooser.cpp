/***************************************************************************
 *   file klfcolorchooser.cpp
 *   This file is part of the KLatexFormula Project.
 *   Copyright (C) 2008 by Philippe Faist
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

#include <QColorDialog>

#include "klfcolorchooser.h"

KLFColorChooser::KLFColorChooser(QWidget *parent)
  : QPushButton(parent), _color(0,0,0), _pix(), _size(50, 20)
{
  _setpix();

  connect(this, SIGNAL(clicked()), this, SLOT(requestColor()));
}


KLFColorChooser::~KLFColorChooser()
{
}


QColor KLFColorChooser::color() const
{
  return _color;
}

void KLFColorChooser::setColor(const QColor& col)
{
  if ( ! col.isValid() )
    return;

  _color = col;
  _setpix();
}

void KLFColorChooser::requestColor()
{
  QColor col = QColorDialog::getColor(_color, this);
  if ( ! col.isValid() )
    return;

  setColor(col);
}

void KLFColorChooser::_setpix()
{
  _pix = QPixmap(_size);
  _pix.fill(_color);

  setIconSize(_pix.size());
  setIcon(_pix);
}



