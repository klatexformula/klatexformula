/***************************************************************************
 *   file klfmainwin.h
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
#include <QDialog>

#include <klfbackend.h>

#include <klfguiutil.h>

#include <klflib.h>
#include <klfconfig.h>
#include <klflatexsymbols.h>



class KLFLibBrowser;
class KLFStyleManager;
class KLFSettings;
class KLFLatexSyntaxHighlighter;
class KLFLatexEdit;



namespace Ui {
  class KLFProgErr;
  class KLFMainWin;
}

class KLFMainWin;


class KLF_EXPORT KLFProgErr : public QDialog
{
  Q_OBJECT
public:
  KLFProgErr(QWidget *parent, QString errtext);
  virtual ~KLFProgErr();

  static void showError(QWidget *parent, QString text);

private:
  Ui::KLFProgErr *u;
};


/** A helper interface class to implement more export formats to save output (to file).
 */
class KLFAbstractOutputSaver
{
public:
  KLFAbstractOutputSaver() { }
  virtual ~KLFAbstractOutputSaver() { }

  /** Returns a list of mime-types of supported file formats */
  virtual QStringList supportedMimeFormats() = 0;

  /** Returns the human-readable, (possibly translated,) label to display in save dialog that
   * the user can select to save in this format.
   *
   * \param key is a mime-type returned by \ref supportedMimeFormats().
   */
  virtual QString formatTitle(const QString& key) = 0;

  /** Returns the file pattern(s) that the files of this format (normally) match.
   * syntax is simple pattern eg. \c "*.png".
   *
   * The patterns are joined to spaces to form a filter that is given to QFileDialog.
   */
  virtual QStringList formatFilePatterns(const QString& key) = 0;

  /** Actually save to the file \c fileName, using the format \c key.
   *
   * The subclass is responsible for notifying the user of possible errors that have occurred.
   *
   * Overwrite confirmation has already been required (if applicable).
   */
  virtual bool saveToFile(const QString& key, const QString& fileName, const KLFBackend::klfOutput& output) = 0;
};

/** A helper interface class to open old PNG files, library files or abstract data, to fill in
 * the main window controls (latex and style), or possibly open a resource into library.
 *
 * Instances of subclasses will be invoked when:
 *  - a file is given on command-line, see \ref canOpenFile() and \ref openFile()
 *  - data is pasted or dropped in editor, see \ref supportedMimeTypes(), \ref canOpenData() and
 *    \ref openData()
 *
 */
class KLF_EXPORT KLFAbstractDataOpener
{
public:
  KLFAbstractDataOpener(KLFMainWin *mainwin) : mMainWin(mainwin) { }
  virtual ~KLFAbstractDataOpener() { }

  /** Returns a list of mime-types we can handle */
  virtual QStringList supportedMimeTypes() = 0;

  /** Is supposed to peek into \c file to try to recognize if its format is one which we can open.
   * The implementation of this function may also rely on the file name's extension.
   *
   * If the file is recognized as one this opener can open, then return TRUE, otherwise return FALSE.
   */
  virtual bool canOpenFile(const QString& file) = 0;

  /** Is supposed to peek into \c data to try to recognize if its format is one which we can open.
   * No indication is given as to which format \c data is in. If the \c data is recognized as
   * a format this opener can open, return TRUE, otherwise, return FALSE. */
  virtual bool canOpenData(const QByteArray& data) = 0;

  /** Actually open the file. You may use the mainWin() to perform something useful.
   *
   * Note: this function will be called for every file the main window tries to open. Do NOT assume
   *   that the file given here is a file that passed the canOpenFile() function test. (Reason:
   *   calling both canOpenFile() and openFile() may result into resources being loaded twice, which
   *   is not optimal).
   *
   * This function should return FALSE if it is not capable of loading the given \c file.
   */
  virtual bool openFile(const QString& file) = 0;

