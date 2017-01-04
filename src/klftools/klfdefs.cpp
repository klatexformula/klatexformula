/***************************************************************************
 *   file klfdefs.cpp
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

#include <stdlib.h>
#include <stdio.h> // vsprintf(), vsnprintf
#include <stdarg.h> // va_list, va_arg, va_end in klfFmt()
#include <sys/time.h>

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QRegExp>
#include <QApplication>
#include <QMetaObject>
#include <QDebug>
#include <QDateTime>

#include "klfdefs.h"

/** \file klfdefs.h
 * \brief Base declarations for klatexformula and some utilities
 *
 */

/** \file klfdebug.h
 * \brief Debugging utilities
 *
 */

/* * This is not a bug, it's cosmetic... \ bug Remove Qt3 support w/ KLFBACKEND_QT4 in all files */


/** \fn int klfVersion()
 *
 * Returns the current version of the KLatexFormula library, given as a string,
 * eg. \c "3.2.1".
 *
 * For non-release builds, this may have a suffix, eg. \c "3.2.0beta2".
 */
/** \fn int klfVersionMaj()
 *
 * Returns the current major version of the KLatexFormula library.
 *
 * For example, if the version is \c "3.2.0", klfVersionMaj() returns \c 3.
 */
/** \fn int klfVersionMin()
 *
 * Returns the current minor version of the KLatexFormula library.
 *
 * For example, if the version is \c "3.2.0", klfVersionMin() returns \c 2.
 */
/** \fn int klfVersionRelease()
 *
 * Returns the current release version of the KLatexFormula library.
 *
 * For example, if the version is \c "3.2.0", klfVersionRelease() returns \c 0.
 */

/** \fn KLF_EXPORT QByteArray klfShortFuncSignature(const char *fullFuncName)
 *
 * Takes as input a funcion signature like
 *  <pre>void MyClass::functionName(const QString& arg1, int arg2) const</pre>
 * and outputs a short form of it, like
 *  <pre>MyClass::functionName</pre>
 */

/** \fn KLF_EXPORT QByteArray klfFmt(const char *fmt, ...)
 *
 * Formats a printf-style string and returns the data as a QByteArray.
 * \warning the final string length must not exceed 8192 bytes, the size of the
 * internal buffer. */

/** \def klfFmtCC
 * \hideinitializer
 *
 * Same as klfFmt(), but returns the formatted string as a <tt>const char*</tt>
 * C-style string.
 *
 * Example:
 * \code
 *  unsigned int flags = ... ;
 *  qDebug()<<"Flags are: "<<klfFmtCC("%#010x", flags) ;
 * \endcode
 *
 * Used in a QDebug stream, this function has the advantage (over klfFmt()) of not
 * having its value enclosed in quotes.
 *
 * <i><b>Note for advanced usage</b><br>
 * This macro, when called as <tt>klfFmtCC (format, args...)</tt> expands to
 * \code  (const char*)klfFmt (format, args...)  \endcode
 * Which means that if you are trying to do something (quite unorthodox) like:
 * \code
 *   cout<<"3rd digit of 280 in base 8 is: "<< klfFmtCC("%o", 280)[2]; // does not compile
 * \endcode
 * then it will fail at compile-time, since the <tt>const char*</tt> cast is evaluated
 * after the <tt>operator[]</tt>. The working example is:
 * \code
 *   cout<<"3rd digit of 280 in base 8 is: "<< (klfFmtCC("%o", 280))[2]; // correct
 * \endcode
 * This macro had to be declared without any arguments, since C preprocessor macros
 * don't support variable number of arguments (as required by printf-style formatting).
 * This is the reason why I couldn't automatically fit in the extra required parenthesis
 * in the macro definition.</i>
 */

/** \fn KLF_EXPORT QByteArray klfFmt(const char * fmt, va_list pp)
 *
 * Implements \ref klfFmt(const char *, ...) functionality, but with
 * a \c va_list argument pointer for use in vsprintf().
 */

/** \fn KLF_EXPORT QString klfTimeOfDay(bool shortFmt = true)
 *
 * Returns something like <tt>456.234589</tt> (in a <tt>QString</tt>) that
 * represents the actual time in seconds from midnight.
 *
 * if \c shortFmt is TRUE, then the seconds are truncated to 3 digits (ie. seconds
 * are given modulo 1000). In this case, the absolute time reference is undefined, but
 * stays always the same along the program execution (useful for debug messages
 * and timing blocks of code).
 *
 * Otherwise if \c shortFmt is FALSE, the full second and micro-second count output of
 * gettimeofday() is given.
 */

/** \class KLFDebugBlock
 *
 * \brief Utility to time the execution of a block
 *
 * Prints message in constructor and in destructor to test
 * block execution time.
 *
 * Consider using the macros \ref KLF_DEBUG_BLOCK and \ref KLF_DEBUG_TIME_BLOCK instead.
 */

/** \class KLFDebugBlockTimer
 *
 * \brief An extension of KLFDebugBlock with millisecond-time display
 *
 * Consider the use of \ref KLF_DEBUG_BLOCK and \ref KLF_DEBUG_TIME_BLOCK macros instead.
 */


