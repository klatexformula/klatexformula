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

/** \brief A class that holds properties.
 *
 * This class is meant to be subclassed to create objects that need to store
 * properties. For an example, see \ref KLFLibEntry.
 *
 * Properties are stored in \ref QVariant "QVariant"s, are referenced either by an
 * integer ID or a QString name.
 *
 * A subclass will be an object of a given logical type, say a library entry. It may
 * want to store a few standard properties (Latex string, Preview image, Category and
 * Tags string, etc.). However, eg. plugins may be able to store and understand
 * more advanced properties (say the icon's position when viewed in an icon view) than
 * what the base class KLFLibEntry had anticipated. In that case, the property needs
 * to be registered statically with \ref registerProperty() or
 * \ref registerBuiltInProperty(). Properties are registered statically, that means
 * that all instances of, say a KLFLibEntry, always share the same property ID and names,
 * even though they each store different values for them.
 *
 * Property values may be queried with \ref property(const QString&) or \ref property(int),
 * and may be set with setProperty().
 *
 * Subclasses have to provide their own public API for setting and/or registering property
 * values, as setProperty() and registerProperty() are protected. This is because depending
 * on the subclass this operation should not be openly publicly permitted without any checks.
 *
 * All instances of a given subclass, say a KLFLibEntry, have values assigned to all
 * registered properties. By default, their values are an invalid QVariant: <tt>QVariant()</tt>.
 *
 * \warning the IDs may not be conserved between two instances or versions of KLatexFormula,
 *   because they may be attributed dynamically, or values in a static enum may change from one
 *   version of, say a KLFLibEntry, to another. When exporting property values, always use the
 *   string identifier (see for example \ref allProperties())
 *
 * <b>What is the <i>Property Name Space</i>?</b> The Property Name Space is a string identifying
 * the type of object an instance belongs to. This will typically be the subclass name. Since
 * all properties are stored statically in this object, different subclasses must access different
 * registered properties, and that is what the &quot;property name spaces&quot; are meant for.
 * Subclasses specify once and for all the property name space to the
 * \ref KLFPropertizedObject(const QString&) constructor, then they can forget about it.
 *
 * \note For fast access, the property values are stored in a \ref QVector&lt;\ref QVariant&gt;.
 *   The \c propId is just an index in that vector. Keep that in mind before defining static
 *   high property IDs. Since property IDs are not fixed, it is better to have the static property
 *   IDs defined contiguously in an enum.
 */
class KLF_EXPORT KLFPropertizedObject
{
public:
  /** Constructs a KLFPropertizedObject, that implements an object of the kind
   * \c propertyNameSpace. See the \ref KLFPropertizedObject "Class Documentation" for more
   * information on property name spaces. */
  explicit KLFPropertizedObject(const QString& propertyNameSpace);
  virtual ~KLFPropertizedObject();

  /** Returns the value of the property designated by the name \c propName, in this current
   * object instance.
   *
   * If the property has not been registered, a warning is printed and this function returns
   * an invalid QVariant.
   *
   * If the property has not yet been set, an invalid QVariant() is returned.
   */
  virtual QVariant property(const QString& propName) const;
  /** Returns the value of the property designated by its ID \c propId in this current object
   * instance.
   *
   * If no value was previously set for the given property, and the property has been registered,
   * returns an invalid QVariant().
   *
   * This function does not check that \c propId has been registered. However the worst that
   * can happen is that an invalid QVariant() is returned.
   */
  virtual QVariant property(int propId) const;

  /** \brief A list of properties that have been set.
   *
   * \returns list of the IDs of all properties that have been set on this object. The
   *   values of these properties are NOT included in the returned list.
   *
   * More exactly: returns all property IDs that have a valid (see \ref QVariant::isValid())
   * value in this object.
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
   * \returns a QMap of all properties (and their values) that have been set on this
   * object, values stored by property name. */
  QMap<QString,QVariant> allProperties() const;

  /** \brief Initializes properties to given values
   *
   * Clears all properties that have been set and replaces them with those given
   * in argument. Any property name in \c propValues that doesn't exist in registered
   * properties, is registered.
   * */
  void setAllProperties(const QMap<QString, QVariant>& propValues);
  

  /** \brief Saves all the properties in binary form.
   *
   * Basically flushes the output of allProperties() into a QByteArray.
   */
  QByteArray allPropertiesToByteArray() const;

