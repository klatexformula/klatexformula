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

#include <QDebug>
#include <QVariant>
#include <QByteArray>
#include <QDataStream>
#include <QTextStream>
#include <QVector>
#include <QList>
#include <QMap>
#include <QStringList>

#include <klfdefs.h>

class KLF_EXPORT KLFPropertizedObject
{
public:
  explicit KLFPropertizedObject(const QString& propertyNameSpace);
  virtual ~KLFPropertizedObject();

  virtual QVariant property(const QString& propname) const;
  virtual QVariant property(int propId) const;

  /** \brief A list of properties that have been set.
   *
   * \returns list of the IDs of all properties that have been set on this object.
   *   values of these properties are NOT included.
   *
   * More exactly: returns all propertie IDs that have a valid (see \ref QVariant::isValid())
   * value in this object.
   *
   * \warning the IDs may not be conserved between two instances or versions of
   *   KLatexFormula
   */
  QList<int> propertyIdList() const;

  /** \brief A list of properties that have been set.
   *
   * Similar to \ref propertyIdList() but returns property names.
   *
   * \returns list of the names of all properties that have been set on this object.
   *   values of these properties are not included in the list... */
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
  

  /** \brief Saves all the properties in binary form.
   */
  QByteArray allPropertiesToByteArray() const;

  /** \brief Loads all properties saved by \ref allPropertiesToByteArray()
   */
  void setAllPropertiesFromByteArray(const QByteArray& data);

  /** \brief Flags for tuning the \ref toString() method.
   */
  enum ToStringFlag {
    ToStringUseHtml = 0x0001, //!< Encapsulates output in an HTML &lt;table&gt; and escapes strings.
    ToStringUseHtmlDiv = 0x0002, //!< Uses &lt;div&gt; with CSS classes instead of a table (HTML only)
    ToStringQuoteValues = 0x0004, //!< Ensures that non-html output is machine parsable.
    ToStringAllProperties = 0x0008 //!< Include also all non-explicitely-set properties
  };
  /** \brief Formats the property contents in a (human and parsable) string
   */
  virtual QString toString(uint toStringFlags = 0) const;

  //! See the corresponding protected static method
  int propertyMaxId() const;
  //! See the corresponding protected static method
  bool propertyIdRegistered(int propId) const;
  //! See the corresponding protected static method
  bool propertyNameRegistered(const QString& propertyName) const;
  //! See the corresponding protected static method
  int propertyIdForName(const QString& propertyName) const;
  //! See the corresponding protected static method
  QString propertyNameForId(int propId) const;
  //! See the corresponding protected static method
  QList<int> registeredPropertyIdList() const;
  //! See the corresponding protected static method
  QStringList registeredPropertyNameList() const;
  //! See the corresponding protected static method
  QMap<QString, int> registeredProperties() const;

protected:

  /** This method is called whenever the value of a given property changes.
   *
   * Subclasses may reimplement to watch the properties for changes. The base
   * implementation does nothing.
   *
   * \param propId the property ID that changed
   * \param oldValue the previous value of the property
   * \param newValue the new value of the property
   */
  virtual void propertyValueChanged(int propId, const QVariant& oldvalue,
				    const QVariant& newValue);


  /** Sets the given property to \c value. If propname is not registered,
   * this function fails. */
  virtual void setProperty(const QString& propname, const QVariant& value);
  virtual void setProperty(int propId, const QVariant& value);

  /** Like \c setProperty(), except the property name \c propname is registered
   * if it is not registered.
   *
   * \returns The property ID that was set or -1 for failure. */
  virtual int loadProperty(const QString& propname, const QVariant& value);

  /** shortcut for the corresponding static method */
  void registerBuiltInProperty(int propId, const QString& name) const;
  /** shortcut for the corresponding static method */
  int registerProperty(const QString& propertyName) const;

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
  /** No default constructor. */
  KLFPropertizedObject() { }

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

QTextStream& operator<<(QTextStream& stream, const KLFPropertizedObject& obj);

QDebug& operator<<(QDebug& stream, const KLFPropertizedObject& obj);




#endif
