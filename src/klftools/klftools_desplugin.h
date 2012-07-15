/***************************************************************************
 *   file klfsearchbar_desplugin.h
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

/* Qt DESIGNER PLUGIN FOR KLFSearchBar WIDGET */

#ifndef KLFSEARCHBAR_DESPLUGIN_H
#define KLFSEARCHBAR_DESPLUGIN_H

#include <QString>
#include <QWidget>
#include <QtDesigner/QDesignerCustomWidgetInterface>
#include <QtDesigner/QDesignerContainerExtension>

#include <klfsearchbar.h>
#include <klfcolorchooser.h>
#include <klflatexedit.h>
#include <klfenumlistwidget.h>
#include <klfsidewidget.h>
#include <klfpathchooser.h>


/** \bug **** PROPER CMAKE VARIABLES FOR KLFSEARCHBAR_DESPLUGIN CMAKE TARGET ******  */

class KLFSearchBarDesPlugin : public QObject, public QDesignerCustomWidgetInterface
{
  Q_OBJECT
  Q_INTERFACES(QDesignerCustomWidgetInterface)
    ;
public:
  KLFSearchBarDesPlugin(QObject *parent = 0);

  bool isContainer() const;
  bool isInitialized() const;
  QIcon icon() const;
  QString domXml() const;
  QString group() const;
  QString includeFile() const;
  QString name() const;
  QString toolTip() const;
  QString whatsThis() const;
  QWidget *createWidget(QWidget *parent);
  void initialize(QDesignerFormEditorInterface *core);

private:
  bool pInitialized;
};


class KLFColorChooseWidgetPaneDesPlugin : public QObject, public QDesignerCustomWidgetInterface
{
  Q_OBJECT
  Q_INTERFACES(QDesignerCustomWidgetInterface)
    ;
public:
  KLFColorChooseWidgetPaneDesPlugin(QObject *parent = 0);

  bool isContainer() const;
  bool isInitialized() const;
  QIcon icon() const;
  QString domXml() const;
  QString group() const;
  QString includeFile() const;
  QString name() const;
  QString toolTip() const;
  QString whatsThis() const;
  QWidget *createWidget(QWidget *parent);
  void initialize(QDesignerFormEditorInterface *core);

private:
  bool pInitialized;
};



class KLFColorComponentSpinBoxDesPlugin : public QObject, public QDesignerCustomWidgetInterface
{
  Q_OBJECT
  Q_INTERFACES(QDesignerCustomWidgetInterface)
    ;
public:
  KLFColorComponentSpinBoxDesPlugin(QObject *parent = 0);

  bool isContainer() const;
  bool isInitialized() const;
  QIcon icon() const;
  QString domXml() const;
  QString group() const;
  QString includeFile() const;
  QString name() const;
  QString toolTip() const;
  QString whatsThis() const;
  QWidget *createWidget(QWidget *parent);
  void initialize(QDesignerFormEditorInterface *core);

private:
  bool pInitialized;
};



class KLFColorClickSquareDesPlugin : public QObject, public QDesignerCustomWidgetInterface
{
  Q_OBJECT
  Q_INTERFACES(QDesignerCustomWidgetInterface)
    ;
public:
  KLFColorClickSquareDesPlugin(QObject *parent = 0);

  bool isContainer() const;
  bool isInitialized() const;
  QIcon icon() const;
  QString domXml() const;
  QString group() const;
  QString includeFile() const;
  QString name() const;
  QString toolTip() const;
  QString whatsThis() const;
  QWidget *createWidget(QWidget *parent);
  void initialize(QDesignerFormEditorInterface *core);

private:
  bool pInitialized;
};




class KLFColorChooseWidgetDesPlugin : public QObject, public QDesignerCustomWidgetInterface
{
  Q_OBJECT
  Q_INTERFACES(QDesignerCustomWidgetInterface)
    ;
public:
  KLFColorChooseWidgetDesPlugin(QObject *parent = 0);

