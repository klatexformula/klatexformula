/***************************************************************************
 *   file klfflowlistwidget_p.h
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
 * This file contains internal definitions for file klfflowlistwidget.cpp.
 * \internal
 */

#ifndef KLFFLOWLISTWIDGET_P_H
#define KLFFLOWLISTWIDGET_P_H

#include <math.h>

#include <QWidget>
#include <QLabel>
#include <QPainter>
#include <QMouseEvent>
#include <QHoverEvent>
#include <QTimer>
#include <QPixmap>

#include <klfdefs.h>


#define MARGINX 5
#define MARGINY 3
#define BTNSPC 8
#define DARKEN_MAX 500
#define DARKEN_FACTOR 900



class KLFFlowListItemWidget : public QWidget
{
  Q_OBJECT

  Q_PROPERTY(QColor hoverColor READ hoverColor WRITE setHoverColor) ;
  Q_PROPERTY(QColor hoverCloseColor READ hoverCloseColor WRITE setHoverCloseColor) ;
public:
  KLFFlowListItemWidget(QWidget *parent) : QWidget(parent), closePix(":/pics/smallcross.png")
  {
    w = NULL;
    darken = 0;
    mouseHoverLabel = false;
    mouseHoverCloseBtn = false;
    mouseClosePressed = false;
    //    setMouseTracking(true);
    setAttribute(Qt::WA_Hover, true);

    fadeTimer = new QTimer(this);
    fadeTimer->setInterval(50);
    connect(fadeTimer, SIGNAL(timeout()), this, SLOT(shadeDarker()));

    pHoverColor = QColor(120, 120, 255, 128);
    pHoverCloseColor = QColor(255, 0, 100, 128);
  }

  inline QColor hoverColor() const { return pHoverColor; }
  inline QColor hoverCloseColor() const { return pHoverCloseColor; }
  void setHoverColor(const QColor& c)
  {
    pHoverColor = c;
    update();
  }
  void setHoverCloseColor(const QColor& c)
  {
    pHoverCloseColor = c;
    update();
  }

  void setItemWidget(QWidget *widget)
  {
    w = widget;
    if (w == NULL)
      return;
    if (w->parentWidget() != this)
      w->setParent(this);
    if (w->minimumSize() != QSize(0,0))
      setMinimumSize(w->minimumSize() + overheadSize());
    if (w->maximumSize().width() < QWIDGETSIZE_MAX || w->maximumSize().height() < QWIDGETSIZE_MAX)
      setMaximumSize(w->maximumSize() + overheadSize());

    setSizePolicy(w->sizePolicy());
    //    w->setMouseTracking(true);
    //    w->installEventFilter(this);
    
    updateWGeom();
  }

  virtual QSize minimumSizeHint() const
  {
    QSize s = QSize(0,0);
    if (w != NULL)
      s = w->minimumSizeHint();
    s = (s + overheadSize()).expandedTo(QSize(0, closePix.height()+2*MARGINY));
    klfDbg("minimumSizeHint="<<s) ;
    return s;
  }

  virtual QSize sizeHint() const
  {
    QSize s = QSize(0,0);
    if (w != NULL)
      s = w->sizeHint();
    s = (s + overheadSize()).expandedTo(minimumSizeHint());
    klfDbg("sizeHint="<<s) ;
    return s;
  }

  //   virtual bool eventFilter(QObject *obj, QEvent *event)
  //   {
  //     if (obj == w && event->type() == QEvent::MouseMove) {
  //       QMouseEvent *e = (QMouseEvent*) event;
  //       QPoint gpos = w->mapToGlobal(e->pos());
  //       QMouseEvent me(QEvent::MouseMove, mapFromGlobal(gpos), gpos, e->button(), e->buttons(),
  // 		     e->modifiers());      
  //       mouseMoveEvent(&me);
  //     }
  //     return QWidget::eventFilter(obj, event);
  //   }

signals:
  void closeClicked();

protected:

  bool event(QEvent *event)
  {
    //     if (event->type() == QEvent::HoverEnter) {
    //       mouseHoverCloseBtn = true;
    //       startShading();
    //     }
    //     if (event->type() == QEvent::HoverMove) {
    //       //      QHoverEvent *e = (QHoverEvent*) event;
    //       //      mouseHoverCloseBtn = box.contains(e->pos());
    //       mouseHoverCloseBtn = true;
    //       startShading();
    //     }
    //     if (event->type() == QEvent::HoverLeave) {
    //       mouseHoverCloseBtn = false;
    //       resetShade();
    //     }
    if (event->type() == QEvent::HoverEnter ||
	event->type() == QEvent::HoverMove ||
	event->type() == QEvent::HoverLeave) {
      QHoverEvent *e = (QHoverEvent*) event;
      mouseHoverCloseBtn = crossbox.contains(e->pos());
      if (mouseHoverCloseBtn)
	startShading();
      else
	resetShade();
      if (!mouseHoverCloseBtn && box.contains(e->pos())) {
	mouseHoverLabel = true;
	update();
      } else {
	mouseHoverLabel = false;
	update();
      }
    }

    return QWidget::event(event);
  }

