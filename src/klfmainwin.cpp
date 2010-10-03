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

#include <QtCore>
#include <QtGui>

#include <klfbackend.h>

#include <ui_klfprogerr.h>
#include <ui_klfmainwin.h>

#include <klflibview.h>
#include <klfguiutil.h>

#include "klflatexedit.h"
#include "klflibbrowser.h"
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
  u = new Ui::KLFProgErr;
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
    _mutex.lock();
    bool abrt = _abort;
    _mutex.unlock();
    if (abrt)
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

    if ( input.latex.trimmed().isEmpty() ) {
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
    }

    _mutex.lock();
    bool abort = _abort;
    bool hasnewinfo = _hasnewinfo;
    _mutex.unlock();

    if (abort)
      return;
    if (hasnewinfo)
      continue;

    emit previewAvailable(img, output.status != 0);

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

#ifdef Q_WS_MAC
  // watch for QFileOpenEvent s on mac
  QApplication::instance()->installEventFilter(this);
#endif

  mPopup = NULL;

  loadSettings();

  _firstshow = true;

  _output.status = 0;
  _output.errorstr = QString();


  // load styless
  loadStyles();

  mLatexSymbols = new KLFLatexSymbols(this, _settings);

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


  connect(u->txtLatex->syntaxHighlighter(), SIGNAL(newSymbolTyped(const QString&)),
	  this, SLOT(slotNewSymbolTyped(const QString&)));

  u->lblOutput->setLabelFixedSize(klfconfig.UI.labelOutputFixedSize);
  u->lblOutput->setEnableToolTipPreview(klfconfig.UI.enableToolTipPreview);
  u->lblOutput->setGlowEffect(klfconfig.UI.glowEffect);
  u->lblOutput->setGlowEffectColor(klfconfig.UI.glowEffectColor);
  u->lblOutput->setGlowEffectRadius(klfconfig.UI.glowEffectRadius);

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
  u->txtLatex->setMainWinDataOpener(this);

  setFixedSize(_shrinkedsize);

  u->btnEvaluate->installEventFilter(this);


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

  loadDefaultStyle();

  // For systematical syntax highlighting
  // make sure syntax highlighting is up-to-date at all times
  QTimer *synthighlighttimer = new QTimer(this);
  connect(synthighlighttimer, SIGNAL(timeout()), u->txtLatex->syntaxHighlighter(), SLOT(refreshAll()));
  connect(synthighlighttimer, SIGNAL(timeout()), u->txtPreamble->syntaxHighlighter(), SLOT(refreshAll()));
  synthighlighttimer->start(250);

  // initialize the margin unit selector
  u->cbxMarginsUnit->setCurrentUnitAbbrev("pt");

  // set custom math modes
  u->cbxMathMode->addItems(klfconfig.UI.customMathModes);

  // load library
  mLibBrowser = new KLFLibBrowser(this);

  refreshShowCorrectClearButton();

  // -- MAJOR SIGNAL/SLOT CONNECTIONS --

  //  connect(u->btnClear, SIGNAL(toggled(bool)), this, SLOT(slotClear()));
  connect(u->aClearLatex, SIGNAL(triggered()), this, SLOT(slotClearLatex()));
  connect(u->aClearAll, SIGNAL(triggered()), this, SLOT(slotClearAll()));
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
  connect(mLatexSymbols, SIGNAL(insertSymbol(const KLFLatexSymbol&)),
	  this, SLOT(insertSymbol(const KLFLatexSymbol&)));


  // our help/about dialog
  connect(u->btnHelp, SIGNAL(clicked()), this, SLOT(showAbout()));

  // -- SMALL REAL-TIME PREVIEW GENERATOR THREAD --

  mPreviewBuilderThread = new KLFPreviewBuilderThread(this, collectInput(false), _settings,
						      klfconfig.UI.labelOutputFixedSize.width(),
						      klfconfig.UI.labelOutputFixedSize.height());

  connect(u->txtLatex, SIGNAL(textChanged()), this,
	  SLOT(updatePreviewBuilderThreadInput()), Qt::QueuedConnection);
  connect(u->txtLatex, SIGNAL(insertContextMenuActions(const QPoint&, QList<QAction*> *)),
	  this, SLOT(slotEditorContextMenuInsertActions(const QPoint&, QList<QAction*> *)));
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

  _evaloutput_uptodate = true;

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
  mHelpLinkActions << HelpLinkAction("/popup_close", this, "slotPopupClose", false);
  mHelpLinkActions << HelpLinkAction("/popup", this, "slotPopupAction", true);
  mHelpLinkActions << HelpLinkAction("/settings", this, "showSettingsHelpLinkAction", true);

  if ( klfconfig.Core.thisVersionMajMinFirstRun && ! klfconfig.checkExePaths() ) {
    addWhatsNewText("<p><b><span style=\"color: #a00000\">"+
		    tr("Your executable paths (latex, dvips, gs) seem not to be detected properly. "
		       "Please adjust the settings in the <a href=\"klfaction:/settings?control="
		       "ExecutablePaths\">settings dialog</a>.",
		       "[[additional text in what's-new-dialog in case of bad detected settings. this "
		       "is HTML formatted text.]]")
		    +"</span></b></p>");
  }

  // and load the library

  _loadedlibrary = true;
  loadLibrary();

  registerDataOpener(new KLFBasicDataOpener(this));
}

void KLFMainWin::retranslateUi(bool alsoBaseUi)
{
  if (alsoBaseUi)
    u->retranslateUi(this);

  // version information as tooltip on the welcome widget
  u->lblPromptMain->setToolTip(tr("KLatexFormula %1").arg(QString::fromUtf8(KLF_VERSION_STRING)));

  refreshStylePopupMenus();
}


KLFMainWin::~KLFMainWin()
{
  klfDbg( "()" ) ;

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

  if (mPreviewBuilderThread)
    delete mPreviewBuilderThread;

  delete u;
}

