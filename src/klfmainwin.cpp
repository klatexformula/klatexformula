/***************************************************************************
 *   file klfmainwin.cpp
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

#include <stdio.h>

#include <QtCore>
#include <QtGui>

#include <klfbackend.h>

#include <ui_klfprogerr.h>
#include <ui_klfmainwin.h>

#include <klfguiutil.h>
#include <klfrelativefont.h>
#include <klflatexedit.h>
#include <klflatexpreviewthread.h>

#include "klflibview.h"
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


static QString extract_latex_error(const QString& str)
{
  // if it was LaTeX, attempt to parse the error...
  QRegExp latexerr("\\n(\\!.*)\\?[^?]*");
  if (latexerr.indexIn(str)) {
    QString s = latexerr.cap(1);
    s.replace(QRegExp("^([^\\n]+)"), "<b>\\1</b>"); // make first line boldface
    return "<pre>"+s+"</pre>";
  }
  return str;
}



KLFProgErr::KLFProgErr(QWidget *parent, QString errtext) : QDialog(parent)
{
  u = new Ui::KLFProgErr;
  u->setupUi(this);
  setObjectName("KLFProgErr");

  setWindowFlags(Qt::Sheet);

  setWindowModality(Qt::WindowModal);

  u->txtError->setText(errtext);
}

KLFProgErr::~KLFProgErr()
{
  delete u;
}

void KLFProgErr::showEvent(QShowEvent */*e*/)
{
}

void KLFProgErr::showError(QWidget *parent, QString errtext)
{
  KLFProgErr dlg(parent, errtext);
  dlg.exec();
}



// ----------------------------------------------------------------------------


KLFMainWin::KLFMainWin()
  : QMainWindow()
{
  u = new Ui::KLFMainWin;
  u->setupUi(this);
  setObjectName("KLFMainWin");
  setAttribute(Qt::WA_StyledBackground);

  mPopup = NULL;
  //  mMacDetailsDrawer = NULL;

  loadSettings();

  _firstshow = true;

  _output.status = 0;
  _output.errorstr = QString();


  // load styless
  loadStyles();

  mLatexSymbols = new KLFLatexSymbols(this, _settings);

  u->txtLatex->setFont(klfconfig.UI.latexEditFont);
  u->txtPreamble->setFont(klfconfig.UI.preambleEditFont);

  //  u->frmOutput->setEnabled(false);
  slotSetViewControlsEnabled(false);
  slotSetSaveControlsEnabled(false);

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

  //  u->lblOutput->setLabelFixedSize(klfconfig.UI.labelOutputFixedSize);
  u->lblOutput->setEnableToolTipPreview(klfconfig.UI.enableToolTipPreview);
  klfconfig.UI.glowEffect.connectQObjectProperty(u->lblOutput, "glowEffect");
  klfconfig.UI.glowEffectColor.connectQObjectProperty(u->lblOutput, "glowEffectColor");
  klfconfig.UI.glowEffectRadius.connectQObjectProperty(u->lblOutput, "glowEffectRadius");

  connect(u->lblOutput, SIGNAL(labelDrag()), this, SLOT(slotDrag()));

  connect(u->btnShowBigPreview, SIGNAL(clicked()),
	  this, SLOT(slotShowBigPreview()));

  int h;
  h = u->btnDrag->sizeHint().height();  u->btnDrag->setFixedHeight(h - 5);
  h = u->btnCopy->sizeHint().height();  u->btnCopy->setFixedHeight(h - 5);
  h = u->btnSave->sizeHint().height();  u->btnSave->setFixedHeight(h - 5);

  QGridLayout *lyt = new QGridLayout(u->lblOutput);
  lyt->setSpacing(0);
  lyt->setMargin(0);
  mExportMsgLabel = new QLabel(u->lblOutput);
  mExportMsgLabel->setObjectName("mExportMsgLabel");
  KLFRelativeFont *exportmsglabelRelFont = new KLFRelativeFont(u->lblOutput, mExportMsgLabel);
  exportmsglabelRelFont->setRelPointSize(-1);
  mExportMsgLabel->setSizePolicy(QSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed));
  mExportMsgLabel->setMargin(1);
  mExportMsgLabel->setAlignment(Qt::AlignRight|Qt::AlignBottom);
  QPalette pal = mExportMsgLabel->palette();
  pal.setColor(QPalette::Window, QColor(180,180,180,200));
  pal.setColor(QPalette::WindowText, QColor(0,0,0,255));
  mExportMsgLabel->setPalette(pal);
  mExportMsgLabel->setAutoFillBackground(true);
  mExportMsgLabel->setProperty("defaultPalette", QVariant::fromValue<QPalette>(pal));
  //u->lyt_frmOutput->addWidget(mExportMsgLabel, 5, 0, 1, 2, Qt::AlignRight|Qt::AlignBottom);
  lyt->addItem(new QSpacerItem(1, 1, QSizePolicy::Fixed, QSizePolicy::Expanding), 0, 1, 2, 1);
  //  lyt->addItem(new QSpacerItem(1, 1, QSizePolicy::Expanding, QSizePolicy::Fixed), 1, 0, 1, 1);
  lyt->addWidget(mExportMsgLabel, 1, 0, 1, 2);

  pExportMsgLabelTimerId = -1;

  mExportMsgLabel->hide();

  //  refreshWindowSizes();

  connect(u->frmDetails, SIGNAL(sideWidgetShown(bool)), this, SLOT(slotDetailsSideWidgetShown(bool)));
#ifdef Q_WS_MAC
  klfDbg("setting up relative font...") ;
  KLFRelativeFont *rf = new KLFRelativeFont(this, u->frmDetails);
  rf->setRelPointSize(-2);
  rf->setThorough(true);
