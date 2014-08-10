/***************************************************************************
 *   file klfkateplugin.h
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

#ifndef KLFKATEPLUGIN_H
#define KLFKATEPLUGIN_H

#include <QEvent>
#include <QObject>
#include <QList>
#include <QVariantList>
#include <QThread>
#include <QWaitCondition>
#include <QMutex>
#include <QPixmap>
#include <QLabel>
#include <QMenu>

#include <kaction.h>
#include <ktexteditor/plugin.h>
#include <ktexteditor/view.h>
#include <kxmlguiclient.h>
#include <klocalizedstring.h>
#include <kpluginfactory.h>

#include <klfbackend.h>
#include <klflatexpreviewthread.h>

#include <klfkteparser.h>


class KLFKtePluginView;


// defined in klfkateplugin_config.cpp

class KLFKteConfigData
{
public:
  KLFKteConfigData();

  void readConfig(KConfigGroup *cfg);
  void writeConfig(KConfigGroup *cfg);

  bool autopopup;
  bool onlyLatexMode;
  int transparencyPercent;
  QString preamble;
  QString klfpath;
  QSize popupMaxSize;
  bool popupLinks;
};





class KLFKtePlugin  :  public KTextEditor::Plugin
{
  Q_OBJECT
public:
  explicit KLFKtePlugin(QObject *parent = 0,
			const QVariantList &args = QVariantList());
  virtual ~KLFKtePlugin();
  
  void addView(KTextEditor::View *view);
  void removeView(KTextEditor::View *view);

  void readConfig();
  void writeConfig();
  
  virtual void readConfig(KConfig *) { }
  virtual void writeConfig(KConfig *) { }
  
  inline KLFKteConfigData * configData() { return pConfigData; }
  KLFLatexPreviewThread * latexPreviewThread() { return pLatexPreviewThread; }
  inline static KLFKtePlugin * getStaticInstance() { return staticInstance; }

public slots:
  void setDontPopupAutomatically();

private:
  QList<KLFKtePluginView*> pViews;

  KLFKteConfigData * pConfigData;

  KLFLatexPreviewThread * pLatexPreviewThread;

  // only used by config module; if this is non-NULL, then this object will be notified if
  // the configuration changes.
  static KLFKtePlugin * staticInstance;
};



class KLFKtePixmapWidget : public QWidget
{
  Q_OBJECT
  Q_PROPERTY(QPixmap pix READ pix WRITE setPix)
public:
  KLFKtePixmapWidget(QWidget *parent);
  virtual ~KLFKtePixmapWidget();
  
  virtual QPixmap pix() const { return pPix; }
  
  virtual QSize sizeHint() const { return pPix.size(); }
  
public Q_SLOTS:
  virtual void setPix(const QPixmap& pix);
  void setSemiTransparent(bool val);
  
protected:
  virtual void paintEvent(QPaintEvent *e);
  
private:
  QPixmap pPix;

  bool pSemiTransparent;
};




class KLFKtePluginView;
namespace Ui { class KLFKtePreviewWidget; };

class KLFKtePreviewWidget : public QWidget
{
  Q_OBJECT
public:
  KLFKtePreviewWidget(KLFKtePluginView * pluginView, KTextEditor::View *viewparent);
  virtual ~KLFKtePreviewWidget();

  bool eventFilter(QObject *obj, QEvent *event);

  bool isCompiling() const;

signals:
  void requestedDontPopupAutomatically();
  void invokeKLF();

public Q_SLOTS:
  void showPreview(const QImage& img, QWidget *view, const QPoint& pos);
  void reshowPreview(QWidget *view, const QPoint& pos);
  void showPreviewError(const QString& strerr);
  void setCompiling(bool compiling = true);

private Q_SLOTS:
  void linkActivated(const QString& url);
  
protected:
  void paintEvent(QPaintEvent *event);

private:
  Ui::KLFKtePreviewWidget *u;

  KLFKtePluginView * pPluginView;

  KLFKtePixmapWidget *lbl;
  QLabel *klfLinks;
};



// -----


class KLFKtePluginView  :  public QObject, public KXMLGUIClient
{
  Q_OBJECT
public:
  explicit KLFKtePluginView(KLFKtePlugin * mainPlugin, KTextEditor::View *view);
  virtual ~KLFKtePluginView();


  inline KLFKteConfigData * configData() {
    KLF_ASSERT_NOT_NULL(pMainPlugin, "!?!? pMainPlugin is NULL!", return NULL; ) ;
    return pMainPlugin->configData();
  }

signals:
  void requestedDontPopupAutomatically();

private:
  KLFKtePlugin * pMainPlugin;

  KTextEditor::View *pView;
  bool pIsGoodHighlightingMode;

  KLFKteParser *pParser;
  
  MathModeContext pCurMathContext;

  KLFBackend::klfSettings klfsettings;

  KLFContLatexPreview * pContLatexPreview;
  KLFKtePreviewWidget *pPreview;

  QImage pLastPreview;

  KAction *aPreviewSel;
  KAction *aInvokeKLF;
  KAction *aShowLastError;

  QString pLastError;

  bool pPreventNextShow;

private Q_SLOTS:
  void slotHighlightingModeChanged(KTextEditor::Document *document);
  void slotReparseCurrentContext();
  void slotSelectionChanged();
  void slotContextMenuAboutToShow(KTextEditor::View *view, QMenu * menu);

  void slotPreparePreview();
  void slotShowSamePreview();
  void slotPreparePreview(const MathModeContext& context);
  void slotHidePreview();
  void slotInvokeKLF();
  void slotShowLastError();

  void slotReadyPreview(const QImage& img);
  void showPreview(const QImage& img);
  void slotPreviewError(const QString& errorString, int errorCode);
};


K_PLUGIN_FACTORY_DECLARATION(KLFKtePluginFactory)


#endif