void KLFMainWin::startupFinished()
{
  //  if ( ! _loadedlibrary ) {
  //    _loadedlibrary = true;
  //
  //    // load the library browser's saved state
  //    QMetaObject::invokeMethod(this, "loadLibrary", Qt::QueuedConnection);
  //  }


  // Export profiles selection list

  pExportProfileQuickMenuActionList.clear();
  int k;
  QList<KLFMimeExportProfile> eplist = KLFMimeExportProfile::exportProfileList();
  QMenu *menu = new QMenu(u->btnSetExportProfile);
  QActionGroup *actionGroup = new QActionGroup(menu);
  actionGroup->setExclusive(true);
  QSignalMapper *smapper = new QSignalMapper(menu);
  for (k = 0; k < eplist.size(); ++k) {
    QAction *action = new QAction(actionGroup);
    action->setText(eplist[k].description());
    action->setData(eplist[k].profileName());
    action->setCheckable(true);
    action->setChecked( (klfconfig.UI.dragExportProfile==klfconfig.UI.copyExportProfile) &&
			(klfconfig.UI.dragExportProfile==eplist[k].profileName()) );
    menu->addAction(action);
    smapper->setMapping(action, eplist[k].profileName());
    connect(action, SIGNAL(triggered()), smapper, SLOT(map()));
    pExportProfileQuickMenuActionList.append(action);
  }
  connect(smapper, SIGNAL(mapped(const QString&)), this, SLOT(slotSetExportProfile(const QString&)));

  u->btnSetExportProfile->setMenu(menu);
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

void KLFMainWin::refreshShowCorrectClearButton()
{
  bool lo = klfconfig.UI.clearLatexOnly;
  u->btnClearAll->setVisible(!lo);
  u->btnClearLatex->setVisible(lo);
}


bool KLFMainWin::loadDefaultStyle()
{
  // load style "default" or "Default" (or translation) if one of them exists
  bool r;
  r = loadNamedStyle(tr("Default", "[[style name]]"));
  r = r || loadNamedStyle("Default");
  r = r || loadNamedStyle("default");
  return r;
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
  _settings.execenv = klfconfig.BackendSettings.execenv;

  _settings.lborderoffset = klfconfig.BackendSettings.lborderoffset;
  _settings.tborderoffset = klfconfig.BackendSettings.tborderoffset;
  _settings.rborderoffset = klfconfig.BackendSettings.rborderoffset;
  _settings.bborderoffset = klfconfig.BackendSettings.bborderoffset;

  _settings.outlineFonts = klfconfig.BackendSettings.outlineFonts;

  _settings_altered = false;
}

void KLFMainWin::saveSettings()
{
  if ( ! _settings_altered ) {    // don't save altered settings
    klfconfig.BackendSettings.tempDir = _settings.tempdir;
    klfconfig.BackendSettings.execLatex = _settings.latexexec;
    klfconfig.BackendSettings.execDvips = _settings.dvipsexec;
    klfconfig.BackendSettings.execGs = _settings.gsexec;
    klfconfig.BackendSettings.execEpstopdf = _settings.epstopdfexec;
    klfconfig.BackendSettings.execenv = _settings.execenv;
    klfconfig.BackendSettings.lborderoffset = _settings.lborderoffset;
    klfconfig.BackendSettings.tborderoffset = _settings.tborderoffset;
    klfconfig.BackendSettings.rborderoffset = _settings.rborderoffset;
    klfconfig.BackendSettings.bborderoffset = _settings.bborderoffset;
    klfconfig.BackendSettings.outlineFonts = _settings.outlineFonts;
  }

  klfconfig.UI.userColorList = KLFColorChooser::colorList();
  klfconfig.UI.colorChooseWidgetRecent = KLFColorChooseWidget::recentColors();
  klfconfig.UI.colorChooseWidgetCustom = KLFColorChooseWidget::customColors();

  klfconfig.writeToConfig();

  mPreviewBuilderThread->settingsChanged(_settings, klfconfig.UI.labelOutputFixedSize.width(),
					 klfconfig.UI.labelOutputFixedSize.height());

  u->lblOutput->setLabelFixedSize(klfconfig.UI.labelOutputFixedSize);
  u->lblOutput->setEnableToolTipPreview(klfconfig.UI.enableToolTipPreview);

  u->lblOutput->setGlowEffect(klfconfig.UI.glowEffect);
  u->lblOutput->setGlowEffectColor(klfconfig.UI.glowEffectColor);
  u->lblOutput->setGlowEffectRadius(klfconfig.UI.glowEffectRadius);

  int k;
  if (klfconfig.UI.dragExportProfile == klfconfig.UI.copyExportProfile) {
    klfDbg("checking quick menu action item export profile="<<klfconfig.UI.copyExportProfile) ;
    // check this export profile in the quick export profile menu
    for (k = 0; k < pExportProfileQuickMenuActionList.size(); ++k) {
      if (pExportProfileQuickMenuActionList[k]->data().toString() == klfconfig.UI.copyExportProfile) {
	klfDbg("checking item #"<<k<<": "<<pExportProfileQuickMenuActionList[k]->data()) ;
	// found the one
	pExportProfileQuickMenuActionList[k]->setChecked(true);
	break; // by auto-exclusivity the other ones will be un-checked.
      }
    }
  } else {
    // uncheck all items (since there is not a unique one set, drag and copy differ)
    for (k = 0; k < pExportProfileQuickMenuActionList.size(); ++k)
      pExportProfileQuickMenuActionList[k]->setChecked(false);
  }

  if (klfconfig.UI.enableRealTimePreview) {
    if ( ! mPreviewBuilderThread->isRunning() ) {
      delete mPreviewBuilderThread;
      mPreviewBuilderThread =
	new KLFPreviewBuilderThread(this, collectInput(false), _settings,
				    klfconfig.UI.labelOutputFixedSize.width(),
				    klfconfig.UI.labelOutputFixedSize.height());
      mPreviewBuilderThread->start();
    }
  } else {
    if ( mPreviewBuilderThread->isRunning() ) {
      delete mPreviewBuilderThread;
      // do NOT leave a NULL mPreviewBuilderThread !
      mPreviewBuilderThread =
	new KLFPreviewBuilderThread(this, collectInput(false), _settings,
				    klfconfig.UI.labelOutputFixedSize.width(),
				    klfconfig.UI.labelOutputFixedSize.height());
    }
  }
}

void KLFMainWin::refreshStylePopupMenus()
{
  if (mStyleMenu == NULL)
    mStyleMenu = new QMenu(this);
  mStyleMenu->clear();

  QAction *a;
  for (int i = 0; i < _styles.size(); ++i) {
    a = mStyleMenu->addAction(_styles[i].name);
    a->setData(i);
    connect(a, SIGNAL(triggered()), this, SLOT(slotLoadStyleAct()));
  }

  mStyleMenu->addSeparator();
  mStyleMenu->addAction(QIcon(":/pics/managestyles.png"), tr("Manage Styles"),
			 this, SLOT(slotStyleManager()), 0 /* accel */);

}



static QString kdelocate(const char *fname)
{
  QString candidate;

  QStringList env = QProcess::systemEnvironment();
  QStringList kdehome = env.filter(QRegExp("^KDEHOME="));
  if (kdehome.size() == 0) {
    candidate = QDir::homePath() + QString("/.kde/share/apps/klatexformula/") + QString::fromLocal8Bit(fname);
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

bool KLFMainWin::try_load_style_list(const QString& styfname)
{
  if ( ! QFile::exists(styfname) )
    return false;

  QFile fsty(styfname);
  if ( ! fsty.open(QIODevice::ReadOnly) )
    return false;

  QDataStream str(&fsty);

  QString readHeader;
  QString readCompatKLFVersion;
  bool r = klfDataStreamReadHeader(str, QStringList()<<QLatin1String("KLATEXFORMULA_STYLE_LIST"),
				   &readHeader, &readCompatKLFVersion);
  if (!r) {
    if (readHeader.isEmpty() || readCompatKLFVersion.isEmpty()) {
      QMessageBox::critical(this, tr("Error"), tr("Error: Style file is incorrect or corrupt!\n"));
      return false;
    }
    // too recent version warning
    QMessageBox::warning(this, tr("Load Styles"),
			 tr("The style file found was created by a more recent version "
			    "of KLatexFormula.\n"
			    "The process of style loading may fail.")
			 );
  }
  // read the header, just need to read data now
  str >> _styles;
  return true;
}

void KLFMainWin::loadStyles()
{
  _styles = KLFStyleList(); // empty list to start with

  QStringList styfnamecandidates;
  styfnamecandidates << klfconfig.homeConfigDir + QString("/styles-klf%1").arg(KLF_DATA_STREAM_APP_VERSION)
		     << klfconfig.homeConfigDir + QLatin1String("/styles")
		     << QLatin1String("kde-locate"); // locate in KDE only if necessary in for loop below

  int k;
  bool result = false;
  for (k = 0; k < styfnamecandidates.size(); ++k) {
    QString fn = styfnamecandidates[k];
    if (fn == QLatin1String("kde-locate"))
      fn = kdelocate("styles");
    // try to load this file
    if ( (result = try_load_style_list(fn)) == true )
      break;
  }
  //  Don't fail if result is false, this is the case on first run (!)
  //  if (!result) {
  //    QMessageBox::critical(this, tr("Error"), tr("Error: Unable to load your style list!"));
  //  }

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
  QString s = klfconfig.homeConfigDir + QString("/styles-klf%1").arg(KLF_DATA_STREAM_APP_VERSION);
  QFile f(s);
  if ( ! f.open(QIODevice::WriteOnly) ) {
    QMessageBox::critical(this, tr("Error"), tr("Error: Unable to write to styles file!\n%1").arg(s));
    return;
  }
  QDataStream stream(&f);

  klfDataStreamWriteHeader(stream, "KLATEXFORMULA_STYLE_LIST");

  stream << _styles;
}

void KLFMainWin::loadLibrary()
{
  KLF_DEBUG_TIME_BLOCK(KLF_FUNC_NAME) ;

  // start by loading the saved library browser state.
  // Inside this function the value of klfconfig.LibraryBrowser.restoreURLs is taken
  // into account.
  loadLibrarySavedState();

  // Locate a good library/history file.

  //  KLFPleaseWaitPopup pwp(tr("Loading library, please wait..."), this);
  //  pwp.showPleaseWait();

  // the default library file
  QString localfname = klfconfig.Core.libraryFileName;
  if (QFileInfo(localfname).isRelative())
    localfname.prepend(klfconfig.homeConfigDir + "/");

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
      importfname = ":/data/defaultlibrary.klf";
    }
  }

  // importfname is non-empty only if the local .klf.db library file is inexistant.

  QUrl localliburl = QUrl::fromLocalFile(localfname);
  localliburl.setScheme(klfconfig.Core.libraryLibScheme);
  localliburl.addQueryItem("klfDefaultSubResource", "History");

  // If the history is already open from library browser saved state, retrieve it
  // This is possibly NULL
  mHistoryLibResource = mLibBrowser->getOpenResource(localliburl);

  if (!QFile::exists(localfname)) {
    // create local library resource
    KLFLibEngineFactory *localLibFactory = KLFLibEngineFactory::findFactoryFor(localliburl.scheme());
    if (localLibFactory == NULL) {
      qWarning()<<KLF_FUNC_NAME<<": Can't find library resource engine factory for scheme "<<localliburl.scheme()<<"!";
      qFatal("%s: Can't find library resource engine factory for scheme '%s'!",
	     KLF_FUNC_NAME, qPrintable(localliburl.scheme()));
      return;
    }
    KLFLibEngineFactory::Parameters param;
    param["Filename"] = localliburl.path();
    param["klfDefaultSubResource"] = QLatin1String("History");
    param["klfDefaultSubResourceTitle"] = tr("History", "[[default sub-resource title for history]]");
    mHistoryLibResource = localLibFactory->createResource(localliburl.scheme(), param, this);
    if (mHistoryLibResource == NULL) {
      qWarning()<<KLF_FUNC_NAME<<": Can't create resource engine for library, of scheme "<<localliburl.scheme()<<"! "
		<<"Create parameters are "<<param;
      qFatal("%s: Can't create resource engine for library!", KLF_FUNC_NAME);
      return;
    }
    mHistoryLibResource->setTitle(tr("Local Library"));
    mHistoryLibResource
      -> createSubResource(QLatin1String("Archive"),
			   tr("Archive", "[[default sub-resource title for archive sub-resource]]"));
    if (mHistoryLibResource->supportedFeatureFlags() & KLFLibResourceEngine::FeatureSubResourceProps) {
      mHistoryLibResource->setSubResourceProperty(QLatin1String("History"),
						  KLFLibResourceEngine::SubResPropViewType,
						  QLatin1String("default+list"));
      mHistoryLibResource->setSubResourceProperty(QLatin1String("Archive"),
						  KLFLibResourceEngine::SubResPropViewType,
						  QLatin1String("default"));
    } else {
      mHistoryLibResource->setViewType(QLatin1String("default+list"));
    }
  }

  if (mHistoryLibResource == NULL) {
    mHistoryLibResource = KLFLibEngineFactory::openURL(localliburl, this);
  }
  if (mHistoryLibResource == NULL) {
    qWarning()<<"KLFMainWin::loadLibrary(): Can't open local history resource!\n\tURL="<<localliburl
	      <<"\n\tNot good! Expect crash!";
    return;
  }

  klfDbg("opened history: resource-ptr="<<mHistoryLibResource<<"\n\tImportFile="<<importfname);

  if (!importfname.isEmpty()) {
    // needs an import
    klfDbg("Importing library from "<<importfname) ;

    // visual feedback for import
    KLFProgressDialog pdlg(QString(), this);
    connect(mHistoryLibResource, SIGNAL(operationStartReportingProgress(KLFProgressReporter *,
									const QString&)),
	    &pdlg, SLOT(startReportingProgress(KLFProgressReporter *)));
    pdlg.setAutoClose(false);
    pdlg.setAutoReset(false);

    // locate the import file and scheme
    QUrl importliburl = QUrl::fromLocalFile(importfname);
    QString scheme = KLFLibBasicWidgetFactory::guessLocalFileScheme(importfname);
    if (scheme.isEmpty()) {
      // assume .klf if not able to guess
      scheme = "klf+legacy";
    }
    importliburl.setScheme(scheme);
    importliburl.addQueryItem("klfReadOnly", "true");
    // import library from an older version library file.
    KLFLibEngineFactory *factory = KLFLibEngineFactory::findFactoryFor(importliburl.scheme());
    if (factory == NULL) {
      qWarning()<<"KLFMainWin::loadLibrary(): Can't find factory for URL="<<importliburl<<"!";
    } else {
      KLFLibResourceEngine *importres = factory->openResource(importliburl, this);
      // import all sub-resources
      QStringList subResList = importres->subResourceList();
      klfDbg( "\tImporting sub-resources: "<<subResList ) ;
      int j;
      for (j = 0; j < subResList.size(); ++j) {
	QString subres = subResList[j];
	pdlg.setDescriptiveText(tr("Importing Library from previous version of KLatexFormula ... "
				   "%3 (%1/%2)")
				.arg(j+1).arg(subResList.size()).arg(subResList[j]));
	QList<KLFLibResourceEngine::KLFLibEntryWithId> allentries
	  = importres->allEntries(subres);
	klfDbg("Got "<<allentries.size()<<" entries from sub-resource "<<subres);
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
  r = mLibBrowser->openResource(mHistoryLibResource,
				KLFLibBrowser::NoCloseRoleFlag|KLFLibBrowser::OpenNoRaise,
  				QLatin1String("default+list"));
  if ( ! r ) {
    qWarning()<<"KLFMainWin::loadLibrary(): Can't open local history resource!\n\tURL="<<localliburl
  	      <<"\n\tExpect Crash!";
    return;
  }

  // open all other sub-resources present in our library
  QStringList subresources = mHistoryLibResource->subResourceList();
  int k;
  for (k = 0; k < subresources.size(); ++k) {
    //    if (subresources[k] == QLatin1String("History")) // already open
    //      continue;
    // if a URL is already open, it won't open it a second time.
    QUrl url = localliburl;
    url.removeAllQueryItems("klfDefaultSubResource");
    url.addQueryItem("klfDefaultSubResource", subresources[k]);

    uint flags = KLFLibBrowser::NoCloseRoleFlag|KLFLibBrowser::OpenNoRaise;
    QString sr = subresources[k].toLower();
    if (sr == "history" || sr == tr("History").toLower())
      flags |= KLFLibBrowser::HistoryRoleFlag;
    if (sr == "archive" || sr == tr("Archive").toLower())
      flags |= KLFLibBrowser::ArchiveRoleFlag;

    mLibBrowser->openResource(url, flags);
  }
}

void KLFMainWin::loadLibrarySavedState()
{
  QString fname = klfconfig.homeConfigDir + "/libbrowserstate.xml";
  if ( ! QFile::exists(fname) ) {
    klfDbg("No saved library browser state found ("<<fname<<")");
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

    qWarning("%s: Ignoring unrecognized node `%s' in file %s.", KLF_FUNC_NAME,
	     qPrintable(e.nodeName()), qPrintable(fname));
  }

  mLibBrowser->loadGuiState(vstatemap, klfconfig.LibraryBrowser.restoreURLs);
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

  f.write(doc.toByteArray(2));
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
  if (restoreFlags & KLFLib::RestoreStyle) {
    KLFStyle style = entry.style();
    slotLoadStyle(style);
  }
  // restore latex after style, so that the parser-hint-popup doesn't appear for packages
  // that we're going to include anyway
  if (restoreFlags & KLFLib::RestoreLatex) {
    // to preserve text edit undo history... call this slot instead of brutally doing txt->setPlainText(..)
    slotSetLatex(entry.latexWithCategoryTagsComments());
  }

  u->lblOutput->display(entry.preview(), entry.preview(), false);
  u->frmOutput->setEnabled(false);
  activateWindow();
  raise();
  u->txtLatex->setFocus();
}

void KLFMainWin::slotLibraryButtonRefreshState(bool on)
{
  u->btnLibrary->setChecked(on);
}


void KLFMainWin::insertSymbol(const KLFLatexSymbol& s)
{
  // see if we need to insert the xtrapreamble
  QStringList cmds = s.preamble;
  int k;
  QString preambletext = u->txtPreamble->toPlainText();
  QString addtext;
  for (k = 0; k < cmds.size(); ++k) {
    slotEnsurePreambleCmd(cmds[k]);
  }

  // only after actually insert the symbol, so as not to interfere with popup package suggestions.

  QTextCursor c1(u->txtLatex->document());
  c1.beginEditBlock();
  c1.setPosition(u->txtLatex->textCursor().position());
  c1.insertText(s.symbol+"\n");
  c1.deletePreviousChar(); // small hack for forcing a separate undo command when two symbols are inserted
  c1.endEditBlock();

  activateWindow();
  raise();
  u->txtLatex->setFocus();
}

void KLFMainWin::insertDelimiter(const QString& delim, int charsBack)
{
  u->txtLatex->insertDelimiter(delim, charsBack);
}


void KLFMainWin::getMissingCmdsFor(const QString& symbol, QStringList * missingCmds,
				   QString *guiText, bool wantHtmlText)
{
  if (missingCmds == NULL) {
    qWarning()<<KLF_FUNC_NAME<<": missingCmds is NULL!";
    return;
  }
  if (guiText == NULL) {
    qWarning()<<KLF_FUNC_NAME<<": guiText is NULL!";
    return;
  }

  missingCmds->clear();
  *guiText = QString();

  if (symbol.isEmpty())
    return;

  KLFLatexSymbol sym = KLFLatexSymbolsCache::theCache()->findSymbol(symbol);
  if (!sym.valid())
    return;

  QStringList cmds = sym.preamble;
  klfDbg("Got symbol for "<<symbol<<"; cmds is "<<cmds);
  if (cmds.isEmpty()) {
    return; // no need to include anything
  }

  QString curpreambletext = u->txtPreamble->toPlainText();
  QStringList packages;
  bool moreDefinitions = false;
  int k;
  for (k = 0; k < cmds.size(); ++k) {
    if (curpreambletext.indexOf(cmds[k]) >= 0)
      continue; // this line is already included

    missingCmds->append(cmds[k]);

    QRegExp rx("\\\\usepackage.*\\{(\\w+)\\}");
    klfDbg(" regexp="<<rx.pattern()<<"; cmd for symbol: "<<cmds[k].trimmed());
    if (rx.exactMatch(cmds[k].trimmed())) {
      packages << rx.cap(1); // the package name
    } else {
      // not a package inclusion
      moreDefinitions = true;
    }
  }
  if (wantHtmlText)
    packages.replaceInStrings(QRegExp("^(.*)$"), QString("<b>\\1</b>"));

  QString requiredtext;
  if (packages.size()) {
    if (packages.size() == 1)
      requiredtext += tr("package %1",
			 "[[part of popup text, if one package only]]").arg(packages[0]);
    else
      requiredtext += tr("packages %1",
			 "[[part of popup text, if multiple packages]]").arg(packages.join(", "));
  }
  if (moreDefinitions)
    requiredtext += packages.size()
      ? tr(" and <i>some more definitions</i>",
	   "[[part of hint popup text, when packages also need to be included]]")
      : tr("<i>some definitions</i>",
	   "[[part of hint popup text, when no packages need to be included]]");
  if (!wantHtmlText)
    requiredtext.replace(QRegExp("<[^>]*>"), "");

  *guiText = requiredtext;
}

void KLFMainWin::slotNewSymbolTyped(const QString& symbol)
{
  if (mPopup == NULL)
    mPopup = new KLFMainWinPopup(this);

  if (!klfconfig.UI.showHintPopups)
    return;

  QStringList cmds;
  QString requiredtext;

  getMissingCmdsFor(symbol, &cmds, &requiredtext);

  if (cmds.isEmpty()) // nothing to include
    return;

  QByteArray cmdsData = klfSaveVariantToText(QVariant(cmds));
  QString msgkey = "preambleCmdStringList:"+QString::fromLatin1(cmdsData);
  QUrl urlaction;
  urlaction.setScheme("klfaction");
  urlaction.setPath("/popup");
  QUrl urlactionClose = urlaction;
  urlaction.addQueryItem("preambleCmdStringList", QString::fromLatin1(cmdsData));
  urlactionClose.addQueryItem("removeMessageKey", msgkey);
  mPopup->addMessage(msgkey,
		     //"[<a href=\""+urlactionClose.toEncoded()+"\">"
		     //"<img border=\"0\" src=\":/pics/smallclose.png\"></a>] - "+
		     tr("Symbol <tt>%3</tt> may require <a href=\"%1\">%2</a>.")
		     .arg(urlaction.toEncoded(), requiredtext, symbol) );
  mPopup->show();
  mPopup->raise();
}
void KLFMainWin::slotPopupClose()
{
  if (mPopup != NULL) {
    delete mPopup;
    mPopup = NULL;
  }
}
void KLFMainWin::slotPopupAction(const QUrl& url)
{
  klfDbg("url="<<url.toEncoded());
  if (url.hasQueryItem("preambleCmdStringList")) {
    // include the given package
    QByteArray data = url.queryItemValue("preambleCmdStringList").toLatin1();
    klfDbg("data="<<data);
    QStringList cmds = klfLoadVariantFromText(data, "QStringList").toStringList();
    klfDbg("ensuring CMDS="<<cmds);
    int k;
    for (k = 0; k < cmds.size(); ++k) {
      slotEnsurePreambleCmd(cmds[k]);
    }
    mPopup->removeMessage("preambleCmdStringList:"+QString::fromLatin1(data));
    if (!mPopup->hasMessages())
      slotPopupClose();
    return;
  }
  if (url.hasQueryItem("acceptAll")) {
    slotPopupAcceptAll();
    return;
  }
  if (url.hasQueryItem("removeMessageKey")) {
    QString key = url.queryItemValue("removeMessageKey");
    mPopup->removeMessage(key);
    if (!mPopup->hasMessages())
      slotPopupClose();
    return;
  }
  if (url.hasQueryItem("configDontShow")) {
    // don't show popups
    slotPopupClose();
    klfconfig.UI.showHintPopups = false;
    saveSettings();
    return;
  }
}
void KLFMainWin::slotPopupAcceptAll()
{
  if (mPopup == NULL)
    return;

  // accept all messages, based on their key
  QStringList keys = mPopup->messageKeys();
  int k;
  for (k = 0; k < keys.size(); ++k) {
    QString s = keys[k];
    if (s.startsWith("preambleCmdStringList:")) {
      QByteArray data = s.mid(strlen("preambleCmdStringList:")).toLatin1();
      QStringList cmds = klfLoadVariantFromText(data, "QStringList").toStringList();
      int k;
      for (k = 0; k < cmds.size(); ++k) {
	slotEnsurePreambleCmd(cmds[k]);
      }
      continue;
    }
    qWarning()<<KLF_FUNC_NAME<<": Unknown message key: "<<s;
  }
  slotPopupClose();
}

void KLFMainWin::slotEditorContextMenuInsertActions(const QPoint& pos, QList<QAction*> *actionList)
{
  KLFLatexEdit *latexEdit = qobject_cast<KLFLatexEdit*>(sender());
  KLF_ASSERT_NOT_NULL( latexEdit, "KLFLatexEdit sender is NULL!", return ) ;

  // try to determine if we clicked on a symbol, and suggest to include the corresponding package

  QTextCursor cur = latexEdit->cursorForPosition(pos);
  QString text = latexEdit->toPlainText();
  int curpos = cur.position();

  QRegExp rxSym("\\\\(\\w+|.)");
  int i = text.lastIndexOf(rxSym, curpos); // search backwards from curpos
  if (i >= 0) {
    // matched somewhere before in text. See if we clicked on it
    int len = rxSym.matchedLength();
    QString symbol = text.mid(i, len);
    QStringList cmds;
    QString guitext;
    getMissingCmdsFor(symbol, &cmds, &guitext, false);
    if (cmds.size()) {
      QAction * aInsCmds = new QAction(latexEdit);
      aInsCmds->setText(tr("Include missing definitions for %1").arg(symbol));
      aInsCmds->setData(QVariant(cmds));
      connect(aInsCmds, SIGNAL(triggered()), this, SLOT(slotInsertMissingPackagesFromActionSender()));
      *actionList << aInsCmds;
    }
  }

  QAction *insertSymbolAction = new QAction(QIcon(":/pics/symbols.png"),
					    tr("Insert Symbol ...", "[[context menu entry]]"), latexEdit);
  connect(insertSymbolAction, SIGNAL(triggered()), this, SLOT(slotSymbols()));
  *actionList << insertSymbolAction;
}


void KLFMainWin::slotInsertMissingPackagesFromActionSender()
{
  QAction * a = qobject_cast<QAction*>(sender());
  if (a == NULL) {
    qWarning()<<KLF_FUNC_NAME<<": NULL QAction sender!";
    return;
  }
  QVariant data = a->data();
  QStringList cmds = data.toStringList();
  int k;
  for (k = 0; k < cmds.size(); ++k) {
    slotEnsurePreambleCmd(cmds[k]);
  }
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

void KLFMainWin::showSettingsHelpLinkAction(const QUrl& link)
{
  klfDbg( ": link="<<link ) ;
  mSettingsDialog->show();
  mSettingsDialog->showControl(link.queryItemValue("control"));
}

void KLFMainWin::helpLinkAction(const QUrl& link)
{
  klfDbg( ": Link is "<<link<<"; scheme="<<link.scheme()<<"; path="<<link.path()
	  <<"; queryItems="<<link.queryItems() ) ;

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
	  QMetaObject::invokeMethod(mHelpLinkActions[k].reciever,
				    mHelpLinkActions[k].memberFunc.constData(),
				    Qt::QueuedConnection, Q_ARG(QUrl, link));
	else
	  QMetaObject::invokeMethod(mHelpLinkActions[k].reciever,
				    mHelpLinkActions[k].memberFunc.constData(),
				    Qt::QueuedConnection);

	calledOne = true;
      }
    }
    if (!calledOne) {
      qWarning()<<KLF_FUNC_NAME<<": no action found for link="<<link;
    }
    return;
  }
  qWarning()<<KLF_FUNC_NAME<<"("<<link<<"): Unrecognized link scheme!";
}

void KLFMainWin::addWhatsNewText(const QString& htmlSnipplet)
{
  mWhatsNewDialog->addExtraText(htmlSnipplet);
}


void KLFMainWin::registerHelpLinkAction(const QString& path, QObject *object, const char * member,
					bool wantUrlParam)
{
  mHelpLinkActions << HelpLinkAction(path, object, member, wantUrlParam);
}


void KLFMainWin::registerOutputSaver(KLFAbstractOutputSaver *outputsaver)
{
  KLF_ASSERT_NOT_NULL( outputsaver, "Refusing to register NULL Output Saver object!",  return ) ;

  pOutputSavers.append(outputsaver);
}

void KLFMainWin::unregisterOutputSaver(KLFAbstractOutputSaver *outputsaver)
{
  pOutputSavers.removeAll(outputsaver);
}

void KLFMainWin::registerDataOpener(KLFAbstractDataOpener *dataopener)
{
  KLF_ASSERT_NOT_NULL( dataopener, "Refusing to register NULL Data Opener object!",  return ) ;

  pDataOpeners.append(dataopener);
}
void KLFMainWin::unregisterDataOpener(KLFAbstractDataOpener *dataopener)
{
  pDataOpeners.removeAll(dataopener);
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
  KLF_DEBUG_BLOCK(KLF_FUNC_NAME) ;

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
  klfDbg("e->type() == "<<e->type());
  
  return QWidget::event(e);
}

void KLFMainWin::childEvent(QChildEvent *e)
{
  QObject *child = e->child();
  if (e->type() == QEvent::ChildAdded && child->isWidgetType()) {
    QWidget *w = qobject_cast<QWidget*>(child);
    if (w->windowFlags() & Qt::Window) {
      // note that all Qt::Tool, Qt::SplashScreen etc. all have the Qt::Window bit set, but
      // not Qt::Widget (!)
      pWindowList.append(w);
      klfDbg( ": Added child "<<w<<" ("<<w->objectName()<<")" ) ;
    }
  } else if (e->type() == QEvent::ChildRemoved && child->isWidgetType()) {
    QWidget *w = qobject_cast<QWidget*>(child);
    int k;
    if ((k = pWindowList.indexOf(w)) >= 0) {
      klfDbg( ": Removing child "<<w<<" ("<<w->objectName()<<")"
	      <<" at position k="<<k ) ;
      pWindowList.removeAt(k);
    }
  }

  QWidget::childEvent(e);
}

bool KLFMainWin::eventFilter(QObject *obj, QEvent *e)
{

  // even with QShortcuts, we still need this event handler (why?)
  if (obj == u->txtLatex) {
    if (e->type() == QEvent::KeyPress) {
      QKeyEvent *ke = (QKeyEvent*) e;
      if (ke->key() == Qt::Key_Return &&
	  (QApplication::keyboardModifiers() == Qt::ShiftModifier ||
	   QApplication::keyboardModifiers() == Qt::ControlModifier)) {
	slotEvaluate();
	return true;
      }
      if (mPopup != NULL && ke->key() == Qt::Key_Return &&
	  QApplication::keyboardModifiers() == Qt::AltModifier) {
	slotPopupAcceptAll();
	return true;
      }
      if (mPopup != NULL && ke->key() == Qt::Key_Escape) {
	slotPopupClose();
	return true;
      }
      if (ke->key() == Qt::Key_F9) {
	slotExpand(true);
	u->tabsOptions->setCurrentWidget(u->tabLatexImage);
	u->txtPreamble->setFocus();
	return true;
      }
    }
  }
  if (obj == u->txtLatex || obj == u->btnEvaluate) {
    if (e->type() == QEvent::KeyPress) {
      QKeyEvent *ke = (QKeyEvent*) e;
      QKeySequence seq = QKeySequence(ke->key() | ke->modifiers());
      if (seq.matches(QKeySequence::Copy)) {
	// copy key shortcut. Check if editor received the event, and if there is a selection
	// in the editor. If there is a selection, let the editor copy it to clipboard. Otherwise
	// we will copy our output
	if (/*obj == u->txtLatex &&*/ u->txtLatex->textCursor().hasSelection()) {
	  u->txtLatex->copy();
	  return true;
	} else {
	  slotCopy();
	  return true;
	}
      }
    }
  }

  // ----
  if ( obj->isWidgetType() && (e->type() == QEvent::Hide || e->type() == QEvent::Show) ) {
    // shown/hidden windows: save/restore geometry
    int k = pWindowList.indexOf(qobject_cast<QWidget*>(obj));
    if (k >= 0) {
      QWidget *w = qobject_cast<QWidget*>(obj);
      // this widget is one of our monitored top-level children.
      if (e->type() == QEvent::Hide && !((QHideEvent*)e)->spontaneous()) {
	if (pLastWindowShownStatus[w])
	  pLastWindowGeometries[w] = klf_get_window_geometry(w);
	pLastWindowShownStatus[w] = false;
      } else if (e->type() == QEvent::Show && !((QShowEvent*)e)->spontaneous()) {
	pLastWindowShownStatus[w] = true;
	klf_set_window_geometry(w, pLastWindowGeometries[w]);
      }
    }
  }
  // ----
  if ( obj == mLibBrowser && (e->type() == QEvent::Hide || e->type() == QEvent::Show) ) {
    slotLibraryButtonRefreshState(mLibBrowser->isVisible());
  }
  if ( obj == mLatexSymbols && (e->type() == QEvent::Hide || e->type() == QEvent::Show) ) {
    slotSymbolsButtonRefreshState(mLatexSymbols->isVisible());
  }

  // ----
  if ( obj == QApplication::instance() && e->type() == QEvent::FileOpen ) {
    // open a file
    openLibFile(((QFileOpenEvent*)e)->file());
    return true;
  }

  // ----
  if ( (obj == u->btnCopy || obj == u->btnDrag) && e->type() == QEvent::ToolTip ) {
    QString tooltipText;
    if (obj == u->btnCopy) {
      tooltipText = tr("Copy the formula to the clipboard. Current export profile: %1")
	.arg(KLFMimeExportProfile::findExportProfile(klfconfig.UI.copyExportProfile).description());
    } else if (obj == u->btnDrag) {
      tooltipText = tr("Click and keep mouse button pressed to drag your formula to an other application. "
		       "Current export profile: %1")
	.arg(KLFMimeExportProfile::findExportProfile(klfconfig.UI.dragExportProfile).description());
    }
    QToolTip::showText(((QHelpEvent*)e)->globalPos(), tooltipText, qobject_cast<QWidget*>(obj));
    return true;
  }

  return QWidget::eventFilter(obj, e);
}


void KLFMainWin::refreshAllWindowStyleSheets()
{
  int k;
  for (k = 0; k < pWindowList.size(); ++k)
    pWindowList[k]->setStyleSheet(pWindowList[k]->styleSheet());
  // me too!
  setStyleSheet(styleSheet());
}


QHash<QWidget*,bool> KLFMainWin::currentWindowShownStatus(bool mainWindowToo)
{
  QHash<QWidget*,bool> status;
  if (mainWindowToo)
    status[this] = isVisible();
  int k;
  for (k = 0; k < pWindowList.size(); ++k)
    status[pWindowList[k]] = pWindowList[k]->isVisible();
  return status;
}
QHash<QWidget*,bool> KLFMainWin::prepareAllWindowShownStatus(bool visibleStatus, bool mainWindowToo)
{
  QHash<QWidget*,bool> status;
  if (mainWindowToo)
    status[this] = visibleStatus;
  int k;
  for (k = 0; k < pWindowList.size(); ++k)
    status[pWindowList[k]] = visibleStatus;
  return status;
}

void KLFMainWin::setWindowShownStatus(const QHash<QWidget*,bool>& shownStatus)
{
  for (QHash<QWidget*,bool>::const_iterator it = shownStatus.begin(); it != shownStatus.end(); ++it)
    QMetaObject::invokeMethod(it.key(), "setVisible", Qt::QueuedConnection, Q_ARG(bool, it.value()));
}


void KLFMainWin::hideEvent(QHideEvent *e)
{
  if ( ! e->spontaneous() ) {
    pSavedWindowShownStatus = currentWindowShownStatus(false);
    pLastWindowGeometries[this] = klf_get_window_geometry(this);
    setWindowShownStatus(prepareAllWindowShownStatus(false, false)); // hide all windows
  }

  QWidget::hideEvent(e);
}

void KLFMainWin::showEvent(QShowEvent *e)
{
  if ( _firstshow ) {
    _firstshow = false;
    // if it's the first time we're run, 
    // show the what's new if needed.
    if ( klfconfig.Core.thisVersionMajMinFirstRun ) {
      QMetaObject::invokeMethod(this, "showWhatsNew", Qt::QueuedConnection);
    }
  }

  if ( ! e->spontaneous() ) {
    // restore shown windows ...
    if (pLastWindowGeometries.contains(this))
      klf_set_window_geometry(this, pLastWindowGeometries[this]);
    setWindowShownStatus(pSavedWindowShownStatus);
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
  case altersetting_OutlineFonts:
    _settings.outlineFonts = (bool)ivalue; break;
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


KLFLatexEdit *KLFMainWin::latexEdit()
{
  return u->txtLatex;
}
KLFLatexSyntaxHighlighter * KLFMainWin::syntaxHighlighter()
{
  return u->txtLatex->syntaxHighlighter();
}
KLFLatexSyntaxHighlighter * KLFMainWin::preambleSyntaxHighlighter()
{
  return u->txtPreamble->syntaxHighlighter();
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
  bool reallyinputchanged = mPreviewBuilderThread->inputChanged(collectInput(false));
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
  klfDbg("preview.size=" << preview.size()<< "  latexerror=" << latexerror);
  if (_evaloutput_uptodate)
    return;

  if (latexerror)
    u->lblOutput->displayError(/*labelenabled:*/false);
  else
    u->lblOutput->display(preview, preview, false);

}

KLFBackend::klfInput KLFMainWin::collectInput(bool final)
{
  // KLFBackend input
  KLFBackend::klfInput input;

  input.latex = u->txtLatex->toPlainText();
  if (u->chkMathMode->isChecked()) {
    input.mathmode = u->cbxMathMode->currentText();
    if (final && u->cbxMathMode->findText(input.mathmode) == -1) {
      u->cbxMathMode->addItem(input.mathmode);
      klfconfig.UI.customMathModes.append(input.mathmode);
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

  input = collectInput(true);

  KLFBackend::klfSettings settings = _settings;
  // see if we need to override settings
  if (u->gbxOverrideMargins->isChecked()) {
    settings.tborderoffset = u->spnMarginTop->valueInRefUnit();
    settings.rborderoffset = u->spnMarginRight->valueInRefUnit();
    settings.bborderoffset = u->spnMarginBottom->valueInRefUnit();
    settings.lborderoffset = u->spnMarginLeft->valueInRefUnit();
  }

  // and GO !
  _output = KLFBackend::getLatexFormula(input, settings);

  if (_output.status < 0) {
    QString comment = "";
    bool showSettingsPaths = false;
    if (_output.status == KLFERR_NOLATEXPROG ||
	_output.status == KLFERR_NODVIPSPROG ||
	_output.status == KLFERR_NOGSPROG ||
	_output.status == KLFERR_NOEPSTOPDFPROG) {
      comment = "\n"+tr("Are you sure you configured your system paths correctly in the settings dialog ?");
      showSettingsPaths = true;
    }
    QMessageBox::critical(this, tr("Error"), _output.errorstr+comment);
    u->lblOutput->displayClear();
    u->frmOutput->setEnabled(false);
    if (showSettingsPaths) {
      mSettingsDialog->show();
      mSettingsDialog->showControl(KLFSettings::ExecutablePaths);
    }
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
    emit evaluateFinished(_output);
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
  u->btnEvaluate->setFocus();
}

void KLFMainWin::slotClearLatex()
{
  u->txtLatex->clearLatex();
}
void KLFMainWin::slotClearAll()
{
  slotClearLatex();
  loadDefaultStyle();
}


void KLFMainWin::slotLibrary(bool showlib)
{
  klfDbg("showlib="<<showlib) ;
  mLibBrowser->setShown(showlib);
}

void KLFMainWin::slotSymbols(bool showsymbs)
{
  mLatexSymbols->setShown(showsymbs);
  slotSymbolsButtonRefreshState(showsymbs);
  mLatexSymbols->activateWindow();
  mLatexSymbols->raise();
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
void KLFMainWin::slotExpand(bool expanded)
{
  if (u->frmDetails->isVisible() == expanded)
    return; // nothing to change
  // change
  slotExpandOrShrink();
}


void KLFMainWin::slotSetLatex(const QString& latex)
{
  u->txtLatex->setLatex(latex);
}

void KLFMainWin::slotSetMathMode(const QString& mathmode)
{
  u->chkMathMode->setChecked(mathmode.simplified() != "...");
  if (mathmode.simplified() != "...")
    u->cbxMathMode->setEditText(mathmode);
}

void KLFMainWin::slotSetPreamble(const QString& preamble)
{
  u->txtPreamble->setLatex(preamble);
}

void KLFMainWin::slotEnsurePreambleCmd(const QString& line)
{
  QTextCursor c = u->txtPreamble->textCursor();
  //  qDebug("Begin preamble edit: preamble text is %s", qPrintable(u->txtPreamble->toPlainText()));
  c.beginEditBlock();
  c.movePosition(QTextCursor::End);

  QString preambletext = u->txtPreamble->toPlainText();
  if (preambletext.indexOf(line) == -1) {
    QString addtext = "";
    if (preambletext.length() > 0 &&
	preambletext[preambletext.length()-1] != '\n')
      addtext = "\n";
    addtext += line;
    c.insertText(addtext);
    preambletext += addtext;
  }

  c.endEditBlock();
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




static QString find_list_agreement(const QStringList& a, const QStringList& b)
{
  // returns the first element in a that is also in b, or a null QString() if there are no
  // common elements.
  int i, j;
  for (i = 0; i < a.size(); ++i)
    for (j = 0; j < b.size(); ++j)
      if (a[i] == b[j])
	return a[i];
  return QString();
}

bool KLFMainWin::canOpenFile(const QString& fileName)
{
  KLF_DEBUG_BLOCK(KLF_FUNC_NAME) ;
  klfDbg("file name="<<fileName) ;
  int k;
  for (k = 0; k < pDataOpeners.size(); ++k)
    if (pDataOpeners[k]->canOpenFile(fileName))
      return true;
  klfDbg("cannot open file.") ;
  return false;
}
bool KLFMainWin::canOpenData(const QByteArray& data)
{
  KLF_DEBUG_BLOCK(KLF_FUNC_NAME) ;
  klfDbg("data. length="<<data.size()) ;
  int k;
  for (k = 0; k < pDataOpeners.size(); ++k)
    if (pDataOpeners[k]->canOpenData(data))
      return true;
  klfDbg("cannot open data.") ;
  return false;
}
bool KLFMainWin::canOpenData(const QMimeData *mimeData)
{
  KLF_DEBUG_BLOCK(KLF_FUNC_NAME) ;
  QStringList formats = mimeData->formats();
  klfDbg("mime data. formats="<<formats) ;

  QString mimetype;
  int k;
  for (k = 0; k < pDataOpeners.size(); ++k) {
    mimetype = find_list_agreement(formats, pDataOpeners[k]->supportedMimeTypes());
    if (!mimetype.isEmpty())
      return true; // this opener can open the data
  }
  klfDbg("cannot open data: no appropriate opener found.") ;
  return false;
}


bool KLFMainWin::openFile(const QString& file)
{
  KLF_DEBUG_TIME_BLOCK(KLF_FUNC_NAME) ;
  klfDbg("file="<<file) ;
  int k;
  for (k = 0; k < pDataOpeners.size(); ++k)
    if (pDataOpeners[k]->openFile(file))
      return true;

  QMessageBox::critical(this, tr("Error"), tr("Failed to load file %1.").arg(file));

  return false;
}

bool KLFMainWin::openFiles(const QStringList& fileList)
{
  KLF_DEBUG_TIME_BLOCK(KLF_FUNC_NAME) ;
  klfDbg("file list="<<fileList) ;
  int k;
  bool result = true;
  for (k = 0; k < fileList.size(); ++k) {
    result = result && openFile(fileList[k]);
    klfDbg("Opened file "<<fileList[k]<<": result="<<result) ;
  }
  return result;
}

bool KLFMainWin::openData(const QMimeData *mimeData, bool *openerFound)
{
  KLF_DEBUG_TIME_BLOCK(KLF_FUNC_NAME) ;
  klfDbg("mime data, formats="<<mimeData->formats()) ;
  int k;
  QString mimetype;
  QStringList fmts = mimeData->formats();
  if (openerFound != NULL)
    *openerFound = false;
  for (k = 0; k < pDataOpeners.size(); ++k) {
    mimetype = find_list_agreement(fmts, pDataOpeners[k]->supportedMimeTypes());
    if (!mimetype.isEmpty()) {
      if (openerFound != NULL)
	*openerFound = true;
      // mime types intersect.
      klfDbg("Opening mimetype "<<mimetype) ;
      QByteArray data = mimeData->data(mimetype);
      if (pDataOpeners[k]->openData(data, mimetype))
	return true;
    }
  }

  return false;
}
bool KLFMainWin::openData(const QByteArray& data)
{
  KLF_DEBUG_TIME_BLOCK(KLF_FUNC_NAME) ;
  klfDbg("data, len="<<data.length()) ;
  int k;
  for (k = 0; k < pDataOpeners.size(); ++k)
    if (pDataOpeners[k]->openData(data, QString()))
      return true;

  return false;
}


bool KLFMainWin::openLibFiles(const QStringList& files, bool showLibrary)
{
  KLF_DEBUG_TIME_BLOCK(KLF_FUNC_NAME) ;
  int k;
  bool imported = false;
  for (k = 0; k < files.size(); ++k) {
    bool ok = openLibFile(files[k], false);
    imported = imported || ok;
    klfDbg("imported file "<<files[k]<<": imported status is now "<<imported) ;
  }
  if (showLibrary && imported)
    slotLibrary(true);
  return imported;
}

bool KLFMainWin::openLibFile(const QString& fname, bool showLibrary)
{
  KLF_DEBUG_TIME_BLOCK(KLF_FUNC_NAME) ;
  QUrl url = QUrl::fromLocalFile(fname);
  url.setScheme(KLFLibBasicWidgetFactory::guessLocalFileScheme(fname));
  QStringList subreslist = KLFLibEngineFactory::listSubResources(url);
  if ( subreslist.isEmpty() ) {
    // error reading sub-resources, or sub-resources not supported
    return mLibBrowser->openResource(url);
  }
  klfDbg("subreslist is "<<subreslist);
  bool loaded = false;
  int k;
  for (k = 0; k < subreslist.size(); ++k) {
    QUrl url2 = url;
    url2.addQueryItem("klfDefaultSubResource", subreslist[k]);
    bool thisloaded =  mLibBrowser->openResource(url2);
    loaded = loaded || thisloaded;
  }
  if (showLibrary && loaded)
    slotLibrary(true);
  return loaded;
}


void KLFMainWin::setApplicationLocale(const QString& locale)
{
  klf_reload_translations(qApp, locale);

  emit applicationLocaleChanged(locale);
}

void KLFMainWin::slotSetExportProfile(const QString& exportProfile)
{
  klfconfig.UI.copyExportProfile = exportProfile;
  klfconfig.UI.dragExportProfile = exportProfile;
  saveSettings();
}

/*
QMimeData * KLFMainWin::resultToMimeData(const QString& exportProfileName)
{
  klfDbg("export profile: "<<exportProfileName);
  if ( _output.result.isNull() )
    return NULL;

  KLFMimeExportProfile p = KLFMimeExportProfile::findExportProfile(exportProfileName);
  QStringList mimetypes = p.mimeTypes();

  QMimeData *mimedata = new QMimeData;
  int k;
  for (k = 0; k < mimetypes.size(); ++k) {
    klfDbg("exporting "<<mimetypes[k]<<" ...");
    QString mimetype = mimetypes[k];
    KLFMimeExporter *exporter = KLFMimeExporter::mimeExporterLookup(mimetype);
    if (exporter == NULL) {
      qWarning()<<KLF_FUNC_NAME<<": Can't find an exporter for mime-type "<<mimetype<<".";
      continue;
    }
    QByteArray data = exporter->data(mimetype, _output);
    mimedata->setData(mimetype, data);
    klfDbg("exporting mimetype done");
  }

  klfDbg("got mime data.");
  return mimedata;
}
*/


void KLFMainWin::slotDrag()
{
  if ( _output.result.isNull() )
    return;

  QDrag *drag = new QDrag(this);
  KLFMimeData *mime = new KLFMimeData(klfconfig.UI.dragExportProfile, _output);

  //  / ** \bug .... Temporary solution for mac os X ... * /
  //#ifdef Q_WS_MAC
  //  mime->setImageData(_output.result);
  //#endif

  drag->setMimeData(mime);
  QImage img;
  img = _output.result.scaled(QSize(200, 100), Qt::KeepAspectRatio, Qt::SmoothTransformation);
  QPixmap p = QPixmap::fromImage(img);
  drag->setPixmap(p);
  drag->setDragCursor(p, Qt::MoveAction);
  drag->setHotSpot(QPoint(p.width()/2, p.height()));
  drag->exec(Qt::CopyAction);
}

void KLFMainWin::slotCopy()
{
  //  /** \bug .... FIXME copy/drag formats on Mac OS X ... */
  //#ifdef Q_WS_MAC
  //  QApplication::clipboard()->setImage(_output.result, QClipboard::Clipboard);
  //  return;
  //#endif

#ifdef Q_WS_WIN
  extern void klfWinClipboardCopy(HWND h, const QStringList& wintypes,
				  const QList<QByteArray>& datalist);

  QString profilename = klfconfig.UI.copyExportProfile;
  KLFMimeExportProfile p = KLFMimeExportProfile::findExportProfile(profilename);

  QStringList mimetypes = p.mimeTypes();
  QStringList wintypes;
  QList<QByteArray> datalist;

  int k;
  for (k = 0; k < mimetypes.size(); ++k) {
    QString mimetype = mimetypes[k];
    QString wintype = p.respectiveWinType(k); // queries the exporter if needed
    if (wintype.isEmpty())
      wintype = mimetype;
    KLFMimeExporter *exporter = KLFMimeExporter::mimeExporterLookup(mimetype);
    if (exporter == NULL) {
      qWarning()<<KLF_FUNC_NAME<<": Can't find exporter for type "<<mimetype
	        <<", winformat="<<wintype<<".";
      continue;
    }
    QByteArray data = exporter->data(mimetype, _output);
    wintypes << wintype;
    datalist << data;
  }

  klfWinClipboardCopy(winId(), wintypes, datalist);

#else
  KLFMimeData *mimedata = new KLFMimeData(klfconfig.UI.copyExportProfile, _output);
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

  QMap<QString,QString> externFormatsByFilterName;
  QMap<QString,KLFAbstractOutputSaver*> externSaverByKey;
  int k, j;
  for (k = 0; k < pOutputSavers.size(); ++k) {
    QStringList xoformats = pOutputSavers[k]->supportedMimeFormats();
    for (j = 0; j < xoformats.size(); ++j) {
      QString f = QString("%1 (%2)").arg(pOutputSavers[k]->formatTitle(xoformats[j]),
					 pOutputSavers[k]->formatFilePatterns(xoformats[j]).join(" "));
      filterformatlist << f;
      externFormatsByFilterName[f] = xoformats[j];
      externSaverByKey[xoformats[j]] = pOutputSavers[k];
    }
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
  if (suggestion.isEmpty())
    suggestion = QDesktopServices::storageLocation(QDesktopServices::DocumentsLocation);

  fname = QFileDialog::getSaveFileName(this, tr("Save Image Formula"), suggestion, filterformat,
				       &selectedfilter);

  if (fname.isEmpty())
    return;

  // first test if it's an extern-format
  if ( externFormatsByFilterName.contains(selectedfilter) ) {
    // use an external output-saver
    QString key = externFormatsByFilterName[selectedfilter];
    if ( ! externSaverByKey.contains(key) ) {
      qWarning()<<KLF_FUNC_NAME<<": Internal error: externSaverByKey() does not contain key="<<key
		<<": "<<externSaverByKey;
      return;
    }
    KLFAbstractOutputSaver *saver = externSaverByKey[key];
    if (saver == NULL) {
      qWarning()<<KLF_FUNC_NAME<<": Internal error: saver is NULL!";
      return;
    }
    bool result = saver->saveToFile(key, fname, _output);
    if (!result)
      qWarning()<<KLF_FUNC_NAME<<": saver failed to save format "<<key<<".";
    return;
  }

  QFileInfo fi(fname);
  klfconfig.UI.lastSaveDir = fi.absolutePath();
  if ( fi.suffix().length() == 0 ) {
    // get format and suffix from selected filter
    if ( ! formatsByFilterName.contains(selectedfilter) && ! externFormatsByFilterName.contains(selectedfilter) ) {
      qWarning("%s: ERROR: Unknown format filter selected: `%s'! Assuming PNG!\n", KLF_FUNC_NAME,
	       qPrintable(selectedfilter));
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
  int index = formatlist.indexOf(fi.suffix().toLower());
  if ( index < 0 ) {
    // select PNG by default if suffix is not recognized
    QMessageBox msgbox(this);
    msgbox.setIcon(QMessageBox::Warning);
    msgbox.setWindowTitle(tr("Extension not recognized"));
    msgbox.setText(tr("Extension <b>%1</b> not recognized.").arg(fi.suffix().toUpper()));
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

  QString error;
  bool res = KLFBackend::saveOutputToFile(_output, fname, format, &error);
  if ( ! res ) {
    QMessageBox::critical(this, tr("Error"), error);
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

  if (u->gbxOverrideMargins->isChecked()) {
    sty.overrideBBoxExpand.top = u->spnMarginTop->valueInRefUnit();
    sty.overrideBBoxExpand.right = u->spnMarginRight->valueInRefUnit();
    sty.overrideBBoxExpand.bottom = u->spnMarginBottom->valueInRefUnit();
    sty.overrideBBoxExpand.left = u->spnMarginLeft->valueInRefUnit();
  }

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

  if (style.overrideBBoxExpand.valid()) {
    u->gbxOverrideMargins->setChecked(true);
    u->spnMarginTop->setValueInRefUnit(style.overrideBBoxExpand.top);
    u->spnMarginRight->setValueInRefUnit(style.overrideBBoxExpand.right);
    u->spnMarginBottom->setValueInRefUnit(style.overrideBBoxExpand.bottom);
    u->spnMarginLeft->setValueInRefUnit(style.overrideBBoxExpand.left);
  } else {
    u->gbxOverrideMargins->setChecked(false);
  }
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

  refreshStylePopupMenus();

  // auto-save our style list
  saveStyles();

  emit stylesChanged();
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




