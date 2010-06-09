/***************************************************************************
 *   file klfmainwin.cpp
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

#include <stdio.h>

#include <QComboBox>
#include <QLineEdit>
#include <QTextEdit>
#include <QLabel>
#include <QCheckBox>
#include <QImage>
#include <QPushButton>
#include <QFile>
#include <QFont>
#include <QFontInfo>
#include <QClipboard>
#include <QDrag>
#include <QTimer>
#include <QApplication>
#include <QMessageBox>
#include <QShortcut>
#include <QToolTip>
#include <QWhatsThis>
#include <QProcess>
#include <QFileDialog>
#include <QKeyEvent>
#include <QMimeData>
#include <QImageWriter>
#include <QInputDialog>
#include <QCloseEvent>
#include <QTemporaryFile>
#include <QByteArray>
#include <QBuffer>
#include <QResource>
#include <QDomDocument>
#include <QDomElement>
#include <QStyle>
#include <QStyleFactory>
#include <QDebug>
#include <QProgressDialog>
#include <QDesktopServices>
#include <QDesktopWidget>

#include <klfbackend.h>

#include <ui_klfprogerrui.h>
#include <ui_klfmainwin.h>

#include "klfutil.h"
#include "klflatexsyntaxhighlighter.h"
#include "klflibbrowser.h"
#include "klflibdbengine.h"
#include "klflatexsymbols.h"
#include "klfsettings.h"
#include "klfmain.h"
#include "klfstylemanager.h"
#include "klfmime.h"

#include "klfmainwin.h"
#include "klfmainwin_p.h"


#define DEBUG_GEOM_ARGS(g) (g).topLeft().x(), (g).topLeft().y(), (g).size().width(), (g).size().height()

static QRect klf_get_window_geometry(QWidget *w)
{
#if defined(Q_WS_X11)
  QRect g = w->frameGeometry();
#else
  QRect g = w->geometry();
#endif
  return g;
}

static void klf_set_window_geometry(QWidget *w, QRect g)
{
  if ( ! g.isValid() )
    return;

  w->setGeometry(g);
}








// ------------------------------------------------------------------------

KLFProgErr::KLFProgErr(QWidget *parent, QString errtext) : QDialog(parent)
{
  u = new Ui::KLFProgErrUI;
  u->setupUi(this);
  setObjectName("KLFProgErr");

  u->txtError->setText(errtext);
}

KLFProgErr::~KLFProgErr()
{
  delete u;
}

void KLFProgErr::showError(QWidget *parent, QString errtext)
{
  KLFProgErr dlg(parent, errtext);
  dlg.exec();
}



// ----------------------------------------------------------------------------

KLFPreviewBuilderThread::KLFPreviewBuilderThread(QObject *parent, KLFBackend::klfInput input,
						 KLFBackend::klfSettings settings, int labelwidth,
						 int labelheight)
  : QThread(parent), _input(input), _settings(settings), _lwidth(labelwidth), _lheight(labelheight),
    _hasnewinfo(false), _abort(false)
{
}
KLFPreviewBuilderThread::~KLFPreviewBuilderThread()
{
  _mutex.lock();
  _abort = true;
  _condnewinfoavail.wakeOne();
  _mutex.unlock();
  wait();
}

void KLFPreviewBuilderThread::run()
{
  KLFBackend::klfInput input;
  KLFBackend::klfSettings settings;
  KLFBackend::klfOutput output;
  QImage img;
  int lwid, lhgt;

  for (;;) {
    if (_abort)
      return;
    // fetch info
    _mutex.lock();
    input = _input;
    settings = _settings;
    settings.epstopdfexec = "";
    lwid = _lwidth;
    lhgt = _lheight;
    _hasnewinfo = false;
    _mutex.unlock();
    // render equation
    //  no performance improvement noticed with lower DPI:
    //    // force 240 DPI (we're only a preview...)
    //    input.dpi = 240;

    if ( input.latex.isEmpty() ) {
      emit previewAvailable(QImage(), 0);
    } else {
      // and GO!
      output = KLFBackend::getLatexFormula(input, settings);
      img = output.result;
      if (output.status == 0) {
	if (img.width() > lwid || img.height() > lhgt)
	  img = img.scaled(QSize(lwid, lhgt), Qt::KeepAspectRatio, Qt::SmoothTransformation);
      } else {
	img = QImage();
      }

      emit previewAvailable(img, output.status != 0);
    }
    if (_abort)
      return;
    if (_hasnewinfo)
      continue;
    _mutex.lock();
    _condnewinfoavail.wait(&_mutex);
    _mutex.unlock();
  }
}
bool KLFPreviewBuilderThread::inputChanged(const KLFBackend::klfInput& input)
{
  QMutexLocker mutexlocker(&_mutex);
  if (_input == input) {
    return false;
  }
  _input = input;
  _hasnewinfo = true;
  _condnewinfoavail.wakeOne();
  return true;
}
void KLFPreviewBuilderThread::settingsChanged(const KLFBackend::klfSettings& settings,
					      int lwidth, int lheight)
{
  _mutex.lock();
  _settings = settings;
  if (lwidth > 0) _lwidth = lwidth;
  if (lheight > 0) _lheight = lheight;
  _hasnewinfo = true;
  _condnewinfoavail.wakeOne();
  _mutex.unlock();
}



// ----------------------------------------------------------------------------



KLFMainWin::KLFMainWin()
  : QWidget(0, Qt::Window)
{
  u = new Ui::KLFMainWin;
  u->setupUi(this);
  setObjectName("KLFMainWin");
  setAttribute(Qt::WA_StyledBackground);

  loadSettings();

  _firstshow = true;
  _loadedlibrary = false;

  _output.status = 0;
  _output.errorstr = QString();
  _output.result = QImage();
  _output.pngdata = QByteArray();
  _output.epsdata = QByteArray();
  _output.pdfdata = QByteArray();

  // load styless
  loadStyles();

  mLatexSymbols = new KLFLatexSymbols(this);

  u->txtLatex->setFont(klfconfig.UI.latexEditFont);
  u->txtPreamble->setFont(klfconfig.UI.preambleEditFont);

  u->frmOutput->setEnabled(false);

  QMenu *DPIPresets = new QMenu(this);
  // 1200 DPI
  connect(u->aDPI1200, SIGNAL(triggered()), this, SLOT(slotPresetDPISender()));
  u->aDPI1200->setData(1200);
  DPIPresets->addAction(u->aDPI1200);
  // 600 DPI
  connect(u->aDPI600, SIGNAL(triggered()), this, SLOT(slotPresetDPISender()));
  u->aDPI600->setData(600);
  DPIPresets->addAction(u->aDPI600);
  // 300 DPI
  connect(u->aDPI300, SIGNAL(triggered()), this, SLOT(slotPresetDPISender()));
  u->aDPI300->setData(300);
  DPIPresets->addAction(u->aDPI300);
  // 150 DPI
  connect(u->aDPI150, SIGNAL(triggered()), this, SLOT(slotPresetDPISender()));
  u->aDPI150->setData(150);
  DPIPresets->addAction(u->aDPI150);
  // set menu to the button
  u->btnDPIPresets->setMenu(DPIPresets);

  mHighlighter = new KLFLatexSyntaxHighlighter(u->txtLatex, this);
  mPreambleHighlighter = new KLFLatexSyntaxHighlighter(u->txtPreamble, this);

  connect(u->txtLatex, SIGNAL(cursorPositionChanged()),
	  mHighlighter, SLOT(refreshAll()));
  connect(u->txtPreamble, SIGNAL(cursorPositionChanged()),
	  mPreambleHighlighter, SLOT(refreshAll()));

  u->lblOutput->setLabelFixedSize(klfconfig.UI.labelOutputFixedSize);

  connect(u->lblOutput, SIGNAL(labelDrag()), this, SLOT(slotDrag()));

  connect(u->btnShowBigPreview, SIGNAL(clicked()),
	  this, SLOT(slotShowBigPreview()));

  int h;
  h = u->btnDrag->sizeHint().height();  u->btnDrag->setFixedHeight(h - 5);
  h = u->btnCopy->sizeHint().height();  u->btnCopy->setFixedHeight(h - 5);
  h = u->btnSave->sizeHint().height();  u->btnSave->setFixedHeight(h - 5);

  refreshWindowSizes();

  u->frmDetails->hide();

  u->txtLatex->installEventFilter(this);

  setFixedSize(_shrinkedsize);

  // Shortcut for quit
  new QShortcut(QKeySequence(tr("Ctrl+Q")), this, SLOT(quit()), SLOT(quit()),
		Qt::ApplicationShortcut);

  // Shortcut for activating editor
  //  QShortcut *editorActivatorShortcut = 
  new QShortcut(QKeySequence(Qt::Key_F4), this, SLOT(slotActivateEditor()),
		SLOT(slotActivateEditor()), Qt::ApplicationShortcut);
  // shortcut for big preview
  new QShortcut(QKeySequence(Qt::Key_F2), this, SLOT(slotShowBigPreview()),
		SLOT(slotShowBigPreview()), Qt::WindowShortcut);

  // Create our style manager
  mStyleManager = new KLFStyleManager(&_styles, this);
  connect(mStyleManager, SIGNAL(refreshStyles()), this, SLOT(refreshStylePopupMenus()));
  connect(this, SIGNAL(stylesChanged()), mStyleManager, SLOT(slotRefresh()));

  connect(this, SIGNAL(stylesChanged()), this, SLOT(saveStyles()));
  connect(mStyleManager, SIGNAL(refreshStyles()), this, SLOT(saveStyles()));

  // load style "default" or "Default" (or translation) if one of them exists
  loadNamedStyle(tr("Default"));
  loadNamedStyle(tr("default"));
  loadNamedStyle("Default");
  loadNamedStyle("default");

  // For systematical syntax highlighting
  // make sure syntax highlighting is up-to-date at all times
  QTimer *synthighlighttimer = new QTimer(this);
  connect(synthighlighttimer, SIGNAL(timeout()), mHighlighter, SLOT(refreshAll()));
  connect(synthighlighttimer, SIGNAL(timeout()), mPreambleHighlighter, SLOT(refreshAll()));
  synthighlighttimer->start(250);


  // load library
  mLibBrowser = new KLFLibBrowser(this);


  // -- MAJOR SIGNAL/SLOT CONNECTIONS --

  connect(u->btnClear, SIGNAL(clicked()), this, SLOT(slotClear()));
  connect(u->btnEvaluate, SIGNAL(clicked()), this, SLOT(slotEvaluate()));
  connect(u->btnSymbols, SIGNAL(toggled(bool)), this, SLOT(slotSymbols(bool)));
  connect(u->btnLibrary, SIGNAL(toggled(bool)), this, SLOT(slotLibrary(bool)));
  connect(u->btnExpand, SIGNAL(clicked()), this, SLOT(slotExpandOrShrink()));
  connect(u->btnCopy, SIGNAL(clicked()), this, SLOT(slotCopy()));
  connect(u->btnDrag, SIGNAL(released()), this, SLOT(slotDrag()));
  connect(u->btnSave, SIGNAL(clicked()), this, SLOT(slotSave()));
  connect(u->btnSettings, SIGNAL(clicked()), this, SLOT(slotSettings()));
  connect(u->btnSaveStyle, SIGNAL(clicked()), this, SLOT(slotSaveStyle()));

  connect(u->btnQuit, SIGNAL(clicked()), this, SLOT(quit()));

  connect(mLibBrowser, SIGNAL(requestRestore(const KLFLibEntry&, uint)),
	  this, SLOT(restoreFromLibrary(const KLFLibEntry&, uint)));
  connect(mLibBrowser, SIGNAL(requestRestoreStyle(const KLFStyle&)),
	  this, SLOT(slotLoadStyle(const KLFStyle&)));
  //  connect(mLibBrowser, SIGNAL(refreshLibraryBrowserShownState(bool)),
  //	  this, SLOT(slotLibraryButtonRefreshState(bool)));
  connect(mLatexSymbols, SIGNAL(insertSymbol(const KLFLatexSymbol&)),
	  this, SLOT(insertSymbol(const KLFLatexSymbol&)));
  connect(mLatexSymbols, SIGNAL(refreshSymbolBrowserShownState(bool)),
	  this, SLOT(slotSymbolsButtonRefreshState(bool)));


  // our help/about dialog
  connect(u->btnHelp, SIGNAL(clicked()), this, SLOT(showAbout()));

  // -- SMALL REAL-TIME PREVIEW GENERATOR THREAD --

  mPreviewBuilderThread = new KLFPreviewBuilderThread(this, collectInput(), _settings,
						      klfconfig.UI.labelOutputFixedSize.width(),
						      klfconfig.UI.labelOutputFixedSize.height());

  connect(u->txtLatex, SIGNAL(textChanged()), this,
	  SLOT(updatePreviewBuilderThreadInput()), Qt::QueuedConnection);
  connect(u->cbxMathMode, SIGNAL(editTextChanged(const QString&)),
	  this, SLOT(updatePreviewBuilderThreadInput()),
	  Qt::QueuedConnection);
  connect(u->chkMathMode, SIGNAL(stateChanged(int)), this, SLOT(updatePreviewBuilderThreadInput()),
	  Qt::QueuedConnection);
  connect(u->colFg, SIGNAL(colorChanged(const QColor&)), this, SLOT(updatePreviewBuilderThreadInput()),
	  Qt::QueuedConnection);
  connect(u->chkBgTransparent, SIGNAL(stateChanged(int)), this, SLOT(updatePreviewBuilderThreadInput()),
	  Qt::QueuedConnection);
  connect(u->colBg, SIGNAL(colorChanged(const QColor&)), this, SLOT(updatePreviewBuilderThreadInput()),
	  Qt::QueuedConnection);

  connect(mPreviewBuilderThread, SIGNAL(previewAvailable(const QImage&, bool)),
	  this, SLOT(showRealTimePreview(const QImage&, bool)), Qt::QueuedConnection);

  if (klfconfig.UI.enableRealTimePreview) {
    mPreviewBuilderThread->start();
  }

  // CREATE SETTINGS DIALOG

  mSettingsDialog = new KLFSettings(this);


  // INSTALL SOME EVENT FILTERS... FOR show/hide EVENTS

  mLibBrowser->installEventFilter(this);
  mLatexSymbols->installEventFilter(this);
  mStyleManager->installEventFilter(this);
  mSettingsDialog->installEventFilter(this);


  // INTERNAL FLAGS

  _evaloutput_uptodate = false;

  _lastwindowshownstatus = 0;
  _savedwindowshownstatus = 0;
  _ignore_close_event = false;

  retranslateUi(false);

  connect(this, SIGNAL(applicationLocaleChanged(const QString&)), this, SLOT(retranslateUi()));
  connect(this, SIGNAL(applicationLocaleChanged(const QString&)), mLibBrowser, SLOT(retranslateUi()));
  connect(this, SIGNAL(applicationLocaleChanged(const QString&)), mSettingsDialog, SLOT(retranslateUi()));
  connect(this, SIGNAL(applicationLocaleChanged(const QString&)), mStyleManager, SLOT(retranslateUi()));

  // About Dialog & What's New dialog
  mAboutDialog = new KLFAboutDialog(this);
  mWhatsNewDialog = new KLFWhatsNewDialog(this);

  connect(mAboutDialog, SIGNAL(linkActivated(const QUrl&)), this, SLOT(helpLinkAction(const QUrl&)));
  connect(mWhatsNewDialog, SIGNAL(linkActivated(const QUrl&)), this, SLOT(helpLinkAction(const QUrl&)));

  mHelpLinkActions << HelpLinkAction("/whats_new", this, "showWhatsNew", false);
  mHelpLinkActions << HelpLinkAction("/about", this, "showAbout", false);

  // library will be loaded on window show event.
}

void KLFMainWin::retranslateUi(bool alsoBaseUi)
{
  if (alsoBaseUi)
    u->retranslateUi(this);

  // version information as tooltip on the welcome widget
  u->lblPromptMain->setToolTip(tr("KLatexFormula %1").arg(QString::fromUtf8(version)));

  refreshStylePopupMenus();
}


KLFMainWin::~KLFMainWin()
{
  //  printf("DEBUG: KLFMainWin::~KLFMainWin(): destroying...\n");

  saveLibraryState();
  saveSettings();
  saveStyles();

  if (mStyleMenu)
    delete mStyleMenu;
  if (mLatexSymbols)
    delete mLatexSymbols;
  if (mLibBrowser)
    delete mLibBrowser; // we aren't its parent
  if (mSettingsDialog)
    delete mSettingsDialog;

  if (mPreambleHighlighter)
    delete mPreambleHighlighter;
  if (mHighlighter)
    delete mHighlighter;
  if (mPreviewBuilderThread)
    delete mPreviewBuilderThread;

  delete u;
}


void KLFMainWin::refreshWindowSizes()
{
  bool curstate_shown = u->frmDetails->isVisible();
  u->frmDetails->show();
  _shrinkedsize = u->frmMain->sizeHint() + QSize(3, 3);
  _expandedsize.setWidth(u->frmMain->sizeHint().width() + u->frmDetails->sizeHint().width() + 3);
  _expandedsize.setHeight(u->frmMain->sizeHint().height() + 3);
  if (curstate_shown) {
    setFixedSize(_expandedsize);
    u->frmDetails->show();
  } else {
    setFixedSize(_shrinkedsize);
    u->frmDetails->hide();
  }
  updateGeometry();
}


bool KLFMainWin::loadNamedStyle(const QString& sty)
{
  // find style with name sty (if existant) and set it
  for (int kl = 0; kl < _styles.size(); ++kl) {
    if (_styles[kl].name == sty) {
      slotLoadStyle(kl);
      return true;
    }
  }
  return false;
}

void KLFMainWin::loadSettings()
{
  _settings.tempdir = klfconfig.BackendSettings.tempDir;
  _settings.latexexec = klfconfig.BackendSettings.execLatex;
  _settings.dvipsexec = klfconfig.BackendSettings.execDvips;
  _settings.gsexec = klfconfig.BackendSettings.execGs;
  _settings.epstopdfexec = klfconfig.BackendSettings.execEpstopdf;

  _settings.lborderoffset = klfconfig.BackendSettings.lborderoffset;
  _settings.tborderoffset = klfconfig.BackendSettings.tborderoffset;
  _settings.rborderoffset = klfconfig.BackendSettings.rborderoffset;
  _settings.bborderoffset = klfconfig.BackendSettings.bborderoffset;

  _settings_altered = false;
}

void KLFMainWin::saveSettings()
{
  if ( ! _settings_altered ) {
    // don't save altered settings
    klfconfig.BackendSettings.tempDir = _settings.tempdir;
    klfconfig.BackendSettings.execLatex = _settings.latexexec;
    klfconfig.BackendSettings.execDvips = _settings.dvipsexec;
    klfconfig.BackendSettings.execGs = _settings.gsexec;
    klfconfig.BackendSettings.execEpstopdf = _settings.epstopdfexec;
    klfconfig.BackendSettings.lborderoffset = _settings.lborderoffset;
    klfconfig.BackendSettings.tborderoffset = _settings.tborderoffset;
    klfconfig.BackendSettings.rborderoffset = _settings.rborderoffset;
    klfconfig.BackendSettings.bborderoffset = _settings.bborderoffset;
  }

  klfconfig.UI.userColorList = KLFColorChooser::colorList();
  klfconfig.UI.colorChooseWidgetRecent = KLFColorChooseWidget::recentColors();
  klfconfig.UI.colorChooseWidgetCustom = KLFColorChooseWidget::customColors();

  klfconfig.writeToConfig();

  mPreviewBuilderThread->settingsChanged(_settings, klfconfig.UI.labelOutputFixedSize.width(),
					 klfconfig.UI.labelOutputFixedSize.height());

  u->lblOutput->setLabelFixedSize(klfconfig.UI.labelOutputFixedSize);

  if (klfconfig.UI.enableRealTimePreview) {
    if ( ! mPreviewBuilderThread->isRunning() ) {
      delete mPreviewBuilderThread;
      mPreviewBuilderThread = new KLFPreviewBuilderThread(this, collectInput(), _settings,
							  klfconfig.UI.labelOutputFixedSize.width(),
							  klfconfig.UI.labelOutputFixedSize.height());
      mPreviewBuilderThread->start();
    }
  } else {
    if ( mPreviewBuilderThread->isRunning() ) {
      delete mPreviewBuilderThread;
      // do NOT leave a NULL mPreviewBuilderThread !
      mPreviewBuilderThread = new KLFPreviewBuilderThread(this, collectInput(), _settings,
							  klfconfig.UI.labelOutputFixedSize.width(),
							  klfconfig.UI.labelOutputFixedSize.height());
    }
  }
}

void KLFMainWin::refreshStylePopupMenus()
{
  if (!mStyleMenu)
    mStyleMenu = new QMenu(this);
  mStyleMenu->clear();

  //  mStyleMenu->insertTitle(tr("Available Styles"), 100000);
  QAction *a;
  for (int i = 0; i < _styles.size(); ++i) {
    a = mStyleMenu->addAction(_styles[i].name);
    a->setData(i);
    connect(a, SIGNAL(triggered()), this, SLOT(slotLoadStyleAct()));
  }
  mStyleMenu->addSeparator();
  mStyleMenu->addAction(QIcon(":/pix/pics/managestyles.png"), tr("Manage Styles"),
			 this, SLOT(slotStyleManager()), 0 /* accel */);
}

