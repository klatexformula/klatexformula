/***************************************************************************
 *   file plugins/skin/skin.cpp
 *   This file is part of the KLatexFormula Project.
 *   Copyright (C) 2009 by Philippe Faist
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

#include <QtCore>
#include <QtGui>

#include <klfmainwin.h>
#include <klfsettings.h>
#include <klflatexsymbols.h>
#include <klfconfig.h>
#include <klfguiutil.h>
#include <klfutil.h>

#include "skin.h"


SkinConfigWidget::SkinConfigWidget(QWidget *parent, KLFPluginConfigAccess *conf)
  : QWidget(parent), config(conf), _modified(false)
{
  setupUi(this);

  connect(cbxSkin, SIGNAL(activated(int)), this, SLOT(skinSelected(int)));
  connect(btnRefresh, SIGNAL(clicked()), this, SLOT(refreshSkin()));
  connect(btnInstallSkin, SIGNAL(clicked()), this, SLOT(installSkin()));
}

void SkinConfigWidget::loadSkinList(QString skinfn)
{
  qDebug("SkinConfigWidget::loadSkinList(%s)", qPrintable(skinfn));
  cbxSkin->clear();

  QStringList stylesheetsdirs;
  stylesheetsdirs
    << ":/plugindata/skin/stylesheets/" << config->homeConfigPluginDataDir(true) + "/stylesheets/";

  int k, j;
  int indf = -1;
  for (j = 0; j < stylesheetsdirs.size(); ++j) {
    if ( ! QFile::exists(stylesheetsdirs[j]) )
      if ( ! klfEnsureDir(stylesheetsdirs[j]) )
	continue;

    QDir stylesheetdir(stylesheetsdirs[j]);
    QStringList skinlist = stylesheetdir.entryList(QStringList() << "*.qss", QDir::Files);
    for (k = 0; k < skinlist.size(); ++k) {
      QString skintitle = QFileInfo(skinlist[k]).baseName();
      QString fn = stylesheetdir.absoluteFilePath(skinlist[k]);
      qDebug("\tgot skin: %s : %s", qPrintable(skintitle), qPrintable(fn));
      if (fn == skinfn) {
	indf = cbxSkin->count();
      }
      cbxSkin->addItem(skintitle, fn);
    }
  }
  // ---
  if (indf >= 0) {
    cbxSkin->blockSignals(true);
    cbxSkin->setCurrentIndex(indf);
    cbxSkin->blockSignals(false);
  }
}

void SkinConfigWidget::skinSelected(int index)
{
  if (index < 0 || index >= cbxSkin->count()-1)
    return;
  _modified = true;
}

void SkinConfigWidget::refreshSkin()
{
  loadSkinList(currentSkin());
  _modified = true; // and re-set this skin after click on 'apply'
}

void SkinConfigWidget::installSkin()
{
  QString docs = QDesktopServices::storageLocation(QDesktopServices::DocumentsLocation);
  QString fname =
    QFileDialog::getOpenFileName(this, tr("Open Skin File"), docs,
				 tr("Qt Style Sheet Files (*.qss);;All files (*)"));
  if (fname.isEmpty())
    return;
  if ( ! QFile::exists(fname) ) {
    qWarning()<<"SkinConfigWidget::installSkin: File "<<fname<<" does not exist.";
    return;
  }
  QString target = config->homeConfigPluginDataDir(true) + "/stylesheets/" + QFileInfo(fname).fileName();
  if ( QFile::exists(target) ) {
    QMessageBox::StandardButton res =
      QMessageBox::warning(this, tr("Skin Already Exists") ,
			   tr("A Skin Named \"%1\" is already installed. Overwrite it?")
			   .arg(QFileInfo(fname).fileName()) ,
			   QMessageBox::Yes|QMessageBox::Cancel, QMessageBox::Cancel);
    if (res != QMessageBox::Yes)
      return;
    // yes, overwrite
    QFile::remove(target);
  }
  bool ok = QFile::copy(fname, target);
  if ( !ok ) {
    QMessageBox::critical(this, tr("Error"),
			  tr("Failed to install skin \"%1\".").arg(QFileInfo(fname).fileName()));
  }
  refreshSkin();
}


// --------------------------------------------------------------------------------


void SkinPlugin::initialize(QApplication *app, KLFMainWin *mainWin, KLFPluginConfigAccess *rwconfig)
{
  qDebug("Initializing Skin plugin (compiled for KLF version %s)", KLF_VERSION_STRING);
  Q_INIT_RESOURCE(skindata);

  _mainwin = mainWin;
  _app = app;
  //  _defaultstyle = NULL;
  _defaultstyle = app->style();
  // aggressively take possession of this style object
  _defaultstyle->setParent(this);

  _config = rwconfig;

  { // add links for skins to what's new page
    QString s =
      tr("<p>Some new <b>interface skins</b> are available in this version. You may want "
	  "to try the <a href=\"%1\">papyrus skin</a>, the <a href=\"%2\">galaxy skin</a>, "
	  "the <a href=\"%3\">flat skin</a>, or the <a href=\"%4\">style-default skin</a>."
	  "</p>", "[[help new features additional text]]")
      .arg("klfaction:/skin_set?skin=:/plugindata/skin/stylesheets/papyrus.qss",
	   "klfaction:/skin_set?skin=:/plugindata/skin/stylesheets/galaxy.qss",
	   "klfaction:/skin_set?skin=:/plugindata/skin/stylesheets/flat.qss",
	   "klfaction:/skin_set?skin=:/plugindata/skin/stylesheets/default.qss");
    _mainwin->addWhatsNewText(s);
    _mainwin->registerHelpLinkAction("/skin_set", this, "changeSkinHelpLinkAction", true);
  }

  // ensure reasonable non-empty value in config
  if ( rwconfig->readValue("skinfilename").isNull() )
    rwconfig->writeValue("skinfilename", QString(":/plugindata/skin/stylesheets/default.qss"));

  applySkin(rwconfig);
}

void SkinPlugin::changeSkin(const QString& newSkin)
{
  qDebug()<<KLF_FUNC_NAME<<": skin="<<newSkin;
  if (_config->readValue("skinfilename").toString() == newSkin)
    return; // already right

  _config->writeValue("skinfilename", QVariant::fromValue<QString>(newSkin));
  applySkin(_config);
  emit skinChanged(newSkin);
}
void SkinPlugin::changeSkinHelpLinkAction(const QUrl& link)
{
  qDebug()<<KLF_FUNC_NAME<<": link="<<link;
  changeSkin(link.queryItemValue("skin"));
}


void SkinPlugin::applySkin(KLFPluginConfigAccess *config)
{
  qDebug("Applying skin!");
  QString ssfn = config->readValue("skinfilename").toString();
  QString stylesheet = SkinConfigWidget::getStyleSheet(ssfn);

  KLFPleaseWaitPopup pleaseWaitPopup(tr("Applying skin, please wait ..."), _mainwin);

  if (_mainwin->isVisible())
    pleaseWaitPopup.showPleaseWait();

  if (_app->style() != _defaultstyle) {
    _app->setStyle(_defaultstyle);
    // but aggressively keep possession of style
    _defaultstyle->setParent(this);
    // and refresh mainwin's idea of application's style
    _mainwin->setProperty("widgetStyle", QVariant(QString::null));
  }
  // set style sheet to whole application (doesn't work...)
  //  _app->setStyleSheet(stylesheet);

  // set top-level widgets' klfTopLevelWidget property to TRUE, and
  // apply our style sheet to all top-level widgets
  QWidgetList toplevelwidgets = _app->topLevelWidgets();
  int k;
  for (k = 0; k < toplevelwidgets.size(); ++k) {
    QWidget *w = toplevelwidgets[k];
    QString objnm = w->objectName();
    if (!_baseStyleSheets.contains(objnm))
      _baseStyleSheets[objnm] = w->styleSheet();

    w->setProperty("klfTopLevelWidget", QVariant(true));
    w->setAttribute(Qt::WA_StyledBackground);
    w->setStyleSheet(_baseStyleSheets[objnm] + "\n" + stylesheet);
  }
  // #ifndef Q_WS_WIN
  //   // BUG? tab widget in settings dialog is always un-skinned after an "apply"...?
  //   KLFSettings *settingsDialog = _mainwin->findChild<KLFSettings*>();
  //   if (settingsDialog) {
  //     QTabWidget * tabs = settingsDialog->findChild<QTabWidget*>("tabs");
  //     if (tabs) {
  //       qDebug("Setting stylesheet to tabs");
  //       tabs->setStyleSheet(stylesheet);
  //     }
  //   }
  // #endif
}

QWidget * SkinPlugin::createConfigWidget(QWidget *parent)
{
  QWidget *w = new SkinConfigWidget(parent, _config);
  w->setProperty("SkinConfigWidget", true);
  connect(this, SIGNAL(skinChanged(const QString&)), w, SLOT(loadSkinList(const QString&)));
  return w;
}

void SkinPlugin::loadFromConfig(QWidget *confwidget, KLFPluginConfigAccess *config)
{
  if (confwidget->property("SkinConfigWidget").toBool() != true) {
    fprintf(stderr, "Error: bad config widget given !!\n");
    return;
  }
  SkinConfigWidget * o = qobject_cast<SkinConfigWidget*>(confwidget);
  o->loadSkinList(config->readValue("skinfilename").toString());
  // reset modified status
  o->getModifiedAndReset();
}
void SkinPlugin::saveToConfig(QWidget *confwidget, KLFPluginConfigAccess *config)
{
  if (confwidget->property("SkinConfigWidget").toBool() != true) {
    fprintf(stderr, "Error: bad config widget given !!\n");
    return;
  }

  SkinConfigWidget * o = qobject_cast<SkinConfigWidget*>(confwidget);

  QString skinfn = o->currentSkin();

  config->writeValue("skinfilename", QVariant::fromValue<QString>(skinfn));

  if ( o->getModifiedAndReset() )
    applySkin(config);
}





// Export Plugin
Q_EXPORT_PLUGIN2(skin, SkinPlugin);
