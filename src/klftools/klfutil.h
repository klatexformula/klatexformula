/***************************************************************************
 *   file klfutil.h
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
#include <QTextFormat>

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



/** \brief Find files matching a path with wildcards
 *
 * This function returns at most \c limit file names that match the given \c wildcard_expression.
 * The latter may be any absolute path in which (any number of) \c * and \c ? wildcards may be
 * placed.
 *
 * This function splits the \c wildcard_expression at \c '/' characters, and by starting at the
 * root directory, recursively exploring all directories matching the given section of the pattern.
 * (native '\\' separators on windows are appropriately converted to universal \c '/', so you don't
 * have to worry about passing '\\'-style paths).
 *
 * For example, searching for <tt>&quot;/usr/lib*</tt><tt>/kde4/kate*.so&quot;</tt> will start looking in
 * the root directory for a directory named \c "usr", in which a directory matching \c "lib*" is
 * searched for. In each of those matches, a directory named \c "kde4" is searched for, in which
 * files named \c "lib*.so.*" are listed. All found matches are returned (up to a given \c limit
 * number of entries if \c limit is positive).
 *
 * The drive letter in \c wildcard_expression on windows may not contain a wildcard.
 */
KLF_EXPORT QStringList klfSearchFind(const QString& wildcard_expression, int limit = -1);

/** \brief Smart executable searching in a given path list with wildcards
 *
 * This function looks for an executable named \c programName. It looks in the directories given in
 * argument \c extra_path, and in the system environment variable \c PATH. \c extra_path and \c PATH
 * are assumed to be a colon-separated list of directories (semicolon-separated on windows, see
 * \ref KLF_PATH_SEP). Each given directory may contain wildcards (in particular, wildcards in
 * \c PATH are also parsed). \c programName itself may also contain wildcards.
 *
 * This function splits \c extra_path and \c PATH into a directory list, and then, for each directory
 * in that list, calls \ref klfSearchFind() with as argument the string
 * <tt><i>&quot;&lt;directory></i>/<i>&lt;programName></i>&quot;</tt>. This function then returns the
 * first result that is an executable file (this check is explicitely performed).
 */
KLF_EXPORT QString klfSearchPath(const QString& prog, const QString& extra_path = "");



/** \brief Returns absolute path to \c path as seen from \c reference
 *
 * If \c path is absolute, then \c path is returned as is. Otherwise, an absolute
 * path constructed by concatenating \c path to \c reference is returned.
 *
 * If \c reference is empty, then the reference is considered to be the application's
 * location, see \ref QCoreApplication::applicationDirPath().
 */
KLF_EXPORT QString klfPrefixedPath(const QString& path, const QString& reference = QString());


/** Returns the file path represented in \c url, interpreted as an (absolute) path to
 * a local file.
 *
 * On windows, this ensures that there is no slash preceeding the drive letter, eg.
 * fixes "/C:/..." to "C:/...", but keeps forward-slashes.
 */
KLF_EXPORT QString klfUrlLocalFilePath(const QUrl& url);


/** Applies operation \c MapOp to each element of list \c list, and returns the 'mapped' list.
 * This utility can be considered a simplistic version of the 'map' primitive in PERL. */
template<class T, class MapOp>
inline QList<T> klfListMap(const QList<T>& list, MapOp op)
{
  QList<T> l;
  for(typename QList<T>::const_iterator it = list.begin(); it != list.end(); ++it) {
    l << op(*it);
  }
  return l;
}

template<class T> inline QList<T> klfMkList(const T& a) { return QList<T>()<<a; }
template<class T> inline QList<T> klfMkList(const T& a, const T& b) { return QList<T>()<<a<<b; }
template<class T> inline QList<T> klfMkList(const T& a, const T& b, const T& c) { return QList<T>()<<a<<b<<c; }
template<class T>
inline QList<T> klfMkList(const T& a, const T& b, const T& c, const T& d) { return QList<T>()<<a<<b<<c<<d; }


class KLFTargeter;

