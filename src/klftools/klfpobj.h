/***************************************************************************
 *   file klfpobj.h
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




/** \brief An abstract object characterized by properties
 *
 * This class is an interface to get and set property values for objects like
 * KLFPropertizedObject, QObject, QVariantMap-based, which store their information
 * as logical properties.
 *
 * Such objects should either inherit explicitely this class, or should provide
 * an adapter class (eg. as for QObject).
 *
 * This class was developed in the general idea of trying to avoid version issues
 * in data formats, and storing everything as properties in a coherent form, eg. XML.
 * See klfSave() and klfLoad().
 *
 * QDataStream operators are implemented for this object.
 */
class KLF_EXPORT KLFAbstractPropertizedObject
{
public:
  KLFAbstractPropertizedObject();
  virtual ~KLFAbstractPropertizedObject();

  /** \brief A string representing this object type
   *
   * The string should be short, and contain only alphanumeric characters (a good idea
   * is to give the class name here).
   *
   * This is used for example as node name in XML exports.
   */
  virtual QString objectKind() const = 0;

  /** \brief get a property's value
   *
   * \returns the value of the given property, as a QVariant.
   */
  virtual QVariant property(const QString& propName) const = 0;

  /** \brief Queries what property are (or can be) set
   *
   * This function should return a list of property names that have been explicitely
   * set on this object.
   */
  virtual QStringList propertyNameList() const = 0;

  /** \brief Assign a value to a property
   *
   * It is up to the subclasse to implement this function.
   *
   * This function checks that the given property \c pname is allowed to be modified to
   * the new value \c value. If this is not the case, returns FALSE. Otherwise, this
   * function tries to save the new value for the given property.
   *
   * \returns TRUE if the property was successfully saved, FALSE if the property was not
   *   allowed to be modified or if the new value could not be set.
   */
  virtual bool setProperty(const QString& pname, const QVariant& value) = 0;


  /** \brief Convenience function to retrieve all properties
   *
   * This function returns all the properties that have been set on this object with
   * their corresponding values.
   *
   * The default implementation uses propertyNameList() and property() to generate
   * the required return value. It should be enough for most cases. If you really want
   * to however, you can reimplement this function for your needs. */
  virtual QMap<QString,QVariant> allProperties() const;

  /** \brief Convenience function to load a set of property values
   *
   * This function tries to set all the properties given in \c data. If one or more
   * \c setProperty() call failed, then this function returns FALSE. TRUE is returned
   * when all properties were set successfully.
   *
   * The default implementation uses propertyNameList() and setProperty() to set all the
   * given properties.
   *
   * Subclasses may be interested in reimplementing this function in order to garantee
   * the the properties are set in a correct order. This might be necessary in some cases
   * where you are allowed to set a given property depending on the value of another, for
   * example a "Locked" property.
   */
  virtual bool setAllProperties(const QMap<QString,QVariant>& data);

  /** \returns TRUE if a all properties have fixed types, i.e. a given property can only
   * contain a value of a certain type.
   */
  virtual bool hasFixedTypes() const { return false; }

  /** \brief Corresonding type for the given property
   *
   * This function is only relevant if hasFixedTypes() returns TRUE.
   */
  virtual QByteArray typeNameFor(const QString& property) const { Q_UNUSED(property); return QByteArray(); }

  /** \brief A type specification for the given property
   *
   * A type specification is some additional data that specifies further the type. For example,
   * a type KLFEnumType has to be specified to which enum values are allowed.
   *
   * This is only used for types that derive KLFSpecifyableType.
   */
  virtual QByteArray typeSpecificationFor(const QString& property) const { Q_UNUSED(property); return QByteArray(); }
};


class KLF_EXPORT KLFSpecifyableType
{
public:
  KLFSpecifyableType();
  virtual ~KLFSpecifyableType();

  virtual QByteArray specification() const = 0;
  virtual bool setSpecification(const QByteArray& data) = 0;
};


class KLF_EXPORT KLFEnumType : public KLFSpecifyableType
{
  int val;
  QStringList enumVals;
public:
  KLFEnumType(int initvalue = 0) : val(initvalue), enumVals()
  {
  }
  KLFEnumType(const KLFEnumType& copy) : KLFSpecifyableType(), val(copy.val), enumVals(copy.enumVals)
  {
  }
  virtual ~KLFEnumType() { }

  inline int value() const
  {
    return val;
  }
  inline void setValue(int v)
  {
    val = v;
  }

  inline QString enumValue() const
  {
    if (val < 0 || val >= enumVals.size()) {
      klfWarning("Invalid value: "<<val<<" for enum values "<<enumVals) ;
      return QString();
    }
    return enumVals[val];
  }