/** \def KLF_DEBUG_DECLARE_REF_INSTANCE
 * \hideinitializer
 *
 * \brief Declares a 'ref-instance' identifier for identifying insances in debug output
 *
 * Useful for debugging classes that are instanciated multiple times, and that may superpose
 * debugging output.
 *
 * Use this macro in the class declaration and specify an expression that will identify the instance.
 * Then this information will be displayed in the debug message when using \ref klfDbg().
 *
 * The expression given as argument to this macro is any valid expression that can be used within a
 * const member function---you may reference eg. private properties, private functions, inherited
 * protected members, public functions, etc. The expression should evaluate to a QString, or any
 * expression that can be added (with <tt>operator+(QString,...)</tt>) to a QString.
 *
 * Example:
 * \code
 * class MyDocument {
 * public:
 *   MyDocument(QString fname) : fileName(fname) { }
 *   // ....
 *   QString currentFileName() const { return fileName; }
 *   // ...
 * private:
 *   QString fileName;
 *   KLF_DEBUG_DECLARE_REF_INSTANCE( currentFileName() ) ;
 *   //  we could also have used (equivalent):
 *   //KLF_DEBUG_DECLARE_REF_INSTANCE( fileName ) ;
 * };
 * \endcode
 *
 * This macro expands to a protected inline member returning the given expression surrounded with
 * square brackets.
 *
 * \note this feature is optional. Classes that do not declare a 'ref-instance' will still be able
 *   to use \c klfDbg() normally, except there is no way (apart from what you output yourself) to
 *   make the difference between messages originating from two different class instances.
 *
 * \warning when you declare a 'ref-instance' in a class, static members of that class cannot use
 *  \c klfDbg any more because that macro will try to invoke the ref-instance of the class. Use
 *  instead \ref klfDbgSt which does the same thing, but explicitely skips the ref-instance. This is not
 *  needed for classes that do not declare a ref-instance.
 */

/** \def KLF_DEBUG_DECLARE_ASSIGNABLE_REF_INSTANCE
 * \hideinitializer
 * \brief Declare that this class has an assignable debugging ref-instance
 *
 * This macro should be used within class declarations. If the KLF_DEBUG symbol is defined, this macro
 * will declare a ref-instance for that class, which other objects may assign with
 * \ref KLF_DEBUG_ASSIGN_REF_INSTANCE().
 *
 * See also \ref KLF_DEBUG_DECLARE_REF_INSTANCE and \ref KLF_DEBUG_ASSIGN_REF_INSTANCE(). See the latter
 * for an example usage.
 */

/** \def KLF_DEBUG_ASSIGN_REF_INSTANCE
 * \hideinitializer
 * \brief Assign a debugging ref-instance to a class instance
 *
 * The class declaration must contain a call to the \ref KLF_DEBUG_DECLARE_ASSIGNABLE_REF_INSTANCE() macro.
 *
 * Example usage:
 * \code
 * // vehicle.h
 * class Vehicle {
 * public:
 *   Vehicle(QString xmlspecsfile) { loadFromXML(xmlspecsfile);  }
 *   ...
 *   double calculate_drag_force(double speed);
 * private:
 *   void loadFromXML(const QString& fname);
 *   ...
 *   KLF_DEBUG_DECLARE_ASSIGNABLE_REF_INSTANCE()
 * }
 * // vehicle.cpp
 * void Vehicle::loadFromXML()
 * {
 *   // ... load vehicle data and parameters from XML file
 * }
 * void Vehicle::calculate_drag_force(double speed)
 * {
 *   klfDbg("shape-factor="<<...<<", speed="<<speed ) ;
 *   ...
 *   return drag_force;
 * }
 * // main.cpp
 * int main()
 * {
 *   ...
 *   Vehicle alpha("vehiclespecs/alpha-romeo.xml");
 *   KLF_DEBUG_ASSIGN_REF_INSTANCE(&alpha, "car/Alpha-Romeo");
 *   Vehicle f16("vehiclespecs/f16.xml");
 *   KLF_DEBUG_ASSIGN_REF_INSTANCE(&f16, "fighter/F-16");
 *   ...
 *   double drag_at_210kmh = alpha.calculate_drag_force(210);  // will produce debugging output
 * }
 * \endcode
 * This program will produce debugging output at the last line marked above:
 * <pre>Vehicle::calculate_drag_force():<b>[car/Alpha-Romeo]</b>
 *        node=[kind=1, children/sz=0,allfetched=false]</pre>
 * Note that the ref-instance is the one assigned with KLF_DEBUG_ASSIGN_REF_INSTANCE().
 */

/** \def KLF_DEBUG_ASSIGN_SAME_REF_INSTANCE(object)
 * \hideinitializer
 * \brief Assigns to the given object the same ref-instance as the current object instance
 *
 * This macro works very much like KLF_DEBUG_ASSIGN_REF_INSTANCE(), but instead of assigning
 * a given string as ref-instance, it assigns the ref-instance from the current object instance
 * it is being called from.
 *
 * Note that the ref-instance string is copied at the time of calling this macro. In other words:
 *  - if the current object has itself an assignable ref-instance, and the ref-instance of the
 *    current object is not yet set, then \c object will be set an empty ref-instance
 *  - if the ref-instance of the current class instance changes, then the second class will not
 *    update its ref-instance.
 *
 * Example usage: in <tt>src/klflibview.cpp</tt>, in KLFLibDefaultView, when the corresponding
 * view model is created, it is assigned the same ref-instance to the view object:
 * \code
 * KLFLibDefaultView::setResource(...)  : ...
 * {
 *   ...
 *   pModel = new KLFLibModel(...);
 *   KLF_DEBUG_ASSIGN_SAME_REF_INSTANCE(pModel) ;
 *   ...
 * }
 * \endcode
 * Debugging messages sent from the KLFLibModel object will be tagged with the same ref-instance
 * string as the messages sent from the KLFLibDefaultView object.
 */


/** \def KLF_DEBUG_TIME_BLOCK
 * \hideinitializer
 *
 * \brief Utility to time the execution of a block
 *
 * This macro behaves exactly like \ref KLF_DEBUG_BLOCK, but also prints the current time (short format) for
 * timing the execution of the given block.
 *
 * The effective internal difference between KLF_DEBUG_TIME_BLOCK and KLF_DEBUG_BLOCK is the use of the
 * helper class KLFDebugBlockTimer instead of KLFDebugBlock.
 *
 * \note KLF_DEBUG needs to be defined at compile-time
 *   to enable this feature. Otherwise, this maco is a no-op.
 *
 * Usage example:
 * \code
 * void do_something() {
 *   KLF_DEBUG_TIME_BLOCK(KLF_FUNC_NAME) ;
 *   ... // some lengthy operation
 *   ... if (failed) return; // for example
 *   ... // no special instruction needed at end of block
 * }
 * \endcode
 */