QString kdelocate(const char *fname)
{
  QString candidate;

  QStringList env = QProcess::systemEnvironment();
  QStringList kdehome = env.filter(QRegExp("^KDEHOME="));
  if (kdehome.size() == 0) {
    candidate = QString("~/.kde/share/apps/klatexformula/") + QString::fromLocal8Bit(fname);
  } else {
    QString kdehomeval = kdehome[0];
    kdehomeval.replace(QRegExp("^KDEHOME="), "");
    candidate = kdehomeval + "/share/apps/klatexformula/" + QString::fromLocal8Bit(fname);
  }
  if (QFile::exists(candidate)) {
    return candidate;
  }
  return QString::null;
}

void KLFMainWin::loadStyles()
{
  _styles = KLFStyleList(); // empty list to start with
  QString styfname = klfconfig.homeConfigDir + "/styles";
  if ( ! QFile::exists(styfname) ) {
    // try KDE version (for KLF 2.x)
    styfname = kdelocate("styles");
  }
  if ( QFile::exists(styfname) ) {
    QFile fsty(styfname);
    if ( ! fsty.open(QIODevice::ReadOnly) )
      QMessageBox::critical(this, tr("Error"), tr("Error: Unable to load your style list!"));
    else {
      QDataStream str(&fsty);
      QString s1;
      str >> s1;
      // check for KLATEXFORMULA_STYLE_LIST
      if (s1 != "KLATEXFORMULA_STYLE_LIST") {
	QMessageBox::critical(this, tr("Error"), tr("Error: Style file is incorrect or corrupt!\n"));
      } else {
	qint16 vmaj, vmin;
	str >> vmaj >> vmin;

	if (vmaj > version_maj || (vmaj == version_maj && vmin > version_min)) {
	  QMessageBox::warning(this, tr("Load Styles"),
			       tr("The style file found was created by a more recent version of KLatexFormula.\n"
				  "The process of style loading may fail.")
			      );
	}
	
	if (vmaj <= 2) {
	  str.setVersion(QDataStream::Qt_3_3);
	} else {
	  qint16 version;
	  str >> version;
	  str.setVersion(version);
	}

	str >> _styles;

      } // bad file format
    }
  } // file exists

  if (_styles.isEmpty()) {
    // if stylelist is empty, populate with default style
    KLFStyle s1(tr("Default"), qRgb(0, 0, 0), qRgba(255, 255, 255, 0), "\\[ ... \\]", "", 600);
    _styles.append(s1);
  }

  mStyleMenu = 0;
  refreshStylePopupMenus();

  u->btnLoadStyle->setMenu(mStyleMenu);
}