  /** \brief Loads all properties saved by \ref allPropertiesToByteArray()
   *
   * Reads the properties from \c data and calls setAllProperties().
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
  /** \brief Formats the property contents in a (human and/or parsable) string
   *
   * \param toStringFlags is a binary OR value of flags defined in \ref ToStringFlag.
   */
  virtual QString toString(uint toStringFlags = 0) const;

  //! See the corresponding protected static method
  /** Shortcut for the corresponding (protected) static method, where the property name
   * space is automatically detected. */
  int propertyMaxId() const;
  //! See the corresponding protected static method
  /** Shortcut for the corresponding (protected) static method, where the property name
   * space is automatically detected. */
  bool propertyIdRegistered(int propId) const;
  //! See the corresponding protected static method
  /** Shortcut for the corresponding (protected) static method, where the property name
   * space is automatically detected. */
  bool propertyNameRegistered(const QString& propertyName) const;
  //! See the corresponding protected static method
  /** Shortcut for the corresponding (protected) static method, where the property name
   * space is automatically detected. */
  int propertyIdForName(const QString& propertyName) const;
  //! See the corresponding protected static method
  /** Shortcut for the corresponding (protected) static method, where the property name
   * space is automatically detected. */
  QString propertyNameForId(int propId) const;
  //! See the corresponding protected static method
  /** Shortcut for the corresponding (protected) static method, where the property name
   * space is automatically detected. */
  QList<int> registeredPropertyIdList() const;
  //! See the corresponding protected static method
  /** Shortcut for the corresponding (protected) static method, where the property name
   * space is automatically detected. */
  QStringList registeredPropertyNameList() const;
  //! See the corresponding protected static method
  /** Shortcut for the corresponding (protected) static method, where the property name
   * space is automatically detected. */
  QMap<QString, int> registeredProperties() const;

protected:

  /** This method is called whenever the value of a given property changes.
   *
   * Subclasses may reimplement to watch the properties for changes. The default
   * implementation does nothing.
   *
   * \param propId the property ID that changed
   * \param oldValue the previous value of the property
   * \param newValue the new value of the property
   */
  virtual void propertyValueChanged(int propId, const QVariant& oldValue,
				    const QVariant& newValue);


  /** Sets the given property to \c value. If propname is not registered,
   * this function fails. */
  virtual void setProperty(const QString& propname, const QVariant& value);
  /** Sets the property identified by propId to the value \c value.
   * Fails if the property \c propId is not registered. */
  virtual void setProperty(int propId, const QVariant& value);

  /** Like \c setProperty(), except the property name \c propname is registered
   * with \ref registerProperty() if it is not registered.
   *
   * \returns The property ID that was set or -1 for failure.
   */
  virtual int loadProperty(const QString& propname, const QVariant& value);

  /** shortcut for the corresponding static method. Detects the correct property
   * name space and calls
   * \ref registerBuiltInProperty(const QString&, int, const QString&) */
  void registerBuiltInProperty(int propId, const QString& name) const;
  /** shortcut for the corresponding static method. Detects the correct property
   * name space and calls \ref registerProperty(const QString&, const QString&). */
  int registerProperty(const QString& propertyName) const;

