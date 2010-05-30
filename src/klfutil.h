/***************************************************************************
 *   file klfutil.h
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


#ifndef KLFUTIL_H
#define KLFUTIL_H

#include <QString>
#include <QStringList>
#include <QVariant>

#include <klfdefs.h>

// utility functions


//! Get System Information
namespace KLFSysInfo
{
  enum Os { Linux, Win32, MacOsX, OtherOs };

  inline int sizeofVoidStar() { return sizeof(void*); }

  //! Returns one of \c "x86" or \c "x86_64", or \c QString() for other/unknown
  KLF_EXPORT QString arch();

  KLF_EXPORT Os os();

  //! Returns one of \c "win32", \c "linux", \c "macosx", or QString() for other/unknown
  KLF_EXPORT QString osString();
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
 *  - unrecognized suffixes are compared lexicographically.
 *
 * Some examples:
 * <pre>   "3.1.0" < "3.1.2"
 *   "2" < "2.1" < "2.1.1"
 *   "3.0.0alpha2" < "3.0.0"
 *   "3.0.2" < "3.0.3alpha0"
 * </pre>
 */
KLF_EXPORT int klfVersionCompare(const QString& v1, const QString& v2);

//! Same as <tt>klfVersionCompare(v1,v2) &lt; 0</tt>
KLF_EXPORT bool klfVersionCompareLessThan(const QString& v1, const QString& v2);

KLF_EXPORT bool klfEnsureDir(const QString& dir);


KLF_EXPORT QStringList klfSearchFind(const QString& wildcard_expression, int limit = -1);
KLF_EXPORT QString klfSearchPath(const QString& prog, const QString& extra_path = "");


KLF_EXPORT QByteArray klfDataToEscaped(const QByteArray& data);
KLF_EXPORT QByteArray klfEscapedToData(const QByteArray& escaped);

/** Saves the variant \c value into a string, stored in Local 8-bit encoding text in QByteArray.
 * The saved string is both human and machine readable, ie. the exact string can be recast again
 * to a variant with \ref klfLoadVariantFromText().
 *
 * This function is aware of various QVariant formats, however maybe not all of them. The unknown
 * formats are stored machine-readable only, by sending the variant in a datastream, and protecting
 * the special characters from encoding artifacts (ascii chars only, proper escape sequences).
 *
 * \note Note however that the saved string does NOT save the type. The data type must be known
 *   when loading the value. See \ref klfLoadVariantFromText().
 * */
KLF_EXPORT QByteArray klfSaveVariantToText(const QVariant& value);

/** Loads the value stored in \c string into a variant of data type \c dataTypeName. The string
 * is parsed and the returned variant will by of the given type name, or invalid if the string
 * could not be parsed.
 *
 * \note The data type must be known when loading the value. It may not necessarily exactly be
 *   guessed by looking at \c string.
 *
 * Example use: to save/store settings values in QSettings in a human-read/writable format.
 *
 * If \c dataTypeName is a list or a map, then please specify the type of the values in the list
 * or map (this function assumes all objects in list or map have the same type).
 *
 * See also \ref klfSaveVariantToText().
 */
KLF_EXPORT QVariant klfLoadVariantFromText(const QByteArray& string, const char * dataTypeName,
					   const char *listOrMapTypeName = NULL);



template<class T>
inline QVariantList klfListToVariantList(const QList<T>& list)
{
  QVariantList l;
  int k;
  for (k = 0; k < list.size(); ++k)
    l << QVariant::fromValue<T>(list[k]);

  return l;
}


template<class T>
inline QList<T> klfVariantListToList(const QVariantList& vlist)
{
  QList<T> list;
  int k;
  for (k = 0; k < vlist.size(); ++k) {
    list << vlist[k].value<T>();
  }
  return list;
}


#endif