  inline QStringList enumValues() const
  {
    return enumVals;
  }
  inline void setEnumValues(const QStringList& list)
  {
    enumVals = list;
  }

  QByteArray specification() const
  {
    return enumVals.join(":").toUtf8();
  }

  bool setSpecification(const QByteArray& data)
  {
    // parse enum values
    setEnumValues(QString::fromUtf8(data).split(QRegExp(":")));
    return true;
  }

};


Q_DECLARE_METATYPE(KLFEnumType) ;

KLF_EXPORT QDataStream& operator<<(QDataStream& stream, const KLFEnumType& e);
KLF_EXPORT QDataStream& operator>>(QDataStream& stream, KLFEnumType& e);
// exact matches
inline bool operator==(const KLFEnumType& a, const KLFEnumType& b)
{ return a.value() == b.value(); }




class KLFPObjPropRefHelper;


/** \brief A class that holds properties.
 *
 * This class is meant to be subclassed to create objects that need to store
 * properties. For an example, see KLFLibEntry in the klfapp library.
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
 * Property values may be queried with property(const QString&) const or
 * property(int propId) const , and may be set with \ref setProperty().
 *
 * Subclasses should override setProperty() to add checks as to whether the user is indeed
 * allowed to set the given property to the given value, and then calling the protected
 * doSetProperty() or doLoadProperty() functions. Subclasses may also provide their
 * own public API for setting and/or registering property values, which could directly call
 * setProperty() with the relevant property name.
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
 * all registered properties are stored (the names and IDs, not the values (!)) statically in this
 * object, different subclasses must access different registered properties, and that is what the
 * &quot;property name spaces&quot; are meant for. Subclasses specify once and for all the property
 * name space to the \ref KLFPropertizedObject(const QString&) constructor, then they can forget
 * about it. Other classes than the direct subclass don't have to know about property name spaces.
 *
 * \note For fast access, the property values are stored in a \ref QVector&lt;\ref QVariant&gt;.
 *   The \c propId is just an index in that vector. Keep that in mind before defining static
 *   high property IDs. Since property IDs are not fixed, it is better to have the static property
 *   IDs defined contiguously in an enum.
 */
