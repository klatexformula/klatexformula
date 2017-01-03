/***************************************************************************
 *   file klfsettings.cpp
 *   This file is part of the KLatexFormula Project.
 *   Copyright (C) 2011 by Philippe Faist
 *   philippe.faist@bluewin.ch
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

#include <stdlib.h>

#include <QDialog>
#include <QCheckBox>
#include <QSpinBox>
#include <QLineEdit>
#include <QFontDatabase>
#include <QFontDialog>
#include <QString>
#include <QMessageBox>
#include <QFileInfo>
#include <QDir>
#include <QFileDialog>
#include <QWhatsThis>
#include <QResource>
#include <QEvent>
#include <QListView> // QListView::LeftToRight|TopToBottom
#include <QMouseEvent>
#include <QStandardPaths>
#include <QToolBar>
#include <QStyleFactory>

#include <klfcolorchooser.h>
#include <klfpathchooser.h>
#include <klfsysinfo.h>

#include <klfbackend.h>

#include <klfrelativefont.h>
#include <klflatexedit.h>
#include <klfsidewidget.h>
#include <klfprogerr.h>

#include <ui_klfsettings.h>

#include "klfmain.h"
#include "klfmainwin.h"
#include "klfconfig.h"
#include "klfmime.h"
#include "klfsettings.h"
#include "klfsettings_p.h"
#include "klfuiloader.h"



// void klf_show_advanced_config_editor()
// {
//   KLFAdvancedConfigEditor *edit = new KLFAdvancedConfigEditor(NULL, &klfconfig);
//   edit->show();
// }




#define KLFSETTINGS_ROLE_PLUGNAME (Qt::UserRole + 5300)

#define KLFSETTINGS_ROLE_USERSCRIPT (Qt::UserRole + 5500)


#define REG_SH_TEXTFORMATENSEMBLE(x)					\
  { KLFSettingsPrivate::TextFormatEnsemble tfe( & klfconfig.SyntaxHighlighter.fmt##x , \
						u->colSH##x, u->colSH##x##Bg , \
						u->chkSH##x##B , u->chkSH##x##I ); \
    d->textformats.append(tfe); }




KLFSettings::KLFSettings(KLFMainWin* parent)
  : QDialog(parent)
{
  KLF_DEBUG_BLOCK(KLF_FUNC_NAME) ;

  KLF_INIT_PRIVATE(KLFSettings) ;

  u = new Ui::KLFSettings;
  u->setupUi(this);
  setObjectName("KLFSettings");

#ifdef KLF_WS_MAC
  //setAttribute(Qt::WA_MacBrushedMetal, klfconfig.UI.macBrushedMetalLook);
  setAutoFillBackground(true);
#endif

  d->mainWin = parent;

  d->pUserSetDefaultAppFont = false;

  u->cbxLibIconViewFlow->setEnumValues(QList<int>()<<QListView::TopToBottom<<QListView::LeftToRight,
				       QStringList()<<tr("Top to Bottom")<<tr("Left to Right"));

  reset();

  u->btns->clear();

  //   QPushButton *b;
  //   b = new QPushButton(QIcon(":/pics/closehide.png"), QString("cancel"), u->btns);
  //   u->btns->addButton(b, QDialogButtonBox::RejectRole);
  //   connect(b, SIGNAL(clicked()), this, SLOT(reject()));
  //   b = new QPushButton(QIcon(":/pics/apply.png"), QString("apply"), u->btns);
  //   u->btns->addButton(b, QDialogButtonBox::ApplyRole);
  //   connect(b, SIGNAL(clicked()), this, SLOT(apply()));
  //   b = new QPushButton(QIcon(":/pics/ok.png"), QString("ok"), u->btns);
  //   u->btns->addButton(b, QDialogButtonBox::AcceptRole);
  //   connect(b, SIGNAL(clicked()), this, SLOT(accept()));
  QAbstractButton *b;
  b = u->btns->addButton(QDialogButtonBox::Cancel);
  b->setIcon(QIcon(":/pics/closehide.png"));
  connect(b, SIGNAL(clicked()), this, SLOT(reject()));
  b = u->btns->addButton(QDialogButtonBox::Apply);
  b->setIcon(QIcon(":/pics/apply.png"));
  connect(b, SIGNAL(clicked()), this, SLOT(apply()));
  b = u->btns->addButton(QDialogButtonBox::Ok);
  b->setIcon(QIcon(":/pics/ok.png"));
  connect(b, SIGNAL(clicked()), this, SLOT(accept()));

  // ---

  //  QToolBar * toolBar = new QToolBar(wToolBar);
#if defined(KLF_WS_MAC) || defined(KLF_WS_WIN)
  //  u->toolBar->setAttribute(Qt::WA_NoSystemBackground, true);
  //  u->toolBar->setAttribute(Qt::WA_NativeWindow, false);
  u->toolBar->setStyle(QStyleFactory::create("fusion"));
#endif


#define SETTINGS_REGISTER_TAB(x)					\
  u->actionTab##x->setData(QByteArray(#x));				\
  d->settingsTabs[#x] = u->tab##x;					\
  d->actionTabs << u->actionTab##x;					\
  connect(u->actionTab##x, SIGNAL(triggered(bool)), d, SLOT(showTabByNameActionSender()));

  SETTINGS_REGISTER_TAB(Interface);
  SETTINGS_REGISTER_TAB(Latex);
  SETTINGS_REGISTER_TAB(Editor);
  SETTINGS_REGISTER_TAB(Library);
  SETTINGS_REGISTER_TAB(UserScripts);
  SETTINGS_REGISTER_TAB(Advanced);

  // and show the interface tab by default
  d->showTabByName("Interface");

  // ---

  d->populateSettingsCombos();

  // initialize the margin unit selector
  u->cbxEPSBBoxUnits->setCurrentUnitAbbrev("pt");

  connect(u->btnAdvancedEditor, SIGNAL(clicked()), this, SLOT(showAdvancedConfigEditor()));
  connect(u->btnSystemMessages, SIGNAL(clicked()), this, SLOT(showSystemMessages()));

  // set some smaller fonts for small titles
  KLFRelativeFont *relfontSHF = new KLFRelativeFont(this, u->lblSHForeground);
  relfontSHF->setRelPointSize(-2);
  KLFRelativeFont *relfontSHB = new KLFRelativeFont(this, u->lblSHBackground);
  relfontSHB->setRelPointSize(-2);
  KLFRelativeFont *relfontSHBold = new KLFRelativeFont(this, u->lblSHBold);
  relfontSHBold->setRelPointSize(-1);
  relfontSHBold->setForceBold(true);
  KLFRelativeFont *relfontSHItalic = new KLFRelativeFont(this, u->lblSHItalic);
  relfontSHItalic->setRelPointSize(-1);
  relfontSHItalic->setForceStyle(QFont::StyleItalic);
  
  connect(u->btnPathsReset, SIGNAL(clicked()), this, SLOT(setDefaultPaths()));

  connect(u->btnAppFont, SIGNAL(clicked()), d, SLOT(slotChangeFontSender()));
  connect(u->btnEditorFont, SIGNAL(clicked()), d, SLOT(slotChangeFontSender()));
  connect(u->btnPreambleFont, SIGNAL(clicked()), d, SLOT(slotChangeFontSender()));

  // prepare some actions as shortcuts for standard fonts
  QFontDatabase fdb;
  u->aFontCMU->setEnabled( fdb.families().contains(
			       klfconfig.defaultCMUFont.family()
			       ) );
  d->pFontBasePresetActions["CMU"] = u->aFontCMU;
  d->pFontBasePresetActions["TT"] = u->aFontTT;
  d->pFontBasePresetActions["Std"] = u->aFontStd;
  d->pFontButtons["AppFont"] = u->btnAppFont;
  d->pFontButtons["EditorFont"] = u->btnEditorFont;
  d->pFontButtons["PreambleFont"] = u->btnPreambleFont;
  QAction *a = NULL;
  QMenu *fontPresetMenu = NULL;
  QVariantMap vmap;
  // remember: action text/icon/font/... is set in retranslateUi().
  // -- AppFont --
  fontPresetMenu = new QMenu(this);
  a = new QAction(this);
  vmap["Action"] = "CMU";
  vmap["Font"] = klfconfig.defaultCMUFont;
  vmap["Button"] = QVariant("AppFont");
  a->setData(QVariant(vmap));
  a->setEnabled(u->aFontCMU->isEnabled());
  fontPresetMenu->addAction(a);
  connect(a, SIGNAL(triggered()), d, SLOT(slotChangeFontPresetSender()));
  d->pFontSetActions << a;
  a = new QAction(this);
  vmap["Action"] = "Std";
  vmap["Font"] = klfconfig.defaultStdFont;
  vmap["Button"] = QVariant("AppFont");
  vmap["isSystemDefaultAppFont"] = QVariant(true);
  a->setData(QVariant(vmap));
  connect(a, SIGNAL(triggered()), d, SLOT(slotChangeFontPresetSender()));
  d->pFontSetActions << a;
  fontPresetMenu->addAction(a);
  u->btnAppFontChoose->setMenu(fontPresetMenu);
  // -- EditorFont --
  fontPresetMenu = new QMenu(this);
  a = new QAction(this);
  vmap["Action"] = "TT";
  vmap["Font"] = klfconfig.defaultTTFont;
  vmap["Button"] = QVariant("EditorFont");
  a->setData(QVariant(vmap));
  connect(a, SIGNAL(triggered()), d, SLOT(slotChangeFontPresetSender()));
  d->pFontSetActions << a;
  fontPresetMenu->addAction(a);
  a = new QAction(this);
  vmap["Action"] = "CMU";
  vmap["Font"] = klfconfig.defaultCMUFont;
  vmap["Button"] = QVariant("EditorFont");
  a->setData(QVariant(vmap));
  a->setEnabled(u->aFontCMU->isEnabled());
  connect(a, SIGNAL(triggered()), d, SLOT(slotChangeFontPresetSender()));
  d->pFontSetActions << a;
  fontPresetMenu->addAction(a);
  a = new QAction(this);
  vmap["Action"] = "Std";
  vmap["Font"] = klfconfig.defaultStdFont;
  vmap["Button"] = QVariant("EditorFont");
  a->setData(QVariant(vmap));
  connect(a, SIGNAL(triggered()), d, SLOT(slotChangeFontPresetSender()));
  d->pFontSetActions << a;
  fontPresetMenu->addAction(a);
  u->btnEditorFontChoose->setMenu(fontPresetMenu);
  // -- PreambleFont --
  fontPresetMenu = new QMenu(this);
  a = new QAction(this);
  vmap["Action"] = "TT";
  vmap["Font"] = klfconfig.defaultTTFont;
  vmap["Button"] = QVariant("PreambleFont");
  a->setData(QVariant(vmap));
  connect(a, SIGNAL(triggered()), d, SLOT(slotChangeFontPresetSender()));
  d->pFontSetActions << a;
  fontPresetMenu->addAction(a);
  a = new QAction(this);
  vmap["Action"] = "CMU";
  vmap["Font"] = klfconfig.defaultCMUFont;
  vmap["Button"] = QVariant("PreambleFont");
  a->setData(QVariant(vmap));
  a->setEnabled(u->aFontCMU->isEnabled());
  connect(a, SIGNAL(triggered()), d, SLOT(slotChangeFontPresetSender()));
  d->pFontSetActions << a;
  fontPresetMenu->addAction(a);
  a = new QAction(this);
  vmap["Action"] = "Std";
  vmap["Font"] = klfconfig.defaultStdFont;
  vmap["Button"] = QVariant("PreambleFont");
  a->setData(QVariant(vmap));
  connect(a, SIGNAL(triggered()), d, SLOT(slotChangeFontPresetSender()));
  d->pFontSetActions << a;
  fontPresetMenu->addAction(a);
  u->btnPreambleFontChoose->setMenu(fontPresetMenu);

  REG_SH_TEXTFORMATENSEMBLE(Keyword);
  REG_SH_TEXTFORMATENSEMBLE(Comment);
  REG_SH_TEXTFORMATENSEMBLE(ParenMatch);
  REG_SH_TEXTFORMATENSEMBLE(ParenMismatch);
  REG_SH_TEXTFORMATENSEMBLE(LonelyParen);


  // user scripts

  connect(u->lstUserScripts, SIGNAL(itemSelectionChanged()), d, SLOT(refreshUserScriptSelected()));
  connect(u->btnReloadUserScripts, SIGNAL(clicked()), d, SLOT(reloadUserScripts()));

  // --- 

  // if we're on a laptop, show the corresponding setting for battery power. Otherwise,
  // hide it.
  u->wRealTimePreviewExceptBattery->setVisible(KLFSysInfo::isLaptop());

  // ---

  retranslateUi(false);
}

void KLFSettings::retranslateUi(bool alsoBaseUi)
{
  if (alsoBaseUi)
    u->retranslateUi(this);

  // translate our preset actions
  int k;
  for (k = 0; k < d->pFontSetActions.size(); ++k) {
    QAction *a = d->pFontSetActions[k];
    QVariantMap vmap = a->data().toMap();
    QString refAKey = vmap["Action"].toString();
    KLF_ASSERT_CONDITION(d->pFontBasePresetActions.contains(refAKey),
			 "Base Reference Preset Action not found: "<<refAKey<<" ?!?",
			 continue ) ;
    QAction *refA = d->pFontBasePresetActions[refAKey];
    a->setText(refA->text());
    a->setIcon(refA->icon());
    a->setToolTip(refA->toolTip());
    QFont f = vmap["Font"].value<QFont>();
    a->setFont(f);
  }
}


KLFSettings::~KLFSettings()
{
  KLF_DELETE_PRIVATE ;
  delete u;
}


void KLFSettingsPrivate::showTabByName(const QByteArray& name)
{
  KLF_ASSERT_CONDITION(settingsTabs.contains(name), "Bad tab name : '"<<name<<"'", return; ) ;
  showTab(settingsTabs[name]);
}
void KLFSettingsPrivate::showTabByNameActionSender()
{
  QAction *actionsender = qobject_cast<QAction*>(sender());
  KLF_ASSERT_NOT_NULL(actionsender, "sender is not a QAction or is NULL!", return;);
  showTabByName(actionsender->data().toByteArray());
}
void KLFSettingsPrivate::showTab(QWidget * tabPage)
{
  KLF_ASSERT_NOT_NULL(tabPage, "given tabPage is NULL!", return ; ) ;

  K->u->tabs->setCurrentWidget(tabPage);
  foreach (QAction *a, actionTabs) {
    a->setChecked(settingsTabs[a->data().toByteArray()] == tabPage);
  }  
}

void KLFSettingsPrivate::populateSettingsCombos()
{
  populateLocaleCombo();
  populateExportProfilesCombos();
  populateDetailsSideWidgetTypeCombo();
}

void KLFSettingsPrivate::populateLocaleCombo()
{
  KLF_DEBUG_BLOCK(KLF_FUNC_NAME) ;

  K->u->cbxLocale->clear();
  // application language : populate combo box
  K->u->cbxLocale->addItem( QLatin1String("English Default"), QVariant::fromValue<QString>("en_US") );
  int k;
  for (k = 0; k < klf_avail_translations.size(); ++k) {
    KLFTranslationInfo ti = klf_avail_translations[k];
    K->u->cbxLocale->addItem( ti.translatedname, QVariant(ti.localename) );
    klfDbg("Added translation "<< ti.translatedname <<" ("<<ti.localename<<")") ;
  }

  // Select the current locale. This is also done in reset(), but these lines are needed here
  // for when this function is called within importAddOn().
  k = K->u->cbxLocale->findData(klfconfig.UI.locale.value());
  if (k == -1) {
    k = 0;
  }
  K->u->cbxLocale->setCurrentIndex(k);
}

void KLFSettingsPrivate::populateExportProfilesCombos()
{
  QList<KLFMimeExportProfile> eplist = mainWin->mimeExportProfileList();

  K->u->cbxCopyExportProfile->clear();
  K->u->cbxDragExportProfile->clear();
  int k;
  for (k = 0; k < eplist.size(); ++k) {
    K->u->cbxCopyExportProfile->addItem(eplist[k].description(), QVariant(eplist[k].profileName()));
    K->u->cbxDragExportProfile->addItem(eplist[k].description(), QVariant(eplist[k].profileName()));
  }
}

void KLFSettingsPrivate::populateDetailsSideWidgetTypeCombo()
{
  QStringList tlist = KLFSideWidgetManagerFactory::allSupportedTypes();
  K->u->cbxDetailsSideWidgetType->clear();
  foreach (QString s, tlist) {
    KLFSideWidgetManagerFactory * factory = KLFSideWidgetManagerFactory::findFactoryFor(s);
    KLF_ASSERT_NOT_NULL(factory, "Factory is NULL!?!", continue; ) ;
    K->u->cbxDetailsSideWidgetType->addItem(factory->getTitleFor(s), QVariant(s));
  }
}

void KLFSettings::show()
{
  KLF_DEBUG_BLOCK(KLF_FUNC_NAME) ;
  d->populateSettingsCombos();

  reset();

  d->refreshUserScriptList();
  d->refreshUserScriptSelected();


  KLF_BLOCK {
    // calculate the minimum width of the main toolbar
    QLayout *tLyt = u->toolBar->layout();
    KLF_ASSERT_NOT_NULL(tLyt, "toolbar layout is NULL!!", break; ) ;
    klfDbg("got toolbar layout: "<<tLyt) ;
    int ml, mt, mr, mb;
    tLyt->getContentsMargins(&ml, &mt, &mr, &mb);
    klfDbg("margins left="<<ml<<" right="<<mr<<" spacing="<<tLyt->spacing()) ;
    int minwidth = ml+mr + d->actionTabs.size()*tLyt->spacing();
    foreach (QAction *a, d->actionTabs) {
      int thiswidth = u->toolBar->widgetForAction(a)->sizeHint().width();
      klfDbg("action "<<a<<"; w = "<<u->toolBar->widgetForAction(a)<<" width="<<thiswidth) ;
      minwidth += thiswidth;
    }
    klfDbg("minwidth="<<minwidth) ;
    u->toolBar->setMinimumWidth(minwidth);
  }

  QDialog::show();
}


/** \internal */
#define __KLF_SHOW_SETTINGS_CONTROL( tabname , focuswidget )	\
  d->showTab(u->tab##tabname);				\
  u->focuswidget->setFocus(Qt::OtherFocusReason);

void KLFSettings::showControl(int control)
{
  /** \bug setup here !!! */
  switch (control) {
  case AppLanguage:
    __KLF_SHOW_SETTINGS_CONTROL(Interface, cbxLocale) ;
    break;
  case AppFonts:
    u->tabsAdvancedSettings->setCurrentWidget(u->tabAdvancedAppearance);
    __KLF_SHOW_SETTINGS_CONTROL(Advanced, btnAppFont) ;
    break;
  case AppLookNFeel:
    u->tabsAdvancedSettings->setCurrentWidget(u->tabAdvancedAppearance);
    __KLF_SHOW_SETTINGS_CONTROL(Advanced, btnAppFont) ;
    break;
  case AppMacOSXMetalLook:
    u->tabsAdvancedSettings->setCurrentWidget(u->tabAdvancedAppearance);
    __KLF_SHOW_SETTINGS_CONTROL(Advanced, chkMacBrushedMetalLook) ;
    break;
  case Preview:
    __KLF_SHOW_SETTINGS_CONTROL(Interface, chkEnableRealTimePreview) ;
    break;
  case TooltipPreview:
    __KLF_SHOW_SETTINGS_CONTROL(Interface, chkEnableToolTipPreview) ;
    break;
  case SyntaxHighlighting:
    __KLF_SHOW_SETTINGS_CONTROL(Editor, chkSHEnable) ;
    break;
  case SyntaxHighlightingColors:
    u->tabsAdvancedSettings->setCurrentWidget(u->tabAdvancedSyntaxHighlighting);
    __KLF_SHOW_SETTINGS_CONTROL(Editor, gbxSHColors) ;
    break;
  case ExecutablePaths:
    __KLF_SHOW_SETTINGS_CONTROL(Latex, pathTempDir) ;
    break;
  case ExpandEPSBBox:
    __KLF_SHOW_SETTINGS_CONTROL(Latex, spnLBorderOffset) ;
    break;
  case CalcEPSBoundingBox:
    u->tabsAdvancedSettings->setCurrentWidget(u->tabAdvancedLatex);
    __KLF_SHOW_SETTINGS_CONTROL(Advanced, chkCalcEPSBoundingBox) ;
    break;
  case ExportProfiles:
    __KLF_SHOW_SETTINGS_CONTROL(Interface, cbxCopyExportProfile) ;
    break;
  case LibrarySettings:
    __KLF_SHOW_SETTINGS_CONTROL(Library, chkLibRestoreURLs) ;
    break;
  case UserScriptInfo:
    __KLF_SHOW_SETTINGS_CONTROL(UserScripts, lstUserScripts) ;
    break;
  default:
    qWarning()<<KLF_FUNC_NAME<<": unknown control number requested : "<<control;
  }
}

/** \internal */
#define __KLF_SETTINGS_TEST_STR_CONTROL( controlName, controlNum )	\
  if (controlName == QLatin1String(#controlNum)) {			\
    showControl(controlNum);						\
    return;								\
  }

void KLFSettings::showControl(const QString& controlName)
{
  __KLF_SETTINGS_TEST_STR_CONTROL( controlName, AppLanguage ) ;
  __KLF_SETTINGS_TEST_STR_CONTROL( controlName, AppFonts ) ;
  __KLF_SETTINGS_TEST_STR_CONTROL( controlName, AppLookNFeel ) ;
  __KLF_SETTINGS_TEST_STR_CONTROL( controlName, AppMacOSXMetalLook ) ;
  __KLF_SETTINGS_TEST_STR_CONTROL( controlName, Preview ) ;
  __KLF_SETTINGS_TEST_STR_CONTROL( controlName, TooltipPreview ) ;
  __KLF_SETTINGS_TEST_STR_CONTROL( controlName, SyntaxHighlighting ) ;
  __KLF_SETTINGS_TEST_STR_CONTROL( controlName, SyntaxHighlightingColors ) ;
  __KLF_SETTINGS_TEST_STR_CONTROL( controlName, ExecutablePaths ) ;
  __KLF_SETTINGS_TEST_STR_CONTROL( controlName, ExpandEPSBBox ) ;
  __KLF_SETTINGS_TEST_STR_CONTROL( controlName, CalcEPSBoundingBox ) ;
  __KLF_SETTINGS_TEST_STR_CONTROL( controlName, LibrarySettings ) ;
  __KLF_SETTINGS_TEST_STR_CONTROL( controlName, UserScriptInfo ) ;

  klfWarning("BUG: unknown control name: "<<controlName) ;
}


bool KLFSettings::eventFilter(QObject *object, QEvent *event)
{
  return QDialog::eventFilter(object, event);
}

void KLFSettings::reset()
{
  int k;
  KLFBackend::klfSettings s = d->mainWin->backendSettings();

  k = u->cbxLocale->findData(klfconfig.UI.locale.toVariant());
  if (k == -1) {
    k = 0;
  }
  u->cbxLocale->setCurrentIndex(k);

  u->pathTempDir->setPath(QDir::toNativeSeparators(s.tempdir));
  u->pathLatex->setPath(s.latexexec);
  u->pathDvips->setPath(s.dvipsexec);
  u->pathGs->setPath(s.gsexec);
  u->spnLBorderOffset->setValueInRefUnit(s.lborderoffset);
  u->spnTBorderOffset->setValueInRefUnit(s.tborderoffset);
  u->spnRBorderOffset->setValueInRefUnit(s.rborderoffset);
  u->spnBBorderOffset->setValueInRefUnit(s.bborderoffset);
  u->chkCalcEPSBoundingBox->setChecked( s.calcEpsBoundingBox );
  u->chkOutlineFonts->setChecked( s.outlineFonts );

  u->txtSetTexInputs->setText( klfconfig.BackendSettings.setTexInputs );

  u->chkEditorTabInsertsTab->setChecked( klfconfig.UI.editorTabInsertsTab );
  u->chkEditorWrapLines->setChecked( klfconfig.UI.editorWrapLines );

  u->chkEditorIncludePreambleDefs->setChecked( klfconfig.UI.symbolIncludeWithPreambleDefs );

  u->chkSHEnable->setChecked(klfconfig.SyntaxHighlighter.enabled);
  u->chkSHHighlightParensOnly->setChecked(klfconfig.SyntaxHighlighter.highlightParensOnly);
  u->chkSHHighlightLonelyParen->setChecked(klfconfig.SyntaxHighlighter.highlightLonelyParens);
  //u->chkSHNoMatchParenTypes->setChecked( ! klfconfig.SyntaxHighlighter.matchParenTypes );

  for (k = 0; k < d->textformats.size(); ++k) {
    if ((*d->textformats[k].fmt)().hasProperty(QTextFormat::ForegroundBrush))
      d->textformats[k].fg->setColor((*d->textformats[k].fmt)().foreground().color());
    else
      d->textformats[k].fg->setColor(QColor());
    if ((*d->textformats[k].fmt)().hasProperty(QTextFormat::BackgroundBrush))
      d->textformats[k].bg->setColor((*d->textformats[k].fmt)().background().color());
    else
      d->textformats[k].bg->setColor(QColor());
    if ((*d->textformats[k].fmt)().hasProperty(QTextFormat::FontWeight))
      d->textformats[k].chkB->setChecked((*d->textformats[k].fmt)().fontWeight() > 60);
    else
      d->textformats[k].chkB->setCheckState(Qt::PartiallyChecked);
    if ((*d->textformats[k].fmt)().hasProperty(QTextFormat::FontItalic))
      d->textformats[k].chkI->setChecked((*d->textformats[k].fmt)().fontItalic());
    else
      d->textformats[k].chkI->setCheckState(Qt::PartiallyChecked);
  }

  u->txtScriptInterpreterPython->setText(
      klfconfig.BackendSettings.userScriptInterpreters().value("py").toString() );
  u->txtScriptInterpreterShell->setText(
      klfconfig.BackendSettings.userScriptInterpreters().value("sh").toString() );

  d->pUserSetDefaultAppFont = klfconfig.UI.useSystemAppFont;
  u->btnAppFont->setFont(klfconfig.UI.applicationFont);
  u->btnAppFont->setProperty("selectedFont", klfconfig.UI.applicationFont.toVariant());
  u->btnEditorFont->setFont(klfconfig.UI.latexEditFont);
  u->btnEditorFont->setProperty("selectedFont", klfconfig.UI.latexEditFont.toVariant());
  u->btnPreambleFont->setFont(klfconfig.UI.preambleEditFont);
  u->btnPreambleFont->setProperty("selectedFont", klfconfig.UI.preambleEditFont.toVariant());

  u->chkEnableRealTimePreview->setChecked(klfconfig.UI.enableRealTimePreview);
  u->chkRealTimePreviewExceptBattery->setChecked(klfconfig.UI.realTimePreviewExceptBattery);
  u->spnPreviewWidth->setValue(klfconfig.UI.smallPreviewSize().width());
  u->spnPreviewHeight->setValue(klfconfig.UI.smallPreviewSize().height());

  u->chkEnableToolTipPreview->setChecked(klfconfig.UI.enableToolTipPreview);
  u->spnToolTipMaxWidth->setValue(klfconfig.UI.previewTooltipMaxSize().width());
  u->spnToolTipMaxHeight->setValue(klfconfig.UI.previewTooltipMaxSize().height());

  u->chkShowHintPopups->setChecked(klfconfig.UI.showHintPopups);
  u->chkClearLatexOnly->setChecked(klfconfig.UI.clearLatexOnly);
  u->gbxGlowEffect->setChecked(klfconfig.UI.glowEffect);
  u->colGlowEffectColor->setColor(klfconfig.UI.glowEffectColor);
  u->spnGlowEffectRadius->setValue(klfconfig.UI.glowEffectRadius);
  int sidewidgettypei = u->cbxDetailsSideWidgetType->findData(QVariant(klfconfig.UI.detailsSideWidgetType()));
  u->cbxDetailsSideWidgetType->setCurrentIndex(sidewidgettypei);
  u->chkMacBrushedMetalLook->setChecked(klfconfig.UI.macBrushedMetalLook);
//#ifndef KLF_WS_MAC
// obsolete option, not used because I couldn't obtain a good look with Qt5 and had
// painting bugs (all black widgets)
  u->chkMacBrushedMetalLook->hide();
//#endif

  int copyi = u->cbxCopyExportProfile->findData(QVariant(klfconfig.ExportData.copyExportProfile));
  u->cbxCopyExportProfile->setCurrentIndex(copyi);
  int dragi = u->cbxDragExportProfile->findData(QVariant(klfconfig.ExportData.dragExportProfile));
  u->cbxDragExportProfile->setCurrentIndex(dragi);
  //  u->chkShowExportProfilesLabel->setChecked(klfconfig.ExportData.showExportProfilesLabel);
  u->chkMenuExportProfileAffectsDrag->setChecked(klfconfig.ExportData.menuExportProfileAffectsDrag);
  u->chkMenuExportProfileAffectsCopy->setChecked(klfconfig.ExportData.menuExportProfileAffectsCopy);

  u->chkLibRestoreURLs->setChecked(klfconfig.LibraryBrowser.restoreURLs);
  u->chkLibConfirmClose->setChecked(klfconfig.LibraryBrowser.confirmClose);
  u->chkLibHistoryTagCopyToArchive->setChecked(klfconfig.LibraryBrowser.historyTagCopyToArchive);
  //  u->chkLibGroupSubCategories->setChecked(klfconfig.LibraryBrowser.groupSubCategories);
  u->cbxLibIconViewFlow->setSelectedValue(klfconfig.LibraryBrowser.iconViewFlow);
}



bool KLFSettingsPrivate::setDefaultFor(const QString& progname, const QString& guessedprog, bool required,
				       KLFPathChooser *destination)
{
  QString progpath = guessedprog;
  if (progpath.isEmpty()) {
    if (QFileInfo(destination->path()).isExecutable()) {
      // field already has a valid value, don't touch it and don't complain
      return true;
    }
    if ( ! required )
      return false;
    QMessageBox msgbox(QMessageBox::Critical, tr("Error"), tr("Could not find %1 executable !")
		       .arg(progname),  QMessageBox::Ok, K);
    msgbox.setInformativeText(tr("Please check your installation and specify the path"
                                 " to %1 executable manually if it is not installed"
                                 " in $PATH.").arg(progname));
    msgbox.setDefaultButton(QMessageBox::Ok);
    msgbox.setEscapeButton(QMessageBox::Ok);
    msgbox.exec();
    return false;
  }

  destination->setPath(progpath);
  return true;
}

void KLFSettings::setDefaultPaths()
{
  KLFBackend::klfSettings defaultsettings;
  KLFBackend::detectSettings(&defaultsettings);
  if ( ! QFileInfo(u->pathTempDir->path()).isDir() )
    u->pathTempDir->setPath(QDir::toNativeSeparators(defaultsettings.tempdir));
  d->setDefaultFor("latex", defaultsettings.latexexec, true, u->pathLatex);
  d->setDefaultFor("dvips", defaultsettings.dvipsexec, true, u->pathDvips);
  d->setDefaultFor("gs", defaultsettings.gsexec, true, u->pathGs);
}


void KLFSettingsPrivate::refreshUserScriptList()
{
  KLF_DEBUG_BLOCK(KLF_FUNC_NAME) ;

  K->u->lstUserScripts->clear();

  klfDbg("User script config is " << klfconfig.UserScripts.userScriptConfig) ;

  QMap<QString, QTreeWidgetItem*> dirs;

  int k;
  for (k = 0; k < klf_user_scripts.size(); ++k) {
    QString userscript = klf_user_scripts[k];
    KLFUserScriptInfo info(userscript);

    klfDbg("loading "<<userscript) ;

    // this is != canonicalFilePath which is _with_ file name
    QString dd = QFileInfo(info.userScriptPath()).canonicalPath();

    QTreeWidgetItem *diritem = dirs.value(dd, NULL);
    if (diritem == NULL) {
      // create the directory node
      diritem = new QTreeWidgetItem(K->u->lstUserScripts, QStringList()<<dd);
      dirs[dd] = diritem;
    }

    // add the user script
    QTreeWidgetItem *item = new QTreeWidgetItem(diritem, QStringList()<<info.userScriptName());
    item->setData(0, KLFSETTINGS_ROLE_USERSCRIPT, QVariant::fromValue<QString>(userscript));

    if (!diritem->isExpanded()) {
      diritem->setExpanded(true);
    }

    // load user script configuration into the respective config widget
    QString uifile = info.settingsFormUI();
    if (!uifile.isEmpty()) {
      klfDbg("populating settings widget for user script " << userscript) ;
      QWidget * scriptconfigwidget = getUserScriptConfigWidget(info, info.relativeFile(uifile));
      // load current configuration
      klfDbg("the corresponding config is "
             << klfconfig.UserScripts.userScriptConfig.value(info.userScriptPath())) ;
      if (klfconfig.UserScripts.userScriptConfig.contains(info.userScriptPath())) {
        displayUserScriptConfig(scriptconfigwidget,
                                klfconfig.UserScripts.userScriptConfig.value(info.userScriptPath()));
      }
    }
  }
  
  K->u->splitUserScripts->setSizes(QList<int>() << 100 << 100);
}


#undef KLF_BLOCK
#define KLF_BLOCK							\
  int _klf_block_broken_by = 0;						\
  for (bool _klf_block_first = true; _klf_block_first; _klf_block_first = false)

// don't use value 0, it's used to mean 'not broken'
#define KLF_BREAK_BECAUSE(why) { _klf_block_broken_by = why; break; }
#define KLF_BREAK { _klf_block_broken_by = -1; break; }
#define KLF_BROKEN_BECAUSE(whyvar)		\
  if (int whyvar = _klf_block_broken_by)
#define KLF_BROKEN				\
  if (_klf_block_broken_by)


void KLFSettingsPrivate::refreshUserScriptSelected()
{
  KLF_DEBUG_BLOCK(KLF_FUNC_NAME) ;

  QTreeWidgetItem * item = K->u->lstUserScripts->currentItem();
  klfDbg("item="<<item) ;

  bool ok = true;

  QString usname;

  if (item == NULL) {
    K->u->lblUserScriptInfo->setText(tr("No user script selected.", "[[user script info label]]"));
    K->u->lblUserScriptInfo->setText(tr("No user script selected.", "[[user script info label]]"));
    ok = false;
  } else {
    QVariant v = item->data(0, KLFSETTINGS_ROLE_USERSCRIPT);
    if (!v.isValid()) {
      K->u->lblUserScriptInfo->setText(tr("No user script selected.",
                                          "[[user script info label]]"));
      ok = false;
    } else {
      usname = v.toString();
      klfDbg("usname = " << usname) ;
      if (!KLFUserScriptInfo::hasScriptInfoInCache(usname)) {
        klfDbg("not in cache -- there's an error.") ;
        KLFUserScriptInfo errinfo(usname);
        K->u->lblUserScriptInfo->setText(tr("User script info error: %1",
                                            "[[user script info label]]").arg(errinfo.scriptInfoErrorString()));
        ok = false;
      }
    }
  }
  if (!ok) {
    K->u->stkUserScriptSettings->setCurrentWidget(K->u->wStkUserScriptSettingsEmptyPage);
    return;
  }

  // update user script info and settings widget
  
  // no need for full reload at this point
  //KLFUserScriptInfo usinfo = KLFUserScriptInfo::forceReloadScriptInfo(
  //    s, d->mainWin->backendSettings(),
  //    klfconfig.UserScripts.userScriptConfig.value(s)
  //    );
  KLFUserScriptInfo usinfo(usname);

  int textpointsize = QFontInfo(K->u->lblUserScriptInfo->font()).pointSize() - 1;
  QString txt = usinfo.htmlInfo(QString::fromLatin1("body { font-size: %1pt }").arg(textpointsize));

  K->u->lblUserScriptInfo->setText(txt);

  // update config widget
  QString uifile = usinfo.settingsFormUI();
  if (!uifile.isEmpty()) {
    uifile = usinfo.relativeFile(uifile);
    QWidget * scriptconfigwidget = getUserScriptConfigWidget(usinfo, uifile);
    K->u->stkUserScriptSettings->setCurrentWidget(scriptconfigwidget);
    klfDbg("Set script config UI. uifile="<<uifile) ;
  } else {
    K->u->stkUserScriptSettings->setCurrentWidget(K->u->wStkUserScriptSettingsEmptyPage);
  }
}

QWidget * KLFSettingsPrivate::getUserScriptConfigWidget(const KLFUserScriptInfo& usinfo, const QString& uifile)
{
  KLF_DEBUG_BLOCK(KLF_FUNC_NAME) ;
  klfDbg("uifile = " << uifile) ;
  if (userScriptConfigWidgets.contains(usinfo.userScriptPath())) {
    klfDbg("widget from cache.") ;
    return userScriptConfigWidgets[usinfo.userScriptPath()];
  }

  QFile uif(uifile);
  uif.open(QFile::ReadOnly);
  QWidget * scriptconfigwidget = klfLoadUI(&uif, K->u->stkUserScriptSettings);
  uif.close();
  KLF_ASSERT_NOT_NULL(scriptconfigwidget, "Can't load script config widget "<<uifile<<".",
		      return K->u->wStkUserScriptSettingsEmptyPage) ;

  K->u->stkUserScriptSettings->addWidget(scriptconfigwidget);
  userScriptConfigWidgets[usinfo.userScriptPath()] = scriptconfigwidget;

  return scriptconfigwidget;
}

QVariantMap KLFSettingsPrivate::getUserScriptConfig(QWidget * w)
{
  KLF_DEBUG_BLOCK(KLF_FUNC_NAME) ;
  
  if (w == NULL) {
    return QVariantMap();
  }

  QVariantMap data;

  // there's config for the user script
  QList<QWidget*> inwidgets = w->findChildren<QWidget*>(QRegExp("^INPUT_.*"));
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

  klfDbg("userscript config is "<<data) ;
  return data;
}

void KLFSettingsPrivate::displayUserScriptConfig(QWidget *w, const QVariantMap& data)
{
  KLF_DEBUG_BLOCK(KLF_FUNC_NAME) ;

  if (w == NULL) {
    return;
  }

  // find the input widgets
  QList<QWidget*> inwidgets = w->findChildren<QWidget*>(QRegExp("^INPUT_.*"));
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
    KLF_ASSERT_CONDITION(data.contains(n), "No value given for config widget "<<n, continue ; );
    QVariant value = data[n];
    inw->setProperty(userPropName.constData(), value);
    klfDbg("Set config widget for " << n << " to " << value);
  }
}


void KLFSettingsPrivate::reloadUserScripts()
{
  klf_reload_user_scripts();

  int k;
  for (k = 0; k < klf_user_scripts.size(); ++k) {
    KLFUserScriptInfo::forceReloadScriptInfo( klf_user_scripts[k] );
  }

  // refresh GUI

  refreshUserScriptList();
}


void KLFSettingsPrivate::slotChangeFontPresetSender()
{
  QAction *a = qobject_cast<QAction*>(sender());
  if (a == 0)
    return;
  const QVariantMap vmap = a->data().toMap();
  klfDbg("Set font from action with data "<<vmap) ;
  QString btnkey = vmap["Button"].toString();
  KLF_ASSERT_CONDITION(pFontButtons.contains(btnkey), "Unknown button "<<btnkey<<" !", return ) ;
  QFont f = vmap["Font"].value<QFont>();
  slotChangeFont(pFontButtons[btnkey], f);
  if (vmap.contains("isSystemDefaultAppFont") && vmap["isSystemDefaultAppFont"].toBool()) {
    klfDbg("Set default application font.") ;
    pUserSetDefaultAppFont = true;
  }
}
void KLFSettingsPrivate::slotChangeFontSender()
{
  QPushButton *w = qobject_cast<QPushButton*>(sender());
  if ( w == 0 )
    return;
  QFont fnt = QFontDialog::getFont(0, w->property("selectedFont").value<QFont>(), K);
  slotChangeFont(w, fnt);
}
void KLFSettingsPrivate::slotChangeFont(QPushButton *w, const QFont& fnt)
{
  if ( w == 0 )
    return;
  w->setFont(fnt);
  w->setProperty("selectedFont", QVariant(fnt));
  if (w == K->u->btnAppFont)
    pUserSetDefaultAppFont = false;
}

void KLFSettings::showAdvancedConfigEditor()
{
  if (d->advancedConfigEditor == NULL) {
    d->advancedConfigEditor = new KLFAdvancedConfigEditor(this, &klfconfig);
    connect(d->advancedConfigEditor, SIGNAL(configModified(const QString&)),
	    this, SLOT(reset()));
    connect(this, SIGNAL(settingsApplied()), d->advancedConfigEditor, SLOT(updateConfig()));
  }
  d->advancedConfigEditor->show();
}

// klfdebug.cpp
extern  QByteArray klf_qt_msg_get_warnings_buffer();

void KLFSettings::showSystemMessages()
{
  QByteArray warnings_buffer = klf_qt_msg_get_warnings_buffer();

  // create the dialog
  KLFProgErr dlg(this, QString());

  // setup the text widget and display the buffer
  QTextEdit *w = dlg.textEditWidget();
  w->setPlainText(warnings_buffer);
  // scroll to end
  QTextCursor curend(w->document());
  curend.setPosition(warnings_buffer.size());
  w->setTextCursor(curend);
  w->ensureCursorVisible();
  
  // show dialog
  dlg.resize(800,400);
  dlg.exec();
}

void KLFSettings::apply()
{
  KLF_DEBUG_BLOCK(KLF_FUNC_NAME) ;

  int k;
  // apply settings here

  bool warnneedrestart = false;

  // the settings object that we will fill, and set to d->mainWin
  KLFBackend::klfSettings backendsettings = d->mainWin->backendSettings();

  backendsettings.tempdir = QDir::fromNativeSeparators(u->pathTempDir->path());
  backendsettings.latexexec = u->pathLatex->path();
  backendsettings.dvipsexec = u->pathDvips->path();
  backendsettings.gsexec = u->pathGs->path();
  //  backendsettings.epstopdfexec = QString();
  //  if (u->chkEpstopdf->isChecked()) {
  //    backendsettings.epstopdfexec = u->pathEpstopdf->path();
  //  }
  // detect environment for those settings (in particular mgs.exe for ghostscript ...)

  KLFBackend::detectOptionSettings(&backendsettings);

  backendsettings.lborderoffset = u->spnLBorderOffset->valueInRefUnit();
  backendsettings.tborderoffset = u->spnTBorderOffset->valueInRefUnit();
  backendsettings.rborderoffset = u->spnRBorderOffset->valueInRefUnit();
  backendsettings.bborderoffset = u->spnBBorderOffset->valueInRefUnit();
  backendsettings.calcEpsBoundingBox = u->chkCalcEPSBoundingBox->isChecked();
  backendsettings.outlineFonts = u->chkOutlineFonts->isChecked();

  klfconfig.BackendSettings.setTexInputs = u->txtSetTexInputs->text();

  backendsettings.userScriptInterpreters["py"] = u->txtScriptInterpreterPython->text();
  backendsettings.userScriptInterpreters["sh"] = u->txtScriptInterpreterShell->text();

  // do this *after* setting 'setTexInputs', because we refer to it when adjusting the
  // execenv setting with the required value of TEXINPUTS...
  d->mainWin->applySettings(backendsettings);

  klfconfig.UI.editorTabInsertsTab = u->chkEditorTabInsertsTab->isChecked();
  klfconfig.UI.editorWrapLines = u->chkEditorWrapLines->isChecked();

  klfconfig.UI.symbolIncludeWithPreambleDefs = u->chkEditorIncludePreambleDefs->isChecked();

  klfconfig.SyntaxHighlighter.enabled = u->chkSHEnable->isChecked();
  klfconfig.SyntaxHighlighter.highlightParensOnly = u->chkSHHighlightParensOnly->isChecked();
  klfconfig.SyntaxHighlighter.highlightLonelyParens = u->chkSHHighlightLonelyParen->isChecked();
  //  klfconfig.SyntaxHighlighter.matchParenTypes = ! u->chkSHNoMatchParenTypes->isChecked();


  for (k = 0; k < d->textformats.size(); ++k) {
    QTextCharFormat fmt = *d->textformats[k].fmt;
    QColor c = d->textformats[k].fg->color();
    if (c.isValid())
      fmt.setForeground(c);
    else
      fmt.clearForeground();
    c = d->textformats[k].bg->color();
    if (c.isValid())
      fmt.setBackground(c);
    else
      fmt.clearBackground();
    Qt::CheckState b = d->textformats[k].chkB->checkState();
    if (b == Qt::PartiallyChecked)
      fmt.clearProperty(QTextFormat::FontWeight);
    else if (b == Qt::Checked)
      fmt.setFontWeight(QFont::Bold);
    else
      fmt.setFontWeight(QFont::Normal);
    Qt::CheckState it = d->textformats[k].chkI->checkState();
    if (it == Qt::PartiallyChecked)
      fmt.clearProperty(QTextFormat::FontItalic);
    else
      fmt.setFontItalic( it == Qt::Checked );

    *d->textformats[k].fmt = fmt;
  }

  // language settings
  QString localename = u->cbxLocale->itemData(u->cbxLocale->currentIndex()).toString();
  klfDbg("New locale name: "<<localename);
  bool localechanged = false;
  if (klfconfig.UI.locale != localename) {
    if ((!klfconfig.UI.locale().isEmpty() && klfconfig.UI.locale != "C" && klfconfig.UI.locale != "en_US") ||
	(!localename.isEmpty() && localename != "C" && localename != "en_US"))
      localechanged = true; // not just switching from "C" to "en_US" which is not a locale change...
  }
  klfconfig.UI.locale = localename;
  QLocale::setDefault(klfconfig.UI.locale());
  d->mainWin->setApplicationLocale(localename);
  if (localechanged) {
    QMessageBox::information(this, tr("Language changed"),
			     tr("You may need to restart KLatexFormula for your new language "
				"settings to fully take effect."));
  }
  //  klf_main_do_the_change_of_locale_and_load_translators(...);
  //  QList<QWidget*> uilist;
  //  uilist << d->mainWin << d->mainWin->libraryBrowserWidget() << d->mainWin->latexSymbolsWidget()
  //	 << d->mainWin->styleManagerWidget() << this ;
  //  for (k = 0; k < uilist.size(); ++k) {
  //    uilist[k]->retranlsateUi(uilist[k]);
  //  }

  klfDbg("Applying font settings etc.") ;

  // font settings
  QFont curAppFont = klfconfig.UI.applicationFont;
  QFont newAppFont = u->btnAppFont->property("selectedFont").value<QFont>();
  if (curAppFont != newAppFont || d->pUserSetDefaultAppFont != klfconfig.UI.useSystemAppFont) {
    klfconfig.UI.useSystemAppFont = d->pUserSetDefaultAppFont;
    klfconfig.UI.applicationFont = newAppFont;
    if (klfconfig.UI.useSystemAppFont) {
      qApp->setFont(klfconfig.defaultStdFont);
      qApp->setFont(QFont());
    } else {
      qApp->setFont(klfconfig.UI.applicationFont);
    }
    // Style sheet refresh is needed to force font (?)
    qApp->setStyleSheet(qApp->styleSheet());
    d->mainWin->refreshAllWindowStyleSheets();
  }
  klfconfig.UI.latexEditFont = u->btnEditorFont->property("selectedFont").value<QFont>();
  d->mainWin->setTxtLatexFont(klfconfig.UI.latexEditFont);
  klfconfig.UI.preambleEditFont = u->btnPreambleFont->property("selectedFont").value<QFont>();
  d->mainWin->setTxtPreambleFont(klfconfig.UI.preambleEditFont);

  klfconfig.UI.smallPreviewSize = QSize(u->spnPreviewWidth->value(), u->spnPreviewHeight->value());
  klfconfig.UI.enableRealTimePreview = u->chkEnableRealTimePreview->isChecked();
  klfconfig.UI.realTimePreviewExceptBattery = u->chkRealTimePreviewExceptBattery->isChecked();

  klfconfig.UI.previewTooltipMaxSize = QSize(u->spnToolTipMaxWidth->value(), u->spnToolTipMaxHeight->value());
  klfconfig.UI.enableToolTipPreview = u->chkEnableToolTipPreview->isChecked();

  klfconfig.UI.showHintPopups = u->chkShowHintPopups->isChecked();
  klfconfig.UI.clearLatexOnly = u->chkClearLatexOnly->isChecked();
  klfconfig.UI.glowEffect = u->gbxGlowEffect->isChecked();
  klfconfig.UI.glowEffectColor = u->colGlowEffectColor->color();
  klfconfig.UI.glowEffectRadius = u->spnGlowEffectRadius->value();

  klfconfig.UI.detailsSideWidgetType =
    u->cbxDetailsSideWidgetType->itemData(u->cbxDetailsSideWidgetType->currentIndex()).toString();
  if (klfconfig.UI.macBrushedMetalLook != u->chkMacBrushedMetalLook->isChecked()) {
    warnneedrestart = true;
  }
  klfconfig.UI.macBrushedMetalLook = u->chkMacBrushedMetalLook->isChecked();

  klfconfig.ExportData.copyExportProfile = 
    u->cbxCopyExportProfile->itemData(u->cbxCopyExportProfile->currentIndex()).toString();
  klfconfig.ExportData.dragExportProfile = 
    u->cbxDragExportProfile->itemData(u->cbxDragExportProfile->currentIndex()).toString();
  //  klfconfig.ExportData.showExportProfilesLabel = u->chkShowExportProfilesLabel->isChecked();
  klfconfig.ExportData.menuExportProfileAffectsDrag = u->chkMenuExportProfileAffectsDrag->isChecked();
  klfconfig.ExportData.menuExportProfileAffectsCopy = u->chkMenuExportProfileAffectsCopy->isChecked();

  klfconfig.LibraryBrowser.restoreURLs = u->chkLibRestoreURLs->isChecked();
  klfconfig.LibraryBrowser.confirmClose = u->chkLibConfirmClose->isChecked();
  klfconfig.LibraryBrowser.historyTagCopyToArchive = u->chkLibHistoryTagCopyToArchive->isChecked();
  //  klfconfig.LibraryBrowser.groupSubCategories = u->chkLibGroupSubCategories->isChecked();
  klfconfig.LibraryBrowser.iconViewFlow =  u->cbxLibIconViewFlow->selectedValue();

  // save userscript config
  klfDbg("saving userscript settings.") ;
  for (k = 0; k < klf_user_scripts.size(); ++k) {
    KLFUserScriptInfo usinfo(klf_user_scripts[k]);

    QString formui = usinfo.settingsFormUI();
    if (formui.isEmpty())
      continue;

    if (!d->userScriptConfigWidgets.contains(usinfo.userScriptPath()))
      continue; // no config for them...

    klfDbg("saving settings for "<<usinfo.userScriptName()) ;

    QWidget * w = d->userScriptConfigWidgets[usinfo.userScriptPath()];

    QVariantMap data = d->getUserScriptConfig(w);

    klfconfig.UserScripts.userScriptConfig[usinfo.userScriptPath()] = data;

    KLFUserScriptInfo::forceReloadScriptInfo(usinfo.userScriptPath());
  }

  if (warnneedrestart) {
    QMessageBox::information(this, tr("Restart KLatexFormula"),
			     tr("You need to restart KLatexFormula for your changes to take effect."));
  }

  d->mainWin->saveSettings();

  // in case eg. the plugins re-change klfconfig in some way (skin does this for syntax highlighting)
  // -> refresh
  reset();

  emit settingsApplied();
}

void KLFSettings::accept()
{
  // apply settings
  apply();
  // and exit dialog
  QDialog::accept();
}

void KLFSettings::help()
{
  QWhatsThis::enterWhatsThisMode();
}