  /** Actually open the data. You may use the mainWin() to perform something useful.
   *
   * \c mimetype is the mime-type of the data.
   *
   * Note: the \c mimetype can be empty, in which case the opener should make no assumption
   *   whatsoever as to the data's format, and try to parse data, and return FALSE if it
   *   is not capable of loading the given data. In particular, it should not be assumed
   *   that canOpenData() has already been called and returned true on this data.
   *
   * This function should return FALSE if it is not capable of loading the given \c data.
   */
  virtual bool openData(const QByteArray& data, const QString& mimetype) = 0;

protected:
  /** Get a pointer to the main window passed to the constructor. */
  KLFMainWin * mainWin() { return mMainWin; }

private:
  KLFMainWin *mMainWin;
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


class KLFAboutDialog;
class KLFWhatsNewDialog;
class KLFMainWinPopup;

/**
 * KLatexFormula Main Window
 * \author Philippe Faist &lt;philippe.faist@bluewin.ch&gt;
 */
class KLF_EXPORT KLFMainWin : public QWidget, public KLFDropDataHandler
{
  Q_OBJECT
  Q_PROPERTY(QString widgetStyle READ widgetStyle WRITE setWidgetStyle)

public:
  KLFMainWin();
  virtual ~KLFMainWin();

  /** called by main.cpp right after show(), just before entering into event loop. */
  void startupFinished();

  bool eventFilter(QObject *obj, QEvent *event);

  KLFStyle currentStyle() const;

  KLFBackend::klfSettings backendSettings() const { return _settings; }

  virtual QFont txtLatexFont() const;
  virtual QFont txtPreambleFont() const;

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
			    altersetting_Epstopdf,
			    altersetting_OutlineFonts, //!< bool given as an int value
			    altersetting_CalcEpsBoundingBox, //!< bool given as an int value
			    altersetting_WantPostProcessedEps //!< bool given as an int value
  };
  /** This function allows to temporarily modify a given setting with a new value. KLatexFormula
   * will NOT remember the new setting in later executions.
   *
   * Used eg. for command-line mode.
   *
   * Note you have to use the correct function for each setting, if the setting requires an int
   * use this function, if it requires a string use alterSetting(altersetting_which, QString).
   */
  void alterSetting(altersetting_which, int ivalue);
  /** See alterSetting(altersetting_which, int) */
  void alterSetting(altersetting_which, QString svalue);

  KLFLibBrowser * libBrowserWidget() { return mLibBrowser; }
  KLFLatexSymbols * latexSymbolsWidget() { return mLatexSymbols; }
  KLFStyleManager * styleManagerWidget() { return mStyleManager; }
  KLFSettings * settingsDialog() { return mSettingsDialog; }
  QMenu * styleMenu() { return mStyleMenu; }
  KLFLatexEdit *latexEdit();
  KLFLatexSyntaxHighlighter * syntaxHighlighter();
  KLFLatexSyntaxHighlighter * preambleSyntaxHighlighter();

  KLFConfig * klfConfig() { return & klfconfig; }

  QHash<QWidget*,bool> currentWindowShownStatus(bool mainWindowToo = false);
  QHash<QWidget*,bool> prepareAllWindowShownStatus(bool visibleStatus, bool mainWindowToo = false);

  QString widgetStyle() const { return _widgetstyle; }

  void registerHelpLinkAction(const QString& path, QObject *object, const char * member, bool wantUrlParam);

  void registerOutputSaver(KLFAbstractOutputSaver *outputsaver);
  void unregisterOutputSaver(KLFAbstractOutputSaver *outputsaver);

  void registerDataOpener(KLFAbstractDataOpener *dataopener);
  void unregisterDataOpener(KLFAbstractDataOpener *dataopener);

  bool canOpenFile(const QString& fileName);
  bool canOpenData(const QByteArray& data);
  bool canOpenData(const QMimeData *mimeData);

  //! Reimplemented from KLFDropDataHandler
  bool canOpenDropData(const QMimeData * data) { return canOpenData(data); }

signals:

  void evaluateFinished(const KLFBackend::klfOutput& output);

  // dialogs (e.g. stylemanager) should connect to this in case styles change unexpectedly
  void stylesChanged();