class KLF_EXPORT KLFPropertizedObject : public KLFAbstractPropertizedObject
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
  /** \brief Get value of a property, with default value
   *
   * Returns the value of the property \c propName, if this property name has been registered and
   * a value set.
   *
   * If the property name has not been registered, or if the property value is not valid
   * (see \ref QVariant::isValid()), then the value \c defaultValue is returned as is.
   *
   * This function does not fail or emit warnings.
   */
  virtual QVariant property(const QString& propName, const QVariant& defaultValue) const;
  /** \brief Tests if a property was set
   *
   * Returns TRUE if the property \c propName was registered and a non-invalid value set; returns
   * FALSE otherwise.
   *
   * This function does not fail or emit warnings. This function calls property(const QString&, const QVariant&).
   */
  virtual bool hasPropertyValue(const QString& propName) const;
  /** \brief Tests if a property was set
   *
   * Returns TRUE if the property \c propId was registered and a non-invalid value set; returns
   * FALSE otherwise.
   *
   * This function does not fail or emit warnings. This function calls hasPropertyValue(const QString&).
   */
  virtual bool hasPropertyValue(int propId) const;

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

  /** \brief Sets the given property to the given value
   *
   * If propname is not registered, this function tries to register the property.
   *
   * Subclasses should reimplement this function to provide tests as to whether the user
   * is allowed to modify the given property with the given value, and then actually
   * set the property and register it if needed.
   *
   * The default implementation just calls doLoadProperty() without any checks.
   *
   * This function is an implementation of the KLFAbstractPropertizedObject::setProperty()
   * pure virtual function. */
  virtual bool setProperty(const QString& propname, const QVariant& value);

  /** \brief Sets the given property to the given value
   *
   * Calls setProperty(const QString&, const QVariant&) with the relevant property
   * name. Thus, in principle, subclasses only need to reimplement the other method.
   */
  virtual bool setProperty(int propId, const QVariant& value);

  /** \brief Initializes properties to given values
   *
   * Clears all properties that have been set and replaces them with those given
   * in argument. Any property name in \c propValues that doesn't exist in registered
   * properties, is registered.
   *
   * This function bypasses all checks by calling loadProperty() directly. Reimplement
   * this function for property name and value checking.
   * */
  virtual bool setAllProperties(const QMap<QString, QVariant>& propValues);


  /** \brief Explicit function name for the simple \c "operator<<".
   * \code
   *   QDataStream stream(...);
   *   pobj.streamInto(stream); // is equivalent to:
   *   stream << pobj;
   * \endcode
   *
   * For more advanced saving/loading techniques, see klfLoad() and klfSave().
   */
  QDataStream& streamInto(QDataStream& stream) const;

  /** \brief Explicit function name for the simple \c "operator>>".
   *
   * Somewhat the opposite of streamInto().
   *
   * For more advanced saving/loading techniques, see klfLoad() and klfSave().
   */
  QDataStream& streamFrom(QDataStream& stream);
  

  /** \brief Saves all the properties in binary form.
   *
   * Basically flushes the output of allProperties() into a QByteArray.
   *
   * For more advanced saving/loading techniques, see klfLoad() and klfSave().
   */
  QByteArray allPropertiesToByteArray() const;

  /** \brief Loads all properties saved by \ref allPropertiesToByteArray()
   *
   * Reads the properties from \c data and calls setAllProperties().
   *
   * For more advanced saving/loading techniques, see klfLoad() and klfSave().
   */
  void setAllPropertiesFromByteArray(const QByteArray& data);

  /*
  / ** \brief Parses a string representation of a property
   *
   * Subclasses should reimplement this function to parse the actual string into a value. Note
   * that this function should NOT set the property, it should just parse the string in a way
   * depending on what kind of information the property \c propId is supposed to store.
   *
   * The formatting is left to the implementation. It could be user input, for example.
   *
   * The default implementation calls \ref parsePropertyValue(const QString&, const QString&) .
   * It is thus enough for subclasses to reimplement the other method.
   * /
   virtual QVariant parsePropertyValue(int propId, const QString& strvalue);

   / ** \brief Parses a string representation of a property
   *
   * Subclasses should reimplement this function to parse the actual string into a value. Note
   * that this function should NOT set the property, it should just parse the string in a way
   * depending on what kind of information the property \c propName is supposed to store.
   *
   * The formatting is left to the implementation. It could be user input, for example.
   *
   * Since the base implementation has no idea what kind of information \c propId is supposed
   * to store, the default implementation returns a null \c QVariant().
   * /
   virtual QVariant parsePropertyValue(const QString& propName, const QString& strvalue);
  */


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
  /** Shortcut for the corresponding (protected) static method propertyMaxId(const QString&).
   * The property name space is automatically detected. */
  int propertyMaxId() const;
  //! See the corresponding protected static method
  /** Shortcut for the corresponding (protected) static method
   * propertyIdRegistered(const QString&, int). The property name space is automatically
   * detected. */
  bool propertyIdRegistered(int propId) const;
  //! See the corresponding protected static method
  /** Shortcut for the corresponding (protected) static method
   * propertyNameRegistered(const QString&, const QString&). The property name space is
   * automatically detected. */
  bool propertyNameRegistered(const QString& propertyName) const;
  //! See the corresponding protected static method
  /** Shortcut for the corresponding (protected) static method
   * propertyIdForName(const QString&, const QString&); the property name space is
   * automatically detected. */
  int propertyIdForName(const QString& propertyName) const;
  //! See the corresponding protected static method
  /** Shortcut for the corresponding (protected) static method
   * propertyNameForId(const QString&, int); the property name space is automatically
   * detected. */
  QString propertyNameForId(int propId) const;
  //! See the corresponding protected static method
  /** Shortcut for the corresponding (protected) static method
   * registeredPropertyIdList(const QString&); the property name space is automatically
   * detected. */
  QList<int> registeredPropertyIdList() const;
  //! See the corresponding protected static method
  /** Shortcut for the corresponding (protected) static method
   * registeredPropertyNameList(const QString); the property name space is automatically
   * detected. */
  QStringList registeredPropertyNameList() const;
  //! See the corresponding protected static method
  /** Shortcut for the corresponding (protected) static method
   * registeredProperties(const QString&);
   * the property name space is automatically detected. */
  QMap<QString, int> registeredProperties() const;

  virtual QString objectKind() const { return pPropNameSpace; }

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
   * this function fails.
   *
   * \returns TRUE for success. */
  virtual bool doSetProperty(const QString& propname, const QVariant& value);

  /** Sets the property identified by propId to the value \c value.
   * Fails if the property \c propId is not registered.
   *
   * \returns TRUE for success. */
  virtual bool doSetProperty(int propId, const QVariant& value);

  /** Like \c doSetProperty(), except the property name \c propname is registered
   * with \ref registerProperty() if it is not registered.
   *
   * \returns The property ID that was set or -1 for failure.
   */
  virtual int doLoadProperty(const QString& propname, const QVariant& value);

  /** shortcut for the corresponding static method. Detects the correct property
   * name space and calls registerBuiltInProperty(const QString&, int, const QString&)
   */
  void registerBuiltInProperty(int propId, const QString& propName) const;

  /** shortcut for the corresponding static method. Detects the correct property
   * name space and calls
   * \ref registerProperty(const QString& propNameSpace, const QString& propName).
   */
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
   * registerBuiltInProperty(int propId, const QString& propName), which automatically uses
   * the correct property name space.
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
   * registerProperty(const QString&) const,
   * which automatically uses the correct property name space.
   */
  static int registerProperty(const QString& propNameSpace, const QString& propertyName);

  /** Returns a number that is guaranteed higher or equal to all registered property IDs in
   * the given property name space.
   *
   * Subclasses may prefer to use the more convenient function
   * propertyMaxId() const, which automatically uses the correct property
   * name space.
   */
  static int propertyMaxId(const QString& propNameSpace);

  /** \returns true if a property with ID \c propId exists (=has been registered) in the
   * property name space \c propNameSpace .
   *
   * Subclasses may prefer to use the more convenient function
   * propertyIdRegistered(int) const, which automatically uses the correct property
   * name space.
   */
  static bool propertyIdRegistered(const QString& propNameSpace, int propId);

  /** \returns true if a property of name \c propertyName exists (=has been registered) in the
   * property name space \c propNameSpace .
   *
   * Subclasses may prefer to use the more convenient function
   * propertyNameRegistered(const QString&) const,
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
   * propertyIdForName(const QString&) const, which automatically uses the correct property
   * name space.
   */
  static int propertyIdForName(const QString& propNameSpace, const QString& propertyName);

  /** Reverse operation of propertyIdForName(), with similar behavior.
   *
   * This function silently returns <tt>QString()</tt> on failure.
   *
   * Subclasses may prefer to use the more convenient function propertyNameForId(int) const,
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


  friend class KLFPObjPropRefHelper;

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

/** \returns TRUE if all values for all registered properties of this object are equal.
 * \relates KLFPropertizedObject
 */
