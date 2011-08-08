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
  if (pObjPropConnections.contains(pname)) {
    // set connected QObject properties
    const QList<ObjPropConnection> clist = pObjPropConnections[pname];
    for (QList<ObjPropConnection>::const_iterator it = clist.begin(); it != clist.end(); ++it) {
      (*it).object->setProperty((*it).objPropName, newValue);
    }
  }
}

void KLFConfigBase::propertyValueRequested(const KLFConfigPropBase */*property*/)
{
}

void KLFConfigBase::connectQObjectProperty(const QString& configPropertyName, QObject *object,
				      const QByteArray& objPropName)
{
  KLF_DEBUG_BLOCK(KLF_FUNC_NAME) ;
  klfDbg("Connecting prop "<<configPropertyName<<" to object "<<object<<", objPropName="<<objPropName ) ;

  // check that configPropertyName is valid
  KLFConfigPropBase *p = NULL;
  for (QList<KLFConfigPropBase*>::const_iterator it = pProperties.begin(); it != pProperties.end(); ++it) {
    if ((*it)->propName() == configPropertyName) {
      p = (*it);
      break;
    }
  }
  KLF_ASSERT_NOT_NULL(p, "Invalid config property name: "<<configPropertyName<<".", return; ) ;

  ObjPropConnection c;
  c.object = object;
  c.objPropName = objPropName;

  QList<ObjPropConnection> clist = pObjPropConnections[configPropertyName];

  for (QList<ObjPropConnection>::const_iterator it = clist.begin(); it != clist.end(); ++it) {
    if (*it == c) {
      qWarning()<<KLF_FUNC_NAME<<": "<<configPropertyName<<" already connected to "<<object<<"/"<<objPropName;
      return;
    }
  }

  pObjPropConnections[configPropertyName].append(c);

  // and initialize the QObject property to the current value of that property
  QVariant value = p->toVariant();
  object->setProperty(objPropName, value);
}
void KLFConfigBase::disconnectQObjectProperty(const QString& configPropertyName, QObject *object,
					 const QByteArray& objPropName)
{
  ObjPropConnection c;
  c.object = object;
  c.objPropName = objPropName;

  QList<ObjPropConnection> & clistref = pObjPropConnections[configPropertyName];

  for (QList<ObjPropConnection>::iterator it = clistref.begin(); it != clistref.end(); ++it) {
    if (*it == c) {
      clistref.erase(it);
      return;
    }
  }

  qWarning()<<KLF_FUNC_NAME<<": "<<configPropertyName<<" is not connected to "<<object<<"/"<<objPropName;
}

void KLFConfigBase::disconnectQObject(QObject * object)
{
  QHash<QString,QList<ObjPropConnection> >::iterator pit;
  for (pit = pObjPropConnections.begin(); pit != pObjPropConnections.end(); ++pit) {
    const QString& pname = pit.key();
    QList<ObjPropConnection> & clistref = pit.value();
    for (QList<ObjPropConnection>::iterator it = clistref.begin(); it != clistref.end(); ++it) {
      if ((*it).object == object) {
	klfDbg("Removing connection between object "<<object<<" and property "<<pname) ;
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
  foreach (KLFConfigPropBase *p, pProperties) {
    if (p->propName() == name)
      return p;
  }
  qWarning()<<KLF_FUNC_NAME<<": Can't find config property name "<<name<<" !";
  return NULL;
}

