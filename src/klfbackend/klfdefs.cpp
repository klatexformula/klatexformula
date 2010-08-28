/***************************************************************************
 *   file klfdefs.cpp
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

#include <stdlib.h>
#include <stdio.h> // vsprintf(), vsnprintf
#include <stdarg.h> // va_list, va_arg, va_end in klfFmt()
#include <sys/time.h>

#include <qdir.h>
#include <qfile.h>
#include <qfileinfo.h>
#include <qregexp.h>

#ifdef KLFBACKEND_QT4
#include <QDebug>
#endif

#include "klfdefs.h"


/** \def KLF_EXPORT
 *
 * symbols we want to export will be declared with this macro. this makes declarations
 * platform-independant. */

/** \def KLF_EXPORT_IF_DEBUG
 *
 * symbols we want to export only if in debug mode will use this definition. This will
 * declare symbols with KLF_EXPORT in debug mode, and without that flag in non-debug mode. */

/** \fn KLF_EXPORT int klfVersion()
 *
 * Returns the current version of the KLatexFormula library, given as a string,
 * eg. \c "3.2.1".
 *
 * For non-release builds, this may have a suffix, eg. \c "3.2.0beta2".
 */
/** \fn KLF_EXPORT int klfVersionMaj()
 *
 * Returns the current major version of the KLatexFormula library.
 *
 * For example, if the version is \c "3.2.0", klfVersionMaj() returns \c 3.
 */
/** \fn KLF_EXPORT int klfVersionMin()
 *
 * Returns the current minor version of the KLatexFormula library.
 *
 * For example, if the version is \c "3.2.0", klfVersionMin() returns \c 2.
 */
/** \fn KLF_EXPORT int klfVersionRelease()
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
 *
 * Utility for replacing the (function) klfFmt()'s QByteArray return value into a
 * regular <tt>const char*</tt>-C-string value, eg. to pass into a QDebug stream.
 *
 * \warning this macro, when called as <tt>klfFmtCC (args...)</tt> expands to
 * \code  (const char*)klfFmt (args...)  \endcode
 * No parentheses can be forced to ensure the <tt>const char*</tt> before any other
 * operation in the context, however casts are higher priority than many other
 * operators, so you should be safe. Still, be warned.
 *
 * \note This macro can take no parameters, since C preprocessor macros don't support
 *   variable number of arguments (as required by printf-style formatting).
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

/** \def KLF_DEBUG_TIME_BLOCK
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

/** \def klf_debug_tee
 *
 * \brief Print the value of expression and return it
 *
 * If KLF_DEBUG preprocessor symbol is not defined, this macro just expands to <tt>(expr)</tt>.
 *
 * \note This macro works only in Qt4.
 *
 * Very useful for debugging return values, eg.
 * \code
 *   return klf_debug_tee(result);
 * \endcode
 * effectively does
 * \code
 *   return result;
 * \endcode
 * however printing the result while returning it.
 */

/** \def klfDbg
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
 *
 * \warning This macro in its full-featured version requires Qt4. When used with Qt3, the
 *   argument may no longer be "any streamable items" because Qt3 does not support qDebug()
 *   streaming. However this macro still works, with the limitation that the argument must
 *   be a QString, or an expression that can be cast into a QString. (see also klfFmt())
 * \code
 * // Qt3 Usage Example
 * klfDbg("debug message") ;
 * klfDbg(QString("debug message. value is %1.").arg(value)) ;
 * klfDbg(klfFmt("debug. value is %d, string is %s, and flags are %#010x.", intvalue, strvalue, flags)) ;
 * \endcode
 */

/** \def klfDbgT
 *
 * \brief print debug stream items, with current time
 *
 * Same as klfDbg(), but also prints current time given by KLF_SHORT_TIME.
 */

/** \def klfDbgSt
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
 *
 * \brief print debug stream items, with current time (special case)
 *
 * Same as klfDbgSt(), but prints also the time like klfDbgT() does.
 */

/** \def KLF_FUNC_SINGLE_RUN
 *
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

/** \def KLF_FUNC_NAME
 *
 * This macro expands to the function name this macro is called in.
 *
 * The header <tt>klfdefs.h</tt> will try to determine which of the symbols
 * <tt>__PRETTY_FUNCTION__</tt>, <tt>__FUNCTION__</tt> and/or <tt>__func__</tt> are defined
 * and use the most "pretty" one, formatting it as necessary with klfShortFuncSignature().
 */