/** \def KLF_DEBUG_BLOCK
 * \hideinitializer
 *
 * \brief Utility to debug the execution of a block
 *
 * Prints msg with \c ": block begin" when this macro is called, and prints msg with \c ": block end"
 * at the end of the block. The end of the block is detected by creating an object on the stack
 * and printing the message in that object's destructor.
 *
 * \note KLF_DEBUG needs to be defined at compile-time to enable this feature. Otherwise, this macro
 *   is a no-op.
 *
 * This macro accepts a QString.
 *
 * Usage example:
 * \code
 * void do_something() {
 *   KLF_DEBUG_BLOCK("block do_something() execution") ;
 *   ... // do something
 *   if (failed) { // for example
 *     KLF_DEBUG_BLOCK(QString("%1: failed if-block").arg(KLF_FUNC_NAME)) ;
 *     ... // more fail treatment...
 *     return;   // both end block messages will be printed here automatically
 *   }
 *   if (go_no_further)
 *     return;   // do_something() end block msg will be printed here automatically
 *   ...
 *   // no special instruction needed at end of block
 * }
 * \endcode
 *
 * \note \c msg is (macro-expanded) added to a <tt>QString("")</tt>, so you can write something like
 * \code
 *   KLF_DEBUG_BLOCK(KLF_FUNC_NAME+"(int,void*)") ;
 * \endcode
 * (the \c '+' is understood well) instead of having to write
 * \code
 *   KLF_DEBUG_BLOCK(QString("")+KLF_FUNC_NAME+"(int,void*)") ;
 * \endcode
 * even though <tt>KLF_FUNC_NAME</tt> expands into a <tt>const char*</tt>.
 *
 * Advanced: If that last note wasn't clear, consider that this macro expands to:
 * \code
 *   KLFDebugBlock __klf_debug_block(QString("")+msg)
 * \endcode
 * where \c KLFDebugBlock is a helper class and \c __klf_debug_block the name of an instance of that
 * class created on the stack. \c msg is passed as argument to the constructor, added to a QString(""),
 * which allows for the feature explained above, using the fact that the preprocessor replaces the macro
 * argument in full text, and the fact that the \c '+' operator operates from left to right.
 *
 * \warning This is a macro that expands its text without explicitely protecting it (this allows you to
 *   do what the above note said).
 */

/** \def KLF_DEBUG_TEE
 * \hideinitializer
 * \brief Print the value of expression and return it
 *
 * If KLF_DEBUG preprocessor symbol is not defined, this macro just expands to <tt>(expr)</tt>.
 *
 * \note This macro works only in Qt4.
 *
 * Very useful for debugging return values, eg.
 * \code
 *   return KLF_DEBUG_TEE(result);
 * \endcode
 * effectively does
 * \code
 *   return result;
 * \endcode
 * however printing the result while returning it.
 */

/** \def klfDebugf
 * \hideinitializer
 *
 * \brief print debug stream items
 *
 * This macro expands to <tt>qDebug(msg, args...)</tt> if KLF_DEBUG is defined, or otherwise expands
 * to nothing.
 *
 * The usage is a bit special, to honor many-argument processing:
 * \code
 * klfDebugf(("%s: this is a debug message. value=%d", KLF_FUNC_NAME, somevalue)) ;
 * \endcode
 * Note the double-parenthesis.
 */

/** \def klfDbg
 * \hideinitializer
 *
 * \brief print debug stream items
 *
 * \warning This macro may be counter-intuitive, as the argument is not evaluated before expansion.
 *   This macro expands more or less to:
 *   \code qDebug()<<KLF_FUNC_NAME [possibly more info, eg ref-instance] <<": "<< streamableItems \endcode
 *   which means that you can list any QDebug-streamable items as macro arguments, separated by a
 *   <tt>&lt;&lt;</tt> operator, keeping in mind that these stream operators will be "seen" only
 *   AFTER the macro has expanded. Example usage:
 *   \code
 * int index = ...; QString mode = ...;
 * klfDbg( "index is "<<index<<" and mode is "<<mode ) ; \endcode
 *
 * The advantage of this syntax is that when disabling debug, the parts in your code where you
 * call this macro are truly "erased" (the macro itself, with all its arguments, expands to
 * nothing), and result into no overhead of having to go eg. through null-streams. Additionally,
 * all macro arguments are NOT evaluated in non-debug mode.
 */

/** \def klfDbgT
 * \hideinitializer
 *
 * \brief print debug stream items, with current time
 *
 * Same as klfDbg(), but also prints current time given by KLF_SHORT_TIME.
 */

/** \def klfDbgSt
 * \hideinitializer
 *
 * \brief print debug stream items (special case)
 *
 * Like klfDbg(), but for use in static functions in classes for which you have declared a 'ref-instance'
 * with \ref KLF_DEBUG_DECLARE_REF_INSTANCE().
 *
 * If you get compilation errors like '<tt>cannot call memeber function ...::__klf_debug_ref_instance() const
 * without object</tt>' then it is likely that you should use this macro instead.
 *
 * Explanation: ref instance works by declaring a protected inline const member in classes. A function with
 * the same name exists globally, returning an empty string, such that classes that do not declare a 
 * ref-instance may use klfDbg() which sees the global no-op ref-instance function. However, static members
 * of classes that declare a ref-instance see the class' ref-instance function, which evidently cannot
 * be called from a static member. Use this macro in that last case, that simply bypasses the ref-instance
 * call (anyway you won't need it in a static function !).
 */

