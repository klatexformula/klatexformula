/***************************************************************************
 *   file klfdefs.cpp
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

#include <stdlib.h>

#include <qdir.h>
#include <qfile.h>
#include <qfileinfo.h>
#include <qregexp.h>

#include "klfdefs.h"

static char __klf_version_string[] = KLF_VERSION_STRING;

KLF_EXPORT const char * klfVersion()
{
  return __klf_version_string;
}


#ifndef KLFBACKEND_QT4
#define qPrintable(x) (x).local8Bit().data()
#define QLatin1String QString::fromLatin1
#define toLocal8Bit local8Bit
// QDir::toNativeSeparators() -> QDir::convertSeparators()
#define toNativeSeparators convertSeparators
#define SPLIT_STRING(x, sep, boolAllowEmptyEntries)		\
  QStringList::split((sep), (x), (boolAllowEmptyEntries))
#define list_indexOf(list, x) (list).findIndex((x))
#else
#define SPLIT_STRING(x, sep, boolAllowEmptyEntries)	\
  (x).split((sep), (boolAllowEmptyEntries) ? QString::KeepEmptyParts : QString::SkipEmptyParts)
#define list_indexOf(list, x) (list).indexOf((x))
#endif

// declared in klfdefs.h

#ifdef KLF_DEBUG
void __klf_debug_time_print(QString str)
{
  struct timeval tv;
  gettimeofday(&tv, NULL);
  qDebug("%03ld.%06ld : %s", tv.tv_sec % 1000, tv.tv_usec, qPrintable(str));
}
KLF_EXPORT QDebug& __klf_debug_time_print_str(QDebug str)
{
  struct timeval tv;
  gettimeofday(&tv, NULL);
  char temp[64];
  sprintf(temp, "%03ld.%06ld", tv.tv_sec % 1000, tv.tv_usec);
  return str<<temp<<": ";
}

#endif

KLF_EXPORT QByteArray klfShortFuncSignature(const QByteArray& ba_funcname)
{
  QString funcname(ba_funcname);
  // returns the section between the last space before the first open paren and the first open paren
  int iSpc, iParen;
#ifdef KLFBACKEND_QT4
  iParen = funcname.indexOf('(');
  iSpc = funcname.lastIndexOf(' ', iParen-2);
#else
  iParen = funcname.find('(');
  iSpc = funcname.findRev(' ', iParen-2);
#endif
  if (iSpc == -1 || iParen == -1 || iSpc > iParen) {
    qWarning("klfShortFuncSignature('%s'): Signature parse error!", qPrintable(funcname));
    return ba_funcname;
  }
  // shorten name
  QString f = funcname.mid(iSpc+1, iParen-(iSpc+1));
  QByteArray data = f.toLocal8Bit();
  return data;
}


KLFDebugBlock::KLFDebugBlock(const QString& blockName)
  : pBlockName(blockName), pPrintMsg(true)
{
  qDebug("%s: block begin", qPrintable(pBlockName));
}
KLFDebugBlock::KLFDebugBlock(bool printmsg, const QString& blockName)
  : pBlockName(blockName), pPrintMsg(printmsg)
{
  if (printmsg)
    qDebug("%s: block begin", qPrintable(pBlockName));
}
KLFDebugBlock::~KLFDebugBlock()
{
  if (pPrintMsg)
    qDebug("%s: block end", qPrintable(pBlockName));
}
KLFDebugBlockTimer::KLFDebugBlockTimer(const QString& blockName)
  : KLFDebugBlock(false, blockName)
{
#ifdef KLF_DEBUG
  __klf_debug_time_print(QString("%1: block begin").arg(pBlockName));
#endif
}
KLFDebugBlockTimer::~KLFDebugBlockTimer()
{
#ifdef KLF_DEBUG
  __klf_debug_time_print(QString("%1: block end").arg(pBlockName));
#endif
}



// ----------------------------------------------------------------------




KLF_EXPORT QString KLFSysInfo::arch()
{
  return KLF_CMAKE_ARCH;
}

KLF_EXPORT KLFSysInfo::Os KLFSysInfo::os()
{
#if defined(Q_OS_LINUX)
  return Linux;
#elif defined(Q_OS_DARWIN)
  return MacOsX;
#elif defined(Q_OS_WIN32)
  return Win32;
#else
  return OtherOs;
#endif
}

KLF_EXPORT QString KLFSysInfo::osString(Os sysos)
{
  switch (sysos) {
  case Linux: return QLatin1String("linux");
  case MacOsX: return QLatin1String("macosx");
  case Win32: return QLatin1String("win32");
  case OtherOs: return QString();
  default: ;
  }
  qWarning("KLFSysInfo::osString: unknown OS: %d", sysos);
  return QString();
}


QStringList klf_version_suffixes =
  QStringList() << "a" << "alpha" << "b" << "beta" << "p" << "pre" << "preview" << "RC" << "rc"
/* */           << "" // empty suffix or any unrecognized suffix
/* */           << "dev" << "devel";

