/***************************************************************************
 *   file plugins/buffers/buffers.h
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

#ifndef PLUGINS_BUFFERS_H
#define PLUGINS_BUFFERS_H

#include <QtCore>
#include <QtGui>

#include <klfrelativefont.h>

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

    pActiveBuffer = NULL;

    KLFRelativeFont *rf = new KLFRelativeFont(this);
    rf->setRelPointSize(-2);
    rf->setThorough(true);

    connect(mainWin, SIGNAL(userInputChanged()), this, SLOT(updateInputFromMainWin()));
    connect(mainWin, SIGNAL(inputCleared()), this, SLOT(detachActiveBuffer()));
    connect(mainWin, SIGNAL(fileOpened(const QString&, KLFAbstractDataOpener *)),
	    this, SLOT(mainwinFileOpened(const QString&)));
    connect(mainWin, SIGNAL(savedToFile(const QString&, const QString&, KLFAbstractOutputSaver *)),
	    this, SLOT(mainwinFileSaved(const QString&, const QString&, KLFAbstractOutputSaver *)));
    connect(mainWin, SIGNAL(evaluateFinished(const KLFBackend::klfOutput&)),
	    this, SLOT(evaluated(const KLFBackend::klfOutput&)));

    connect(lstBuffers, SIGNAL(bufferSelected(OpenBuffer *)), this, SLOT(setActiveBuffer(OpenBuffer *)));

    refreshButtonsState();
  }

public slots:
  void updateInputFromMainWin()
  {
    KLF_DEBUG_BLOCK(KLF_FUNC_NAME) ;
    if (pActiveBuffer != NULL) {
      pActiveBuffer->updateInputFromMainWin();
      lstBuffers->refreshBuffer(pActiveBuffer);
    }
    refreshButtonsState();
  }

  void detachActiveBuffer()
  {
    KLF_DEBUG_BLOCK(KLF_FUNC_NAME) ;
    lstBuffers->clearSelection();
    //    pActiveBuffer = NULL;
    // this will call the list's selectionChanged() which in turn will call our setActiveBuffer() and
    // set pActiveBuffer correctly to NULL.
    refreshButtonsState();
  }

  void mainwinFileOpened(const QString& file)
  {
    KLF_DEBUG_BLOCK(KLF_FUNC_NAME) ;
    OpenBuffer * newbuffer = new OpenBuffer(pMainWin, file, QString());
    pBufferList.append(newbuffer);
    lstBuffers->setBufferList(pBufferList, newbuffer);
    pActiveBuffer = newbuffer;
    newbuffer->updateInputFromMainWin();
    newbuffer->forceUnmodified();
    refreshButtonsState();
    if (pBufferList.size() > 1)
      show();
  }
  void mainwinFileSaved(const QString& file, const QString& format, KLFAbstractOutputSaver *saver)
  {
    KLF_DEBUG_BLOCK(KLF_FUNC_NAME) ;
    OpenBuffer * newbuffer = new OpenBuffer(pMainWin, file, format, saver);
    newbuffer->updateInputFromMainWin();
    newbuffer->evaluated(pMainWin->currentKLFBackendOutput());
    newbuffer->forceUnmodified();
    pBufferList.append(newbuffer);
    lstBuffers->setBufferList(pBufferList, newbuffer);
    pActiveBuffer = newbuffer;
    refreshButtonsState();
    if (pBufferList.size() > 1)
      show();
  }

  void evaluated(const KLFBackend::klfOutput& output)
  {
    KLF_DEBUG_BLOCK(KLF_FUNC_NAME) ;
    if (pActiveBuffer != NULL) {
      pActiveBuffer->evaluated(output);
      lstBuffers->refreshBuffer(pActiveBuffer);
    }
  }

  void on_btnNew_clicked()
  {
    KLF_DEBUG_BLOCK(KLF_FUNC_NAME) ;
    pMainWin->slotClearLatex();
  }

  void on_btnSave_clicked()
  {
    KLF_DEBUG_BLOCK(KLF_FUNC_NAME) ;
    if (pActiveBuffer != NULL) {
      pActiveBuffer->save();
      lstBuffers->refreshBuffer(pActiveBuffer);
    }
    refreshButtonsState();
  }

  void on_btnSaveAs_clicked()
  {
    KLF_DEBUG_BLOCK(KLF_FUNC_NAME) ;
    pMainWin->slotSave();
  }

  void on_btnClose_clicked()
  {
    KLF_DEBUG_BLOCK(KLF_FUNC_NAME) ;
    if (pActiveBuffer != NULL) {
      // TODO check for modifications etc....
      pBufferList.removeAll(pActiveBuffer);
      lstBuffers->setBufferList(pBufferList, NULL);
    }
  }

  void on_btnOpen_clicked()
  {
    KLF_DEBUG_BLOCK(KLF_FUNC_NAME) ;
    QString fn = QFileDialog::getOpenFileName(this, tr("Open a file"));
    if (fn.isEmpty())
      return; // cancelled by user

    bool ok = pMainWin->openFile(fn);
    if (!ok) {
      klfWarning("Failed to open file "<<fn) ;
    }
  }

  void on_btnSaveAll_clicked()
  {
    /** \todo ............ */
  }

  void setActiveBuffer(OpenBuffer * buffer)
  {
    KLF_DEBUG_BLOCK(KLF_FUNC_NAME) ;
    pActiveBuffer = buffer;
    if (pActiveBuffer != NULL) {
      pActiveBuffer->activate();
    } else {
      pMainWin->slotClear();
    }
    refreshButtonsState();
  }

  void refreshButtonsState()
  {
    btnSaveAll->setEnabled(false);
    if (pActiveBuffer != NULL) {
      // there is an active buffer
      btnSave->show();
      btnSave->setEnabled(pActiveBuffer->ismodified());
      btnSaveAs->hide();
      btnClose->setEnabled(true);
    } else {
      // no active buffer
      btnSaveAs->show();
      btnSave->hide();
      btnClose->setEnabled(false);
    }
  }

private:
  KLFMainWin * pMainWin;

  QList<OpenBuffer*> pBufferList;
  OpenBuffer * pActiveBuffer;
};





class BuffersPlugin : public QObject, public KLFPluginGenericInterface
{
  Q_OBJECT
  Q_INTERFACES(KLFPluginGenericInterface)
public:
  BuffersPlugin() : QObject(qApp) { }
  virtual ~BuffersPlugin() { }

  virtual QVariant pluginInfo(PluginInfo which) const {
    switch (which) {
    case PluginName: return QString("buffers");
    case PluginAuthor: return QString("Philippe Faist <philippe.faist")+QString("@bluewin.ch>");
    case PluginTitle: return tr("Buffers");
    case PluginDescription: return tr("Work in multiple buffers");
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