/** \def klfDbgStT
 * \hideinitializer
 *
 * \brief print debug stream items, with current time (special case)
 *
 * Same as klfDbgSt(), but prints also the time like klfDbgT() does.
 */

/** \def klfWarning
 * \hideinitializer
 *
 * Emit a warning to the standard error. Will do so regardless of whether KLF_DEBUG is defined or not.
 * Its usage is identical to \ref klfDbg:
 * \code
 *   int some_function(int x)
 *   {
 *     if (x < 0) {
 *       klfWarning("Expected positive x; I was given x="<<x) ;
 *     }
 *   }
 * \endcode
 */

/** \def KLF_FUNC_SINGLE_RUN
 * \hideinitializer
 * \brief Simple test for one-time-run functions
 *
 * Usage example:
 * \code
 * void init_load_stuff()
 * {
 *   KLF_FUNC_SINGLE_RUN ;
 *   // ... eg. load some initializing data in a static structure ...
 *   // this operation will only be performed the first time that init_load_stuff() is called.
 *   // ...
 * }
 * \endcode
 */


/** \def KLF_DECLARE_PRIVATE(ClassName)
 * \hideinitializer
 *
 * To be used inside a class declaration. Declares a private member pointer \c d as type
 * <tt><i>ClassName</i>Private *</tt>.
 *
 * Minimal Example:
 * \code
 *  // myclass.h
 *  // ...
 *  class MyClassPrivate;
 *  class MyClass {
 *  public:
 *    MyClass(...);
 *    ~MyClass(...);
 *    // ...
 *    some_method();
 *    // ...
 *    KLF_DECLARE_PRIVATE(MyClass) ;
 *  }
 *
 *  // myclass.cpp
 *  struct MyClassPrivate {
 *    KLF_PRIVATE_HEAD(MyClass) {
 *      // initialize our properties, this is the constructor
 *      someproperty = false;
 *    }
 *
 *    bool someproperty;
 *
 *    void somelocalmethod() {
 *      // can access MyClass instance with the 'K' pointer object
 *      K->some_method();
 *    }
 *  }
 *
 *  MyClass::MyClass()
 *  {
 *    KLF_INIT_PRIVATE(MyClass) ;    
 *  }
 *  MyClass::~MyClass()
 *  {
 *    KLF_DELETE_PRIVATE ;
 *  }
 *  MyClass::some_method()
 *  {
 *    // may access the private class as 'd' pointer
 *    do_something_with(d->someproperty);
 *  }
 *  // ...
 * \endcode
 *
 * See also \ref KLF_PRIVATE_HEAD, \ref KLF_INIT_PRIVATE, \ref KLF_DELETE_PRIVATE.
 */

/** \def KLF_PRIVATE_HEAD
 * \hideinitializer
 *
 * Declare the first lines with constructor of the private struct/class. This declaration
 * must be followed by a pair of braces containing constructor content, like shown
 * in the example given for \ref KLF_DECLARE_PRIVATE.
 *
 * This method makes sure a local pointer named <tt>K</tt> points back to the main class.
 */

/** \def KLF_PRIVATE_QOBJ_HEAD
 * \hideinitializer
 *
 * Same as \ref KLF_PRIVATE_HEAD, except use this if your private class is a QObject subclass, and
 * you want to initialize it with the parent being the main class instance. The pointer <tt>K</tt>
 * is also initialized. You still have to manually include the Q_OBJECT macro into your definition.
 */

/** \def KLF_INIT_PRIVATE
 * \hideinitializer
 *
 * Use this in the constructor of your main class to initialize the \c d pointer. See example
 * given in \ref KLF_DECLARE_PRIVATE.
 */

/** \def KLF_DELETE_PRIVATE
 * \hideinitializer
 *
 * Use this in the destructor of your main class to delete the \c d private object. See example
 * given in \ref KLF_DECLARE_PRIVATE.
 *
 * \note this is not needed if your private class object is a QObject-subclass (not as in the
 *   example), and you used KLF_PRIVATE_QOBJ_HEAD.
 */


/* -- * ------- NOT PROVIDED. NOT NICE. -------------------
  \def KLF_PROPERTY_GET
  \hideinitializer
 
  \bug DOES THIS MACRO PREVENT DOXYGEN DOCUMENTATION ?
 
  Declares a functions, one public designed to get a property value. This macro is to be used
  within a class declaration.
 
  Useful in conjunction with Q_PROPERTY declarations.
 
  This macro expands to
  \code 
    public: <type and propertyName>() const;
  \endcode
 
  \param type_and_prop is the type and the name of the property, separated by space
 
  Possible definitions of this functions are provided with the macros \ref KLF_DEFINE_PROPERTY_GET
  and \ref KLF_DEFINE_PROPERTY_GETSET.
*/

/** \def KLF_DEFINE_PROPERTY_GET
 * \hideinitializer
 *
 * Use this to define a getter function (whose prototype matches one declared by
 * \ref KLF_PROPERTY_GETSET). The property value is assumed to be stored
 * in a Private class instance as declared by KLF_DECLARE_PRIVATE and friends. The
 * member name in the Private class is assumed to be exactly the property name.
 *
 * This macro expands to
 * \code
 *  <type> ClassName::<propertyName>() const { return d-><propertyName>; }
 * \endcode
 */
/** \def KLF_DEFINE_PROPERTY_GETSET
 * \hideinitializer
 *
 * Same as \ref KLF_DEFINE_PROPERTY_GET, but also defines a setter function.
 *
 * This macro expands to
 * \code
 *  <type> ClassName::<propertyName>() const { return d-><propertyName>; }
 *  void ClassName::set<PropertyName>(<type> value) { d-><propertyName> = value; }
 * \endcode
 *
 * See also \ref KLF_DEFINE_PROPERTY_GETSET_C.
 */

/** \def KLF_DEFINE_PROPERTY_GETSET_C
 * \hideinitializer
 *
 * Same as \ref KLF_DEFINE_PROPERTY_GETSET, but defines the setter with a <tt>const &lt;type&gt;&</tt>
 * argument. Use this with properties of class type.
 */