static int __klf_version_compare_suffix_words(QString w1, QString w2)
{
  // a list of known words
  const QStringList& words = klf_version_suffixes;
  // now compare the words
  int borderBeforeAfter = list_indexOf(words, "");
  if (borderBeforeAfter < 0)
    qWarning("klfVersionCompare: suffix words list doesn't contain \"\"!");
  int i1 = list_indexOf(words, w1);
  int i2 = list_indexOf(words, w2);
  if (i1 == -1 && i2 == -1)
    return QString::compare(w1, w2);
  if (i2 == -1)
    return i1 < borderBeforeAfter ? -1 : +1;
  if (i1 == -1)
    return i2 < borderBeforeAfter ? +1 : -1;
  // both are recognized words
  return i1 - i2;
}


KLF_EXPORT int klfVersionCompare(const QString& v1, const QString& v2)
{
  qDebug("klfVersionCompare(): Comparing versions %s and %s", qPrintable(v1), qPrintable(v2));
  //           *1     2  *3     4  *5    *6
  QRegExp rx1("^(\\d+)(\\.(\\d+)(\\.(\\d+)([a-zA-Z]+\\d*)?)?)?$");
  QRegExp rx2(rx1);
  if (!rx1.exactMatch(v1)) {
    qWarning("klfVersionLessThan: Invalid version number format: %s", qPrintable(v1));
    return -200;
  }
  if (!rx2.exactMatch(v2)) {
    qWarning("klfVersionLessThan: Invalid version number format: %s", qPrintable(v2));
    return -200;
  }
  int maj1 = rx1.cap(1).toInt();
  int maj2 = rx2.cap(1).toInt();
  if (maj1 != maj2)
    return maj1 - maj2;
  bool hasmin1 = !rx1.cap(2).isEmpty();
  bool hasmin2 = !rx2.cap(2).isEmpty();
  if ( ! hasmin1 && ! hasmin2 )
    return 0; // equal
  if ( ! hasmin1 && hasmin2 )
    return -1; // 3 < 3.x
  if ( hasmin1 && ! hasmin2 )
    return +1; // 3.x > 3
  int min1 = rx1.cap(3).toInt();
  int min2 = rx2.cap(3).toInt();
  if ( min1 != min2 )
    return min1 - min2;

  bool hasrel1 = !rx1.cap(4).isEmpty();
  bool hasrel2 = !rx2.cap(4).isEmpty();
  if ( ! hasrel1 && ! hasrel2 )
    return 0; // equal
  if ( ! hasrel1 && hasrel2 )
    return -1; // 3.x < 3.x.y
  if ( hasrel1 && ! hasrel2 )
    return +1; // 3.x.y > 3.x
  int rel1 = rx1.cap(5).toInt();
  int rel2 = rx2.cap(5).toInt();
  if ( rel1 != rel2 )
    return rel1 - rel2;

  QString suffix1 = rx1.cap(6);
  QString suffix2 = rx2.cap(6);

  //  qDebug("Suffix1=%s, suffix2=%s", qPrintable(suffix1), qPrintable(suffix2));

  if (suffix1 == suffix2)
    return 0; // equal

  //             1          2
  QRegExp rxs1("^([a-zA-Z]*)(\\d*)$");
  QRegExp rxs2(rxs1);
  rxs1.exactMatch(suffix1); // must necessarily match, already matched global regex
  rxs2.exactMatch(suffix2);

  QString w1 = rxs1.cap(1);
  QString w2 = rxs2.cap(1);
  QString ns1 = rxs1.cap(2);
  QString ns2 = rxs2.cap(2);

  int cmp = __klf_version_compare_suffix_words(w1, w2);
  if (cmp != 0)
    return cmp; // words are enough to make the difference

  // the words are the same -> compare ns1<->ns2
  if (ns1.isEmpty()) {
    if (ns2.isEmpty())
      return 0; // equal
    return -1;
  }
  if (ns2.isEmpty()) {
    return +1;
  }

  int n1 = ns1.toInt();
  int n2 = ns2.toInt();
  return n1 - n2;
}

