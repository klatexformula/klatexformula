/***************************************************************************
 *   file klfconfigbase.cpp
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

#include "klfconfigbase.h"


KLFConfigPropBase::KLFConfigPropBase()
{
}
KLFConfigPropBase::~KLFConfigPropBase()
{
}

// ---------


bool KLFConfigBase::okChangeProperty(KLFConfigPropBase */*property*/, const QVariant& /*oldValue*/,
				     const QVariant& /*newValue*/)
{
  return true;
}

void KLFConfigBase::propertyChanged(KLFConfigPropBase *property, const QVariant& /*oldValue*/,
				    const QVariant& newValue)
{
  KLF_ASSERT_NOT_NULL(property, "property is NULL!!", return; ) ;

  const QString pname = property->propName();
  if (pObjConnections.contains(pname)) {
    // set connected QObject properties
    const QList<ObjConnection> clist = pObjConnections[pname];
    for (QList<ObjConnection>::const_iterator it = clist.begin(); it != clist.end(); ++it) {
      if ((*it).target == Property) {
	(*it).object->setProperty((*it).targetName, newValue);
      } else if ((*it).target == Slot) {
	QMetaObject::invokeMethod((*it).object, (*it).targetName,
				  QGenericArgument(newValue.typeName(), newValue.data()));
      } else {
	qWarning()<<KLF_FUNC_NAME<<": Unknown target type "<<(*it).target<<" !";
      }
    }
  }
}

void KLFConfigBase::propertyValueRequested(const KLFConfigPropBase */*property*/)
{
}


void KLFConfigBase::connectQObjectProperty(const QString& configPropertyName, QObject *object,
					   const QByteArray& objPropName)
{
  connectQObject(configPropertyName, object, Property, objPropName);
}
void KLFConfigBase::connectQObjectSlot(const QString& configPropertyName, QObject *object,
					   const QByteArray& slotName)
{
  connectQObject(configPropertyName, object, Slot, slotName);
}
void KLFConfigBase::disconnectQObjectProperty(const QString& configPropertyName, QObject *object,
					      const QByteArray& objPropName)
{
  disconnectQObject(configPropertyName, object, Property, objPropName);
}
void KLFConfigBase::disconnectQObjectSlot(const QString& configPropertyName, QObject *object,
					   const QByteArray& slotName)
{
  disconnectQObject(configPropertyName, object, Slot, slotName);
}

void KLFConfigBase::connectQObject(const QString& configPropertyName, QObject *object,
				   ConnectionTarget target, const QByteArray& targetName)
{
  KLF_DEBUG_BLOCK(KLF_FUNC_NAME) ;
  klfDbg("Connecting prop "<<configPropertyName<<" to object "<<object<<", targettype="<<target
	 <<", targetName="<<targetName ) ;

  // check that configPropertyName is valid
  KLFConfigPropBase *p = NULL;
  for (QList<KLFConfigPropBase*>::const_iterator it = pProperties.begin(); it != pProperties.end(); ++it) {
    if ((*it)->propName() == configPropertyName) {
      p = (*it);
      break;
    }
  }
  KLF_ASSERT_NOT_NULL(p, "Invalid config property name: "<<configPropertyName<<".", return; ) ;

  ObjConnection c;
  c.target = target;
  c.object = object;
  c.targetName = targetName;

  QList<ObjConnection> clist = pObjConnections[configPropertyName];

  for (QList<ObjConnection>::const_iterator it = clist.begin(); it != clist.end(); ++it) {
    if (*it == c) {
      qWarning()<<KLF_FUNC_NAME<<": "<<configPropertyName<<" already connected to "<<object<<"/"<<targetName;
      return;
    }
  }

  pObjConnections[configPropertyName].append(c);

  // and initialize the QObject property/slot to the current value of that config property
  QVariant value = p->toVariant();
  if (c.target == Property) {
    object->setProperty(targetName, value);
  } else if (c.target == Slot) {
    QMetaObject::invokeMethod(object, targetName, QGenericArgument(value.typeName(), value.data()));
  }
}

void KLFConfigBase::disconnectQObject(const QString& configPropertyName, QObject *object,
				      ConnectionTarget target, const QByteArray& targetName)
{
  ObjConnection c;
  c.target = target;
  c.object = object;
  c.targetName = targetName;

  QList<ObjConnection> & clistref = pObjConnections[configPropertyName];

  for (QList<ObjConnection>::iterator it = clistref.begin(); it != clistref.end(); ++it) {
    if (*it == c) {
      klfDbg("removed QObject-connection target-type "<<(*it).target<<", target-name "<<(*it).targetName) ;
      clistref.erase(it);
      return;
    }
  }

  qWarning()<<KLF_FUNC_NAME<<": "<<configPropertyName<<" is not connected to "<<object<<"/"<<targetName;
}

void KLFConfigBase::disconnectQObject(QObject * object)
{
  QHash<QString,QList<ObjConnection> >::iterator pit;
  for (pit = pObjConnections.begin(); pit != pObjConnections.end(); ++pit) {
    const QString& pname = pit.key();
    Q_UNUSED(pname) ;
    QList<ObjConnection> & clistref = pit.value();
    for (QList<ObjConnection>::iterator it = clistref.begin(); it != clistref.end(); ++it) {
      if ((*it).object == object) {
	klfDbg("Removing connection between object "<<object<<" and config property "<<pname) ;
	clistref.erase(it);
      }
    }
  }
}


QStringList KLFConfigBase::propertyList() const
{
  QStringList list;
  foreach (KLFConfigPropBase *p, pProperties) {
    list << p->propName();
  }
  return list;
}

KLFConfigPropBase * KLFConfigBase::property(const QString& name)
{
  if (name.isEmpty()) {
    klfWarning("Requesting property instance for empty name !?! Program might crash!") ;
    return NULL;
  }
  foreach (KLFConfigPropBase *p, pProperties) {
    if (p->propName() == name)
      return p;
  }
  qWarning()<<KLF_FUNC_NAME<<": Can't find config property name "<<name<<" !";
  return NULL;
}

