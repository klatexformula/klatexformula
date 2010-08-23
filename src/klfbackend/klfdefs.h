/***************************************************************************
 *   file klfdefs.h
 *   This file is part of the KLatexFormula Project.
 *   Copyright (C) 2009 by Philippe Faist
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

#ifndef KLFDEFS_H_
#define KLFDEFS_H_

// first, detect a missing KLFBACKEND_QT4 definition
#if defined(QT_VERSION) && QT_VERSION >= 0x040000
# ifndef KLFBACKEND_QT4
#  define KLFBACKEND_QT4
# endif
#endif

#include <qstring.h>
#include <qvariant.h>


// EXPORTING SYMBOLS TO E.G. PLUGINS ...
#ifndef KLF_EXPORT
#  if defined(Q_OS_WIN)
#    if defined(KLF_SRC_BUILD)
#      define KLF_EXPORT __declspec(dllexport)
#    else
#      define KLF_EXPORT __declspec(dllimport)
#    endif
#  else
#    define KLF_EXPORT __attribute__((visibility("default")))
#  endif
#endif


// VERSION INFORMATION


/** Returns the current version of the KLatexFormula library, given as a string,
 * eg. \c "3.2.1".
 *
 * For non-release builds, this may have a suffix, eg. \c "3.2.0beta2".
 */
KLF_EXPORT const char * klfVersion();

/** Returns the current major version of the KLatexFormula library.
 *
 * For example, if the version is "3.2.0", klfVersionMaj() returns \c 3.
 */
KLF_EXPORT int klfVersionMaj();
/** Returns the current minor version of the KLatexFormula library.
 *
 * For example, if the version is "3.2.0", klfVersionMin() returns \c 2.
 */
KLF_EXPORT int klfVersionMin();
/** Returns the current release version of the KLatexFormula library.
 *
 * For example, if the version is "3.2.0", klfVersionRelease() returns \c 0.
 */
KLF_EXPORT int klfVersionRelease();



/** Takes as input a funcion signature like
 *  <pre>void MyClass::functionName(const QString& arg1, int arg2) const</pre>
 * and outputs a short form of it, like
 *  <pre>MyClass::functionName</pre>
 */
KLF_EXPORT QByteArray klfShortFuncSignature(const QByteArray& fullFuncName);
inline QByteArray klfShortFuncSignature(const char *fullFuncName)
#ifdef KLFBACKEND_QT4
{ return klfShortFuncSignature(QByteArray(fullFuncName)); }
#else
{ return klfShortFuncSignature(QByteArray().duplicate(fullFuncName, strlen(fullFuncName))); }
#endif

/** Formats a printf-style string and returns the data as a QByteArray.
 * \warning the length must not exceed 8192 bytes, the size of the internal
 *   buffer. */
KLF_EXPORT QByteArray klfFmt(const char * fmt, ...)
#if defined(Q_CC_GNU) && !defined(__INSURE__)
    __attribute__ ((format (printf, 1, 2)))
#endif
;

/** Implements \ref klfFmt(const char *, ...) functionality, but with
 * a \c va_list argument pointer for use in vsprintf().
 */
KLF_EXPORT QByteArray klfFmt(const char * fmt, va_list pp) ;

KLF_EXPORT QString klfTimeOfDay(bool shortFmt = true);


#ifdef KLFBACKEND_QT4
/** Returns something like <tt>456.234589</tt> (in a <tt>const char*</tt>) that
 * represents the actual time. The absolute reference is undefined, but stays
 * always the same. Useful for debug messages.
 */
#define KLF_SHORT_TIME qPrintable(klfTimeOfDay())
#else
#define KLF_SHORT_TIME (klfTimeOfDay().ascii())
#endif



// DEBUG UTILITIES (SOME WITH TIME PRINTING FOR TIMING OPERATIONS)

/** \brief Utility to time the execution of a block
 *
 * Prints message in constructor and in destructor to test
 * block execution time.
 *
 * Consider using the macro \ref KLF_DEBUG_BLOCK and \ref KLF_DEBUG_TIME_BLOCK. */
class KLF_EXPORT KLFDebugBlock
{
public:
  KLFDebugBlock(const QString& blockName);
  KLFDebugBlock(bool printmsg, const QString& blockName);

