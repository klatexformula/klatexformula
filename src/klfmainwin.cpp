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
#include <math.h>

#include <QtCore>
#include <QtGui>
#include <QtWidgets>
#include <QUiLoader>

#include <ui_klfmainwin.h>

#include <klfprogerr.h>
#include <klfutil.h>
#include <klfguiutil.h>
#include <klfrelativefont.h>
#include <klflatexedit.h>
#include <klflatexpreviewthread.h>
#include <klfsysinfo.h>

#include <klfuserscript.h>
#include <klfbackend.h>

#ifdef KLF_USE_SPARKLE
#include "macosx/klfsparkleupdater.h"
#endif
#ifdef KLF_USE_WINSPARKLE
#include "mswin/klfwinsparkleupdater.h"
#endif

#include "klflibview.h"
#include "klflibbrowser.h"
#include "klflatexsymbols.h"
#include "klfsettings.h"
#include "klfmain.h"
#include "klfstylemanager.h"
#include "klfmime.h"
#include "klfcmdiface.h"
#include "klfuiloader.h"
#include "klfexporter.h"
#include "klfexporter_p.h"

#include "klfmainwin.h"
#include "klfmainwin_p.h"






KLFMainWin::KLFMainWin()
  : QMainWindow()
{
  KLF_DEBUG_TIME_BLOCK(KLF_FUNC_NAME) ;

  KLF_INIT_PRIVATE(KLFMainWin) ;

  u = new Ui::KLFMainWin;
  u->setupUi(this);
  setObjectName("KLFMainWin");
  setAttribute(Qt::WA_StyledBackground);

  d->mPopup = NULL;

  loadSettings();

  d->firstshow = true;

  d->output.status = 0;
  d->output.errorstr = QString();

  d->pCmdIface = new KLFCmdIface(this);
  d->pCmdIface->registerObject(this, QLatin1String("klfmainwin.klfapp.klf"));

  setProperty("defaultPalette", QVariant::fromValue<QPalette>(palette()));

  // try hiding the prompt label -- it takes up space and isn't useful
  u->lblPromptMain->setVisible(false);

  // load styless
  loadStyles();

  klfDbg("Instantiating LaTeX symbols") ;
  d->mLatexSymbols = new KLFLatexSymbols(this, d->settings);

  u->txtLatex->setFont(klfconfig.UI.latexEditFont);
  u->txtPreamble->setFont(klfconfig.UI.preambleEditFont);

  u->txtLatex->setWrapLines(klfconfig.UI.editorWrapLines);
  u->txtLatex->setTabChangesFocus(!klfconfig.UI.editorTabInsertsTab);
  u->txtPreamble->setWrapLines(klfconfig.UI.editorWrapLines);
  u->txtPreamble->setTabChangesFocus(!klfconfig.UI.editorTabInsertsTab);

  d->slotSetViewControlsEnabled(false);
  d->slotSetSaveControlsEnabled(false);

  QMenu *DPIPresets = new QMenu(this);
  // 1200 DPI
  connect(u->aDPI1200, SIGNAL(triggered()), d, SLOT(slotPresetDPISender()));
  u->aDPI1200->setData(1200);
  DPIPresets->addAction(u->aDPI1200);
  // 600 DPI
  connect(u->aDPI600, SIGNAL(triggered()), d, SLOT(slotPresetDPISender()));
  u->aDPI600->setData(600);
  DPIPresets->addAction(u->aDPI600);
  // 300 DPI
  connect(u->aDPI300, SIGNAL(triggered()), d, SLOT(slotPresetDPISender()));
  u->aDPI300->setData(300);
  DPIPresets->addAction(u->aDPI300);
  // 150 DPI
  connect(u->aDPI150, SIGNAL(triggered()), d, SLOT(slotPresetDPISender()));
  u->aDPI150->setData(150);
  DPIPresets->addAction(u->aDPI150);
  // set menu to the button
  u->btnDPIPresets->setMenu(DPIPresets);


  klfDbg("Setting up LaTeX fonts") ;
  // latex fonts
  d->reloadLatexFontDefs();
  u->cbxLatexFont->addItem(tr("Computer Modern (default font)"), QVariant(QString()));
  int k;
  for (k = 0; k < d->pLatexFontDefs.size(); ++k) {
    u->cbxLatexFont->addItem(d->pLatexFontDefs[k].title, QVariant(d->pLatexFontDefs[k].identifier));
  }
  // no idea why this seems to be needed to get text on combo box to display:
  u->cbxLatexFont->setPalette(QPalette(Qt::white));
  u->cbxUserScript->setPalette(QPalette(Qt::white));
  u->cbxMarginsUnit->setPalette(QPalette(Qt::white));


  connect(u->txtLatex->syntaxHighlighter(), SIGNAL(newSymbolTyped(const QString&)),
	  d, SLOT(slotNewSymbolTyped(const QString&)));

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
  d->mExportMsgLabel = new QLabel(u->lblOutput);
  d->mExportMsgLabel->setObjectName("mExportMsgLabel");
  KLFRelativeFont *exportmsglabelRelFont = new KLFRelativeFont(u->lblOutput, d->mExportMsgLabel);
  exportmsglabelRelFont->setRelPointSize(-1);
  d->mExportMsgLabel->setSizePolicy(QSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed));
  d->mExportMsgLabel->setMargin(1);
  d->mExportMsgLabel->setAlignment(Qt::AlignRight|Qt::AlignBottom);
  QPalette pal = d->mExportMsgLabel->palette();
  pal.setColor(QPalette::Window, QColor(180,180,180,200));
  pal.setColor(QPalette::WindowText, QColor(0,0,0,255));
  d->mExportMsgLabel->setPalette(pal);
  d->mExportMsgLabel->setAutoFillBackground(true);
  d->mExportMsgLabel->setProperty("defaultPalette", QVariant::fromValue<QPalette>(pal));
  //u->lyt_frmOutput->addWidget(mExportMsgLabel, 5, 0, 1, 2, Qt::AlignRight|Qt::AlignBottom);
  lyt->addItem(new QSpacerItem(1, 1, QSizePolicy::Fixed, QSizePolicy::Expanding), 0, 1, 2, 1);
  //  lyt->addItem(new QSpacerItem(1, 1, QSizePolicy::Expanding, QSizePolicy::Fixed), 1, 0, 1, 1);
  lyt->addWidget(d->mExportMsgLabel, 1, 0, 1, 2);

  d->pExportMsgLabelTimerId = -1;

  d->mExportMsgLabel->hide();

  connect(u->frmDetails, SIGNAL(sideWidgetShown(bool)), d, SLOT(slotDetailsSideWidgetShown(bool)));
  klfDbg("setting up relative font...") ;
  KLFRelativeFont *rf = new KLFRelativeFont(this, u->frmDetails);
#ifdef KLF_WS_MAC
  rf->setRelPointSize(-2);
  rf->setThorough(true);
#else
  rf->setRelPointSize(-1);
#endif

  //  u->frmDetails->setSideWidgetManager(klfconfig.UI.detailsSideWidgetType);
  klfconfig.UI.detailsSideWidgetType.connectQObjectProperty(u->frmDetails, "sideWidgetManagerType");
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
  d->mShortcutNextParenType =
    new QShortcut(QKeySequence(Qt::CTRL + Qt::Key_BracketLeft), this, SLOT(slotCycleParenTypes()),
		  SLOT(slotCycleParenTypes()), Qt::WindowShortcut);
  new QShortcut(QKeySequence(Qt::CTRL + Qt::Key_BracketRight), this, SLOT(slotCycleParenTypesBack()),
		SLOT(slotCycleParenTypesBack()), Qt::WindowShortcut);
  d->mShortcutNextParenModifierType =
    new QShortcut(QKeySequence(Qt::CTRL + Qt::SHIFT + Qt::Key_BracketLeft), this, SLOT(slotCycleParenModifiers()),
		  SLOT(slotCycleParenModifiers()), Qt::WindowShortcut);
  new QShortcut(QKeySequence(Qt::CTRL + Qt::SHIFT + Qt::Key_BracketRight), this, SLOT(slotCycleParenModifiersBack()),
		SLOT(slotCycleParenModifiersBack()), Qt::WindowShortcut);

  // Create our style manager
  d->mStyleManager = new KLFStyleManager(& d->styles, this);
  connect(d->mStyleManager, SIGNAL(refreshStyles()), d, SLOT(refreshStylePopupMenus()));
  connect(this, SIGNAL(stylesChanged()), d->mStyleManager, SLOT(slotRefresh()));

  connect(this, SIGNAL(stylesChanged()), this, SLOT(saveStyles()));
  connect(d->mStyleManager, SIGNAL(refreshStyles()), this, SLOT(saveStyles()));

  loadDefaultStyle();

  // initialize the margin unit selector
  u->cbxMarginsUnit->setCurrentUnitAbbrev("pt");

  // set custom math modes
  u->cbxMathMode->addItems(klfconfig.UI.customMathModes);

  // load library
  d->mLibBrowser = new KLFLibBrowser(this);
  //#ifdef KLF_WS_MAC
  //   // library browser relative font
  //   KLFRelativeFont *rf_libbrowser = new KLFRelativeFont(this, mLibBrowser);
  //   rf_libbrowser->setRelPointSize(-2);
  //   rf_libbrowser->setThorough(true);
  // #endif

  d->refreshShowCorrectClearButton();

  // -- MAJOR SIGNAL/SLOT CONNECTIONS --

  klfDbg("Setting up some major signal/slot connections") ;

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

  connect(u->btnQuit, SIGNAL(clicked()), this, SLOT(quit()));

  connect(d->mLibBrowser, SIGNAL(requestRestore(const KLFLibEntry&, uint)),
	  this, SLOT(restoreFromLibrary(const KLFLibEntry&, uint)));
  connect(d->mLibBrowser, SIGNAL(requestRestoreStyle(const KLFStyle&)),
	  this, SLOT(slotLoadStyle(const KLFStyle&)));
  connect(d->mLatexSymbols, SIGNAL(insertSymbol(const KLFLatexSymbol&)),
	  this, SLOT(insertSymbol(const KLFLatexSymbol&)));

  connect(this, SIGNAL(klfConfigChanged()), d->mLatexSymbols, SLOT(slotKlfConfigChanged()));

  connect(u->cbxUserScript, SIGNAL(activated(int)), d, SLOT(slotUserScriptSet(int)));

  connect(u->btnUserScriptReload, SIGNAL(clicked()), this, SLOT(slotReloadUserScripts()));
  connect(u->btnShowUserScriptLog, SIGNAL(clicked()), this, SLOT(slotShowUserScriptLog()));
  connect(u->btnUserScriptInfo, SIGNAL(clicked()), d, SLOT(slotUserScriptShowInfo()));

  connect(this, SIGNAL(userInputChanged()), this, SIGNAL(userActivity()));

  // our help/about dialog
  connect(u->btnHelp, SIGNAL(clicked()), this, SLOT(showAbout()));


  // SMALL REAL-TIME PREVIEW GENERATOR THREAD

  klfDbg("Setting up real-time preview generator thread") ;
  d->pLatexPreviewThread = new KLFLatexPreviewThread(this);
  d->pContLatexPreview = new KLFContLatexPreview(d->pLatexPreviewThread);
  //  klfconfig.UI.labelOutputFixedSize.connectQObjectProperty(pLatexPreviewThread, "previewSize");
  klfconfig.UI.previewTooltipMaxSize.connectQObjectProperty(d->pContLatexPreview, "largePreviewSize");
  d->pContLatexPreview->setInput(d->collectInput(false));
  d->pContLatexPreview->setSettings(currentSettings());

  klfDbg("about to set up more connections ...") ;

  connect(u->txtLatex, SIGNAL(insertContextMenuActions(const QPoint&, QList<QAction*> *)),
	  d, SLOT(slotEditorContextMenuInsertActions(const QPoint&, QList<QAction*> *)));
  connect(u->txtLatex, SIGNAL(textChanged()), d,
	  SLOT(updatePreviewThreadInput()), Qt::QueuedConnection);
  connect(u->cbxMathMode, SIGNAL(editTextChanged(const QString&)),
	  d, SLOT(updatePreviewThreadInput()), Qt::QueuedConnection);
  connect(u->chkMathMode, SIGNAL(stateChanged(int)), d, SLOT(updatePreviewThreadInput()),
	  Qt::QueuedConnection);
  connect(u->colFg, SIGNAL(colorChanged(const QColor&)), d, SLOT(updatePreviewThreadInput()),
	  Qt::QueuedConnection);
  //  connect(u->chkBgTransparent, SIGNAL(stateChanged(int)), d, SLOT(updatePreviewThreadInput()),
  //	  Qt::QueuedConnection);
  connect(u->colBg, SIGNAL(colorChanged(const QColor&)), d, SLOT(updatePreviewThreadInput()),
	  Qt::QueuedConnection);
  connect(u->cbxUserScript, SIGNAL(activated(const QString&)), d, SLOT(updatePreviewThreadInput()),
	  Qt::QueuedConnection);
  connect(u->cbxLatexFont, SIGNAL(activated(const QString&)), d, SLOT(updatePreviewThreadInput()),
	  Qt::QueuedConnection);
  connect(u->spnLatexFontSize, SIGNAL(valueChanged(int)), d, SLOT(updatePreviewThreadInput()),
	  Qt::QueuedConnection);
  connect(u->txtPreamble, SIGNAL(textChanged()), d, SLOT(updatePreviewThreadInput()),
	  Qt::QueuedConnection);
  connect(u->spnDPI, SIGNAL(valueChanged(int)), d, SLOT(updatePreviewThreadInput()),
	  Qt::QueuedConnection);
  connect(u->spnVectorScale, SIGNAL(valueChanged(double)), d, SLOT(updatePreviewThreadInput()),
	  Qt::QueuedConnection);
  connect(u->gbxOverrideMargins, SIGNAL(toggled(bool)), d, SLOT(updatePreviewThreadSettings()),
	  Qt::QueuedConnection);
  connect(u->spnMarginTop, SIGNAL(valueChanged(double)), d, SLOT(updatePreviewThreadSettings()),
	  Qt::QueuedConnection);
  connect(u->spnMarginRight, SIGNAL(valueChanged(double)), d, SLOT(updatePreviewThreadSettings()),
	  Qt::QueuedConnection);
  connect(u->spnMarginBottom, SIGNAL(valueChanged(double)), d, SLOT(updatePreviewThreadSettings()),
	  Qt::QueuedConnection);
  connect(u->spnMarginLeft, SIGNAL(valueChanged(double)), d, SLOT(updatePreviewThreadSettings()),
	  Qt::QueuedConnection);
  connect(u->cbxUserScript, SIGNAL(activated(const QString&)), d, SLOT(updatePreviewThreadSettings()),
	  Qt::QueuedConnection);

  connect(d->pContLatexPreview, SIGNAL(previewError(const QString&, int)),
	  d, SLOT(showRealTimeError(const QString&, int)));
  connect(d->pContLatexPreview, SIGNAL(previewAvailable(const QImage&, const QImage&, const QImage&)),
	  d, SLOT(showRealTimePreview(const QImage&, const QImage&)));
  connect(d->pContLatexPreview, SIGNAL(previewReset()), d, SLOT(showRealTimeReset()));


  // MIME EXPORT PROFILES

  klfDbg("Loading mime export profiles") ;

  // the KLFMimeExportProfileManager is also responsible for instantiating appropriate
  // QMacPasteboardMime, QWinClipboard classes
  d->pMimeExportProfileManager.addExportProfiles(
      KLFMimeExportProfileManager::loadKlfExportProfileList()
      );

  // LOAD USER SCRIPTS

  klfDbg("Loading user scripts") ;

  slotReloadUserScripts();

  // and set up the default config, if the user script can provide a default config
  klfDbg("About to load default settings for user scripts, if necessary.  Currently user script config = "
         << klfconfig.UserScripts.userScriptConfig) ;
  foreach( QString us, klf_user_scripts) {
    klfDbg("Considering user script " << us) ;
    if (!klfconfig.UserScripts.userScriptConfig.contains(us) ||
        klfconfig.UserScripts.userScriptConfig.value(us).isEmpty()) {
      KLFUserScriptInfo usinfo(us);
      if (usinfo.canProvideDefaultSettings()) {
        // set defaults
        klfDbg("setting default config for user script " << us) ;
        klfconfig.UserScripts.userScriptConfig[usinfo.userScriptPath()] = usinfo.queryDefaultSettings(&d->settings);
        klfDbg("user script config is now " << klfconfig.UserScripts.userScriptConfig[usinfo.userScriptPath()]) ;
      }
    }
  }


  // REGISTER OUR EXPORTERS

  d->pExporterManager = new KLFExporterManager;

  klfDbg("Registering exporters") ;

  registerExporter(new KLFBackendFormatsExporter(this));
  registerExporter(new KLFTexExporter(this));
  registerExporter(new KLFHtmlDataExporter(this));
  registerExporter(new KLFOpenOfficeDrawExporter(this));
  registerExporter(new KLFTempFileUriExporter(this));
  registerExporter(new KLFTempImgRefHtmlExporter(this));

  // now, create one instance of KLFUserScriptExporter per export type user script ...
  extern QStringList klf_user_scripts;
  for (int k = 0; k < klf_user_scripts.size(); ++k) {
    if (KLFUserScriptInfo(klf_user_scripts[k]).category() == QLatin1String("klf-export-type")) {
      QString usname = klf_user_scripts[k];
      klfDbg("Loading exporter for user script " << usname) ;
      KLFExportTypeUserScriptInfo usinfo(usname);
      registerExporter(new KLFUserScriptExporter(usname, this));
      // add any corresponding export profiles, as well.
      QString profilesXmlFile = usinfo.mimeExportProfilesXmlFile();
      klfDbg("profilesXmlFile = " << profilesXmlFile) ;
      if (profilesXmlFile.size()) {
        klfDbg("Loading profiles from XML file " << profilesXmlFile) ;
        d->pMimeExportProfileManager.addExportProfiles(
            KLFMimeExportProfile::loadProfilesFromXmlFile(usinfo.relativeFile(profilesXmlFile))
            );
      }
    }
  }


  // REGISTER PLATFORM-SPECIFIC CLIPBOARD CONVERTERS

