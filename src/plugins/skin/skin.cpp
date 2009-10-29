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



SkinConfigObject::SkinConfigObject(QWidget *skinconfigwidget, KLFPluginConfigAccess *conf)
  : QObject(skinconfigwidget), _skinconfigwidget(skinconfigwidget), config(conf)
{
  setObjectName("SkinConfigObject");

  cbxSkin = _skinconfigwidget->findChild<QComboBox*>("cbxSkin");
  txtStyleSheet = _skinconfigwidget->findChild<QTextEdit*>("txtStyleSheet");
  QPushButton *btnSaveCustom = _skinconfigwidget->findChild<QPushButton*>("btnSaveCustom");

  connect(cbxSkin, SIGNAL(activated(int)), this, SLOT(skinSelected(int)));
  connect(txtStyleSheet, SIGNAL(textChanged()), this, SLOT(stylesheetChanged()));
  connect(btnSaveCustom, SIGNAL(clicked()), this, SLOT(saveCustom()));
}

void SkinConfigObject::load(QString skin, QString stylesheet)
{
  cbxSkin->clear();
  _skins.clear();

  QDir stylesheetdir(":/plugindata/skin/stylesheets/");
  QStringList skinlist = stylesheetdir.entryList(QStringList() << "*.qss", QDir::Files);
  int k;
  int indf = -1;
  for (k = 0; k < skinlist.size(); ++k) {
    QString skintitle = QFileInfo(skinlist[k]).baseName();

    QFile f(stylesheetdir.absoluteFilePath(skinlist[k]));
    f.open(QIODevice::ReadOnly);
    QByteArray stylesheetdata = f.readAll();
    f.close();
    QString stylesheet = QString::fromUtf8(stylesheetdata.constData(), stylesheetdata.size());

    Skin sk(true, skinlist[k], skintitle, stylesheet);
    _skins.push_back(sk);
    if (skinlist[k] == skin)
      indf = _skins.size()-1;
    cbxSkin->addItem(skintitle, QVariant(_skins.size()-1));
  }
  QList<QVariant> customskins
    = config->readValue("CustomSkins").value<QList<QVariant> >() ;
  for (k = 0; k < customskins.size(); ++k) {
    printf("%d...\n", k);
    QList<QVariant> vthiscustomskin = customskins[k].value<QList<QVariant> >();
    Skin sk(false, vthiscustomskin[0].toString(), vthiscustomskin[1].toString(),
		  vthiscustomskin[2].toString());
    _skins.push_back(sk);
    if (sk.name == skin)
      indf = _skins.size()-1;
    cbxSkin->addItem(sk.title, QVariant(_skins.size()-1));
  }

  cbxSkin->addItem(tr("Custom ..."), QVariant(QString::null));

  // ---

  txtStyleSheet->setPlainText(stylesheet);
  if (indf >= 0) {
    k = cbxSkin->findData(indf);
    if ( k >= 0 ) {
      cbxSkin->blockSignals(true);
      cbxSkin->setCurrentIndex(k);
      cbxSkin->blockSignals(false);
    }
  }
}

void SkinConfigObject::skinSelected(int index)
{
  if (index < 0 || index >= cbxSkin->count()-1)
    return;

  int k = cbxSkin->itemData(index).toInt();

  txtStyleSheet->blockSignals(true);
  txtStyleSheet->setPlainText(_skins[k].stylesheet);
  txtStyleSheet->blockSignals(false);
}

void SkinConfigObject::stylesheetChanged()
{
  cbxSkin->setCurrentIndex(cbxSkin->count()-1);
}

void SkinConfigObject::saveCustom()
{
  bool ok;
  QString name = QInputDialog::getText(_skinconfigwidget, tr("Skin Name"), tr("Please enter skin name:"),
				       QLineEdit::Normal, tr("[New Skin Name]"), &ok);
  if (!ok)
    return;

  _skins.push_back(Skin(false, name, name, txtStyleSheet->toPlainText()));
  cbxSkin->insertItem(cbxSkin->count()-1, name, _skins.size()-1);
  cbxSkin->setCurrentIndex(cbxSkin->count()-2);
  
  QList<QVariant> customskins;
  int k;
  for (k = 0; k < _skins.size(); ++k) {
    if (_skins[k].builtin)
      continue;
    QList<QVariant> thisskin;
    thisskin << _skins[k].name << _skins[k].title << _skins[k].stylesheet;
    customskins.push_back( thisskin );
  }

  config->writeValue("CustomSkins", customskins);
}


// --------------------------------------------------------------------------------


void SkinPlugin::initialize(QApplication */*app*/, KLFMainWin *mainWin, KLFPluginConfigAccess *rwconfig)
{
  _mainwin = mainWin;

  _config = rwconfig;

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

  (void) new SkinConfigObject(w, _config);
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
