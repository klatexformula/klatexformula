/***************************************************************************
 *   file klfpluginiface.h
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

#ifndef KLFPLUGINIFACE_H
#define KLFPLUGINIFACE_H

#include <QObject>
#include <QString>

#include <qapplication.h>


class KLFMainWin;
class KLFPluginConfigAccess;


/** Generic interface to access (almost all!) the internals of KLatexFormula
 */
class KLFPluginGenericInterface
{
public:
  virtual ~KLFPluginGenericInterface() { }

  virtual QString pluginName() const = 0;
  virtual QString pluginTitle() const = 0;
  virtual QString pluginDescription() const = 0;

  virtual void initialize(QApplication *app, KLFMainWin *mainWin, KLFPluginConfigAccess *config) = 0;

  virtual QWidget * createConfigWidget(QWidget *parent) = 0;

  virtual void loadConfig(QWidget *configWidget, KLFPluginConfigAccess *config) = 0;
  virtual void saveConfig(QWidget *configWidget, KLFPluginConfigAccess *config) = 0;

};

Q_DECLARE_INTERFACE(KLFPluginGenericInterface,
		    "org.klatexformula.KLatexFormula.Plugin.GenericInterface/1.0");

#endif
