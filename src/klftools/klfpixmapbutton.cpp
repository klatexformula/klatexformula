/***************************************************************************
 *   file klfpixmapbutton.cpp
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

#include <QApplication>
#include <QPushButton>
#include <QStyleOption>
#include <QPainter>
#include <QPixmap>
#include <QStyle>
#include <QPaintEvent>

#include <klfdefs.h>
#include "klfpixmapbutton.h"



KLFPixmapButton::KLFPixmapButton(const QPixmap& pix, QWidget *parent)
  : QPushButton(parent), _pix(pix), _pixmargin(2), _xalignfactor(0.5f), _yalignfactor(0.5f), _pixscale(1.f)
{
  setText(QString());
  setIcon(QIcon());
  if (parent != NULL) {
    _pixscale = parent->devicePixelRatio();
  }
}

QSize KLFPixmapButton::minimumSizeHint() const
{
  return sizeHint();
}

QSize KLFPixmapButton::sizeHint() const
{
  // inspired by QPushButton::sizeHint() in qpushbutton.cpp

  ensurePolished();

  int w = 0, h = 0;
  QStyleOptionButton opt;
  initStyleOption(&opt);

  // calculate contents size...
  w = _pix.width() / _pixscale + _pixmargin;
  h = _pix.height() / _pixscale + _pixmargin;

  if (menu()) {
    w += style()->pixelMetric(QStyle::PM_MenuButtonIndicator, &opt, this);
  }

  return (style()->sizeFromContents(QStyle::CT_PushButton, &opt, QSize(w, h), this).
	  expandedTo(QApplication::globalStrut()).expandedTo(QSize(50, 30)));
  // (50,30) is minimum non-square buttons on Qt/Mac
}

void KLFPixmapButton::paintEvent(QPaintEvent *event)
{
  QPushButton::paintEvent(event);
  QPainter p(this);
  p.setClipRect(event->rect());
  QSizeF pixsz = _pix.size(); pixsz /= _pixscale;
  p.drawPixmap(QRectF(QPointF( _xalignfactor*(width()-(2*_pixmargin+pixsz.width())) + _pixmargin,
                               _yalignfactor*(height()-(2*_pixmargin+pixsz.height())) + _pixmargin ),
                      pixsz),
	       _pix,
               QRectF(QPointF(0,0), _pix.size()));
}