KLF_EXPORT bool klfVersionCompareLessThan(const QString& v1, const QString& v2)
{
  return klfVersionCompare(v1,v2) < 0;
}



// negative limit means "no limit"
static QStringList __search_find_test(const QString& root, const QStringList& pathlist,
				      int level, int limit)
{
  if (limit == 0)
    return QStringList();

  if (limit < 0)
    limit = -1; // normalize negative values to -1 (mostly cosmetic...)

  QStringList newpathlist = pathlist;
  // our level: levelpathlist contains items in pathlist from 0 to level-1 inclusive.
  QStringList levelpathlist;
  int k;
  for (k = 0; k < level; ++k) { levelpathlist << newpathlist[k]; }
  // the dir/file at our level:
  QString flpath = root+levelpathlist.join("/");
  QFileInfo flinfo(flpath);
  if (flinfo.isDir()) {
    QDir d(flpath);
    QStringList entries;
#ifdef KLFBACKEND_QT4
    entries = d.entryList(QStringList()<<pathlist[level]);
#else
    entries = d.entryList(pathlist[level]);
#endif
    QStringList hitlist;
    for (k = 0; k < entries.size(); ++k) {
      newpathlist[level] = entries[k];
      hitlist += __search_find_test(root, newpathlist, level+1, limit - hitlist.size());
      if (limit >= 0 && hitlist.size() >= limit) // reached limit
	break;
    }
    return hitlist;
  }
  if (flinfo.exists()) {
    return QStringList() << QDir::toNativeSeparators(root+pathlist.join("/"));
  }
  return QStringList();
}

// returns at most limit results matching wildcard_expression (which is given as absolute path
// with wildcards)
KLF_EXPORT QStringList klfSearchFind(const QString& wildcard_expression, int limit)
{
  QString expr;
#ifdef KLFBACKEND_QT4
  expr = QDir::fromNativeSeparators(wildcard_expression);
#else
  expr = wildcard_expression;  expr.replace(QDir::separator(), "/");
#endif
  QStringList pathlist = SPLIT_STRING(expr, "/", false);
  QString root = "/";
  static QRegExp driveregexp("^[A-Za-z]:$");
  if (driveregexp.exactMatch(pathlist[0])) {
    // Windows System with X: drive letter
    root = pathlist[0]+"/";
    pathlist.pop_front();
  }
  return __search_find_test(root, pathlist, 0, limit);
}

// smart search PATH that will interpret wildcards in PATH+extra_path and return the first matching
// executable
KLF_EXPORT QString klfSearchPath(const QString& prog, const QString& extra_path)
{
  static const QString PATH = getenv("PATH");
  static const QString pathsep = QString("")+KLF_PATH_SEP;

  QString path = PATH;
  if (!extra_path.isEmpty())
    path += pathsep + extra_path;

  const QStringList paths = SPLIT_STRING(path, pathsep, true);
  QString test;
  int k, j;
  for (k = 0; k < paths.size(); ++k) {
    QStringList hits = klfSearchFind(paths[k]+"/"+prog);
    for (j = 0; j < hits.size(); ++j) {
      if ( QFileInfo(hits[j]).isExecutable() ) {
	return hits[j];
      }
    }
  }
  return QString::null;
}