#if defined(KLF_WS_MAC)
  d->macFlavorsConverter = new KLFMacPasteboardMime;
#elif defined(KLF_WS_WIN)
  d->winFormatsConverter = new KLFWinClipboard;
#else
  // no special needs on X11
#endif

  // CREATE SETTINGS DIALOG

  d->mSettingsDialog = new KLFSettings(this);


  // INSTALL SOME EVENT FILTERS... FOR show/hide EVENTS

  klfDbg("Installing some event filters") ;

  d->mLibBrowser->installEventFilter(this);
  d->mLatexSymbols->installEventFilter(this);
  d->mStyleManager->installEventFilter(this);
  d->mSettingsDialog->installEventFilter(this);


  // ADDITIONAL SETUP

#ifdef KLF_WS_MAC
  klfDbg("Mac OS X main win features setup") ;

  // watch for QFileOpenEvent s on mac
  QApplication::instance()->installEventFilter(this);

  klfconfig.UI.macBrushedMetalLook.connectQObjectSlot(this, "setMacBrushedMetalLook");

  // And add a native Mac OS X menu bar
  d->pMacOSXMenu = new QMenuBar(0);
  d->pMacOSXMenu->setNativeMenuBar(true);
  // this is a virtual menu...
  QMenu *filemenu = d->pMacOSXMenu->addMenu("File");
  // ... because the 'Preferences' action is detected and is sent to the 'klatexformula' menu...
  // (and don't translate it, Qt will replace it by the default mac 'preferences' menu item)
  filemenu->addAction("Preferences", this, SLOT(slotSettings()));
  filemenu->addAction("About", this, SLOT(showAbout()));
  d->pCheckForUpdatesAction = new QAction(tr("Check for Updates"), this);
  d->pCheckForUpdatesAction->setMenuRole(QAction::ApplicationSpecificRole);
  filemenu->addAction(d->pCheckForUpdatesAction);

  QMenu *shortmenu = d->pMacOSXMenu->addMenu("Shortcuts");
  // remember, on Qt/Mac Ctrl key is mapped to Command
  shortmenu->addAction(tr("LaTeX Symbols Palette"), this, SLOT(slotToggleSymbols()), QKeySequence("Ctrl+1"));
  shortmenu->addAction(tr("Equation Library Browser"), this, SLOT(slotToggleLibrary()), QKeySequence("Ctrl+2"));
  shortmenu->addAction(tr("Show Details"), this, SLOT(slotExpandOrShrink()), QKeySequence("Ctrl+D"));
  shortmenu->addAction(tr("Activate Editor"), this, SLOT(slotActivateEditor()), QKeySequence("Ctrl+L"));
  shortmenu->addAction(tr("Activate Editor and Select All"), this, SLOT(slotActivateEditorSelectAll()),
		       QKeySequence("Ctrl+Shift+L"));
  shortmenu->addAction(tr("Show Big Preview"), this, SLOT(slotShowBigPreview()), QKeySequence("Ctrl+P"));

  // Shortcut for parens mod/type cycle
  //   new QShortcut(QKeySequence(Qt::CTRL + Qt::Key_BracketLeft), this, SLOT(slotCycleParenTypes()),
  // 		SLOT(slotCycleParenTypes()), Qt::WindowShortcut);
  //   new QShortcut(QKeySequence(Qt::CTRL + Qt::SHIFT + Qt::Key_BracketLeft), this, SLOT(slotCycleParenModifiers()),
  // 		SLOT(slotCycleParenModifiers()), Qt::WindowShortcut);
  

  // dock menu will be added once shown only, to avoid '...autoreleased without NSAutoreleasePool...'
  // annoying messages
#else
  d->pMacOSXMenu = NULL;
  d->pCheckForUpdatesAction = NULL;
#endif


  // INTERNAL FLAGS

  d->evaloutput_uptodate = true;


  // OTHER STUFF

  retranslateUi(false);

  connect(this, SIGNAL(applicationLocaleChanged(const QString&)), this, SLOT(retranslateUi()));
  connect(this, SIGNAL(applicationLocaleChanged(const QString&)), d->mLibBrowser, SLOT(retranslateUi()));
  connect(this, SIGNAL(applicationLocaleChanged(const QString&)), d->mSettingsDialog, SLOT(retranslateUi()));
  connect(this, SIGNAL(applicationLocaleChanged(const QString&)), d->mStyleManager, SLOT(retranslateUi()));

  // About Dialog & What's New dialog
  d->mAboutDialog = new KLFAboutDialog(this);
  d->mWhatsNewDialog = new KLFWhatsNewDialog(this);

  connect(d->mAboutDialog, SIGNAL(linkActivated(const QUrl&)), this, SLOT(helpLinkAction(const QUrl&)));
  connect(d->mWhatsNewDialog, SIGNAL(linkActivated(const QUrl&)), this, SLOT(helpLinkAction(const QUrl&)));

  d->mHelpLinkActions << KLFMainWinPrivate::HelpLinkAction("/whats_new", this, "showWhatsNew", false);
  d->mHelpLinkActions << KLFMainWinPrivate::HelpLinkAction("/about", this, "showAbout", false);
  d->mHelpLinkActions << KLFMainWinPrivate::HelpLinkAction("/popup_close", d, "slotPopupClose", false);
  d->mHelpLinkActions << KLFMainWinPrivate::HelpLinkAction("/popup", d, "slotPopupAction", true);
  d->mHelpLinkActions << KLFMainWinPrivate::HelpLinkAction("/settings", this, "showSettingsHelpLinkAction", true);

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

  d->mHistoryLibResource = NULL;

  d->loadedlibrary = true;
  loadLibrary();

  registerDataOpener(new KLFBasicDataOpener(this));
//  registerDataOpener(new KLFAddOnDataOpener(this));
  registerDataOpener(new KLFTexDataOpener(this));

  // and present a cool window size
  adjustSize();
}

void KLFMainWin::retranslateUi(bool alsoBaseUi)
{
  if (alsoBaseUi)
    u->retranslateUi(this);

  // version information as tooltip on the welcome widget
  u->lblPromptMain->setToolTip(tr("KLatexFormula %1").arg(QString::fromUtf8(KLF_VERSION_STRING)));

  d->refreshStylePopupMenus();
}


KLFMainWin::~KLFMainWin()
{
  KLF_DEBUG_BLOCK(KLF_FUNC_NAME) ;

  // flush data on all pending KLFMimeData instances
  klfDbg("flushing any data on any KLFMimeData instances so that no more delayed retrieval is required.");
  foreach (KLFMimeData * mimedata, KLFMimeData::activeMimeDataInstances()) {
    klfDbg("transmitting all data on mimedata with formats="<<mimedata->formats()) ;
    mimedata->transmitAllData();
  }
  klfDbg("mime datas flushed.");


  klfDbg("about to save settings etc.");

  saveLibraryState();
  saveSettings();
  saveStyles();

  klfDbg("deleting style menu...") ;
  if (d->mStyleMenu)
    delete d->mStyleMenu;
  klfDbg("deleting latex symbols...") ;
  if (d->mLatexSymbols)
    delete d->mLatexSymbols;
  klfDbg("deleting library browser...") ;
  if (d->mLibBrowser)
    delete d->mLibBrowser; // we aren't necessarily its parent
  klfDbg("deleting settings dialog...") ;
  if (d->mSettingsDialog)
    delete d->mSettingsDialog;

  klfDbg("deleting latex preview thread...") ;
  if (d->pContLatexPreview)
    delete d->pContLatexPreview;
  if (d->pLatexPreviewThread)
    delete d->pLatexPreviewThread;


// Do not delete these -- segfault??
//
// #if defined(KLF_WS_MAC)
//   if (d->macFlavorsConverter) {
//     delete d->macFlavorsConverter;
//   }
// #elif defined(KLF_WS_WIN)
//   if (d->winFormatsConverter) {
//     delete d->winFormatsConverter;
//   }
// #else
//   // no special needs on X11
// #endif

  delete d->pExporterManager;

  KLF_DELETE_PRIVATE ;

  klfDbg("deleting interface...") ;
  delete u;
}


void KLFMainWinPrivate::refreshExportTypesMenu()
{
  KLF_DEBUG_BLOCK(KLF_FUNC_NAME) ;

  // Export profiles selection list

  pExportProfileQuickMenuActionList.clear();
  int k;
  QList<KLFMimeExportProfile> eplist = pMimeExportProfileManager.exportProfileList();
  QMenu *menu = new QMenu(K->u->btnSetExportProfile);
  QActionGroup *actionGroup = new QActionGroup(menu);
  actionGroup->setExclusive(true);
  QSignalMapper *smapper = new QSignalMapper(menu);
  QMap<QString,QAction*> submenuacts;
  QMap<QString,QMenu*> submenuactions;
  for (k = 0; k < eplist.size(); ++k) {
    QString submenu = eplist[k].inSubMenu();
    QWidget *parentmenu = NULL;
    if (submenuactions.contains(submenu)) {
      parentmenu = submenuactions[submenu];
    } else if (submenu.isEmpty()) {
      parentmenu = menu;
    } else {
      // create new submenu for this item
      QAction *smact = new QAction(submenu, menu);
      QMenu *smenu = new QMenu(menu);
      klfDbg("new submenu "<<submenu<<"; menuptr="<<smenu) ;
      smact->setMenu(smenu);
      submenuacts[submenu] = smact;
      submenuactions[submenu] = smenu;
      parentmenu = smenu;
    }
    KLF_ASSERT_NOT_NULL(parentmenu, "?! Parent Menu is NULL for submenu="<<submenu<<" !?", continue; ) ;

    klfDbg("adding "<<eplist[k].description()<<"; profile="<<eplist[k].profileName()<<" in menu="<<menu);

    QAction *action = new QAction(menu);
    action->setText(eplist[k].description());
    action->setData(eplist[k].profileName());
    action->setCheckable(true);
    action->setChecked( (klfconfig.ExportData.dragExportProfile()==klfconfig.ExportData.copyExportProfile()) &&
			(klfconfig.ExportData.dragExportProfile()==eplist[k].profileName()) );
    actionGroup->addAction(action);
    parentmenu->addAction(action);
    smapper->setMapping(action, eplist[k].profileName());
    connect(action, SIGNAL(triggered()), smapper, SLOT(map()));
    pExportProfileQuickMenuActionList.append(action);
  }
  // and add submenus at the end
  for (QMap<QString,QAction*>::iterator it = submenuacts.begin(); it != submenuacts.end(); ++it) {
    menu->addAction(*it);
  }

  connect(smapper, SIGNAL(mapped(const QString&)), K, SLOT(slotSetExportProfile(const QString&)));

  K->u->btnSetExportProfile->setMenu(menu);
}

void KLFMainWin::startupFinished()
{
  d->mLatexSymbols->load();

  d->refreshExportTypesMenu();

#if defined(KLF_USE_SPARKLE)
  // if we're on Mac, start the Sparkle updater
  KLFSparkleAutoUpdater * updater = new KLFSparkleAutoUpdater(this, KLF_SPARKLE_FEED_URL);
  updater->checkForUpdates(true);

  if (d->pCheckForUpdatesAction != NULL) {
    connect(d->pCheckForUpdatesAction, SIGNAL(triggered()), updater, SLOT(checkForUpdates()));
  }
#endif

#if defined(KLF_USE_WINSPARKLE)
  // start the winsparkle updater
  KLFWinSparkleUpdater * updater = new KLFWinSparkleUpdater(this, KLF_WINSPARKLE_FEED_URL);
  updater->checkForUpdates(true);
#endif

  /// \todo autoupdate: Add a UI item to enable/disable auto-check for updates, check now, etc.

  if (klfconfig.UI.enableRealTimePreview) {
    d->pLatexPreviewThread->start();
    d->pLatexPreviewThread->setPriority(QThread::LowestPriority);
  }
}



void KLFMainWinPrivate::refreshShowCorrectClearButton()
{
  bool lo = klfconfig.UI.clearLatexOnly;
  K->u->btnClearAll->setVisible(!lo);
  K->u->btnClearLatex->setVisible(lo);
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
  KLF_DEBUG_BLOCK(KLF_FUNC_NAME) ;
  klfDbg( "style name = " << sty ) ;
  // find style with name sty (if existant) and set it
  for (int kl = 0; kl < d->styles.size(); ++kl) {
    if (d->styles[kl].name == sty) {
      slotLoadStyle(kl);
      return true;
    }
  }
  return false;
}

void KLFMainWin::loadSettings()
{
  // start by filling d->setting with defaults
  KLFBackend::detectSettings(& d->settings);

  // then override those defaults with our specific settings

  d->settings.tempdir = klfconfig.BackendSettings.tempDir;
  d->settings.latexexec = klfconfig.BackendSettings.execLatex;
  d->settings.dvipsexec = klfconfig.BackendSettings.execDvips;
  d->settings.gsexec = klfconfig.BackendSettings.execGs;
  d->settings.epstopdfexec = klfconfig.BackendSettings.execEpstopdf;
  d->settings.execenv = klfconfig.BackendSettings.execenv;

  d->settings.lborderoffset = klfconfig.BackendSettings.lborderoffset;
  d->settings.tborderoffset = klfconfig.BackendSettings.tborderoffset;
  d->settings.rborderoffset = klfconfig.BackendSettings.rborderoffset;
  d->settings.bborderoffset = klfconfig.BackendSettings.bborderoffset;

  d->settings.calcEpsBoundingBox = klfconfig.BackendSettings.calcEpsBoundingBox;
  d->settings.outlineFonts = klfconfig.BackendSettings.outlineFonts;
  d->settings.wantPDF = klfconfig.BackendSettings.wantPDF;
  d->settings.wantSVG = klfconfig.BackendSettings.wantSVG;

  klfDbg("klfconfig.BackendSettings.userScriptInterpreters="
         << klfconfig.BackendSettings.userScriptInterpreters()) ;
  QVariantMap userScriptInterpreters = klfconfig.BackendSettings.userScriptInterpreters;
  for (QVariantMap::iterator it = userScriptInterpreters.begin(); it != userScriptInterpreters.end(); ++it) {
    d->settings.userScriptInterpreters[it.key()] = it.value().toString();
  }

  d->settings_altered = false;
}

