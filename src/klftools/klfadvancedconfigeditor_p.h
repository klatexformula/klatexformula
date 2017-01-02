/***************************************************************************
 *   file klfadvancedconfigeditor_p.h
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

/** \file
 * This header contains (in principle private) auxiliary classes for
 * library routines defined in klfadvancedconfigeditor.cpp */

#ifndef KLFADVANCEDCONFIGEDITOR_P_H
#define KLFADVANCEDCONFIGEDITOR_P_H

#include <QAbstractItemModel>
#include <QStyledItemDelegate>
#include <QLineEdit>
#include <QStandardItemModel>
#include <QMessageBox>
#include <QFontDialog>


#include <klfdefs.h>
#include <klfdatautil.h>

#include "klfadvancedconfigeditor.h"
#include <ui_klfadvancedconfigeditor.h>


#define CONFIG_VIEW_ROLE_PROPNAME	(Qt::UserRole)
#define CONFIG_VIEW_ROLE_TYPENAME	(Qt::UserRole+1)
#define CONFIG_VIEW_ROLE_INNERTYPENAME	(Qt::UserRole+2)


// ---------------------------------------------

class KLFAdvancedConfigItemDelegate : public QStyledItemDelegate
{
  Q_OBJECT
public:
  KLFAdvancedConfigItemDelegate(QObject *parent) : QStyledItemDelegate(parent)
  {
  }

  virtual QWidget * createEditor(QWidget *parent, const QStyleOptionViewItem& option,
                                 const QModelIndex& index) const
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
    KLF_ASSERT_NOT_NULL(e, "Editor is NULL or not a QLineEdit!", return ; ) ;
    KLF_ASSERT_NOT_NULL(model, "Model is NULL!", return ; ) ;
    QByteArray datavalue = e->text().toLatin1();
    QByteArray typname = model->data(index, CONFIG_VIEW_ROLE_TYPENAME).toByteArray();
    QByteArray innertypname = model->data(index, CONFIG_VIEW_ROLE_INNERTYPENAME).toByteArray();
    QVariant value = klfLoadVariantFromText(datavalue, typname, innertypname);
    klfDbg("value is "<<value) ;
    model->setData(index, value, Qt::EditRole);
  }

protected:

  virtual bool editorEvent(QEvent *event, QAbstractItemModel *model, const QStyleOptionViewItem& option,
			   const QModelIndex& index)
  {
    // Note: editor closes and doesn't apply new value for colors, which open a separate popup
    // widget... (MAC OS X ONLY)   SOLUTION: use an actual dialog as the widget, this is handled nicely.
    return QStyledItemDelegate::editorEvent(event, model, option, index);
  }
};


// ---------------------------------------------------------------


struct KLFAdvancedConfigEditorPrivate : public QObject
{
  Q_OBJECT
public:
  KLF_PRIVATE_QOBJ_HEAD(KLFAdvancedConfigEditor, QObject)
  {
    pConfigBase = NULL;
    pCurrentInternalUpdate = false;
    _are_resetting_config = false;
  }

  KLFConfigBase * pConfigBase;

  bool _are_resetting_config;

  QStandardItemModel *pConfModel;

  bool pCurrentInternalUpdate;

public slots: // public is just for us... the class is still private :)