  void applicationLocaleChanged(const QString& newLocale);

  void klfConfigChanged();

public slots:

  void slotEvaluate();
  void slotClear() { slotClearLatex(); }
  void slotClearLatex();
  void slotClearAll();
  void slotLibrary(bool showlib);
  void slotLibraryButtonRefreshState(bool on);
  void slotSymbols(bool showsymbs = true);
  void slotSymbolsButtonRefreshState(bool on);
  void slotExpandOrShrink();
  void slotExpand(bool expanded = true);
  void slotSetLatex(const QString& latex);
  void slotSetMathMode(const QString& mathmode);
  void slotSetPreamble(const QString& preamble);
  /** If \c line is already in the preamble, then does nothing. Otherwise appends
   * \c line to the preamble text. */
  void slotEnsurePreambleCmd(const QString& line);
  void slotSetDPI(int DPI);
  void slotSetFgColor(const QColor& fgcolor);
  void slotSetFgColor(const QString& fgcolor);
  void slotSetBgColor(const QColor& bgcolor);
  void slotSetBgColor(const QString& bgcolor);

  // will actually save only if output non empty.
  void slotEvaluateAndSave(const QString& output, const QString& format);

  bool openFile(const QString& file);
  bool openFiles(const QStringList& fileList);
  bool openData(const QMimeData *mimeData, bool *openerFound = NULL);
  //! Reimplemented from KLFDropDataHandler
  int openDropData(const QMimeData *mimeData);
  bool openData(const QByteArray& data);

  bool openLibFiles(const QStringList& files, bool showLibrary = true);
  bool openLibFile(const QString& file, bool showLibrary = true);

  void setApplicationLocale(const QString& locale);

  void retranslateUi(bool alsoBaseUi = true);

  bool loadDefaultStyle();
  bool loadNamedStyle(const QString& sty);

  void slotDrag();
  void slotCopy();
  void slotSave(const QString& suggestedFname = QString::null);
  void slotSetExportProfile(const QString& exportProfile);

  void slotActivateEditor();
  void slotActivateEditorSelectAll();

  void slotShowBigPreview();

  void slotPresetDPISender();
  void slotLoadStyle(int stylenum);
  void slotLoadStyle(const KLFStyle& style);
  void slotSaveStyle();
  void slotStyleManager();
  void slotSettings();


  void refreshWindowSizes();

  void refreshShowCorrectClearButton();

  void refreshStylePopupMenus();
  void loadStyles();
  void loadLibrary(); // load library stuff
  void loadLibrarySavedState();
  void saveStyles();
  void restoreFromLibrary(const KLFLibEntry& entry, uint restoreflags);
  void insertSymbol(const KLFLatexSymbol& symbol);
  /** Inserts a delimiter \c delim, and brings the cursor \c charsBack characters
   * back. Eg. you can insert \c "\mathrm{}" and bring the cursor 1 space back. */
  void insertDelimiter(const QString& delim, int charsBack = 1);
  void saveSettings();
  void saveLibraryState();
  void loadSettings();

  void addWhatsNewText(const QString& htmlSnipplet);

  void showAbout();
  void showWhatsNew();
  void showSettingsHelpLinkAction(const QUrl& link);
  void helpLinkAction(const QUrl& link);

  void setWidgetStyle(const QString& qtstyle);

  void setTxtLatexFont(const QFont& f);
  void setTxtPreambleFont(const QFont& f);

  void showRealTimePreview(const QImage& preview, bool latexerror);

  void updatePreviewBuilderThreadInput();

  void displayError(const QString& errormsg);

  void setWindowShownStatus(const QHash<QWidget*,bool>& windowshownflags);

  void refreshAllWindowStyleSheets();

  void setQuitOnClose(bool quitOnClose);

  void quit();

private slots:
  // private : only as slot to an action containing the style # as user data
  void slotLoadStyleAct();

  void slotOpenHistoryLibraryResource();