class KLF_EXPORT KLFTarget {
public:
  KLFTarget() : pTargetOf() { }
  virtual ~KLFTarget();

protected:
  QList<KLFTargeter*> pTargetOf;
  friend class KLFTargeter;
};

class KLF_EXPORT KLFTargeter {
public:
  KLFTargeter() : pTarget(NULL) { }
  virtual ~KLFTargeter();

  virtual void setTarget(KLFTarget *target);

protected:
  KLFTarget *pTarget;
  friend class KLFTarget;
};




/** \brief Stores a pointer to an object with refcount
 *
 * This class provides a normal datatype (with default constructor,
 * copy constructor, assignment, ...) that stores a pointer to a
 * structure that provides the functions ref() and deref() to track
 * reference counts.
 *
 * When this object is copied, or a pointer is assigned, then the
 * pointed object's ref() function is called. When another pointer
 * is assigned, or when this object is destroyed, then the function
 * deref() is called on the previously pointed object, and the
 * object is possibly \c delete'd if needed and required.
 *
 * Automatic object deletion upon zero refcount is optional, see
 * \ref autoDelete() and \ref setAutoDelete(). It is on by default.
 *
 * When constructed with the default constructor, pointers are initialized
 * to NULL.
 *
 * The copy constructor and the assignment operator also preserves autodelete
 * setting, eg.
 * \code
 *  KLFRefPtr<Obj> ptr1 = ...;
 *  ptr1.setAutoDelete(false);
 *  KLFRefPtr<Obj> ptr2(ptr1);
 *  // ptr2.autoDelete() == false
 * \endcode
 *
 * The pointed/referenced object must:
 *  - provide a ref() function (no arguments, return type unimportant)
 *  - provide a deref() function (no arguments). Its return type, when
 *    cast into an \c int, is strictly positive as long as the object
 *    is still referenced.
 *
 * Example:
 * \code
 * class MyObj {
 *   QString name;
 *   int refcount;
 * public:
 *   MyObj(const QString& s) : name(s), refcount(0) { }
 *   int ref() { return ++refcount; }
 *   int deref() { return --refcount; }
 *   ...
 *   QString objname() const { return name; }
 * };
 * 
 * int main() {
 *   KLFRefPtr<MyObj> ptr;
 *   KLFRefPtr<MyObj> ptr2;
 *   ptr = new MyObj("Alice");
 *   ptr2 = ptr;  // this will increase "Alice"'s refcount by one
 *   ptr = new MyObj("Bob"); // "Alice" is still pointed by ptr2, so it is not yet deleted.
 *   ptr2 = NULL; // now "Alice" is deleted (refcount reached zero)
 *   ptr = obj_random_name();
 *   ptr2 = obj_random_name();
 *   do_something(ptr):
 *   do_something_2(ptr2);
 *   // ptr->field works as expected:
 *   printf("ptr points on object name=%s\n", qPrintable(ptr->objname()));
 * }
 * KLFRefPtr<MyObj> obj_random_name() {
 *   static QStringList names
 *      = QStringList()<<"Marty"<<"Jane"<<"Don"<<"John"<<"Phoebe"<<"Matthew"<<"Melissa"<<"Jessica"
 *   KLFRefPtr<MyObj> o = new MyObj(names[rand()%names.size()]);
 *   // the pointer survives the `return' statement because of refcount increase in copy constructor
 *   return o;
 * }
 * void do_something(MyObj *object)  {  ...  }
 * void do_something_2(KLFRefPtr<MyObj> ptr)  {  ...  }
 * 
 * \endcode
 */
