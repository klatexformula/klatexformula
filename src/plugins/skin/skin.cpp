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
#include <QtUiTools/QtUiTools>

#include <klfmainwin.h>
#include <klflibrary.h>
#include <klflatexsymbols.h>
#include <klfconfig.h>

#include "skin.h"



SkinConfigWidget::SkinConfigWidget(QWidget *parent, KLFPluginConfigAccess *conf)
  : QWidget(parent), config(conf)
{
  setupUi(this);

  connect(cbxSkin, SIGNAL(activated(int)), this, SLOT(skinSelected(int)));
  connect(txtStyleSheet, SIGNAL(textChanged()), this, SLOT(stylesheetChanged()));
  connect(btnSaveCustom, SIGNAL(clicked()), this, SLOT(saveCustom()));
  connect(btnDeleteSkin, SIGNAL(clicked()), this, SLOT(deleteCustom()));
}

void SkinConfigWidget::load(QString skin, QString stylesheet)
{
  cbxSkin->clear();
  _skins.clear();

  QStringList stylesheetsdirs;
  stylesheetsdirs << ":/plugindata/skin/stylesheets/" << config->homeConfigDir() + "/plugins/skin/stylesheets";

  int j;
  int k;
  int indf = -1;
  for (j = 0; j < stylesheetsdirs.size(); ++j) {
    QDir stylesheetdir(stylesheetsdirs[j]);
    QStringList skinlist = stylesheetdir.entryList(QStringList() << "*.qss", QDir::Files);
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
  }
  QList<QVariant> customskins
    = config->readValue("CustomSkins").value<QList<QVariant> >() ;
  for (k = 0; k < customskins.size(); ++k) {
    //    printf("%d...\n", k);
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

void SkinConfigWidget::skinSelected(int index)
{
  if (index < 0 || index >= cbxSkin->count()-1)
    return;

  int k = cbxSkin->itemData(index).toInt();

  txtStyleSheet->blockSignals(true);
  txtStyleSheet->setPlainText(_skins[k].stylesheet);
  txtStyleSheet->blockSignals(false);

  btnDeleteSkin->setEnabled( ! _skins[k].builtin );
}

void SkinConfigWidget::stylesheetChanged()
{
  cbxSkin->setCurrentIndex(cbxSkin->count()-1);
}

void SkinConfigWidget::saveCustom()
{
  bool ok;
  QString title = QInputDialog::getText(this, tr("Skin Name"), tr("Please enter skin name:"),
				       QLineEdit::Normal, tr("[New Skin Name]"), &ok);
  if (!ok)
    return;

  Skin newskin(false, title, title, txtStyleSheet->toPlainText());

  int k;
  for (k = 0; k < _skins.size(); ++k) {
    if (_skins[k].title == title) {
      if (_skins[k].builtin) {
	QMessageBox::critical(this, tr("Error"), tr("Can't overwrite a built-in skin. Please choose another name."));
	// recurse to re-ask for a new title
	saveCustom();
	return;
      } else {
	int confirmation = 
	  QMessageBox::question(this, tr("Overwrite skin?"), tr("You are about to overwrite skin %1. Are you sure?")
				.arg(title), QMessageBox::Yes|QMessageBox::No|QMessageBox::Cancel, QMessageBox::Cancel);
	if ( confirmation == QMessageBox::Cancel )
	  return;
	if ( confirmation == QMessageBox::Yes ) {
	  // overwrite the other skin
	  _skins[k] = newskin;
	  int index = cbxSkin->findData(k);
	  if ( index >= 0 )
	    cbxSkin->setCurrentIndex(index);
	  return;
	} else {
	  // not sure
	  // ask for new name: recurse current function
	  saveCustom();
	  return;
	}
      }
    }
  }
  // skin not already existing

  _skins.push_back(newskin);
  cbxSkin->insertItem(cbxSkin->count()-1, title, _skins.size()-1);
  cbxSkin->setCurrentIndex(cbxSkin->count()-2);

  saveCustomSkins();
}

void SkinConfigWidget::deleteCustom()
{
  int index = cbxSkin->currentIndex();
  int k = cbxSkin->itemData(index).toInt();
  int confirmation
    = QMessageBox::warning(this, tr("Delete skin?", "[[confirmation messagebox title]]"),
			   tr("Are you sure you want to delete the skin named `%1' ?").arg(_skins[k].name),
			   QMessageBox::Yes|QMessageBox::Cancel, QMessageBox::Cancel);
  if (confirmation != QMessageBox::Yes) {
    return;
  }

  _skins.removeAt(k);

  cbxSkin->removeItem(index);
  cbxSkin->setCurrentIndex(cbxSkin->count()-1);

  saveCustomSkins();
}

void SkinConfigWidget::saveCustomSkins()
{
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
  o->load(config->readValue("skin").toString(),
	  config->readValue("stylesheet").toString());
}
void SkinPlugin::saveToConfig(QWidget *confwidget, KLFPluginConfigAccess *config)
{
  if (confwidget->property("SkinConfigWidget").toBool() != true) {
    fprintf(stderr, "Error: bad config widget given !!\n");
    return;
  }

  SkinConfigWidget * o = qobject_cast<SkinConfigWidget*>(confwidget);

  QString skin = o->currentSkin();
  QString stylesheet = o->currentStyleSheet();

  config->writeValue("skin", QVariant(skin));
  config->writeValue("stylesheet", QVariant(stylesheet));

  applySkin(config);
}





// Export Plugin
Q_EXPORT_PLUGIN2(skin, SkinPlugin);