/** \def KLF_FUNC_NAME
 * \hideinitializer
 * This macro expands to the function name this macro is called in.
 *
 * The header <tt>klfdefs.h</tt> will try to determine which of the symbols
 * <tt>__PRETTY_FUNCTION__</tt>, <tt>__FUNCTION__</tt> and/or <tt>__func__</tt> are defined
 * and use the most "pretty" one, formatting it as necessary with klfShortFuncSignature().
 */

/** \def KLF_ASSERT_NOT_NULL
 * \hideinitializer
 * \brief Asserting Non-NULL pointers (NON-FATAL)
 *
 * This macro is equivalent to
 * \code
 *  KLF_ASSERT_CONDITION(ptr != NULL, message, failaction) ;
 * \endcode
 *
 * If the given pointer \c ptr is NULL, then prints function name and the given message \c msg to
 * standard warning output (using Qt's qWarning()) and executes instructions given by \c failaction.
 *
 * On Qt 4, \c msg may contain << operators to chain output to a QDebug.
 *
 * On Qt 3, \c msg may be anything that can be added with \c '+' to a QString.
 *
 * See \ref KLF_ASSERT_CONDITION for more information.
 */

/** \def KLF_ASSERT_CONDITION
 * \hideinitializer
 * \brief Asserting Conditions (NON-FATAL)
 *
 * If the given expression \c expr is FALSE, then prints function name and the given message \c msg to
 * standard warning output (using Qt's qWarning()) and executes instructions given by \c failaction.
 *
 * \c failaction is any C/C++ instructions that you can place inside a normal code block (in this
 * case the code will be contained in an 'if' block). The allowed instructions include for example
 * \c 'return', \c 'continue' or \c 'break' instructions.
 *
 * On Qt 4, \c msg may contain <tt>&lt;&lt;</tt> operators to chain output to a QDebug.
 *
 * On Qt 3, \c msg may be anything that can be added with \c '+' to a QString.
 *
 * A helper macro for asserting non-NULL pointers is also defined as \ref KLF_ASSERT_NOT_NULL. It directly
 * calls KLF_ASSERT_CONDITION with the expression <tt>(ptr) != NULL</tt>.
 *
 * Example:
 * \code
 *  bool static_has_failed = false;
 *  void * find_whatever_pointer(SomeContainer c, const char * querystring) {
 *    int i = ... // find 'querystring' in the container 'c'
 *    KLF_ASSERT_CONDITION( i >= 0 && i < c.size() ,
 *                          "Can't find string "<<querystring<<"!" ,  // Assuming Qt 4 for << streaming
 *                          static_has_failed = true;  return NULL;  )
 *    ...
 *  }
 *  QString get_myobject_name(MyObject *object) {
 *    KLF_ASSERT_NOT_NULL( object , "object is NULL!" , return QString() ) ;
 *    return object->name;
 *  }
 * \endcode
 *
 * Note Qt4 is needed to use the streaming operators as in the first asserting condition above. With
 * Qt 3, one has to use QString's '+' operator:
 * \code
 *  KLF_ASSERT_CONDITION( ... , "Can't find string "+querystring+"!" , ... );
 * \endcode
 * Which works since on Qt 3, the \c msg argument text is expanded after a <tt>QString("")+</tt>.
 *
 * This macro is not affected by the KLF_DEBUG symbol. This macro is always defined and functional.
 */

/** \def KLF_ASSERT_CONDITION_ELSE
 * \hideinitializer
 * \brief Asserting Conditions previous to block (NON-FATAL)
 *
 * Similar to \ref KLF_ASSERT_CONDITION, but expects a block of actions to perform if indeed
 * the condition is met. An example:
 * \code
 * class MyObject {
 *   MyTarget *pTarget;
 *   int otherfield;
 *   ...
 *
 *   void reinitialize();
 * };
 *
 * void MyObject::reinitialize()
 * {
 *   ...
 *   // if the condition is not met, print the message, and don't attempt to dereference
 *   // the pointer, but proceed in this function, eg. assign `otherfield' etc.
 *   KLF_ASSERT_CONDITION_ELSE( pTarget != NULL , "Warning n != 0 !!" , )
 *   {
 *     // for example...
 *     pTarget->reinitializeTarget();
 *   }
 *
 *   otherfield = ...;
 *   ...
 * }
 * \endcode
 */




/** \fn KLF_EXPORT int klfVersionCompare(const QString& v1, const QString& v2)
 *
 * \brief Compares two version strings
 *
 * \c v1 and \c v2 must be of the form \c "<MAJ>.<MIN>.<REL><suffix>" or \c "<MAJ>.<MIN>.<REL>"
 * or \c "<MAJ>.<MIN>" or \c "<MAJ>" or an empty string.
 *
 * Empty strings are considered less than any other version string, except to other empty strings
 * to which they compare equal.
 *
 * \returns a negative value if v1 < v2, \c 0 if v1 == v2 and a positive value if v2 < v1. This
 *   function returns \c -200 if either of the version strings are invalid.
 *
 * A less specific version number is considered as less than a more specific version number of
 * equal common numbers, eg. "3.1" < "3.1.2".
 *
 * When a suffix is appended to the version, it is attempted to be recognized:
 *  - "alpha" or "alphaN" is alpha version, eg. "3.1.1alpha2" < "3.1.1.alpha5" < "3.1.1"
 *  - "dev" is INTERNAL versioning, should not be published, it means further development after
 *    the given version number; for the next release, a higher version number has to be
 *    decided upon.
 *  - unrecognized suffixes are compared lexicographically, case sensitive.
 *  - after any non-empty suffix, you can add an optional integer number, which will be taken
 *    into account when comparing same version numbers with same suffix words, see examples
 *    below.
 *
 * The full list of recognized suffixes is, in order from "least" to "most" recent version:
 *  - \c "a"
 *  - \c "alpha"
 *  - \c "b"
 *  - \c "beta"
 *  - \c "p"
 *  - \c "pre"
 *  - \c "preview"
 *  - \c "RC"
 *  - \c "rc"
 *  - \c "" ( version number specified without prefix)
 *  - \c "post"
 *  - \c "dev"
 *  - \c "devel"
 *
 * Some examples, where "<" represents a logical "less than", characterized by this function
 * returning a strictly negative value when called with both arguments:
 * <pre>   "3.1.0" < "3.1.2"
 *   "2" < "2.1" < "2.1.1"
 *   "3.0.0alpha2" < "3.0.0"
 *   "3.0.2" < "3.0.3alpha0"
 *   "3.2.0alpha" < "3.2.0beta"
 *   "3.2.0alpha" < "3.2.0alpha0"
 *   "3.2.0RC3" < "3.2.0RC4"
 * </pre>
 *
 * This function, when exchanging the arguments, returns a value that is of opposite sign, ie.
 * \code
 *  klfVersionCompare(v1, v2)  ==  - klfVersionCompare(v2, v1)
 * \endcode
 */

