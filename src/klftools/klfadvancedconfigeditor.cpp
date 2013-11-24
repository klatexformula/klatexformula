/***************************************************************************
 *   file klfadvancedconfigeditor.cpp
 *   This file is part of the KLatexFormula Project.
 *   Copyright (C) 2012 by Philippe Faist
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

#include <QStandardItemModel>
#include <QStyledItemDelegate>
#include <QItemEditorFactory>
#include <QStandardItemEditorCreator>
#include <QMessageBox>
#include <QLineEdit>

#include <klfitemviewsearchtarget.h>
#include "klfadvancedconfigeditor.h"
#include "klfadvancedconfigeditor_p.h"



// --------------


#define REGISTER_EDITOR(factory, type, editorclass)			\
  { QItemEditorCreatorBase *anEditor = new QStandardItemEditorCreator<editorclass>(); \
    factory->registerEditor(type, anEditor); }



KLFAdvancedConfigEditor::KLFAdvancedConfigEditor(QWidget *parent, KLFConfigBase *c)
  : QDialog(parent)
{
  KLF_INIT_PRIVATE(KLFAdvancedConfigEditor) ;

  d->pConfigBase = c;

  u = new Ui::KLFAdvancedConfigEditor;
  u->setupUi(this);
  
  QItemEditorFactory *factory = new QItemEditorFactory;

  REGISTER_EDITOR(factory, QVariant::Color, KLFColorDialog);
  REGISTER_EDITOR(factory, QVariant::Font, KLFFontDialog);
  
  d->pConfModel = new QStandardItemModel(this);
  d->pConfModel->setColumnCount(3);
  d->pConfModel->setHorizontalHeaderLabels(QStringList() << tr("Config Entry")
					   << tr("Current Value") << tr("Encoded Value Entry"));
  u->configView->setModel(d->pConfModel);
  KLFAdvancedConfigItemDelegate *delegate = new KLFAdvancedConfigItemDelegate(this);
  delegate->setItemEditorFactory(factory);
  u->configView->setItemDelegate(delegate);
  u->configView->setColumnWidth(0, 200);
  u->configView->setColumnWidth(1, 200);
  u->configView->setColumnWidth(2, 200);
  
  KLFItemViewSearchTarget *searchtarget = new KLFItemViewSearchTarget(u->configView, this);
  u->searchBar->setSearchTarget(searchtarget);
  u->searchBar->registerShortcuts(this);

  connect(d->pConfModel, SIGNAL(itemChanged(QStandardItem *)),
	  d, SLOT(configEntryEdited(QStandardItem *)));

  // add "reset default value" action
  QAction *resetDefault = new QAction(tr("Reset Default Value"), this);
  connect(resetDefault, SIGNAL(triggered()),
          d, SLOT(resetDefault()));
  u->configView->addAction(resetDefault);
  u->configView->setContextMenuPolicy(Qt::ActionsContextMenu);
}

KLFAdvancedConfigEditor::~KLFAdvancedConfigEditor()
{
  KLF_DELETE_PRIVATE ;

  delete u;
}

void KLFAdvancedConfigEditor::setVisible(bool visible)
{
  if (visible) {
    d->updateConfigView();
  } else {
    //      unloadConfigView();
  }
  QDialog::setVisible(visible);
}


void KLFAdvancedConfigEditor::updateConfig()
{
  d->_are_resetting_config = true;
  d->updateConfigView();
  d->_are_resetting_config = false;
}



