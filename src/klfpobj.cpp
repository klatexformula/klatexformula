/***************************************************************************
 *   file klfpobj.cpp
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

#include <QDebug>
#include <QByteArray>
#include <QDataStream>

#include "klfpobj.h"

KLFPropertizedObject::KLFPropertizedObject(const QString& propNameSpace)
  : pPropNameSpace(propNameSpace)
{
}

KLFPropertizedObject::~KLFPropertizedObject()
{
}


QVariant KLFPropertizedObject::property(const QString& propname) const
{
  if ( ! propertyNameRegistered(propname) ) {
    qWarning("property(): Property `%s' not registered in property name space `%s'.",
	     qPrintable(propname), qPrintable(pPropNameSpace));
    return QVariant();
  }
  return property(propertyIdForName(propname));
}
QVariant KLFPropertizedObject::property(int propId) const
{
  if (propId >= 0 && propId < pProperties.size()) {
    // all ok, return property (possibly an empty QVariant() if prop not set)
    return pProperties[propId];
  }
  if (propId < 0) {
    qWarning("%s/property(%d): invalid property ID.", qPrintable(pPropNameSpace), propId);
    return QVariant();
  }
  // property not set
  return QVariant();
}


void KLFPropertizedObject::setProperty(const QString& propname, const QVariant& value)
{
  if ( ! propertyNameRegistered(propname) ) {
    qWarning("setProperty(): Property `%s' not registered in property name space `%s'.",
	     qPrintable(propname), qPrintable(pPropNameSpace));
    return;
  }
  setProperty(propertyIdForName(propname), value);
}
void KLFPropertizedObject::setProperty(int propId, const QVariant& value)
{
  if (propId >= 0 && propId < pProperties.size()) {
    // all ok, set this property
    pProperties[propId] = value;
    return;
  }
  if (propId < 0) {
    qWarning("%s/setProperty(id=%d): invalid property ID.", qPrintable(pPropNameSpace), propId);
    return;
  }
  // maybe our properties array needs resize for properties that could have been
  // registered after last access
  int maxId = propertyMaxId();
  if (propId <= maxId) {
    pProperties.resize(maxId + 1);
  }
  if (propId < 0 || propId >= pProperties.size() ||
      ! propertyIdRegistered(propId) ) {
    qWarning("%s/setProperty(id=%d): invalid property id.", qPrintable(pPropNameSpace), propId);
    return;
  }
  pProperties[propId] = value;
}
void KLFPropertizedObject::loadProperty(const QString& propname, const QVariant& value)
{
  int propId = propertyIdForName(propname);
  if ( propId < 0 ) {
    // register property
    propId = registerProperty(propname);
    if (propId < 0)
      return;
  }
  setProperty(propId, value);
}

QList<int> KLFPropertizedObject::propertyIdList() const
{
  QList<int> idList;
  int k;
  // walk vector and a fill a QList of all set properties' ID
  for (k = 0; k < pProperties.size(); ++k) {
    if (pProperties[k].isValid())
      idList << k;
  }
  return idList;
}

QStringList KLFPropertizedObject::propertyNameList() const
{
  QStringList propSetList;
  int k;
  // walk vector and a fill a QStringList of all set properties.
  for (k = 0; k < pProperties.size(); ++k) {
    if (pProperties[k].isValid())
      propSetList << propertyNameForId(k);
  }
  return propSetList;
}




QMap<QString,QVariant> KLFPropertizedObject::allProperties() const
{
  QList<int> propertyList = propertyIdList();
  QMap<QString,QVariant> properties;
  int k;
  // walk all properties and insert them into list
  for (k = 0; k < propertyList.size(); ++k) {
    properties[propertyNameForId(propertyList[k])] = property(propertyList[k]);
  }
  return properties;
}

void KLFPropertizedObject::setAllProperties(QMap<QString, QVariant> propValues)
{
  QStringList propKeys = propValues.keys();
  int k;
  for (k = 0; k < propKeys.size(); ++k) {
    loadProperty(propKeys[k], propValues[propKeys[k]]);
  }
}


QByteArray KLFPropertizedObject::allPropertiesToByteArray() const
{
  QByteArray data;
  {
    QDataStream stream(&data, QIODevice::WriteOnly);
    stream << *this;
    // force close of buffer in destroying stream
  }
  return data;
}

void KLFPropertizedObject::setAllPropertiesFromByteArray(const QByteArray& data)
{
  QDataStream stream(data);
  stream >> *this;
}







//  ----  PROTECTED  ----

void KLFPropertizedObject::registerBuiltInProperty(int propId, const QString& name) const
{
  registerBuiltInProperty(pPropNameSpace, propId, name);
}
int KLFPropertizedObject::registerProperty(const QString& propName) const
{
  return registerProperty(pPropNameSpace, propName);
}
int KLFPropertizedObject::propertyMaxId() const
{
  return propertyMaxId(pPropNameSpace);
}
bool KLFPropertizedObject::propertyIdRegistered(int propId) const
{
  return propertyIdRegistered(pPropNameSpace, propId);
}
bool KLFPropertizedObject::propertyNameRegistered(const QString& propertyName) const
{
  return propertyNameRegistered(pPropNameSpace, propertyName);
}
int KLFPropertizedObject::propertyIdForName(const QString& propName) const
{
  return propertyIdForName(pPropNameSpace, propName);
}
QString KLFPropertizedObject::propertyNameForId(int id) const
{
  return propertyNameForId(pPropNameSpace, id);
}
QStringList KLFPropertizedObject::registeredPropertyNameList() const
{
  return registeredPropertyNameList(pPropNameSpace);
}
QList<int> KLFPropertizedObject::registeredPropertyIdList() const
{
  return registeredPropertyIdList(pPropNameSpace);
}
QMap<QString, int> KLFPropertizedObject::registeredProperties() const
{
  return registeredProperties(pPropNameSpace);
}


//  ----  STATIC PROTECTED  ----

void KLFPropertizedObject::registerBuiltInProperty(const QString& pnamespace, int propId,
						   const QString& name)
{
  internalRegisterProperty(pnamespace, name, propId);
}
int KLFPropertizedObject::registerProperty(const QString& propNameSpace, const QString& propName)
{
  return internalRegisterProperty(propNameSpace, propName, -1);
}
int KLFPropertizedObject::propertyMaxId(const QString& propNameSpace)
{
  if ( ! pRegisteredPropertiesMaxId.contains(propNameSpace) ) {
    qWarning("propertyMaxId: property name space `%s' does not exist!", qPrintable(propNameSpace));
    return -1;
  }
  return pRegisteredPropertiesMaxId[propNameSpace];
}
bool KLFPropertizedObject::propertyIdRegistered(const QString& propNameSpace, int propId)
{
  return  ( ! propertyNameForId(propNameSpace, propId).isNull() ) ;
}
bool KLFPropertizedObject::propertyNameRegistered(const QString& propNameSpace,
						  const QString& propertyName)
{
  return  (propertyIdForName(propNameSpace, propertyName) != -1) ;
}
int KLFPropertizedObject::propertyIdForName(const QString& propNameSpace, const QString& name)
{
  if ( ! pRegisteredProperties.contains(propNameSpace) ) {
    qWarning("propertyIdForName: property name space `%s' does not exist!", qPrintable(propNameSpace));
    return -1;
  }
  QMap<QString, int> propList = pRegisteredProperties[propNameSpace];
  if ( ! propList.contains(name) )
    return -1;
  return propList.value(name);
}
QString KLFPropertizedObject::propertyNameForId(const QString& propNameSpace, int propId)
{
  if ( ! pRegisteredProperties.contains(propNameSpace) ) {
    qWarning("propertyNameForId: property name space `%s' does not exist!", qPrintable(propNameSpace));
    return QString();
  }
  QMap<QString, int> propList = pRegisteredProperties[propNameSpace];
  QList<QString> keyList = propList.keys(propId);
  if (keyList.isEmpty())
    return QString();
  if (keyList.size() > 1) {
    qWarning("What's going on?? property Id=%d not unique in prop name space `%s'.",
	     propId, qPrintable(propNameSpace));
  }
  return keyList[0];
}
QStringList KLFPropertizedObject::registeredPropertyNameList(const QString& propNameSpace)
{
  if ( ! pRegisteredProperties.contains(propNameSpace) ) {
    qWarning("registeredPropertyNameList: property name space `%s' does not exist!",
	     qPrintable(propNameSpace));
    return QStringList();
  }

  return pRegisteredProperties[propNameSpace].keys();
}
QList<int> KLFPropertizedObject::registeredPropertyIdList(const QString& propNameSpace)
{
  if ( ! pRegisteredProperties.contains(propNameSpace) ) {
    qWarning("registeredPropertyIdList: property name space `%s' does not exist!",
	     qPrintable(propNameSpace));
    return QList<int>();
  }

  return pRegisteredProperties[propNameSpace].values();
}

QMap<QString, int> KLFPropertizedObject::registeredProperties(const QString& propNameSpace)
{
  if ( ! pRegisteredProperties.contains(propNameSpace) ) {
    qWarning("registeredPropertyIdList: property name space `%s' does not exist!",
	     qPrintable(propNameSpace));
    return QMap<QString, int>();
  }
  return pRegisteredProperties[propNameSpace];
}


//  ----  PRIVATE  ----

QMap<QString, QMap<QString, int> > KLFPropertizedObject::pRegisteredProperties =
  QMap<QString, QMap<QString, int> >();

QMap<QString, int> KLFPropertizedObject::pRegisteredPropertiesMaxId = QMap<QString,int>();


int KLFPropertizedObject::internalRegisterProperty(const QString& propNameSpace,
						   const QString& propName,
						   int propId)
{
  QMap<QString, int> propList = pRegisteredProperties[propNameSpace];
  int propMaxId = -1;
  if (pRegisteredPropertiesMaxId.contains(propNameSpace)) {
    propMaxId = pRegisteredPropertiesMaxId[propNameSpace];
  }
  if (propId == -1) {
    // propMaxId is maximum ID already used, so +1 gives a free ID.
    propId = propMaxId + 1;
    // and update propMaxId to reflect the new propId...
    propMaxId = propId;
  } else {
    // used the fixed propId. Update propMaxId if necessary.
    if (propId > propMaxId)
      propMaxId = propId;
  }
  if ( propList.keys(propId).size() > 0 ) {
    qWarning("Property ID `%d' in property name space `%s' is already registered!", propId,
	     qPrintable(propNameSpace));
    return -1;
  }
  // make sure property name is valid and unique
  if (propName.isEmpty()) {
    qWarning("Cannot Register a property with empty name (in prop name space `%s')!",
	     qPrintable(propNameSpace));
    return -1;
  }
  if (propList.contains(propName)) {
    qWarning("Property `%s' already registered in prop name space `%s'.", qPrintable(propName),
	     qPrintable(propNameSpace));
    return -1;
  }
  // name and ID are valid and unique.
  // finally insert the property into list of known properties.
  pRegisteredProperties[propNameSpace][propName] = propId;
  // propMaxId was updated according to the new ID earlier in this function.
  pRegisteredPropertiesMaxId[propNameSpace] = propMaxId;
  return propId;
}



QDataStream& operator<<(QDataStream& stream, const KLFPropertizedObject& obj)
{
  stream << obj.allProperties();
  return stream;
}
QDataStream& operator>>(QDataStream& stream, KLFPropertizedObject& obj)
{
  QMap<QString,QVariant> props;
  stream >> props;
  obj.setAllProperties(props);
  return stream;
}