bool operator==(const KLFPropertizedObject& a, const KLFPropertizedObject& b);

/**
 * \note For more advanced saving/loading techniques, see klfLoad() and klfSave().
 */
KLF_EXPORT QDataStream& operator<<(QDataStream& stream, const KLFPropertizedObject& obj);
/**
 * \note For more advanced saving/loading techniques, see klfLoad() and klfSave().
 */
KLF_EXPORT QDataStream& operator>>(QDataStream& stream, KLFPropertizedObject& obj);

KLF_EXPORT QTextStream& operator<<(QTextStream& stream, const KLFPropertizedObject& obj);

KLF_EXPORT QDebug& operator<<(QDebug& stream, const KLFPropertizedObject& obj);


/** \internal
 *
 * Helper class because of friend declaration not liking templates (??)
 */
class KLFPObjPropRefHelper
{
protected:
  void registerbuiltinprop(KLFPropertizedObject *obj, int propid, const QString& pname)
  {
    obj->registerBuiltInProperty(propid, pname);
  }
  QString propertyNameSpace(KLFPropertizedObject *obj) const { return obj->propertyNameSpace(); }
};

template<class T>
class KLF_EXPORT KLFPObjPropRef : private KLFPObjPropRefHelper
{
  KLFPropertizedObject *pPObj;
  int pPropId;
public:
  typedef T Type;

  KLFPObjPropRef(KLFPropertizedObject *pobj, int propId)
    : pPObj(pobj), pPropId(propId)
  {
  }
  KLFPObjPropRef(const KLFPObjPropRef& other)
    : pPObj(other.pPObj), pPropId(other.pPropId)
  {
  }
  /** Constructs the object and automatically registers the given property in \c pobj
   * as a built-in property.
   *
   * \warning Should ONLY be used by KLFPropertizedObject subclasses in their constructor,
   *   for KLFPObjPropRef objects that are declared as members.
   */
  KLFPObjPropRef(KLFPropertizedObject *pobj, int builtInPropId, const QString& pname)
    : pPObj(pobj), pPropId(builtInPropId)
  {
    init(pname);
  }
  /** Constructs the object and automatically registers the given property in \c pobj
   * as a built-in property. Initializes the property's value to \c value.
   *
   * \warning Should ONLY be used by KLFPropertizedObject subclasses in their constructor,
   *   for KLFPObjPropRef objects that are declared as members.
   */
  KLFPObjPropRef(KLFPropertizedObject *pobj, int builtInPropId, const QString& pname, const T& value)
    : pPObj(pobj), pPropId(builtInPropId)
  {
    init(pname);
    set(value);
  }

