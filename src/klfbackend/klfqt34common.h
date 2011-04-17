/***************************************************************************
 *   file klfqt34common.h
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

#ifndef KLFQT34COMMON_H
#define KLFQT34COMMON_H

#include <string.h>

// some convenient #define's for Qt3/4 compatibility

#ifdef KLFBACKEND_QT4
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
#else
#define qPrintable(x) (x).local8Bit().data()
#define QLatin1String QString::fromLatin1
#define dir_native_separators(x) QDir::convertSeparators(x)
#define dir_entry_list(namefilter) entryList((namefilter))
#define ba_assign(otherba) duplicate((otherba).data(), (otherba).size())
#define ba_cat(baptr, moredata) klfqt3_ba_cat((baptr), (moredata))
#define ba_indexOf klfqt3_ba_indexof
#define ba_replace(baptr, start, len, replacement) klfqt3_ba_replace((baptr), (start), (len), (replacement))
//! Usage: <tt>buf_decl_ba(QBuffer buffer, mybytearray)</tt>
/** will expand to <tt>QBuffer buffer(&mybytearray)</tt> on Qt4. */
#define buf_decl_ba(bufdecl, ba) bufdecl((ba));
#define dev_WRITEONLY IO_WriteOnly
#define dev_READONLY IO_ReadOnly
#define dev_TEXTTRANSLATE IO_Translate
#define dev_write writeBlock
#define fi_suffix extension
#define fi_absolutePath() dirPath(true)
#define f_open_fp(fp) open(dev_WRITEONLY, (fp))
#define f_setFileName setName
#define f_filename name
#define f_error errorString
#define s_trimmed stripWhiteSpace
#define s_toUpper upper
#define s_toLatin1 latin1
#define s_toLocal8Bit local8Bit
#define s_indexOf find
#define str_split(string, sep, boolAllowEmptyEntries)		\
  QStringList::split((sep), (string), (boolAllowEmptyEntries))
#define list_indexOf(x) findIndex((x))
#define rx_indexin(str) search((str))
#define rx_indexin_i(str, i) search((str), (i))
#define img_settext(key, value)  setText((key), 0, (value))
#endif


// EXTRA DEFINITIONS

#ifndef KLFBACKEND_QT4
// QT 3

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


/* concatenate QByteArray's */
inline void klfqt3_ba_cat(QByteArray *ba, const QByteArray& moredata)
{
  if (moredata.isNull())
    return; // nothing to do

  int s1 = ba->size();
  ba->resize(s1 + moredata.size());
  char *it1, *it2;
  // copy contents of moredata into second part of ba
  it1 = ba->data() + s1;
  it2 = moredata.data();
  KLF_ASSERT_NOT_NULL(it1, "QByteArray data it1 is NULL in klfqt3_ba_cat!", return; ) ;
  KLF_ASSERT_NOT_NULL(it2, "QByteArray data it2 is NULL in klfqt3_ba_cat!", return; ) ;
  while ((it1 - ba->data()) < ba->size()) {
    *it1++ = *it2++;
  }
}

inline int klfqt3_ba_indexof(const QByteArray& ba, const char *str, int from = 0)
{
  int l = strlen(str);
  while (from < (int)ba.size()) {
    if (!strncmp(ba.data()+from, str, l))
      return from;
    ++from;
  }
  return -1;
}

inline void klfqt3_ba_replace_ba(QByteArray *ba, int start, int length, const QByteArray & replacement)
{
  KLF_DEBUG_BLOCK(KLF_FUNC_NAME) ;
  int s1 = ba->size();
  int k;
  int delta = replacement.size() - length;

  // see if we must grow or shrink
  if (delta > 0) {
    // we grow, copy the data further down, possibly overlapping; copy from end of shifted data
    ba->resize(s1 + delta);
    // k points to the "source" data
    for (k = s1 - 1; k >= start+length; --k)
      ba->at(k+delta) = ba->at(k);
  }
  if (delta < 0) {
    // we shrink, copy the data back up, possibly overlapping; copy from beginning of shifted data
    // k points to the "source" data
    for (k = start+length; k < s1; ++k)
      ba->at(k+delta) = ba->at(k); // !!! delta<0: i.e. k+delta == k-abs(delta) !!!
    ba->resize(s1 + delta);
  }

  // now, just copy replacement onto [start+length]
  for (k = 0; k < (int)replacement.size(); ++k)
    ba->at(start+k) = replacement.at(k);
  return;

}
inline void klfqt3_ba_replace(QByteArray *ba, int start, int length, const char * replacementstr)
{
  QByteArray replacement;
  int l = strlen(replacementstr);
  replacement.setRawData(replacementstr, l);
  klfDebugf(("replacement: '%s', size=%d", replacementstr, replacement.size())) ;
  klfqt3_ba_replace_ba(ba, start, length, replacement);
  replacement.resetRawData(replacementstr, l);
}

#else
// QT 4

inline int klfqt4_ba_indexof(const QByteArray& ba, const char *str, int from = 0)
{ return ba.indexOf(str, from); }

#endif





#endif // KLFQT34COMMON_H