template<class T> class KLFRefPtr
{
public:
  /** The pointer type. Alias for <tt>T *</tt>. */
  typedef T* Pointer;

  KLFRefPtr() : p(NULL), autodelete(true)
  {
    KLF_DEBUG_BLOCK(KLF_FUNC_NAME) ;
  }

  KLFRefPtr(const KLFRefPtr& copy) : p(copy.p), autodelete(copy.autodelete)
  {
    KLF_DEBUG_BLOCK(KLF_FUNC_NAME+" [copy]") ;
    set();
  }

  ~KLFRefPtr()
  {
    KLF_DEBUG_BLOCK(KLF_FUNC_NAME) ;
    unset();
  }

  T * ptr() { return p; }
  const T * ptr() const { return p; }

  bool autoDelete() const { return autodelete; }
  void setAutoDelete(bool on) { autodelete = on; }

  KLFRefPtr<T>& operator=(const KLFRefPtr<T>& other)
  {
    KLF_DEBUG_BLOCK(KLF_FUNC_NAME+"(KLFRefPtr<T*>)") ;
    this->operator=(other.p);
    autodelete = other.autodelete;
    return *this;
  }
  template<class OtherPtr> KLFRefPtr<T>& operator=(OtherPtr aptr)
  {
    KLF_DEBUG_BLOCK(KLF_FUNC_NAME+"(OtherPtr)") ;
    return this->operator=(static_cast<Pointer>(aptr));
  }
  KLFRefPtr<T>& operator=(Pointer newptr)
  {
    KLF_DEBUG_BLOCK(KLF_FUNC_NAME+"(Pointer)") ;
    klfDbg("new pointer: "<<newptr<<"; old pointer was "<<p) ;
    if (newptr == p) // no change
      return *this;
    unset();
    p = newptr;
    set();
    return *this;
  };
  
  inline operator T *()
  {  return p;  }
  inline operator const T *() const
  {  return p;  }

  /*  inline T & operator*()
      {  return *p; }
      inline const T & operator*() const
      {  return *p; } */

  inline Pointer operator->()
  {  return p;  }
  inline Pointer operator->() const
  {  return p;  }

  template<class OtherPtr>
  inline OtherPtr dyn_cast() { return dynamic_cast<OtherPtr>(p); }

  template<class OtherPtr>
  inline const OtherPtr dyn_cast() const { return dynamic_cast<const OtherPtr>(p); }

private:
  void operator+=(int n) { }
  void operator-=(int n) { }

  /** The pointer itself */
  Pointer p;
  /** Whether to auto-delete the object once its deref() function
   * returns <= 0. */
  bool autodelete;

  void unset() {
    KLF_DEBUG_BLOCK(KLF_FUNC_NAME) ;
    if (p != NULL) {
      int n = p->deref();
      klfDbg(p<<": deref()! n="<<n<<";  autodelete="<<autodelete) ;
      if (autodelete && n <= 0) {
	klfDbg("Deleting at refcount="<<n<<".") ;
	delete p;
      }
      p = NULL;
    }
  }
  void set() {
    KLF_DEBUG_BLOCK(KLF_FUNC_NAME) ;
    if (p != NULL) {
      int n = p->ref();
      klfDbg(p<<": ref()! n="<<n) ;
    }
  }

};


// -----



template<class T>
struct KLFVariantConverter {
  static QVariant convert(const T& value)
  {
    return QVariant::fromValue<T>(value);
  }
  static T recover(const QVariant& variant)
  {
    return variant.value<T>();
  }
};
template<class T>
struct KLFVariantConverter<QList<T> > {
  static QVariant convert(const QList<T>& list)
  {
    return QVariant::fromValue<QVariantList>(klfListToVariantList(list));
  }
  static QList<T> recover(const QVariant& variant)
  {
    return klfVariantListToList<T>(variant.toList());
  }
};
template<>
struct KLFVariantConverter<QTextCharFormat> {
  static QVariant convert(const QTextCharFormat& value)
  {
    return QVariant::fromValue<QTextFormat>(value);
  }
  static QTextCharFormat recover(const QVariant& variant)
  {
    return variant.value<QTextFormat>().toCharFormat();
  }
};




/** \brief Call this from your main program, so that the klftools resource is initialized.
 * \hideinitializer
 */
#define KLFTOOLS_INIT				\
  Q_INIT_RESOURCE(klftoolsres)



#endif
