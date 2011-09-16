/***************************************************************************
 *   file klflegacymacros_p.h
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

#ifndef KLFLEGACYMACROS_P_H
#define KLFLEGACYMACROS_P_H

//#include <string.h>

// This file used to contain some convenient #define's for Qt3/4 compatibility
// Now, Qt3 dependency was dropped and I was too lazy to go through all the source
// code and replace those macros with their regular Qt4 counterparts. So the
// Qt4 versions of these macros remain here in a private header.

#define dir_native_separators(x) QDir::toNativeSeparators(x)
#define dir_entry_list(namefilter) entryList(QStringList()<<(namefilter))
#define ba_assign(otherba) operator=(otherba)
#define ba_cat(baptr, moredata) (*(baptr) += (moredata))
#define ba_indexOf klfqt4_ba_indexof
#define ba_replace(baptr, start, len, replacement) (baptr)->replace((start), (len), (replacement))
//! Usage: <tt>buf_decl_ba(QBuffer buffer, mybytearray)</tt>
/** will expand to <tt>QBuffer buffer(&mybytearray)</tt> on Qt4. */
#define buf_decl_ba(bufdecl, ba) bufdecl(&(ba));
#define dev_WRITEONLY QIODevice::WriteOnly
#define dev_READONLY QIODevice::ReadOnly
#define dev_TEXTTRANSLATE QIODevice::Text
#define dev_write write
#define fi_suffix suffix
#define fi_absolutePath absolutePath
#define f_open_fp(fp) open((fp), dev_WRITEONLY)
#define f_setFileName setFileName
#define f_filename fileName
#define f_error error
#define s_trimmed trimmed
#define s_toUpper toUpper
#define s_toLatin1 toLatin1
#define s_toLocal8Bit toLocal8Bit
#define s_indexOf indexOf
#define str_split(string, sep, boolAllowEmptyEntries)			\
  (string).split((sep), (boolAllowEmptyEntries) ? QString::KeepEmptyParts : QString::SkipEmptyParts)
#define list_indexOf(x) indexOf((x))
#define rx_indexin(str) indexIn((str))
#define rx_indexin_i(str, i) indexIn((str), (i))
#define img_settext(key, value)  setText((key), (value))


// EXTRA DEFINITIONS

// QT 4

inline int klfqt4_ba_indexof(const QByteArray& ba, const char *str, int from = 0)
{ return ba.indexOf(str, from); }



#endif // KLFLEGACYMACROS_P_H