void KLFMainWin::saveStyles()
{
  klfconfig.ensureHomeConfigDir();
  QString s = klfconfig.homeConfigDir + "/styles";
  QFile f(s);
  if ( ! f.open(QIODevice::WriteOnly) ) {
    QMessageBox::critical(this, tr("Error"), tr("Error: Unable to write to styles file!\n%1").arg(s));
    return;
  }
  QDataStream stream(&f);
  stream.setVersion(QDataStream::Qt_3_3);
  // we write KLF-3.0-compatible stream
  stream << QString("KLATEXFORMULA_STYLE_LIST") << (qint16)3 << (qint16)0
	 << (qint16)stream.version() << _styles;
}

void KLFMainWin::loadLibrary()
{
  KLF_DEBUG_TIME_BLOCK(KLF_FUNC_NAME) ;
  // Locate a good library/history file.

  // the default library file
  QString localfname = klfconfig.homeConfigDir + "/library.klf.db";
  QString importfname;
  if ( ! QFile::exists(localfname) ) {
    // if unexistant, try to load:
    importfname = klfconfig.homeConfigDir + "/library"; // the 3.1 library file
    if ( ! QFile::exists(importfname) )
      importfname = kdelocate("library"); // or the KDE KLF 2.1 library file
    if ( ! QFile::exists(importfname) )
      importfname = kdelocate("history"); // or the KDE KLF 2.0 history file
    if ( ! QFile::exists(importfname) ) {
      // as last resort we load our default library bundled with KLatexFormula
      importfname = ":/data/defaultlibrary.klf.db";
    }
  }

  // importfname is non-empty only if the local .klf.db library file is inexistant.

  QUrl localliburl = QUrl::fromLocalFile(localfname);
  localliburl.setScheme("klf+sqlite");
  localliburl.addQueryItem("klfDefaultSubResource", "history");

  mHistoryLibResource = NULL;

  if (!QFile::exists(localfname)) {
    // create local library resource
    KLFLibEngineFactory *localLibFactory = KLFLibEngineFactory::findFactoryFor("klf+sqlite");
    KLFLibEngineFactory::Parameters param;
    param["Filename"] = localliburl.path();
    param["klfDefaultSubResource"] = QLatin1String("history");
    param["klfDefaultSubResourceTitle"] = tr("History", "[[default sub-resource title for history]]");
    mHistoryLibResource = localLibFactory->createResource("klf+sqlite", param, this);
    mHistoryLibResource->setTitle(tr("Local Library"));
    mHistoryLibResource
      -> createSubResource(QLatin1String("archive"),
			   tr("Archive", "[[default sub-resource title for archive sub-resource]]"));
    mHistoryLibResource->setSubResourceProperty(QLatin1String("history"),
						KLFLibResourceEngine::SubResPropViewType,
						QLatin1String("default+list"));
  }

  if (mHistoryLibResource == NULL) {
    mHistoryLibResource = KLFLibEngineFactory::openURL(localliburl, this);
  }
  if (mHistoryLibResource == NULL) {
    qWarning()<<"KLFMainWin::loadLibrary(): Can't open local history resource!\n\tURL="<<localliburl
	      <<"\n\tNot good! Expect crash!";
    return;
  }

  qDebug("KLFMainWin::loadLibrary(): opened history: resource-ptr=%p\n\tImportFile=%s",
	 (void*)mHistoryLibResource, qPrintable(importfname));

  if (!importfname.isEmpty()) {
    // needs an import

    // visual feedback for import
    KLFProgressDialog pdlg(QString(), this);
    connect(mHistoryLibResource, SIGNAL(operationStartReportingProgress(KLFProgressReporter *, const QString&)),
	    &pdlg, SLOT(startReportingProgress(KLFProgressReporter *)));
    pdlg.setAutoClose(false);
    pdlg.setAutoReset(false);

    // locate the import file and scheme
    QUrl importliburl = QUrl::fromLocalFile(importfname);
    if (importfname.endsWith(".klf.db")) {
      importliburl.setScheme("klf+sqlite");
    } else {
      importliburl.setScheme("klf+legacy");
    }
    importliburl.addQueryItem("klfReadOnly", "true");
    // import library from an older version library file.
    KLFLibEngineFactory *factory = KLFLibEngineFactory::findFactoryFor(importliburl.scheme());
    if (factory == NULL) {
      qWarning()<<"KLFMainWin::loadLibrary(): Can't find factory for URL="<<importliburl<<"!";
    } else {
      KLFLibResourceEngine *importres = factory->openResource(importliburl, this);
      // import all sub-resources
      QStringList subResList = importres->subResourceList();
      qDebug()<<"\tImporting sub-resources: "<<subResList;
      int j;
      for (j = 0; j < subResList.size(); ++j) {
	QString subres = subResList[j];
	pdlg.setDescriptiveText(tr("Importing Library from previous version of KLatexFormula ... %3 (%1/%2)")
				.arg(j+1).arg(subResList.size()).arg(subResList[j]));
	QList<KLFLibResourceEngine::KLFLibEntryWithId> allentries
	  = importres->allEntries(subres);
	qDebug("\tGot %d entries from sub-resource %s", allentries.size(), qPrintable(subres));
	int k;
	KLFLibEntryList insertentries;
	for (k = 0; k < allentries.size(); ++k) {
	  insertentries << allentries[k].entry;
	}
	if ( ! mHistoryLibResource->hasSubResource(subres) ) {
	  mHistoryLibResource->createSubResource(subres);
	}
	mHistoryLibResource->insertEntries(subres, insertentries);
      }
    }
  }

  // Open history sub-resource into library browser
  bool r;
  r = mLibBrowser->openResource(mHistoryLibResource, KLFLibBrowser::NoCloseRoleFlag,
  				QLatin1String("default+list"));
  if ( ! r ) {
    qWarning()<<"KLFMainWin::loadLibrary(): Can't open local history resource!\n\tURL="<<localliburl
  	      <<"\n\tExpect Crash!";
    return;
  }
  // Delayed open
  //  QMetaObject::invokeMethod(this, "slotOpenHistoryLibraryResource", Qt::QueuedConnection);

  // open all other sub-resources present in our library
  QStringList subresources = mHistoryLibResource->subResourceList();
  int k;
  for (k = 0; k < subresources.size(); ++k) {
    if (subresources[k] == QLatin1String("history"))
      continue;
    QUrl url = localliburl;
    url.removeAllQueryItems("klfDefaultSubResource");
    url.addQueryItem("klfDefaultSubResource", subresources[k]);
    mLibBrowser->openResource(url, KLFLibBrowser::NoCloseRoleFlag);
    //    QMetaObject::invokeMethod(mLibBrowser, "openResource",
    //			      Qt::QueuedConnection, Q_ARG(QUrl, url),
    //			      Q_ARG(uint, KLFLibBrowser::NoCloseRoleFlag),
    //			      Q_ARG(QString, QString()));
  }

  loadLibrarySavedState();
}

