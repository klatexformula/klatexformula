/***************************************************************************
 *   file klfkateplugin_config.cpp
 *   This file is part of the KLatexFormula Project.
 *   Copyright (C) 2010 by Philippe Faist
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

#include <QLabel>
#include <QBoxLayout>

#include <klocale.h>
#include <kpluginfactory.h>
#include <kpluginloader.h>
#include <klineedit.h>
#include <kconfiggroup.h>
#include <kstandarddirs.h>

#include <ui_klfkatepluginconfigwidget.h>

#include "klfkateplugin_config.h"
#include "klfkateplugin.h"


KLFKteConfigData *KLFKteConfigData::instance = NULL;


const char *klfkteDefaultPreamble =
  "\\usepackage{amsmath}\n"
  "\\usepackage{amssymb}\n"
  "\\usepackage{amsfonts}\n";

// static
KLFKteConfigData * KLFKteConfigData::inst()
{
  if (instance == NULL) {
    // create object
    instance = new KLFKteConfigData(qApp);
  }
  return instance;
}

void KLFKteConfigData::readConfig(KConfigGroup *cg)
{
  autopopup = cg->readEntry("autopopup", true);
  onlyLatexMode = cg->readEntry("onlylatexmode", true);
  transparencyPercent = cg->readEntry("transparencyPercent", 20);
  preamble = cg->readEntry("preamble", klfkteDefaultPreamble);
  klfpath = cg->readEntry("klfpath", KStandardDirs::findExe("klatexformula"));
  popupMaxSize = cg->readEntry("popupMaxSize", QSize(600, 200));
  popupLinks = cg->readEntry("popupLinks", true);
}

void KLFKteConfigData::writeConfig(KConfigGroup *cg)
{
  cg->writeEntry("autopopup", autopopup);
  cg->writeEntry("onlylatexmode", onlyLatexMode);
  cg->writeEntry("transparencyPercent", transparencyPercent);
  cg->writeEntry("preamble", preamble);
  cg->writeEntry("klfpath", klfpath);
  cg->writeEntry("popupMaxSize", popupMaxSize);
  cg->writeEntry("popupLinks", popupLinks);
}



// --------------

static QList<QSize> maxsizesteps;


KLFKteConfig::KLFKteConfig(QWidget *parent, const QVariantList &args)
    : KCModule(KLFKtePluginFactory::componentData(), parent, args)
{
  u = new Ui::KLFKatePluginConfigWidget;
  u->setupUi(this);
  
  if (maxsizesteps.isEmpty()) {
    maxsizesteps << QSize(200, 75) << QSize(280, 90) << QSize(400, 150) << QSize(500,200)
		 << QSize(600, 250) << QSize(800, 350) << QSize(1000,400) << QSize(1200,600);
  }

  // note: KLF_VERSION_STRING can be used only in KLF source. external code must
  // call klfVersion() instead.
  u->lblTitle->setText(u->lblTitle->text().arg(KLF_VERSION_STRING));
  
  u->sldMaxSize->setMinimum(0);
  u->sldMaxSize->setMaximum(maxsizesteps.size()-1);

  connect(u->chkAutoPopup, SIGNAL(stateChanged(int)), this, SLOT(slotChanged()));
  connect(u->chkOnlyLatexMode, SIGNAL(stateChanged(int)), this, SLOT(slotChanged()));
  connect(u->spnTransparency, SIGNAL(valueChanged(int)), this, SLOT(slotChanged()));
  connect(u->txtPreamble, SIGNAL(textChanged()), this, SLOT(slotChanged()));
  connect(u->pathKLF, SIGNAL(textChanged(const QString&)), this, SLOT(slotChanged()));
  connect(u->pathKLF, SIGNAL(urlSelected(const KUrl&)), this, SLOT(slotChanged()));
  connect(u->sldMaxSize, SIGNAL(valueChanged(int)), this, SLOT(slotChanged()));
  connect(u->sldMaxSize, SIGNAL(valueChanged(int)), this, SLOT(slotMaxSize(int)));
  connect(u->chkPopupLinks, SIGNAL(stateChanged(int)), this, SLOT(slotChanged()));

  load();
}

KLFKteConfig::~KLFKteConfig()
{
}

void KLFKteConfig::save()
{
  KLFKteConfigData * d = KLFKteConfigData::inst();
  d->autopopup = u->chkAutoPopup->isChecked();
  d->onlyLatexMode = u->chkOnlyLatexMode->isChecked();
  d->transparencyPercent = u->spnTransparency->value();
  d->preamble = u->txtPreamble->toPlainText();
  d->klfpath = u->pathKLF->text();
  d->popupMaxSize = maxsizesteps[u->sldMaxSize->value()];
  d->popupLinks = u->chkPopupLinks->isChecked();

  KConfigGroup cg(KGlobal::config(), "KLatexFormula Plugin");
  d->writeConfig(&cg);

  emit changed(false);
}

void KLFKteConfig::load()
{
  KLFKteConfigData * d = KLFKteConfigData::inst();
  
  KConfigGroup cg(KGlobal::config(), "KLatexFormula Plugin");
  d->readConfig(&cg);
  
  u->chkAutoPopup->setChecked(d->autopopup);
  u->chkOnlyLatexMode->setChecked(d->onlyLatexMode);
  u->spnTransparency->setValue(d->transparencyPercent);
  u->txtPreamble->setPlainText(d->preamble);
  u->pathKLF->setText(d->klfpath);
  int k = 0;
  while (k < maxsizesteps.size() && maxsizesteps[k].width() < d->popupMaxSize.width())
    ++k;
  u->sldMaxSize->setValue(k);
  u->chkPopupLinks->setChecked(d->popupLinks);

  emit changed(false);
}

void KLFKteConfig::defaults()
{
  u->chkAutoPopup->setChecked(true);
  u->chkOnlyLatexMode->setChecked(true);
  u->spnTransparency->setValue(20);
  u->txtPreamble->setPlainText(klfkteDefaultPreamble);
  u->pathKLF->setText(KStandardDirs::findExe("klatexformula"));
  int k = 0;
  while (k < maxsizesteps.size() && maxsizesteps[k].width() < 600)
    ++k;
  u->sldMaxSize->setValue(k);
  u->chkPopupLinks->setChecked(true);

  slotChanged();
}

void KLFKteConfig::slotChanged()
{
  emit changed(true);
}

void KLFKteConfig::slotMaxSize(int step)
{
  u->lblMaxSize->setText(QString("%1x%2").arg(maxsizesteps[step].width()).arg(maxsizesteps[step].height()));
}



#include "klfkateplugin_config.moc"
