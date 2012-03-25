/***************************************************************************
 *   file wikiscan.h
 *   This file is part of the KLatexFormula Project.
 *   Copyright (C) 2012 by Philippe Faist
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


#ifndef WIKISCAN_H
#define WIKISCAN_H


#include <QtCore>
#include <QtGui>

#include <klfdefs.h>
#include <klfpluginiface.h>
#include <klfconfig.h>



#include <ui_wikiscanconfigwidget.h>

class WikiScanConfigWidget : public QWidget, public Ui::WikiScanConfigWidget
{
  Q_OBJECT
public:
  WikiScanConfigWidget(QWidget *parent);
  virtual ~WikiScanConfigWidget() { }

};



class WikiScanPlugin : public QObject, public KLFPluginGenericInterface
{
  Q_OBJECT
  Q_INTERFACES(KLFPluginGenericInterface)
public:
  virtual ~WikiScanPlugin() { }

  virtual QVariant pluginInfo(PluginInfo which) const {
    switch (which) {
    case PluginName: return QString("wikiscan");
    case PluginAuthor: return "Philippe Faist <philippe.f"+QString("aist@bluewin.c")+"h>";
    case PluginTitle: return tr("Wikipedia Scanner");
    case PluginDescription: return tr("Search for formulas on Wikipedia pages");
    case PluginDefaultEnable: return true;
    default: return QVariant();
    }
  }

  virtual void initialize(QApplication *app, KLFMainWin *mainWin, KLFPluginConfigAccess *config);

  virtual QWidget * createConfigWidget(QWidget *parent);
  virtual void loadFromConfig(QWidget *confwidget, KLFPluginConfigAccess *config);
  virtual void saveToConfig(QWidget *confwidget, KLFPluginConfigAccess *config);

  virtual void apply();

  virtual bool eventFilter(QObject *obj, QEvent *e);

  virtual bool isMinimized();


protected:
  KLFMainWin *_mainwin;
  KLFPluginConfigAccess *_config;
};






#endif
