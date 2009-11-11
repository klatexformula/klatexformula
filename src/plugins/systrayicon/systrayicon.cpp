/***************************************************************************
 *   file plugins/systrayicon/systrayicon.cpp
 *   This file is part of the KLatexFormula Project.
 *   Copyright (C) 2009 by Philippe Faist
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

#include <QtCore>
#include <QtGui>

#include <klfmainwin.h>
#include <klflibrary.h>
#include <klflatexsymbols.h>
#include <klfconfig.h>

#include "systrayicon.h"


SysTrayIconConfigWidget::SysTrayIconConfigWidget(QWidget *parent)
  : QWidget(parent)
{
  setupUi(this);

#ifdef Q_WS_X11
  connect(chkSysTrayOn, SIGNAL(toggled(bool)), chkRestoreOnHover, SLOT(setEnabled(bool)));
#else
  chkRestoreOnHover->hide();
#endif
}


void SysTrayIconPlugin::initialize(QApplication */*app*/, KLFMainWin *mainWin, KLFPluginConfigAccess *rwconfig)
{
  _mainwin = mainWin;
  _config = rwconfig;

  QMenu *menu = new QMenu(mainWin);
  menu->addAction(tr("Minimize"), this, SLOT(minimize()));
  menu->addAction(tr("Restore"), this, SLOT(restore()));
  menu->addAction(tr("Paste From Clipboard"), this, SLOT(latexFromClipboard()));
  menu->addAction(QIcon(":/pics/closehide.png"), tr("Quit"), mainWin, SLOT(quit()));

  _systrayicon = new QSystemTrayIcon(QIcon(":/pics/hi32-app-klatexformula.png"), mainWin);
  _systrayicon->setToolTip("KLatexFormula");
  _systrayicon->setContextMenu(menu);

  _systrayicon->installEventFilter(this);

  connect(_systrayicon, SIGNAL(activated(QSystemTrayIcon::ActivationReason)),
	  this, SLOT(slotSysTrayActivated(QSystemTrayIcon::ActivationReason)));

  apply();
}

void SysTrayIconPlugin::apply()
{
  bool systrayon = _config->readValue("systrayon").toBool();

  if (systrayon && QSystemTrayIcon::isSystemTrayAvailable()) {
    _systrayicon->show();
    _mainwin->setQuitOnClose(false);
  } else {
    _systrayicon->hide();
    _mainwin->setQuitOnClose(true);
  }

}

bool SysTrayIconPlugin::eventFilter(QObject *obj, QEvent *e)
{
  if (obj == _systrayicon && e->type() == QEvent::ToolTip) {
    // works on X11 only
    if (_config->readValue("restoreonhover").toBool()) {
      restore();
    }
  }
  return false;
}


QWidget * SysTrayIconPlugin::createConfigWidget(QWidget *parent)
{
  return new SysTrayIconConfigWidget(parent);
}

void SysTrayIconPlugin::loadFromConfig(QWidget *confqwidget, KLFPluginConfigAccess *config)
{
  SysTrayIconConfigWidget *confwidget = qobject_cast<SysTrayIconConfigWidget*>(confqwidget);
  bool systrayon = config->readValue("systrayon").toBool();
  bool restoreonhover = config->readValue("restoreonhover").toBool();
  confwidget->chkSysTrayOn->setChecked(systrayon);
  confwidget->chkRestoreOnHover->setChecked(restoreonhover);
}
void SysTrayIconPlugin::saveToConfig(QWidget *confqwidget, KLFPluginConfigAccess *config)
{
  SysTrayIconConfigWidget *confwidget = qobject_cast<SysTrayIconConfigWidget*>(confqwidget);
  bool systrayon = confwidget->chkSysTrayOn->isChecked();
  bool restoreonhover = confwidget->chkRestoreOnHover->isChecked();
  config->writeValue("systrayon", systrayon);
  config->writeValue("restoreonhover", restoreonhover);
  apply();
}


void SysTrayIconPlugin::latexFromClipboard()
{
  restore();
  QString cliptext = QApplication::clipboard()->text();
  _mainwin->slotSetLatex(cliptext);
}


void SysTrayIconPlugin::restore()
{
  _mainwin->show();
  
}

void SysTrayIconPlugin::minimize()
{
  _mainwin->hide();
}
void SysTrayIconPlugin::slotSysTrayActivated(QSystemTrayIcon::ActivationReason reason)
{
  if (reason == QSystemTrayIcon::Trigger) {
    if ( ! _mainwin->isVisible() ) {
      _mainwin->show();
    } else {
      if ( QApplication::activeWindow() != NULL ) {
	_mainwin->hide();
      } else {
	_mainwin->raise();
	_mainwin->activateWindow();
      }
    }
  }
}





// Export Plugin
Q_EXPORT_PLUGIN2(systrayicon, SysTrayIconPlugin);
