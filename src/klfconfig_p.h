/***************************************************************************
 *   file klfconfig_p.h
 *   This file is part of the KLatexFormula Project.
 *   Copyright (C) 2011 by Philippe Faist
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

/** \file
 * This header contains (in principle private) auxiliary classes for
 * library routines defined in ****.cpp */

#ifndef KLFCONFIG_P_H
#define KLFCONFIG_P_H

#include <QDialog>
#include <QStandardItemModel>
#include <QStyledItemDelegate>
#include <QItemEditorFactory>
#include <QStandardItemEditorCreator>
#include <QMessageBox>

#include <klfcolorchooser.h>
#include "klfconfig.h"

#include <ui_klfadvancedconfigeditor.h>


#define REGISTER_EDITOR(factory, type, editorclass)			\
  { QItemEditorCreatorBase *anEditor = new QStandardItemEditorCreator<editorclass>(); \
    factory->registerEditor(type, anEditor); }



class KLFAdvancedConfigEditor : public QDialog
{
  Q_OBJECT
public:
  KLFAdvancedConfigEditor(QWidget *parent, KLFConfigBase *c)
    : QDialog(parent), pConfigBase(c)
  {
    u = new Ui::KLFAdvancedConfigEditor;
    u->setupUi(this);

    QItemEditorFactory *factory = new QItemEditorFactory;
    REGISTER_EDITOR(factory, QVariant::Color, KLFColorChooseWidget);

    pConfModel = new QStandardItemModel(this);
    pConfModel->setColumnCount(2);
    pConfModel->setHorizontalHeaderLabels(QStringList() << tr("Config Entry") << tr("Current Value"));
    u->configView->setModel(pConfModel);
    QStyledItemDelegate *delegate = new QStyledItemDelegate(this);
    delegate->setItemEditorFactory(factory);
    u->configView->setItemDelegate(delegate);
    u->configView->setColumnWidth(0, 200);
    u->configView->setColumnWidth(1, 200);

    connect(pConfModel, SIGNAL(itemChanged(QStandardItem *)),
	    this, SLOT(configEntryEdited(QStandardItem *)));
  }
  virtual ~KLFAdvancedConfigEditor()
  {
    delete u;
  }

  virtual void setVisible(bool visible)
  {
    if (visible) {
      updateConfigView();
    } else {
      //      unloadConfigView();
    }
    QDialog::setVisible(visible);
  }

private slots:
  void configEntryEdited(QStandardItem *item)
  {
    klfDbg( "item="<<item<<"" ) ;
    QVariant value = item->data(Qt::EditRole);
    QString pname = item->data(Qt::UserRole).toString();
    KLFConfigPropBase *p = pConfigBase->property(pname);
    bool r = p->setValue(value);
    if ( ! r ) {
      QMessageBox::critical(this, tr("Error"),
			    tr("Failed to set config entry `%1;.").arg(pname));
    }
  }

private:

  KLFConfigBase * pConfigBase;

  Ui::KLFAdvancedConfigEditor * u;

  void updateConfigView()
  {
    pConfModel->setRowCount(0);
    int k;
    QStringList props = pConfigBase->propertyList();
    QPalette pal = u->configView->palette();
    for (k = 0; k < props.size(); ++k) {
      QString pname = props[k];
      KLFConfigPropBase *p = pConfigBase->property(pname);
      QVariant val = p->toVariant();
      QStandardItem *i1 = new QStandardItem(pname);
      i1->setEditable(false);
      QStandardItem *i2 = new QStandardItem(val.toString());
      bool editable = pConfigBase->okChangeProperty(p, QVariant(), QVariant());
      i2->setEditable(editable);
      QPalette::ColorGroup cg = editable ? QPalette::Active : QPalette::Disabled;
      i2->setForeground(pal.brush(cg, QPalette::Text));
      i2->setBackground(pal.brush(cg, QPalette::Base));
      i2->setData(val, Qt::EditRole);
      i2->setData(pname, Qt::UserRole); // user data is property name
      pConfModel->appendRow(QList<QStandardItem*>() << i1 << i2);
    }
  }


  QStandardItemModel *pConfModel;
};





#endif