void KLFMainWin::loadLibrarySavedState()
{
  QString fname = klfconfig.homeConfigDir + "/libbrowserstate.xml";
  if ( ! QFile::exists(fname) ) {
    qDebug("%s: No saved library browser state found (%s).", KLF_FUNC_NAME, qPrintable(fname));
    return;
  }

  QFile f(fname);
  if ( ! f.open(QIODevice::ReadOnly) ) {
    qWarning()<<"Can't open file "<<fname<<" for loading library browser state";
    return;
  }
  QDomDocument doc("klflibbrowserstate");
  doc.setContent(&f);
  f.close();

  QDomNode rootNode = doc.documentElement();
  if (rootNode.nodeName() != "klflibbrowserstate") {
    qWarning()<<"Invalid root node in file "<<fname<<" !";
    return;
  }
  QDomNode n;
  QVariantMap vstatemap;
  for (n = rootNode.firstChild(); ! n.isNull(); n = n.nextSibling()) {
    QDomElement e = n.toElement();
    if ( e.isNull() || n.nodeType() != QDomNode::ElementNode )
      continue;
    if ( e.nodeName() == "statemap" ) {
      vstatemap = klfLoadVariantMapFromXML(e);
      continue;
    }

    qWarning("Ignoring unrecognized node `%s' in file %s.", qPrintable(e.nodeName()), qPrintable(fname));
  }

  mLibBrowser->loadGuiState(vstatemap);
}

void KLFMainWin::saveLibraryState()
{
  QString fname = klfconfig.homeConfigDir + "/libbrowserstate.xml";

  QVariantMap statemap = mLibBrowser->saveGuiState();

  QFile f(fname);
  if ( ! f.open(QIODevice::WriteOnly) ) {
    qWarning()<<"Can't open file "<<fname<<" for saving library browser state";
    return;
  }

  QDomDocument doc("klflibbrowserstate");
  QDomElement rootNode = doc.createElement("klflibbrowserstate");
  QDomElement statemapNode = doc.createElement("statemap");

  klfSaveVariantMapToXML(statemap, statemapNode);

  rootNode.appendChild(statemapNode);
  doc.appendChild(rootNode);

  f.write(doc.toByteArray());
}

// private
void KLFMainWin::slotOpenHistoryLibraryResource()
{
  KLF_DEBUG_BLOCK(KLF_FUNC_NAME) ;

  bool r;
  r = mLibBrowser->openResource(mHistoryLibResource, KLFLibBrowser::NoCloseRoleFlag,
  				QLatin1String("default+list"));
  if ( ! r )
    qWarning()<<KLF_FUNC_NAME<<": Can't open local history resource!";

}



void KLFMainWin::restoreFromLibrary(const KLFLibEntry& entry, uint restoreFlags)
{
  if (restoreFlags & KLFLib::RestoreLatex) {
    slotSetLatex(entry.latex()); // to preserve text edit undo history...
  }
  if (restoreFlags & KLFLib::RestoreStyle) {
    KLFStyle style = entry.style();
    slotLoadStyle(style);
  }

  u->lblOutput->display(entry.preview(), entry.preview(), false);
  u->frmOutput->setEnabled(false);
  activateWindow();
  raise();
}

void KLFMainWin::slotLibraryButtonRefreshState(bool on)
{
  u->btnLibrary->setChecked(on);
  //   if (...............kapp->sessionSaving()) {
  //     // if we're saving the session, remember that history browser is on
  //   } else {
  //     _libraryBrowserIsShown = on;
  //   }
}


