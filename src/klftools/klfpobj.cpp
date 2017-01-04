/***************************************************************************
 *   file klfpobj.cpp
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

#include <QtDebug>
#include <QByteArray>
#include <QDataStream>
#include <QTextStream>

#include "klfpobj.h"


KLFAbstractPropertizedObject::KLFAbstractPropertizedObject()
{
}
KLFAbstractPropertizedObject::~KLFAbstractPropertizedObject()
{
}

KLFSpecifyableType::KLFSpecifyableType()
{
}
KLFSpecifyableType::~KLFSpecifyableType()
{
}



// -----------------------------


QMap<QString,QVariant> KLFAbstractPropertizedObject::allProperties() const
{
  QMap<QString,QVariant> data;
  QStringList pnames = propertyNameList();
  foreach (QString pname, pnames) {
    data[pname] = property(pname);
  }
  return data;
}

bool KLFAbstractPropertizedObject::setAllProperties(const QMap<QString,QVariant>& data)
{
  KLF_DEBUG_BLOCK(KLF_FUNC_NAME) ;
  klfDbg("data="<<data) ;
  bool allok = true;
  for (QVariantMap::const_iterator it = data.begin(); it != data.end(); ++it) {
    bool ok = setProperty(it.key(), it.value());
    if (!ok) {
      allok = false;
      qWarning()<<KLF_FUNC_NAME<<": Can't set property "<<it.key()<<" to "<<it.value();
    }
  }
  return allok;
}


// --------

KLF_EXPORT QDataStream& operator<<(QDataStream& stream, const KLFEnumType& e)
{
  return stream << e.specification() << qint32(e.value());
}
KLF_EXPORT QDataStream& operator>>(QDataStream& stream, KLFEnumType& e)
{
  QByteArray s;
  qint32 x;
  stream >> s >> x;
  e.setSpecification(s);
  e.setValue(x);
  return stream;
}


// -------




KLFPropertizedObject::KLFPropertizedObject(const QString& propNameSpace)
  : pPropNameSpace(propNameSpace)
{
  // ensure the property name space exists
  if (!pRegisteredProperties.contains(propNameSpace))
    pRegisteredProperties[propNameSpace] = QMap<QString,int>();
  if (!pRegisteredPropertiesMaxId.contains(propNameSpace))
    pRegisteredPropertiesMaxId[propNameSpace] = -1;
}

KLFPropertizedObject::~KLFPropertizedObject()
{
}


QVariant KLFPropertizedObject::property(const QString& propname) const
{
  int propId = propertyIdForName(propname);
  if (propId < 0) {
    qWarning("%s[%s](): Property `%s' not registered.", KLF_FUNC_NAME, qPrintable(pPropNameSpace),
	     qPrintable(propname));
    return QVariant();
  }
  return property(propId);
}
QVariant KLFPropertizedObject::property(int propId) const
{
  if (propId >= 0 && propId < pProperties.size()) {
    // all ok, return property (possibly an empty QVariant() if prop not set)
    return pProperties[propId];
  }
  if (propId < 0) {
    qWarning("%s[%s](%d): invalid property ID.", KLF_FUNC_NAME, qPrintable(pPropNameSpace),
	     propId);
    return QVariant();
  }
  // property not set (or property not registered)
  return QVariant();
}

QVariant KLFPropertizedObject::property(const QString& propname, const QVariant& defaultValue) const
{
  int propId = propertyIdForName(propname);
  if (propId < 0) {
    return defaultValue;
  }
  QVariant value = property(propId);
  if (value.isValid())
    return value;
  return defaultValue;
}

bool KLFPropertizedObject::hasPropertyValue(const QString& propName) const
{
  return property(propName, QVariant()).isValid();
}

bool KLFPropertizedObject::hasPropertyValue(int propId) const
{
  if (!propertyIdRegistered(propId))
    return false;

  return hasPropertyValue(propertyNameForId(propId));
}


void KLFPropertizedObject::propertyValueChanged(int , const QVariant& ,
						const QVariant& )
{
  // do nothing. Subclasses may implement thier own behavior.
}

bool KLFPropertizedObject::doSetProperty(const QString& propname, const QVariant& value)
{
  if ( ! propertyNameRegistered(propname) ) {
    qWarning("%s[%s](): Property `%s' not registered.", KLF_FUNC_NAME, qPrintable(pPropNameSpace),
	     qPrintable(propname));
    return false;
  }
  return doSetProperty(propertyIdForName(propname), value);
}
bool KLFPropertizedObject::doSetProperty(int propId, const QVariant& value)
{
  if (propId >= 0 && propId < pProperties.size()) {
    // all ok, set this property
    QVariant oldvalue = pProperties[propId];
    pProperties[propId] = value;
    propertyValueChanged(propId, oldvalue, value);
    return true;
  }
  if (propId < 0) {
    qWarning("%s[%s](id=%d): invalid property ID.", KLF_FUNC_NAME, qPrintable(pPropNameSpace),
	     propId);
    return false;
  }
  // maybe our properties array needs resize for properties that could have been
  // registered after last access
  int maxId = propertyMaxId();
  if (propId <= maxId) {
    pProperties.resize(maxId + 1);
  }
  if (propId < 0 || propId >= pProperties.size() ||
      ! propertyIdRegistered(propId) ) {
    qWarning("%s[%s](id=%d): invalid property id.", KLF_FUNC_NAME, qPrintable(pPropNameSpace),
	     propId);
    return false;
  }
  QVariant oldvalue = pProperties[propId];
  pProperties[propId] = value;
  propertyValueChanged(propId, oldvalue, value);
  return true;
}
int KLFPropertizedObject::doLoadProperty(const QString& propname, const QVariant& value)
{
  klfDbg("propname="<<propname<<" value="<<value) ;
  int propId = propertyIdForName(propname);
  if ( propId < 0 ) {
    // register property
    propId = registerProperty(propname);
    if (propId < 0)
      return -1;
  }
  doSetProperty(propId, value);
  return propId;
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

bool KLFPropertizedObject::setProperty(const QString& propname, const QVariant& value)
{
  return doLoadProperty(propname, value) >= 0;
}

bool KLFPropertizedObject::setProperty(int propId, const QVariant& value)
{
  KLF_ASSERT_CONDITION(propertyIdRegistered(propId), "Property ID="<<propId<<" is not registered!",
		       return false; ) ;

  return setProperty(propertyNameForId(propId), value);
}


bool KLFPropertizedObject::setAllProperties(const QMap<QString, QVariant>& propValues)
{
  KLF_DEBUG_BLOCK(KLF_FUNC_NAME) ;
  klfDbg("propValues="<<propValues) ;

  bool allok = true;
  QStringList propKeys = propValues.keys();
  int k;
  for (k = 0; k < propKeys.size(); ++k) {
    // bypass check, set property anyway
    bool ok = (doLoadProperty(propKeys[k], propValues[propKeys[k]]) >= 0);
    if (!ok) {
      allok = false;
      qWarning()<<KLF_FUNC_NAME<<": Failed to load property "<<propKeys[k]<<" with value "<<propValues[propKeys[k]];
    }
  }
  return allok;
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

/*
QVariant KLFPropertizedObject::parsePropertyValue(int propId, const QString& strvalue)
{
  KLF_ASSERT_CONDITION(propertyIdRegistered(propId), "Property ID="<<propId<<" is not registered!",
		       return QVariant(); ) ;

  return parsePropertyValue(propertyNameForId(propId), strvalue);
}

QVariant KLFPropertizedObject::parsePropertyValue(const QString& / *propName* /, const QString& / *strvalue* /)
{
  return QVariant();
}
*/





