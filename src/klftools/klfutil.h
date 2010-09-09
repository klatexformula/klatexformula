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
#include <QUrl>
#include <QMap>
#include <QVariant>
#include <QProgressDialog>
#include <QLabel>
#include <QDomElement>

#include <klfdefs.h>


//! Ensure existence of a directory
KLF_EXPORT bool klfEnsureDir(const QString& dir);


//! Implements default equality tester with operator==
/** Used in eg. \ref klfMapIsIncludedIn() algorithm to test for items equality.
 *
 * See also \ref klfStrCaseEqualFunc.
 */
template<class Value>
class klfEqualFunc
{
public:
  bool operator()(const Value& a, const Value& b) { return a == b; }
};

//! implements an equality tester between strings
/** Can be used in eg. \ref klfMapIsIncludedIn().
 *
 * It is possible to specify whether to compare strings case sensitive or insensitive.
 *
 * See also \ref klfEqualFunc.
 */
class klfStrCaseEqualFunc
{
  Qt::CaseSensitivity cs;
public:
  klfStrCaseEqualFunc(Qt::CaseSensitivity caseSensitive) : cs(caseSensitive) { }
  bool operator()(const QString& a, const QString& b) { return QString::compare(a, b, cs) == 0; }
};


/** \brief Compares two QMap's for inclusion
 *
 * returns TRUE if all keys in \c a are present in \c b, with same values. It is still
 * possible however that map \c b contains more keys than \c a.
 *
 * This function uses a general value comparer helper, \c cfunc. You can give for example
 * \ref klfEqualFunc or \ref klfStrCaseEqualFunc.
 */
template<class Key, class Value, class ValCompareFunc>
inline bool klfMapIsIncludedIn(const QMap<Key,Value>& a, const QMap<Key,Value>& b,
			       ValCompareFunc cfunc = klfEqualFunc<Value>())
{
  typename QMap<Key,Value>::const_iterator iter;
  for (iter = a.begin(); iter != a.end(); ++iter) {
    if (!b.contains(iter.key()) || ! cfunc(b[iter.key()], iter.value())) {
      return false;
    }
  }
  // the map a is included in b
  return true;
}

//! Compares two QMap's for inclusion (values QString's)
/** Same as klfMapIsIncludedIn(const QMap<Key,Value>&, const QMap<Key,Value>&, ValCompareFunc), except
 * that values have to be QStrings, and that the value comparision is done by comparing strings for
 * equality, with case sensitivity \c cs.
 */
template<class Key>
inline bool klfMapIsIncludedIn(const QMap<Key,QString>& a, const QMap<Key,QString>& b, Qt::CaseSensitivity cs)
{
  return klfMapIsIncludedIn(a, b, klfStrCaseEqualFunc(cs));
}


//! Some relevant values for \ref klfUrlCompare()
/**
 * \note when multiple query items are given with the same key, ONLY the last one is taken into
 *   account and the other are ignored. For example, the two following URLs are equal:
 *   <pre>
 *     http://host.com/path/to/somewhere/file.php?x=ABC&otherkey=othervalue&x=XYZ    and
 *     http://host.com/path/to/somewhere/file.php?x=XYZ&otherkey=othervalue</pre>
 *
 *
 * \note By <i>base URL</i> we mean <i>what remains of the URL when all query items are stripped</i>.
 *   This is: scheme, host, port, user info, path, and fragment (ie. anchor). See \ref QUrl doc for
 *   more info.
 */
enum KlfUrlCompareFlag {
  /** \brief Urls are equal. The order of query items may be different, but the same are given
   * with the same values. */
  KlfUrlCompareEqual = 0x01,
  /** \brief Urls have same base URL. All query items in \c url1 are present in \c url2 with the
   *    same values, but although possibly they may be equal, \c url2 may specify more query items
   *    that are not present in \c url1. */
  KlfUrlCompareLessSpecific = 0x02,
  /** \brief Urls have same base URL. All query items in \c url2 are present in \c url1 with the
   *    same values, but although possibly they may be equal, \c url1 may specify more query items
   *    that are not present in \c url2. */
  KlfUrlCompareMoreSpecific = 0x04,
  /** \brief Urls have same base URL. Query items are ignored. */
  KlfUrlCompareBaseEqual = 0x08,

  /** \brief This is NOT a specific test. It modifies the behavior of klfUrlCompare() by instructing
   * it to compare query item _values_ in a non-case-sensitive manner (query item name (=key)
   * comparisions are always case sensitive). Namely, with this flag set \c "file:///path?name=value" is
   * equal to \c "file:///path?name=VALUE" but is not equal to \c "file:///path?NAME=value". */
  klfUrlCompareFlagIgnoreQueryItemValueCase = 0x1000000
};
//! Compares two URLs and returns some flags as to how they differ
/** The return value is an binary-OR'ed value of flags given in \ref KlfUrlCompareFlag.
 *
 * If the \c interestFlag parameter is set, only the tests that are given in \c interestFlags
 * are performed. The returned flags are those flags set in \c interestFlags that are true.
 *
 * If the \c interestQueryItems is set, all query items other than those specified in
 * \c interestQueryItems are ignored. If \c interestQueryItems is an empty list, no query items
 * are ignored, they are all taken into account.
 */
