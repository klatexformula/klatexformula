/***************************************************************************
 *   file klftools_desplugin_collection.cpp
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


#include "klftools_desplugin.h"
#include "klftools_desplugin_collection.h"


// -----------------------------------------------------------



KLFToolsDesPlugin::KLFToolsDesPlugin(QObject* parent)
  : QObject(parent)
{
  _widgetPlugins.push_back(new KLFSearchBarDesPlugin(this));
  _widgetPlugins.push_back(new KLFColorChooseWidgetPaneDesPlugin(this));
  _widgetPlugins.push_back(new KLFColorComponentSpinBoxDesPlugin(this));
  _widgetPlugins.push_back(new KLFColorClickSquareDesPlugin(this));
  _widgetPlugins.push_back(new KLFColorChooseWidgetDesPlugin(this));
  _widgetPlugins.push_back(new KLFColorChooserDesPlugin(this));
  _widgetPlugins.push_back(new KLFLatexEditDesPlugin(this));
  _widgetPlugins.push_back(new KLFEnumListWidgetDesPlugin(this));
  _widgetPlugins.push_back(new KLFSideWidgetDesPlugin(this));
  _widgetPlugins.push_back(new KLFPathChooserDesPlugin(this));
}

KLFToolsDesPlugin::~KLFToolsDesPlugin()
{
  QList<QDesignerCustomWidgetInterface*>::iterator it;
  for (it = _widgetPlugins.begin(); it != _widgetPlugins.end(); it++) {
    delete (*it);
  }
  _widgetPlugins.clear();
}
