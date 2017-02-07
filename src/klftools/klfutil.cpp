/***************************************************************************
 *   file klfutil.cpp
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

#include <stdlib.h>

#include <QFile>
#include <QDir>
#include <QLibraryInfo>
#include <QUrl>
#include <QUrlQuery>
#include <QMessageBox>
#include <QPushButton>
#include <QApplication>
#include <QDesktopWidget>
#include <QProcess>

#include "klfutil.h"
#include "klfsysinfo.h"


KLF_EXPORT bool klfEnsureDir(const QString& dir)
{
  if ( ! QDir(dir).exists() ) {
    bool r = QDir("/").mkpath(dir);
    if ( ! r ) {
      qWarning("Can't create local directory %s!", qPrintable(dir));
      return false;
    }
    // set permissions to "rwx------"
    r = QFile::setPermissions(dir, QFile::ReadOwner|QFile::WriteOwner|QFile::ExeOwner|
			      QFile::ReadUser|QFile::WriteUser|QFile::ExeUser);
    if ( ! r ) {
      qWarning("Can't set permissions to local config directory `%s' !", qPrintable(dir));
      return false;
    }
  }
  return true;
}



static QMap<QString,QString> klf_url_query_items_map(const QUrl& url,
						     const QStringList& interestQueryItems)
{
  QList<QPair<QString,QString> > qitems = QUrlQuery(url).queryItems();
  QMap<QString,QString> map;
  int k;
  for (k = 0; k < qitems.size(); ++k) {
    const QPair<QString,QString>& p = qitems[k];
    if (interestQueryItems.isEmpty() || interestQueryItems.contains(p.first))
      map[p.first] = p.second;
  }
  return map;
}



KLF_EXPORT uint klfUrlCompare(const QUrl& url1, const QUrl& url2, uint interestFlags,
			      const QStringList& interestQueryItems)
{
  KLF_DEBUG_BLOCK(KLF_FUNC_NAME);
  klfDbg( ": 1="<<url1<<"; 2="<<url2<<"; interestflags="<<interestFlags<<"; int.q.i="
	  <<interestQueryItems ) ;
  uint compareflags = 0x00;

  Qt::CaseSensitivity queryItemValsCS = Qt::CaseSensitive;
  if (interestFlags & klfUrlCompareFlagIgnoreQueryItemValueCase)
    queryItemValsCS = Qt::CaseInsensitive;

  QMap<QString,QString> qitems_map1;
  QMap<QString,QString> qitems_map2;

  QUrl u1 = url1;
  QUrl u2 = url2;
  u1.setQuery("");
  u2.setQuery("");

  klfDbg( " after q-i-stripping: u1="<<u1<<"; u2="<<u2 ) ;

  if (interestFlags &
      (KlfUrlCompareEqual|KlfUrlCompareLessSpecific|KlfUrlCompareMoreSpecific)) {
    // have an operation that needs these maps, so load them
    qitems_map1 = klf_url_query_items_map(url1, interestQueryItems);
    qitems_map2 = klf_url_query_items_map(url2, interestQueryItems);
  }

  if (interestFlags & KlfUrlCompareEqual) {
    // test equality
    if (u1 == u2 && qitems_map1 == qitems_map2)
      compareflags |= KlfUrlCompareEqual;
  }

  if (interestFlags & KlfUrlCompareLessSpecific) {
    // test url1 is less specific than url2   <-> url1 items are included in those of url2
    if (u1 == u2) {
      bool ok = klfMapIsIncludedIn(qitems_map1, qitems_map2, queryItemValsCS);
      if (ok)
	compareflags |= KlfUrlCompareLessSpecific;
    }
  }
  if (interestFlags & KlfUrlCompareMoreSpecific) {
    // test url1 is more specific than url2  <-> url2 items are included in those of url1
    if (u1 == u2) {
      bool ok = klfMapIsIncludedIn(qitems_map2, qitems_map1, queryItemValsCS);
      if (ok)
	compareflags |= KlfUrlCompareMoreSpecific;
    }
  }

  if (interestFlags & KlfUrlCompareBaseEqual) {
    if (u1 == u2)
      compareflags |= KlfUrlCompareBaseEqual;
  }

  klfDbg( "... and the result is compareflags="<<compareflags ) ;
  return compareflags;
}


// ------------------------------------------------------------



// ignores: flags: Recurse, Wrap. (!)
KLF_EXPORT bool klfMatch(const QVariant& testForHitCandidateValue, const QVariant& queryValue,
			 Qt::MatchFlags flags, const QString& queryStringCache /* = QString()*/)
{
  //
  // *** NOTE ***
  //   code inspired from Qt's QAbstractItemModel::match() defined in
  //   src/corelib/kernel/qabstractitemmodel.cpp
  //

  uint matchType = flags & 0x0F;
  Qt::CaseSensitivity cs = (flags & Qt::MatchCaseSensitive)
    ? Qt::CaseSensitive
    : Qt::CaseInsensitive;

  const QVariant& v = testForHitCandidateValue; // the name is a bit long :)
  
  // QVariant based matching
  if (matchType == Qt::MatchExactly)
    return (queryValue == v);

  // QString based matching
  QString text = !queryStringCache.isNull() ? queryStringCache : queryValue.toString();
  QString t = v.toString();
  switch (matchType) {
  case Qt::MatchRegExp:
    return (QRegExp(text, cs).exactMatch(t));
  case Qt::MatchWildcard:
    return (QRegExp(text, cs, QRegExp::Wildcard).exactMatch(t));
  case Qt::MatchStartsWith:
    return (t.startsWith(text, cs));
  case Qt::MatchEndsWith:
    return (t.endsWith(text, cs));
  case Qt::MatchFixedString:
    return (QString::compare(t, text, cs) == 0);
  case Qt::MatchContains:
  default:
    return (t.contains(text, cs));
  }
}