/** \namespace KLFSysInfo
 * \brief Utilities to get system information
 */

/** \enum KLFSysInfo::Os
 * \brief List of known operating systems
 */
/** \var KLFSysInfo::Os KLFSysInfo::Linux
 * \brief systems on which Q_OS_LINUX is defined
 */
/** \var KLFSysInfo::Os KLFSysInfo::Win32
 * \brief systems on which Q_OS_WIN32 is defined
 */
/** \var KLFSysInfo::Os KLFSysInfo::MacOsX
 * \brief systems on which Q_OS_DARWIN is defined
 */
/** \var KLFSysInfo::Os KLFSysInfo::OtherOs
 * \brief systems on which neither Q_OS_{LINUX|WIN32|DARWIN} is defined
 */

/** \fn inline int KLFSysInfo::sizeofVoidStar()
 * \brief the processor register size.
 *
 * Typically 4 or 8 to indicate a 32-bit or a 64-bit system, respectively.
 */

/** \fn KLF_EXPORT QString KLFSysInfo::arch()
 * \brief The architecture of this sytem
 *
 * \returns The current running system architecture.
 */
/** \fn KLF_EXPORT KLFSysInfo::Os KLFSysInfo::os()
 * \brief Which operating system this system is running
 *
 * \returns one of the values of the \ref Os enum.
 */
/** \fn KLF_EXPORT QString KLFSysInfo::osString(KLFSysInfo::Os sysos = os())
 * \brief The operating system we are running, returned as a string
 *
 * \returns one of \c "win32", \c "linux", \c "macosx", or QString() for other/unknown.
 */




/** \fn KLF_EXPORT bool klfVersionCompareLessThan(const QString& v1, const QString& v2)
 *
 * \brief Same as
 *   <tt>\ref klfVersionCompare(const QString&,const QString&) "klfVersionCompare"(v1,v2) &lt; 0</tt>
 */

/** \def KLF_PATH_SEP
 * \hideinitializer
 * \brief The character used in the $PATH environment variable to separate different locations
 *
 * Expands to \c ':' (colon) on unices/Mac and to \c ';' (semicolon) on Windows.
 *
 * Note that the character is given as a \c char (observe the single-quotes), not in a string.
 */





// INTERNAL

/** \if klf_internal_docs
 * \def KLF_EXPORT
 * \hideinitializer
 * \internal
 *
 * symbols we want to export will be declared with this macro. this makes declarations
 * platform-independant.
 * \endif
 */

/** \if klf_internal_docs
 * \def KLF_EXPORT_IF_DEBUG
 * \hideinitializer
 * \internal
 *
 * symbols we want to export only if in debug mode will use this definition. This will
 * declare symbols with KLF_EXPORT in debug mode, and without that flag in non-debug mode.
 * \endif
 */




static char __klf_version_string[] = KLF_VERSION_STRING;


KLF_EXPORT const char * klfVersion()
{
  return __klf_version_string;
}

KLF_EXPORT int klfVersionMaj()
{
  return KLF_VERSION_MAJ;
}
KLF_EXPORT int klfVersionMin()
{
  return KLF_VERSION_MIN;
}
KLF_EXPORT int klfVersionRelease()
{
  return KLF_VERSION_REL;
}




// declared in klfdebug.h

class KLFDebugObjectWatcherPrivate
{
public:
  QMap<quintptr, QString> refInfos;
};

// static
KLFDebugObjectWatcher *KLFDebugObjectWatcher::instance = NULL;
// static
KLFDebugObjectWatcher *KLFDebugObjectWatcher::getWatcher()
{
  if (instance == NULL)
    instance = new KLFDebugObjectWatcher;
  return instance;
}

void KLFDebugObjectWatcher::registerObjectRefInfo(QObject *object, const QString& refinfo)
{
  p->refInfos[(quintptr)object] = refinfo;
}

KLFDebugObjectWatcher::KLFDebugObjectWatcher()
  : QObject(qApp)
{
  p = new KLFDebugObjectWatcherPrivate;
}
KLFDebugObjectWatcher::~KLFDebugObjectWatcher()
{
  delete p;
}
void KLFDebugObjectWatcher::debugObjectDestroyed(QObject *object)
{
  quintptr obji = (quintptr) object;
  if (p->refInfos.contains(obji)) {
    klfDbg(klfFmtCC("Object destroyed: (%s*)%p; object reference name is `%s'",
		    (object ? object->metaObject()->className() : "void"),
		    (void*)object, qPrintable(p->refInfos[obji])));
  } else {
    klfDbg(klfFmtCC("Object destroyed: (%s*)%p",
		    (object ? object->metaObject()->className() : "void"),
		    (void*)object));
  }
}



