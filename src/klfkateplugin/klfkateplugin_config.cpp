/**
  * Copyright (C) 2010 Philippe Faist  philippe dot faist at bluewin dot ch
  *
  * This library is free software; you can redistribute it and/or
  * modify it under the terms of the GNU Library General Public
  * License version 2 as published by the Free Software Foundation.
  *
  * This library is distributed in the hope that it will be useful,
  * but WITHOUT ANY WARRANTY; without even the implied warranty of
  * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  * Library General Public License for more details.
  *
  * You should have received a copy of the GNU Library General Public License
  * along with this library; see the file COPYING.LIB.  If not, write to
  * the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
  * Boston, MA 02110-1301, USA.
  */

#include "klfkateplugin_config.h"
#include "klfkateplugin.h"

#include <QtGui/QLabel>
#include <QtGui/QBoxLayout>

#include <klocale.h>
#include <kpluginfactory.h>
#include <kpluginloader.h>
#include <klineedit.h>
#include <kconfiggroup.h>
#include <kstandarddirs.h>

extern const char * klfkteDefaultPreamble;


KLFKteConfig::KLFKteConfig(QWidget *parent, const QVariantList &args)
    : KCModule(KLFKtePluginFactory::componentData(), parent, args)
{
  u = new Ui::KLFKatePluginConfigWidget;
  u->setupUi(this);

  connect(u->chkAutoPopup, SIGNAL(stateChanged(int)), this, SLOT(slotChanged()));
  connect(u->chkOnlyLatexMode, SIGNAL(stateChanged(int)), this, SLOT(slotChanged()));
  connect(u->txtPreamble, SIGNAL(textChanged()), this, SLOT(slotChanged()));
  connect(u->pathKLF, SIGNAL(textChanged(const QString&)), this, SLOT(slotChanged()));
  connect(u->pathKLF, SIGNAL(urlSelected(const KUrl&)), this, SLOT(slotChanged()));

  load();
}

KLFKteConfig::~KLFKteConfig()
{
}

void KLFKteConfig::save()
{
  if (KLFKtePlugin::self()) {
    KLFKtePlugin::self()->autopopup = u->chkAutoPopup->isChecked();
    KLFKtePlugin::self()->onlyLatexMode = u->chkOnlyLatexMode->isChecked();
    KLFKtePlugin::self()->preamble = u->txtPreamble->toPlainText();
    KLFKtePlugin::self()->klfpath = u->pathKLF->text();
    KLFKtePlugin::self()->writeConfig();
  } else {
    KConfigGroup cg(KGlobal::config(), "KLatexFormula Plugin");
    cg.writeEntry("autopopup", u->chkAutoPopup->isChecked());
    cg.writeEntry("onlylatexmode", u->chkOnlyLatexMode->isChecked());
    cg.writeEntry("preamble", u->txtPreamble->toPlainText());
    cg.writeEntry("klfpath", u->pathKLF->text());
  }

  emit changed(false);
}

void KLFKteConfig::load()
{
  if (KLFKtePlugin::self()) {
    KLFKtePlugin::self()->readConfig();
    u->chkAutoPopup->setChecked(KLFKtePlugin::self()->autopopup);
    u->chkOnlyLatexMode->setChecked(KLFKtePlugin::self()->onlyLatexMode);
    u->txtPreamble->setPlainText(KLFKtePlugin::self()->preamble);
    u->pathKLF->setText(KLFKtePlugin::self()->klfpath);
  } else {
    KConfigGroup cg(KGlobal::config(), "KLatexFormula Plugin" );
    u->chkAutoPopup->setChecked(cg.readEntry("autopopup", true));
    u->chkOnlyLatexMode->setChecked(cg.readEntry("onlylatexmode", true));
    u->txtPreamble->setPlainText(cg.readEntry("preamble", klfkteDefaultPreamble));
    u->pathKLF->setText(cg.readEntry("klfpath", KStandardDirs::findExe("klatexformula")));
  }
  emit changed(false);
}

void KLFKteConfig::defaults()
{
  u->chkAutoPopup->setChecked(true);
  u->chkOnlyLatexMode->setChecked(true);
  u->txtPreamble->setPlainText(klfkteDefaultPreamble);
  u->pathKLF->setText(KStandardDirs::findExe("klatexformula"));

  slotChanged();
}

void KLFKteConfig::slotChanged()
{
  emit changed(true);
}


#include "klfkateplugin_config.moc"