// ----------------------------------------------------


// negative limit means "no limit"
static QStringList __search_find_test(const QString& root, const QStringList& pathlist,
				      int level, int limit)
{
  if (limit == 0)
    return QStringList();

  if (limit < 0)
    limit = -1; // normalize negative values to -1 (mostly cosmetic...)

  klfDebugf(("root=`%s', pathlist=`%s', level=%d, limit=%d", qPrintable(root), qPrintable(pathlist.join("\t")),
	     level, limit));

  QStringList newpathlist = pathlist;
  // our level: levelpathlist contains items in pathlist from 0 to level-1 inclusive.
  QStringList levelpathlist;
  int k;
  for (k = 0; k < level; ++k) { levelpathlist << newpathlist[k]; }
  // the dir/file at our level:
  QString flpath = root+levelpathlist.join("/");
  klfDebugf(("our path = `%s' ...", qPrintable(flpath)));
  QFileInfo flinfo(flpath);
  if (flinfo.isDir() && level < pathlist.size()) { // is dir, and we still have path level to explore
    klfDebugf(("... is dir.")) ;
    QDir d(flpath);
    QStringList entries;
    entries = d.entryList(QStringList()<<pathlist[level], QDir::AllEntries|QDir::System|QDir::Hidden);
    if (entries.size()) {
      klfDebugf(("got entry list: %s", qPrintable(entries.join("\t"))));
    }
    QStringList hitlist;
    for (k = 0; k < (int)entries.size(); ++k) {
      newpathlist[level] = entries[k];
      hitlist += __search_find_test(root, newpathlist, level+1, limit - hitlist.size());
      if (limit >= 0 && (int)hitlist.size() >= limit) // reached limit
	break;
    }
    return hitlist;
  }
  if (flinfo.exists()) {
    klfDebugf(("... is existing file.")) ;
    return QStringList() << QDir::toNativeSeparators(root+pathlist.join("/"));
  }
  klfDebugf(("... is invalid.")) ;
  return QStringList();
}








KLF_EXPORT QStringList klfSearchFind(const QString& wildcard_expression, int limit)
{
  klfDbg("looking for "+wildcard_expression) ;

  QString expr;
  expr = QDir::fromNativeSeparators(wildcard_expression);
  QStringList pathlist = expr.split("/", QString::SkipEmptyParts);
  QString root = "/";
  // drive regular expression, or simply QResource :/path
  static QRegExp driveregexp("^[A-Za-z]?:$");
  if (driveregexp.exactMatch(pathlist[0])) {
    // Windows System with X: drive letter
    root = pathlist[0]+"/";
    pathlist.pop_front();
  }
  return __search_find_test(root, pathlist, 0, limit);
}

