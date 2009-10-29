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

#include <QtCore>
#include <QtGui>
#include <QtUiTools/QtUiTools>

#include <klfmainwin.h>
#include <klflibrary.h>
#include <klflatexsymbols.h>
#include <klfconfig.h>

#include "skin.h"


SkinConfigObject::SkinConfigObject(QWidget *skinconfigwidget)
  : QObject(skinconfigwidget), _skinconfigwidget(skinconfigwidget)
{
  setObjectName("SkinConfigObject");

  cbxSkin = _skinconfigwidget->findChild<QComboBox*>("cbxSkin");
  txtStyleSheet = _skinconfigwidget->findChild<QTextEdit*>("txtStyleSheet");

  QDir stylesheetdir(":/plugindata/skin/stylesheets/");
  QStringList skinlist = stylesheetdir.entryList(QStringList() << "*.qss", QDir::Files);
  int k;
  for (k = 0; k < skinlist.size(); ++k) {
    QString skintitle = QFileInfo(skinlist[k]).baseName();
    cbxSkin->addItem(skintitle, QVariant(skinlist[k]));
  }
  cbxSkin->addItem(tr("Custom ..."), QVariant(QString::null));

  connect(cbxSkin, SIGNAL(activated(int)), this, SLOT(skinSelected(int)));
  connect(txtStyleSheet, SIGNAL(textChanged()), this, SLOT(stylesheetChanged()));
}

void SkinConfigObject::load(QString skin, QString stylesheet)
{
  txtStyleSheet->setPlainText(stylesheet);
  int k = cbxSkin->findData(skin);
  if ( k >= 0 ) {
    cbxSkin->blockSignals(true);
    cbxSkin->setCurrentIndex(k);
    cbxSkin->blockSignals(false);
  }
}

void SkinConfigObject::skinSelected(int index)
{
  QString skinname = cbxSkin->itemData(index).toString();

  if (skinname.isNull())
    return;

  QFile f(":/plugindata/skin/stylesheets/" + skinname);
  f.open(QIODevice::ReadOnly);
  QByteArray stylesheetdata = f.readAll();
  f.close();
  QString stylesheet = QString::fromUtf8(stylesheetdata.constData(), stylesheetdata.size());

  txtStyleSheet->blockSignals(true);
  txtStyleSheet->setPlainText(stylesheet);
  txtStyleSheet->blockSignals(false);
}

void SkinConfigObject::stylesheetChanged()
{
  cbxSkin->setCurrentIndex(cbxSkin->count()-1);
}


// --------------------------------------------------------------------------------


void SkinPlugin::initialize(QApplication */*app*/, KLFMainWin *mainWin, KLFPluginConfigAccess *rwconfig)
{
  _mainwin = mainWin;

  // ensure reasonable non-empty value in config
  if ( rwconfig->readValue("stylesheet").isNull() ) {
    rwconfig->writeValue("stylesheet",
			 QString("QWidget {\n"
				 "  background-color: rgb(128, 128, 255);\n"
				 "  selection-background-color: rgb(0, 0, 64);\n"
				 "  selection-color: rgb(128, 128, 255);\n"
				 "  color: rgb(255, 255, 255);\n"
				 "}"));
  }

  applySkin(rwconfig);
}

void SkinPlugin::applySkin(KLFPluginConfigAccess *config)
{
  QVariant ss = config->readValue("stylesheet");
  QString stylesheet = ss.toString();
  _mainwin->setStyleSheet(stylesheet);
  _mainwin->libraryBrowserWidget()->setStyleSheet(stylesheet);
  _mainwin->latexSymbolsWidget()->setStyleSheet(stylesheet);
}

QWidget * SkinPlugin::createConfigWidget(QWidget *parent)
{
  QUiLoader loader;
  QFile file(":/plugindata/skin/skinconfigwidget.ui");
  file.open(QFile::ReadOnly);
  QWidget *w = loader.load(&file, parent);
  file.close();

  w->setProperty("SkinConfigWidget", true);

  (void) new SkinConfigObject(w);
  return w;
}

void SkinPlugin::loadConfig(QWidget *confwidget, KLFPluginConfigAccess *config)
{
  if (confwidget->property("SkinConfigWidget").toBool() != true) {
    fprintf(stderr, "Error: bad config widget given !!\n");
    return;
  }
  SkinConfigObject * o = confwidget->findChild<SkinConfigObject*>("SkinConfigObject");
  o->load(config->readValue("skin").toString(),
	  config->readValue("stylesheet").toString());
}
void SkinPlugin::saveConfig(QWidget *confwidget, KLFPluginConfigAccess *config)
{
  if (confwidget->property("SkinConfigWidget").toBool() != true) {
    fprintf(stderr, "Error: bad config widget given !!\n");
    return;
  }

  SkinConfigObject * o = confwidget->findChild<SkinConfigObject*>("SkinConfigObject");

  QString skin = o->currentSkin();
  QString stylesheet = o->currentStyleSheet();

  config->writeValue("skin", QVariant(skin));
  config->writeValue("stylesheet", QVariant(stylesheet));

  applySkin(config);
}





// Export Plugin
Q_EXPORT_PLUGIN2(skin, SkinPlugin);
