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

#include <klfutil.h>
#include <klfmainwin.h>
#include <klfsettings.h>
#include <klflatexsymbols.h>
#include <klfconfig.h>

#include "skin.h"


SkinConfigWidget::SkinConfigWidget(QWidget *parent, KLFPluginConfigAccess *conf)
  : QWidget(parent), config(conf), _modified(false)
{
  setupUi(this);

  connect(cbxSkin, SIGNAL(activated(int)), this, SLOT(skinSelected(int)));
  connect(btnRefresh, SIGNAL(clicked()), this, SLOT(refreshSkin()));
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



// --------------------------------------------------------------------------------


void SkinPlugin::initialize(QApplication *app, KLFMainWin *mainWin, KLFPluginConfigAccess *rwconfig)
{
  Q_INIT_RESOURCE(skindata);

  _mainwin = mainWin;
  _app = app;
  //  _defaultstyle = NULL;
  _defaultstyle = app->style();
  // aggressively take possession of this style object
  _defaultstyle->setParent(this);

  _config = rwconfig;

  // ensure reasonable non-empty value in config
  if ( rwconfig->readValue("skinfilename").isNull() )
    rwconfig->writeValue("skinfilename", QString(":/plugindata/skin/stylesheets/default.qss"));

  applySkin(rwconfig);
}


void SkinPlugin::applySkin(KLFPluginConfigAccess *config)
{
  qDebug("Applying skin!");
  QString ssfn = config->readValue("skinfilename").toString();
  QString stylesheet = SkinConfigWidget::getStyleSheet(ssfn);

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
    w->setProperty("klfTopLevelWidget", QVariant(true));
    w->setAttribute(Qt::WA_StyledBackground);
    w->setStyleSheet(stylesheet);
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