KLF_EXPORT QString klfSearchPath(const QString& programName, const QString& extra_path)
{
  KLF_DEBUG_BLOCK(KLF_FUNC_NAME) ;

  static const QString PATH = getenv("PATH");
  static const QString pathsep = QString("")+KLF_PATH_SEP;

  QFileInfo fi(programName);
  if (fi.isAbsolute() && fi.exists() && fi.isExecutable())
    return programName;


  QString path = PATH;
  if (!extra_path.isEmpty())
    path = extra_path + pathsep + path;

  const QStringList paths = path.split(pathsep, QString::KeepEmptyParts);
  QString test;
  int k, j;
  for (k = 0; k < (int)paths.size(); ++k) {
    klfDbg("searching for "<<programName<<" in "<<paths[k]) ;
    QStringList hits = klfSearchFind(paths[k]+"/"+programName);
    klfDbg("\t...resulting in hits = "<<hits) ;
    for (j = 0; j < (int)hits.size(); ++j) {
      if ( QFileInfo(hits[j]).isExecutable() ) {
	klfDbg("\tFound definitive (executable) hit at "+hits[j]) ;
	return hits[j];
      }
    }
  }
  return QString::null;
}

KLF_EXPORT QString klfSearchPath(const QString& fname, const QStringList& paths)
{
  KLF_DEBUG_BLOCK(KLF_FUNC_NAME) ;

  if (QFileInfo(fname).isAbsolute() && QFile::exists(fname))
    return fname;

  QString test;
  int k;
  QStringList hits;
  for (k = 0; k < (int)paths.size(); ++k) {
    klfDbg("searching for "<<fname<<" in "<<paths[k]) ;
    hits = klfSearchFind(paths[k]+"/"+fname);
    klfDbg("\t...resulting in hits = "<<hits) ;
    if (hits.size() > 0) {
      klfDbg("\t...returning "<<hits[0]);
      return hits[0];
    }
  }
  return QString::null;
}

KLF_EXPORT QString klfPrefixedPath(const QString& path_, const QString& ref_)
{
  KLF_DEBUG_BLOCK(KLF_FUNC_NAME) ;

  QString path = path_;
  QString ref = ref_;

  klfDbg("path="<<path<<"; reference="<<ref) ;

  if (path == "~") {
    return QDir::homePath();
  }
  if (path.startsWith("~/")) {
    path = QDir::homePath() + "/" + path.mid(2);
  }

  QFileInfo fi(path);
  
  if (fi.isAbsolute()) {
    return fi.absoluteFilePath();
  }

  if (!ref.isEmpty()) {
    if (ref == "~") {
      ref = QDir::homePath();
    } else if (ref.startsWith("~/")) {
      ref = QDir::homePath() + "/" + path.mid(2);
    }
    ref = QFileInfo(ref).absoluteFilePath();
  } else {
    ref = QCoreApplication::applicationDirPath();
  }

  // return path relative to reference
  if (!ref.endsWith("/")) {
    ref += "/";
  }
  klfDbg("reference is "<<ref) ;

  QString result = QFileInfo(ref + path).absoluteFilePath();
  klfDbg("result = " << result) ;
  return result;
}


// -----------------------------------------------------


QString klfGetEnvironmentVariable(const QStringList& env, const QString& var)
{
  QString vareq = var + QLatin1String("=");
  // search for declaration of var in list
  int k;
  for (k = 0; k < env.size(); ++k) {
    if (env[k].startsWith(vareq)) {
      return env[k].mid(vareq.length());
    }
  }
  // declaration not found. Return null QString().
  return QString();
}

KLF_EXPORT void klfSetEnvironmentVariable(QStringList * env, const QString& var,
					  const QString& value)
{
  QString vareq = var + QLatin1String("=");
  // search for declaration of var in list
  int k;
  for (k = 0; k < env->size(); ++k) {
    if (env->operator[](k).startsWith(vareq)) {
      env->operator[](k) = vareq+value;
      return;
    }
  }
  // declaration not found, just append
  env->append(vareq+value);
  return;
}

QStringList klfSetEnvironmentVariable(const QStringList& env, const QString& var,
				      const QString& value)
{
  QStringList env2 = env;
  klfSetEnvironmentVariable(&env2, var, value);
  return env2;
}

QStringList klfMapToEnvironmentList(const QMap<QString,QString>& map)
{
  QStringList list;
  for (QMap<QString,QString>::const_iterator it = map.begin(); it != map.end(); ++it)
    list << it.key() + "=" + it.value();
  return list;
}