/** \def KLF_ASSERT_NOT_NULL
 *
 * \brief Asserting Non-NULL pointers (NON-FATAL)
 *
 * If the given \c ptr is NULL, then prints function name and message to standard warning
 * output (using Qt's qWarning()) and executes instructions given by \c failaction.
 * \c msg may contain << operators to chain output to a QDebug.
 *
 * \c failaction is any C/C++ instructions that you can place in an 'if' block, including \c 'return',
 *   'continue' or 'break' instructions.
 *
 * This macro is not affected by the KLF_DEBUG symbol. This macro is always defined and functional.
 *
 * Example:
 * \code
 *  QString get_myobject_name(MyObject *object)
 *  {
 *    KLF_ASSERT_NOT_NULL( object , "object is NULL!" , return QString() ) ;
 *    return object->name;
 *  }
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
 * When a suffix is appended to the version, it is attempted to be recognized as one of:
 *  - "alpha" or "alphaN" is alpha version, eg. "3.1.1alpha2" < "3.1.1.alpha5" < "3.1.1"
 *  - "dev" is INTERNAL versioning, should not be published, it means further development after
 *    the given version number; for the next release, a higher version number has to be
 *    decided upon.
 *  - unrecognized suffixes are compared lexicographically, case sensitive.
 *
 * Some examples:
 * <pre>   "3.1.0" < "3.1.2"
 *   "2" < "2.1" < "2.1.1"
 *   "3.0.0alpha2" < "3.0.0"
 *   "3.0.2" < "3.0.3alpha0"
 * </pre>
 */

/** \fn KLF_EXPORT bool klfVersionCompareLessThan(const QString& v1, const QString& v2)
 *
 * \brief Same as
 *   <tt>\ref klfVersionCompare(const QString&,const QString&) "klfVersionCompare"(v1,v2) &lt; 0</tt>
 */

/** \def KLF_PATH_SEP
 *
 * \brief The character used in the $PATH environment variable to separate different locations
 *
 * Expands to \c ':' (colon) on unices/Mac and to \c ';' (semicolon) on Windows.
 */

/** \fn KLF_EXPORT QStringList klfSearchFind(const QString& wildcard_expression, int limit = -1)
 *
 * \bug ...... MISSING DOX DOC ...........
 */

