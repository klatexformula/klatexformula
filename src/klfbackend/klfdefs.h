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



// DEBUG WITH TIME PRINTING FOR TIMING OPERATIONS

#ifdef KLF_DEBUG_TIME_PRINT
#include <sys/time.h>
// FOR DEBUGGING: Print Debug message with precise current time
KLF_EXPORT void __klf_debug_time_print(QString str);
#define klf_debug_time_print(x) __klf_debug_time_print(x)
#else
/** \brief Print debug message with precise current time
 *
 * This function outputs something like:
 * <pre>010.038961 : <i>debug message given in x</i></pre>
 * and can be used to print debug messages and locate time-consuming
 * instructions for example. */
#define klf_debug_time_print(x)
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
#define KLF_FUNC_NAME __PRETTY_FUNCTION__
#elif defined KLF_CMAKE_HAS_FUNCTION
#define KLF_FUNC_NAME __FUNCTION__
#elif defined KLF_CMAKE_HAS_FUNC
#define KLF_FUNC_NAME __func__
#else
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





#endif
