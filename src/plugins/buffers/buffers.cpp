/***************************************************************************
 *   file plugins/openfile/openfile.cpp
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

#include <QtCore>
#include <QtGui>


#include "openfile.h"

// --------------------------------------------------------------------------------


void OpenFilePlugin::initialize(QApplication *app, KLFMainWin *mainWin, KLFPluginConfigAccess *rwconfig)
{
  KLF_DEBUG_BLOCK(KLF_FUNC_NAME) ;
  klfDbg("Initializing Openfile plugin (compiled for KLF version " KLF_VERSION_STRING ")");

  Q_INIT_RESOURCE(openfiledata);
  klfDbg("initialized openfile resource data.") ;

  _mainwin = mainWin;
  _app = app;

  _config = rwconfig;

  klfDbg("About to create buffers widget and show.") ;

  klfWarning("!!!!!!!!!") ;

  _widget = new OpenBuffersWidget(mainWin);
  _widget->show();
}

QWidget * OpenFilePlugin::createConfigWidget(QWidget *parent)
{
  KLF_DEBUG_BLOCK(KLF_FUNC_NAME) ;
  QWidget *w = new QWidget(parent);
  return w;
}

void OpenFilePlugin::loadFromConfig(QWidget *confwidget, KLFPluginConfigAccess *config)
{
  KLF_DEBUG_BLOCK(KLF_FUNC_NAME) ;
}
void OpenFilePlugin::saveToConfig(QWidget *confwidget, KLFPluginConfigAccess *config)
{
  KLF_DEBUG_BLOCK(KLF_FUNC_NAME) ;
}






// Export Plugin
Q_EXPORT_PLUGIN2(openfile, OpenFilePlugin);