void KLFMainWin::saveSettings()
{
  KLF_DEBUG_BLOCK(KLF_FUNC_NAME) ;

  d->refreshShowCorrectClearButton();

  if ( ! d->settings_altered ) {    // don't save altered settings
    klfconfig.BackendSettings.tempDir = d->settings.tempdir;
    klfconfig.BackendSettings.execLatex = d->settings.latexexec;
    klfconfig.BackendSettings.execDvips = d->settings.dvipsexec;
    klfconfig.BackendSettings.execGs = d->settings.gsexec;
    klfconfig.BackendSettings.execEpstopdf = d->settings.epstopdfexec;
    klfconfig.BackendSettings.execenv = d->settings.execenv;
    klfconfig.BackendSettings.lborderoffset = d->settings.lborderoffset;
    klfconfig.BackendSettings.tborderoffset = d->settings.tborderoffset;
    klfconfig.BackendSettings.rborderoffset = d->settings.rborderoffset;
    klfconfig.BackendSettings.bborderoffset = d->settings.bborderoffset;
    klfconfig.BackendSettings.calcEpsBoundingBox = d->settings.calcEpsBoundingBox;
    klfconfig.BackendSettings.outlineFonts = d->settings.outlineFonts;
    klfconfig.BackendSettings.wantPDF = d->settings.wantPDF;
    klfconfig.BackendSettings.wantSVG = d->settings.wantSVG;
    QVariantMap map;
    for ( QMap<QString,QString>::iterator it = d->settings.userScriptInterpreters.begin();
          it != d->settings.userScriptInterpreters.end(); ++it ) {
      map[it.key()] = it.value();
    }
    klfconfig.BackendSettings.userScriptInterpreters = map;
    klfDbg("klfconfig.BackendSettings.userScriptInterpreters="
           << klfconfig.BackendSettings.userScriptInterpreters()) ;
  }

  klfconfig.UI.userColorList = KLFColorChooser::colorList();
  klfconfig.UI.colorChooseWidgetRecent = KLFColorChooseWidget::recentColors();
  klfconfig.UI.colorChooseWidgetCustom = KLFColorChooseWidget::customColors();

  klfconfig.writeToConfig();

  //  u->lblOutput->setLabelFixedSize(klfconfig.UI.labelOutputFixedSize);
  u->lblOutput->setEnableToolTipPreview(klfconfig.UI.enableToolTipPreview);

  int k;
  if (klfconfig.ExportData.dragExportProfile == klfconfig.ExportData.copyExportProfile) {
    klfDbg("checking quick menu action item export profile="<<klfconfig.ExportData.copyExportProfile) ;
    // check this export profile in the quick export profile menu
    for (k = 0; k < d->pExportProfileQuickMenuActionList.size(); ++k) {
      if (d->pExportProfileQuickMenuActionList[k]->data().toString() == klfconfig.ExportData.copyExportProfile) {
	klfDbg("checking item #"<<k<<": "<<d->pExportProfileQuickMenuActionList[k]->data()) ;
	// found the one
	d->pExportProfileQuickMenuActionList[k]->setChecked(true);
	break; // by auto-exclusivity the other ones will be un-checked.
      }
    }
  } else {
    // uncheck all items (since there is not a unique one set, drag and copy differ)
    for (k = 0; k < d->pExportProfileQuickMenuActionList.size(); ++k)
      d->pExportProfileQuickMenuActionList[k]->setChecked(false);
  }

  u->btnSetExportProfile->setEnabled(klfconfig.ExportData.menuExportProfileAffectsDrag ||
				     klfconfig.ExportData.menuExportProfileAffectsCopy);
  
  if (klfconfig.UI.enableRealTimePreview) {
    if ( ! d->pLatexPreviewThread->isRunning() ) {
      d->pLatexPreviewThread->start();
      d->pLatexPreviewThread->setPriority(QThread::LowestPriority);
    }
  } else {
    if ( d->pLatexPreviewThread->isRunning() ) {
      d->pLatexPreviewThread->stop();
    }
  }

  u->txtLatex->setWrapLines(klfconfig.UI.editorWrapLines);
  u->txtLatex->setTabChangesFocus(!klfconfig.UI.editorTabInsertsTab);
  u->txtPreamble->setWrapLines(klfconfig.UI.editorWrapLines);
  u->txtPreamble->setTabChangesFocus(!klfconfig.UI.editorTabInsertsTab);

  emit klfConfigChanged();
}

void KLFMainWinPrivate::showExportMsgLabel(const QString& msg, int timeout)
{
  mExportMsgLabel->show();
  mExportMsgLabel->setPalette(mExportMsgLabel->property("defaultPalette").value<QPalette>());

  mExportMsgLabel->setProperty("timeTotal", timeout);
  mExportMsgLabel->setProperty("timeRemaining", timeout);
  mExportMsgLabel->setProperty("timeInterval", 200);

  mExportMsgLabel->setText(msg);

  if (pExportMsgLabelTimerId == -1) {
    pExportMsgLabelTimerId = K->startTimer(mExportMsgLabel->property("timeInterval").toInt());
  }
}

void KLFMainWinPrivate::refreshStylePopupMenus()
{
  if (mStyleMenu == NULL)
    mStyleMenu = new QMenu(K);
  mStyleMenu->clear();

  QAction *a;
  for (int i = 0; i < styles.size(); ++i) {
    a = mStyleMenu->addAction(styles[i].name);
    a->setData(i);
    connect(a, SIGNAL(triggered()), this, SLOT(slotLoadStyleAct()));
  }

  mStyleMenu->addSeparator();
  mStyleMenu->addAction(//QIcon(":/pics/savestyle.png"),
                        tr("Save Current Style"),
			K, SLOT(slotSaveStyle()));
  mStyleMenu->addAction(//QIcon(":/pics/savestyle.png"),
                        tr("Save Current Style As Default"),
			K, SLOT(slotSaveStyleAsDefault()));
  mStyleMenu->addAction(//QIcon(":/pics/managestyles.png"),
                        tr("Manage Styles"),
			K, SLOT(slotStyleManager()), 0 /* accel */);

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

bool KLFMainWinPrivate::try_load_style_list(const QString& styfname)
{
  if ( ! QFile::exists(styfname) )
    return false;

  QFile fsty(styfname);
  if ( ! fsty.open(QIODevice::ReadOnly) )
    return false;

  QDataStream str(&fsty);
  // data stream version is set in klfDataStreamReadHeader()

  QString readHeader;
  QString readCompatKLFVersion;
  bool r = klfDataStreamReadHeader(str, QStringList()<<QLatin1String("KLATEXFORMULA_STYLE_LIST"),
				   &readHeader, &readCompatKLFVersion);
  if (!r) {
    if (readHeader.isEmpty() || readCompatKLFVersion.isEmpty()) {
      QMessageBox::critical(K, tr("Error"), tr("Error: Style file is incorrect or corrupt!\n"));
      return false;
    }
    // too recent version warning
    QMessageBox::warning(K, tr("Load Styles"),
			 tr("The style file found was created by a more recent version "
			    "of KLatexFormula.\n"
			    "The process of style loading may fail.")
			 );
  }
  // we already read the header, so we just need to read the data now
  str >> styles;
  return true;
}

void KLFMainWin::loadStyles()
{
  KLF_DEBUG_BLOCK(KLF_FUNC_NAME) ;

  d->styles = KLFStyleList(); // empty list to start with

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
    if ( (result = d->try_load_style_list(fn)) == true )
      break;
  }
  //  Don't fail if result is false, this is the case on first run (!)
  //  if (!result) {
  //    QMessageBox::critical(this, tr("Error"), tr("Error: Unable to load your style list!"));
  //  }

  if (d->styles.isEmpty()) {
    // if stylelist is empty, populate with default style
    KLFStyle s1(tr("Default", "[[style name]]"), qRgb(0, 0, 0), qRgba(255, 255, 255, 0),
		"\\begin{align*} ... \\end{align*}",
		"\\usepackage{amsmath}\n\\usepackage{amssymb}\n\\usepackage{amsfonts}\n",
		600);
    //    KLFStyle s2(tr("Inline"), qRgb(0, 0, 0), qRgba(255, 255, 255, 0), "\\[ ... \\]", "", 150);
    //    s2.overrideBBoxExpand = KLFStyle::BBoxExpand(0,0,0,0);
    d->styles.append(s1);
    //    d->styles.append(s2);
  }

  d->mStyleMenu = 0;
  d->refreshStylePopupMenus();

  u->btnLoadStyle->setMenu(d->mStyleMenu);
}