QString KLFPropertizedObject::toString(uint toStringFlags) const
{
  QString s;

  int k;
  QList<int> props;
  bool html = (toStringFlags & ToStringUseHtml);
  bool quote = (toStringFlags & ToStringQuoteValues);
  bool allprops = (toStringFlags & ToStringAllProperties);
  bool usehtmldiv = (toStringFlags & ToStringUseHtmlDiv);

  if (allprops)
    props = registeredPropertyIdList();
  else
    props = propertyIdList();

  if (html) {
    if (usehtmldiv) {
      s = QString("<div class=\"klfpobj_entry\">\n<div class=\"klfpobj_name\">%2</div>\n")
	.arg(pPropNameSpace.toHtmlEscaped());
    } else {
      s = QString("<table class=\"klfpobj_tentry\">\n"
		  "<tr colspan=\"2\" class=\"klfpobj_tname\"><th>%1</th></tr>\n")
	.arg(pPropNameSpace.toHtmlEscaped());
    }
  } else {
    s = QString("%1\n").arg(pPropNameSpace);
  }

  for (k = 0; k < props.size(); ++k) {
    QString pname = propertyNameForId(props[k]);
    QVariant vval = property(props[k]);
    bool isnull = !vval.isValid();
    bool canstring = vval.canConvert(QVariant::String);
    QString value = vval.toString();
    if (html) {
      if (usehtmldiv)
	s += QString("<div class=\"klfpobj_prop_%1\"><div class=\"klfpobj_propname\">%2</div>: "
		     "<div class=\"klfpobj_propvalue\">%3</div></div>\n")
	  .arg(pname, pname, value.toHtmlEscaped());
      else
	s += QString("  <tr class=\"klfpobj_tprop_%1\"><td class=\"klfpobj_tpropname\">%2</td>"
		     "<td class=\"klfpobj_tpropvalue\">%3</td></tr>\n")
	  .arg(pname, pname, value.toHtmlEscaped());
    } else {
      if (quote) {
	if (!isnull && canstring) {
	  value.replace("\\", "\\\\");
	  value.replace("\"", "\\\"");
	  value = '"' + value + '"';
	} else if (!isnull) {
	  value = QString("[%1]").arg(vval.typeName());
	} else {
	  // true null
	  value = "NULL";
	}
      }
      s += QString("%1: %2\n").arg(propertyNameForId(props[k]), value);
    }
  }
  s += "\n";
  if (html) {
    if (usehtmldiv) {
      s += "</div>\n";
    } else {
      s += "</table>\n";
    }
  }
  
  return s;
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

// ----  PROTECTED  ----

void KLFPropertizedObject::registerBuiltInProperty(int propId, const QString& name) const
{
  registerBuiltInProperty(pPropNameSpace, propId, name);
}
int KLFPropertizedObject::registerProperty(const QString& propName) const
{
  return registerProperty(pPropNameSpace, propName);
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
    qWarning("%s(): property name space `%s' does not exist!", KLF_FUNC_NAME,
	     qPrintable(propNameSpace));
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
    qWarning("%s: property name space `%s' does not exist!", KLF_FUNC_NAME,
	     qPrintable(propNameSpace));
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
    qWarning("%s: property name space `%s' does not exist!", KLF_FUNC_NAME,
	     qPrintable(propNameSpace));
    return QString();
  }
  QMap<QString, int> propList = pRegisteredProperties[propNameSpace];
  QList<QString> keyList = propList.keys(propId);
  if (keyList.isEmpty())
    return QString();
  if (keyList.size() > 1) {
    qWarning("%s: What's going on?? property Id=%d not unique in prop name space `%s'.",
	     KLF_FUNC_NAME, propId, qPrintable(propNameSpace));
  }
  return keyList[0];
}
QStringList KLFPropertizedObject::registeredPropertyNameList(const QString& propNameSpace)
{
  if ( ! pRegisteredProperties.contains(propNameSpace) ) {
    qWarning("%s: property name space `%s' does not exist!", KLF_FUNC_NAME,
	     qPrintable(propNameSpace));
    return QStringList();
  }

  return pRegisteredProperties[propNameSpace].keys();
}
QList<int> KLFPropertizedObject::registeredPropertyIdList(const QString& propNameSpace)
{
  if ( ! pRegisteredProperties.contains(propNameSpace) ) {
    qWarning("%s: property name space `%s' does not exist!", KLF_FUNC_NAME,
	     qPrintable(propNameSpace));
    return QList<int>();
  }

  return pRegisteredProperties[propNameSpace].values();
}