static bool parse_env_line(const QString& s, QString * var, QString * val)
{
  KLF_ASSERT_NOT_NULL(var, "var argument is NULL!", return false; ) ;
  KLF_ASSERT_NOT_NULL(val, "val argument is NULL!", return false; ) ;
  int i = s.indexOf('=');
  if (i == -1) {
    *var = *val = QString();
    klfWarning("Line "<<s<<" is not an environment variable setting.") ;
    return false;
  }
  *var = s.mid(0, i);
  *val = s.mid(i+1);
  return true;
}

QMap<QString,QString> klfEnvironmentListToMap(const QStringList& env)
{
  QMap<QString,QString> map;

  foreach (QString s, env) {
    QString var, val;
    if (!parse_env_line(s, &var, &val))
      continue; // warning already issued
    if (map.contains(var))
      klfWarning("Line "<<s<<" will overwrite previous value of variable "<<var) ;
    map[var] = val;
  }

  return map;
}


void klfMergeEnvironment(QStringList * env, const QStringList& addvars, const QStringList& pathvars, uint actions)
{
  KLF_DEBUG_BLOCK(KLF_FUNC_NAME) ;
  foreach (QString s, addvars) {
    QString var, val;
    if (!parse_env_line(s, &var, &val))
      continue; // warning issued already

    if (actions & KlfEnvMergeExpandVars) {
      val = klfExpandEnvironmentVariables(val, *env, !(actions & KlfEnvMergeExpandNotRecursive));
    }
    if (pathvars.contains(var)) {
      // this variable is a PATH, special treatment: prepend new values to old ones
      val = klfSetEnvironmentPath(klfGetEnvironmentVariable(*env, var), val, actions);
    }

    klfSetEnvironmentVariable(env, var, val);
  }
}

QStringList klfMergeEnvironment(const QStringList& env, const QStringList& addvars,
				const QStringList& pathvars, uint actions)
{
  KLF_DEBUG_BLOCK(KLF_FUNC_NAME) ;
  QStringList merged = env;
  klfMergeEnvironment(&merged, addvars, pathvars, actions);
  return merged;
}


QStringList klfGetEnvironmentPath(const QStringList& env, const QString& var)
{
  QString value = klfGetEnvironmentVariable(env, var);
  // split value according to PATH_SEP
  return klfSplitEnvironmentPath(value);
}

QStringList klfSplitEnvironmentPath(const QString& value)
{
  if (value.isEmpty())
    return QStringList();
  QStringList items = value.split(KLF_PATH_SEP, QString::KeepEmptyParts);

  // with KeepEmptyParts, if there is a trailing or leading colon, then two empty parts
  // will (probably) be generated. remove one of them.
  if (items.size() >= 2) {
    if (items[0].isEmpty() && items[1].isEmpty())
      items.removeAt(0);
    if (items[items.size()-1].isEmpty() && items[items.size()-2].isEmpty())
      items.removeAt(items.size()-1);
  }
  return items;
}

QString klfJoinEnvironmentPath(const QStringList& paths)
{
  return paths.join(QString("")+KLF_PATH_SEP);
}

/*
  enum KlfEnvPathAction {
  KlfEnvPathPrepend      = 0x0001, //!< Prepend given value to list of path items
  KlfEnvPathReplace      = 0x0002, //!< Replace current path items by given ones
  KlfEnvPathAppend       = 0x0003, //!< Append given path items to current list
  KlfEnvPathNoAction     = 0x0000, //!< Don't take any action, just apply flags
  KlfEnvPathActionMask   = 0x00ff, //!< Mask out the requested action
  KlfEnvPathNoDuplicates = 0x0100, //!< Remove duplicates from the variable
  KlfEnvPathFlagsMask    = 0xff00, //!< Mask out the flags
  };
*/

QStringList klfSetEnvironmentPath(const QStringList& oldpaths, const QStringList& newpaths, uint action)
{
  QStringList newitems;
  switch (action & KlfEnvPathActionMask) {
  case KlfEnvPathPrepend:
    newitems = QStringList() << newpaths << oldpaths;
    break;
  case KlfEnvPathAppend:
    newitems = QStringList() << oldpaths << newpaths;
    break;
  case KlfEnvPathReplace:
    newitems = newpaths;
    break;
  case KlfEnvPathNoAction:
    newitems = oldpaths;
    break;
  default:
    klfWarning("No or unknown action specified! action="<<action) ;
    break;
  }
  if (action & KlfEnvPathNoDuplicates) {
    // remove duplicates from newitems
    QStringList newitems2;
    int k;
    for (k = 0; k < newitems.size(); ++k) {
      if (newitems2.contains(newitems[k]))
	continue;
      newitems2.append(newitems[k]);
    }
    newitems = newitems2;
  }
  return newitems;
}