  operator QVariant() const
  {
    return variantValue();
  }
  operator T() const
  {
    return value<T>();
  }
  T operator ()() const
  {
    return value<T>();
  }
  const KLFPObjPropRef& operator=(const QVariant& v)
  {
    pPObj->setProperty(pPropId, v);
    return *this;
  }
  const KLFPObjPropRef& operator=(const T& value)
  {
    pPObj->setProperty(pPropId, QVariant::fromValue<T>(value));
    return *this;
  }
  /** \note This assigns the value; it does NOT change the property reference! */
  const KLFPObjPropRef& operator=(const KLFPObjPropRef& value)
  {
    return this->operator=(value.value());
  }

  QVariant variantValue() const
  {
    return pPObj->property(pPropId);
  }

  T value() const
  {
    return value<T>();
  }

  template<class VariantType>
  T value() const
  {
    QVariant v = pPObj->property(pPropId);
    return T(v.value<VariantType>());
  }

  template<class VariantType>
  void set(const T& value)
  {
    pPObj->setProperty(pPropId, QVariant::fromValue<VariantType>(value));
  }
  void set(const T& value)
  {
    set<T>(value);
  }

  template<class VariantType>
  bool equals(const KLFPObjPropRef& other) const
  {
    return (value<VariantType>() == other.value<VariantType>());
  }
  bool equals(const KLFPObjPropRef& other) const
  {
    return equals<T>(other);
  }

  bool operator==(const T& val) const
  {
    return (value() == val);
  }
  bool operator==(const KLFPObjPropRef& other) const
  {
    return (value() == other.value());
  }

private:
  void init(const QString& pname)
  {
    if (!pPObj->propertyIdRegistered(pPropId)) {
      // from our helper base class
      registerbuiltinprop(pPObj, pPropId, pname);
    } else {
      // make sure the correct name was registered
      KLF_ASSERT_CONDITION(pPObj->propertyNameForId(pPropId) == pname,
			   qPrintable(propertyNameSpace(pPObj))<<": Built-In property ID "<<pPropId
			   <<" does not have name "<<pname<<" !",
			   ; ) ;
    }
  }

};



// ----

/** \internal */
class KLFPObjRegisteredType {
public:
  KLFPObjRegisteredType(const char *name)
  {
    doregister(Register, name);
  }

  static bool isRegistered(const char *name)
  {
    return doregister(Query, name);
  }

private:
  enum Action { Query, Register };
  static int doregister(Action action, const char * name)
  {
    static QList<QByteArray> registered_types;
    bool x;
    switch (action) {
    case Query: // is querying the existance of a registered type
      x = registered_types.contains(QByteArray(name));
      return x;
    case Register: // want to register the given type
      registered_types.append(QByteArray(name));
      return 0;
    default:
      fprintf(stderr, "ERRORORROOERROR: %s: what is your action?? `%d' for name `%s'\n",
	      KLF_FUNC_NAME, (int)action, name);
    }
    return -1;
  }
};

/** Put this in the .cpp for the given type.
 *
 * \todo ###: Which type? any abstractobj or just klfproperitzedobject?
 */
#define KLF_DECLARE_POBJ_TYPE(TYPE)					\
  static KLFPObjRegisteredType __klf_pobj_regtype_##TYPE = KLFPObjRegisteredType(#TYPE) ;

/** \internal */
class KLFSpecifyableRegisteredType {
public:
  KLFSpecifyableRegisteredType(const char *name)
  {
    doregister(Register, name);
  }

  static bool isRegistered(const char *name)
  {
    return doregister(Query, name);
  }

private:
  enum Action { Query, Register };
  static int doregister(Action action, const char * name)
  {
    static QList<QByteArray> registered_types;
    bool x;
    switch (action) {
    case Query: // is querying the existance of a registered type
      x = registered_types.contains(QByteArray(name));
      return x;
    case Register: // want to register the given type
      registered_types.append(QByteArray(name));
      return 0;
    default:
      fprintf(stderr, "ERRORORROORORR: %s: what is your action?? `%d' for name `%s'\n",
	      KLF_FUNC_NAME, (int)action, name);
    }
    return -1;
  }
};

/** Put this in the .cpp for the given type */
#define KLF_DECLARE_SPECIFYABLE_TYPE(TYPE)					\
  static KLFSpecifyableRegisteredType __klf_specifyable_regtype_##TYPE = KLFSpecifyableRegisteredType(#TYPE) ;



#endif