KLF_EXPORT QByteArray klfShortFuncSignature(const QByteArray& funcname)
{
  // returns the section between the last space before the first open paren and the first open paren
  int iSpc, iParen;
  iParen = funcname.indexOf('(');
  iSpc = funcname.lastIndexOf(' ', iParen-2);
  // if iSpc is -1, leave it at -1 (eg. constructor signature), the following code still works.
  if (iParen == -1 || iSpc > iParen) {
    qWarning("klfShortFuncSignature('%s'): Signature parse error!", funcname.constData());
    return funcname;
  }
  // shorten name
  QByteArray f = funcname.mid(iSpc+1, iParen-(iSpc+1));
  // special case: if 'operator...' then keep the arguments, because it doesn't help us much otherwise!
  if (f.startsWith("operator")) {
    return funcname.mid(iSpc+1);
  }
  return f;
}




KLF_EXPORT QByteArray klfFmt(const char * fmt, va_list pp)
{
  static const int bufferSize = 8192;
  char buffer[bufferSize];
  int len;
#if defined(_BSD_SOURCE) || _XOPEN_SOURCE >= 500 || defined(_ISOC99_SOURCE)
  // stdio.h provided vsnprintf()
  len = vsnprintf(buffer, bufferSize, fmt, pp);
  if (len >= bufferSize) {
    // output was truncated
    qWarning("%s(): output from format string \"%s\" was truncated from %d to %d bytes.",
	     KLF_FUNC_NAME, fmt, len, (bufferSize-1));
    len = bufferSize-1;
  }
#else
  len = vsprintf(buffer, fmt, pp);
#endif

  if (len < 0) {
    qWarning("%s(): vs(n)printf() failed for format \"%s\"", KLF_FUNC_NAME, fmt);
    return QByteArray();
  }

  // create a QByteArray
  QByteArray data;
  data = QByteArray(buffer, len);
  return data;
}

KLF_EXPORT QByteArray klfFmt(const char * fmt, ...)
{
  va_list pp;
  va_start(pp, fmt);
  QByteArray data = klfFmt(fmt, pp);
  va_end(pp);
  return data;
}


KLF_EXPORT QByteArray klfFmtDouble(double num, char fmt, int precision)
{
  QString s = QString::number(num, fmt, precision);

  QByteArray data;
  data = s.toLatin1();
  return data;
}




KLF_EXPORT QString klfTimeOfDay(bool shortfmt)
{
  struct timeval tv;
  gettimeofday(&tv, NULL);
  char temp[128];
  if (shortfmt)
    sprintf(temp, "%03ld.%06ld", (ulong)tv.tv_sec % 1000, (ulong)tv.tv_usec);
  else
    sprintf(temp, "%ld.%06ld", (ulong)tv.tv_sec, (ulong)tv.tv_usec);
  return QString::fromLatin1(temp);
}


//#ifdef KLF_DEBUG // see comment in KLFDebugBlock::KLFDebugBlock()
static int __klf_dbg_block_depth_counter = 0;
//#endif

KLFDebugBlock::KLFDebugBlock(const QString& blockName)
  : pBlockName(blockName), pPrintMsg(true)
{
  // unconditionally display the debug message. If we compile the library without KLF_DEBUG
  // and then use this feature in a program /with/ KLF_DEBUG, then we want the debug message
  // to display.

  //#ifdef KLF_DEBUG
  qDebug("%s: [%02d]block begin", qPrintable(pBlockName), ++__klf_dbg_block_depth_counter);
  //#endif
}
KLFDebugBlock::KLFDebugBlock(bool printmsg, const QString& blockName)
  : pBlockName(blockName), pPrintMsg(printmsg)
{
  //#ifdef KLF_DEBUG // see comment in other constructor.
  // convention: __klf_dbg_block_depth_counter is incremented/decremented only when displayed
  if (printmsg)
    qDebug("%s: [%02d]block begin", qPrintable(pBlockName), ++__klf_dbg_block_depth_counter);
  //#endif
}
KLFDebugBlock::~KLFDebugBlock()
{
  //#ifdef KLF_DEBUG // see comment in KLFDebugBlock::KLFDebugBlock()
  // convention: __klf_dbg_block_depth_counter is incremented/decremented only when displayed
  if (pPrintMsg)
    qDebug("%s: [%02d]block end", qPrintable(pBlockName), __klf_dbg_block_depth_counter--);
  //#endif
}
KLFDebugBlockTimer::KLFDebugBlockTimer(const QString& blockName)
  : KLFDebugBlock(false, blockName)
{
  //#ifdef KLF_DEBUG // see comment in KLFDebugBlock::KLFDebugBlock()
  // convention: __klf_dbg_block_depth_counter is incremented/decremented only when displayed
  qDebug("+T:%s: %s: [%02d]block begin", KLF_SHORT_TIME, qPrintable(pBlockName), ++__klf_dbg_block_depth_counter);
  //#endif
}
KLFDebugBlockTimer::~KLFDebugBlockTimer()
{
  //#ifdef KLF_DEBUG // see comment in KLFDebugBlock::KLFDebugBlock()
  // convention: __klf_dbg_block_depth_counter is incremented/decremented only when displayed
  qDebug("+T:%s: %s: [%02d]block end", KLF_SHORT_TIME, qPrintable(pBlockName), __klf_dbg_block_depth_counter--);
  //#endif
}

inline QString func_name_w_parens(const char *f)
{
  QString s = QLatin1String(f);
  if (s.indexOf('(') == -1)
    return s+"()";
  return s;
}

