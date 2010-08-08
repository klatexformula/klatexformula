/**
  * Copyright (C) 2010 Philippe Faist  philippe dot faist at bluewin dot ch
  *
  * This library is free software; you can redistribute it and/or
  * modify it under the terms of the GNU Library General Public
  * License version 2 as published by the Free Software Foundation.
  *
  * This library is distributed in the hope that it will be useful,
  * but WITHOUT ANY WARRANTY; without even the implied warranty of
  * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  * Library General Public License for more details.
  *
  * You should have received a copy of the GNU Library General Public License
  * along with this library; see the file COPYING.LIB.  If not, write to
  * the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
  * Boston, MA 02110-1301, USA.
  */

#ifndef KLFKATEPLUGIN_H
#define KLFKATEPLUGIN_H

#include <QEvent>
#include <QObject>
#include <QList>
#include <QVariantList>
#include <QThread>
#include <QWaitCondition>
#include <QMutex>

#include <QLabel>

#include <ktexteditor/plugin.h>
#include <ktexteditor/view.h>
#include <kxmlguiclient.h>
#include <klocalizedstring.h>
#include <kpluginfactory.h>

#define KLF_HAS_PRETTY_FUNCTION // ............... DEBUG ......................
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
  
  bool autopopup;
  QString klfpath;
  bool onlyLatexMode;
  QString preamble;
  
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
  bool setInput(const KLFBackend::klfInput& input);
  void setSettings(const KLFBackend::klfSettings& settings);

protected:
  KLFBackend::klfInput _input;
  KLFBackend::klfSettings _settings;

  QMutex _mutex;
  QWaitCondition _condnewinfoavail;

  bool _hasnewinfo;
  bool _abort;
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
void showPreview(const QImage& img, const QPoint& pos);

private Q_SLOTS:
  void linkActivated(const QString& url);

private:
  QLabel *lbl;
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

private Q_SLOTS:
  void slotHighlightingModeChanged(KTextEditor::Document *document);
  void slotReparseCurrentContext();
  void slotSelectionChanged();

  void slotPreview();
  void slotPreview(const MathContext& context);
  void slotHidePreview();
  void slotInvokeKLF();

  void slotReadyPreview(const QImage& img);

};


K_PLUGIN_FACTORY_DECLARATION(KLFKtePluginFactory)


#endif

