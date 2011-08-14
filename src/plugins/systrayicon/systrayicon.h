/***************************************************************************
 *   file plugins/systrayicon/systrayicon.h
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

#ifndef PLUGINS_SYSTRAYICON_H
#define PLUGINS_SYSTRAYICON_H

#include <QtCore>
#include <QtGui>

#include <klfpluginiface.h>
#include <klfconfig.h>

#include <ui_systrayiconconfigwidget.h>
#include <ui_systraymainiconifybuttons.h>


#if defined(Q_WS_MAC) && defined(QT_MAC_USE_COCOA)
#define KLF_MAC_HIDE_INSTEAD
#endif


class SysTrayIconConfigWidget : public QWidget, public Ui::SysTrayIconConfigWidget
{
  Q_OBJECT
public:
  SysTrayIconConfigWidget(QWidget *parent);
  virtual ~SysTrayIconConfigWidget() { }

};



class SysTrayMainIconifyButtons : public QWidget, public Ui::SysTrayMainIconifyButtons
{
  Q_OBJECT
public:
  SysTrayMainIconifyButtons(QWidget *parent);
};

class SysTrayIconPlugin : public QObject, public KLFPluginGenericInterface
{
  Q_OBJECT
  Q_INTERFACES(KLFPluginGenericInterface)
public:
  virtual ~SysTrayIconPlugin() { }

  virtual QVariant pluginInfo(PluginInfo which) const {
    switch (which) {
    case PluginName: return QString("systrayicon");
    case PluginAuthor: return "Philippe Faist <philippe.f"+QString("aist@bluewin.c")+"h>";
    case PluginTitle: return tr("System Tray Icon");
    case PluginDescription: return tr("Dock KLatexFormula into system tray");
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

signals:

  void setLatexText(const QString& text);

public slots:

  void restore();
  void minimize();
  void slotSysTrayActivated(QSystemTrayIcon::ActivationReason);
  // menu entry
  void latexFromClipboard(QClipboard::Mode mode = QClipboard::Clipboard);
  void latexFromClipboardSelection() { latexFromClipboard(QClipboard::Selection); }

protected:
  KLFMainWin *_mainwin;
  KLFPluginConfigAccess *_config;

  QSystemTrayIcon *_systrayicon;

  SysTrayMainIconifyButtons *_mainButtonBar;
};


#endif