void KLFMainWin::insertSymbol(const KLFLatexSymbol& s)
{
  QTextCursor c1(u->txtLatex->document());
  c1.beginEditBlock();
  c1.setPosition(u->txtLatex->textCursor().position());
  c1.insertText(s.symbol+"\n");
  c1.deletePreviousChar(); // small hack for forcing a separate undo command when two symbols are inserted
  c1.endEditBlock();
  // see if we need to insert the xtrapreamble
  QStringList cmds = s.preamble;
  int k;
  QString preambletext = u->txtPreamble->toPlainText();
  QString addtext;
  QTextCursor c = u->txtPreamble->textCursor();
  //  qDebug("Begin preamble edit: preamble text is %s", qPrintable(u->txtPreamble->toPlainText()));
  c.beginEditBlock();
  c.movePosition(QTextCursor::End);
  for (k = 0; k < cmds.size(); ++k) {
    if (preambletext.indexOf(cmds[k]) == -1) {
      addtext = "";
      if (preambletext.length() > 0 &&
	  preambletext[preambletext.length()-1] != '\n')
	addtext = "\n";
      addtext += cmds[k];
      c.insertText(addtext);
      preambletext += addtext;
    }
  }
  c.endEditBlock();

  //  qDebug("End preamble edit; premable text is %s", qPrintable(u->txtPreamble->toPlainText()));

  activateWindow();
  raise();
  u->txtLatex->setFocus();
}

void KLFMainWin::slotSymbolsButtonRefreshState(bool on)
{
  u->btnSymbols->setChecked(on);
}

void KLFMainWin::showAbout()
{
  mAboutDialog->show();
}

void KLFMainWin::showWhatsNew()
{
  // center the widget on the desktop
  QSize desktopSize = QApplication::desktop()->screenGeometry(this).size();
  QSize wS = mWhatsNewDialog->sizeHint();
  mWhatsNewDialog->move(desktopSize.width()/2 - wS.width()/2,
			desktopSize.height()/2 - wS.height()/2);
  mWhatsNewDialog->show();
}

void KLFMainWin::helpLinkAction(const QUrl& link)
{
  qDebug()<<"Link is "<<link<<"; scheme="<<link.scheme()<<"; path="<<link.path()<<"; queryItems="
	  <<link.queryItems();

  if (link.scheme() == "http") {
    // web link
    QDesktopServices::openUrl(link);
    return;
  }
  if (link.scheme() == "klfaction") {
    // find the help link action(s) that is (are) associated with this link
    bool calledOne = false;
    int k;
    for (k = 0; k < mHelpLinkActions.size(); ++k) {
      if (mHelpLinkActions[k].path == link.path()) {
	// got one
	if (mHelpLinkActions[k].wantParam)
	  QMetaObject::invokeMethod(mHelpLinkActions[k].reciever, mHelpLinkActions[k].memberFunc.constData(),
				    Q_ARG(QUrl, link));
	else
	  QMetaObject::invokeMethod(mHelpLinkActions[k].reciever, mHelpLinkActions[k].memberFunc.constData());

	calledOne = true;
      }
    }
    if (!calledOne) {
      qWarning()<<KLF_FUNC_NAME<<": no action found for link="<<link;
    }
    return;
  }
  qWarning()<<KLF_FUNC_NAME<<"("<<link<<"): Unrecognized link!";
}

void KLFMainWin::addWhatsNewText(const QString& htmlSnipplet)
{
  mWhatsNewDialog->addWhatsNewText(htmlSnipplet);
}


void KLFMainWin::registerHelpLinkAction(const QString& path, QObject *object, const char * member,
					bool wantUrlParam)
{
  mHelpLinkActions << HelpLinkAction(path, object, member, wantUrlParam);
}




void KLFMainWin::setWidgetStyle(const QString& qtstyle)
{
  //  qDebug("setWidgetStyle(\"%s\")", qPrintable(qtstyle));
  if (_widgetstyle == qtstyle) {
    //    qDebug("This style is already applied.");
    return;
  }
  if (qtstyle.isNull()) {
    // setting a null style resets internal widgetstyle name...
    _widgetstyle = QString::null;
    return;
  }
  QStringList stylelist = QStyleFactory::keys();
  if (stylelist.indexOf(qtstyle) == -1) {
    qWarning("Bad Style: %s. List of possible styles are:", qPrintable(qtstyle));
    int k;
    for (k = 0; k < stylelist.size(); ++k)
      qWarning("\t%s", qPrintable(stylelist[k]));
    return;
  }
  //  qDebug("Setting the style %s. are we visible?=%d", qPrintable(qtstyle), (int)QWidget::isVisible());
  QStyle *s = QStyleFactory::create(qtstyle);
  //  qDebug("Got style ptr=%p", (void*)s);
  if ( ! s ) {
    qWarning("Can't instantiate style %s!", qPrintable(qtstyle));
    return;
  }
  _widgetstyle = qtstyle;
  qApp->setStyle( s );
}

void KLFMainWin::setQuitOnClose(bool quitOnClose)
{
  _ignore_close_event = ! quitOnClose;
}


void KLFMainWin::quit()
{
  hide();

  if (mLibBrowser)
    mLibBrowser->hide();
  if (mLatexSymbols)
    mLatexSymbols->hide();
  if (mStyleManager)
    mStyleManager->hide();
  if (mSettingsDialog)
    mSettingsDialog->hide();
  qApp->quit();
}

bool KLFMainWin::event(QEvent *e)
{
  qDebug("Event: e->type() == %d", (int)e->type());
  
  return QWidget::event(e);
}


bool KLFMainWin::eventFilter(QObject *obj, QEvent *e)
{
  //  qDebug("eventFilter, e->type()==%d", (int)e->type());
  if (obj == u->txtLatex) {
    if (e->type() == QEvent::KeyPress) {
      QKeyEvent *ke = (QKeyEvent*) e;
      if (ke->key() == Qt::Key_Return && (QApplication::keyboardModifiers() == Qt::ShiftModifier ||
					  QApplication::keyboardModifiers() == Qt::ControlModifier)) {
	slotEvaluate();
	return true;
      }
    }
  }

  // ----
  if ( e->type() == QEvent::Hide || e->type() == QEvent::Show ) {
    // shown/hidden windows: save/restore geometry
    QWidget *sh_widgets[] = { mLibBrowser, mLatexSymbols, mSettingsDialog, mStyleManager, NULL };
    int sh_enum[] = { LibBrowser, LatexSymbols, SettingsDialog, StyleManager, -1 };
    int k;
    for (k = 0; sh_widgets[k] != NULL && sh_enum[k] >= 0; ++k) {
      if (obj == sh_widgets[k] && e->type() == QEvent::Hide && !((QHideEvent*)e)->spontaneous()) {
	if (_lastwindowshownstatus & sh_enum[k])
	  _lastwindowgeometries[sh_enum[k]] = klf_get_window_geometry(sh_widgets[k]);
	_lastwindowshownstatus &= ~sh_enum[k];
      }
      // -
      if (obj == sh_widgets[k] && e->type() == QEvent::Show && !((QShowEvent*)e)->spontaneous()) {
	_lastwindowshownstatus |= sh_enum[k];
	klf_set_window_geometry(sh_widgets[k], _lastwindowgeometries[sh_enum[k]]);
      }
    }
  }
  // ----
  if ( obj == mLibBrowser && (e->type() == QEvent::Hide || e->type() == QEvent::Show) ) {
    slotLibraryButtonRefreshState(mLibBrowser->isVisible());
  }
  return QWidget::eventFilter(obj, e);
}


uint KLFMainWin::currentWindowShownStatus()
{
  uint flags = 0;

  flags |= isVisible();

  if (mLibBrowser && mLibBrowser->isVisible())
    flags |= LibBrowser;
  if (mLatexSymbols && mLatexSymbols->isVisible())
    flags |= LatexSymbols;
  if (mStyleManager && mStyleManager->isVisible())
    flags |= StyleManager;
  if (mSettingsDialog && mSettingsDialog->isVisible())
    flags |= SettingsDialog;

  return flags;
}

void KLFMainWin::setWindowShownStatus(uint fl, bool metoo)
{
  if (metoo)
    setVisible(fl);

  if (mLibBrowser)
    mLibBrowser->setVisible(fl & LibBrowser);
  if (mLatexSymbols)
    mLatexSymbols->setVisible(fl & LatexSymbols);
  if (mStyleManager)
    mStyleManager->setVisible(fl & StyleManager);
  if (mSettingsDialog)
    mSettingsDialog->setVisible(fl & SettingsDialog);
}


void KLFMainWin::hideEvent(QHideEvent *e)
{
  if ( ! e->spontaneous() ) {
    _savedwindowshownstatus = currentWindowShownStatus();
    _lastwindowgeometries[MainWin] = klf_get_window_geometry(this);
    setWindowShownStatus(0); // hide all windows
  }

  QWidget::hideEvent(e);
}