void KLFMainWin::saveStyles()
{
  KLF_DEBUG_BLOCK(KLF_FUNC_NAME) ;

  klfconfig.ensureHomeConfigDir();
  QString s = klfconfig.homeConfigDir + QString("/styles-klf%1").arg(KLF_DATA_STREAM_APP_VERSION);
  QFile f(s);
  if ( ! f.open(QIODevice::WriteOnly) ) {
    QMessageBox::critical(this, tr("Error"), tr("Error: Unable to write to styles file!\n%1").arg(s));
    return;
  }
  QDataStream stream(&f);
  // stream version is set by klfDataStreamWriteHeader():
  klfDataStreamWriteHeader(stream, "KLATEXFORMULA_STYLE_LIST");

  stream << d->styles;
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
  QUrlQuery localliburlq(localliburl);
  localliburlq.addQueryItem("klfDefaultSubResource", "History");
  localliburl.setQuery(localliburlq);

  // If the history is already open from library browser saved state, retrieve it
  // This is possibly NULL
  d->mHistoryLibResource = d->mLibBrowser->getOpenResource(localliburl);

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
    d->mHistoryLibResource = localLibFactory->createResource(localliburl.scheme(), param, this);
    if (d->mHistoryLibResource == NULL) {
      qWarning()<<KLF_FUNC_NAME<<": Can't create resource engine for library, of scheme "<<localliburl.scheme()<<"! "
		<<"Create parameters are "<<param;
      qFatal("%s: Can't create resource engine for library!", KLF_FUNC_NAME);
      return;
    }
    d->mHistoryLibResource->setTitle(tr("Local Library"));
    d->mHistoryLibResource
      -> createSubResource(QLatin1String("Archive"),
			   tr("Archive", "[[default sub-resource title for archive sub-resource]]"));
    if (d->mHistoryLibResource->supportedFeatureFlags() & KLFLibResourceEngine::FeatureSubResourceProps) {
      d->mHistoryLibResource->setSubResourceProperty(QLatin1String("History"),
						  KLFLibResourceEngine::SubResPropViewType,
						  QLatin1String("default+list"));
      d->mHistoryLibResource->setSubResourceProperty(QLatin1String("Archive"),
						  KLFLibResourceEngine::SubResPropViewType,
						  QLatin1String("default"));
    } else {
      d->mHistoryLibResource->setViewType(QLatin1String("default+list"));
    }
  }

  if (d->mHistoryLibResource == NULL) {
    d->mHistoryLibResource = KLFLibEngineFactory::openURL(localliburl, this);
  }
  if (d->mHistoryLibResource == NULL) {
    qWarning()<<"KLFMainWin::loadLibrary(): Can't open local history resource!\n\tURL="<<localliburl
	      <<"\n\tNot good! Expect crash!";
    return;
  }

  klfDbg("opened history: resource-ptr="<<d->mHistoryLibResource<<"\n\tImportFile="<<importfname);

  if (!importfname.isEmpty()) {
    // needs an import
    klfDbg("Importing library from "<<importfname) ;

    // visual feedback for import
    KLFProgressDialog pdlg(QString(), this);
    connect(d->mHistoryLibResource, SIGNAL(operationStartReportingProgress(KLFProgressReporter *,
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
    QUrlQuery importliburlq(importliburl);
    importliburlq.addQueryItem("klfReadOnly", "true");
    importliburl.setQuery(importliburlq);
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
	if ( ! d->mHistoryLibResource->hasSubResource(subres) ) {
	  d->mHistoryLibResource->createSubResource(subres);
	}
	d->mHistoryLibResource->insertEntries(subres, insertentries);
      }
    }
  }

  // Open history sub-resource into library browser
  bool r;
  r = d->mLibBrowser->openResource(d->mHistoryLibResource,
				   KLFLibBrowser::NoCloseRoleFlag|KLFLibBrowser::OpenNoRaise,
				   QLatin1String("default+list"));
  if ( ! r ) {
    qWarning()<<"KLFMainWin::loadLibrary(): Can't open local history resource!\n\tURL="<<localliburl
  	      <<"\n\tExpect Crash!";
    return;
  }

  // open all other sub-resources present in our library
  QStringList subresources = d->mHistoryLibResource->subResourceList();
  int k;
  for (k = 0; k < subresources.size(); ++k) {
    //    if (subresources[k] == QLatin1String("History")) // already open
    //      continue;
    // if a URL is already open, it won't open it a second time.
    QUrl url = localliburl;
    QUrlQuery urlq(url);
    urlq.removeAllQueryItems("klfDefaultSubResource");
    urlq.addQueryItem("klfDefaultSubResource", subresources[k]);
    url.setQuery(urlq);

    uint flags = KLFLibBrowser::NoCloseRoleFlag|KLFLibBrowser::OpenNoRaise;
    QString sr = subresources[k].toLower();
    if (sr == "history" || sr == tr("History").toLower())
      flags |= KLFLibBrowser::HistoryRoleFlag;
    if (sr == "archive" || sr == tr("Archive").toLower())
      flags |= KLFLibBrowser::ArchiveRoleFlag;

    d->mLibBrowser->openResource(url, flags);
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

  d->mLibBrowser->loadGuiState(vstatemap, klfconfig.LibraryBrowser.restoreURLs);
}

void KLFMainWin::saveLibraryState()
{
  KLF_DEBUG_BLOCK(KLF_FUNC_NAME) ;

  QString fname = klfconfig.homeConfigDir + "/libbrowserstate.xml";

  QVariantMap statemap = d->mLibBrowser->saveGuiState();

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

void KLFMainWinPrivate::slotOpenHistoryLibraryResource()
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
  KLF_DEBUG_BLOCK(KLF_FUNC_NAME) ;

  emit inputCleared();

  if (restoreFlags & KLFLib::RestoreStyle) {
    KLFStyle style = entry.style();
    klfDbg("restoring style :" <<style) ;
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
  d->slotSetViewControlsEnabled(true);
  d->slotSetSaveControlsEnabled(false);
  activateWindow();
  raise();
  u->txtLatex->setFocus();

  emit userActivity();
}

void KLFMainWinPrivate::slotLibraryButtonRefreshState(bool on)
{
  K->u->btnLibrary->setChecked(on);
}


void KLFMainWin::insertSymbol(const KLFLatexSymbol& s)
{
  // see if we need to insert the xtrapreamble
  QStringList cmds = s.preamble;
  int k;
  QString preambletext = u->txtPreamble->latex();
  QString addtext;
  if (klfconfig.UI.symbolIncludeWithPreambleDefs) {
    for (k = 0; k < cmds.size(); ++k) {
      slotEnsurePreambleCmd(cmds[k]);
    }
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
      if (n > 0) {
	insertsymbol += "{"+sel+"}";
	--n;
      } else {
	// insert before selection, with space
	insertsymbol += " "+sel;
      }
    }
    while (n-- > 0) {
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


void KLFMainWinPrivate::getMissingCmdsFor(const QString& symbol, QStringList * missingCmds,
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

  QList<KLFLatexSymbol> syms = mLatexSymbols->cache()->findSymbols(symbol);
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

  QString curpreambletext = K->u->txtPreamble->latex();
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


void KLFMainWinPrivate::slotNewSymbolTyped(const QString& symbol)
{
  if (mPopup == NULL)
    mPopup = new KLFMainWinPopup(K);

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
  QUrlQuery urlactionq(urlaction);
  urlactionq.addQueryItem("preambleCmdStringList", QString::fromLatin1(cmdsData));
  urlaction.setQuery(urlactionq);
  QUrlQuery urlactionCloseq(urlactionClose);
  urlactionCloseq.addQueryItem("removeMessageKey", msgkey);
  urlactionClose.setQuery(urlactionCloseq);
  mPopup->addMessage(msgkey,
			tr("Symbol <tt>%3</tt> may require <a href=\"%1\">%2</a>.")
			.arg(urlaction.toEncoded(), requiredtext, symbol) );
  mPopup->show();
  mPopup->raise();
}
void KLFMainWinPrivate::slotPopupClose()
{
  if (mPopup != NULL) {
    delete mPopup;
    mPopup = NULL;
  }
}
void KLFMainWinPrivate::slotPopupAction(const QUrl& url)
{
  klfDbg("url="<<url.toEncoded());
  QUrlQuery urlq(url);
  if (urlq.hasQueryItem("preambleCmdStringList")) {
    // include the given package
    QByteArray data = urlq.queryItemValue("preambleCmdStringList").toLatin1();
    klfDbg("data="<<data);
    QStringList cmds = klfLoadVariantFromText(data, "QStringList").toStringList();
    klfDbg("ensuring CMDS="<<cmds);
    int k;
    for (k = 0; k < cmds.size(); ++k) {
      K->slotEnsurePreambleCmd(cmds[k]);
    }
    mPopup->removeMessage("preambleCmdStringList:"+QString::fromLatin1(data));
    if (!mPopup->hasMessages())
      slotPopupClose();
    return;
  }
  if (urlq.hasQueryItem("acceptAll")) {
    slotPopupAcceptAll();
    return;
  }
  if (urlq.hasQueryItem("removeMessageKey")) {
    QString key = urlq.queryItemValue("removeMessageKey");
    mPopup->removeMessage(key);
    if (!mPopup->hasMessages())
      slotPopupClose();
    return;
  }
  if (urlq.hasQueryItem("configDontShow")) {
    // don't show popups
    slotPopupClose();
    klfconfig.UI.showHintPopups = false;
    K->saveSettings();
    return;
  }
}
void KLFMainWinPrivate::slotPopupAcceptAll()
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
	K->slotEnsurePreambleCmd(cmds[k]);
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

  open_paren_modifiers  = KLFLatexSyntaxHighlighter::ParsedBlock::parenSpecs.openParenModifiers();
  close_paren_modifiers = KLFLatexSyntaxHighlighter::ParsedBlock::parenSpecs.closeParenModifiers();
  open_paren_types  = KLFLatexSyntaxHighlighter::ParsedBlock::parenSpecs.openParenList();
  close_paren_types = KLFLatexSyntaxHighlighter::ParsedBlock::parenSpecs.closeParenList();
  // add 'no modifier' possibility :
  open_paren_modifiers.prepend( QString() );
  close_paren_modifiers.prepend( QString() );
  // and remove "{","}" pair because that's a LaTeX argument, don't offer to change those parens
  open_paren_types.removeAll("{");
  close_paren_types.removeAll("}");
}

QVariantMap KLFMainWinPrivate::parseLatexEditPosParenInfo(KLFLatexEdit *latexEdit, int curpos)
{
  KLF_DEBUG_BLOCK(KLF_FUNC_NAME) ;

  QString text = latexEdit->latex();

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


void KLFMainWinPrivate::slotEditorContextMenuInsertActions(const QPoint& pos, QList<QAction*> *actionList)
{
  KLF_DEBUG_BLOCK(KLF_FUNC_NAME) ;

  KLFLatexEdit *latexEdit = qobject_cast<KLFLatexEdit*>(sender());
  KLF_ASSERT_NOT_NULL( latexEdit, "KLFLatexEdit sender is NULL!", return ) ;

  // try to determine if we clicked on a symbol, and suggest to include the corresponding package

  QTextCursor cur = latexEdit->cursorForPosition(pos);
  QString text = latexEdit->latex();
  int curpos = cur.position();

  // matches a latex symbol like \symbol (backslash-word) or \; (backslash-anychar)
  QRegExp rxSym("\\\\" // literal backslash
		"(\\w+|.)" // word or character
		);
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
    /** \bug here on Mac OS X "Ctrl" stays Ctrl and is not translated to "Cmd" */
    amenumod->setText(tr("Paren Modifier (%1) ...")
		      .arg(mShortcutNextParenModifierType->key().toString(QKeySequence::NativeText)));
    amenutyp->setText(tr("Change Paren (%1) ...")
		      .arg(mShortcutNextParenType->key().toString(QKeySequence::NativeText)));
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
    *actionList << amenutyp << amenumod;
  }

  // suggest to wrap selection into a delimiter

  QTextCursor textcur = latexEdit->textCursor();
  if (textcur.hasSelection()) {
    QString s = textcur.selectedText();
    klfDbg("has selection, will add an 'enclose in delimiter...' menu. Selected="<< s) ;
    // get a list of possible delimiters
    QAction *adelim = new QAction(latexEdit);
    adelim->setText(tr("Enclose in delimiter"));
    QMenu *menuDelim = new QMenu(latexEdit);

    int pos = textcur.position();
    int anc = textcur.anchor();
    if (pos > anc) {
      qSwap(pos, anc);
    }
    int len = anc - pos;

    QMenu *parenDelimMenu = new QMenu(latexEdit);
    QList<KLFLatexParenSpecs::ParenSpec> pspecs = KLFLatexSyntaxHighlighter::ParsedBlock::parenSpecs.parenSpecList();
    foreach (KLFLatexParenSpecs::ParenSpec p, pspecs) {
      QAction * a = parenDelimMenu->addAction(QString("%1 ... %2").arg(p.open, p.close),
					      this, SLOT(slotEncloseRegionInDelimiterFromActionSender()));
      QVariantMap v;
      v["pos"] = pos;
      v["len"] = len;
      v["opendelim"] = p.open;
      v["closedelim"] = p.close;
      a->setData(QVariant(v));
    }

    QAction *aMenuParenDelim = menuDelim->addAction(tr("Parenthesis-like", "[[in paren menu delimiter type]]"));
    aMenuParenDelim->setMenu(parenDelimMenu);
    adelim->setMenu(menuDelim);
    *actionList << adelim;
  }
}


void KLFMainWinPrivate::slotInsertMissingPackagesFromActionSender()
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
    K->slotEnsurePreambleCmd(cmds[k]);
  }
}

static inline bool latexGlueMaybeSpace(const QString& a, const QString& b)
{
  if (QRegExp("\\w$").indexIn(a) != -1 && QRegExp("^\\w").indexIn(b) != -1)
    return true;
  return false;
}


void KLFMainWinPrivate::slotChangeParenFromActionSender()
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
void KLFMainWinPrivate::slotChangeParenFromVariantMap(const QVariantMap& data)
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

  QString latextext = K->u->txtLatex->latex();
  if (latexGlueMaybeSpace(s, latextext.mid(pos+len))) {
    // need to separate the paren symbol (e.g. \langle) with the following text, i.e. make
    // sure that we have "\langle x", not "\langlex"
    s += " ";
  }
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

    QString os = omod + ostr;

    QString latextext = K->u->txtLatex->latex();
    if (latexGlueMaybeSpace(os, latextext.mid(opos+olen))) {
      // need to separate the paren symbol (e.g. \langle) with the following text, i.e. make
      // sure that we have "\rangle x", not "\ranglex"
      os += " ";
    }

    latexEditReplace(opos, olen, os);
  }
}

void KLFMainWin::slotCycleParenModifiers(bool forward)
{
  d->slotCycleParenModifiers(forward);
}
void KLFMainWinPrivate::slotCycleParenModifiers(bool forward)
{
  int curpos = K->u->txtLatex->textCursor().position();
  const QVariantMap vmap = parseLatexEditPosParenInfo(K->u->txtLatex, curpos);

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
  mod_index += forward ? 1 : -1;
  if (mod_index < 0)
    mod_index += paren_mod_list.size();
  mod_index %= paren_mod_list.size();
  data["newparenmod_index"] = mod_index;

  slotChangeParenFromVariantMap(data);
}

void KLFMainWin::slotCycleParenTypes(bool forward)
{
  d->slotCycleParenTypes(forward);
}
void KLFMainWinPrivate::slotCycleParenTypes(bool forward)
{
  int curpos = K->u->txtLatex->textCursor().position();
  const QVariantMap vmap = parseLatexEditPosParenInfo(K->u->txtLatex, curpos);

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
  str_index += forward ? 1 : -1;
  if (str_index < 0)
    str_index += paren_typ_list.size();
  str_index %= paren_typ_list.size();
  data["newparenstr_index"] = str_index;

  slotChangeParenFromVariantMap(data);
}

void KLFMainWinPrivate::slotEncloseRegionInDelimiterFromActionSender()
{
  QAction * a = qobject_cast<QAction*>(sender());
  if (a == NULL) {
    qWarning()<<KLF_FUNC_NAME<<": NULL QAction sender!";
    return;
  }
  QVariant v = a->data();
  QVariantMap data = v.toMap();

  slotEncloseRegionInDelimiter(data);
}

void KLFMainWinPrivate::slotEncloseRegionInDelimiter(const QVariantMap& vmap)
{
  int pos = vmap["pos"].toInt();
  int len = vmap["len"].toInt();
  QString opendelim = vmap["opendelim"].toString();
  QString closedelim = vmap["closedelim"].toString();

  QString text = K->u->txtLatex->latex().mid(pos, len);

  if (latexGlueMaybeSpace(opendelim, text))
    opendelim += " ";
  if (latexGlueMaybeSpace(closedelim, K->u->txtLatex->latex().mid(pos+len)))
    closedelim += " ";
  latexEditReplace(pos, len, opendelim + text + closedelim);
}

void KLFMainWinPrivate::latexEditReplace(int pos, int len, const QString& text)
{
  QTextCursor c = K->u->txtLatex->textCursor();
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
  K->u->txtLatex->setTextCursor(c);
}



void KLFMainWinPrivate::slotUserScriptDisableInputs(KLFUserScriptInfo * infobase)
{
  QStringList disableInputs = QStringList();
  if (infobase != NULL) {
    disableInputs = KLFBackendEngineUserScriptInfo(infobase->userScriptPath()).disableInputs();
  }
  bool disableall = disableInputs.contains("ALL", Qt::CaseInsensitive);
  bool disableallinput = disableInputs.contains("ALL_INPUT", Qt::CaseInsensitive);
  bool disableall_except = disableInputs.contains("EXCEPT", Qt::CaseInsensitive);
#define WANT_DISABLE_ALL(x, disableall)                                 \
  ( (!disableall && disableInputs.contains(x, Qt::CaseInsensitive))     \
    || (disableall && !disableall_except)                               \
    || (disableall && disableall_except && !disableInputs.contains(x, Qt::CaseInsensitive)) )
#define WANT_DISABLE_INPUT(x)                           \
  WANT_DISABLE_ALL(x, (disableall || disableallinput))
#define WANT_DISABLE_NORMAL(x)                  \
  WANT_DISABLE_ALL(x, disableall)
  
  // names are the same as in klfbackend.cpp
  K->u->colFg->setDisabled(WANT_DISABLE_INPUT("FG_COLOR"));
  K->u->colBg->setDisabled(WANT_DISABLE_INPUT("BG_COLOR"));
  K->u->chkMathMode->setDisabled(WANT_DISABLE_INPUT("MATHMODE"));
  K->u->cbxMathMode->setDisabled(WANT_DISABLE_INPUT("MATHMODE") || !K->u->chkMathMode->isChecked());
  K->u->txtPreamble->setDisabled(WANT_DISABLE_INPUT("PREAMBLE"));
  K->u->cbxLatexFont->setDisabled(WANT_DISABLE_INPUT("FONT"));
  K->u->spnLatexFontSize->setDisabled(WANT_DISABLE_INPUT("FONTSIZE"));
  K->u->spnDPI->setDisabled(WANT_DISABLE_NORMAL("DPI"));
  K->u->btnDPIPresets->setDisabled(WANT_DISABLE_NORMAL("DPI"));
  K->u->chkVectorScale->setDisabled(WANT_DISABLE_NORMAL("VECTORSCALE"));
  K->u->spnVectorScale->setDisabled(WANT_DISABLE_NORMAL("VECTORSCALE")
                                    || !K->u->chkVectorScale->isChecked());
  K->u->gbxOverrideMargins->setDisabled(WANT_DISABLE_NORMAL("MARGINS"));
}

void KLFMainWinPrivate::slotUserScriptSet(int index)
{
  KLF_DEBUG_BLOCK(KLF_FUNC_NAME) ;
  klfDbg("index="<<index) ;

  if (index == 0) {
    // no user script
    userScriptCurrentInfo = tr("<no user script selected>", "[[popup info]]");
    slotUserScriptDisableInputs(NULL);
    K->u->stkScriptInput->setCurrentWidget(K->u->wScriptInputEmptyPage);
    K->u->lblScriptInputEmptyPage->setText(tr("<no user script selected>",
					      "[[space for user script custom input]]"));
    K->u->btnUserScriptInfo->setIcon(QIcon(":/pics/info.png"));
    K->u->btnUserScriptInfo->setEnabled(false);
    return;
  }

  K->u->btnUserScriptInfo->setEnabled(true);

  // update user script info and settings widget
  
  KLFBackendEngineUserScriptInfo usinfo(K->u->cbxUserScript->itemData(index).toString());

  if (usinfo.hasWarnings() || usinfo.hasErrors())
    K->u->btnUserScriptInfo->setIcon(QIcon(":/pics/infowarn.png"));
  else
    K->u->btnUserScriptInfo->setIcon(QIcon(":/pics/info.png"));

  int textpointsize = QFontInfo(K->u->lblUserScript->font()).pointSize();
  QString extracss = QString::fromLatin1("body { font-size: %1pt; }\n").arg(textpointsize-1) +
    QString::fromLatin1("p.msgerror { color: #a00000; font-size: %1pt;}\n").arg(textpointsize+1) +
    QString::fromLatin1("p.msgwarning { color: #a04000; font-size: %1pt; }\n").arg(textpointsize+1);
  userScriptCurrentInfo = usinfo.htmlInfo(extracss);

  slotUserScriptDisableInputs(&usinfo);

  K->u->lblScriptInputEmptyPage->setText(tr("<user script has no custom input>",
					    "[[space for user script custom input]]"));

  QString uifile = usinfo.inputFormUI();
  if (!uifile.isEmpty()) {
    QString uifilefull = usinfo.relativeFile(uifile);
    QWidget * scriptinputwidget = getUserScriptInputWidget(uifilefull);
    K->u->stkScriptInput->setCurrentWidget(scriptinputwidget);
    klfDbg("Set script input UI. uifile="<<uifilefull) ;
  } else {
    K->u->stkScriptInput->setCurrentWidget(K->u->wScriptInputEmptyPage);
  }
}

QWidget * KLFMainWinPrivate::getUserScriptInputWidget(const QString& uifile)
{
  KLF_DEBUG_BLOCK(KLF_FUNC_NAME) ;
  klfDbg("uifile = " << uifile) ;
  if (userScriptInputWidgets.contains(uifile)) {
    klfDbg("widget from cache.") ;
    return userScriptInputWidgets[uifile];
  }

  QFile uif(uifile);
  uif.open(QFile::ReadOnly);
  QWidget * scriptinputwidget = klfLoadUI(&uif, K->u->stkScriptInput);
  uif.close();
  KLF_ASSERT_NOT_NULL(scriptinputwidget, "Can't load script input widget "<<uifile<<".",
		      return K->u->wScriptInputEmptyPage) ;

  K->u->stkScriptInput->addWidget(scriptinputwidget);
  userScriptInputWidgets[uifile] = scriptinputwidget;
  return scriptinputwidget;
}

void KLFMainWinPrivate::slotUserScriptShowInfo()
{
  QWhatsThis::showText(K->u->btnUserScriptInfo->mapToGlobal(QPoint(10,10)), userScriptCurrentInfo,
		       K->u->btnUserScriptInfo);
}