KLF_EXPORT uint klfUrlCompare(const QUrl& url1, const QUrl& url2, uint interestFlags = 0xffffffff,
			      const QStringList& interestQueryItems = QStringList());


/** \brief Generalized value matching
 *
 * This function tests to see if the value \c testForHitCandidateValue matches the value
 * \c queryValue according to the match flags \c flags.
 *
 * If you call this function repeatedly with the same \c queryValue, the query value may
 * be converted (unnecessarily) repeatedly to a string with <tt>queryValue.toString()</tt>.
 * To optimize this, you may cache that string and pass each time the string representation
 * for the \c queryValue as parameter to \c queryStringCache. If however a null string is passed,
 * the conversion is performed automatically.
 * */
KLF_EXPORT bool klfMatch(const QVariant& testForHitCandidateValue, const QVariant& queryValue,
			 Qt::MatchFlags flags, const QString& queryStringCache = QString());

/** Escapes every character in \c data that is not in the range 32-126 (included) as
 * \\xHH whith HH the hex code of the character. Backslashes are replaced by double-backslashes.
 */
KLF_EXPORT QByteArray klfDataToEscaped(const QByteArray& data);
/** Performs the exact inverse of \ref klfDataToEscaped().
 */
KLF_EXPORT QByteArray klfEscapedToData(const QByteArray& escaped);

/** Saves the variant \c value into a string, stored in Local 8-bit encoding text in QByteArray.
 * The saved string is both human and machine readable, ie. the exact string can be recast again
 * to a variant with \ref klfLoadVariantFromText().
 *
 * This function is aware of various QVariant formats, however maybe not all of them. The unknown
 * formats are stored machine-readable only, by sending the variant in a datastream, and protecting
 * the special characters from encoding artifacts (ascii chars only, proper escape sequences).
 *
 * If \c saveListAndMapsAsXML is FALSE (the default), then variant-lists and -maps are saved in
 * a format like \c "[element-1,element-2,...]" or \c "{key1=value1,key2=value2,....}", assuming
 * that all elements are of the same QVariant type. If \c saveListAndMapsAsXML is TRUE, then
 * variant lists and maps are saved with klfSaveVariantListToXML() and klfSaveVariantMapToXML(),
 * which enables you to save arbitrary combination of types.
 *
 * \note Note however that the saved string does NOT save the type. The data type must be known
 *   when loading the value. See \ref klfLoadVariantFromText().
 * */
KLF_EXPORT QByteArray klfSaveVariantToText(const QVariant& value, bool saveListAndMapsAsXML = false);

/** Loads the value stored in \c string into a variant of data type \c dataTypeName. The string
 * is parsed and the returned variant will by of the given type name, or invalid if the string
 * could not be parsed.
 *
 * \note The data type must be known when loading the value. It may not necessarily exactly be
 *   guessed by looking at \c string.
 *
 * Example use: to save/store settings values in QSettings in a human-read/writable format.
 *
 * If \c dataTypeName is a variant list or map, then please specify the type of the values in
 * the list or map (this function assumes all objects in list or map have the same type). As
 * a special case, you can pass \c "XML" to load the list or map data with klfLoadVariantListFromXML()
 * or klfLoadVariantMapFromXML(), which enables you to save arbitrary combination of types.
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

//! Lossless save of full map to XML with type information
KLF_EXPORT QDomElement klfSaveVariantMapToXML(const QVariantMap& vmap, QDomElement xmlNode);
//! Load a map saved with klfSaveVariantMapToXML()
KLF_EXPORT QVariantMap klfLoadVariantMapFromXML(const QDomElement& xmlNode);

//! Lossless save of full list to XML with type information
KLF_EXPORT QDomElement klfSaveVariantListToXML(const QVariantList& vlist, QDomElement xmlNode);
//! Load a list saved with klfSaveVariantListToXML()
KLF_EXPORT QVariantList klfLoadVariantListFromXML(const QDomElement& xmlNode);




/** Returns the file path represented in \c url, interpreted as an (absolute) path to
 * a local file.
 *
 * Under windows, this ensures that there is no slash preceeding the drive letter, eg.
 * fixes "/C:/..." to "C:/...", but keeps forward-slashes.
 */
KLF_EXPORT QString klfUrlLocalFilePath(const QUrl& url);




#endif