/** \fn KLF_EXPORT QString klfSearchPath(const QString& prog, const QString& extra_path = "")
 *
 * \bug ....... MISSING DOX DOC ...........
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



#ifndef KLFBACKEND_QT4
#define qPrintable(x) (x).local8Bit().data()
#define QLatin1String QString::fromLatin1
#define toLocal8Bit local8Bit
// QDir::toNativeSeparators() -> QDir::convertSeparators()
#define toNativeSeparators convertSeparators
#define SPLIT_STRING(x, sep, boolAllowEmptyEntries)		\
  QStringList::split((sep), (x), (boolAllowEmptyEntries))
#define list_indexOf(list, x) (list).findIndex((x))
#else
#define SPLIT_STRING(x, sep, boolAllowEmptyEntries)	\
  (x).split((sep), (boolAllowEmptyEntries) ? QString::KeepEmptyParts : QString::SkipEmptyParts)
#define list_indexOf(list, x) (list).indexOf((x))
#endif

// declared in klfdefs.h

KLF_EXPORT QByteArray klfShortFuncSignature(const QByteArray& ba_funcname)
{
  QString funcname(ba_funcname);
  // returns the section between the last space before the first open paren and the first open paren
  int iSpc, iParen;
#ifdef KLFBACKEND_QT4
  iParen = funcname.indexOf('(');
  iSpc = funcname.lastIndexOf(' ', iParen-2);
#else
  iParen = funcname.find('(');
  iSpc = funcname.findRev(' ', iParen-2);
#endif
  // if iSpc is -1, leave it at -1 (eg. constructor signature), the following code still works.
  if (iParen == -1 || iSpc > iParen) {
    qWarning("klfShortFuncSignature('%s'): Signature parse error!", qPrintable(funcname));
    return ba_funcname;
  }
  // shorten name
  QString f = funcname.mid(iSpc+1, iParen-(iSpc+1));
  QByteArray data = f.toLocal8Bit();
  return data;
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
#ifdef KLFBACKEND_QT4
  data = QByteArray(buffer, len);
#else
  data.duplicate(buffer, len);
#endif
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


KLF_EXPORT QString klfTimeOfDay(bool shortfmt)
{
  struct timeval tv;
  gettimeofday(&tv, NULL);
  char temp[128];
  if (shortfmt)
    sprintf(temp, "%03ld.%06ld", (ulong)tv.tv_sec % 1000, (ulong)tv.tv_usec);
  else
    sprintf(temp, "%ld.%06ld", (ulong)tv.tv_sec, (ulong)tv.tv_usec);
  return QString::fromAscii(temp);
}


#ifdef KLF_DEBUG
static int __klf_dbg_block_depth_counter = 0;
#endif

KLFDebugBlock::KLFDebugBlock(const QString& blockName)
  : pBlockName(blockName), pPrintMsg(true)
{
#ifdef KLF_DEBUG
  qDebug("%s: [%02d]block begin", qPrintable(pBlockName), ++__klf_dbg_block_depth_counter);
#endif
}
KLFDebugBlock::KLFDebugBlock(bool printmsg, const QString& blockName)
  : pBlockName(blockName), pPrintMsg(printmsg)
{
#ifdef KLF_DEBUG
  // convention: __klf_dbg_block_depth_counter is incremented/decremented only when displayed
  if (printmsg)
    qDebug("%s: [%02d]block begin", qPrintable(pBlockName), ++__klf_dbg_block_depth_counter);
#endif
}
KLFDebugBlock::~KLFDebugBlock()
{
#ifdef KLF_DEBUG
  // convention: __klf_dbg_block_depth_counter is incremented/decremented only when displayed
  if (pPrintMsg)
    qDebug("%s: [%02d]block end", qPrintable(pBlockName), __klf_dbg_block_depth_counter--);
#endif
}
KLFDebugBlockTimer::KLFDebugBlockTimer(const QString& blockName)
  : KLFDebugBlock(false, blockName)
{
#ifdef KLF_DEBUG
  // convention: __klf_dbg_block_depth_counter is incremented/decremented only when displayed
  qDebug("+T:%s: %s: [%02d]block begin", KLF_SHORT_TIME, qPrintable(pBlockName), ++__klf_dbg_block_depth_counter);
#endif
}
KLFDebugBlockTimer::~KLFDebugBlockTimer()
{
#ifdef KLF_DEBUG
  // convention: __klf_dbg_block_depth_counter is incremented/decremented only when displayed
  qDebug("+T:%s: %s: [%02d]block end", KLF_SHORT_TIME, qPrintable(pBlockName), __klf_dbg_block_depth_counter--);
#endif
}

#ifdef KLFBACKEND_QT4
// QT 4 >>
KLF_EXPORT QDebug __klf_dbg_hdr(QDebug dbg, const char * funcname, const char *refinstance, const char * shorttime)
{
#  ifdef KLF_DEBUG
  if (shorttime == NULL)
    return dbg.nospace()<<funcname<<"():"<<refinstance<<"\n  ";
  else
    return dbg.nospace()<<"+T:"<<shorttime<<": "<<funcname<<"():"<<refinstance<<"\n  ";
#  else
  Q_UNUSED(funcname) ;
  Q_UNUSED(shorttime) ;
  Q_UNUSED(refinstance) ;

  return dbg; // do nothing (not debug mode)
#  endif
}
// << QT 4
#else
// QT 3 >>
int __klf_dbg_string_obj::operator=(const QString& msg)
{
#  ifdef KLF_DEBUG
  qDebug("%s", qPrintable(hdr + msg));
#  endif
  return 0;
}
KLF_EXPORT __klf_dbg_string_obj
/* */ __klf_dbg_hdr_qt3(const char *funcname, const char *refinstance, const char *shorttime)
{
#  ifdef KLF_DEBUG
  QString s;
  if (shorttime == NULL)
    s = QString::fromLocal8Bit(funcname) + "():" + refinstance + "\n  ";
  else
    s = QString::fromLocal8Bit("+T:") + shorttime + ": " + funcname + "(): " + refinstance + "\n  ";
  return  __klf_dbg_string_obj(s);
#  else
  return  __klf_dbg_string_obj(QString());
#  endif
}
// << QT 3
#endif




// ----------------------------------------------------------------------




KLF_EXPORT QString KLFSysInfo::arch()
{
  return KLF_CMAKE_ARCH;
}

KLF_EXPORT KLFSysInfo::Os KLFSysInfo::os()
{
#if defined(Q_OS_LINUX)
  return Linux;
#elif defined(Q_OS_DARWIN)
  return MacOsX;
#elif defined(Q_OS_WIN32)
  return Win32;
#else
  return OtherOs;
#endif
}

KLF_EXPORT QString KLFSysInfo::osString(Os sysos)
{
  switch (sysos) {
  case Linux: return QLatin1String("linux");
  case MacOsX: return QLatin1String("macosx");
  case Win32: return QLatin1String("win32");
  case OtherOs: return QString();
  default: ;
  }
  qWarning("KLFSysInfo::osString: unknown OS: %d", sysos);
  return QString();
}


