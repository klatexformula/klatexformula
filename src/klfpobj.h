/***************************************************************************
 *   file klfpobj.h
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


#ifndef KLFPOBJ_H
#define KLFPOBJ_H

#include <QVariant>
#include <QDataStream>
#include <QVector>
#include <QList>
#include <QMap>
#include <QStringList>

#include <klfdefs.h>

class KLF_EXPORT KLFPropertizedObject
{
public:
  KLFPropertizedObject(const QString& propertyNameSpace);
  virtual ~KLFPropertizedObject();

  virtual QVariant property(const QString& propname) const;
  virtual QVariant property(int propId) const;
  /** Sets the given property to \c value. If propname is not registered,
   * this function fails. */
  virtual void setProperty(const QString& propname, const QVariant& value);
  virtual void setProperty(int propId, const QVariant& value);

  /** \brief A list of properties that have been set.
   *
   * \returns list of the IDs of all properties that have been set on this object.
   *   values of these properties are NOT included.
   *
   * \warning the IDs may not be conserved between two instances or versions of
   *   KLatexFormula */
  QList<int> propertyIdList() const;

  /** \brief A list of properties that have been set.
   *
   * \returns list of the names of all properties that have been set on this object.
   *   values of these properties are NOT included. */
  QStringList propertyNameList() const;

  /** \brief Returns all properties that have been set.
   *
   * \returns a QMap of all properties (their values) that have been set on this
   * object, values stored by property name (property ID is not available). */
  QMap<QString,QVariant> allProperties() const;

  /** \brief Initializes properties to given values
   *
   * Clears all properties that have been set and replaces them with those given
   * in argument. Any property name in \c propValues that doesn't exist in registered
   * properties, is registered. */
  void setAllProperties(QMap<QString, QVariant> propValues);
  


protected:
  // shortcuts for the corresponding static methods
  void registerBuiltInProperty(int propId, const QString& name) const;
  int registerProperty(const QString& propertyName) const;
  int propertyMaxId() const;
  bool propertyIdRegistered(int propId) const;
  bool propertyNameRegistered(const QString& propertyName) const;
  int propertyIdForName(const QString& propertyName) const;
  QString propertyNameForId(int propId) const;
  QList<int> registeredPropertyIdList() const;
  QStringList registeredPropertyNameList() const;
  QMap<QString, int> registeredProperties() const;

  /** \note If property name space does not exist, it is created. */
  static void registerBuiltInProperty(const QString& propNameSpace, int propId,
				      const QString& name);
  static int registerProperty(const QString& propNameSpace, const QString& propertyName);
  static int propertyMaxId(const QString& propNameSpace);
  /** true if a property with ID \c propId exists in property name space
   * \c propNameSpace . */
  static bool propertyIdRegistered(const QString& propNameSpace, int propId);
  /** true if a property of name \c propertyName exists in property name space
   * \c propNameSpace . */
  static bool propertyNameRegistered(const QString& propNameSpace, const QString& propertyName);
  /** \returns the ID corresponding to property name \c propertyName in property name
   * space \c propNameSpace. If the property name space does not exist, or if the property
   * does not exist in that name space, \c -1 is returned (silently).
   *
   * \c propNameSpace must exist, however, or a warning message is issued. */
  static int propertyIdForName(const QString& propNameSpace, const QString& propertyName);
  /** opposite of propertyIdForName() with similar behavior. Silently returns
   * <tt>QString()</tt> on failure. */
  static QString propertyNameForId(const QString& propNameSpace, int propId);
  static QList<int> registeredPropertyIdList(const QString& propNameSpace);
  static QStringList registeredPropertyNameList(const QString& propNameSpace);
  static QMap<QString, int> registeredProperties(const QString& propNameSpace);

  QString propertyNameSpace() const { return pPropNameSpace; }

  QVector<QVariant> propertyVector() const { return pProperties; }

private:

  QString pPropNameSpace;

  QVector<QVariant> pProperties;

  /** Creates \c propNameSpace property name space if it doesn't exist.
   * \returns the property ID (\c propId or a new property ID if \c propId
   * is \c -1) */
  static int internalRegisterProperty(const QString& propNameSpace, const QString& name,
				      int propId = -1);
  static QMap<QString, QMap<QString, int> > pRegisteredProperties;
  static QMap<QString, int> pRegisteredPropertiesMaxId;
};


QDataStream& operator<<(QDataStream& stream, const KLFPropertizedObject& obj);
QDataStream& operator>>(QDataStream& stream, KLFPropertizedObject& obj);




#endif
