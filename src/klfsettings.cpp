/***************************************************************************
 *   file klfsettings.cpp
 *   This file is part of the KLatexFormula Project.
 *   Copyright (C) 2007 by Philippe Faist
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
#include <QFontDialog>
#include <QString>
#include <QMessageBox>
#include <QFileInfo>
#include <QDir>
#include <QFileDialog>
#include <QWhatsThis>
#include <QResource>

#include <klfcolorchooser.h>
#include <klfpathchooser.h>

#include <klfbackend.h>

#include "klfmain.h"
#include "klfmainwin.h"
#include "klfconfig.h"
#include "klfpluginiface.h"
#include "klfsettings.h"


#define KLFSETTINGS_ROLE_PLUGNAME (Qt::UserRole + 5300)
#define KLFSETTINGS_ROLE_PLUGINDEX (KLFSETTINGS_ROLE_PLUGNAME + 1)

#define KLFSETTINGS_ROLE_ADDONINDEX (Qt::UserRole + 5400)


#define REG_SH_TEXTFORMATENSEMBLE(x) \
  _textformats.append( TextFormatEnsemble( & klfconfig.SyntaxHighlighter.fmt##x , \
					   colSH##x, colSH##x##Bg , chkSH##x##B , chkSH##x##I ) );

#if defined(Q_OS_WIN32) || defined(Q_OS_WIN64)
#  define PROG_LATEX "latex.exe"
#  define PROG_DVIPS "dvips.exe"
#  define PROG_GS "gswin32c.exe"
#  define PROG_EPSTOPDF "epstopdf.exe"
static QString standard_extra_paths =
  "C:\\Program Files\\MiKTeX*\\miktex\\bin;C:\\Program Files\\gs\\gs*\\bin";
#else
#  define PROG_LATEX "latex"
#  define PROG_DVIPS "dvips"
#  define PROG_GS "gs"
#  define PROG_EPSTOPDF "epstopdf"
static QString standard_extra_paths = "";
#endif




KLFSettings::KLFSettings(KLFMainWin* parent)
  : QDialog(parent), KLFSettingsUI()
{
  setupUi(this);
  setObjectName("KLFSettings");

  _mainwin = parent;

  reset();

  btns->clear();

  QPushButton *b;
  b = new QPushButton(QIcon(":/pics/closehide.png"), tr("Cancel"), btns);
  btns->addButton(b, QDialogButtonBox::RejectRole);
  connect(b, SIGNAL(clicked()), this, SLOT(reject()));
  b = new QPushButton(QIcon(":/pics/apply.png"), tr("Apply"), btns);
  btns->addButton(b, QDialogButtonBox::ApplyRole);
  connect(b, SIGNAL(clicked()), this, SLOT(apply()));
  b = new QPushButton(QIcon(":/pics/ok.png"), tr("OK"), btns);
  btns->addButton(b, QDialogButtonBox::AcceptRole);
  connect(b, SIGNAL(clicked()), this, SLOT(accept()));

  // application language : populate combo box
  cbxLocale->addItem( tr("English Default", "[[first item of language graphical choice box]]") ,
		      QVariant(QString::null) );
  int k;
  for (k = 0; k < klf_avail_translations.size(); ++k) {
    QLocale lc(klf_avail_translations[k]);
    QString s;
    if ( klf_avail_translations[k].indexOf("_") != -1 ) {
      // has country information in locale
      s = tr("%1 (%2)", "[[%1=Language (%2=Country)]]")
	.arg(QLocale::languageToString(lc.language()))
	.arg(QLocale::countryToString(lc.country()));
    } else {
      s = tr("%1", "[[%1=Language, no country is specified]]").arg(QLocale::languageToString(lc.language()));
    }
    cbxLocale->addItem( s, // + QString("  [%3]").arg(klf_avail_translations[k]) ,
			QVariant(klf_avail_translations[k]) );
  }

  // set some smaller fonts for small titles
  QFont f = lblSHForeground->font();
  f.setPointSize(QFontInfo(f).pointSize() - 1);
  lblSHForeground->setFont(f);
  lblSHBackground->setFont(f);

  connect(btnPathsReset, SIGNAL(clicked()), this, SLOT(setDefaultPaths()));

  connect(lstPlugins, SIGNAL(itemSelectionChanged()), this, SLOT(refreshPluginSelected()));
  connect(lstAddOns, SIGNAL(itemSelectionChanged()), this, SLOT(refreshAddOnSelected()));
  connect(btnImportAddOn, SIGNAL(clicked()), this, SLOT(importAddOn()));
  connect(btnRemoveAddOn, SIGNAL(clicked()), this, SLOT(removeAddOn()));
  //  connect(btnRemovePlugin, SIGNAL(clicked()), this, SLOT(removePlugin()));

  connect(btnAppFont, SIGNAL(clicked()), this, SLOT(slotChangeFont()));
  connect(btnAppearFont, SIGNAL(clicked()), this, SLOT(slotChangeFont()));
  connect(btnAppearPreambleFont, SIGNAL(clicked()), this, SLOT(slotChangeFont()));

  REG_SH_TEXTFORMATENSEMBLE(Keyword);
  REG_SH_TEXTFORMATENSEMBLE(Comment);
  REG_SH_TEXTFORMATENSEMBLE(ParenMatch);
  REG_SH_TEXTFORMATENSEMBLE(ParenMismatch);
  REG_SH_TEXTFORMATENSEMBLE(LonelyParen);

  btnImportAddOn->setEnabled(klf_addons_canimport);
  btnRemoveAddOn->setEnabled(klf_addons_canimport);
    
  refreshAddOnList();
  refreshAddOnSelected();
  refreshPluginSelected();

  // dont load plugin data here as this dialog is created BEFORE plugins are loaded
  _pluginstuffloaded = false;
}

KLFSettings::~KLFSettings()
{
}

void KLFSettings::show()
{
  reset();
  initPluginControls();
  QDialog::show();
}


void KLFSettings::reset()
{
  int k;
  KLFBackend::klfSettings s = _mainwin->currentSettings();

  k = cbxLocale->findData(klfconfig.UI.locale);
  if (k == -1) {
    k = 0;
  }
  cbxLocale->setCurrentIndex(k);

  pathTempDir->setPath(QDir::toNativeSeparators(s.tempdir));
  pathLatex->setPath(s.latexexec);
  pathDvips->setPath(s.dvipsexec);
  pathGs->setPath(s.gsexec);
  pathEpstopdf->setPath(s.epstopdfexec);
  chkEpstopdf->setChecked( ! s.epstopdfexec.isEmpty() );
  spnLBorderOffset->setValue( s.lborderoffset );
  spnTBorderOffset->setValue( s.tborderoffset );
  spnRBorderOffset->setValue( s.rborderoffset );
  spnBBorderOffset->setValue( s.bborderoffset );

  chkSHEnable->setChecked(klfconfig.SyntaxHighlighter.configFlags
			  &  KLFLatexSyntaxHighlighter::Enabled);
  chkSHHighlightParensOnly->setChecked(klfconfig.SyntaxHighlighter.configFlags
				       &  KLFLatexSyntaxHighlighter::HighlightParensOnly);
  chkSHHighlightLonelyParen->setChecked(klfconfig.SyntaxHighlighter.configFlags
					&  KLFLatexSyntaxHighlighter::HighlightLonelyParen);

  for (k = 0; k < _textformats.size(); ++k) {
    if (_textformats[k].fmt->hasProperty(QTextFormat::ForegroundBrush))
      _textformats[k].fg->setColor(_textformats[k].fmt->foreground().color());
    else
      _textformats[k].fg->setColor(QColor());
    if (_textformats[k].fmt->hasProperty(QTextFormat::BackgroundBrush))
      _textformats[k].bg->setColor(_textformats[k].fmt->background().color());
    else
      _textformats[k].bg->setColor(QColor());
    if (_textformats[k].fmt->hasProperty(QTextFormat::FontWeight))
      _textformats[k].chkB->setChecked(_textformats[k].fmt->fontWeight() > 60);
    else
      _textformats[k].chkB->setCheckState(Qt::PartiallyChecked);
    if (_textformats[k].fmt->hasProperty(QTextFormat::FontItalic))
      _textformats[k].chkI->setChecked(_textformats[k].fmt->fontItalic());
    else
      _textformats[k].chkI->setCheckState(Qt::PartiallyChecked);
  }

  btnAppFont->setFont(klfconfig.UI.applicationFont);
  btnAppFont->setProperty("selectedFont", QVariant(klfconfig.UI.applicationFont));
  btnAppearFont->setFont(klfconfig.UI.latexEditFont);
  btnAppearFont->setProperty("selectedFont", QVariant(klfconfig.UI.latexEditFont));
  btnAppearPreambleFont->setFont(klfconfig.UI.preambleEditFont);
  btnAppearPreambleFont->setProperty("selectedFont", QVariant(klfconfig.UI.preambleEditFont));

  chkEnableRealTimePreview->setChecked(klfconfig.UI.enableRealTimePreview);
  spnPreviewWidth->setValue(klfconfig.UI.labelOutputFixedSize.width());
  spnPreviewHeight->setValue(klfconfig.UI.labelOutputFixedSize.height());

  chkEnableToolTipPreview->setChecked(klfconfig.UI.enableToolTipPreview);
  spnTooltipMaxWidth->setValue(klfconfig.UI.previewTooltipMaxSize.width());
  spnTooltipMaxHeight->setValue(klfconfig.UI.previewTooltipMaxSize.height());
}


void KLFSettings::initPluginControls()
{
  if ( _pluginstuffloaded ) {
    return;
  }

  lstPlugins->setColumnWidth(0, 185);

  // remove default Qt Designer Page
  QWidget * w = tbxPluginsConfig->widget(tbxPluginsConfig->currentIndex());
  tbxPluginsConfig->removeItem(tbxPluginsConfig->currentIndex());
  delete w;

  int j;
  int n_pluginconfigpages = 0;
  QTreeWidgetItem *litem;
  for (j = 0; j < klf_plugins.size(); ++j) {
    QString name = klf_plugins[j].name;
    QString title = klf_plugins[j].title;
    QString description = klf_plugins[j].description;
    KLFPluginGenericInterface *instance = klf_plugins[j].instance;
    
    litem = new QTreeWidgetItem(lstPlugins);
    litem->setCheckState(0,
			 klfconfig.Plugins.pluginConfig[name]["__loadenabled"].toBool() ?
			 Qt::Checked : Qt::Unchecked);
    litem->setText(0, title);
    
    litem->setData(0, KLFSETTINGS_ROLE_PLUGNAME, name);
    litem->setData(0, KLFSETTINGS_ROLE_PLUGINDEX, j);

    if ( instance != NULL ) {
      mPluginConfigWidgets[name] = instance->createConfigWidget( NULL );
      tbxPluginsConfig->addItem( mPluginConfigWidgets[name] , QIcon(":/pics/bullet22.png"), title );
      KLFPluginConfigAccess pconfa = klfconfig.getPluginConfigAccess(name, KLFPluginConfigAccess::Read);
      instance->loadFromConfig(mPluginConfigWidgets[name], &pconfa);
      n_pluginconfigpages++;
    }
  }
  if (n_pluginconfigpages == 0) {
    QLabel * lbl;
    lbl = new QLabel(tr("No Plugins have been loaded. Please install and enable individual plugins "
			"before trying to configure them."), tbxPluginsConfig);
    lbl->hide();
    lbl->setWordWrap(true);
    lbl->setMargin(20);
    tbxPluginsConfig->addItem(lbl, tr("No Plugins Loaded"));
  }

  _pluginstuffloaded = true;
}

void KLFSettings::refreshPluginSelected()
{
  QList<QTreeWidgetItem*> sel = lstPlugins->selectedItems();
  if (sel.size() != 1) {
    //    btnRemovePlugin->setEnabled(false);
    lblPluginInfo->setText("");
    return;
  }
  int k = sel[0]->data(0, KLFSETTINGS_ROLE_PLUGINDEX).toInt();
  if (k < 0 || k >= klf_plugins.size()) {
    //    btnRemovePlugin->setEnabled(false);
    lblPluginInfo->setText("");
    return;
  }

  //  btnRemovePlugin->setEnabled(true);
  int smallpointsize = QFontInfo(font()).pointSize() - 1;
  lblPluginInfo->setText(tr("<p style=\"-qt-block-indent: 0; text-indent: 0px; margin-bottom: 0px;\">"
			    "<tt><span style=\"font-style: italic; font-weight: 600;\">"
			    "Plugin Information</span></tt><br />\n"
			    "<tt>Name:</tt> <span style=\"font-weight:600;\">%1</span><br />\n"
			    "<tt>Author:</tt> <span style=\"font-weight:600;\">%2</span><br />\n"
			    "<tt>Description:</tt></p>\n"
			    "<p style=\"font-weight: 600; margin-top: 2px; margin-left: 25px;"
			    "   margin-bottom: 0px;\">%3</p>\n"
			    "<p style=\"-qt-block-indent: 0; text-indent: 0px; margin-top: 2px;\">\n"
			    "<tt>File Location:</tt> <span style=\"font-size: %4pt;\">%5</span>\n")
			 .arg(Qt::escape(klf_plugins[k].title)).arg(Qt::escape(klf_plugins[k].author))
			 .arg(Qt::escape(klf_plugins[k].description))
			 .arg(smallpointsize)
			 .arg(Qt::escape(QDir::toNativeSeparators(QFileInfo(klf_plugins[k].fpath)
								  .canonicalFilePath())))
			 );
}

void KLFSettings::removePlugin()
{
  // THIS FUNCTION IS NO LONGER USED. PLUGINS ARE AUTOMATICALLY REMOVED WHEN THE CORRESPONDING
  // ADD-ON IS REMOVED. THIS FUNCTION IS KEPT IN CASE I CHANGE SOMETHING IN THE FUTURE.

  QList<QTreeWidgetItem*> sel = lstPlugins->selectedItems();
  if (sel.size() != 1) {
    qWarning("KLFSettings::removePlugin: No Selection or many selection");
    return;
  }
  QTreeWidgetItem * selectedItem = sel[0];
  int k = selectedItem->data(0, KLFSETTINGS_ROLE_PLUGINDEX).toInt();
  if (k < 0 || k >= klf_plugins.size()) {
    qWarning("KLFSettings::removePlugin: Error: What's going on?? k=%d > klf_plugins.size=%d", k, klf_plugins.size());
    return;
  }

  QMessageBox confirmdlg(this);
  confirmdlg.setIcon(QMessageBox::Warning);
  confirmdlg.setWindowTitle(tr("Remove Plugin?"));
  confirmdlg.setText(tr("<qt>Are you sure you want to remove Plugin <i>%1</i>?</qt>").arg(klf_plugins[k].title));
  confirmdlg.setDetailedText(tr("The Plugin File %1 will be removed from disk.").arg(klf_plugins[k].fpath));
  confirmdlg.setStandardButtons(QMessageBox::Yes | QMessageBox::Cancel);
  confirmdlg.setEscapeButton(QMessageBox::Cancel);
  confirmdlg.setDefaultButton(QMessageBox::Cancel);

  int confirmation = confirmdlg.exec();
  if (confirmation != QMessageBox::Yes) {
    // action cancelled by user
    return;
  }

  bool r = QFile::remove(klf_plugins[k].fpath);

  if ( r ) {
    QMessageBox::information(this, tr("Remove Plugin"),
			     tr("<p style=\"-qt-block-indent: 0; text-indent: 0px;\">Please note the following:<br />\n"
				"<ul><li>You need to restart KLatexFormula for changes to take effect\n"
				"<li>If this plugin was privided in an add-on, you need to remove the corresponding "
				"add-on too or the plugin will be automatically re-installed."
				"</p>"));
    // remove plugin list item
    delete selectedItem;
  } else {
    qWarning("Failed to remove plugin '%s'", qPrintable(klf_plugins[k].fpath));
    QMessageBox::critical(this, tr("Error"), tr("Failed to remove Plugin."));
  }

  refreshAddOnList();
  refreshAddOnSelected();
}

void KLFSettings::removePlugin(const QString& fname)
{
  int k;
  for (k = 0; k < klf_plugins.size() && klf_plugins[k].fname != fname; ++k)
    ;
  if (k < 0 || k >= klf_plugins.size()) {
    qWarning("KLFSettings::removePlugin: internal error: didn't find plugin name %s", qPrintable(fname));
    return;
  }

  bool r = QFile::remove(klf_plugins[k].fpath);

  if ( r ) {
    // find corresponding tree widget item
    QTreeWidgetItemIterator it(lstPlugins);
    while (*it) {
      if ( (*it)->data(0, KLFSETTINGS_ROLE_PLUGINDEX).toInt() == k ) {
	// remove plugin list item
	delete (*it);
	break;
      }
      ++it;
    }
  } else {
    qWarning("Failed to remove plugin '%s'", qPrintable(klf_plugins[k].fpath));
    QMessageBox::critical(this, tr("Error"), tr("Failed to remove Plugin %1.").arg(klf_plugins[k].title));
  }
}


bool KLFSettings::setDefaultFor(const QString& progname, const QString& guessedprog, bool required,
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
    QMessageBox msgbox(QMessageBox::Critical, tr("Error"), tr("Could not find %1 executable !").arg(progname),
		       QMessageBox::Ok);
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
  KLFConfig tempobject;
  KLFConfig::loadDefaultBackendPaths(&tempobject, true);
  if ( ! QFileInfo(pathTempDir->path()).isDir() )
    pathTempDir->setPath(QDir::toNativeSeparators(tempobject.BackendSettings.tempDir));
  setDefaultFor("latex", tempobject.BackendSettings.execLatex, true, pathLatex);
  setDefaultFor("dvips", tempobject.BackendSettings.execDvips, true, pathDvips);
  setDefaultFor("gs", tempobject.BackendSettings.execGs, true, pathGs);
  bool r = setDefaultFor("epstopdf", tempobject.BackendSettings.execEpstopdf, false, pathEpstopdf);
  chkEpstopdf->setChecked(r);
}


void KLFSettings::refreshAddOnList()
{
  lstAddOns->clear();
  lstAddOns->setColumnWidth(0, 160);

  // explore all addons
  int k;
  for (k = 0; k < klf_addons.size(); ++k) {
    QTreeWidgetItem *item = new QTreeWidgetItem(lstAddOns);
    item->setData(0, KLFSETTINGS_ROLE_ADDONINDEX, QVariant((int)k));

    item->setText(0, klf_addons[k].title);
    item->setText(1, klf_addons[k].description);

    if (klf_addons[k].isfresh) {
      item->setBackground(0, QColor(200, 255, 200));
      item->setBackground(1, QColor(200, 255, 200));
    } else if (klf_addons[k].islocal) {
      item->setBackground(0, QColor(200, 200, 255));
      item->setBackground(1, QColor(200, 200, 255));
    }
  }
}

void KLFSettings::refreshAddOnSelected()
{
  QList<QTreeWidgetItem*> sel = lstAddOns->selectedItems();
  if (sel.size() != 1) {
    lblAddOnInfo->setText("");
    btnRemoveAddOn->setEnabled(false);
    return;
  }
  int k = sel[0]->data(0, KLFSETTINGS_ROLE_ADDONINDEX).toInt();
  if (k < 0 || k >= klf_addons.size()) {
    lblAddOnInfo->setText("");
    btnRemoveAddOn->setEnabled(false);
    return;
  }

  // enable remove button only if this addon is "local", i.e. precisely removable
  btnRemoveAddOn->setEnabled(klf_addons[k].islocal);

  int smallpointsize = QFontInfo(font()).pointSize() - 1;
  lblAddOnInfo->setText(tr("<p style=\"-qt-block-indent: 0; text-indent: 0px; margin-bottom: 0px\">"
			   "<tt><span style=\"font-style: italic; font-weight: 600;\">"
			   "Add-On Information</span></tt><br />\n"
			   "<tt>Name:</tt> <span style=\"font-weight:600;\">%1</span><br />\n"
			   "<tt>Author:</tt> <span style=\"font-weight:600;\">%2</span><br />\n"
			   "<tt>Description:</tt></p>\n"
			    "<p style=\"font-weight: 600; margin-top: 2px; margin-left: 25px;"
			   "   margin-bottom: 0px;\">%3</p>\n"
			   "<p style=\"-qt-block-indent: 0; text-indent: 0px; margin-top: 2px;\">\n"
			   "<tt>File Name:</tt> <span style=\"font-size: %5pt;\">%4</span><br />\n"
			   "<tt>File Location:</tt> <span style=\"font-size: %5pt;\">%6</span><br />\n"
			   "<tt><i>%7</i></tt>").arg(Qt::escape(klf_addons[k].title))
			.arg(Qt::escape(klf_addons[k].author)).arg(Qt::escape(klf_addons[k].description))
			.arg(Qt::escape(klf_addons[k].fname))
			.arg(smallpointsize)
			.arg(Qt::escape(QDir::toNativeSeparators(QFileInfo(klf_addons[k].dir)
								 .canonicalFilePath()) + QDir::separator()))
			.arg( klf_addons[k].islocal ?
			      tr("Add-On installed locally") :
			      tr("Add-On installed globally on system") )
			);

}


void KLFSettings::importAddOn()
{
  QStringList efnames = QFileDialog::getOpenFileNames(this, tr("Please select add-on file(s) to import"),
						      QString(), "Qt Resource Files (*.rcc)");
  int i;
  QString destination = klfconfig.homeConfigDir + "/rccresources/";
  for (i = 0; i < efnames.size(); ++i) {
    QString destfpath = destination + QFileInfo(efnames[i]).fileName();
    if ( QFile::exists(destfpath) ) {
      QMessageBox::critical(this, tr("Error"), tr("An Add-On with the same file name has already been imported."));
    } else {
      bool r = QFile::copy(efnames[i], destfpath);
      if ( r ) {
	// import succeeded, show the add-on as fresh.
	KLFAddOnInfo addoninfo(destfpath);
	addoninfo.isfresh = true;
	klf_addons.append(addoninfo);
	refreshAddOnList();
      } else {
	QMessageBox::critical(this, tr("Error"), tr("Import of add-on file %1 failed.").arg(efnames[i]));
      }
    }
  }
  // display message to user to restart KLatexFormula, if needed
  if (i > 0) {
    QMessageBox::information(this, tr("Import"), tr("Please restart KLatexFormula for changes to take effect."));
  }
}

void KLFSettings::removeAddOn()
{
  QList<QTreeWidgetItem*> sel = lstAddOns->selectedItems();
  if (sel.size() != 1) {
    qWarning("Expected single add-on selection for removal !");
    return;
  }

  int k = sel[0]->data(0, KLFSETTINGS_ROLE_ADDONINDEX).toInt();
  if (k < 0 || k >= klf_addons.size()) {
    // what's going on ???
    return;
  }

  QMessageBox confirmdlg(this);
  confirmdlg.setIcon(QMessageBox::Warning);
  confirmdlg.setWindowTitle(tr("Remove Add-On?"));
  confirmdlg.setText(tr("<qt>Are you sure you want to remove Add-On <i>%1</i>?</qt>").arg(klf_addons[k].title));
  confirmdlg.setDetailedText(tr("The Add-On File %1 will be removed from disk.").arg(klf_addons[k].fpath));
  confirmdlg.setStandardButtons(QMessageBox::Yes | QMessageBox::Cancel);
  confirmdlg.setEscapeButton(QMessageBox::Cancel);
  confirmdlg.setDefaultButton(QMessageBox::Cancel);

  int confirmation = confirmdlg.exec();
  if (confirmation != QMessageBox::Yes) {
    // action cancelled by user
    return;
  }

  bool r = QFile::remove(klf_addons[k].fpath);
  // remove all corresponding plugins too
  int j;
  if ( r ) {
    QMessageBox::information(this, tr("Remove Add-On"), tr("Please restart KLatexFormula for changes to take effect."));
  } else {
    qWarning("Failed to remove add-on '%s'", qPrintable(klf_addons[k].fpath));
    QMessageBox::critical(this, tr("Error"), tr("Failed to remove Add-On."));
    return;
  }

  // remove all corresponding plug-ins
  for (j = 0; j < klf_addons[k].plugins.size(); ++j)
    removePlugin(klf_addons[k].plugins[j]);

  klf_addons.removeAt(k);

  refreshAddOnList();
  refreshAddOnSelected();
}

void KLFSettings::slotChangeFont()
{
  QWidget *w = dynamic_cast<QWidget*>(sender());
  if ( w == 0 )
    return;
  QFont fnt = QFontDialog::getFont(0, w->property("selectedFont").value<QFont>(), this);
  w->setFont(fnt);
  w->setProperty("selectedFont", QVariant(fnt));
}

void KLFSettings::apply()
{
  int k;
  // apply settings here

  // create a temporary settings object
  KLFBackend::klfSettings s;

  s.tempdir = QDir::fromNativeSeparators(pathTempDir->path());
  s.latexexec = pathLatex->path();
  s.dvipsexec = pathDvips->path();
  s.gsexec = pathGs->path();
  s.epstopdfexec = QString();
  if (chkEpstopdf->isChecked()) {
    s.epstopdfexec = pathEpstopdf->path();
  }
  s.lborderoffset = spnLBorderOffset->value();
  s.tborderoffset = spnTBorderOffset->value();
  s.rborderoffset = spnRBorderOffset->value();
  s.bborderoffset = spnBBorderOffset->value();

  _mainwin->applySettings(s);

  if (chkSHEnable->isChecked())
    klfconfig.SyntaxHighlighter.configFlags |= KLFLatexSyntaxHighlighter::Enabled;
  else
    klfconfig.SyntaxHighlighter.configFlags &= ~KLFLatexSyntaxHighlighter::Enabled;
  if (chkSHHighlightParensOnly->isChecked())
    klfconfig.SyntaxHighlighter.configFlags |= KLFLatexSyntaxHighlighter::HighlightParensOnly;
  else
    klfconfig.SyntaxHighlighter.configFlags &= ~KLFLatexSyntaxHighlighter::HighlightParensOnly;
  if (chkSHHighlightLonelyParen->isChecked())
    klfconfig.SyntaxHighlighter.configFlags |= KLFLatexSyntaxHighlighter::HighlightLonelyParen;
  else
    klfconfig.SyntaxHighlighter.configFlags &= ~KLFLatexSyntaxHighlighter::HighlightLonelyParen;

  for (k = 0; k < _textformats.size(); ++k) {
    QColor c = _textformats[k].fg->color();
    if (c.isValid())
      _textformats[k].fmt->setForeground(c);
    else
      _textformats[k].fmt->clearForeground();
    c = _textformats[k].bg->color();
    if (c.isValid())
      _textformats[k].fmt->setBackground(c);
    else
      _textformats[k].fmt->clearBackground();
    Qt::CheckState b = _textformats[k].chkB->checkState();
    if (b == Qt::PartiallyChecked)
      _textformats[k].fmt->clearProperty(QTextFormat::FontWeight);
    else if (b == Qt::Checked)
      _textformats[k].fmt->setFontWeight(QFont::Bold);
    else
      _textformats[k].fmt->setFontWeight(QFont::Normal);
    Qt::CheckState it = _textformats[k].chkI->checkState();
    if (it == Qt::PartiallyChecked)
      _textformats[k].fmt->clearProperty(QTextFormat::FontItalic);
    else
      _textformats[k].fmt->setFontItalic( it == Qt::Checked );
  }

  // language settings
  QString localename = cbxLocale->itemData(cbxLocale->currentIndex()).toString();
  bool localechanged =  (  klfconfig.UI.locale != localename  ) ;
  klfconfig.UI.locale = localename;
  if (localechanged) {
    QMessageBox::information(this, tr("Language changed"),
			     tr("You need to restart KLatexFormula for your new language settings to take effect."));
  }
  //  klf_main_do_the_change_of_locale_and_load_translators(...);
  //  QList<QWidget*> uilist;
  //  uilist << _mainwin << _mainwin->libraryBrowserWidget() << _mainwin->latexSymbolsWidget()
  //	 << _mainwin->styleManagerWidget() << this ;
  //  for (k = 0; k < uilist.size(); ++k) {
  //    uilist[k]->retranlsateUi(uilist[k]);
  //  }


  // font settings
  klfconfig.UI.applicationFont = btnAppFont->property("selectedFont").value<QFont>();
  qApp->setFont(klfconfig.UI.applicationFont);
  klfconfig.UI.latexEditFont = btnAppearFont->property("selectedFont").value<QFont>();
  _mainwin->setTxtLatexFont(klfconfig.UI.latexEditFont);
  klfconfig.UI.preambleEditFont = btnAppearPreambleFont->property("selectedFont").value<QFont>();
  _mainwin->setTxtPreambleFont(klfconfig.UI.preambleEditFont);
  // recalculate window sizes etc.
  _mainwin->refreshWindowSizes();

  klfconfig.UI.labelOutputFixedSize = QSize(spnPreviewWidth->value(), spnPreviewHeight->value());
  klfconfig.UI.enableRealTimePreview = chkEnableRealTimePreview->isChecked();

  klfconfig.UI.previewTooltipMaxSize = QSize(spnTooltipMaxWidth->value(), spnTooltipMaxHeight->value());
  klfconfig.UI.enableToolTipPreview = chkEnableToolTipPreview->isChecked();

  // save plugin config
  bool warnneedrestart = false;
  QTreeWidgetItemIterator it(lstPlugins);
  while (*it) {
    int j = (*it)->data(0, KLFSETTINGS_ROLE_PLUGINDEX).toInt();
    QString name = (*it)->data(0, KLFSETTINGS_ROLE_PLUGNAME).toString();
    bool loadenable = ( (*it)->checkState(0) == Qt::Checked ) ;
    if (loadenable != klfconfig.Plugins.pluginConfig[name]["__loadenabled"])
      warnneedrestart = true;
    klfconfig.Plugins.pluginConfig[name]["__loadenabled"] = loadenable;

    if (klf_plugins[j].instance != NULL) {
      KLFPluginConfigAccess pconfa = klfconfig.getPluginConfigAccess(name, KLFPluginConfigAccess::ReadWrite);
      klf_plugins[j].instance->saveToConfig(mPluginConfigWidgets[name], &pconfa);
    }

    ++it;
  }
  if (warnneedrestart) {
    QMessageBox::information(this, tr("Restart KLatexFormula"),
			     tr("You need to restart KLatexFormula for your changes to take effect."));
  }

  _mainwin->saveSettings();
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