void KLFMainWin::showEvent(QShowEvent *e)
{
  if ( _firstshow ) {
    _firstshow = false;
    // if it's the first time we're run, 
    // show the what's new if needed.
    if ( klfconfig.General.thisVersionFirstRun ) {
      QMetaObject::invokeMethod(this, "showWhatsNew", Qt::QueuedConnection);
    }
  }
  if ( ! _loadedlibrary ) {
    _loadedlibrary = true;

    // load the library browser's saved state
    QMetaObject::invokeMethod(this, "loadLibrary", Qt::QueuedConnection);
  }

  if ( ! e->spontaneous() ) {
    // restore shown windows ...
    klf_set_window_geometry(this, _lastwindowgeometries[MainWin]);
    setWindowShownStatus(_savedwindowshownstatus);
  }

  QWidget::showEvent(e);
}



void KLFMainWin::alterSetting(altersetting_which which, int ivalue)
{
  _settings_altered = true;
  switch (which) {
  case altersetting_LBorderOffset:
    _settings.lborderoffset = ivalue; break;
  case altersetting_TBorderOffset:
    _settings.tborderoffset = ivalue; break;
  case altersetting_RBorderOffset:
    _settings.rborderoffset = ivalue; break;
  case altersetting_BBorderOffset:
    _settings.bborderoffset = ivalue; break;
  default:
    break;
  }
}
void KLFMainWin::alterSetting(altersetting_which which, QString svalue)
{
  _settings_altered = true;
  switch (which) {
  case altersetting_TempDir:
    _settings.tempdir = svalue; break;
  case altersetting_Latex:
    _settings.latexexec = svalue; break;
  case altersetting_Dvips:
    _settings.dvipsexec = svalue; break;
  case altersetting_Gs:
    _settings.gsexec = svalue; break;
  case altersetting_Epstopdf:
    _settings.epstopdfexec = svalue; break;
  default:
    break;
  }
}


void KLFMainWin::applySettings(const KLFBackend::klfSettings& s)
{
  _settings = s;
  _settings_altered = false;
}

void KLFMainWin::displayError(const QString& error)
{
  QMessageBox::critical(this, tr("Error"), error);
}


void KLFMainWin::updatePreviewBuilderThreadInput()
{
  bool reallyinputchanged = mPreviewBuilderThread->inputChanged(collectInput());
  if (reallyinputchanged) {
    _evaloutput_uptodate = false;
  }
}

void KLFMainWin::setTxtLatexFont(const QFont& f)
{
  u->txtLatex->setFont(f);
}
void KLFMainWin::setTxtPreambleFont(const QFont& f)
{
  u->txtPreamble->setFont(f);
}

void KLFMainWin::showRealTimePreview(const QImage& preview, bool latexerror)
{
  if (_evaloutput_uptodate)
    return;

  if (latexerror)
    u->lblOutput->displayError(/*labelenabled:*/false);
  else
    u->lblOutput->display(preview, preview, false);

}

KLFBackend::klfInput KLFMainWin::collectInput()
{
  // KLFBackend input
  KLFBackend::klfInput input;

  input.latex = u->txtLatex->toPlainText();
  if (u->chkMathMode->isChecked()) {
    input.mathmode = u->cbxMathMode->currentText();
    if (u->cbxMathMode->findText(input.mathmode) == -1) {
      u->cbxMathMode->addItem(input.mathmode);
    }
  } else {
    input.mathmode = "...";
  }
  input.preamble = u->txtPreamble->toPlainText();
  input.fg_color = u->colFg->color().rgb();
  if (u->chkBgTransparent->isChecked() == false)
    input.bg_color = u->colBg->color().rgb();
  else
    input.bg_color = qRgba(255, 255, 255, 0);

  input.dpi = u->spnDPI->value();

  return input;
}

void KLFMainWin::slotEvaluate()
{
  // KLFBackend input
  KLFBackend::klfInput input;

  u->btnEvaluate->setEnabled(false); // don't allow user to click us while we're not done, and
  //				     additionally this gives visual feedback to the user

  input = collectInput();

  // and GO !
  _output = KLFBackend::getLatexFormula(input, _settings);
  _lastrun_input = input;

  if (_output.status < 0) {
    QString comment = "";
    if (_output.status == -4) {
      comment = "\n"+tr("Are you sure you configured your system paths correctly in the settings dialog ?");
    }
    QMessageBox::critical(this, tr("Error"), _output.errorstr+comment);
    u->lblOutput->displayClear();
    u->frmOutput->setEnabled(false);
  }
  if (_output.status > 0) {
    KLFProgErr::showError(this, _output.errorstr);
    u->frmOutput->setEnabled(false);
  }
  if (_output.status == 0) {
    // ALL OK
    _evaloutput_uptodate = true;

    QPixmap sc;
    // scale to view size (klfconfig.UI.labelOutputFixedSize)
    QSize fsize = klfconfig.UI.labelOutputFixedSize;
    if (_output.result.width() > fsize.width() || _output.result.height() > fsize.height())
      sc = QPixmap::fromImage(_output.result.scaled(fsize, Qt::KeepAspectRatio, Qt::SmoothTransformation));
    else
      sc = QPixmap::fromImage(_output.result);

    QSize goodsize = _output.result.size();
    QImage tooltipimg = _output.result;
    if ( klfconfig.UI.previewTooltipMaxSize != QSize(0, 0) && // QSize(0,0) meaning no resize
	 ( tooltipimg.width() > klfconfig.UI.previewTooltipMaxSize.width() ||
	   tooltipimg.height() > klfconfig.UI.previewTooltipMaxSize.height() ) ) {
      tooltipimg =
	_output.result.scaled(klfconfig.UI.previewTooltipMaxSize, Qt::KeepAspectRatio,
			      Qt::SmoothTransformation);
    }
    u->lblOutput->display(sc.toImage(), tooltipimg, true);
    u->frmOutput->setEnabled(true);

    KLFLibEntry newentry = KLFLibEntry(input.latex, QDateTime::currentDateTime(), sc.toImage(),
				       currentStyle());
    int eid = mHistoryLibResource->insertEntry(newentry);
    bool result = (eid >= 0);
    if ( ! result && mHistoryLibResource->locked() ) {
      int r = QMessageBox::warning(this, tr("Warning"),
				   tr("Can't add the item to history library because the history "
				      "resource is locked. Do you want to unlock it?"),
				   QMessageBox::Yes|QMessageBox::No, QMessageBox::Yes);
      if (r == QMessageBox::Yes) {
	mHistoryLibResource->setLocked(false);
	result = mHistoryLibResource->insertEntry(newentry); // retry inserting entry
      }
    }
    uint fl = mHistoryLibResource->supportedFeatureFlags();
    if ( ! result && (fl & KLFLibResourceEngine::FeatureSubResources) &&
	 (fl & KLFLibResourceEngine::FeatureSubResourceProps) &&
	 mHistoryLibResource->subResourceProperty(mHistoryLibResource->defaultSubResource(),
						  KLFLibResourceEngine::SubResPropLocked).toBool() ) {
      int r = QMessageBox::warning(this, tr("Warning"),
				   tr("Can't add the item to history library because the history "
				      "sub-resource is locked. Do you want to unlock it?"),
				   QMessageBox::Yes|QMessageBox::No, QMessageBox::Yes);
      if (r == QMessageBox::Yes) {
	mHistoryLibResource->setSubResourceProperty(mHistoryLibResource->defaultSubResource(),
						    KLFLibResourceEngine::SubResPropLocked, false);
	result = mHistoryLibResource->insertEntry(newentry); // retry inserting entry
      }
    }
    if ( ! result && mHistoryLibResource->isReadOnly() ) {
      qWarning("KLFMainWin::slotEvaluate: History resource is READ-ONLY !! Should NOT!");
      QMessageBox::critical(this, tr("Error"),
			    tr("Can't add the item to history library because the history "
			       "resource is opened in read-only mode. This should not happen! "
			       "You will need to manually copy and paste your Latex code somewhere "
			       "else to save it."),
			    QMessageBox::Ok, QMessageBox::Ok);
    }
    if ( ! result ) {
      qWarning("KLFMainWin::slotEvaluate: History resource couldn't be written!");
      QMessageBox::critical(this, tr("Error"),
			    tr("An error occurred when trying to write the new entry into the "
			       "history resource!"
			       "You will need to manually copy and paste your Latex code somewhere "
			       "else to save it."),
			    QMessageBox::Ok, QMessageBox::Ok);
    }

  }

  u->btnEvaluate->setEnabled(true); // re-enable our button
}

void KLFMainWin::slotClear()
{
  u->txtLatex->setPlainText("");
  u->txtLatex->setFocus();
}

void KLFMainWin::slotLibrary(bool showlib)
{
  mLibBrowser->setShown(showlib);
  slotLibraryButtonRefreshState(showlib);
}

void KLFMainWin::slotSymbols(bool showsymbs)
{
  mLatexSymbols->setShown(showsymbs);
  slotSymbolsButtonRefreshState(showsymbs);
}

