/***************************************************************************
 *   file klfpixmapbutton.h
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

#ifndef KLFPIXMAPBUTTON_H
#define KLFPIXMAPBUTTON_H

#include <QPushButton>
#include <QPixmap>

#include <klfdefs.h>

/** A button that displays a pixmap */
class KLF_EXPORT KLFPixmapButton : public QPushButton
{
  Q_OBJECT
public:
  Q_PROPERTY(QPixmap pixmap READ pixmap WRITE setPixmap USER true)
  Q_PROPERTY(int pixmapMargin READ pixmapMargin WRITE setPixmapMargin)
  Q_PROPERTY(float pixXAlignFactor READ pixXAlignFactor WRITE setPixXAlignFactor)
  Q_PROPERTY(float pixYAlignFactor READ pixYAlignFactor WRITE setPixYAlignFactor)

  KLFPixmapButton(const QPixmap& pix, QWidget *parent = 0);
  virtual ~KLFPixmapButton() { }

  virtual QSize minimumSizeHint() const;
  virtual QSize sizeHint() const;

  virtual QPixmap pixmap() const { return _pix; }
  virtual int pixmapMargin() const { return _pixmargin; }
  virtual float pixXAlignFactor() const { return _xalignfactor; }
  virtual float pixYAlignFactor() const { return _yalignfactor; }

public slots:
  virtual void setPixmap(const QPixmap& pix) { _pix = pix; }
  virtual void setPixmapMargin(int pixels) { _pixmargin = pixels; }
  virtual void setPixXAlignFactor(float xalignfactor) { _xalignfactor = xalignfactor; }
  virtual void setPixYAlignFactor(float yalignfactor) { _yalignfactor = yalignfactor; }

protected:
  virtual void paintEvent(QPaintEvent *event);

private:
  QPixmap _pix;
  int _pixmargin;
  float _xalignfactor, _yalignfactor;
};







#endif
