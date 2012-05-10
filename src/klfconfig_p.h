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
#include <QLineEdit>

#include <klfcolorchooser.h>
#include "klfconfig.h"



// EXPERIMENTAL FEATURE: use  cmake -DKLF_EXPERIMENTAL=on  to enable
#ifdef KLF_EXPERIMENTAL

#include <ui_klfadvancedconfigeditor.h>


#define CONFIG_VIEW_ROLE_PROPNAME	(Qt::UserRole)
#define CONFIG_VIEW_ROLE_TYPENAME	(Qt::UserRole+1)
#define CONFIG_VIEW_ROLE_INNERTYPENAME	(Qt::UserRole+2)


#define REGISTER_EDITOR(factory, type, editorclass)			\
  { QItemEditorCreatorBase *anEditor = new QStandardItemEditorCreator<editorclass>(); \
    factory->registerEditor(type, anEditor); }

/*
class KLFStringValueVariantEditor : public QLineEdit
{
  Q_OBJECT

  // QVariant is itself not a variant-able value...
  //Q_PROPERTY(QVariant value READ value WRITE setValue USER true) ;
  //Q_PROPERTY(QVariant::Type type READ type USER false) ;
  //Q_PROPERTY(QString typeName READ typeName USER false) ;

public:
  KLFStringValueVariantEditor(QWidget *parent = NULL) : QLineEdit(parent)
  {
    connect(this, SIGNAL(textChanged(const QString&)), this, SLOT(slotValueChanged(const QString&)));

    // this sets both the value and the type
    setValue(QVariant());
  };
  virtual ~KLFStringValueVariantEditor()
  {
  }

  QVariant value() const { return pValue; }
  QVariant::Type type() const { return pValue.type(); }
  QString typeName() const { return QString::fromAscii(pValue.typeName()); }

public slots:
  void setValue(const QVariant& value)
  {
    QByteArray data = klfSaveVariantToText(value);
    setText(data);
  }

private slots:
  void slotValueChanged(const QString& value)
  {
    KLF_DEBUG_BLOCK(KLF_FUNC_NAME) ;
    klfDbg("value="<<value) ;
    
  }

private:
  QVariant pValue;
};
*/

class KLFAdvancedConfigItemDelegate : public QStyledItemDelegate
{
  Q_OBJECT
public:
  KLFAdvancedConfigItemDelegate(QObject *parent) : QStyledItemDelegate(parent)
  {
  }

  virtual QWidget * createEditor(QWidget *parent, const QStyleOptionViewItem& option, const QModelIndex& index) const
  {
    KLF_DEBUG_BLOCK(KLF_FUNC_NAME) ;
    if (index.column() < 2)
      return QStyledItemDelegate::createEditor(parent, option, index);
    return new QLineEdit(parent);
  }

  virtual void setEditorData(QWidget *editor, const QModelIndex& index) const
  {
    KLF_DEBUG_BLOCK(KLF_FUNC_NAME) ;
    klfDbg("editor="<<editor<<", index="<<index) ;

    if (index.column() < 2)
      return QStyledItemDelegate::setEditorData(editor, index);
    // else:
    QLineEdit * e = qobject_cast<QLineEdit*>(editor);
    KLF_ASSERT_NOT_NULL(e, "Editor is NULL or not a QLineEdit!", return ; ) ;
    QVariant value = index.data(Qt::EditRole);
    klfDbg("value is "<<value) ;
    e->setText(klfSaveVariantToText(value));
  }

  virtual void setModelData(QWidget *editor, QAbstractItemModel *model, const QModelIndex& index) const
  {
    KLF_DEBUG_BLOCK(KLF_FUNC_NAME) ;
    klfDbg("editor="<<editor<<", model="<<model<<", index="<<index) ;

    if (index.column() < 2) {
      klfDbg("letting base class handle this.") ;
      return QStyledItemDelegate::setModelData(editor, model, index);
    }
    // else:
    QLineEdit * e = qobject_cast<QLineEdit*>(editor);
    KLF_ASSERT_NOT_NULL(e, "Editor is NULL or not a KLFStringValueVariantEditor!", return ; ) ;
    KLF_ASSERT_NOT_NULL(model, "Model is NULL!", return ; ) ;
    QByteArray datavalue = e->text().toAscii();
    QByteArray typname = model->data(index, CONFIG_VIEW_ROLE_TYPENAME).toByteArray();
    QByteArray innertypname = model->data(index, CONFIG_VIEW_ROLE_INNERTYPENAME).toByteArray();
    QVariant value = klfLoadVariantFromText(datavalue, typname, innertypname);
    klfDbg("value is "<<value) ;
    model->setData(index, value, Qt::EditRole);
  }
};