void KLFMainWinPrivate::reloadLatexFontDefs()
{
  // find collection of XML files
  QStringList fxmllist;
  // in the following directories
  QStringList fxmldirs;
  fxmldirs << klfconfig.homeConfigDir + "/conf/latexfontdefs.d/"
	   << klfconfig.globalShareDir + "/conf/latexfontdefs.d/"
	   << ":/conf/latexfontdefs.d/";

  klfDbgT("starting to collect XML files from dirs "<<fxmldirs) ;

  // collect all XML files
  int k, j;
  for (k = 0; k < fxmldirs.size(); ++k) {
    QDir fxmldir(fxmldirs[k]);
    QStringList xmllist = fxmldir.entryList(QStringList()<<"*.xml", QDir::Files);
    for (j = 0; j < xmllist.size(); ++j)
      fxmllist << fxmldir.absoluteFilePath(xmllist[j]);
  }
  klfDbgT("files collected: "<<fxmllist) ;

  // clear our list of font definitions
  pLatexFontDefs.clear();

  // now read the file list
  for (k = 0; k < fxmllist.size(); ++k) {
    KLF_DEBUG_TIME_BLOCK(KLF_FUNC_NAME+"/fxmllist["+QString::number(k)+"]");
    klfDbg("reading XML file="<<fxmllist[k]);

    QString fn = fxmllist[k];
    QFile file(fn);
    if ( ! file.open(QIODevice::ReadOnly) ) {
      klfWarning("Can't open latex font definitions XML file "<<fn<<": "
                 <<file.errorString()<<"!");
      continue;
    }

    QDomDocument doc("latex-font-defs-list");
    QString errMsg; int errLine, errCol;
    bool r = doc.setContent(&file, false, &errMsg, &errLine, &errCol);
    if (!r) {
      klfWarning("Error parsing file "<<fn<<": "<<errMsg<<" at line "<<errLine<<", col "<<errCol);
      continue;
    }
    file.close();
    
    QDomElement root = doc.documentElement();
    if (root.nodeName() != "latex-font-defs-list") {
      klfWarning("Error parsing XML for latex font defs from file "<<fn<<": unexpected root tag `"
                 <<root.nodeName()<<"'.");
      continue;
    }
    QString minklfversion = root.attribute("minklfversion", "3.3.0a");
    if (klfVersionCompare(minklfversion, KLF_VERSION_STRING) > 0) {
      klfWarning("Ignoring XML latex font definition file " << fn << " which requires KLF version "
                 << minklfversion << ".");
      continue;
    }

    QDomNode n;
    for (n = root.firstChild(); ! n.isNull(); n = n.nextSibling()) {
      QDomElement e = n.toElement(); // try to convert the node to an element.
      if ( e.isNull() || n.nodeType() != QDomNode::ElementNode )
	continue;
      if ( e.nodeName() != "font-def" ) {
	klfWarning("Parsing XML file "<<fn<<": ignoring unexpected tag <"<<e.nodeName()<<">") ;
	continue;
      }

      LatexFontDef thisfont;

      QDomNode efdetails;
      for (efdetails = e.firstChild(); ! efdetails.isNull(); efdetails = efdetails.nextSibling() ) {
	if ( efdetails.isNull() || efdetails.nodeType() != QDomNode::ElementNode )
	  continue;
	QDomElement eefdetails = efdetails.toElement();

	klfDbg("read element "<<efdetails.nodeName());

	if ( eefdetails.nodeName() == "title" ) {
          if (!thisfont.title.isEmpty()) {
            klfWarning("File "<<fn<<": Second title for font definition: already has title "<<thisfont.title) ;
          }
          thisfont.title = qApp->translate("xmltr_latexfontdefs", eefdetails.text().toUtf8().constData(),
                                           "[[tag: <category-title>]]");
        } else if ( eefdetails.nodeName() == "identifier" ) {
          if (!thisfont.identifier.isEmpty()) {
            klfWarning("File "<<fn<<": Second identifier for font definition: already has identifier "
                       <<thisfont.identifier) ;
          }
          thisfont.identifier = eefdetails.text();
        } else if ( eefdetails.nodeName() == "latexdefs" ) {
          if (!thisfont.latexdefs.isEmpty()) {
            klfWarning("File "<<fn<<": Latex defs for "<<thisfont.identifier<<" already given") ;
          }
          thisfont.latexdefs += eefdetails.text();
        } else {
          klfWarning("File "<<fn<<": Ignoring unknown node "<<eefdetails.nodeName()) ;
        }
      } // font def details

      // now, save the font def:
      pLatexFontDefs.append(thisfont);

    } // font defs in XML file
  } // iterate over XML files
}



void KLFMainWinPrivate::slotSymbolsButtonRefreshState(bool on)
{
  K->u->btnSymbols->setChecked(on);
}

void KLFMainWin::showAbout()
{
  d->mAboutDialog->show();
}

void KLFMainWin::showWhatsNew()
{
  // center the widget on the desktop
  QSize desktopSize = QApplication::desktop()->screenGeometry(this).size();
  QSize wS = d->mWhatsNewDialog->sizeHint();
  d->mWhatsNewDialog->move(desktopSize.width()/2 - wS.width()/2,
			   desktopSize.height()/2 - wS.height()/2);
  d->mWhatsNewDialog->show();
}

void KLFMainWin::showSettingsHelpLinkAction(const QUrl& link)
{
  klfDbg( ": link="<<link ) ;
  d->mSettingsDialog->show();
  d->mSettingsDialog->showControl(QUrlQuery(link).queryItemValue("control"));
}

void KLFMainWin::helpLinkAction(const QUrl& link)
{
  klfDbg( ": Link is "<<link<<"; scheme="<<link.scheme()<<"; path="<<link.path()
	  <<"; queryItems="<<QUrlQuery(link).queryItems() ) ;

  if (link.scheme() == "http") {
    // web link
    QDesktopServices::openUrl(link);
    return;
  }
  if (link.scheme() == "klfaction") {
    // find the help link action(s) that is (are) associated with this link
    bool calledOne = false;
    int k;
    for (k = 0; k < d->mHelpLinkActions.size(); ++k) {
      if (d->mHelpLinkActions[k].path == link.path()) {
	// got one
	if (d->mHelpLinkActions[k].wantParam)
	  QMetaObject::invokeMethod(d->mHelpLinkActions[k].reciever,
				    d->mHelpLinkActions[k].memberFunc.constData(),
				    Qt::QueuedConnection, Q_ARG(QUrl, link));
	else
	  QMetaObject::invokeMethod(d->mHelpLinkActions[k].reciever,
				    d->mHelpLinkActions[k].memberFunc.constData(),
				    Qt::QueuedConnection);

	calledOne = true;
      }
    }
    if (!calledOne) {
      klfWarning("no action found for link="<<link.toString()) ;
    }
    return;
  }
  klfWarning(link.toString()<<": Unrecognized link scheme!") ;
}

void KLFMainWin::addWhatsNewText(const QString& htmlSnipplet)
{
  d->mWhatsNewDialog->addExtraText(htmlSnipplet);
}


void KLFMainWin::registerHelpLinkAction(const QString& path, QObject *object, const char * member,
					bool wantUrlParam)
{
  d->mHelpLinkActions << KLFMainWinPrivate::HelpLinkAction(path, object, member, wantUrlParam);
}


void KLFMainWin::registerExporter(KLFExporter *exporter)
{
  d->pExporterManager->registerExporter(exporter) ;
}

void KLFMainWin::unregisterExporter(KLFExporter *exporter)
{
  d->pExporterManager->registerExporter(exporter);
}

void KLFMainWin::registerDataOpener(KLFAbstractDataOpener *dataopener)
{
  KLF_ASSERT_NOT_NULL( dataopener, "Refusing to register NULL Data Opener object!",  return ) ;

  d->pDataOpeners.append(dataopener);
}
void KLFMainWin::unregisterDataOpener(KLFAbstractDataOpener *dataopener)
{
  d->pDataOpeners.removeAll(dataopener);
}