  void paintEvent(QPaintEvent *event)
  {
    KLF_DEBUG_BLOCK(KLF_FUNC_NAME) ;
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing, true);
    if (mouseHoverLabel)
      p.setBrush(pHoverColor);
    p.drawPath(box);
    // and draw the shaded close button part
    p.save();
    p.setClipRect(QRect(QPoint(width()-MARGINX-closePix.width()-BTNSPC/2, 0),
			QSize(MARGINX+closePix.width()+BTNSPC/2, height())));
    if (mouseHoverCloseBtn) {
      klfDbg("in mouse hover mode, darken="<<darken) ;
      double factor = exp(-(double)darken/DARKEN_FACTOR);
      p.setBrush(QColor(pHoverCloseColor.red()*factor, pHoverCloseColor.green()*factor,
			pHoverCloseColor.blue()*factor));
    } else if (!mouseHoverLabel) {
      QColor c = pHoverCloseColor;
      c.setAlpha(50);
      p.setBrush(c);
    }
    p.drawPath(box);
    p.restore();
    // --
    int xpix = width()-MARGINX-closePix.width();
    QColor c = p.pen().color();
    c.setAlpha(128);
    p.save();
    p.setPen(QPen(c, 0.));
    p.drawLine(xpix - BTNSPC/2, height()*0.2,
	       xpix - BTNSPC/2, height()*0.8);
    p.restore();
    p.drawPixmap(xpix, height()/2-closePix.height()/2, closePix);
    QWidget::paintEvent(event);
  }

  virtual void resizeEvent(QResizeEvent *)
  {
    box = QPainterPath();
    box.addRoundedRect(QRectF(QPointF(1,1), QSizeF(width()-2,height()-2)), 6., 4., Qt::AbsoluteSize);
    crossbox = QPainterPath();
    crossbox.addRect(QRectF(QPointF(width()-MARGINX-closePix.width()-BTNSPC/2 , MARGINY),
			    QSizeF(closePix.width()+MARGINX+BTNSPC/2 , height()-2*MARGINY)));
    updateWGeom();
  }
  virtual void moveEvent(QMoveEvent *)
  {
    updateWGeom();
  }

  virtual void mouseMoveEvent(QMouseEvent *)
  {
  }
  virtual void mousePressEvent(QMouseEvent *me)
  {
    if (crossbox.contains(me->pos())) {
      mouseClosePressed = true;
      darken = DARKEN_MAX;
    }
  }
  virtual void mouseReleaseEvent(QMouseEvent *me)
  {
    if (mouseClosePressed && crossbox.contains(me->pos()))
      emit closeClicked();
    mouseClosePressed = false;
  }
  virtual void leaveEvent(QEvent *)
  {
    mouseClosePressed = false;
  }

protected slots:
  void shadeDarker()
  {
    darken += fadeTimer->interval();
    if (darken >= DARKEN_MAX)
      fadeTimer->stop();
    //    klfDbg("timer timeout: darken shade to "<<darken) ;
    update();
  }
  void startShading()
  {
    if (darken == 0) {
      fadeTimer->start();
      update();
    }
  }
  void resetShade()
  {
    fadeTimer->stop();
    darken = 0;
    update();
  }

private:
  QWidget *w;

  QTimer *fadeTimer;

  QPixmap closePix;

  QPainterPath box;
  QPainterPath crossbox;
  int darken;
  bool mouseHoverLabel;
  bool mouseHoverCloseBtn;
  bool mouseClosePressed;

  QColor pHoverColor;
  QColor pHoverCloseColor;

  void updateWGeom()
  {
    if (w == NULL)
      return;
    QRect r(QPoint(MARGINX,MARGINY), size() - overheadSize());
    klfDbg("updateWGeom: I am at "<<geometry()<<", calculated child rect at "<<r) ;
    w->setGeometry(r);
  }

  inline QSize overheadSize() const { return QSize(2*MARGINX + BTNSPC + closePix.width(),2*MARGINY); }
};



#endif