  /** Registers the property named \c name with the fixed ID \c propId.
   *
   * If the property is already registered with the exact same name and ID, then this
   * function does nothing.
   *
   * If the given property name or the given property ID is already registered, but
   * the name or the ID doesn't match exactly, then a warning is printed and the
   * property is not registered.
   *
   * The property name has to be non-empty (\ref QString::isEmpty()), or a warning is
   * printed and the property is not registered.
   *
   * Subclasses should call this function _once_ per built-in property they want to
   * register. For example this could be achieved with the following code:
   * \code
   * // mybook.h
   * class MyBook : public KLFPropertizedObject {
   * public:
   *   enum { PropTitle, PropAuthor, PropEditor, PropYear };
   *   MyBook();
   *
   *   ... // API to access/write properties
   *
   * private:
   *   static void initBuiltInProperties();
   * };
   *
   * MyBook::MyBook() : KLFPropertizedObject("MyBook") {
   *   initBuiltInProperties();
   * }
   * void MyBook::initBuiltInProperties() {
   *   KLF_FUNC_SINGLE_RUN ;
   *
   *   registerBuiltInProperty(PropTitle, "Title");
   *   registerBuiltInProperty(PropAuthor, "Author");
   *   registerBuiltInProperty(PropEditor, "Editor");
   *   registerBuiltInProperty(PropYear, "Year");
   * }
   *
   * ... // other function definitions
   *
   * \endcode
   *
   * \note If property name space does not exist, it is created.
   *
   * Subclasses may prefer to use the more convenient function
   * \ref registerBuiltInProperty(int propId, const QString& propName),
   * which automatically uses the correct property name space.
   */
  static void registerBuiltInProperty(const QString& propNameSpace, int propId,
				      const QString& name);
  /** Registers the property \c propertyName and assigns a new unused ID for that property,
   * and returns the newly assigned ID.
   *
   * If the property name is already registered, this function prints a warning and returns
   * \c -1.
   *
   * The property name must be non-empty (see \ref QString::isEmpty()), or this function
   * will print a warning and return \c -1.
   *
   * Subclasses may prefer to use the more convenient function
   * \ref registerProperty(const QString&),
   * which automatically uses the correct property name space.
   */
  static int registerProperty(const QString& propNameSpace, const QString& propertyName);
  /** Returns a number that is guaranteed higher or equal to all registered property IDs in
   * the given property name space.
   */
  static int propertyMaxId(const QString& propNameSpace);
  /** \returns true if a property with ID \c propId exists (=has been registered) in the
   * property name space \c propNameSpace .
   *
   * Subclasses may prefer to use the more convenient function \ref propertyIdRegistered(int),
   * which automatically uses the correct property name space.
   * */
  static bool propertyIdRegistered(const QString& propNameSpace, int propId);
  /** \returns true if a property of name \c propertyName exists (=has been registered) in the
   * property name space \c propNameSpace .
   *
   * Subclasses may prefer to use the more convenient function
   * \ref propertyNameRegistered(const QString&),
   * which automatically uses the correct property name space.
   * */
  static bool propertyNameRegistered(const QString& propNameSpace, const QString& propertyName);
  /** \returns the ID corresponding to property name \c propertyName in property name
   * space \c propNameSpace.
   *
   * If the property name space does not exist, or if the property does not exist in that
   * name space, \c -1 is returned (silently).
   *
   * The property name space \c propNameSpace must exist or a warning message is issued.
   *
   * Subclasses may prefer to use the more convenient function
   * \ref propertyIdForName(const QString&),
   * which automatically uses the correct property name space.
   */
  static int propertyIdForName(const QString& propNameSpace, const QString& propertyName);
  /** Reverse operation of propertyIdForName(), with similar behavior.
   *
   * This function silently returns <tt>QString()</tt> on failure.
   *
   * Subclasses may prefer to use the more convenient function \ref propertyNameForId(int),
   * which automatically uses the correct property name space.
   */
  static QString propertyNameForId(const QString& propNameSpace, int propId);
  /** Returns a list of all registered property IDs in the given property name space.
   *
   * Subclasses may prefer to use the more convenient function
   * \ref registeredPropertyIdList(),
   * which automatically uses the correct property name space.
   * */
  static QList<int> registeredPropertyIdList(const QString& propNameSpace);
  /** Returns a list of all registered property names in the given property name space.
   *
   * Subclasses may prefer to use the more convenient function
   * \ref registeredPropertyNameList(),
   * which automatically uses the correct property name space.
   * */
  static QStringList registeredPropertyNameList(const QString& propNameSpace);
  /** Returns a map of all registered properties, with the property names as map keys and
   * the property IDs as map values, in the given property name space.
   *
   * Subclasses may prefer to use the more convenient function \ref registeredProperties(),
   * which automatically uses the correct property name space.
   * */
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

  friend bool operator==(const KLFPropertizedObject& a, const KLFPropertizedObject& b);
};

/** \returns TRUE if all values for all registered properties of this object are equal. */
bool operator==(const KLFPropertizedObject& a, const KLFPropertizedObject& b);

KLF_EXPORT QDataStream& operator<<(QDataStream& stream, const KLFPropertizedObject& obj);
KLF_EXPORT QDataStream& operator>>(QDataStream& stream, KLFPropertizedObject& obj);

KLF_EXPORT QTextStream& operator<<(QTextStream& stream, const KLFPropertizedObject& obj);

KLF_EXPORT QDebug& operator<<(QDebug& stream, const KLFPropertizedObject& obj);




#endif