QStringList klf_version_suffixes =
  QStringList() << "a" << "alpha" << "b" << "beta" << "p" << "pre" << "preview" << "RC" << "rc"
/* */           << "" // empty suffix or any unrecognized suffix
/* */           << "dev" << "devel";

static int __klf_version_compare_suffix_words(QString w1, QString w2)
{
  // a list of known words
  const QStringList& words = klf_version_suffixes;
  // now compare the words
  int borderBeforeAfter = list_indexOf(words, "");
  if (borderBeforeAfter < 0)
    qWarning("klfVersionCompare: suffix words list doesn't contain \"\"!");
  int i1 = list_indexOf(words, w1);
  int i2 = list_indexOf(words, w2);
  if (i1 == -1 && i2 == -1)
    return QString::compare(w1, w2);
  if (i2 == -1)
    return i1 < borderBeforeAfter ? -1 : +1;
  if (i1 == -1)
    return i2 < borderBeforeAfter ? +1 : -1;
  // both are recognized words
  return i1 - i2;
}


KLF_EXPORT int klfVersionCompare(const QString& v1, const QString& v2)
{
  qDebug("klfVersionCompare(): Comparing versions %s and %s", qPrintable(v1), qPrintable(v2));
  if (v1 == v2)
    return 0;
  if (v1.isEmpty()) // v2 is not empty because of test above
    return -1;
  if (v2.isEmpty()) // same note with v1
    return 1;
  //           *1     2  *3     4  *5    *6
  QRegExp rx1("^(\\d+)(\\.(\\d+)(\\.(\\d+)([a-zA-Z]+\\d*)?)?)?$");
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



// negative limit means "no limit"
static QStringList __search_find_test(const QString& root, const QStringList& pathlist,
				      int level, int limit)
{
  if (limit == 0)
    return QStringList();

  if (limit < 0)
    limit = -1; // normalize negative values to -1 (mostly cosmetic...)

  QStringList newpathlist = pathlist;
  // our level: levelpathlist contains items in pathlist from 0 to level-1 inclusive.
  QStringList levelpathlist;
  int k;
  for (k = 0; k < level; ++k) { levelpathlist << newpathlist[k]; }
  // the dir/file at our level:
  QString flpath = root+levelpathlist.join("/");
  QFileInfo flinfo(flpath);
  if (flinfo.isDir()) {
    QDir d(flpath);
    QStringList entries;
#ifdef KLFBACKEND_QT4
    entries = d.entryList(QStringList()<<pathlist[level]);
#else
    entries = d.entryList(pathlist[level]);
#endif
    QStringList hitlist;
    for (k = 0; k < (int)entries.size(); ++k) {
      newpathlist[level] = entries[k];
      hitlist += __search_find_test(root, newpathlist, level+1, limit - hitlist.size());
      if (limit >= 0 && (int)hitlist.size() >= limit) // reached limit
	break;
    }
    return hitlist;
  }
  if (flinfo.exists()) {
    return QStringList() << QDir::toNativeSeparators(root+pathlist.join("/"));
  }
  return QStringList();
}

// returns at most limit results matching wildcard_expression (which is given as absolute path
// with wildcards)
KLF_EXPORT QStringList klfSearchFind(const QString& wildcard_expression, int limit)
{
  QString expr;
#ifdef KLFBACKEND_QT4
  expr = QDir::fromNativeSeparators(wildcard_expression);
#else
  expr = wildcard_expression;  expr.replace(QDir::separator(), "/");
#endif
  QStringList pathlist = SPLIT_STRING(expr, "/", false);
  QString root = "/";
  static QRegExp driveregexp("^[A-Za-z]:$");
  if (driveregexp.exactMatch(pathlist[0])) {
    // Windows System with X: drive letter
    root = pathlist[0]+"/";
    pathlist.pop_front();
  }
  return __search_find_test(root, pathlist, 0, limit);
}

// smart search PATH that will interpret wildcards in PATH+extra_path and return the first matching
// executable
KLF_EXPORT QString klfSearchPath(const QString& prog, const QString& extra_path)
{
  static const QString PATH = getenv("PATH");
  static const QString pathsep = QString("")+KLF_PATH_SEP;

  QString path = PATH;
  if (!extra_path.isEmpty())
    path += pathsep + extra_path;

  const QStringList paths = SPLIT_STRING(path, pathsep, true);
  QString test;
  int k, j;
  for (k = 0; k < (int)paths.size(); ++k) {
    QStringList hits = klfSearchFind(paths[k]+"/"+prog);
    for (j = 0; j < (int)hits.size(); ++j) {
      if ( QFileInfo(hits[j]).isExecutable() ) {
	return hits[j];
      }
    }
  }
  return QString::null;
}





