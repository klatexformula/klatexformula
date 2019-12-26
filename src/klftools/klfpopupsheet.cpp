/***************************************************************************
 *   file klfpopupsheet.cpp
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

#include "klfpopupsheet.h"
#include "klfpopupsheet_p.h"

#include <QWidget>
#include <QEvent>
#include <QPalette>
#include <QPainter>
#include <QBitmap>
#include <QAbstractButton>
#include <QPaintEvent>
#include <QStylePainter>


KLFPopupSheet::KLFPopupSheet(QWidget * parent)
  : QWidget(parent, Qt::Popup | Qt::FramelessWindowHint)
{
  KLF_INIT_PRIVATE(KLFPopupSheet) ;

  setAttribute(Qt::WA_TranslucentBackground);

  d->mLayout = new QGridLayout(this);
  d->mLayout->setContentsMargins(6,14,6,6);
  setLayout(d->mLayout);

  installEventFilter(d);

  connect(d, SIGNAL(doDelayedEmitPopupVisible(bool)),
          d, SLOT(slotPopupVisible(bool)),
          Qt::QueuedConnection);
}

KLFPopupSheet::~KLFPopupSheet()
{
  KLF_DELETE_PRIVATE ;
}


QPainterPath get_popup_shape(qreal width, qreal height, qreal borderradius,
                             qreal tipxposition, qreal tiphalfwidth, qreal tipheight,
                             qreal pathoffset) // pathoffset grows outwards, negative shrinks
{
  // the origin (0,0) is the top left of the popup rounded rectangle; i.e., the
  // left edge is at X=0 and the top edge is at Y=0.  The top of the tip has the
  // negative Y-coordinate Y=-tipheight.

  // correct all coordinates with the pathoffset
  width += 2*pathoffset;
  height += 2*pathoffset;
  tipxposition += pathoffset;
  // plus, the path will need to be shifted by (-pathoffset,-pathoffset)

  QPainterPath path;

  // start at the left edge of the tip mark
  path.moveTo(tipxposition-tiphalfwidth, 0);
  path.lineTo(tipxposition, -tipheight); // 27,0);
  path.lineTo(tipxposition+tiphalfwidth, 0); // 35,9);
  path.lineTo(width-borderradius, 0); // width()-5,9);
  path.cubicTo(QPointF(width, 0), QPointF(width, 0), QPointF(width, borderradius));
  // QPointF(width()-1,9), QPointF(width()-1,9), QPointF(width()-1,13));
  path.lineTo(width, height-borderradius); // width()-1,height()-5);
  path.cubicTo(QPointF(width, height), QPointF(width, height), QPointF(width-borderradius, height));
  // QPointF(width()-1,height()-1), QPointF(width()-1,height()-1),
  // QPointF(width()-5,height()-1));
  path.lineTo(borderradius, height); // 5,height()-1);
  path.cubicTo(QPointF(0,height), QPointF(0,height), QPointF(0,height-borderradius));
  // QPointF(1,height()-1), QPointF(1,height()-1),
  // QPointF(1,height()-5));
  path.lineTo(0, borderradius); // 1,13);
  path.cubicTo(QPointF(0,0), QPointF(0,0), QPointF(borderradius,0));
  // QPointF(1,9), QPointF(1,9), QPointF(5,9));
  path.lineTo(tipxposition-tiphalfwidth, 0); // 20,9);

  path.translate(-pathoffset,-pathoffset);

  return path;
}


void KLFPopupSheet::resizeEvent(QResizeEvent * event)
{
  QWidget::resizeEvent(event);
  
  // if (!QX11Info::isPlatformX11()) {
  //   return;
  // }

  QBitmap bmp(width(), height());
  bmp.clear();
  QPainter painter(&bmp);
  painter.translate(0,9); // top left corner of popup rounded rectangle (not
                          // counting path offset)
  QPainterPath path = get_popup_shape(width(), height()-9, 4,
                                      30, 7, 9,
                                      0);
  painter.fillPath(path, Qt::color1);

  setMask(bmp);
}

void KLFPopupSheet::paintEvent(QPaintEvent * )
{
  QPainter painter(this);
  
  QPalette pal = palette();
  //QColor outlineColor = pal.color(QPalette::WindowText);
  //QColor bgColor = pal.color(QPalette::Window);
  //painter.setPen(QPen(outlineColor, 1));
  //painter.setBrush(bgColor);
  //painter.drawRoundedRect(QRectF(1, 9, width()-2, height()-11), 3, 3);

  painter.translate(0,9); // top left corner of popup rounded rectangle (not
                          // counting path offset)

  QPainterPath path = get_popup_shape(width(), height()-9, 4,
                                      30, 7, 9,
                                      -2);

  // QPainterPath path(QPointF(20,9));
  // path.lineTo(27,0);
  // path.lineTo(35,9);
  // path.lineTo(width()-5,9);
  // path.cubicTo(QPointF(width()-1,9), QPointF(width()-1,9), QPointF(width()-1,13));
  // path.lineTo(width()-1,height()-5);
  // path.cubicTo(QPointF(width()-1,height()-1), QPointF(width()-1,height()-1),
  //              QPointF(width()-5,height()-1));
  // path.lineTo(5,height()-1);
  // path.cubicTo(QPointF(1,height()-1), QPointF(1,height()-1),
  //              QPointF(1,height()-5));
  // path.lineTo(1,13);
  // path.cubicTo(QPointF(1,9), QPointF(1,9), QPointF(5,9));
  // path.lineTo(20,9);

  painter.setPen(QPen(pal.color(QPalette::Base), 3));
  painter.setBrush(pal.color(QPalette::Window));
  painter.drawPath(path);

  QColor bcol = pal.color(QPalette::WindowText);
  bcol.setAlpha(128);
  painter.setPen(QPen(bcol, 1));
  painter.setBrush(QBrush());
  painter.drawPath(path);
}



void KLFPopupSheet::setCentralWidget(QWidget * w)
{
  QSize sz = w->size();
  QMargins cm = d->mLayout->contentsMargins();
  d->mLayout->addWidget(w, 0, 0);
  resize(sz + QSize(cm.left()+cm.right(), cm.top()+cm.bottom()));
}



void KLFPopupSheet::associateWithButton(QAbstractButton * button)
{
  d->mAssociatedButton = button;
  connect(d->mAssociatedButton, SIGNAL(toggled(bool)),
          d, SLOT(slotAssociatedButtonToggled(bool)));
}



bool KLFPopupSheetPrivate::eventFilter(QObject * object, QEvent * event)
{
  if (object == K) {
    klfDbg("Event filter, object is K and event = " << event) ;
    if (event->type() == QEvent::Show) {
      emit doDelayedEmitPopupVisible(true);
      return false;
    } else if (event->type() == QEvent::Hide) {
      emit doDelayedEmitPopupVisible(false);
      return false;
    }
  }
  return false;
}

void KLFPopupSheetPrivate::slotAssociatedButtonToggled(bool on)
{
  klfDbg("KLF Popup: Associated button toggled ! " << on) ;

  if (on) {
    QPoint pos = mAssociatedButton->mapToGlobal(QPoint(0,mAssociatedButton->height()));
    K->showPopup(pos);
  } else {
    K->setVisible(false);
  }
}

void KLFPopupSheetPrivate::slotPopupVisible(bool on)
{
  if (mAssociatedButton) {
    QSignalBlocker sigblocker(mAssociatedButton) ;
    mAssociatedButton->setChecked(on);
  }

  emit K->popupVisible(on);
  if (on) {
    emit K->popupShown();
  } else {
    emit K->popupHidden();
  }
}


void KLFPopupSheet::showPopup(const QPoint & pos)
{
  move(pos - QPoint(0,10));
  setVisible(true);
}

void KLFPopupSheet::hidePopup()
{
  setVisible(false);
}
