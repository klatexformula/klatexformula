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

#include <QAction>
#include <QMenu>
#include <QPainter>
#include <QColorDialog>
#include <QPaintEvent>
#include <QStyle>
#include <QStyleOptionButton>

#include "klfconfig.h"
#include "klfcolorchooser.h"



void KLFColorList::addColor(const QColor& color)
{
  int i;
  if ( (i = list.indexOf(color)) >= 0 )
    list.removeAt(i);

  list.append(color);
  while (list.size() >= klfconfig.UI.maxUserColors)
    list.pop_front();

  emit listChanged();
}

// static
KLFColorList *KLFColorChooser::_colorlist = NULL;

KLFColorChooser::KLFColorChooser(QWidget *parent)
  : QPushButton(parent), _color(0,0,0), _pix(), _allowdefaultstate(false), _autoadd(true), _size(120, 20),
    _xalignfactor(0.5f), _yalignfactor(0.5f),
    mMenu(0)
{
  ensureColorListInstance();
  connect(_colorlist, SIGNAL(listChanged()), this, SLOT(_makemenu()));
  _makemenu();
  _setpix();
}


KLFColorChooser::~KLFColorChooser()
{
}


QColor KLFColorChooser::color() const
{
  return _color;
}

QSize KLFColorChooser::sizeHint() const
{
  // inspired by QPushButton::sizeHint() in qpushbutton.cpp

  ensurePolished();

  int w = 0, h = 0;
  QStyleOptionButton opt;
  initStyleOption(&opt);

  // calculate contents size...
  w = _pix.width()+4;
  h = _pix.height()+2;

  if (menu())
    w += style()->pixelMetric(QStyle::PM_MenuButtonIndicator, &opt, this);

  return (style()->sizeFromContents(QStyle::CT_PushButton, &opt, QSize(w, h), this).
	  expandedTo(QApplication::globalStrut()));
}

void KLFColorChooser::setColor(const QColor& col)
{
  if ( ! _allowdefaultstate && ! col.isValid() )
    return;

  _color = col;
  _setpix();

  if (_autoadd && _color.isValid()) {
    _colorlist->addColor(_color);
  }
}

void KLFColorChooser::setDefaultColor()
{
  setColor(QColor());
}

void KLFColorChooser::setAllowDefaultState(bool allow)
{
  _allowdefaultstate = allow;
  _makemenu();
}

void KLFColorChooser::requestColor()
{
  QColor col = QColorDialog::getColor(_color, this);
  if ( ! col.isValid() )
    return;

  setColor(col);
}

void KLFColorChooser::setSenderPropertyColor()
{
  QColor c = sender()->property("setColor").value<QColor>();
  setColor(c);
}

void KLFColorChooser::_makemenu()
{
  if (mMenu) {
    setMenu(0);
    mMenu->deleteLater();
  }

  QSize menuIconSize = QSize(16,16);

  mMenu = new QMenu(this);

  if (_allowdefaultstate) {
    mMenu->addAction(QIcon(colorPixmap(QColor(), menuIconSize)), tr("[ Default ]"), this, SLOT(setDefaultColor()));
    mMenu->addSeparator();
  }

  int n, k, nk;
  ensureColorListInstance();
  n = _colorlist->list.size();
  for (k = 0; k < n; ++k) {
    nk = n - k - 1;
    QAction *a = mMenu->addAction(QIcon(colorPixmap(_colorlist->list[nk], menuIconSize)), _colorlist->list[nk].name(),
				  this, SLOT(setSenderPropertyColor()));
    a->setProperty("setColor", QVariant(_colorlist->list[nk]));
  }
  if (k > 0)
    mMenu->addSeparator();

  mMenu->addAction(tr("Custom ..."), this, SLOT(requestColor()));

  setMenu(mMenu);
}

void KLFColorChooser::paintEvent(QPaintEvent *e)
{
  QPushButton::paintEvent(e);
  QPainter p(this);
  p.setClipRect(e->rect());
  p.drawPixmap(QPointF(_xalignfactor*(width()-_pix.width()), _yalignfactor*(height()-_pix.height())), _pix);
}

void KLFColorChooser::_setpix()
{
  //  if (_color.isValid()) {
  _pix = colorPixmap(_color, _size);
  // DON'T setIcon() because we draw ourselves ! see paintEvent() !
  //  setIconSize(_pix.size());
  //  setIcon(_pix);
  setText("");
  //  } else {
  //    _pix = QPixmap();
  //    setIcon(QIcon());
  //    setIconSize(QSize(0,0));
  //    setText("");
  //  }
}


QPixmap KLFColorChooser::colorPixmap(const QColor& color, const QSize& size)
{
  QPixmap pix = QPixmap(size);
  if (color.isValid()) {
    pix.fill(color);
  } else {
    // draw "transparent"-representing pixmap
    pix.fill(QColor(127,127,127,80));
    QPainter p(&pix);
    p.setPen(QPen(QColor(255,0,0), 2));
    p.drawLine(0,0,size.width(),size.height());
  }
  return pix;
}



// static
void KLFColorChooser::ensureColorListInstance()
{
  if ( _colorlist == 0 )
    _colorlist = new KLFColorList;
}
// static
void KLFColorChooser::setColorList(const QList<QColor>& colors)
{
  ensureColorListInstance();
  //  *_colorlist = colors;
  _colorlist->list = colors;
  _colorlist->notifyListChanged();
}

// static
QList<QColor> KLFColorChooser::colorList()
{
  ensureColorListInstance();
  QList<QColor> l = _colorlist->list;
  return l;
}

