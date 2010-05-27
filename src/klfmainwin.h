/***************************************************************************
 *   file klfmainwin.h
 *   This file is part of the KLatexFormula Project.
 *   Copyright (C) 2007 by Philippe Faist
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

#ifndef KLFMAINWIN_H
#define KLFMAINWIN_H

#include <stdio.h>

#include <QObject>
#include <QList>
#include <QFont>
#include <QCheckBox>
#include <QMenu>
#include <QTextEdit>
#include <QWidget>
#include <QMimeData>
#include <QThread>
#include <QMutex>
#include <QWaitCondition>

#include <klfbackend.h>

#include <klflib.h>
#include <klfconfig.h>
//#include <klfdata.h>
#include <klflatexsymbols.h>
#include <klflatexsyntaxhighlighter.h>

#include <ui_klfmainwinui.h>


//class KLFLibraryBrowser;
class KLFLibBrowser;
class KLFLatexSymbols;
class KLFStyleManager;
class KLFSettings;



namespace Ui {
  class KLFProgErrUI;
}

class KLF_EXPORT KLFProgErr : public QDialog
{
  Q_OBJECT
public:
  KLFProgErr(QWidget *parent, QString errtext);
  virtual ~KLFProgErr();

  static void showError(QWidget *parent, QString text);

private:
  Ui::KLFProgErrUI *mUI;
};



/**
 * A helper that runs in a different thread that generates previews in real-time as user types text,
 * without blocking the GUI.
 */
class KLF_EXPORT KLFPreviewBuilderThread : public QThread
{
  Q_OBJECT

public:
  KLFPreviewBuilderThread(QObject *parent, KLFBackend::klfInput input, KLFBackend::klfSettings settings,
			  int labelwidth, int labelheight);
  virtual ~KLFPreviewBuilderThread();
  void run();

signals:
  void previewAvailable(const QImage& preview, bool latexerror);

public slots:
  bool inputChanged(const KLFBackend::klfInput& input);
  void settingsChanged(const KLFBackend::klfSettings& settings, int labelwidth, int labelheight);

protected:
  KLFBackend::klfInput _input;
  KLFBackend::klfSettings _settings;
  int _lwidth, _lheight;

  QMutex _mutex;
  QWaitCondition _condnewinfoavail;

  bool _hasnewinfo;
  bool _abort;
};



/**
 * KLatexFormula Main Window
 * \author Philippe Faist &lt;philippe.faist@bluewin.ch&gt;
 */
class KLF_EXPORT KLFMainWin : public QWidget, private Ui::KLFMainWinUI
{
  Q_OBJECT
  Q_PROPERTY(QString widgetStyle READ widgetStyle WRITE setWidgetStyle)

public:
  KLFMainWin();
  virtual ~KLFMainWin();

  bool eventFilter(QObject *obj, QEvent *event);

  KLFStyle currentStyle() const;

  KLFBackend::klfSettings backendSettings() const { return _settings; }

  virtual QFont txtLatexFont() const { return txtLatex->font(); }
  virtual QFont txtPreambleFont() const { return txtPreamble->font(); }

  KLFBackend::klfSettings currentSettings() const { return _settings; }

  void applySettings(const KLFBackend::klfSettings& s);

  KLFBackend::klfOutput currentKLFBackendOutput() const { return _output; }

  enum altersetting_which { altersetting_LBorderOffset = 100,
			    altersetting_TBorderOffset,
			    altersetting_RBorderOffset,
			    altersetting_BBorderOffset,
			    altersetting_TempDir,
			    altersetting_Latex,
			    altersetting_Dvips,
			    altersetting_Gs,
			    altersetting_Epstopdf };
  void alterSetting(altersetting_which, int ivalue);
  void alterSetting(altersetting_which, QString svalue);

  QMimeData * resultToMimeData();
  
  KLFLibBrowser * libBrowserWidget() { return mLibBrowser; }
  KLFLatexSymbols * latexSymbolsWidget() { return mLatexSymbols; }
  KLFStyleManager * styleManagerWidget() { return mStyleManager; }
  KLFSettings * settingsDialog() { return mSettingsDialog; }
  QMenu * styleMenu() { return mStyleMenu; }
  KLFLatexSyntaxHighlighter * syntaxHighlighter() { return mHighlighter; }
  KLFLatexSyntaxHighlighter * preambleSyntaxHighlighter() { return mPreambleHighlighter; }

  KLFConfig * klfConfig() { return & klfconfig; }

