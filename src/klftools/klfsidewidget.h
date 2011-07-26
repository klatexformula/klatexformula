/***************************************************************************
 *   file klfsidewidget.h
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

#ifndef KLFSIDEWIDGET_H
#define KLFSIDEWIDGET_H


#include <QObject>
#include <QWidget>


#include <klfdefs.h>


class KLFSideWidgetManagerBasePrivate;
class KLFSideWidgetManagerBase : public QObject
{
  Q_OBJECT
public:
  /**
   * \warning This function does NOT initialize its internal pParentWidget and pSideWidget members---they
   *   are always initialized to NULL. Reason: subclasses should set these in their constructors via
   *   setSideWidget() and setOurParentWidget(), which then call newSideWidgetSet() etc., which passes
   *   through the virtual call (it does not in the base class constructor). */
  KLFSideWidgetManagerBase(QWidget *parentWidget = NULL, QWidget *sideWidget = NULL);
  virtual ~KLFSideWidgetManagerBase();

  inline QWidget * sideWidget() { return pSideWidget; }
  inline QWidget * ourParentWidget() { return pParentWidget; }

  void setSideWidget(QWidget *widget);
  void setOurParentWidget(QWidget *widget);

  virtual bool sideWidgetVisible() = 0;

public slots:
  virtual void showSideWidget(bool show) = 0;
  void showSideWidget() { showSideWidget(true); }
  void hideSideWidget(bool hide = true) { showSideWidget(!hide); }
  void toggleSideWidget() { showSideWidget(!sideWidgetVisible()); }

protected:
  virtual void newSideWidgetSet(QWidget *oldSideWidget, QWidget *newSideWidget)
  {  Q_UNUSED(oldSideWidget); Q_UNUSED(newSideWidget);  }
  virtual void newParentWidgetSet(QWidget *oldParentWidget, QWidget *newParentWidget)
  {  Q_UNUSED(oldParentWidget); Q_UNUSED(newParentWidget); }

private:

  QWidget *pSideWidget;
  QWidget *pParentWidget;

  KLF_DECLARE_PRIVATE(KLFSideWidgetManagerBase);
};


class KLFShowHideSideWidgetManagerPrivate;
class KLFShowHideSideWidgetManager : public KLFSideWidgetManagerBase
{
  Q_OBJECT

  Q_PROPERTY(Qt::Orientation orientation READ orientation WRITE setOrientation) ;
  Q_PROPERTY(int calcSpacing READ calcSpacing WRITE setCalcSpacing) ;
  KLF_PROPERTY_GET(Qt::Orientation orientation) ;
  KLF_PROPERTY_GET(int calcSpacing) ;
public:
  KLFShowHideSideWidgetManager(QWidget *parentWidget = NULL, QWidget *sideWidget = NULL);
  virtual ~KLFShowHideSideWidgetManager();

  virtual bool eventFilter(QObject *obj, QEvent *event);

  virtual bool sideWidgetVisible();

public slots:
  virtual void showSideWidget(bool show);
  void setOrientation(Qt::Orientation o);
  void setCalcSpacing(int cs);

protected:
  virtual bool event(QEvent *event);

  virtual void newSideWidgetSet(QWidget *oldSideWidget, QWidget *newSideWidget);
  virtual void newParentWidgetSet(QWidget *oldParentWidget, QWidget *newParentWidget);

private slots:
  void resizeParentWidget(const QSize& size);

private:
  KLF_DECLARE_PRIVATE(KLFShowHideSideWidgetManager);
};



class KLFDrawerSideWidgetManagerPrivate;
class KLFDrawerSideWidgetManager : public KLFSideWidgetManagerBase
{
  Q_OBJECT

  Q_PROPERTY(Qt::DockWidgetArea openEdge READ openEdge WRITE setOpenEdge) ;
  KLF_PROPERTY_GET(Qt::DockWidgetArea openEdge) ;
public:
  KLFDrawerSideWidgetManager(QWidget *parentWidget = NULL, QWidget *sideWidget = NULL);
  virtual ~KLFDrawerSideWidgetManager();

  virtual bool eventFilter(QObject *obj, QEvent *event);

  virtual bool sideWidgetVisible();

public slots:
  virtual void showSideWidget(bool show);
  void setOpenEdge(Qt::DockWidgetArea edge);

protected:
  virtual bool event(QEvent *event);

  virtual void newSideWidgetSet(QWidget *oldSideWidget, QWidget *newSideWidget);
  virtual void newParentWidgetSet(QWidget *oldWidget, QWidget *newWidget);

private:
  KLF_DECLARE_PRIVATE(KLFDrawerSideWidgetManager) ;
};



#endif
