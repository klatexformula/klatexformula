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


void KLFPopupSheet::paintEvent(QPaintEvent * /*event*/)
{
  QPainter painter(this);
  
  QPalette pal = palette();
  //QColor outlineColor = pal.color(QPalette::WindowText);
  //QColor bgColor = pal.color(QPalette::Window);
  //painter.setPen(QPen(outlineColor, 1));
  //painter.setBrush(bgColor);
  //painter.drawRoundedRect(QRectF(1, 9, width()-2, height()-11), 3, 3);

  QPainterPath path(QPointF(20,9));
  path.lineTo(27,0);
  path.lineTo(35,9);
  path.lineTo(width()-5,9);
  path.cubicTo(QPointF(width()-1,9), QPointF(width()-1,9), QPointF(width()-1,13));
  path.lineTo(width()-1,height()-5);
  path.cubicTo(QPointF(width()-1,height()-1), QPointF(width()-1,height()-1),
               QPointF(width()-5,height()-1));
  path.lineTo(5,height()-1);
  path.cubicTo(QPointF(1,height()-1), QPointF(1,height()-1),
               QPointF(1,height()-5));
  path.lineTo(1,13);
  path.cubicTo(QPointF(1,9), QPointF(1,9), QPointF(5,9));
  path.lineTo(20,9);


  painter.setPen(QPen(pal.color(QPalette::Base), 3));
  painter.setBrush(pal.color(QPalette::Window));
  painter.drawPath(path);

  painter.setPen(QPen(pal.color(QPalette::WindowText), 1));
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