void KLFMainWin::slotExpandOrShrink()
{
  if (u->frmDetails->isVisible()) {
    u->frmDetails->hide();
    setFixedSize(_shrinkedsize);
    //    adjustSize();
    u->btnExpand->setIcon(QIcon(":/pics/switchexpanded.png"));
  } else {
    setFixedSize(_expandedsize);
    u->frmDetails->show();
    //    adjustSize();
    u->btnExpand->setIcon(QIcon(":/pics/switchshrinked.png"));
  }
}

void KLFMainWin::slotSetLatex(const QString& latex)
{
  QTextCursor cur = u->txtLatex->textCursor();
  cur.beginEditBlock();
  cur.select(QTextCursor::Document);
  cur.removeSelectedText();
  cur.insertText(latex);
  cur.endEditBlock();
}

void KLFMainWin::slotSetMathMode(const QString& mathmode)
{
  u->chkMathMode->setChecked(mathmode.simplified() != "...");
  if (mathmode.simplified() != "...")
    u->cbxMathMode->setEditText(mathmode);
}

void KLFMainWin::slotSetPreamble(const QString& preamble)
{
  QTextCursor cur = u->txtPreamble->textCursor();
  cur.beginEditBlock();
  cur.select(QTextCursor::Document);
  cur.removeSelectedText();
  cur.insertText(preamble);
  cur.endEditBlock();
}

void KLFMainWin::slotSetDPI(int DPI)
{
  u->spnDPI->setValue(DPI);
}

void KLFMainWin::slotSetFgColor(const QColor& fg)
{
  u->colFg->setColor(fg);
}
void KLFMainWin::slotSetFgColor(const QString& s)
{
  QColor fgcolor;
  fgcolor.setNamedColor(s);
  slotSetFgColor(fgcolor);
}
void KLFMainWin::slotSetBgColor(const QColor& bg)
{
  u->colBg->setColor(bg);
}
void KLFMainWin::slotSetBgColor(const QString& s)
{
  QColor bgcolor;
  if (s == "-")
    bgcolor.setRgb(255, 255, 255, 0); // white transparent
  else
    bgcolor.setNamedColor(s);
  slotSetBgColor(bgcolor);
}

void KLFMainWin::slotEvaluateAndSave(const QString& output, const QString& format)
{
  if ( u->txtLatex->toPlainText().isEmpty() )
    return;

  slotEvaluate();

  if ( ! output.isEmpty() ) {
    if ( _output.result.isNull() ) {
      QMessageBox::critical(this, tr("Error"), tr("There is no image to save."));
    } else {
      KLFBackend::saveOutputToFile(_output, output, format.trimmed().toUpper());
    }
  }

}

bool KLFMainWin::importCmdlKLFFiles(const QStringList& files, bool showLibrary)
{
  int k;
  bool imported = false;
  for (k = 0; k < files.size(); ++k) {
    bool ok = importCmdlKLFFile(files[k], false);
    imported = imported || ok;
  }
  if (showLibrary && imported)
    slotLibrary(true);
  return imported;
}

bool KLFMainWin::importCmdlKLFFile(const QString& fname, bool showLibrary)
{
  QUrl url = QUrl::fromLocalFile(fname);
  url.setScheme(KLFLibBasicWidgetFactory::guessLocalFileScheme(fname));
  QStringList subreslist = KLFLibEngineFactory::listSubResources(url);
  if ( subreslist.isEmpty() ) {
    // error reading sub-resources, or sub-resources not supported
    return mLibBrowser->openResource(url);
  }
  bool loaded = false;
  int k;
  for (k = 0; k < subreslist.size(); ++k) {
    QUrl url2 = url;
    url2.addQueryItem("klfDefaultSubResource", subreslist[k]);
    loaded = loaded || mLibBrowser->openResource(url2);
  }
  return loaded;
}


void KLFMainWin::setApplicationLocale(const QString& locale)
{
  klf_reload_translations(qApp, locale);

  emit applicationLocaleChanged(locale);
}


QMimeData * KLFMainWin::resultToMimeData()
{
  if ( _output.result.isNull() )
    return NULL;

  // klf export profile to use (for now, use first = default)
  /** \todo .......TODO: choose export profile ! */
  KLFMimeExportProfile p = KLFMimeExportProfile::exportProfileList() [0];
  QStringList mimetypes = p.mimeTypes();

  QMimeData *mimedata = new QMimeData;
  int k;
  for (k = 0; k < mimetypes.size(); ++k) {
    QString mimetype = mimetypes[k];
    QByteArray data = KLFMimeExporter::mimeExporterLookup(mimetype)->data(mimetype, _output);
    mimedata->setData(mimetype, data);
  }

  return mimedata;
}


void KLFMainWin::slotDrag()
{
  if ( _output.result.isNull() )
    return;

  QDrag *drag = new QDrag(this);
  QMimeData *mime = resultToMimeData();
  drag->setMimeData(mime);
  QImage img;
  img = _output.result.scaled(QSize(200, 100), Qt::KeepAspectRatio, Qt::SmoothTransformation);
  QPixmap p = QPixmap::fromImage(img);
  drag->setPixmap(p);
  drag->setDragCursor(p, Qt::MoveAction);
  drag->setHotSpot(QPoint(p.width()/2, p.height()));
  drag->exec();
}
void KLFMainWin::slotCopy()
{
#ifdef Q_WS_WIN
  extern void klfWinClipboardCopy(HWND h, const QStringList& wintypes,
				  const QList<QByteArray>& datalist);

  // klf export profile to use (for now, use first = default)
  /** \todo ............................... Choose export profile ...? */
  KLFMimeExportProfile p = KLFMimeExportProfile::exportProfileList() [0];

  QStringList mimetypes = p.mimeTypes();
  QStringList respectivewintypes = p.respectiveWinTypes();
  QStringList wintypes;
  QList<QByteArray> datalist;

  int k;
  for (k = 0; k < mimetypes.size(); ++k) {
    QString mimetype = mimetypes[k];
    QString wintype = respectivewintypes[k];
    if (wintype.isEmpty())
      wintype = mimetype;
    QByteArray data = KLFMimeExporter::mimeExporterLookup(mimetype)->data(mimetype, _output);
    wintypes << wintype;
    datalist << data;
  }

  klfWinClipboardCopy(winId(), wintypes, datalist);

#else
  QMimeData *mimedata = resultToMimeData();
  QApplication::clipboard()->setMimeData(mimedata, QClipboard::Clipboard);
#endif
}