  bool isContainer() const;
  bool isInitialized() const;
  QIcon icon() const;
  QString domXml() const;
  QString group() const;
  QString includeFile() const;
  QString name() const;
  QString toolTip() const;
  QString whatsThis() const;
  QWidget *createWidget(QWidget *parent);
  void initialize(QDesignerFormEditorInterface *core);

private:
  bool pInitialized;
};





class KLFColorChooserDesPlugin : public QObject, public QDesignerCustomWidgetInterface
{
  Q_OBJECT
  Q_INTERFACES(QDesignerCustomWidgetInterface)
    ;
public:
  KLFColorChooserDesPlugin(QObject *parent = 0);

  bool isContainer() const;
  bool isInitialized() const;
  QIcon icon() const;
  QString domXml() const;
  QString group() const;
  QString includeFile() const;
  QString name() const;
  QString toolTip() const;
  QString whatsThis() const;
  QWidget *createWidget(QWidget *parent);
  void initialize(QDesignerFormEditorInterface *core);

private:
  bool pInitialized;
};





class KLFLatexEditDesPlugin : public QObject, public QDesignerCustomWidgetInterface
{
  Q_OBJECT
  Q_INTERFACES(QDesignerCustomWidgetInterface)
    ;
public:
  KLFLatexEditDesPlugin(QObject *parent = 0);

  bool isContainer() const;
  bool isInitialized() const;
  QIcon icon() const;
  QString domXml() const;
  QString group() const;
  QString includeFile() const;
  QString name() const;
  QString toolTip() const;
  QString whatsThis() const;
  QWidget *createWidget(QWidget *parent);
  void initialize(QDesignerFormEditorInterface *core);

private:
  bool pInitialized;
};


class KLFEnumListWidgetDesPlugin : public QObject, public QDesignerCustomWidgetInterface
{
  Q_OBJECT
  Q_INTERFACES(QDesignerCustomWidgetInterface)
    ;
public:
  KLFEnumListWidgetDesPlugin(QObject *parent = 0);

  bool isContainer() const;
  bool isInitialized() const;
  QIcon icon() const;
  QString domXml() const;
  QString group() const;
  QString includeFile() const;
  QString name() const;
  QString toolTip() const;
  QString whatsThis() const;
  QWidget *createWidget(QWidget *parent);
  void initialize(QDesignerFormEditorInterface *core);

private:
  bool pInitialized;
};


class KLFSideWidgetDesPlugin : public QObject, public QDesignerCustomWidgetInterface
{
  Q_OBJECT
  Q_INTERFACES(QDesignerCustomWidgetInterface)
    ;
public:
  KLFSideWidgetDesPlugin(QObject *parent = 0);

  bool isContainer() const;
  bool isInitialized() const;
  QIcon icon() const;
  QString domXml() const;
  QString group() const;
  QString includeFile() const;
  QString name() const;
  QString toolTip() const;
  QString whatsThis() const;
  QWidget *createWidget(QWidget *parent);
  void initialize(QDesignerFormEditorInterface *core);

private:
  bool pInitialized;
};


class KLFPathChooserDesPlugin : public QObject, public QDesignerCustomWidgetInterface
{
  Q_OBJECT
  Q_INTERFACES(QDesignerCustomWidgetInterface)
    ;
public:
  KLFPathChooserDesPlugin(QObject *parent = 0);

  bool isContainer() const;
  bool isInitialized() const;
  QIcon icon() const;
  QString domXml() const;
  QString group() const;
  QString includeFile() const;
  QString name() const;
  QString toolTip() const;
  QString whatsThis() const;
  QWidget *createWidget(QWidget *parent);
  void initialize(QDesignerFormEditorInterface *core);

private:
  bool pInitialized;
};




// -----------------------


class KLFToolsDesPlugin : public QObject, public QDesignerCustomWidgetCollectionInterface
{
  Q_OBJECT
  Q_INTERFACES(QDesignerCustomWidgetCollectionInterface)

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