QString klfSetEnvironmentPath(const QString& oldpaths, const QString& newpaths, uint action)
{
  return klfSetEnvironmentPath(klfSplitEnvironmentPath(oldpaths),
			       klfSplitEnvironmentPath(newpaths),
			       action) . join(QString("")+KLF_PATH_SEP);
}

QStringList klfSetEnvironmentPath(const QStringList& env, const QStringList& newpaths,
				  const QString& var, uint action)
{
  QStringList newval;
  newval = klfSetEnvironmentPath(klfGetEnvironmentPath(env, var), newpaths, action);
  return klfSetEnvironmentVariable(env, var, klfJoinEnvironmentPath(newval));
}

void klfSetEnvironmentPath(QStringList * env, const QStringList& newpaths,
			   const QString& var, uint action)
{
  QStringList newval;
  newval = klfSetEnvironmentPath(klfGetEnvironmentPath(*env, var), newpaths, action);
  klfSetEnvironmentVariable(env, var, klfJoinEnvironmentPath(newval));
}


static QString __klf_expandenvironmentvariables(const QString& expression, const QStringList& env,
						bool recursive, const QStringList& recstack)
{
  QString s = expression;
  QRegExp rx("\\$(?:(\\$|(?:[A-Za-z0-9_]+))|\\{([A-Za-z0-9_]+)\\})");
  int i = 0;
  while ( (i = rx.indexIn(s, i)) != -1 ) {
    // match found, replace it
    QString envvarname = rx.cap(1);
    if (envvarname.isEmpty() || envvarname == QLatin1String("$")) {
      // note: empty variable name expands to a literal '$'
      s.replace(i, rx.matchedLength(), QLatin1String("$"));
      i += 1;
      continue;
    }
    // if we're already expanding the value of this variable at some higher recursion level, abort
    // this replacement and leave the variable name.
    if (recstack.contains(envvarname)) {
      klfWarning("Recursive definition detected for variable "<<envvarname<<"!") ;
      i += rx.matchedLength();
      continue;
    }
    // get the environment variable's value
    QString val;
    if (env.isEmpty()) {
      const char *svalue = getenv(qPrintable(envvarname));
      val = (svalue != NULL) ? QString::fromLocal8Bit(svalue) : QString();
    } else {
      val = klfGetEnvironmentVariable(env, envvarname);
    }
    // if recursive, perform replacements on its value
    if (recursive) {
      val = __klf_expandenvironmentvariables(val, env, true, QStringList()<<recstack<<envvarname) ;
    }
    klfDbg("Replaced value of "<<envvarname<<" is "<<val) ;
    s.replace(i, rx.matchedLength(), val);
    i += val.length();
  }
  // replacements performed
  return s;
}

QString klfExpandEnvironmentVariables(const QString& expression, const QStringList& env,
				      bool recursive)
{
  return __klf_expandenvironmentvariables(expression, env, recursive, QStringList());
}


KLF_EXPORT QStringList klfCurrentEnvironment()
{
  return QProcess::systemEnvironment();
}







// ----------------------------------------------------

KLF_EXPORT QString klfUrlLocalFilePath(const QUrl& url)
{
#ifdef Q_OS_WIN32
  QString p = url.path();
  if (p.startsWith("/"))
    p = p.mid(1);
  return p;
#else
  return url.path();
#endif
}

// ------------------------------------------------------

KLFTarget::~KLFTarget()
{
  KLF_DEBUG_BLOCK(KLF_FUNC_NAME) ;

  int k;
  const QList<KLFTargeter*> targeters = pTargetOf;
  for (k = 0; k < targeters.size(); ++k) {
    targeters[k]->pTarget = NULL;
  }
}

KLFTargeter::~KLFTargeter()
{
  KLF_DEBUG_BLOCK(KLF_FUNC_NAME) ;

  if (pTarget != NULL)
    pTarget->pTargetOf.removeAll(this);
}

void KLFTargeter::setTarget(KLFTarget *target)
{
  KLF_DEBUG_BLOCK(KLF_FUNC_NAME) ;
  klfDbg("target="<<target) ;

  if (pTarget != NULL)
    pTarget->pTargetOf.removeAll(this);

  pTarget = target;

  if (pTarget != NULL)
    pTarget->pTargetOf.append(this);
}
