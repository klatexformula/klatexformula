/***************************************************************************
 *   file plugins/openfile/openfile.h
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

#ifndef PLUGINS_OPENFILE_H
#define PLUGINS_OPENFILE_H

#include <QtCore>
#include <QtGui>

#include <klfpluginiface.h>
#include <klfconfig.h>

#include <ui_openbufferswidget.h>



class OpenBuffersWidget : public QWidget, protected Ui::OpenBuffersWidget
{
  Q_OBJECT
public:
  OpenBuffersWidget(KLFMainWin *mainWin)
    : QWidget(mainWin, Qt::Tool), pMainWin(mainWin)
  {
    setupUi(this);

    connect(mainWin, SIGNAL(userInputChanged()), this, SLOT(updateInputFromMainWin()));
    connect(mainWin, SIGNAL(inputCleared()), this, SLOT(detachActiveBuffer()));
    connect(mainWin, SIGNAL(fileOpened(const QString&, const QString&, KLFAbstractDataOpener *)),
	    this, SLOT(mainwinFileOpened(const QString&, const QString&)));
    connect(mainWin, SIGNAL(savedToFile(const QString&, const QString&, KLFAbstractOutputSaver *)),
	    this, SLOT(mainwinFileSaved(const QString&, const QString&, KLFAbstractOutputSaver *)));
  }

public slots:
  void updateInputFromMainWin()
  {
    if (pActiveBuffer != NULL) {
      pActiveBuffer->updateInputFromMainWin();
      lstBuffers->refreshBuffer(pActiveBuffer);
    }
  }

  void detachActiveBuffer()
  {
    setActiveBuffer(NULL);
  }

  void mainwinFileOpened(const QString& file, const QString& format)
  {
    OpenBuffer * newbuffer = new OpenBuffer(pMainWin, file, format);
    pBufferList.append(newbuffer);
    lstBuffers->setBufferList(pBufferList, newbuffer);
    pActiveBuffer = newbuffer;
  }
  void mainwinFileSaved(const QString& file, const QString& format, KLFAbstractOutputSaver *saver)
  {
    OpenBuffer * newbuffer = new OpenBuffer(pMainWin, file, format, saver);
    pBufferList.append(newbuffer);
    lstBuffers->setBufferList(pBufferList, newbuffer);
    pActiveBuffer = newbuffer;
  }

  void on_btnSave_clicked()
  {
    if (pActiveBuffer != NULL) {
      pActiveBuffer->save();
      lstBuffers->refreshBuffer(pActiveBuffer);
    }
  }

  void on_btnClose_clicked()
  {
    if (pActiveBuffer != NULL) {
      // TODO check for modifications etc....
      pBufferList.removeAll(pActiveBuffer);
      lstBuffers->setBufferList(pBufferList, NULL);
    }
  }

  void on_btnOpen_clicked()
  {
    QString fn = QFileDialog::getOpenFileName(this, tr("Open a file"));
    bool ok = pMainWin->openFile(fn);
    if (!ok) {
      klfWarning("Failed to open file "<<fn) ;
    }
  }

  void setActiveBuffer(OpenBuffer * buffer)
  {
    pActiveBuffer = buffer;
  }

private:
  KLFMainWin * pMainWin;

  QList<OpenBuffer*> pBufferList;
  OpenBuffer * pActiveBuffer;
};





class OpenFilePlugin : public QObject, public KLFPluginGenericInterface
{
  Q_OBJECT
  Q_INTERFACES(KLFPluginGenericInterface)
public:
  OpenFilePlugin() : QObject(qApp) { }
  virtual ~OpenFilePlugin() { }

  virtual QVariant pluginInfo(PluginInfo which) const {
    switch (which) {
    case PluginName: return QString("openfile");
    case PluginAuthor: return QString("Philippe Faist <philippe.faist")+QString("@bluewin.ch>");
    case PluginTitle: return tr("Open Buffer");
    case PluginDescription: return tr("Work on formula file");
    case PluginDefaultEnable: return true;
    default:
      return QVariant();
    }
  }

  virtual void initialize(QApplication *app, KLFMainWin *mainWin, KLFPluginConfigAccess *config);

  virtual QWidget * createConfigWidget(QWidget *parent);
  virtual void loadFromConfig(QWidget *confwidget, KLFPluginConfigAccess *config);
  virtual void saveToConfig(QWidget *confwidget, KLFPluginConfigAccess *config);

protected:
  KLFMainWin *_mainwin;
  QApplication *_app;

  KLFPluginConfigAccess *_config;

  OpenBuffersWidget *_widget;
};


#endif