// the following is defined for both debug and non-debug modes. In non-debug modes, it provides the symbol
// __klf_dbg_hdr for debugging eg. plugins compiled in debug mode
KLF_EXPORT QDebug __klf_dbg_hdr(QDebug dbg, const char * funcname, const char *refinstance, const char * shorttime)
{
  if (shorttime == NULL)
    return dbg.nospace()<<qPrintable(func_name_w_parens(funcname))<<":"<<refinstance
			<<"\n        ";
  else
    return dbg.nospace()<<"+T:"<<shorttime<<": "<<qPrintable(func_name_w_parens(funcname))<<":"<<refinstance
			<<"\n        ";
}

KLF_EXPORT QDebug __klf_warning_hdr(QDebug warndbg, const char * funcname, const char * shorttime)
{
  Q_UNUSED(shorttime) ; // maybe if I one day decide to change the format to include shorttime... (?)
  return warndbg.nospace()<<"***** In function "<<qPrintable(func_name_w_parens(funcname))
			  <<"  " // shorttime is too short:
			  <<qPrintable(QDateTime::currentDateTime().toString(Qt::DefaultLocaleShortDate))
			  <<" *****"
			  <<"\n        ";

}



// ----------------------------------------




QStringList klf_version_suffixes =
  QStringList() << "a" << "alpha" << "b" << "beta" << "p" << "pre" << "preview" << "RC" << "rc"
/* */           << "" // empty suffix or any unrecognized suffix
/* */           << "post" << "dev" << "devel";

static int __klf_version_compare_suffix_words(QString w1, QString w2)
{
  // a list of known words
  const QStringList& words = klf_version_suffixes;
  // now compare the words
  int borderBeforeAfter = words.indexOf("");
  if (borderBeforeAfter < 0)
    qWarning("klfVersionCompare: suffix words list doesn't contain \"\"!");
  int i1 = words.indexOf(w1);
  int i2 = words.indexOf(w2);
  if (i1 == -1 && i2 == -1)
    return QString::compare(w1, w2);
  if (i2 == -1)
    return i1 < borderBeforeAfter ? -1 : +1;
  if (i1 == -1)
    return i2 < borderBeforeAfter ? +1 : -1;
  // both are recognized words
  return i1 - i2;
}

#define VERSION_RX "^(\\d+)(\\.(\\d+)(\\.(\\d+)([a-zA-Z]+\\d*)?)?)?$"

KLF_EXPORT bool klfIsValidVersion(const QString& v)
{
  QRegExp rx(VERSION_RX);
  return rx.exactMatch(v);
}

KLF_EXPORT int klfVersionCompare(const QString& v1, const QString& v2)
{
  qDebug("klfVersionCompare(): Comparing versions %s and %s", qPrintable(v1), qPrintable(v2));
  if (v1 == v2)
    return 0;
  if (v1.isEmpty()) // v1 is empty, but v2 is not empty because of test above
    return -1;
  if (v2.isEmpty()) // v2 is empty, but not v1 because of test above
    return 1;
  //           *1     2  *3     4  *5    *6
  QRegExp rx1(VERSION_RX);
  QRegExp rx2(rx1);
  if (!rx1.exactMatch(v1)) {
    qWarning("klfVersionLessThan: Invalid version number format: %s", qPrintable(v1));
    return -200;
  }
  if (!rx2.exactMatch(v2)) {
    qWarning("klfVersionLessThan: Invalid version number format: %s", qPrintable(v2));
    return -200;
  }
  int maj1 = rx1.cap(1).toInt();
  int maj2 = rx2.cap(1).toInt();
  if (maj1 != maj2)
    return maj1 - maj2;
  bool hasmin1 = !rx1.cap(2).isEmpty();
  bool hasmin2 = !rx2.cap(2).isEmpty();
  if ( ! hasmin1 && ! hasmin2 )
    return 0; // equal
  if ( ! hasmin1 && hasmin2 )
    return -1; // 3 < 3.x
  if ( hasmin1 && ! hasmin2 )
    return +1; // 3.x > 3
  int min1 = rx1.cap(3).toInt();
  int min2 = rx2.cap(3).toInt();
  if ( min1 != min2 )
    return min1 - min2;

  bool hasrel1 = !rx1.cap(4).isEmpty();
  bool hasrel2 = !rx2.cap(4).isEmpty();
  if ( ! hasrel1 && ! hasrel2 )
    return 0; // equal
  if ( ! hasrel1 && hasrel2 )
    return -1; // 3.x < 3.x.y
  if ( hasrel1 && ! hasrel2 )
    return +1; // 3.x.y > 3.x
  int rel1 = rx1.cap(5).toInt();
  int rel2 = rx2.cap(5).toInt();
  if ( rel1 != rel2 )
    return rel1 - rel2;

  QString suffix1 = rx1.cap(6);
  QString suffix2 = rx2.cap(6);

  //  qDebug("Suffix1=%s, suffix2=%s", qPrintable(suffix1), qPrintable(suffix2));

  if (suffix1 == suffix2)
    return 0; // equal

  //             1          2
  QRegExp rxs1("^([a-zA-Z]*)(\\d*)$");
  QRegExp rxs2(rxs1);
  rxs1.exactMatch(suffix1); // must necessarily match, already matched global regex
  rxs2.exactMatch(suffix2);

  QString w1 = rxs1.cap(1);
  QString w2 = rxs2.cap(1);
  QString ns1 = rxs1.cap(2);
  QString ns2 = rxs2.cap(2);

  int cmp = __klf_version_compare_suffix_words(w1, w2);
  if (cmp != 0)
    return cmp; // words are enough to make the difference

  // the words are the same -> compare ns1<->ns2
  if (ns1.isEmpty()) {
    if (ns2.isEmpty())
      return 0; // equal
    // with suffix number compares greater than without
    return -1;
  }
  if (ns2.isEmpty()) {
    return +1;
  }

  int n1 = ns1.toInt();
  int n2 = ns2.toInt();
  return n1 - n2;
}

KLF_EXPORT bool klfVersionCompareLessThan(const QString& v1, const QString& v2)
{
  return klfVersionCompare(v1,v2) < 0;
}


