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
 * <pre></pre>
 * and can be used to print debug messages and locate time-consuming
 * instructions for example. */
#define klf_debug_time_print(x)
#endif



#endif