class KLFAdvancedConfigEditor : public QDialog
{
  Q_OBJECT
public:
  KLFAdvancedConfigEditor(QWidget *parent, KLFConfigBase *c)
    : QDialog(parent), pConfigBase(c)
  {
    u = new Ui::KLFAdvancedConfigEditor;
    u->setupUi(this);

    pCurrentInternalUpdate = false;

    QItemEditorFactory *factory = new QItemEditorFactory;
    REGISTER_EDITOR(factory, QVariant::Color, KLFColorChooser);

    pConfModel = new QStandardItemModel(this);
    pConfModel->setColumnCount(3);
    pConfModel->setHorizontalHeaderLabels(QStringList() << tr("Config Entry")
					  << tr("Current Value") << tr("Encoded Value Entry"));
    u->configView->setModel(pConfModel);
    KLFAdvancedConfigItemDelegate *delegate = new KLFAdvancedConfigItemDelegate(this);
    delegate->setItemEditorFactory(factory);
    u->configView->setItemDelegate(delegate);
    u->configView->setColumnWidth(0, 200);
    u->configView->setColumnWidth(1, 200);
    u->configView->setColumnWidth(2, 200);

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

signals:
  void configModified(const QString& property);

public slots:
  void updateConfig()
  {
    updateConfigView();
  }

private slots:
  void configEntryEdited(QStandardItem *item)
  {
    KLF_DEBUG_BLOCK(KLF_FUNC_NAME);
    klfDbg( "item="<<item<<"" ) ;
    if (pCurrentInternalUpdate)
      return;

    klfDbg("config entry edited...");

    QVariant value = item->data(Qt::EditRole);
    QString pname = item->data(CONFIG_VIEW_ROLE_PROPNAME).toString();
    KLFConfigPropBase *p = pConfigBase->property(pname);
    QVariant oldvalue = p->toVariant();

    QMessageBox msgBox;
    msgBox.setText(tr("You are changing an advanced config setting. Are you sure?"));
    msgBox.setIcon(QMessageBox::Information);
    msgBox.setInformativeText(tr("Change config entry %1 from %2 to %3?")
			      .arg( "<b>"+pname+"</b>" ,
				    "<b>"+klfSaveVariantToText(oldvalue)+"</b> <i>("+oldvalue.typeName()+")</i>" ,
				    "<b>"+klfSaveVariantToText(value)+"</b> <i>("+value.typeName()+")</i>"));
    msgBox.setStandardButtons(QMessageBox::Save | QMessageBox::Cancel);
    msgBox.setDefaultButton(QMessageBox::Save);
    int r = msgBox.exec();
    if (r != QMessageBox::Save) {
      updateConfigEntry(item->row());
      return;
    }

    r = p->setValue(value);
    if ( ! r ) {
      QMessageBox::critical(this, tr("Error"),
			    tr("Failed to set config entry `%1;.").arg(pname));
    }
    updateConfigEntry(item->row());
  }

private:

  KLFConfigBase * pConfigBase;

  Ui::KLFAdvancedConfigEditor * u;

#define EDITTYPE_TEST_FOR(t, x)  if ((t) == x) { return true; }
  static bool is_editable_type(int t)
  {
    EDITTYPE_TEST_FOR(t, QVariant::Bool);
    EDITTYPE_TEST_FOR(t, QVariant::Double);
    EDITTYPE_TEST_FOR(t, QVariant::Int);
    EDITTYPE_TEST_FOR(t, QVariant::UInt);
    EDITTYPE_TEST_FOR(t, QVariant::Date);
    EDITTYPE_TEST_FOR(t, QVariant::Time);
    EDITTYPE_TEST_FOR(t, QVariant::DateTime);
    EDITTYPE_TEST_FOR(t, QVariant::String);
    EDITTYPE_TEST_FOR(t, QVariant::Color);
    return false;
  }

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
      bool varianteditable = editable && is_editable_type(val.userType());
      i2->setEditable(varianteditable);
      QPalette::ColorGroup cg = varianteditable ? QPalette::Active : QPalette::Disabled;
      i2->setForeground(pal.brush(cg, QPalette::Text));
      i2->setBackground(pal.brush(cg, QPalette::Base));
      i2->setData(val, Qt::EditRole);
      i2->setData(pname, CONFIG_VIEW_ROLE_PROPNAME); // user data is property name
      QByteArray savedtype, savedinnertype;
      QByteArray datavalue = klfSaveVariantToText(val, false, &savedtype, &savedinnertype);
      klfDbg("i3: datavalue="<<datavalue) ;
      QStandardItem *i3 = new QStandardItem(QString::fromAscii(datavalue));
      cg = editable ? QPalette::Active : QPalette::Disabled;
      i3->setForeground(pal.brush(cg, QPalette::Text));
      i3->setBackground(pal.brush(cg, QPalette::Base));
      i3->setData(val, Qt::EditRole);
      i3->setData(QString::fromAscii(datavalue), Qt::DisplayRole);
      i3->setData(pname, CONFIG_VIEW_ROLE_PROPNAME);
      i3->setData(savedtype, CONFIG_VIEW_ROLE_TYPENAME);
      i3->setData(savedinnertype, CONFIG_VIEW_ROLE_INNERTYPENAME);
      pConfModel->appendRow(QList<QStandardItem*>() << i1 << i2 << i3);
    }
  }
  void updateConfigEntry(int row)
  {
    pCurrentInternalUpdate = true;
    QStandardItem *i2 = pConfModel->item(row, 1);
    QStandardItem *i3 = pConfModel->item(row, 2);
    QString pname = i2->data(CONFIG_VIEW_ROLE_PROPNAME).toString();
    KLF_ASSERT_CONDITION(pname == i3->data(CONFIG_VIEW_ROLE_PROPNAME).toString(),
			 "BUG?! pnames don't match for both config items",
			 return ; );
    QVariant val = pConfigBase->property(pname)->toVariant();
    i2->setText(val.toString());
    i2->setData(val, Qt::EditRole);
    i3->setData(val, Qt::EditRole);
    i3->setData(klfSaveVariantToText(val), Qt::DisplayRole);
    pCurrentInternalUpdate = false;
  }

  QStandardItemModel *pConfModel;

  bool pCurrentInternalUpdate;
};

#endif // KLF_EXPERIMENTAL



#endif