  virtual ~KLFDebugBlock();

protected:
  QString pBlockName;
private:
  bool pPrintMsg;
};

/** \brief An extension of KLFDebugBlock with millisecond-time display
 *
 * Consider the use of \ref KLF_DEBUG_BLOCK and \ref KLF_DEBUG_TIME_BLOCK macros instead.
 */
class KLF_EXPORT KLFDebugBlockTimer : public KLFDebugBlock
{
public:
  KLFDebugBlockTimer(const QString& blockName);
  virtual ~KLFDebugBlockTimer();
};


#ifdef KLF_DEBUG


#ifdef KLFBACKEND_QT4
#include <QDebug>
#endif


template<class T>
inline const T& __klf_debug_tee(const T& expr)
#ifdef KLFBACKEND_QT4
{ qDebug()<<"TEE VALUE: "<<expr; return expr; }
#else
{ return expr; } // sorry, no  qDebug()<<(anything you want)  in Qt 3 ...
#endif

// dox doc is in next (unfunctional) definitions in next #if block
#define KLF_DEBUG_TIME_BLOCK(msg) KLFDebugBlockTimer __klf_debug_timer_block(QString("")+msg)
#define KLF_DEBUG_BLOCK(msg) KLFDebugBlock __klf_debug_block(QString("")+msg)
#define klf_debug_tee(expr) __klf_debug_tee(expr)
#ifdef KLFBACKEND_QT4
KLF_EXPORT QDebug __klf_dbg_hdr(QDebug dbg, const char * funcname, const char * shorttime);
#define klfDbg( streamableItems )				\
  __klf_dbg_hdr(qDebug(), KLF_FUNC_NAME, NULL) << streamableItems
#define klfDbgT( streamableItems )					\
  __klf_dbg_hdr(qDebug(), KLF_FUNC_NAME, KLF_SHORT_TIME) << streamableItems
#endif


#else // KLF_DEBUG


/** \brief Utility to time the execution of a block
 *
 * \note KLF_DEBUG needs to be defined at compile-time
 *   to enable this feature. Otherwise, this maco is a no-op.
 *
 * Usage example:
 * \code
 * void do_something() {
 *   KLF_DEBUG_TIME_BLOCK("block do_something() execution") ;
 *   ... // some lengthy operation
 *   ... if (failed) return; // for example
 *   ... // no special instruction needed at end of block
 * }
 * \endcode
 *
 * \note msg is (macro-expanded) added to a <tt>QString("")</tt>, so you can write something like
 * \code
 *   KLF_DEBUG_TIME_BLOCK(KLF_FUNC_NAME+"(QString)") ;
 * \endcode
 * even though <tt>KLF_FUNC_NAME</tt> expands into a <tt>const char*</tt>.
 *
 * \warning This is a macro that expands its text without protecting it (this allows you to do what
 *   the above note said).
 */
#define KLF_DEBUG_TIME_BLOCK(msg)
/** \brief Utility to debug the execution of a block
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
 * \note msg is (macro-expanded) added to a <tt>QString("")</tt>, so you can write something like
 * \code
 *   KLF_DEBUG_BLOCK(KLF_FUNC_NAME+"(QString)") ;
 * \endcode
 * even though <tt>KLF_FUNC_NAME</tt> expands into a <tt>const char*</tt>.
 *
 * \warning This is a macro that expands its text without protecting it (this allows you to do what
 *   the above note said).
 */
#define KLF_DEBUG_BLOCK(msg)

/** \brief Print the value of expression and return it
 *
 * If KLF_DEBUG preprocessor symbol is not defined, this macro just expands to <tt>(expr)</tt>.
 *
 * \note This macro works only in Qt4.
 *
 * Very useful for debugging return values, eg.
 * \code
 *   return klf_debug_tee(result);
 * \endcode
 */
#define klf_debug_tee(expr) (expr)

