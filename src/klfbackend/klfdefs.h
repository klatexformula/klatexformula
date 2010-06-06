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

#include <qstring.h>
#include <qvariant.h>


// EXPORTING SYMBOLS TO PLUGINS ...
#if defined(Q_OS_WIN)
#  if defined(KLF_SRC_BUILD)
#    define KLF_EXPORT __declspec(dllexport)
#  else
#    define KLF_EXPORT __declspec(dllimport)
#  endif
#else
#  define KLF_EXPORT __attribute__((visibility("default")))
#endif


/** Takes as input a funcion signature like
 *  <pre>void MyClass::functionName(const QString& arg1, int arg2) const</pre>
 * and outputs a short form of it, like
 *  <pre>MyClass::functionName()</pre>
 */
KLF_EXPORT QByteArray klfShortFuncSignature(const QByteArray& fullFuncName);


// DEBUG UTILITIES (SOME WITH TIME PRINTING FOR TIMING OPERATIONS)

/** \brief Utility to time the execution of a block
 *
 * Prints message in constructor and in destructor to test
 * block execution time */
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

class KLF_EXPORT KLFDebugBlockTimer : public KLFDebugBlock
{
public:
  KLFDebugBlockTimer(const QString& blockName);
  virtual ~KLFDebugBlockTimer();
};


#ifdef KLF_DEBUG
#include <sys/time.h>
/** FOR DEBUGGING: Print Debug message with precise current time */
KLF_EXPORT void __klf_debug_time_print(QString str);

// dox doc is in next (unfunctional) definition in next #if block
#define klf_debug_time_print(msg) __klf_debug_time_print(msg)
#define KLF_DEBUG_TIME_BLOCK(msg) KLFDebugBlockTimer __klf_debug_timer_block(msg)
#define KLF_DEBUG_BLOCK(msg) KLFDebugBlock __klf_debug_block(msg)
#else


/** \brief Print debug message with precise current time
 *
 * This function outputs something like:
 * <pre>010.038961 : <i>debug message given in x</i></pre>
 * and can be used to print debug messages and locate time-consuming
 * instructions for example.
 *
 * \note KLF_DEBUG needs to be defined at compile-time
 *   to enable this feature. Otherwise, this maco is a no-op.
 */
#define klf_debug_time_print(x)
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
 */
#define KLF_DEBUG_TIME_BLOCK(msg)
/** \brief Utility to debug the execution of a block
 *
 * \note KLF_DEBUG needs to be defined at compile-time to enable
 *   this feature. Otherwise, this macro is a no-op.
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
 *     return;
 *   }
 *   ... // no special instruction needed at end of block
 * }
 * \endcode
 */
#define KLF_DEBUG_BLOCK(msg)

#endif


// Simple test for one-time-run functions
#define KLF_FUNC_SINGLE_RUN \
  { static bool first_run = true;  if ( ! first_run )  return; first_run = false; }


#if __STDC_VERSION__ < 199901L
# if __GNUC__ >= 2
#  define __func__ __FUNCTION__
# else
#  define __func__ "<unknown>"
# endif
#endif
#if defined KLF_CMAKE_HAS_PRETTY_FUNCTION
#define KLF_FUNC_NAME klfShortFuncSignature(__PRETTY_FUNCTION__).data()
#elif defined KLF_CMAKE_HAS_FUNCTION
#define KLF_FUNC_NAME __FUNCTION__
#elif defined KLF_CMAKE_HAS_FUNC
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




#ifdef QT_NO_DEBUG_OUTPUT
inline QDebug& operator<<(QDebug& str, const QVariant& v) { return str; }
//template<class Key, class Value>
//QDebug& operator<<(QDebug& str, const QMap<Key,Value>& map) { return str; }
#endif





#endif
