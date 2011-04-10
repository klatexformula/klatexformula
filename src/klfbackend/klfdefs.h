/***************************************************************************
 *   file klfdefs.h
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

#ifndef KLFDEFS_H_
#define KLFDEFS_H_

#include <qobject.h>

// first, detect a missing KLFBACKEND_QT4 definition
#if defined(QT_VERSION) && QT_VERSION >= 0x040000
#  ifndef KLFBACKEND_QT4
#    define KLFBACKEND_QT4
#   endif
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

KLF_EXPORT const char * klfVersion();

KLF_EXPORT int klfVersionMaj();
KLF_EXPORT int klfVersionMin();
KLF_EXPORT int klfVersionRelease();


KLF_EXPORT QByteArray klfFmt(const char * fmt, ...)
#if defined(Q_CC_GNU) && !defined(__INSURE__)
    __attribute__ ((format (printf, 1, 2)))
#endif
;

#define klfFmtCC   (const char*)klfFmt

KLF_EXPORT QByteArray klfFmt(const char * fmt, va_list pp) ;


#define KLF_FUNC_SINGLE_RUN						\
  { static bool first_run = true;  if ( ! first_run )  return; first_run = false; }



// utility functions


namespace KLFSysInfo
{
  enum Os { Linux, Win32, MacOsX, OtherOs };

  inline int sizeofVoidStar() { return sizeof(void*); }

  KLF_EXPORT QString arch();

  KLF_EXPORT KLFSysInfo::Os os();

  KLF_EXPORT QString osString(KLFSysInfo::Os sysos = os());
};




KLF_EXPORT bool klfIsValidVersion(const QString& v);

KLF_EXPORT int klfVersionCompare(const QString& v1, const QString& v2);

KLF_EXPORT bool klfVersionCompareLessThan(const QString& v1, const QString& v2);


#if defined(Q_OS_WIN32) || defined(Q_OS_WIN64)
#  define KLF_PATH_SEP ';'
#else
#  define KLF_PATH_SEP ':'
#endif

KLF_EXPORT QStringList klfSearchFind(const QString& wildcard_expression, int limit = -1);
KLF_EXPORT QString klfSearchPath(const QString& prog, const QString& extra_path = "");



// Import debugging utilities

#include <klfdebug.h>



#endif
