/***************************************************************************
 *   file klfkateplugin.h
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


class KLFKtePluginView;


class KLFKtePlugin  :  public KTextEditor::Plugin
{
public:
  explicit KLFKtePlugin(QObject *parent = 0,
			const QVariantList &args = QVariantList());
  virtual ~KLFKtePlugin();
  
  static KLFKtePlugin *self() { return plugin; }
  
  void addView(KTextEditor::View *view);
  void removeView(KTextEditor::View *view);

  void readConfig();
  void writeConfig();
  
  virtual void readConfig (KConfig *) {}
  virtual void writeConfig (KConfig *) {}
  
private:
  static KLFKtePlugin *plugin;
  
  QList<KLFKtePluginView*> pViews;
};


class KLFKteLatexRunThread  :  public QThread
{
  Q_OBJECT
public:
  KLFKteLatexRunThread(QObject *parent = NULL);
  virtual ~KLFKteLatexRunThread();
  void run();

signals:
  void previewError(const QString& errorString, int errorCode);
  void previewAvailable(const QImage& preview);

public slots:
  bool setNewInput(const KLFBackend::klfInput& input);
  void setSettings(const KLFBackend::klfSettings& settings);

protected:
  KLFBackend::klfInput _input;
  KLFBackend::klfSettings _settings;

  QMutex _mutex;
  QWaitCondition _condnewinfoavail;

  bool _hasnewinfo;
  bool _abort;
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
  
protected:
  virtual void paintEvent(QPaintEvent *e);
  
private:
  QPixmap pPix;
};

class KLFKtePreviewWidget : public QWidget
{
  Q_OBJECT
public:
  KLFKtePreviewWidget(KTextEditor::View *viewparent);
  virtual ~KLFKtePreviewWidget();

  bool eventFilter(QObject *obj, QEvent *event);

signals:
  void invokeKLF();

public Q_SLOTS:
  void showPreview(const QImage& img, QWidget *view, const QPoint& pos);

private Q_SLOTS:
  void linkActivated(const QString& url);
  
protected:
  void paintEvent(QPaintEvent *event);

private:
  KLFKtePixmapWidget *lbl;
  QLabel *klfLinks;
};



class KLFKtePluginView  :  public QObject, public KXMLGUIClient
{
  Q_OBJECT
public:
  explicit KLFKtePluginView(KTextEditor::View *view = 0);
  ~KLFKtePluginView();

private:
  KTextEditor::View *pView;
  bool pIsGoodHighlightingMode;
  
  struct MathContext {
    bool isValidMathContext;
    QString latexequation;
    QString mathmodebegin;
    QString mathmodeend;
    QString klfmathmode;
  };
  MathContext pCurMathContext;

  KLFBackend::klfSettings klfsettings;

  KLFKteLatexRunThread *pLatexRunThread;
  KLFKtePreviewWidget *pPreview;

  KAction *aPreviewSel;
  KAction *aInvokeKLF;

  bool pPreventNextShow;

private Q_SLOTS:
  void slotHighlightingModeChanged(KTextEditor::Document *document);
  void slotReparseCurrentContext();
  void slotSelectionChanged();
  void slotContextMenuAboutToShow(KTextEditor::View *view, QMenu * menu);

  void slotPreview();
  void slotPreview(const MathContext& context);
  void slotHidePreview();
  void slotInvokeKLF();

  void slotReadyPreview(const QImage& img);
};


K_PLUGIN_FACTORY_DECLARATION(KLFKtePluginFactory)


#endif