QMap<QString, int> KLFPropertizedObject::registeredProperties(const QString& propNameSpace)
{
  if ( ! pRegisteredProperties.contains(propNameSpace) ) {
    qWarning("%s: property name space `%s' does not exist!", KLF_FUNC_NAME,
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
  KLF_DEBUG_BLOCK(KLF_FUNC_NAME) ;
  klfDbg("propNameSpace = " << propNameSpace << ", propName = " << propName << ", propId = " << propId) ;
  
  const QMap<QString, int> propList = pRegisteredProperties[propNameSpace];
  int propMaxId = -1;
  if (pRegisteredPropertiesMaxId.contains(propNameSpace)) {
    propMaxId = pRegisteredPropertiesMaxId[propNameSpace];
  }
  if (propId == -1) {
    // propMaxId is maximum ID already used, so +1 gives a free ID.
    propId = propMaxId + 1;
    // and update propMaxId to account for the new propId...
    propMaxId = propId;
  } else {
    // used the fixed propId. Update propMaxId if necessary.
    if (propId > propMaxId) {
      propMaxId = propId;
    }
  }
  if ( propList.keys(propId).size() > 0 ) {
    QString oldPropName = propList.keys(propId).at(0);
    if (propName == oldPropName) {
      return propId; // already registered, return that property ID
    }
    qWarning("%s[%s]: Property ID `%d' is already registered with conflicting names!\n"
	     "\told name is `%s', new is `%s'",
	     KLF_FUNC_NAME, qPrintable(propNameSpace), propId, qPrintable(oldPropName),
	     qPrintable(propName));
    return -1;
  }
  // make sure property name is valid and unique
  if (propName.isEmpty()) {
    qWarning("%s[%s]: Cannot Register a property with empty name!", KLF_FUNC_NAME,
	     qPrintable(propNameSpace));
    return -1;
  }
  if (propList.contains(propName)) {
    qWarning("%s[%s]: Property `%s' already registered.", KLF_FUNC_NAME, qPrintable(propNameSpace),
	     qPrintable(propName));
    return -1;
  }
  // name and ID are valid and unique.
  // finally insert the property into list of known properties.
  pRegisteredProperties[propNameSpace][propName] = propId;
  // propMaxId was updated according to the new ID earlier in this function.
  pRegisteredPropertiesMaxId[propNameSpace] = propMaxId;
  return propId;
}


bool operator==(const KLFPropertizedObject& a, const KLFPropertizedObject& b)
{
  if (a.pPropNameSpace != b.pPropNameSpace)
    return false;
  QList<int> propIds = a.registeredPropertyIdList();
  int k;
  for (k = 0; k < propIds.size(); ++k)
    if (a.property(propIds[k]) != b.property(propIds[k]))
      return false;

  return true;
}



QDataStream& KLFPropertizedObject::streamInto(QDataStream& stream) const
{
  stream << allProperties();
  return stream;
}
QDataStream& KLFPropertizedObject::streamFrom(QDataStream& stream)
{
  QMap<QString,QVariant> props;
  stream >> props;
  setAllProperties(props);
  return stream;
}
QDataStream& operator<<(QDataStream& stream, const KLFPropertizedObject& obj)
{
  return obj.streamInto(stream);
}
QDataStream& operator>>(QDataStream& stream, KLFPropertizedObject& obj)
{
  return obj.streamFrom(stream);
}


QTextStream& operator<<(QTextStream& stream, const KLFPropertizedObject& obj)
{
  return stream << obj.toString();
}



QDebug& operator<<(QDebug& stream, const KLFPropertizedObject& obj)
{
  stream << obj.allProperties();
  return stream;
}