void KLFMainWin::slotSave(const QString& suggestfname)
{
  // application-long persistent selectedfilter
  static QString selectedfilter;

  QStringList formatlist, filterformatlist;
  QMap<QString,QString> formatsByFilterName;
  QList<QByteArray> formats = QImageWriter::supportedImageFormats();

  for (QList<QByteArray>::iterator it = formats.begin(); it != formats.end(); ++it) {
    QString f = (*it).toLower();
    if (f == "jpg" || f == "jpeg" || f == "png" || f == "pdf" || f == "eps")
      continue;
    QString s = tr("%1 Image (*.%2)").arg(f).arg(f);
    filterformatlist << s;
    formatlist.push_back(f);
    formatsByFilterName[s] = f;
  }

  QString s;
  s = tr("EPS PostScript (*.eps)");
  filterformatlist.push_front(s);
  formatlist.push_front("eps");
  formatsByFilterName[s] = "eps";
  s = tr("PDF Portable Document Format (*.pdf)");
  filterformatlist.push_front(s);
  formatlist.push_front("pdf");
  formatsByFilterName[s] = "pdf";
  s = tr("Standard JPEG Image (*.jpg *.jpeg)");
  filterformatlist.push_front(s);
  formatlist.push_front("jpeg");
  formatlist.push_front("jpg");
  formatsByFilterName[s] = "jpeg";
  selectedfilter = s = tr("Standard PNG Image (*.png)");
  filterformatlist.push_front(s);
  formatlist.push_front("png");
  formatsByFilterName[s] = "png";

  QString filterformat = filterformatlist.join(";;");
  QString fname, format;
  QString suggestion = suggestfname;
  if (suggestion.isEmpty())
    suggestion = klfconfig.UI.lastSaveDir;
  fname = QFileDialog::getSaveFileName(this, tr("Save Image Formula"), suggestion, filterformat, &selectedfilter);

  if (fname.isEmpty())
    return;

  QFileInfo fi(fname);
  klfconfig.UI.lastSaveDir = fi.absolutePath();
  if ( fi.suffix().length() == 0 ) {
    // get format and suffix from selected filter
    if ( ! formatsByFilterName.contains(selectedfilter) ) {
      qWarning("ERROR: Unknown format filter selected: `%s'! Assuming PNG!\n", qPrintable(selectedfilter));
      format = "png";
    } else {
      format = formatsByFilterName[selectedfilter];
    }
    fname += "."+format;
    // !! : fname has changed, potentially the file could exist, user would not have been warned.
    if (QFile::exists(fname)) {
      int r = QMessageBox::warning(this, tr("File Exists"),
				   tr("The file <b>%1</b> already exists.\nOverwrite?").arg(fname),
				   QMessageBox::Yes|QMessageBox::No|QMessageBox::Cancel, QMessageBox::No);
      if (r == QMessageBox::Yes) {
	// will continue in this function.
      } else if (r == QMessageBox::No) {
	// re-prompt for file name & save (by recursion), and return
	slotSave(fname);
	return;
      } else {
	// cancel
	return;
      }
    }
  }
  fi.setFile(fname);
  int index = formatlist.indexOf(fi.suffix());
  if ( index < 0 ) {
    // select PNG by default if suffix is not recognized
    QMessageBox msgbox(this);
    msgbox.setIcon(QMessageBox::Warning);
    msgbox.setWindowTitle(tr("Extension not recognized"));
    msgbox.setText(tr("Extension <b>%1</b> not recognized.").arg(fi.suffix()));
    msgbox.setInformativeText(tr("Press \"Change\" to change the file name, or \"Use PNG\" to save as PNG."));
    QPushButton *png = new QPushButton(tr("Use PNG"), &msgbox);
    msgbox.addButton(png, QMessageBox::AcceptRole);
    QPushButton *chg  = new QPushButton(tr("Change ..."), &msgbox);
    msgbox.addButton(chg, QMessageBox::ActionRole);
    QPushButton *cancel = new QPushButton(tr("Cancel"), &msgbox);
    msgbox.addButton(cancel, QMessageBox::RejectRole);
    msgbox.setDefaultButton(chg);
    msgbox.setEscapeButton(cancel);
    msgbox.exec();
    if (msgbox.clickedButton() == png) {
      format = "png";
    } else if (msgbox.clickedButton() == cancel) {
      return;
    } else {
      // re-prompt for file name & save (by recursion), and return
      slotSave(fname);
      return;
    }
  } else {
    format = formatlist[index];
  }

  // The Qt dialog, or us, already asked user to confirm overwriting existing files

  QByteArray *dataptr = 0;
  if (format == "ps" || format == "eps") {
    dataptr = &_output.epsdata;
  } else if (format == "pdf") {
    dataptr = &_output.pdfdata;
  } else if (format == "png") {
    // we want to add meta-info in PNG image...
    //dataptr = &_output.pngdata;
    dataptr = 0;
  } else {
    dataptr = 0;
  }
  
  if (dataptr) {
    if (dataptr->size() == 0) {
      QMessageBox::critical(this, tr("Error"), tr("Sorry, format `%1' is not available.").arg(format));
      slotSave(); // recurse and prompt user.
      return;
    }
    QFile fsav(fname);
    if ( ! fsav.open(QIODevice::WriteOnly) ) {
      QMessageBox::critical(this, tr("Error"), tr("Error: Can't write to file %1!").arg(fname));
      return;
    }
    fsav.write(*dataptr);
  } else {
    // add text information for latex formula, style, etc.
    // with QImageWriter
    QImageWriter writer(fname, format.toUpper().toLatin1());
    writer.setQuality(90);
    writer.setText("Application", tr("Created with KLatexFormula version %1").arg(QString::fromAscii(version)));
    writer.setText("AppVersion", QString::fromLatin1("KLF ")+version);
    writer.setText("InputLatex", _lastrun_input.latex);
    writer.setText("InputMathMode", _lastrun_input.mathmode);
    writer.setText("InputPreamble", _lastrun_input.preamble);
    writer.setText("InputFgColor", QString("rgb(%1, %2, %3)").arg(qRed(_lastrun_input.fg_color))
		   .arg(qGreen(_lastrun_input.fg_color)).arg(qBlue(_lastrun_input.fg_color)));
    writer.setText("InputBgColor", QString("rgba(%1, %2, %3, %4)").arg(qRed(_lastrun_input.bg_color))
		   .arg(qGreen(_lastrun_input.bg_color)).arg(qBlue(_lastrun_input.bg_color))
		   .arg(qAlpha(_lastrun_input.bg_color)));
    writer.setText("InputDPI", QString::number(_lastrun_input.dpi));

    //    _output.result.save(fname, format.toUpper().toLocal8Bit().constData());

    bool res = writer.write(_output.result);
    if ( ! res ) {
      QMessageBox::critical(this, tr("Error"), writer.errorString());
      return;
    }
  }

}


void KLFMainWin::slotActivateEditor()
{
  raise();
  activateWindow();
  u->txtLatex->setFocus();
}

void KLFMainWin::slotShowBigPreview()
{
  if ( ! u->btnShowBigPreview->isEnabled() )
    return;

  QString text =
    tr("<p>%1</p><p align=\"right\" style=\"font-size: %2pt; font-style: italic;\">"
       "This preview can be opened with the <strong>F2</strong> key. Hit "
       "<strong>Esc</strong> to close.</p>")
    .arg(u->lblOutput->bigPreviewText())
    .arg(QFontInfo(qApp->font()).pointSize()-1);
  QWhatsThis::showText(u->btnEvaluate->pos(), text, u->lblOutput);
}



void KLFMainWin::slotPresetDPISender()
{
  QAction *a = qobject_cast<QAction*>(sender());
  if (a) {
    slotSetDPI(a->data().toInt());
  }
}


KLFStyle KLFMainWin::currentStyle() const
{
  KLFStyle sty;

  sty.name = QString::null;
  sty.fg_color = u->colFg->color().rgb();
  QColor bgc = u->colBg->color();
  sty.bg_color = qRgba(bgc.red(), bgc.green(), bgc.blue(), u->chkBgTransparent->isChecked() ? 0 : 255 );
  sty.mathmode = (u->chkMathMode->isChecked() ? u->cbxMathMode->currentText() : "...");
  sty.preamble = u->txtPreamble->toPlainText();
  sty.dpi = u->spnDPI->value();

  return sty;
}

QFont KLFMainWin::txtLatexFont() const
{
  return u->txtLatex->font();
}
QFont KLFMainWin::txtPreambleFont() const
{
  return u->txtPreamble->font();
}

void KLFMainWin::slotLoadStyleAct()
{
  QAction *a = qobject_cast<QAction*>(sender());
  if (a) {
    slotLoadStyle(a->data().toInt());
  }
}

void KLFMainWin::slotLoadStyle(const KLFStyle& style)
{
  QColor cfg, cbg;
  cfg.setRgb(style.fg_color);
  cbg.setRgb(style.bg_color);
  u->colFg->setColor(cfg);
  u->colBg->setColor(cbg);
  u->chkBgTransparent->setChecked(qAlpha(style.bg_color) == 0);
  u->chkMathMode->setChecked(style.mathmode.simplified() != "...");
  if (style.mathmode.simplified() != "...")
    u->cbxMathMode->setEditText(style.mathmode);
  slotSetPreamble(style.preamble); // to preserve text edit undo/redo
  u->spnDPI->setValue(style.dpi);
}

void KLFMainWin::slotLoadStyle(int n)
{
  if (n < 0 || n >= (int)_styles.size())
    return; // let's not try to load an inexistant style...

  slotLoadStyle(_styles[n]);
}
void KLFMainWin::slotSaveStyle()
{
  KLFStyle sty;

  QString name = QInputDialog::getText(this, tr("Enter Style Name"),
				       tr("Enter new style name:"));
  if (name.isEmpty()) {
    return;
  }

  // check to see if style exists already
  int found_i = -1;
  for (int kl = 0; found_i == -1 && kl < _styles.size(); ++kl) {
    if (_styles[kl].name.trimmed() == name.trimmed()) {
      found_i = kl;
      // style exists already
      int r = QMessageBox::question(this, tr("Overwrite Style"),
				    tr("Style name already exists. Do you want to overwrite?"),
				    QMessageBox::Yes|QMessageBox::No|QMessageBox::Cancel, QMessageBox::No);
      switch (r) {
      case QMessageBox::No:
	slotSaveStyle(); // recurse, ask for new name.
	return; // and stop this function execution of course!
      case QMessageBox::Yes:
	break; // continue normally
      default:
	return;	// forget everything; "Cancel"
      }
    }
  }

  sty = currentStyle();
  sty.name = name;

  if (found_i == -1)
    _styles.append(sty);
  else
    _styles[found_i] = sty;

  emit stylesChanged();

  refreshStylePopupMenus();

  // auto-save our style list
  saveStyles();
}

void KLFMainWin::slotStyleManager()
{
  mStyleManager->show();
}


void KLFMainWin::slotSettings()
{
  mSettingsDialog->show();
}


void KLFMainWin::closeEvent(QCloseEvent *event)
{
  if (_ignore_close_event) {
    // simple hide, not close
    hide();

    event->ignore();
    return;
  }

  event->accept();
  quit();
}