  void configEntryEdited(QStandardItem *item)
  {
    KLF_DEBUG_BLOCK(KLF_FUNC_NAME);
    klfDbg( "item="<<item<<"" ) ;
    KLF_ASSERT_NOT_NULL(item, "item is NULL!", return; ) ;

    if (pCurrentInternalUpdate)
      return;

    if (item->column() < 1 || item->column() > 2)
      return; // false edit

    klfDbg("config entry edited...");

    QVariant value = item->data(Qt::EditRole);
    QString pname = item->data(CONFIG_VIEW_ROLE_PROPNAME).toString();
    KLFConfigPropBase *p = pConfigBase->property(pname);
    KLF_ASSERT_NOT_NULL(p, "Property is NULL!", return; ) ;
    QVariant oldvalue = p->toVariant();

    if (value == oldvalue) {
      // never mind, the user didn't change anything.
      // but still update the row, because otherwise the wrong string representation is shown (why?)
      updateConfigEntry(item->row());
      return;
    }

    QMessageBox msgBox;
    msgBox.setText(tr("You are changing an advanced config setting. Please confirm your action."));
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
      QMessageBox::critical(K, tr("Error"),
			    tr("Failed to set config entry `%1'.").arg(pname));
    }
    updateConfigEntry(item->row());
    if (!_are_resetting_config)
      emit K->configModified(pname);
  }

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
    EDITTYPE_TEST_FOR(t, QVariant::Font);
    return false;
  }

  void updateConfigView()
  {
    pConfModel->setRowCount(0);
    int k;
    QStringList props = pConfigBase->propertyList();
    QPalette pal = K->u->configView->palette();
    for (k = 0; k < props.size(); ++k) {
      QString pname = props[k];
      KLFConfigPropBase *p = pConfigBase->property(pname);
      QVariant val = p->toVariant();
      // Config Entry
      QStandardItem *i1 = new QStandardItem(pname);
      i1->setEditable(false);
      // Config Value (edit type directly)
      QStandardItem *i2 = new QStandardItem(val.toString());
      bool editable = pConfigBase->okChangeProperty(p, QVariant(), QVariant());
      bool varianteditable = editable && is_editable_type(val.userType());
      i2->setEditable(varianteditable);
      QPalette::ColorGroup cg = varianteditable ? QPalette::Active : QPalette::Disabled;
      i2->setForeground(pal.brush(cg, QPalette::Text));
      i2->setBackground(pal.brush(cg, QPalette::Base));
      //      klfDbg("Adding value: val="<<val<<"; displaystring="<<displayString(val)) ;
      i2->setData(val, Qt::EditRole);
      i2->setData(displayString(val), Qt::DisplayRole);
      i2->setData(pname, CONFIG_VIEW_ROLE_PROPNAME); // user data is property name
      // Config Value (config text representation)
      QByteArray savedtype, savedinnertype;
      QByteArray datavalue = klfSaveVariantToText(val, false, &savedtype, &savedinnertype);
      klfDbg("i3: datavalue="<<datavalue) ;
      QStandardItem *i3 = new QStandardItem(QString::fromLatin1(datavalue));
      cg = editable ? QPalette::Active : QPalette::Disabled;
      i3->setForeground(pal.brush(cg, QPalette::Text));
      i3->setBackground(pal.brush(cg, QPalette::Base));
      i3->setData(val, Qt::EditRole);
      i3->setData(QString::fromLatin1(datavalue), Qt::DisplayRole);
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
    i2->setData(displayString(val), Qt::DisplayRole);
    i3->setData(val, Qt::EditRole);
    i3->setData(klfSaveVariantToText(val), Qt::DisplayRole);
    pCurrentInternalUpdate = false;
  }

  void resetDefault()
  {
    QModelIndex index = K->u->configView->currentIndex();
    if (index == QModelIndex())
      return;
    int row = index.row();
    QStandardItem *i2 = pConfModel->item(row, 1);
    QStandardItem *i3 = pConfModel->item(row, 2);
    QString pname = i2->data(CONFIG_VIEW_ROLE_PROPNAME).toString();
    KLF_ASSERT_CONDITION(pname == i3->data(CONFIG_VIEW_ROLE_PROPNAME).toString(),
			 "BUG?! pnames don't match for both config items",
			 return ; );
    KLFConfigPropBase *p = pConfigBase->property(pname);
    KLF_ASSERT_NOT_NULL(p, "Property is NULL!", return; ) ;
    QVariant oldvalue = p->toVariant();
    QVariant defval = p->defaultValueVariant();

    QMessageBox msgBox;
    msgBox.setText(tr("You are resetting an advanced config setting to its factory default value. "
                      "Please confirm your action."));
    msgBox.setIcon(QMessageBox::Information);
    msgBox.setInformativeText(tr("Change config entry %1 from %2 to its factory default value %3?")
			      .arg( "<b>"+pname+"</b>" ,
				    "<b>"+klfSaveVariantToText(oldvalue)+"</b> <i>("+oldvalue.typeName()+")</i>" ,
				    "<b>"+klfSaveVariantToText(defval)+"</b> <i>("+defval.typeName()+")</i>"));
    msgBox.setStandardButtons(QMessageBox::Save | QMessageBox::Cancel);
    msgBox.setDefaultButton(QMessageBox::Save);
    int r = msgBox.exec();
    if (r != QMessageBox::Save) {
      updateConfigEntry(row);
      return;
    }

    // save the default value
    r = p->setValue(defval);
    if ( ! r ) {
      QMessageBox::critical(K, tr("Error"),
			    tr("Failed to set config entry `%1'.").arg(pname));
    }
    updateConfigEntry(row);
  }

private:
  //   QString displayString(QVariant val) {
  //     if (val.type() == QVariant::Color) {
  //       QColor c = val.value<QColor>();
  //       if (c.alpha() == 255)
  //         return c.name();
  //       return c.name()+" ("+QString::number(c.alpha()*100/255)+"%)";
  //     }
  //     return val.toString();
  //   }

  // If a string DisplayRole is set, then the list pops up the wrong editor. (Bug in Qt?)
  inline QVariant displayString(QVariant v) {
    return v;
  }
};




// ------------------------


class KLFFontDialog : public QFontDialog
{
  Q_OBJECT

  // just expose this property as the USER property, which is not done in QFontDialog.
  Q_PROPERTY(QFont theFont READ currentFont WRITE setCurrentFont USER true);
public:
  KLFFontDialog(QWidget *parent = 0)
    : QFontDialog(parent)
  {
  }
  
  virtual ~KLFFontDialog()
  {
  }
};


#endif