#endif

  u->frmDetails->setSideWidgetManager(klfconfig.UI.detailsSideWidgetType);
  u->frmDetails->showSideWidget(false);

  u->txtLatex->installEventFilter(this);
  u->txtLatex->setDropDataHandler(this);

  // configure syntax highlighting colors
  KLF_CONNECT_CONFIG_SH_LATEXEDIT(u->txtLatex) ;
  KLF_CONNECT_CONFIG_SH_LATEXEDIT(u->txtPreamble) ;

  u->btnEvaluate->installEventFilter(this);

  // for appropriate tooltips
  u->btnDrag->installEventFilter(this);
  u->btnCopy->installEventFilter(this);

  // Shortcut for quit
  new QShortcut(QKeySequence(tr("Ctrl+Q")), this, SLOT(quit()), SLOT(quit()),
		Qt::ApplicationShortcut);

  // Shortcut for activating editor
  //  QShortcut *editorActivatorShortcut = 
  new QShortcut(QKeySequence(Qt::Key_F4), this, SLOT(slotActivateEditor()),
		SLOT(slotActivateEditor()), Qt::ApplicationShortcut);
  //  QShortcut *editorActivatorShortcut = 
  new QShortcut(QKeySequence(Qt::Key_F4 | Qt::ShiftModifier), this, SLOT(slotActivateEditorSelectAll()),
		SLOT(slotActivateEditorSelectAll()), Qt::ApplicationShortcut);
  // shortcut for big preview
  new QShortcut(QKeySequence(Qt::Key_F2), this, SLOT(slotShowBigPreview()),
		SLOT(slotShowBigPreview()), Qt::WindowShortcut);

  // Shortcut for parens mod/type cycle
  new QShortcut(QKeySequence(Qt::CTRL + Qt::Key_BracketLeft), this, SLOT(slotCycleParenTypes()),
		SLOT(slotCycleParenTypes()), Qt::WindowShortcut);
  new QShortcut(QKeySequence(Qt::CTRL + Qt::SHIFT + Qt::Key_BracketLeft), this, SLOT(slotCycleParenModifiers()),
		SLOT(slotCycleParenModifiers()), Qt::WindowShortcut);

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
  //  connect(u->btnSaveStyle, SIGNAL(clicked()), this, SLOT(slotSaveStyle()));

  connect(u->btnQuit, SIGNAL(clicked()), this, SLOT(quit()));

  connect(mLibBrowser, SIGNAL(requestRestore(const KLFLibEntry&, uint)),
	  this, SLOT(restoreFromLibrary(const KLFLibEntry&, uint)));
  connect(mLibBrowser, SIGNAL(requestRestoreStyle(const KLFStyle&)),
	  this, SLOT(slotLoadStyle(const KLFStyle&)));
  connect(mLatexSymbols, SIGNAL(insertSymbol(const KLFLatexSymbol&)),
	  this, SLOT(insertSymbol(const KLFLatexSymbol&)));

  connect(this, SIGNAL(klfConfigChanged()), mLatexSymbols, SLOT(slotKlfConfigChanged()));


  // our help/about dialog
  connect(u->btnHelp, SIGNAL(clicked()), this, SLOT(showAbout()));

  // -- SMALL REAL-TIME PREVIEW GENERATOR THREAD --

  pLatexPreviewThread = new KLFLatexPreviewThread(this);
  //  klfconfig.UI.labelOutputFixedSize.connectQObjectProperty(pLatexPreviewThread, "previewSize");
  klfconfig.UI.previewTooltipMaxSize.connectQObjectProperty(pLatexPreviewThread, "largePreviewSize");
  pLatexPreviewThread->setInput(collectInput(false));
  pLatexPreviewThread->setSettings(_settings);

  connect(u->txtLatex, SIGNAL(insertContextMenuActions(const QPoint&, QList<QAction*> *)),
	  this, SLOT(slotEditorContextMenuInsertActions(const QPoint&, QList<QAction*> *)));
  connect(u->txtLatex, SIGNAL(textChanged()), this,
	  SLOT(updatePreviewThreadInput()), Qt::QueuedConnection);
  connect(u->cbxMathMode, SIGNAL(editTextChanged(const QString&)),
	  this, SLOT(updatePreviewThreadInput()), Qt::QueuedConnection);
  connect(u->chkMathMode, SIGNAL(stateChanged(int)), this, SLOT(updatePreviewThreadInput()),
	  Qt::QueuedConnection);
  connect(u->colFg, SIGNAL(colorChanged(const QColor&)), this, SLOT(updatePreviewThreadInput()),
	  Qt::QueuedConnection);
  //  connect(u->chkBgTransparent, SIGNAL(stateChanged(int)), this, SLOT(updatePreviewThreadInput()),
  //	  Qt::QueuedConnection);
  connect(u->colBg, SIGNAL(colorChanged(const QColor&)), this, SLOT(updatePreviewThreadInput()),
	  Qt::QueuedConnection);
  connect(u->cbxUserScript, SIGNAL(activated(const QString&)), this, SLOT(updatePreviewThreadInput()),
	  Qt::QueuedConnection);

  connect(pLatexPreviewThread, SIGNAL(previewError(const QString&, int)),
	  this, SLOT(showRealTimeError(const QString&, int)));
  connect(pLatexPreviewThread, SIGNAL(previewAvailable(const QImage&, const QImage&, const QImage&)),
	  this, SLOT(showRealTimePreview(const QImage&, const QImage&)));
  connect(pLatexPreviewThread, SIGNAL(previewReset()), this, SLOT(showRealTimeReset()));

  if (klfconfig.UI.enableRealTimePreview) {
    pLatexPreviewThread->start();
  }

  // CREATE SETTINGS DIALOG

  mSettingsDialog = new KLFSettings(this);


  // INSTALL SOME EVENT FILTERS... FOR show/hide EVENTS

  mLibBrowser->installEventFilter(this);
  mLatexSymbols->installEventFilter(this);
  mStyleManager->installEventFilter(this);
  mSettingsDialog->installEventFilter(this);


  // LOAD USER SCRIPTS

  QStringList userscripts = klf_user_scripts;
  u->cbxUserScript->clear();
  u->cbxUserScript->addItem(tr("<none>", "[[no user script]]"), QVariant(QString()));
  for (int kkl = 0; kkl < userscripts.size(); ++kkl) {
    u->cbxUserScript->addItem(QFileInfo(userscripts[kkl]).baseName(), QVariant(userscripts[kkl]));
  }


  // ADDITIONAL SETUP

#ifdef Q_WS_MAC
  // watch for QFileOpenEvent s on mac
  QApplication::instance()->installEventFilter(this);

  if (klfconfig.UI.macBrushedMetalLook) {
    // Mac Brushed Metal Look
    setAttribute(Qt::WA_MacBrushedMetal);
    //    u->txtLatex->setAttribute(Qt::WA_MacBrushedMetal);
  }

  // And add a native Mac OS X menu bar
  QMenuBar *macOSXMenu = new QMenuBar(0);
  macOSXMenu->setNativeMenuBar(true);
  // this is a virtual menu...
  QMenu *filemenu = macOSXMenu->addMenu("File");
  // ... because the 'Preferences' action is detected and is sent to the 'klatexformula' menu...
  filemenu->addAction("Preferences", this, SLOT(slotSettings()));

  // dock menu will be added once shown only, to avoid '...autoreleased without NSAutoreleasePool...'
  // annoying messages
#endif


  // INTERNAL FLAGS

  _evaloutput_uptodate = true;


  // OTHER STUFF

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

  klfDbg("Font is "<<font()<<", family is "<<fontInfo().family()) ;
  if (QFontInfo(klfconfig.UI.applicationFont).family().contains("CMU")) {
    addWhatsNewText("<p>" +
		    tr("LaTeX' Computer Modern Sans Serif font is used as the <b>default application font</b>."
		       " Don't like it? <a href=\"%1\">Choose your preferred application font</a>.")
		    .arg("klfaction:/settings?control=AppFonts") + "</p>");
  }

  // and load the library

  _loadedlibrary = true;
  loadLibrary();

  registerDataOpener(new KLFBasicDataOpener(this));
  registerDataOpener(new KLFAddOnDataOpener(this));

  // and present a cool window size
  adjustSize();
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

  if (pLatexPreviewThread)
    delete pLatexPreviewThread;

  delete u;
}

