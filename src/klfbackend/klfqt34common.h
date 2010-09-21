/***************************************************************************
 *   file klfqt34common.h
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

#ifndef KLFQT34COMMON_H
#define KLFQT34COMMON_H


// some convenient #define's for Qt3/4 compatibility

#ifdef KLFBACKEND_QT4
#define dir_native_separators(x) QDir::toNativeSeparators(x)
#define ba_assign(otherba) operator=(otherba)
#define dev_WRITEONLY QIODevice::WriteOnly
#define dev_READONLY QIODevice::ReadOnly
#define dev_write write
#define fi_suffix suffix
#define f_open_fp(fp) open((fp), dev_WRITEONLY)
#define f_setFileName setFileName
#define f_error error
#define s_trimmed trimmed
#define s_toUpper toUpper
#define s_toLatin1 toLatin1
#define s_toLocal8Bit toLocal8Bit
#define str_split(string, sep, boolAllowEmptyEntries)			\
  (string).split((sep), (boolAllowEmptyEntries) ? QString::KeepEmptyParts : QString::SkipEmptyParts)
#define list_indexOf(x) indexOf((x))
#define rx_indexin(str) indexIn((str))
#define img_settext(key, value)  setText((key), (value))
#else
#define qPrintable(x) (x).local8Bit().data()
#define QLatin1String QString::fromLatin1
#define dir_native_separators(x) QDir::convertSeparators(x)
#define ba_assign(otherba) duplicate((otherba).data(), (otherba).size())
#define dev_WRITEONLY IO_WriteOnly
#define dev_READONLY IO_ReadOnly
#define dev_write writeBlock
#define fi_suffix extension
#define f_open_fp(fp) open(dev_WRITEONLY, (fp))
#define f_setFileName setName
#define f_error errorString
#define s_trimmed stripWhiteSpace
#define s_toUpper upper
#define s_toLatin1 latin1
#define s_toLocal8Bit local8Bit
#define str_split(string, sep, boolAllowEmptyEntries)		\
  QStringList::split((sep), (string), (boolAllowEmptyEntries))
#define list_indexOf(x) findIndex((x))
#define rx_indexin(str) search((str))
#define img_settext(key, value)  setText((key), 0, (value))
#endif


// EXTRA DEFINITIONS

#ifndef KLFBACKEND_QT4
// The following lines defining quintptr/qptrdiff are taken from Qt's corelib/global/qglobal.h
// source file. (changed QIntegerForSize -> __klf_integer_for_size)
/*
  quintptr and qptrdiff is guaranteed to be the same size as a pointer, i.e.

      sizeof(void *) == sizeof(quintptr)
      && sizeof(void *) == sizeof(qptrdiff)
*/
template <int> struct __klf_integer_for_size;
template <>    struct __klf_integer_for_size<1> { typedef Q_UINT8  Unsigned; typedef Q_INT8  Signed; };
template <>    struct __klf_integer_for_size<2> { typedef Q_UINT16 Unsigned; typedef Q_INT16 Signed; };
template <>    struct __klf_integer_for_size<4> { typedef Q_UINT32 Unsigned; typedef Q_INT32 Signed; };
template <>    struct __klf_integer_for_size<8> { typedef Q_UINT64 Unsigned; typedef Q_INT64 Signed; };
template <class T> struct __klf_integer_for_sizeof: __klf_integer_for_size<sizeof(T)> { };
typedef __klf_integer_for_sizeof<void*>::Unsigned quintptr;
typedef __klf_integer_for_sizeof<void*>::Signed qptrdiff;
#endif





#endif // KLFQT34COMMON_H