void KLFMainWin::setWidgetStyle(const QString& qtstyle)
{
  //  qDebug("setWidgetStyle(\"%s\")", qPrintable(qtstyle));
  if (d->widgetstyle == qtstyle) {
    //    qDebug("This style is already applied.");
    return;
  }
  if (qtstyle.isNull()) {
    // setting a null style resets internal widgetstyle name...
    d->widgetstyle = QString::null;
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
  d->widgetstyle = qtstyle;
  qApp->setStyle( s );
}

void KLFMainWin::setQuitOnClose(bool quitOnClose)
{
  d->ignore_close_event = ! quitOnClose;
}

bool KLFMainWin::executeURLCommandsFromFile(const QString& fname)
{
  KLF_DEBUG_BLOCK(KLF_FUNC_NAME) ;
  klfDbg("fname="<<fname) ;

  QFile f(fname);
  if (!f.open(QIODevice::ReadOnly)) {
    klfDbg("Unable to open file "<<fname) ;
    return false;
  }
  // read commands and execute them
  QByteArray header = f.readLine();
  if (!header.startsWith("KLFCmdIface/URL: ")) {
    klfDbg("Bad file header "<<fname) ;
    return false;
  }

  /** \bug TODO..... parse header, allow empty lines before header etc. */

  bool wantautoremove = false;

  while (!f.atEnd()) {
    QByteArray s = f.readLine();
    //    if (!f.errorString().isEmpty()) {
    //      qWarning()<<KLF_FUNC_NAME<<": Error : "<<f.errorString();
    //      return false;
    //    }
    klfDbg("read line: "<<s) ;
    if (s.isEmpty())
      continue;
    if (s.endsWith("\n"))
      s = s.mid(0, s.length()-1);
    if (!s.startsWith("_")) {
      QUrl url = QUrl::fromEncoded(s);
      klfDbg("command: "<<url) ;
      d->pCmdIface->executeCommand(url);
    } else if (s == "_autoremovefile") {
      wantautoremove = true;
    } else {
      klfWarning("Unknown pragma: "<<s) ;
    }
  }
  f.close();

  if (wantautoremove) {
    klfDbg("removing file.") ;
    QFile::remove(fname);
  }

  return true;
}

bool KLFMainWin::isApplicationVisible() const
{
#ifdef KLF_WS_MAC
  klfDbg("(mac version)") ;
  extern bool klf_mac_application_hidden(const KLFMainWin *);
  return KLF_DEBUG_TEE( ! klf_mac_application_hidden(this) ) ;
#else
  klfDbg("(non-mac version)") ;
  return KLF_DEBUG_TEE( isVisible() ) ;
#endif
}

void KLFMainWin::hideApplication()
{
#ifdef KLF_WS_MAC
  extern void klf_mac_hide_application(const KLFMainWin *);
  klf_mac_hide_application(this);
#else
  klfHideWindows();
  //  qWarning()<<KLF_FUNC_NAME<<": this function is only available on Mac OS X.";
#endif
}

void KLFMainWin::quit()
{
  // a slot which basically calls qApp->quit() (and can safely be called several
  // times in the quitting process)

  KLF_DEBUG_BLOCK(KLF_FUNC_NAME) ;

  if (d->is_quitting) {
    // already called.
    klfDbg("already quitting.") ;
    return;
  }
  d->is_quitting = true;

  u->frmDetails->showSideWidget(false);
  u->frmDetails->sideWidgetManager()->waitForShowHideActionFinished();

  hide();
  /** \bug ### go through all top-level widgets given by QApplication::topLevelWidgets or so ? */
  if (d->mLibBrowser)
    d->mLibBrowser->hide();
  if (d->mLatexSymbols)
    d->mLatexSymbols->hide();
  if (d->mStyleManager)
    d->mStyleManager->hide();
  if (d->mSettingsDialog)
    d->mSettingsDialog->hide();

  if (!d->qapp_quit_called) {
    d->qapp_quit_called = true;
    qApp->quit();
  }
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

bool KLFMainWin::nativeEvent(const QByteArray & , void * , long * )
{
  // if we need to process native X11/Mac/Win events, do it here.
  return false;
}


void KLFMainWin::childEvent(QChildEvent *e)
{
  KLF_DEBUG_BLOCK(KLF_FUNC_NAME) ;
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
      if (d->mPopup != NULL && ke->key() == Qt::Key_Return &&
	  QApplication::keyboardModifiers() == Qt::AltModifier) {
	d->slotPopupAcceptAll();
	return true;
      }
      if (d->mPopup != NULL && ke->key() == Qt::Key_Escape) {
	d->slotPopupClose();
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
  if ( obj == d->mLibBrowser && (e->type() == QEvent::Hide || e->type() == QEvent::Show) ) {
    d->slotLibraryButtonRefreshState(d->mLibBrowser->isVisible());
  }
  if ( obj == d->mLatexSymbols && (e->type() == QEvent::Hide || e->type() == QEvent::Show) ) {
    d->slotSymbolsButtonRefreshState(d->mLatexSymbols->isVisible());
  }

  // ----
  if ( obj == QApplication::instance() ) {
    klfDbg("Application event: type="<<e->type()) ;
    if ( e->type() == QEvent::Close && ! d->qapp_quit_called ) {
      // intercept close-event, which means that qApp->quit() was called directly
      d->qapp_quit_called = true;
      quit();
    }
  }
  if ( obj == QApplication::instance() && e->type() == QEvent::FileOpen ) {
    // open a URL or file
    QFileOpenEvent *fe = (QFileOpenEvent*)e;
    klfDbg("QFileOpenEvent: Opening: "<<fe->file()<<"; URL="<<fe->url()) ;
#if QT_VERSION >= 0x040600 // for QFileOpenEvent::url()
    if (fe->url().scheme() == "klfcommand") {
      // this is a KLFCmdIface-command
      d->pCmdIface->executeCommand(fe->url());
      return true;
    }
#endif
    openFile(fe->file());
    return true;
  }

  // ----
  if ( (obj == u->btnCopy || obj == u->btnDrag) && e->type() == QEvent::ToolTip ) {
    QString tooltipText;
    if (obj == u->btnCopy) {
      tooltipText = tr("Copy the formula to the clipboard. Current export profile: %1")
	.arg(d->pMimeExportProfileManager.findExportProfile(klfconfig.ExportData.copyExportProfile)
             .description());
    } else if (obj == u->btnDrag) {
      tooltipText = tr("Click and keep mouse button pressed to drag your formula to another application. "
		       "Current export profile: %1")
	.arg(d->pMimeExportProfileManager.findExportProfile(klfconfig.ExportData.dragExportProfile)
             .description());
    }
    QToolTip::showText(((QHelpEvent*)e)->globalPos(), tooltipText, qobject_cast<QWidget*>(obj));
    return true;
  }

  return QWidget::eventFilter(obj, e);
}


void KLFMainWin::refreshAllWindowStyleSheets()
{
  KLF_DEBUG_BLOCK(KLF_FUNC_NAME) ;
  QWidgetList wlist = QApplication::topLevelWidgets();
  foreach (QWidget *w, wlist) {
    w->setStyleSheet(w->styleSheet());
  }
}


void KLFMainWin::hideEvent(QHideEvent *e)
{
  KLF_DEBUG_BLOCK(KLF_FUNC_NAME) ;

  klfDbg("...") ;
  QWidget::hideEvent(e);
}

void KLFMainWin::showEvent(QShowEvent *e)
{
  if ( d->firstshow ) {
    d->firstshow = false;
    // if it's the first time we're run, 
    // show the what's new if needed.
    if ( klfconfig.Core.thisVersionMajMinFirstRun ) {
      QMetaObject::invokeMethod(this, "showWhatsNew", Qt::QueuedConnection);
    }

#ifdef KLF_WS_MAC
    // add a menu item in dock to paste from clipboard
    QMenu *adddockmenu = new QMenu;
    adddockmenu->addAction(tr("Paste Clipboard Contents", "[[Dock Menu Entry on MacOSX]]"),
			   this, SLOT(pasteLatexFromClipboard()));
    extern void qt_mac_set_dock_menu(QMenu *);
    qt_mac_set_dock_menu(adddockmenu);
#endif
  }

  QWidget::showEvent(e);
}


void KLFMainWin::timerEvent(QTimerEvent *e)
{
  if (e->timerId() == d->pExportMsgLabelTimerId) {
    int total = d->mExportMsgLabel->property("timeTotal").toInt();
    int remaining = d->mExportMsgLabel->property("timeRemaining").toInt();
    int interval = d->mExportMsgLabel->property("timeInterval").toInt();

    int fadepercent = (100 * remaining / total) * 3; // 0 ... 300
    if (fadepercent < 100 && fadepercent >= 0) {
      QPalette pal = d->mExportMsgLabel->property("defaultPalette").value<QPalette>();
      QColor c = pal.color(QPalette::Window);
      c.setAlpha(c.alpha() * fadepercent / 100);
      pal.setColor(QPalette::Window, c);
      QColor c2 = pal.color(QPalette::WindowText);
      c2.setAlpha(c2.alpha() * fadepercent / 100);
      pal.setColor(QPalette::WindowText, c2);
      d->mExportMsgLabel->setPalette(pal);
    }

    remaining -= interval;
    if (remaining < 0) {
      d->mExportMsgLabel->hide();
      killTimer(d->pExportMsgLabelTimerId);
      d->pExportMsgLabelTimerId = -1;
    } else {
      d->mExportMsgLabel->setProperty("timeRemaining", QVariant(remaining));
    }
  }
}



void KLFMainWin::alterSetting(int which, int ivalue)
{
  d->settings_altered = true;
  switch (which) {
  case altersetting_LBorderOffset:
    d->settings.lborderoffset = ivalue; break;
  case altersetting_TBorderOffset:
    d->settings.tborderoffset = ivalue; break;
  case altersetting_RBorderOffset:
    d->settings.rborderoffset = ivalue; break;
  case altersetting_BBorderOffset:
    d->settings.bborderoffset = ivalue; break;
  case altersetting_CalcEpsBoundingBox:
    d->settings.calcEpsBoundingBox = (bool)ivalue; break;
  case altersetting_OutlineFonts:
    d->settings.outlineFonts = (bool)ivalue; break;
  case altersetting_WantSVG:
    d->settings.wantSVG = (bool)ivalue; break;
  case altersetting_WantPDF:
    d->settings.wantPDF = (bool)ivalue; break;
  default:
    qWarning()<<KLF_FUNC_NAME<<": Unknown setting to alter: "<<which<<", maybe you should give a string instead?";
    break;
  }
}
void KLFMainWin::alterSetting(int which, QString svalue)
{
  d->settings_altered = true;
  switch (which) {
  case altersetting_TempDir:
    d->settings.tempdir = svalue; break;
  case altersetting_Latex:
    d->settings.latexexec = svalue; break;
  case altersetting_Dvips:
    d->settings.dvipsexec = svalue; break;
  case altersetting_Gs:
    d->settings.gsexec = svalue; break;
  case altersetting_Epstopdf:
    d->settings.epstopdfexec = svalue; break;
  case altersetting_LBorderOffset:
    d->settings.lborderoffset = svalue.toDouble(); break;
  case altersetting_TBorderOffset:
    d->settings.tborderoffset = svalue.toDouble(); break;
  case altersetting_RBorderOffset:
    d->settings.rborderoffset = svalue.toDouble(); break;
  case altersetting_BBorderOffset:
    d->settings.bborderoffset = svalue.toDouble(); break;
  default:
    qWarning()<<KLF_FUNC_NAME<<": Unknown setting to alter: "<<which<<", maybe you should give an int instead?";
    break;
  }
}

KLFLibBrowser * KLFMainWin::libBrowserWidget()
{
  return d->mLibBrowser;
}
KLFLatexSymbols * KLFMainWin::latexSymbolsWidget()
{
  return d->mLatexSymbols;
}
KLFStyleManager * KLFMainWin::styleManagerWidget()
{
  return d->mStyleManager;
}
KLFSettings * KLFMainWin::settingsDialog()
{
  return d->mSettingsDialog;
}
QMenu * KLFMainWin::styleMenu()
{
  return d->mStyleMenu;
}

KLFConfig * KLFMainWin::klfConfig()
{
  return & klfconfig;
}

QString KLFMainWin::widgetStyle() const
{
  return d->widgetstyle;
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
  d->settings = s;
  d->settings_altered = false;

  d->updatePreviewThreadSettings();
}

void KLFMainWinPrivate::displayError(const QString& error)
{
  QMessageBox::critical(K, tr("Error"), error);
}


void KLFMainWinPrivate::updatePreviewThreadInput()
{
  // recheck battery status, in case we have the proper setting on
  // TODO: write this & the setting
  klfDbg("laptop?: "<<KLFSysInfo::isLaptop()<<"; on battery power?: "<<KLFSysInfo::isOnBatteryPower()
         <<"; no previews on battery power?: " << klfconfig.UI.realTimePreviewExceptBattery) ;
  if (klfconfig.UI.enableRealTimePreview && klfconfig.UI.realTimePreviewExceptBattery
      && KLFSysInfo::isLaptop()) {
    // update battery status
    bool onbattery = KLFSysInfo::isOnBatteryPower();
    bool running = pLatexPreviewThread->isRunning();
    if (running && onbattery) {
      // we need to stop the preview thread
      klfDbg("Stopping preview thread because we're on battery power") ;
      pLatexPreviewThread->stop();
    } else if (!running && !onbattery) {
      // we can restart the preview thread, as we're back on power.
      klfDbg("Restarting preview thread because we're no longer on battery power") ;
      pLatexPreviewThread->start();
      pLatexPreviewThread->setPriority(QThread::LowestPriority);
    }
  }

  bool inputchanged = pContLatexPreview->setInput(collectInput(false));
  if (inputchanged) {
    emit K->userInputChanged();
  }
  if (evaloutput_uptodate && !inputchanged) {
    // if we are in 'evaluated' mode, with a displayed result, then don't allow a window resize
    // to invalidate evaluated contents
    return;
  }
  bool sizechanged =
    pContLatexPreview->setPreviewSize(calcPreviewSize());
  if (inputchanged || sizechanged) {
    evaloutput_uptodate = false;
  }
}

QSize KLFMainWinPrivate::calcPreviewSize()
{
  return (QSizeF(K->u->lblOutput->size()) * K->devicePixelRatioF()).toSize();
}

void KLFMainWinPrivate::updatePreviewThreadSettings()
{
  KLFBackend::klfSettings s = K->currentSettings();
  klfDbg("Updating preview thread's settings. currentSettings().execenv="<<s.execenv);
  pContLatexPreview->setSettings(s);
}

void KLFMainWin::setTxtLatexFont(const QFont& f)
{
  u->txtLatex->setFont(f);
}
void KLFMainWin::setTxtPreambleFont(const QFont& f)
{
  u->txtPreamble->setFont(f);
}

void KLFMainWin::setMacBrushedMetalLook(bool metallook)
{
  KLF_DEBUG_BLOCK(KLF_FUNC_NAME) ;

#ifdef KLF_WS_MAC
  klfDbg("On mac. metallook = " << metallook) ;

  // doesn't work ... :(
  //setUnifiedTitleAndToolBarOnMac(metallook);
  //setAttribute(Qt::WA_MacBrushedMetal, metallook);
  const char * pn = metallook ? "paletteMacBrushedMetalLook" : "paletteDefault";
  u->txtLatex->setPalette(u->txtLatex->property(pn).value<QPalette>());
  u->txtLatex->setStyleSheet(u->txtLatex->styleSheet());
  if (u->frmDetails->sideWidgetManagerType() == QLatin1String("ShowHide")) {
    u->txtPreamble->setPalette(u->txtPreamble->property(pn).value<QPalette>());
    u->txtPreamble->setStyleSheet(u->txtPreamble->styleSheet());
    u->cbxMathMode->setPalette(u->txtPreamble->property(pn).value<QPalette>());
    u->cbxMathMode->setStyleSheet(u->cbxMathMode->styleSheet());
    u->cbxMathMode->lineEdit()->setPalette(u->txtPreamble->property(pn).value<QPalette>());
    u->cbxMathMode->lineEdit()->setStyleSheet(u->cbxMathMode->lineEdit()->styleSheet());
  }

#else
  Q_UNUSED(metallook);
  if (metallook) {
    klfWarning("Mac brushed metal look is only implemented under Mac OS X !");
  }
#endif
}


void KLFMainWinPrivate::showRealTimeReset()
{
  klfDbg("reset.") ;
  K->u->lblOutput->display(QImage(), QImage(), false);
  slotSetViewControlsEnabled(false);
  slotSetSaveControlsEnabled(false);
}

void KLFMainWinPrivate::showRealTimePreview(const QImage& preview, const QImage& largePreview)
{
  klfDbg("preview.size=" << preview.size()<< "  largepreview.size=" << largePreview.size());
  if (evaloutput_uptodate) // we have clicked on 'Evaluate' button, use that image!
    return;

  K->u->lblOutput->display(preview, largePreview, false);
  slotSetViewControlsEnabled(true);
  slotSetSaveControlsEnabled(false);

  emit K->previewAvailable(preview, largePreview);
}

void KLFMainWinPrivate::showRealTimeError(const QString& errmsg, int errcode)
{
  klfDbg("errormsg[0..99]="<<errmsg.mid(0,99)<<", code="<<errcode) ;
  QString s = errmsg;
  if (errcode == KLFERR_PROGERR_LATEX)
    s = KLFProgErr::extractLatexError(errmsg);
  if (s.length() > 800)
    s = tr("LaTeX error, click 'Run LaTeX' to see error message.", "[[real-time preview tooltip]]");
  K->u->lblOutput->displayError(s, /*labelenabled:*/false);
  slotSetViewControlsEnabled(false);
  slotSetSaveControlsEnabled(false);

  emit K->previewRealTimeError(errmsg, errcode);
}


QVariantMap KLFMainWinPrivate::collectUserScriptInput() const
{
  // check for user script input
  QWidget * scriptInputWidget = K->u->stkScriptInput->currentWidget();
  if (scriptInputWidget == K->u->wScriptInputEmptyPage)
    return QVariantMap();

  QVariantMap data;
  // there's input for the user script
  QList<QWidget*> inwidgets = scriptInputWidget->findChildren<QWidget*>(QRegExp("^INPUT_.*"));
  Q_FOREACH (QWidget *inw, inwidgets) {
    QString n = inw->objectName();
    KLF_ASSERT_CONDITION(n.startsWith("INPUT_"), "?!? found child widget "<<n<<" that is not INPUT_*?",
			 continue; ) ;
    n = n.mid(strlen("INPUT_"));
    // get the user property
    QByteArray userPropName;
    if (inw->inherits("KLFLatexEdit")) {
      userPropName = "plainText";
    } else {
      QMetaProperty userProp = inw->metaObject()->userProperty();
      KLF_ASSERT_CONDITION(userProp.isValid(), "user property of widget "<<n<<" invalid!", continue ; ) ;
      userPropName = userProp.name();
    }
    QVariant value = inw->property(userPropName.constData());
    data[n] = value;
  }

  klfDbg("userscriptinput is "<<data) ;
  return data;
}


KLFBackend::klfInput KLFMainWinPrivate::collectInput(bool final)
{
  KLFBackend::klfInput input = K->currentInputState();

  // remember the math mode if the user clicked on 'Evaluate' (final==true)
  if (final && K->u->cbxMathMode->findText(input.mathmode) == -1) {
    K->u->cbxMathMode->addItem(input.mathmode);
    klfconfig.UI.customMathModes = QStringList() << klfconfig.UI.customMathModes() << input.mathmode;
  }

  return input;
}

void KLFMainWin::slotEvaluate()
{
  KLF_DEBUG_TIME_BLOCK(KLF_FUNC_NAME) ;

  // KLFBackend input
  KLFBackend::klfInput input;

  u->btnEvaluate->setEnabled(false); // don't allow user to click us while we're not done, and
  //				     additionally this gives visual feedback to the user

  input = d->collectInput(true);

  // this accounts for both user script configuration and overriding of bbox margins
  KLFBackend::klfSettings settings = currentSettings();

  // ****  and GO !
  d->output = KLFBackend::getLatexFormula(input, settings);
  // ****

  // for 9.08 <= gs <= 9.14
  if (d->output.status == KLFERR_GSPOSTPROC_NOOUTLINEFONTS) {
    QMessageBox mbox(this);
    mbox.setIcon(QMessageBox::Warning);
    mbox.setWindowTitle(tr("No Font Outlining"));
    mbox.setText(tr("Your version of ghostscript does not allow fonts to be outlined."));
    mbox.setInformativeText(
        tr("This means that vector formats may not display well when pasted or dropped into other programs. "
           "I'll continue now without font outlining. To get font outlining, update or downgrade your "
           "ghostscript to gs <= 9.07 or gs >= 9.15. To suppress this warning, disable font outlining in "
           "\"Settings->Advanced->LaTeX & Image\".")
        );
    mbox.addButton(QMessageBox::Ok);
    mbox.exec();

    // re-run without font outlines
    settings.outlineFonts = false;
    d->output = KLFBackend::getLatexFormula(input, settings);
  }

  if (d->output.status < 0) {
    QString comment = "";
    bool showSettingsPaths = false;
    if (d->output.status == KLFERR_LATEX_NORUN ||
	d->output.status == KLFERR_DVIPS_NORUN ||
	d->output.status == KLFERR_GSBBOX_NORUN ||
        d->output.status == KLFERR_GSPOSTPROC_NORUN ||
        d->output.status == KLFERR_GSPNG_NORUN ||
        d->output.status == KLFERR_GSPDF_NORUN ||
        d->output.status == KLFERR_GSSVG_NORUN) {
      comment = "\n"+tr("Are you sure you configured your system paths correctly in the settings dialog ?");
      showSettingsPaths = true;
    }
    QMessageBox::critical(this, tr("Error"), d->output.errorstr+comment);
    u->lblOutput->displayClear();
    d->slotSetViewControlsEnabled(false);
    d->slotSetSaveControlsEnabled(false);
    if (showSettingsPaths) {
      d->mSettingsDialog->show();
      d->mSettingsDialog->showControl(KLFSettings::ExecutablePaths);
    }
  }
  if (d->output.status > 0) {
    QString s = d->output.errorstr;
    if (d->output.status == KLFERR_PROGERR_LATEX) {
      s = KLFProgErr::extractLatexError(d->output.errorstr) + "<br/><br/>" + s;
    }
    KLFProgErr::showError(this, s);
    d->slotSetViewControlsEnabled(false);
    d->slotSetSaveControlsEnabled(false);
  }
  if (d->output.status == 0) {
    // ALL OK
    d->evaloutput_uptodate = true;

    // scale to view size (klfconfig.UI.labelOutputFixedSize)
    QSize fsize = d->calcPreviewSize();
    QImage scimg = d->output.result;
    if (d->output.result.width() > fsize.width() ||
        d->output.result.height() > fsize.height()) {
      scimg = d->output.result.scaled(fsize, Qt::KeepAspectRatio, Qt::SmoothTransformation);
    }
    QImage tooltipimg = d->output.result;
    if ( klfconfig.UI.previewTooltipMaxSize != QSize(0, 0) ) { // QSize(0,0) meaning no resize
      QSize tooltipmaxsize = (QSizeF(klfconfig.UI.previewTooltipMaxSize) * devicePixelRatioF()).toSize();
      if ( tooltipimg.width() > tooltipmaxsize.width() ||
	   tooltipimg.height() > tooltipmaxsize.height() ) {
        tooltipimg = d->output.result.scaled(tooltipmaxsize, Qt::KeepAspectRatio, Qt::SmoothTransformation);
      }
    }

    emit evaluateFinished(d->output);

    u->lblOutput->display(scimg, tooltipimg, true);

    //    u->frmOutput->setEnabled(true);
    d->slotSetViewControlsEnabled(true);
    d->slotSetSaveControlsEnabled(true);

    QImage smallPreview = d->output.result;
    if (d->output.result.width() < klfconfig.UI.smallPreviewSize().width() &&
	d->output.result.height() < klfconfig.UI.smallPreviewSize().height()) {
      // OK, keep full sized image
    } else if (scimg.size() == klfconfig.UI.smallPreviewSize) {
      // use already-scaled image, don't scale it twice !
      smallPreview = scimg;
    } else {
      // scale to small-preview size
      smallPreview = d->output.result.scaled(klfconfig.UI.smallPreviewSize, Qt::KeepAspectRatio,
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
    int num = d->mHistoryLibResource->query(d->mHistoryLibResource->defaultSubResource(), query, &queryResult);
    if (num >= 1) {
      KLFLibEntry e = queryResult.rawEntryList[0];
      if (e.latex() == newentry.latex() &&
	  e.category() == newentry.category() &&
	  e.tags() == newentry.tags() &&
	  e.style() == newentry.style())
	needInsertThisEntry = false;
    }

    if (needInsertThisEntry) {
      int eid = d->mHistoryLibResource->insertEntry(newentry);
      bool result = (eid >= 0);
      if ( ! result && d->mHistoryLibResource->locked() ) {
	int r = QMessageBox::warning(this, tr("Warning"),
				     tr("Can't add the item to history library because the history "
					"resource is locked. Do you want to unlock it?"),
				     QMessageBox::Yes|QMessageBox::No, QMessageBox::Yes);
	if (r == QMessageBox::Yes) {
	  d->mHistoryLibResource->setLocked(false);
	  result = d->mHistoryLibResource->insertEntry(newentry); // retry inserting entry
	}
      }
      uint fl = d->mHistoryLibResource->supportedFeatureFlags();
      if ( ! result && (fl & KLFLibResourceEngine::FeatureSubResources) &&
	   (fl & KLFLibResourceEngine::FeatureSubResourceProps) &&
	   d->mHistoryLibResource->subResourceProperty(d->mHistoryLibResource->defaultSubResource(),
						       KLFLibResourceEngine::SubResPropLocked).toBool() ) {
	int r = QMessageBox::warning(this, tr("Warning"),
				     tr("Can't add the item to history library because the history "
					"sub-resource is locked. Do you want to unlock it?"),
				     QMessageBox::Yes|QMessageBox::No, QMessageBox::Yes);
	if (r == QMessageBox::Yes) {
	  d->mHistoryLibResource->setSubResourceProperty(d->mHistoryLibResource->defaultSubResource(),
							 KLFLibResourceEngine::SubResPropLocked, false);
	  result = d->mHistoryLibResource->insertEntry(newentry); // retry inserting entry
      }
      }
      if ( ! result && d->mHistoryLibResource->isReadOnly() ) {
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
  emit inputCleared();
  u->txtLatex->clearLatex();
  emit userActivity();
}
void KLFMainWin::slotClearAll()
{
  emit inputCleared();
  slotClearLatex();
  loadDefaultStyle();
  emit userActivity();
}


void KLFMainWin::slotToggleLibrary()
{
  slotLibrary(!d->mLibBrowser->isVisible());
}
void KLFMainWin::slotLibrary(bool showlib)
{
  klfDbg("showlib="<<showlib) ;
  d->mLibBrowser->setVisible(showlib);
  if (showlib) {
    // already shown, raise the window
    d->mLibBrowser->activateWindow();
    d->mLibBrowser->raise();
  }
}

void KLFMainWin::slotToggleSymbols()
{
  slotSymbols(!d->mLatexSymbols->isVisible());
}
void KLFMainWin::slotSymbols(bool showsymbs)
{
  d->mLatexSymbols->setVisible(showsymbs);
  d->slotSymbolsButtonRefreshState(showsymbs);
  d->mLatexSymbols->activateWindow();
  d->mLatexSymbols->raise();
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
  if (latex == u->txtLatex->latex())
    return;

  u->txtLatex->setLatex(latex);
  emit userActivity();
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

void KLFMainWin::slotSetUserScript(const QString& userScript)
{
  KLF_DEBUG_BLOCK(KLF_FUNC_NAME) ;

  if (userScript.isEmpty()) {
    if (u->cbxUserScript->currentIndex() == 0)
      return;
    u->cbxUserScript->setCurrentIndex(0);
    d->slotUserScriptSet(0);
    return;
  }

  bool isscriptname = !userScript.contains(QString()+KLF_DIR_SEP);
  QString userscript = userScript;
  if (!isscriptname)
    userscript = QFileInfo(userScript).canonicalFilePath();
  int k;
  bool ok = false;
  for (k = 0; k < u->cbxUserScript->count(); ++k) {
    QFileInfo ufi(u->cbxUserScript->itemData(k).toString());
    if ( (isscriptname && ufi.fileName() == userscript) ||
	 (!isscriptname && ufi.canonicalFilePath() == userscript) ) {
      // found
      ok = true;
      u->cbxUserScript->setCurrentIndex(k);
      d->slotUserScriptSet(k);
      break;
    }
  }
  if (!ok) {
    QMessageBox::warning(this, tr("User Script Not Available"),
			 tr("The user script %1 is not available.")
			 .arg(userScript));
    klfWarning("Can't find user script "<<userScript);
  }
  
  //   // append our new item
  //   KLFUserScriptInfo us(userScript);
  //   if (us.scriptInfoError()) {
  //     klfWarning("Can't get info for user script "<<userScript<<": "<<us.scriptInfoErrorString()) ;
  //     return;
  //   }
  //   u->cbxUserScript->addItem(us.name(), QVariant(userScript));
  //   k = u->cbxUserScript->count()-1;
  //   u->cbxUserScript->setCurrentIndex(k);
  //   // not called by the slot connected to the combo box (??)
  //   d->slotUserScriptSet(k);
}

void KLFMainWin::slotShowUserScriptLog()
{
  KLFProgErr::showError(this, KLFUserScriptFilterProcess::getUserScriptLogHtml());
}

void KLFMainWin::slotReloadUserScripts()
{
  KLF_DEBUG_BLOCK(KLF_FUNC_NAME) ;

  QString curUserScript = u->cbxUserScript->itemData(u->cbxUserScript->currentIndex()).toString();

  klf_reload_user_scripts();

  QStringList userscripts = klf_user_scripts;
  klfDbg("userscripts: "<<userscripts) ;
  u->cbxUserScript->clear();
  u->cbxUserScript->addItem(tr("<none>", "[[no user script]]"), QVariant(QString()));
  for (int kkl = 0; kkl < userscripts.size(); ++kkl) {
    KLFUserScriptInfo scriptinfo = KLFUserScriptInfo::forceReloadScriptInfo( userscripts[kkl] );
    klfDbg("Considering userscript "<<userscripts[kkl]<<", category="<<scriptinfo.category()) ;
    if (scriptinfo.category() == QLatin1String("klf-backend-engine")) {
      u->cbxUserScript->addItem(scriptinfo.name(), QVariant(userscripts[kkl]));
    }
  }

  slotSetUserScript(curUserScript);
}

void KLFMainWin::slotEnsurePreambleCmd(const QString& line)
{
  QTextCursor c = u->txtPreamble->textCursor();
  //  qDebug("Begin preamble edit: preamble text is %s", qPrintable(u->txtPreamble->latex()));
  c.beginEditBlock();
  c.movePosition(QTextCursor::End);

  QString preambletext = u->txtPreamble->latex();
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
  if ( u->txtLatex->latex().isEmpty() ) {
    qWarning()<<KLF_FUNC_NAME<<": Can't evaluate: empty input";
    return;
  }

  slotEvaluate();

  if ( ! output.isEmpty() ) {
    if ( d->output.result.isNull() ) {
      QMessageBox::critical(this, tr("Error"), tr("There is no image to save."));
    } else {
      KLFBackend::saveOutputToFile(d->output, output, format.trimmed().toUpper());
    }
  }

  emit userActivity();
}


void KLFMainWin::pasteLatexFromClipboard(QClipboard::Mode mode)
{
  QString cliptext = QApplication::clipboard()->text(mode);
  slotSetLatex(cliptext);
  // already in slotSetLatex()
  //  emit userActivity();
}



QList<KLFExporter*> KLFMainWin::registeredExporters()
{
  return d->pExporterManager->exporterList();
}
QList<KLFAbstractDataOpener*> KLFMainWin::registeredDataOpeners()
{
  return d->pDataOpeners;
}

QList<KLFMimeExportProfile> KLFMainWin::mimeExportProfileList()
{
  return d->pMimeExportProfileManager.exportProfileList();
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
  for (k = 0; k < d->pDataOpeners.size(); ++k) {
    if (d->pDataOpeners[k]->canOpenFile(fileName)) {
      return true;
    }
  }
  klfDbg("cannot open file.") ;
  return false;
}
bool KLFMainWin::canOpenData(const QByteArray& data)
{
  KLF_DEBUG_BLOCK(KLF_FUNC_NAME) ;
  klfDbg("data. length="<<data.size()) ;
  int k;
  for (k = 0; k < d->pDataOpeners.size(); ++k) {
    if (d->pDataOpeners[k]->canOpenData(data)) {
      return true;
    }
  }
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
  for (k = 0; k < d->pDataOpeners.size(); ++k) {
    mimetype = find_list_agreement(formats, d->pDataOpeners[k]->supportedMimeTypes());
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

  emit inputCleared();

  int k;
  for (k = 0; k < d->pDataOpeners.size(); ++k) {
    if (d->pDataOpeners[k]->openFile(file)) {
      emit fileOpened(file, d->pDataOpeners[k]);
      return true;
    }
  }

  QMessageBox::critical(this, tr("Error"), tr("Failed to load file %1.").arg(file));

  slotClearLatex();

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

  emit inputCleared();

  QString mimetype;
  QStringList fmts = mimeData->formats();

  for (k = 0; k < d->pDataOpeners.size(); ++k) {
    mimetype = find_list_agreement(fmts, d->pDataOpeners[k]->supportedMimeTypes());
    if (!mimetype.isEmpty()) {
      // mime types intersect.
      klfDbg("Opening mimetype "<<mimetype) ;
      QByteArray data = mimeData->data(mimetype);
      if (d->pDataOpeners[k]->openData(data, mimetype)) {
	emit dataOpened(mimetype, data, d->pDataOpeners[k]);
	return OpenDataOk;
      }
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

  emit inputCleared();

  int k;
  for (k = 0; k < d->pDataOpeners.size(); ++k) {
    if (d->pDataOpeners[k]->openData(data, QString())) {
      emit dataOpened(QString(), data, d->pDataOpeners[k]);
      return true;
    }
  }

  klfWarning("failed to open data.") ;
  slotClearLatex();

  return false;
}

bool KLFMainWin::canOpenDropData(const QMimeData * data)
{
  return canOpenData(data);
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
    return d->mLibBrowser->openResource(url);
  }
  klfDbg("subreslist is "<<subreslist);
  bool loaded = false;
  int k;
  for (k = 0; k < subreslist.size(); ++k) {
    QUrl url2 = url;
    QUrlQuery url2q(url2);
    url2q.addQueryItem("klfDefaultSubResource", subreslist[k]);
    url2.setQuery(url2q);
    bool thisloaded =  d->mLibBrowser->openResource(url2);
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


void KLFMainWin::slotDrag()
{
  // abort if we don't have anything to copy/drag. Use btnDrag's enabled status as a witness
  // for that. (not the best idea, but whatever....)
  if (!u->btnDrag->isEnabled()) {
    klfDbg("Nothing to copy.") ;
    return;
  }

  if ( d->output.result.isNull() )
    return;


  void aboutToDragData();

  QDrag *drag = new QDrag(this);
  KLFMimeData *mime = new KLFMimeData(
      d->pMimeExportProfileManager.findExportProfile(klfconfig.ExportData.dragExportProfile),
      d->pExporterManager,
      d->output
      );

//  connect(mime, SIGNAL(droppedData(const QString&)), this, SIGNAL(draggedDataWasDropped(const QString&)));

  drag->setMimeData(mime);

  QSize sz = QSize(200, 100);
  QImage img = d->output.result;
  if (img.width() > sz.width() || img.height() > sz.height())
    img = img.scaled(sz, Qt::KeepAspectRatio, Qt::SmoothTransformation);
  // imprint the export type on the drag pixmap
  QString exportProfileText = d->pMimeExportProfileManager.findExportProfile(
      klfconfig.ExportData.dragExportProfile
      ).description();
  { QPainter painter(&img);
    QFont smallfont = QFont("Helvetica", 6);
    smallfont.setPixelSize(11);
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
  KLF_DEBUG_BLOCK(KLF_FUNC_NAME) ;

  // abort if we don't have anything to copy. Use btnCopy's enabled status as a witness
  // for that. (not the best idea, but whatever....)
  if (!u->btnCopy->isEnabled()) {
    klfDbg("Nothing to copy.") ;
    return;
  }

  emit aboutToCopyData();

  KLFMimeData *mimedata = new KLFMimeData(
      d->pMimeExportProfileManager.findExportProfile(klfconfig.ExportData.copyExportProfile),
      d->pExporterManager,
      d->output
      );

  klfDbg("created mime data object.") ;

  // On Mac, Qt's clipboard support doesn't allow to query the data later and
  // gets all data now (that works only for drag & drop) (WHY?!?!?!?!?!?).  But
  // I can't see any other solution...
  QApplication::clipboard()->setMimeData(mimedata, QClipboard::Clipboard);

  klfDbg("set MIME data.") ;

  KLFMimeExportProfile profile = d->pMimeExportProfileManager.findExportProfile(
      klfconfig.ExportData.copyExportProfile
      );
  d->showExportMsgLabel(tr("Copied as <b>%1</b>").arg(profile.description()));

  emit copiedData(profile.profileName());
}

void KLFMainWin::slotSave(const QString& suggestfname)
{
  // make sure we are allowed to save
  if (!u->btnSave->isEnabled()) {
    // we are not allowed to save!
    klfWarning("There is nothing to save!") ;
    return;
  }

  // notify that we are about to save to file
  emit aboutToSaveAs();

  // application-long persistent selectedfilter
  static QString selectedfilter;


  // prepare list of filters & available formats

  QStringList filterformatlist;
  QMap<QString,QString> formatsByFilterName;
  QMap<QString,KLFExporter*> exporterByFilterName;

  int k, j, ll;

  QList<KLFExporter*> exporters = d->pExporterManager->exporterList();

  for (k = 0; k < exporters.size(); ++k) {
    KLFExporter * exporter = exporters[k];
    if (!exporter->isSuitableForFileSave()) {
      continue;
    }
    QStringList xoformats = exporter->supportedFormats(d->output);
    for (j = 0; j < xoformats.size(); ++j) {
      QString fmt = xoformats[j];
      QStringList exts = exporter->fileNameExtensionsFor(fmt);
      QString patterns;
      for (ll = 0; ll < exts.size(); ++ll) {
        if (ll > 0) {
          patterns += " ";
        }
        patterns += "*." + exts[ll].toLower();
      }
      QString f = QString("%1 (%2)").arg(exporter->titleFor(fmt)).arg(patterns);
      filterformatlist.push_back(f);
      formatsByFilterName[f] = fmt;
      exporterByFilterName[f] = exporter;
    }
  }

  // finally, join all filters together and show the file dialog.

  QString filterformat = filterformatlist.join(";;");
  QString fname, format;

  QString suggestion = suggestfname;
  if (suggestion.isEmpty()) {
    suggestion = klfconfig.UI.lastSaveDir;
  }
  if (suggestion.isEmpty()) {
    QStringList docpaths = QStandardPaths::standardLocations(QStandardPaths::DocumentsLocation);
    if (docpaths.isEmpty()) {
      suggestion = "";
    } else {
      suggestion = docpaths[0];
    }
  }

  fname = QFileDialog::getSaveFileName(this, tr("Save Latex Output"), suggestion, filterformat,
				       &selectedfilter);

  if (fname.isEmpty()) {
    return;
  }

  QFileInfo fi(fname);
  // save this path as default to suggest next time
  klfconfig.UI.lastSaveDir = fi.absolutePath();

  // first test if it's an extern-format
  if ( exporterByFilterName.contains(selectedfilter) ) {
    // use an external output-saver
    QString formatname = formatsByFilterName[selectedfilter];
    KLFExporter * exporter = exporterByFilterName[selectedfilter];

    if (exporter == NULL) {
      klfWarning("Internal error: exporter is NULL!");
      return;
    }

    QByteArray data = exporter->getData(formatname, d->output);
    if (data.isEmpty()) {
      QMessageBox::critical(this, tr("Error saving file"),
                            tr("Error exporting the data: %1").arg(exporter->errorString()));
      return;
    }
    {
      QFile fout(fname);
      bool ok = fout.open(QIODevice::WriteOnly);
      if (!ok) {
        QMessageBox::critical(this, tr("Error saving file"),
                              tr("Can't write to file %1: %2").arg(fname).arg(fout.errorString()));
        return;
      }
      fout.write(data);
      fout.close();
    }
    
    emit savedToFile(fname, formatname, exporter);
    return;
  }

  klfWarning("Invalid selected filter: " << selectedfilter << "!") ;
  QMessageBox::critical(this, tr("Error saving file"),
                        tr("Internal error: can't determine save format"));
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



void KLFMainWinPrivate::slotPresetDPISender()
{
  QAction *a = qobject_cast<QAction*>(sender());
  KLF_ASSERT_NOT_NULL(a, "Action sender is NULL!", return ; ) ;
  
  K->slotSetDPI(a->data().toInt());
}


bool KLFMainWin::isExpandedMode() const
{
  return u->frmDetails->sideWidgetVisible();
}

KLFStyle KLFMainWin::currentStyle() const
{
  KLF_DEBUG_BLOCK(KLF_FUNC_NAME) ;

  KLFStyle sty;

  sty.name = QString::null;
  int idx = u->cbxLatexFont->currentIndex();
  if (idx >= 0) {
    sty.fontname = u->cbxLatexFont->itemData(idx).toString();
  } else {
    sty.fontname = QString();
  }

  sty.fg_color = u->colFg->color().rgb();
  QColor bgc = u->colBg->color();
  if (bgc.isValid())
    sty.bg_color = qRgba(bgc.red(), bgc.green(), bgc.blue(), 255);
  else
    sty.bg_color = qRgba(255, 255, 255, 0);
  sty.mathmode = (u->chkMathMode->isChecked() ? u->cbxMathMode->currentText() : "...");
  sty.preamble = u->txtPreamble->latex();
  sty.fontsize = u->spnLatexFontSize->value();
  if (sty.fontsize < 0.001) sty.fontsize = -1;
  sty.dpi = u->spnDPI->value();
  sty.vectorscale = 1.0;
  if (u->chkVectorScale->isChecked())
    sty.vectorscale = u->spnVectorScale->value() / 100.0;

  if (u->gbxOverrideMargins->isChecked()) {
    KLFStyle::BBoxExpand bbox(
        u->spnMarginTop->valueInRefUnit(),
        u->spnMarginRight->valueInRefUnit(),
        u->spnMarginBottom->valueInRefUnit(),
        u->spnMarginLeft->valueInRefUnit()
        );
    sty.overrideBBoxExpand = bbox;
  }

  sty.userScript = QFileInfo(u->cbxUserScript->itemData(u->cbxUserScript->currentIndex()).toString()).fileName();
  sty.userScriptInput = d->collectUserScriptInput();

  klfDbg("Returning style; bgcol="<<klfFmtCC("#%08lx", (ulong)sty.bg_color)<<": us="<<sty.userScript()) ;
  klfDbg("full style="<< sty << "; override bbox expand = " << sty.overrideBBoxExpand) ;

  return sty;
}

KLFBackend::klfSettings KLFMainWin::backendSettings() const
{
  return d->settings;
}

KLFBackend::klfSettings KLFMainWin::currentSettings() const
{
  KLF_DEBUG_BLOCK(KLF_FUNC_NAME) ;

  KLFBackend::klfSettings settings = d->settings;
  // see if we need to override settings
  if (u->gbxOverrideMargins->isChecked()) {
    klfDbg("overriding settings margins.") ;
    settings.tborderoffset = u->spnMarginTop->valueInRefUnit();
    settings.rborderoffset = u->spnMarginRight->valueInRefUnit();
    settings.bborderoffset = u->spnMarginBottom->valueInRefUnit();
    settings.lborderoffset = u->spnMarginLeft->valueInRefUnit();
  }

  klfDbg("settings.execenv = "<<settings.execenv);

  klfDbg("backendsettings.setTexInputs = "<<klfconfig.BackendSettings.setTexInputs());
  if (!klfconfig.BackendSettings.setTexInputs().isEmpty()) {
    klfDbg("old environment is"<<settings.execenv);
    QStringList newitems = klfSplitEnvironmentPath(klfconfig.BackendSettings.setTexInputs);
    klfSetEnvironmentPath(&settings.execenv,
			  newitems,
			  QLatin1String("TEXINPUTS"),
			  KlfEnvPathPrepend|KlfEnvPathNoDuplicates);
    klfDbg("added "<<newitems<<" to TEXINPUTS, new environment is "<<settings.execenv) ;
  }
  // also add the userscripts paths (to expose our `pyklfuserscript` utility or any users'
  // library) to the PYTHONPATH for python scripts
  QStringList pypaths;
  pypaths << klfconfig.globalShareDir+"/userscripts"
          << klfconfig.homeConfigDirUserScripts ;
  klfSetEnvironmentPath(&settings.execenv, pypaths, QLatin1String("PYTHONPATH"),
                        KlfEnvPathPrepend|KlfEnvPathNoDuplicates);
  klfDbg("Added "<<pypaths<<" to PYTHONPATHS for user scripts to access our python libraries") ;

  klfDbg("now settings.execenv = "<<settings.execenv);

  KLFBackend::klfInput input = currentInputState();
  // setup user script configuration
  if (!input.userScript.isEmpty()) {
    QString usfn = KLFUserScriptInfo(input.userScript).userScriptPath();
    if (klfconfig.UserScripts.userScriptConfig.contains(usfn)) {
      QVariantMap data = klfconfig.UserScripts.userScriptConfig[usfn];
      QMap<QString,QString> mdata;
      for (QVariantMap::const_iterator it = data.begin(); it != data.end(); ++it)
	mdata[QLatin1String("KLF_USCONFIG_") + it.key()] = klfSaveVariantToText(it.value(), true);
      klfMergeEnvironment(&settings.execenv, klfMapToEnvironmentList(mdata));
    }
  }
  
  klfDbg("Full environment (w/ userscript config) is "<<settings.execenv) ;

  return settings;
}

KLFBackend::klfOutput KLFMainWin::currentKLFBackendOutput() const
{
  return d->output;
}

KLFBackend::klfInput KLFMainWin::currentInputState() const
{
  // KLFBackend input
  KLFBackend::klfInput input;

  if (u->chkVectorScale->isChecked())
    input.vectorscale = u->spnVectorScale->value() / 100.0;
  else
    input.vectorscale = 1.0;

  input.latex = u->txtLatex->latex();
  klfDbg("latex="<<input.latex) ;

  input.userScript = u->cbxUserScript->itemData(u->cbxUserScript->currentIndex()).toString();

  QVariantMap userScriptInput = d->collectUserScriptInput();
  for (QVariantMap::const_iterator usparam = userScriptInput.begin();
       usparam != userScriptInput.end();
       ++usparam) {
    input.userScriptParam[usparam.key()] = usparam.value().toString();
  }

  if (u->chkMathMode->isChecked()) {
    input.mathmode = u->cbxMathMode->currentText();
  } else {
    input.mathmode = "...";
  }
  int fsval = u->spnLatexFontSize->value();
  if (fsval <= 0)
    input.fontsize = -1;
  else
    input.fontsize = fsval;
  input.preamble = u->txtPreamble->latex();
  if (u->cbxLatexFont->isEnabled()) {
    int idx = u->cbxLatexFont->currentIndex();
    if (idx > 0) {
      // find corresponding font in list
      int k;
      for (k = 0; k < d->pLatexFontDefs.size(); ++k) {
        if (d->pLatexFontDefs[k].identifier == u->cbxLatexFont->itemData(idx).toString()) {
          input.preamble += d->pLatexFontDefs[k].latexdefs;
          break;
        }
      }
      if (k == d->pLatexFontDefs.size()) {
        klfWarning("Couldn't find font "<<u->cbxLatexFont->currentText()<<" !");
      }
    }
  }
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



QFont KLFMainWin::txtLatexFont() const
{
  return u->txtLatex->font();
}
QFont KLFMainWin::txtPreambleFont() const
{
  return u->txtPreamble->font();
}

QString KLFMainWin::currentInputLatex() const
{
  return u->txtLatex->latex();
}

void KLFMainWinPrivate::slotLoadStyleAct()
{
  QAction *a = qobject_cast<QAction*>(sender());
  if (a) {
    K->slotLoadStyle(a->data().toInt());
  }
}

void KLFMainWinPrivate::slotDetailsSideWidgetShown(bool shown)
{
  if (K->u->frmDetails->sideWidgetManagerType() == QLatin1String("ShowHide")) {
    if (shown)
      K->u->btnExpand->setIcon(QIcon(":/pics/switchshrinked.svg"));
    else
      K->u->btnExpand->setIcon(QIcon(":/pics/switchexpanded.svg"));
  } else if (K->u->frmDetails->sideWidgetManagerType() == QLatin1String("Drawer")) {
    if (shown)
      K->u->btnExpand->setIcon(QIcon(":/pics/switchshrinked_drawer.svg"));
    else
      K->u->btnExpand->setIcon(QIcon(":/pics/switchexpanded_drawer.svg"));
  } else {
    // "Float", or any possible custom side widget type
    if (shown)
      K->u->btnExpand->setIcon(QIcon(":/pics/hidetoolwindow.png"));
    else
      K->u->btnExpand->setIcon(QIcon(":/pics/showtoolwindow.png"));
  }
}

void KLFMainWin::slotLoadStyle(const KLFStyle& style)
{
  KLF_DEBUG_BLOCK(KLF_FUNC_NAME) ;
  klfDbg("style: "<<style) ;

  QColor cfg, cbg;
  cfg.setRgba(style.fg_color);
  cbg.setRgba(style.bg_color);
  u->colFg->setColor(cfg);
  if (cbg.alpha() < 100)
    cbg = QColor(); // transparent
  klfDbg("setting color: "<<cbg) ;
  u->colBg->setColor(cbg);
  u->chkMathMode->setChecked(style.mathmode().simplified() != "...");
  if (style.mathmode().simplified() != "...")
    u->cbxMathMode->setEditText(style.mathmode);
  if (style.fontsize < 0.001) {
    u->spnLatexFontSize->setValue(0);
  } else {
    u->spnLatexFontSize->setValue((int)(style.fontsize + 0.5));
  }
  slotSetPreamble(style.preamble); // to preserve text edit undo/redo, don't use txtPreamble->setText()
  u->spnDPI->setValue(style.dpi);
  u->spnVectorScale->setValue(style.vectorscale * 100.0);
  u->chkVectorScale->setChecked(fabs(style.vectorscale - 1.0) > 0.00001);

  int idx;
  for (idx = 0; idx < u->cbxLatexFont->count(); ++idx) {
    if (u->cbxLatexFont->itemData(idx).toString() == style.fontname) {
      u->cbxLatexFont->setCurrentIndex(idx);
      break;
    }
  }
  if (idx == u->cbxLatexFont->count()) {
    // didn't find font
    QMessageBox::warning(this, tr("Can't find font"), tr("Sorry, I don't know about font `%1'.").arg(style.fontname));
  }


  if (style.overrideBBoxExpand().valid()) {
    u->gbxOverrideMargins->setChecked(true);
    u->spnMarginTop->setValueInRefUnit(style.overrideBBoxExpand().top);
    u->spnMarginRight->setValueInRefUnit(style.overrideBBoxExpand().right);
    u->spnMarginBottom->setValueInRefUnit(style.overrideBBoxExpand().bottom);
    u->spnMarginLeft->setValueInRefUnit(style.overrideBBoxExpand().left);
  } else {
    u->gbxOverrideMargins->setChecked(false);
  }

  klfDbg("About to load us="<<style.userScript()) ;
  slotSetUserScript(style.userScript());

  // initialize the user script input widget
  // check for user script input
  QWidget * scriptInputWidget = u->stkScriptInput->currentWidget();
  if (scriptInputWidget != u->wScriptInputEmptyPage) {
    QVariantMap data = style.userScriptInput();
    // find the input widgets
    QList<QWidget*> inwidgets = scriptInputWidget->findChildren<QWidget*>(QRegExp("^INPUT_.*"));
    Q_FOREACH (QWidget *inw, inwidgets) {
      QString n = inw->objectName();
      KLF_ASSERT_CONDITION(n.startsWith("INPUT_"), "?!? found child widget "<<n<<" that is not INPUT_*?",
			   continue; ) ;
      n = n.mid(strlen("INPUT_"));
      // find the user property
      QByteArray userPropName;
      if (inw->inherits("KLFLatexEdit")) {
	userPropName = "plainText";
      } else {
	QMetaProperty userProp = inw->metaObject()->userProperty();
	KLF_ASSERT_CONDITION(userProp.isValid(), "user property of widget "<<n<<" invalid!", continue ; ) ;
	userPropName = userProp.name();
      }
      // find this widget in our list of input values
      KLF_ASSERT_CONDITION(data.contains(n), "No value given for input widget "<<n, continue ; );
      QVariant value = data[n];
      inw->setProperty(userPropName.constData(), value);
    }
  }

}

void KLFMainWin::slotLoadStyle(int n)
{
  if (n < 0 || n >= (int)d->styles.size())
    return; // let's not try to load an inexistant style...

  slotLoadStyle(d->styles[n]);
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
  for (int kl = 0; found_i == -1 && kl < d->styles.size(); ++kl) {
    if (d->styles[kl].name().trimmed() == name.trimmed()) {
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
    d->styles.append(sty);
  else
    d->styles[found_i] = sty;

  d->refreshStylePopupMenus();

  // auto-save our style list
  saveStyles();

  emit stylesChanged();
}

void KLFMainWin::slotSaveStyleAsDefault()
{
  KLFStyle sty;

  sty = currentStyle();
  QString defname = tr("Default", "[[style name]]");
  sty.name = defname;

  // check to see if style exists already (it should)
  int found_i = -1;
  for (int kl = 0; found_i == -1 && kl < d->styles.size(); ++kl) {
    if (d->styles[kl].name().trimmed() == defname.trimmed())
      found_i = kl;
  }
  if (found_i == -1)
    d->styles.append(sty);
  else
    d->styles[found_i] = sty;

  d->refreshStylePopupMenus();

  // auto-save our style list
  saveStyles();

  emit stylesChanged();
}

void KLFMainWin::slotStyleManager()
{
  d->mStyleManager->show();
}


void KLFMainWin::slotSettings()
{
  d->mSettingsDialog->show();
}


void KLFMainWinPrivate::slotSetViewControlsEnabled(bool enabled)
{
  emit K->userViewControlsActive(enabled);
  K->u->btnShowBigPreview->setEnabled(enabled);
}

void KLFMainWinPrivate::slotSetSaveControlsEnabled(bool enabled)
{
  emit K->userSaveControlsActive(enabled);
  K->u->btnSetExportProfile->setEnabled(enabled);
  K->u->btnDrag->setEnabled(enabled);
  K->u->btnCopy->setEnabled(enabled);
  K->u->btnSave->setEnabled(enabled);
}


void KLFMainWin::closeEvent(QCloseEvent *event)
{
  KLF_DEBUG_BLOCK(KLF_FUNC_NAME) ;

  // close side widget and wait for it
  u->frmDetails->showSideWidget(false);
  u->frmDetails->sideWidgetManager()->waitForShowHideActionFinished();

  if (d->ignore_close_event) {
    // simple hide, not close
    hide();

    event->ignore();
    return;
  }

  event->accept();

  quit();
}