void KLFMainWin::startupFinished()
{
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
    action->setChecked( (klfconfig.ExportData.dragExportProfile()==klfconfig.ExportData.copyExportProfile()) &&
			(klfconfig.ExportData.dragExportProfile()==eplist[k].profileName()) );
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

  _settings.calcEpsBoundingBox = klfconfig.BackendSettings.calcEpsBoundingBox;
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
    klfconfig.BackendSettings.calcEpsBoundingBox = _settings.calcEpsBoundingBox;
    klfconfig.BackendSettings.outlineFonts = _settings.outlineFonts;
  }

  klfconfig.UI.userColorList = KLFColorChooser::colorList();
  klfconfig.UI.colorChooseWidgetRecent = KLFColorChooseWidget::recentColors();
  klfconfig.UI.colorChooseWidgetCustom = KLFColorChooseWidget::customColors();

  klfconfig.writeToConfig();

  pLatexPreviewThread->setSettings(_settings);

  //  u->lblOutput->setLabelFixedSize(klfconfig.UI.labelOutputFixedSize);
  u->lblOutput->setEnableToolTipPreview(klfconfig.UI.enableToolTipPreview);

  int k;
  if (klfconfig.ExportData.dragExportProfile == klfconfig.ExportData.copyExportProfile) {
    klfDbg("checking quick menu action item export profile="<<klfconfig.ExportData.copyExportProfile) ;
    // check this export profile in the quick export profile menu
    for (k = 0; k < pExportProfileQuickMenuActionList.size(); ++k) {
      if (pExportProfileQuickMenuActionList[k]->data().toString() == klfconfig.ExportData.copyExportProfile) {
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

  u->btnSetExportProfile->setEnabled(klfconfig.ExportData.menuExportProfileAffectsDrag ||
				     klfconfig.ExportData.menuExportProfileAffectsCopy);

  if (klfconfig.UI.enableRealTimePreview) {
    if ( ! pLatexPreviewThread->isRunning() ) {
      pLatexPreviewThread->start();
    }
  } else {
    if ( pLatexPreviewThread->isRunning() ) {
      pLatexPreviewThread->stop();
    }
  }

  emit klfConfigChanged();
}

void KLFMainWin::showExportMsgLabel(const QString& msg, int timeout)
{
  mExportMsgLabel->show();
  mExportMsgLabel->setPalette(mExportMsgLabel->property("defaultPalette").value<QPalette>());

  mExportMsgLabel->setProperty("timeTotal", timeout);
  mExportMsgLabel->setProperty("timeRemaining", timeout);
  mExportMsgLabel->setProperty("timeInterval", 200);

  mExportMsgLabel->setText(msg);

  if (pExportMsgLabelTimerId == -1) {
    pExportMsgLabelTimerId = startTimer(mExportMsgLabel->property("timeInterval").toInt());
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
  mStyleMenu->addAction(QIcon(":/pics/savestyle.png"), tr("Save Current Style"),
			this, SLOT(slotSaveStyle()));
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
    KLFStyle s1(tr("Default"), qRgb(0, 0, 0), qRgba(255, 255, 255, 0),
		"\\begin{align*} ... \\end{align*}", "", 600);
    //    KLFStyle s2(tr("Inline"), qRgb(0, 0, 0), qRgba(255, 255, 255, 0), "\\[ ... \\]", "", 150);
    //    s2.overrideBBoxExpand = KLFStyle::BBoxExpand(0,0,0,0);
    _styles.append(s1);
    //    _styles.append(s2);
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
  //  u->frmOutput->setEnabled(false);
  slotSetViewControlsEnabled(true);
  slotSetSaveControlsEnabled(false);
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

  // only after that actually insert the symbol, so as not to interfere with popup package suggestions.

  // if we have a selection, insert it into the symbol arguments
  QTextCursor cur = u->txtLatex->textCursor();
  QString sel = cur.selectedText(); //.replace(QChar(0x2029), "\n"); // Qt's Unicode paragraph separators->\n

  cur.beginEditBlock();

  QString insertsymbol = s.symbol;
  int nmoveleft = -1;
  if (s.symbol_option)
    insertsymbol += "[]";
  if (s.symbol_numargs >= 0) {
    int n = s.symbol_numargs;
    if (sel.size()) {
      insertsymbol += "{"+sel+"}";
      --n;
    }
    while (n--) {
      insertsymbol += "{}";
      nmoveleft += 2;
    }
  }
  cur.insertText(insertsymbol);
  if (nmoveleft > 0) {
    cur.movePosition(QTextCursor::Left, QTextCursor::MoveAnchor, nmoveleft);
  } else {
    // select char next to what we inserted, to see if we need to add a space
    QTextCursor cur2 = cur;
    cur2.movePosition(QTextCursor::Right, QTextCursor::KeepAnchor, 1);
    if (cur2.selectedText().size()) {
      int c = cur2.selectedText()[0].unicode();
      if ((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z')) {
	// have to insert space to separate from the ascii char that follows
	cur.insertText(" ");
      }
    }
  }
  cur.endEditBlock();
  u->txtLatex->setTextCursor(cur);

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

  QList<KLFLatexSymbol> syms = KLFLatexSymbolsCache::theCache()->findSymbols(symbol);
  if (syms.isEmpty())
    return;

  // choose the symbol that requires the least extra packages...
  int k, kbest = 0;
  for (k = 0; k < syms.size(); ++k) {
    if (syms[k].preamble.size() < syms[kbest].preamble.size())
      kbest = k;
  }
  KLFLatexSymbol sym = syms[kbest];

  QStringList cmds = sym.preamble;
  klfDbg("Got symbol for "<<symbol<<"; cmds is "<<cmds);
  if (cmds.isEmpty()) {
    return; // no need to include anything
  }

  QString curpreambletext = u->txtPreamble->toPlainText();
  QStringList packages;
  bool moreDefinitions = false;
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

// ---

static QStringList open_paren_modifiers;
static QStringList close_paren_modifiers;
static QStringList open_paren_types;
static QStringList close_paren_types;

static void init_paren_modifiers_and_types()
{
  if (!open_paren_modifiers.isEmpty())
    return;

  open_paren_modifiers  = KLFLatexSyntaxHighlighter::ParsedBlock::openParenModifiers;
  close_paren_modifiers = KLFLatexSyntaxHighlighter::ParsedBlock::closeParenModifiers;
  open_paren_types  = KLFLatexSyntaxHighlighter::ParsedBlock::openParenList;
  close_paren_types = KLFLatexSyntaxHighlighter::ParsedBlock::closeParenList;
  // add 'no modifier' possibility :
  open_paren_modifiers.prepend( QString() );
  close_paren_modifiers.prepend( QString() );
  // and remove "{","}" pair because that's a LaTeX argument, don't offer to change those parens
  open_paren_types.removeAll("{");
  close_paren_types.removeAll("}");
}

QVariantMap KLFMainWin::parseLatexEditPosParenInfo(KLFLatexEdit *latexEdit, int curpos)
{
  KLF_DEBUG_BLOCK(KLF_FUNC_NAME) ;

  QString text = latexEdit->toPlainText();

  KLFLatexSyntaxHighlighter *sh = latexEdit->syntaxHighlighter();
  KLF_ASSERT_NOT_NULL(sh, "syntax highlighter is NULL! ", return QVariantMap(); ) ;

  QList<KLFLatexSyntaxHighlighter::ParsedBlock> blocks =
    sh->parsedBlocksForPos(curpos, KLFLatexSyntaxHighlighter::ParsedBlock::ParenMask);
  klfDbg("got blocks: "<<blocks) ;
  int k;
  for (k = 0; k < blocks.size(); ++k) {
    KLFLatexSyntaxHighlighter::ParsedBlock block = blocks[k];

    if (block.parenIsLatexBrace())
      continue;

    QVariantMap vmap;
    vmap["has_paren"] = true;

    vmap["pos"] = block.pos;
    vmap["len"] = block.len;
    vmap["parenisopening"] = block.parenisopening;
    vmap["parenmod"] = block.parenmodifier;
    vmap["parenstr"] = block.parenstr;

    // find corresponding opening/closing paren
    QList<KLFLatexSyntaxHighlighter::ParsedBlock> otherblocks;
    if (block.parenotherpos >= 0)
      otherblocks = sh->parsedBlocksForPos(block.parenotherpos, KLFLatexSyntaxHighlighter::ParsedBlock::ParenMask);
    KLFLatexSyntaxHighlighter::ParsedBlock otherblock, openblock, closeblock;
    bool hasotherblock = false;
    for (int klj = 0; klj < otherblocks.size(); ++klj) {
      if (otherblocks[klj].parenotherpos == block.pos) { // found the other block
	otherblock = otherblocks[klj];
	hasotherblock = true;
	if (block.parenisopening) {
	  openblock = block; closeblock = otherblock;
	} else {
	  openblock = otherblock; closeblock = block;
	}
      }
    }
    if (hasotherblock) { // all ok, exists
      vmap["hasotherparen"] = true;
      vmap["otherpos"] = otherblock.pos;
      vmap["otherlen"] = otherblock.len;
      vmap["othermod"] = otherblock.parenmodifier;
      vmap["otherstr"] = otherblock.parenstr;
    }

    vmap["openparenmod"] = openblock.parenmodifier;
    vmap["openparenstr"] = openblock.parenstr;
    vmap["closeparenmod"] = closeblock.parenmodifier;
    vmap["closeparenstr"] = closeblock.parenstr;

    KLF_ASSERT_CONDITION(open_paren_modifiers.size() == close_paren_modifiers.size(),
			 "Open and Close paren modifiers list do NOT have same size !!!", return QVariantMap(); ) ;
    KLF_ASSERT_CONDITION(open_paren_types.size() == close_paren_types.size(),
			 "Open and Close paren types list do NOT have same size !!!", return QVariantMap(); ) ;

    init_paren_modifiers_and_types();

    // Qt's implicit sharing system should speed this up, this is probably not _that_ dumb...
    vmap["open_paren_modifiers"] = open_paren_modifiers;
    vmap["close_paren_modifiers"] = close_paren_modifiers;
    vmap["open_paren_types"] = open_paren_types;
    vmap["close_paren_types"] = close_paren_types;

    return vmap;
  }

  return QVariantMap();
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

  /*
    QAction *insertSymbolAction = new QAction(QIcon(":/pics/symbols.png"),
    tr("Insert Symbol ...", "[[context menu entry]]"), latexEdit);
    connect(insertSymbolAction, SIGNAL(triggered()), this, SLOT(slotSymbols()));
    *actionList << insertSymbolAction;
    */

  const QVariantMap vmap = parseLatexEditPosParenInfo(latexEdit, curpos);

  if (!vmap.isEmpty() && vmap.value("has_paren", false).toBool()) {

    QAction *amenumod = new QAction(latexEdit);
    QAction *amenutyp = new QAction(latexEdit);
    amenumod->setText(tr("Paren Modifier..."));
    amenutyp->setText(tr("Change Paren..."));
    QMenu *changemenumod = new QMenu(latexEdit);
    QMenu *changemenutyp = new QMenu(latexEdit);

    QStringList open_paren_modifiers = vmap["open_paren_modifiers"].toStringList();
    QStringList close_paren_modifiers = vmap["close_paren_modifiers"].toStringList();
    QStringList open_paren_types = vmap["open_paren_types"].toStringList();
    QStringList close_paren_types = vmap["close_paren_types"].toStringList();

    // now create our menus
    int kl;
    for (kl = 0; kl < open_paren_modifiers.size(); ++kl) {
      QString title = vmap["parenisopening"].toBool() ? open_paren_modifiers[kl] : close_paren_modifiers[kl];
      if (title.isEmpty()) {
	title = tr("<no modifier>", "[[in editor context menu]]");
      } else if (vmap["hasotherparen"].toBool()) {
	title = open_paren_modifiers[kl] + vmap["openparenstr"].toString() + " ... "
	  + close_paren_modifiers[kl] + vmap["closeparenstr"].toString();
      }
      QAction *a = changemenumod->addAction(title, this, SLOT(slotChangeParenFromActionSender()));
      QVariantMap data = vmap;
      data["newparenmod_index"] = kl;
      a->setData(data);
    }
    for (kl = 0; kl < open_paren_types.size(); ++kl) {
      QString title;
      if (vmap["hasotherparen"].toBool()) {
	title = vmap["openparenmod"].toString() + open_paren_types[kl] + " ... "
	  + vmap["closeparenmod"].toString() + close_paren_types[kl];
      } else {
	title = vmap["parenisopening"].toBool() ? open_paren_types[kl] : close_paren_types[kl];
      }
      QAction *a = changemenutyp->addAction(title, this, SLOT(slotChangeParenFromActionSender()));
      QVariantMap data = vmap;
      data["newparenstr_index"] = kl;
      a->setData(data);
    }
    amenumod->setMenu(changemenumod);
    amenutyp->setMenu(changemenutyp);
    *actionList << amenumod << amenutyp;
  }
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

void KLFMainWin::slotChangeParenFromActionSender()
{
  QAction * a = qobject_cast<QAction*>(sender());
  if (a == NULL) {
    qWarning()<<KLF_FUNC_NAME<<": NULL QAction sender!";
    return;
  }
  QVariant v = a->data();
  QVariantMap data = v.toMap();

  slotChangeParenFromVariantMap(data);
}
void KLFMainWin::slotChangeParenFromVariantMap(const QVariantMap& data)
{
  int pos = data["pos"].toInt();
  int len = data["len"].toInt();
  bool isopening = data.value("parenisopening", QVariant(false)).toBool();
  QString mod = data["parenmod"].toString();
  QString str = data["parenstr"].toString();
  QString othermod, otherstr;
  int mod_index = data.value("newparenmod_index", QVariant(-1)).toInt();
  int str_index = data.value("newparenstr_index", QVariant(-1)).toInt();

  QStringList open_paren_modifiers = data["open_paren_modifiers"].toStringList();
  QStringList close_paren_modifiers = data["close_paren_modifiers"].toStringList();
  QStringList open_paren_types = data["open_paren_types"].toStringList();
  QStringList close_paren_types = data["close_paren_types"].toStringList();

  if (mod_index >= 0) {
    mod = open_paren_modifiers[mod_index];
    othermod = close_paren_modifiers[mod_index];
    if (!isopening)
      qSwap(mod, othermod);
  }
  if (str_index >= 0) {
    str = open_paren_types[str_index];
    otherstr = close_paren_types[str_index];
    if (!isopening)
      qSwap(str, otherstr);
  }

  QString s = mod + str;
  latexEditReplace(pos, len, s);
  // and correct the 'other' pos index by a delta, in case the 'other' pos index
  // is _after_ our first replacement... (yes, indexes have been shifted by our
  // first replacement)
  int delta =  isopening ? s.length() - len : 0;

  bool hasother = data["hasotherparen"].toBool();
  if (hasother) {
    int opos = data["otherpos"].toInt() + delta;
    int olen = data["otherlen"].toInt();
    QString omod = data["othermod"].toString();
    QString ostr = data["otherstr"].toString();

    if (mod_index >= 0)
      omod = othermod;
    if (str_index >= 0)
      ostr = otherstr;

    latexEditReplace(opos, olen, omod+ostr);
  }
}

void KLFMainWin::slotCycleParenModifiers()
{
  int curpos = u->txtLatex->textCursor().position();
  const QVariantMap vmap = parseLatexEditPosParenInfo(u->txtLatex, curpos);

  if (vmap.isEmpty() || !vmap.value("has_paren", false).toBool())
    return;

  bool isopening = vmap["parenisopening"].toBool();
  QString mod = vmap["parenmod"].toString();

  const QStringList& paren_mod_list = isopening
    ? vmap["open_paren_modifiers"].toStringList()
    : vmap["close_paren_modifiers"].toStringList();

  int mod_index = paren_mod_list.indexOf(mod);
  if (mod_index < 0) {
    qWarning()<<KLF_FUNC_NAME<<": failed to find paren modifier "<<mod<<" in list "<<paren_mod_list;
    return;
  }

  QVariantMap data = vmap;
  data["newparenmod_index"] = (mod_index + 1) % paren_mod_list.size();

  slotChangeParenFromVariantMap(data);
}

void KLFMainWin::slotCycleParenTypes()
{
  int curpos = u->txtLatex->textCursor().position();
  const QVariantMap vmap = parseLatexEditPosParenInfo(u->txtLatex, curpos);

  if (vmap.isEmpty() || !vmap.value("has_paren", false).toBool())
    return;

  bool isopening = vmap["parenisopening"].toBool();
  QString str = vmap["parenstr"].toString();

  const QStringList& paren_typ_list = isopening
    ? vmap["open_paren_types"].toStringList()
    : vmap["close_paren_types"].toStringList();

  int str_index = paren_typ_list.indexOf(str);
  if (str_index < 0) {
    qWarning()<<KLF_FUNC_NAME<<": failed to find paren str "<<str<<" in list "<<paren_typ_list;
    return;
  }

  QVariantMap data = vmap;
  data["newparenstr_index"] = (str_index + 1) % paren_typ_list.size();

  slotChangeParenFromVariantMap(data);
}

void KLFMainWin::latexEditReplace(int pos, int len, const QString& text)
{
  QTextCursor c = u->txtLatex->textCursor();
  int cpos = c.position();
  // account for changed text in cpos
  if (cpos > pos + len)
    cpos += (text.length() - len);
  else if (cpos > pos)
    cpos = pos + text.length(); // end of edited text
  c.setPosition(pos);
  c.movePosition(QTextCursor::Right, QTextCursor::KeepAnchor, len);
  c.removeSelectedText();
  c.insertFragment(QTextDocumentFragment::fromPlainText(text));
  c.setPosition(cpos);
  u->txtLatex->setTextCursor(c);
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

void KLFMainWin::macHideApplication()
{
#ifdef Q_WS_MAC
  extern void klf_mac_hide_application(KLFMainWin *);
  klf_mac_hide_application(this);
#else
  qWarning()<<KLF_FUNC_NAME<<": this function is only available on Mac OS X.";
#endif
}

void KLFMainWin::quit()
{
  KLF_DEBUG_BLOCK(KLF_FUNC_NAME) ;

  u->frmDetails->showSideWidget(false);
  u->frmDetails->sideWidgetManager()->waitForShowHideActionFinished();

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

  if (e->type() == QEvent::ApplicationFontChange) {
    klfDbg("Application font change.") ;
    if (klfconfig.UI.useSystemAppFont)
      klfconfig.UI.applicationFont = QApplication::font(); // refresh the font setting...
  }
  
  return QWidget::event(e);
}

void KLFMainWin::childEvent(QChildEvent *e)
{
  KLF_DEBUG_BLOCK(KLF_FUNC_NAME) ;
  QObject *child = e->child();
  klfDbg("Child list looks like: "<<pWindowList) ;
  if (e->type() == QEvent::ChildAdded && child->isWidgetType()) {
    QWidget *w = qobject_cast<QWidget*>(child);
    if (w->windowFlags() & Qt::Window && !w->inherits("QMenu")) {
      // note that all Qt::Tool, Qt::SplashScreen etc. all have the Qt::Window bit set, but
      // not Qt::Widget (!)
      if (pWindowList.indexOf(w) == -1) // if not already present
	pWindowList.append(w);
      klfDbg( "Added child "<<w<<" ("<<w->objectName()<<")" ) ;
    }
  } else if (e->type() == QEvent::ChildRemoved && child->isWidgetType()) {
    QWidget *w = qobject_cast<QWidget*>(child);
    int k;
    if ((k = pWindowList.indexOf(w)) >= 0) {
      klfDbg( "Removing child "<<w<<" ("<<w->objectName()<<")"
	      <<" at position k="<<k ) ;
      pWindowList.removeAt(k);
    }
  }
  klfDbg("Child list now looks like: "<<pWindowList) ;

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
    openFile(((QFileOpenEvent*)e)->file());
    return true;
  }
  //  if (obj == QApplication::instance()) {
  //    klfDbg("got application event="<<e<<"; type="<<e->type());
  //  }

  // ----
  if ( (obj == u->btnCopy || obj == u->btnDrag) && e->type() == QEvent::ToolTip ) {
    QString tooltipText;
    if (obj == u->btnCopy) {
      tooltipText = tr("Copy the formula to the clipboard. Current export profile: %1")
	.arg(KLFMimeExportProfile::findExportProfile(klfconfig.ExportData.copyExportProfile).description());
    } else if (obj == u->btnDrag) {
      tooltipText = tr("Click and keep mouse button pressed to drag your formula to another application. "
		       "Current export profile: %1")
	.arg(KLFMimeExportProfile::findExportProfile(klfconfig.ExportData.dragExportProfile).description());
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
  KLF_DEBUG_BLOCK(KLF_FUNC_NAME) ;
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
  KLF_DEBUG_BLOCK(KLF_FUNC_NAME) ;
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
  KLF_DEBUG_BLOCK(KLF_FUNC_NAME) ;
  for (QHash<QWidget*,bool>::const_iterator it = shownStatus.begin(); it != shownStatus.end(); ++it) {
    klfDbg("...") ;
    klfDbg("widget: "<<it.key()) ;
    QMetaObject::invokeMethod(it.key(), "setVisible", Qt::QueuedConnection, Q_ARG(bool, it.value()));
  }
}


void KLFMainWin::hideEvent(QHideEvent *e)
{
  KLF_DEBUG_BLOCK(KLF_FUNC_NAME) ;

  if ( ! e->spontaneous() ) {
    pSavedWindowShownStatus = currentWindowShownStatus(false);
    pLastWindowGeometries[this] = klf_get_window_geometry(this);
    setWindowShownStatus(prepareAllWindowShownStatus(false, false)); // hide all windows
  }

  klfDbg("...") ;
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

#ifdef Q_WS_MAC
    // add a menu item in dock to paste from clipboard
    QMenu *adddockmenu = new QMenu;
    adddockmenu->addAction(tr("Paste Clipboard Contents", "[[Dock Menu Entry on MacOSX]]"),
			   this, SLOT(pasteLatexFromClipboard()));
    extern void qt_mac_set_dock_menu(QMenu *);
    qt_mac_set_dock_menu(adddockmenu);
#endif
  }

  if ( ! e->spontaneous() ) {
    // restore shown windows ...
    if (pLastWindowGeometries.contains(this))
      klf_set_window_geometry(this, pLastWindowGeometries[this]);
    setWindowShownStatus(pSavedWindowShownStatus);
  }

  QWidget::showEvent(e);
}


void KLFMainWin::timerEvent(QTimerEvent *e)
{
  if (e->timerId() == pExportMsgLabelTimerId) {
    int total = mExportMsgLabel->property("timeTotal").toInt();
    int remaining = mExportMsgLabel->property("timeRemaining").toInt();
    int interval = mExportMsgLabel->property("timeInterval").toInt();

    int fadepercent = (100 * remaining / total) * 3; // 0 ... 300
    if (fadepercent < 100 && fadepercent >= 0) {
      QPalette pal = mExportMsgLabel->property("defaultPalette").value<QPalette>();
      QColor c = pal.color(QPalette::Window);
      c.setAlpha(c.alpha() * fadepercent / 100);
      pal.setColor(QPalette::Window, c);
      QColor c2 = pal.color(QPalette::WindowText);
      c2.setAlpha(c2.alpha() * fadepercent / 100);
      pal.setColor(QPalette::WindowText, c2);
      mExportMsgLabel->setPalette(pal);
    }

    remaining -= interval;
    if (remaining < 0) {
      mExportMsgLabel->hide();
      killTimer(pExportMsgLabelTimerId);
      pExportMsgLabelTimerId = -1;
    } else {
      mExportMsgLabel->setProperty("timeRemaining", QVariant(remaining));
    }
  }
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
  case altersetting_CalcEpsBoundingBox:
    _settings.calcEpsBoundingBox = (bool)ivalue; break;
  case altersetting_OutlineFonts:
    _settings.outlineFonts = (bool)ivalue; break;
  default:
    qWarning()<<KLF_FUNC_NAME<<": Unknown setting to alter: "<<which<<", maybe you should give a string instead?";
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
  case altersetting_LBorderOffset:
    _settings.lborderoffset = svalue.toDouble(); break;
  case altersetting_TBorderOffset:
    _settings.tborderoffset = svalue.toDouble(); break;
  case altersetting_RBorderOffset:
    _settings.rborderoffset = svalue.toDouble(); break;
  case altersetting_BBorderOffset:
    _settings.bborderoffset = svalue.toDouble(); break;
  default:
    qWarning()<<KLF_FUNC_NAME<<": Unknown setting to alter: "<<which<<", maybe you should give an int instead?";
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


void KLFMainWin::updatePreviewThreadInput()
{
  bool changed = pLatexPreviewThread->setPreviewSize(u->lblOutput->size());
  bool inputchanged = pLatexPreviewThread->setInput(collectInput(false));
  if (changed || inputchanged) {
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

void KLFMainWin::showRealTimeReset()
{
  klfDbg("reset.") ;
  u->lblOutput->display(QImage(), QImage(), false);
  slotSetViewControlsEnabled(false);
  slotSetSaveControlsEnabled(false);
}

void KLFMainWin::showRealTimePreview(const QImage& preview, const QImage& largePreview)
{
  klfDbg("preview.size=" << preview.size()<< "  largepreview.size=" << largePreview.size());
  if (_evaloutput_uptodate) // we have clicked on 'Evaluate' button, use that image!
    return;

  u->lblOutput->display(preview, largePreview, false);
  slotSetViewControlsEnabled(true);
  slotSetSaveControlsEnabled(false);
}

void KLFMainWin::showRealTimeError(const QString& errmsg, int errcode)
{
  klfDbg("errormsg[0..99]="<<errmsg.mid(0,99)<<", code="<<errcode) ;
  QString s = errmsg;
  if (errcode == KLFERR_PROGERR_LATEX)
    s = extract_latex_error(errmsg);
  if (s.length() > 800)
    s = tr("LaTeX error, click 'Evaluate' to see error message.", "[[real-time preview tooltip]]");
  u->lblOutput->displayError(s, /*labelenabled:*/false);
  slotSetViewControlsEnabled(false);
  slotSetSaveControlsEnabled(false);
}


KLFBackend::klfInput KLFMainWin::collectInput(bool final)
{
  // KLFBackend input
  KLFBackend::klfInput input;

  input.latex = u->txtLatex->toPlainText();
  klfDbg("latex="<<input.latex) ;

  input.wrapperScript = u->cbxUserScript->itemData(u->cbxUserScript->currentIndex()).toString();

  if (u->chkMathMode->isChecked()) {
    input.mathmode = u->cbxMathMode->currentText();
    if (final && u->cbxMathMode->findText(input.mathmode) == -1) {
      u->cbxMathMode->addItem(input.mathmode);
      klfconfig.UI.customMathModes = klfconfig.UI.customMathModes() << input.mathmode;
    }
  } else {
    input.mathmode = "...";
  }
  input.preamble = u->txtPreamble->toPlainText();
  input.fg_color = u->colFg->color().rgb();
  QColor bgcolor = u->colBg->color();
  if (bgcolor.isValid())
    input.bg_color = bgcolor.rgb();
  else
    input.bg_color = qRgba(255, 255, 255, 0);
  klfDbg("input.bg_color="<<klfFmtCC("#%08lx", (ulong)input.bg_color)) ;

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
    if (_output.status == KLFERR_LATEX_NORUN ||
	_output.status == KLFERR_DVIPS_NORUN ||
	_output.status == KLFERR_GSBBOX_NORUN) {
      comment = "\n"+tr("Are you sure you configured your system paths correctly in the settings dialog ?");
      showSettingsPaths = true;
    }
    QMessageBox::critical(this, tr("Error"), _output.errorstr+comment);
    u->lblOutput->displayClear();
    //    u->frmOutput->setEnabled(false);
    slotSetViewControlsEnabled(false);
    slotSetSaveControlsEnabled(false);
    if (showSettingsPaths) {
      mSettingsDialog->show();
      mSettingsDialog->showControl(KLFSettings::ExecutablePaths);
    }
  }
  if (_output.status > 0) {
    QString s = _output.errorstr;
    if (_output.status == KLFERR_PROGERR_LATEX) {
      s = extract_latex_error(_output.errorstr);
    }
    KLFProgErr::showError(this, s);
    //    u->frmOutput->setEnabled(false);
    slotSetViewControlsEnabled(false);
    slotSetSaveControlsEnabled(false);
  }
  if (_output.status == 0) {
    // ALL OK
    _evaloutput_uptodate = true;

    QPixmap sc;
    // scale to view size (klfconfig.UI.labelOutputFixedSize)
    QSize fsize = u->lblOutput->labelSize(); //klfconfig.UI.labelOutputFixedSize;
    QImage scimg = _output.result;
    if (_output.result.width() > fsize.width() || _output.result.height() > fsize.height())
      scimg = _output.result.scaled(fsize, Qt::KeepAspectRatio, Qt::SmoothTransformation);

    sc = QPixmap::fromImage(scimg);

    QSize goodsize = _output.result.size();
    QImage tooltipimg = _output.result;
    if ( klfconfig.UI.previewTooltipMaxSize != QSize(0, 0) && // QSize(0,0) meaning no resize
	 ( tooltipimg.width() > klfconfig.UI.previewTooltipMaxSize().width() ||
	   tooltipimg.height() > klfconfig.UI.previewTooltipMaxSize().height() ) ) {
      tooltipimg =
	_output.result.scaled(klfconfig.UI.previewTooltipMaxSize, Qt::KeepAspectRatio,
			      Qt::SmoothTransformation);
    }
    emit evaluateFinished(_output);
    u->lblOutput->display(scimg, tooltipimg, true);
    //    u->frmOutput->setEnabled(true);
    slotSetViewControlsEnabled(true);
    slotSetSaveControlsEnabled(true);

    QImage smallPreview = _output.result;
    if (_output.result.width() < klfconfig.UI.smallPreviewSize().width() &&
	_output.result.height() < klfconfig.UI.smallPreviewSize().height()) {
      // OK, keep full sized image
    } else if (scimg.size() == klfconfig.UI.smallPreviewSize) {
      // use already-scaled image, don't scale it twice !
      smallPreview = scimg;
    } else {
      // scale to small-preview size
      smallPreview = _output.result.scaled(klfconfig.UI.smallPreviewSize, Qt::KeepAspectRatio,
					   Qt::SmoothTransformation);
    }

    KLFLibEntry newentry = KLFLibEntry(input.latex, QDateTime::currentDateTime(), smallPreview,
				       currentStyle());
    // check that this is not already the last entry. Perform a query to check this.
    bool needInsertThisEntry = true;
    KLFLibResourceEngine::Query query;
    query.matchCondition = KLFLib::EntryMatchCondition::mkMatchAll();
    query.skip = 0;
    query.limit = 1;
    query.orderPropId = KLFLibEntry::DateTime;
    query.orderDirection = Qt::DescendingOrder;
    query.wantedEntryProperties =
      QList<int>() << KLFLibEntry::Latex << KLFLibEntry::Style << KLFLibEntry::Category << KLFLibEntry::Tags;
    KLFLibResourceEngine::QueryResult queryResult(KLFLibResourceEngine::QueryResult::FillRawEntryList);
    int num = mHistoryLibResource->query(mHistoryLibResource->defaultSubResource(), query, &queryResult);
    if (num >= 1) {
      KLFLibEntry e = queryResult.rawEntryList[0];
      if (e.latex() == newentry.latex() &&
	  e.category() == newentry.category() &&
	  e.tags() == newentry.tags() &&
	  e.style() == newentry.style())
	needInsertThisEntry = false;
    }

    if (needInsertThisEntry) {
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
  if (showlib) {
    // already shown, raise the window
    mLibBrowser->activateWindow();
    mLibBrowser->raise();
  }
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
  slotExpand(!isExpandedMode());
}

void KLFMainWin::slotExpand(bool expanded)
{
  klfDbg("expanded="<<expanded<<"; sideWidgetVisible="<<u->frmDetails->sideWidgetVisible()) ;
  u->frmDetails->showSideWidget(expanded);
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
  klfDbg("Setting background color "<<bg) ;
  if (bg.alpha() < 100)
    u->colBg->setColor(QColor()); // transparent
  else
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
  if ( u->txtLatex->toPlainText().isEmpty() ) {
    qWarning()<<KLF_FUNC_NAME<<": Can't evaluate: empty input";
    return;
  }

  slotEvaluate();

  if ( ! output.isEmpty() ) {
    if ( _output.result.isNull() ) {
      QMessageBox::critical(this, tr("Error"), tr("There is no image to save."));
    } else {
      KLFBackend::saveOutputToFile(_output, output, format.trimmed().toUpper());
    }
  }

}


void KLFMainWin::pasteLatexFromClipboard(QClipboard::Mode mode)
{
  QString cliptext = QApplication::clipboard()->text(mode);
  slotSetLatex(cliptext);
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

int KLFMainWin::openDropData(const QMimeData *mimeData)
{
  KLF_DEBUG_TIME_BLOCK(KLF_FUNC_NAME) ;
  klfDbg("mime data, formats="<<mimeData->formats()) ;
  int k;
  QString mimetype;
  QStringList fmts = mimeData->formats();

  for (k = 0; k < pDataOpeners.size(); ++k) {
    mimetype = find_list_agreement(fmts, pDataOpeners[k]->supportedMimeTypes());
    if (!mimetype.isEmpty()) {
      // mime types intersect.
      klfDbg("Opening mimetype "<<mimetype) ;
      QByteArray data = mimeData->data(mimetype);
      if (pDataOpeners[k]->openData(data, mimetype))
	return OpenDataOk;
      return OpenDataFailed;
    }
  }

  return OpenDataCantHandle;
}
bool KLFMainWin::openData(const QMimeData *mimeData, bool *openerFound)
{
  int r = openDropData(mimeData);
  if (r == OpenDataOk) {
    if (openerFound != NULL)
      *openerFound = true;
    return true;
  }
  if (r == OpenDataCantHandle) {
    if (openerFound != NULL)
      *openerFound = false;
    return false;
  }
  //  if (r == OpenDataFailed) { // only remaining case
  if (openerFound != NULL)
    *openerFound = true;
  return false;
  //  }
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
  klfDbg("fname="<<fname<<"; showLibrary="<<showLibrary) ;
  QUrl url = QUrl::fromLocalFile(fname);
  QString scheme = KLFLibBasicWidgetFactory::guessLocalFileScheme(fname);
  if (scheme.isEmpty()) {
    klfDbg("Can't guess scheme for file "<<fname) ;
    return false;
  }
  url.setScheme(scheme);
  klfDbg("url to open is "<<url) ;
  QStringList subreslist = KLFLibEngineFactory::listSubResources(url);
  if ( subreslist.isEmpty() ) {
    klfDbg("no sub-resources...") ;
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
  KLF_DEBUG_BLOCK(KLF_FUNC_NAME) ;

  klf_reload_translations(qApp, locale);

  emit applicationLocaleChanged(locale);
}

void KLFMainWin::slotSetExportProfile(const QString& exportProfile)
{
  if (klfconfig.ExportData.menuExportProfileAffectsCopy)
    klfconfig.ExportData.copyExportProfile = exportProfile;
  if (klfconfig.ExportData.menuExportProfileAffectsDrag)
    klfconfig.ExportData.dragExportProfile = exportProfile;
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
  KLFMimeData *mime = new KLFMimeData(klfconfig.ExportData.dragExportProfile, _output);

  drag->setMimeData(mime);

  QSize sz = QSize(200, 100);
  QImage img = _output.result;
  if (img.width() > sz.width() || img.height() > sz.height())
    img = img.scaled(sz, Qt::KeepAspectRatio, Qt::SmoothTransformation);
  // imprint the export type on the drag pixmap
  QString exportProfileText = KLFMimeExportProfile::findExportProfile(klfconfig.ExportData.dragExportProfile).description();
  { QPainter painter(&img);
    QFont smallfont = QFont("Helvetica", 6);
    smallfont.setPixelSize(12);
    painter.setFont(smallfont);
    painter.setRenderHint(QPainter::TextAntialiasing, false);
    QRect rall = QRect(QPoint(0,0), img.size());
    QRect bb = painter.boundingRect(rall, Qt::AlignBottom|Qt::AlignRight, exportProfileText);
    klfDbg("BB is "<<bb) ;
    painter.setPen(QPen(QColor(255,255,255,0), 0, Qt::NoPen));
    painter.fillRect(bb, QColor(150,150,150,128));
    painter.setPen(QPen(QColor(0,0,0), 1, Qt::SolidLine));
    painter.drawText(rall, Qt::AlignBottom|Qt::AlignRight, exportProfileText);
    /*
    QRect bb = painter.fontMetrics().boundingRect(exportProfileText);
    QPoint translate = QPoint(img.width(), img.height()) - bb.bottomRight();
    painter.setPen(QPen(QColor(255,255,255,0), 0, Qt::NoPen));
    painter.fillRect(translate.x(), translate.y(), bb.width(), bb.height(), QColor(128,128,128,128));
    painter.setPen(QPen(QColor(0,0,0), 1, Qt::SolidLine));
    painter.drawText(translate, exportProfileText);*/
  }
  QPixmap p = QPixmap::fromImage(img);
  drag->setPixmap(p);
  drag->setDragCursor(p, Qt::MoveAction);
  drag->setHotSpot(QPoint(p.width()/2, p.height()));
  drag->exec(Qt::CopyAction);
}

void KLFMainWin::slotCopy()
{
#ifdef Q_WS_WIN
  extern void klfWinClipboardCopy(HWND h, const QStringList& wintypes,
				  const QList<QByteArray>& datalist);

  QString profilename = klfconfig.ExportData.copyExportProfile;
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
  KLFMimeData *mimedata = new KLFMimeData(klfconfig.ExportData.copyExportProfile, _output);
  QApplication::clipboard()->setMimeData(mimedata, QClipboard::Clipboard);
#endif

  KLFMimeExportProfile profile = KLFMimeExportProfile::findExportProfile(klfconfig.ExportData.copyExportProfile);
  showExportMsgLabel(tr("Copied as <b>%1</b>").arg(profile.description()));
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
  s = tr("LaTeX DVI (*.dvi)");
  filterformatlist.push_front(s);
  formatlist.push_front("dvi");
  formatsByFilterName[s] = "dvi";
  s = tr("EPS PostScript (*.eps)");
  filterformatlist.push_front(s);
  formatlist.push_front("eps");
  formatsByFilterName[s] = "eps";
  s = tr("Scalable Vector Graphics SVG (*.svg)");
  filterformatlist.push_front(s);
  formatlist.push_front("svg");
  formatsByFilterName[s] = "svg";
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

void KLFMainWin::slotActivateEditorSelectAll()
{
  slotActivateEditor();
  u->txtLatex->selectAll();
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


bool KLFMainWin::isExpandedMode() const
{
  return u->frmDetails->sideWidgetVisible();
}

KLFStyle KLFMainWin::currentStyle() const
{
  KLFStyle sty;

  sty.name = QString::null;
  sty.fg_color = u->colFg->color().rgb();
  QColor bgc = u->colBg->color();
  if (bgc.isValid())
    sty.bg_color = qRgba(bgc.red(), bgc.green(), bgc.blue(), 255);
  else
    sty.bg_color = qRgba(255, 255, 255, 0);
  sty.mathmode = (u->chkMathMode->isChecked() ? u->cbxMathMode->currentText() : "...");
  sty.preamble = u->txtPreamble->toPlainText();
  sty.dpi = u->spnDPI->value();

  if (u->gbxOverrideMargins->isChecked()) {
    sty.overrideBBoxExpand.top = u->spnMarginTop->valueInRefUnit();
    sty.overrideBBoxExpand.right = u->spnMarginRight->valueInRefUnit();
    sty.overrideBBoxExpand.bottom = u->spnMarginBottom->valueInRefUnit();
    sty.overrideBBoxExpand.left = u->spnMarginLeft->valueInRefUnit();
  }

  klfDbg("Returning style; bgcol="<<klfFmtCC("#%08lx", (ulong)sty.bg_color)) ;

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

void KLFMainWin::slotDetailsSideWidgetShown(bool shown)
{
  if (u->frmDetails->sideWidgetManagerType() == QLatin1String("ShowHide")) {
    if (shown)
      u->btnExpand->setIcon(QIcon(":/pics/switchshrinked.png"));
    else
      u->btnExpand->setIcon(QIcon(":/pics/switchexpanded.png"));
  } else if (u->frmDetails->sideWidgetManagerType() == QLatin1String("Drawer")) {
    if (shown)
      u->btnExpand->setIcon(QIcon(":/pics/switchshrinked_drawer.png"));
    else
      u->btnExpand->setIcon(QIcon(":/pics/switchexpanded_drawer.png"));
  } else {
    // "Float", or any possible custom side widget type
    if (shown)
      u->btnExpand->setIcon(QIcon(":/pics/hidetoolwindow.png"));
    else
      u->btnExpand->setIcon(QIcon(":/pics/showtoolwindow.png"));
  }
}

void KLFMainWin::slotLoadStyle(const KLFStyle& style)
{
  QColor cfg, cbg;
  cfg.setRgba(style.fg_color);
  cbg.setRgba(style.bg_color);
  u->colFg->setColor(cfg);
  if (cbg.alpha() < 100)
    cbg = QColor(); // transparent
  klfDbg("setting color: "<<cbg) ;
  u->colBg->setColor(cbg);
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


void KLFMainWin::slotSetViewControlsEnabled(bool enabled)
{
  u->btnShowBigPreview->setEnabled(enabled);
}

void KLFMainWin::slotSetSaveControlsEnabled(bool enabled)
{
  u->btnSetExportProfile->setEnabled(enabled);
  u->btnDrag->setEnabled(enabled);
  u->btnCopy->setEnabled(enabled);
  u->btnSave->setEnabled(enabled);
}


void KLFMainWin::closeEvent(QCloseEvent *event)
{
  KLF_DEBUG_BLOCK(KLF_FUNC_NAME) ;

  // close side widget and wait for it
  u->frmDetails->showSideWidget(false);
  u->frmDetails->sideWidgetManager()->waitForShowHideActionFinished();

  if (_ignore_close_event) {
    // simple hide, not close
    hide();

    event->ignore();
    return;
  }

  event->accept();
  quit();
}




