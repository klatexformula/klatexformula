/***************************************************************************
 *   file klfsidewidget_p.h
 *   This file is part of the KLatexFormula Project.
 *   Copyright (C) 2016 by Philippe Faist
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


#ifndef KLFSIDEWIDGET_P_H
#define KLFSIDEWIDGET_P_H

#include <QWidget>
#include <QDockWidget>

#include <klfsidewidget.h>



#ifdef KLF_WS_MAC
struct KLFDrawerSideWidgetManagerPrivate;

//! A side drawer show/hide manager (mac only)
/** \warning NOTE: You do not have to instantiate this class yourself to use KLFSideWidget. This is automatically
 * done in \ref KLFSideWidget::setSideWidgetManager().
 *
 * This class is provided in case you want to subclass it and use part of its functionality to write custom, more
 * advanced side widget managers. See also \ref KLFSideWidgetManagerBase.
 */
class KLF_EXPORT KLFDrawerSideWidgetManager : public KLFSideWidgetManagerBase
{
  Q_OBJECT

  Q_PROPERTY(Qt::DockWidgetArea openEdge READ openEdge WRITE setOpenEdge) ;
public:
  KLFDrawerSideWidgetManager(QWidget *parentWidget = NULL, QWidget *sideWidget = NULL,
			     QObject *managerParent = NULL);
  virtual ~KLFDrawerSideWidgetManager();

  Qt::DockWidgetArea openEdge() const;

  virtual bool sideWidgetVisible() const;

public slots:
  virtual void showSideWidget(bool show);
  void setOpenEdge(Qt::DockWidgetArea edge);

protected:
  virtual void newSideWidgetSet(QWidget *oldSideWidget, QWidget *newSideWidget);
  virtual void newParentWidgetSet(QWidget *oldWidget, QWidget *newWidget);

  virtual bool showHideIsAnimating();

private:
  KLF_DECLARE_PRIVATE(KLFDrawerSideWidgetManager) ;
};
#endif // KLF_WS_MAC





#endif // header guard