  enum KLFWindowsEnum {
    MainWin = 0x01,
    LatexSymbols = 0x02,
    LibBrowser = 0x04,
    SettingsDialog = 0x08,
    StyleManager = 0x10
  };
  uint currentWindowShownStatus();

  QString widgetStyle() const { return _widgetstyle; }

signals:

  void stylesChanged(); // dialogs (e.g. stylemanager) should connect to this in case styles change unexpectedly
  //  void libraryAllChanged();

public slots:

  void slotEvaluate();
  void slotClear();
  void slotLibrary(bool showlib);
  void slotLibraryButtonRefreshState(bool on);
  void slotSymbols(bool showsymbs);
  void slotSymbolsButtonRefreshState(bool on);
  void slotExpandOrShrink();
  void slotSetLatex(const QString& latex);
  void slotSetMathMode(const QString& mathmode);
  void slotSetPreamble(const QString& preamble);
  void slotSetDPI(int DPI);
  void slotSetFgColor(const QColor& fgcolor);
  void slotSetFgColor(const QString& fgcolor);
  void slotSetBgColor(const QColor& bgcolor);
  void slotSetBgColor(const QString& bgcolor);

  // will actually save only if output non empty.
  void slotEvaluateAndSave(const QString& output, const QString& format);

  bool importCmdlKLFFiles(const QStringList& files, bool showLibrary = true);
  bool importCmdlKLFFile(const QString& file, bool showLibrary = false);

  bool loadNamedStyle(const QString& sty);

  void slotDrag();
  void slotCopy();
  void slotSave(const QString& suggestedFname = QString::null);

  void slotActivateEditor();

  void slotShowBigPreview();

  void slotPresetDPISender();
  void slotLoadStyle(int stylenum);
  void slotLoadStyle(const KLFStyle& style);
  void slotSaveStyle();
  void slotStyleManager();
  void slotSettings();


  void refreshWindowSizes();

  void refreshStylePopupMenus();
  void loadStyles();
  void loadLibrary(); // load library stuff
  void saveStyles();
  void restoreFromLibrary(const KLFLibEntry& entry, uint restoreflags);
  void insertSymbol(const KLFLatexSymbol& symbol);
  void saveSettings();
  void loadSettings();

  void showAbout();

  void setWidgetStyle(const QString& qtstyle);

  void setTxtLatexFont(const QFont& f) { txtLatex->setFont(f); }
  void setTxtPreambleFont(const QFont& f) { txtPreamble->setFont(f); }

  void showRealTimePreview(const QImage& preview, bool latexerror);

  void updatePreviewBuilderThreadInput();

  void displayError(const QString& errormsg);

  void setWindowShownStatus(uint windowshownflags, bool setMainWinToo = false);

  void setQuitOnClose(bool quitOnClose);

  void quit();

private slots:
  void slotLoadStyleAct(); // private : only as slot to an action containing the style # as user data

protected:
  KLFLibBrowser *mLibBrowser;
  KLFLatexSymbols *mLatexSymbols;
  KLFStyleManager *mStyleManager;
  KLFSettings *mSettingsDialog;

  KLFLibResourceEngine *mHistoryLibResource;

  QMenu *mStyleMenu;

  KLFLatexSyntaxHighlighter *mHighlighter;
  KLFLatexSyntaxHighlighter *mPreambleHighlighter;

  KLFBackend::klfSettings _settings; // settings we pass to KLFBackend
  bool _settings_altered;

  KLFBackend::klfOutput _output; // output from KLFBackend
  KLFBackend::klfInput _lastrun_input; // input that generated _output

  /** If TRUE, then the output contained in _output is up-to-date, meaning that we favor displaying
   * _output.result instead of the image given by mPreviewBuilderThread. */
  bool _evaloutput_uptodate;
  /** The Thread that will create real-time previews of formulas. */
  KLFPreviewBuilderThread *mPreviewBuilderThread;

  KLFStyleList _styles;

  QSize _shrinkedsize;
  QSize _expandedsize;

  KLFBackend::klfInput collectInput();

  bool event(QEvent *e);
  void closeEvent(QCloseEvent *e);
  void hideEvent(QHideEvent *e);
  void showEvent(QShowEvent *e);

  bool _ignore_close_event;

  int _lastwindowshownstatus;
  QMap<int, QRect> _lastwindowgeometries;
  int _savedwindowshownstatus;

  QString _widgetstyle;
};

#endif