/** \brief print debug stream items
 *
 * \warning This macro may be counter-intuitive, as the argument is not evaluated before expansion.
 *   This macro expands exactly to:
 *   \code qDebug()<<KLF_FUNC_NAME<<":"<< streamableItems \endcode
 *   which means that you can list any QDebug-streamable items as macro arguments, separated by a
 *   <tt>&lt;&lt;</tt> operator, keeping in mind that these stream operators will be "seen" only
 *   AFTER the macro has expanded. Example usage:
 *   \code
 * int index = ...; QString mode = ...;
 * klfDbg( "index is "<<index<<" and mode is "<<mode ); \endcode
 *
 * The advantage of this syntax is that when disabling debug, the parts in your code where you
 * call this macro are truly "erased" (the macro itself, with all its arguments, expands to
 * nothing), and result into no overhead of having to go eg. through null-streams. Additionally,
 * all macro arguments are NOT evaluated in non-debug mode.
 */
#define klfDbg( streamableItems )
/** \brief print debug stream items, with current time
 *
 * Same as klfDbg(), but also prints current time given by KLF_SHORT_TIME.
 */
#define klfDbgT( streamableItems )

#endif // KLF_DEBUG


//! Simple test for one-time-run functions
#define KLF_FUNC_SINGLE_RUN						\
  { static bool first_run = true;  if ( ! first_run )  return; first_run = false; }


#if defined(__STDC_VERSION__) && __STDC_VERSION__ < 199901L
# if defined(__GNUC__) && __GNUC__ >= 2
#  define __func__ __FUNCTION__
# else
#  define __func__ "<unknown>"
# endif
#endif
#if defined KLF_HAS_PRETTY_FUNCTION
#define KLF_FUNC_NAME (klfShortFuncSignature(__PRETTY_FUNCTION__).data())
#elif defined KLF_HAS_FUNCTION
#define KLF_FUNC_NAME __FUNCTION__
#elif defined KLF_HAS_FUNC
#define KLF_FUNC_NAME __func__
#else
/** This macro expands to the function name this macro is called in */
#define KLF_FUNC_NAME "<unknown>"
#endif

//! Asserting Non-NULL pointers (NON-FATAL)
/**
 * If the given \c ptr is NULL, then prints function name and message to standard warning
 * output (using Qt's qWarning()) and executes instructions given by \c failaction.
 * \c msg may contain << operators to chain output to a QDebug.
 */
#define KLF_ASSERT_NOT_NULL(ptr, msg, failaction)			\
  if ((ptr) == NULL) {							\
    qWarning().nospace()<<"In function "<<KLF_FUNC_NAME<<":\n\t"<<msg;	\
    failaction;								\
  }




#if defined(KLFBACKEND_QT4) && defined(QT_NO_DEBUG_OUTPUT)
inline QDebug& operator<<(QDebug& str, const QVariant& v) { return str; }
#endif



// utility functions


//! Get System Information
namespace KLFSysInfo
{
  enum Os { Linux, Win32, MacOsX, OtherOs };

  inline int sizeofVoidStar() { return sizeof(void*); }

  //! Returns one of \c "x86" or \c "x86_64", or \c QString() for other/unknown
  KLF_EXPORT QString arch();

  KLF_EXPORT KLFSysInfo::Os os();

  //! Returns one of \c "win32", \c "linux", \c "macosx", or QString() for other/unknown
  KLF_EXPORT QString osString(KLFSysInfo::Os sysos = os());
};




//! Compares two version strings
/** \c v1 and \c v2 must be of the form \c "<MAJ>.<MIN>.<REL><suffix>" or \c "<MAJ>.<MIN>.<REL>"
 * or \c "<MAJ>.<MIN>" or \c "<MAJ>".
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
KLF_EXPORT int klfVersionCompare(const QString& v1, const QString& v2);

/** \brief Same as
 *    <tt>\ref klfVersionCompare(const QString&,const QString&) "klfVersionCompare"(v1,v2) &lt; 0</tt>
 */
KLF_EXPORT bool klfVersionCompareLessThan(const QString& v1, const QString& v2);


#if defined(Q_OS_WIN32) || defined(Q_OS_WIN64)
#define KLF_PATH_SEP ';'
#else
#define KLF_PATH_SEP ':'
#endif

KLF_EXPORT QStringList klfSearchFind(const QString& wildcard_expression, int limit = -1);
KLF_EXPORT QString klfSearchPath(const QString& prog, const QString& extra_path = "");





#endif