  void slotNewSymbolTyped(const QString& symbol);
  void slotPopupClose();
  void slotPopupAction(const QUrl& helpLinkUrl);
  void slotPopupAcceptAll();

  void slotEditorContextMenuInsertActions(const QPoint& pos, QList<QAction*> *actionList);
  void slotInsertMissingPackagesFromActionSender();

protected:
  Ui::KLFMainWin *u;

  KLFLibBrowser *mLibBrowser;
  KLFLatexSymbols *mLatexSymbols;
  KLFStyleManager *mStyleManager;
  KLFSettings *mSettingsDialog;
  KLFAboutDialog *mAboutDialog;
  KLFWhatsNewDialog *mWhatsNewDialog;

  KLFMainWinPopup *mPopup;

  /** \internal */
  struct HelpLinkAction {
    HelpLinkAction(const QString& p, QObject *obj, const char *func, bool param)
      : path(p), reciever(obj), memberFunc(func), wantParam(param) { }
    QString path;
    QObject *reciever;
    QByteArray memberFunc;
    bool wantParam;
  };
  QList<HelpLinkAction> mHelpLinkActions;

  KLFLibResourceEngine *mHistoryLibResource;

  KLFStyleList _styles;

  bool try_load_style_list(const QString& fileName);

  QMenu *mStyleMenu;

  bool _loadedlibrary;
  bool _firstshow;

  KLFBackend::klfSettings _settings; // settings we pass to KLFBackend
  bool _settings_altered;

  KLFBackend::klfOutput _output; // output from KLFBackend
  //  KLFBackend::klfInput _lastrun_input; // input that generated _output (now _output.input)

  /** If TRUE, then the output contained in _output is up-to-date, meaning that we favor displaying
   * _output.result instead of the image given by mPreviewBuilderThread. */
  bool _evaloutput_uptodate;
  /** The Thread that will create real-time previews of formulas. */
  KLFPreviewBuilderThread *mPreviewBuilderThread;

  QLabel *mExportMsgLabel;
  void showExportMsgLabel(const QString& msg, int timeout = 3000);
  int pExportMsgLabelTimerId;

  /** Returns the input corresponding to the current GUI state. If \c isFinal is TRUE, then
   * the input data may be "remembered" as used (the exact effect depends on the setting), eg.
   * math mode is memorized into combo box choices. Typically \c isFinal is TRUE when called
   * from slotEvaluate() and FALSE when called to update the preview builder thread. */
  KLFBackend::klfInput collectInput(bool isFinal);

  QList<QAction*> pExportProfileQuickMenuActionList;

  QSize _shrinkedsize;
  QSize _expandedsize;

  bool event(QEvent *e);
#ifdef Q_WS_X11
  bool x11Event(XEvent *event);
#endif
  void childEvent(QChildEvent *e);
  void closeEvent(QCloseEvent *e);
  void hideEvent(QHideEvent *e);
  void showEvent(QShowEvent *e);
  void timerEvent(QTimerEvent *e);

  bool _ignore_close_event;

  QList<QWidget*> pWindowList;
  /** "last" window status flags are used in eventFilter() to detect individual dialog
   * geometries resetting */
  QHash<QWidget*,bool> pLastWindowShownStatus;
  QHash<QWidget*,QRect> pLastWindowGeometries;
  /** "saved" window status flags are used in hideEvent() to save the individual dialog visible
   * states, as the "last" status flags will be overridden by all the windows hiding. */
  QHash<QWidget*,bool> pSavedWindowShownStatus;
  //  QHash<QWidget*, bool> _lastwindowshownstatus;
  //  QHash<QWidget*, QRect> _lastwindowgeometries;
  //  QHash<QWidget*, bool> _savedwindowshownstatus;

  QString _widgetstyle;

  void getMissingCmdsFor(const QString& symbol, QStringList * missingCmds, QString *guiText,
			 bool wantHtmlText = true);

  QList<KLFAbstractOutputSaver*> pOutputSavers;
  QList<KLFAbstractDataOpener*> pDataOpeners;
};

#endif
