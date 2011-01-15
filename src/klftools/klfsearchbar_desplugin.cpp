/***************************************************************************
 *   file klfsearchbar_desplugin.cpp
 *   This file is part of the KLatexFormula Project.
 *   Copyright (C) 2010 by Philippe Faist
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

#include <stdio.h>

#include <klfsearchbar.h>

#include "klfsearchbar_desplugin.h"

KLFSearchBarDesPlugin::KLFSearchBarDesPlugin(QObject *parent)
{
  Q_UNUSED(parent) ;
}


bool KLFSearchBarDesPlugin::isContainer() const
{
  return false;
}
bool KLFSearchBarDesPlugin::isInitialized() const
{
  return pInitialized;
}
QIcon KLFSearchBarDesPlugin::icon() const
{
  return QIcon();
}
QString KLFSearchBarDesPlugin::domXml() const
{
  return "<widget class=\"KLFSearchBar\" name=\"searchBar\"></widget>\n";
}
QString KLFSearchBarDesPlugin::group() const
{
  return "Utilities";
}
QString KLFSearchBarDesPlugin::includeFile() const
{
  return "klfsearchbar.h";
}
QString KLFSearchBarDesPlugin::name() const
{
  return "KLFSearchBar";
}
QString KLFSearchBarDesPlugin::toolTip() const
{
  return "A full-featured search bar";
}
QString KLFSearchBarDesPlugin::whatsThis() const
{
  return "A full-featured search bar";
}
QWidget *KLFSearchBarDesPlugin::createWidget(QWidget *parent)
{
  return new KLFSearchBar(parent);
}
void KLFSearchBarDesPlugin::initialize(QDesignerFormEditorInterface *core)
{
  Q_UNUSED(core) ;
  if (pInitialized)
    return;

  // initialize ... :)

  pInitialized = true;
}


Q_EXPORT_PLUGIN2(klfsearchbardesplugin, KLFSearchBarDesPlugin)
  ;

