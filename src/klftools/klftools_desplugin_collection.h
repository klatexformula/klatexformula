/***************************************************************************
 *   file klftools_desplugin_collection.h
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

#ifndef KLFTOOLS_DESPLUGIN_COLLECTION_H
#define KLFTOOLS_DESPLUGIN_COLLECTION_H

#include <QString>
#include <QWidget>
#include <QtUiPlugin/QDesignerCustomWidgetInterface>
#include <QtDesigner/QDesignerContainerExtension>
#include <QtUiPlugin/QDesignerCustomWidgetCollectionInterface>


// -----------------------


class KLFToolsDesPlugin : public QObject, public QDesignerCustomWidgetCollectionInterface
{
  Q_OBJECT
  Q_INTERFACES(QDesignerCustomWidgetCollectionInterface)

  Q_PLUGIN_METADATA(IID "org.klatexformula.klftools.qtdesignerplugin")
    ;

public:
  KLFToolsDesPlugin(QObject* parent = NULL);
  virtual ~KLFToolsDesPlugin();

  QList<QDesignerCustomWidgetInterface*> customWidgets() const
  {
    return _widgetPlugins;
  }

private:
   QList<QDesignerCustomWidgetInterface*> _widgetPlugins;
};




#endif
