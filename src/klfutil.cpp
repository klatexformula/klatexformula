/***************************************************************************
 *   file klfutil.cpp
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

#include <qglobal.h>
#include <QByteArray>
#include <QString>
#include <QDebug>
#include <QFile>
#include <QDir>
#include <QLibraryInfo>
#include <QUrl>
#include <QMessageBox>
#include <QTextCodec>
#include <QDateTime>
#include <QRect>

#include "klfutil.h"
#include "klflib.h" // KLFStyle



KLF_EXPORT QString KLFSysInfo::arch()
{
#ifdef Q_OS_LINUX
  return QLibraryInfo::buildKey().section(' ', 0, 0, QString::SectionSkipEmpty);
#endif
  return QString();
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

KLF_EXPORT QString KLFSysInfo::osString()
{
  switch (os()) {
  case Linux: return QLatin1String("linux");
  case MacOsX: return QLatin1String("macosx");
  case Win32: return QLatin1String("win32");
  default: ;
  }
  return QString();
}




KLF_EXPORT int klfVersionCompare(const QString& v1, const QString& v2)
{
  qDebug()<<"Comparing versions "<<v1<< " and "<<v2;
  //           *1     2  *3     4  *5    *6
  QRegExp rx1("^(\\d+)(\\.(\\d+)(\\.(\\d+)(.*)?)?)?$");
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
  qDebug()<<"Maj1="<<maj1<<"; maj2="<<maj2;
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
  qDebug()<<"Min1="<<min1<<"; min2="<<min2;
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
  qDebug()<<"rel1="<<rel1<<"; rel2="<<rel2;
  if ( rel1 != rel2 )
    return rel1 - rel2;

  QString suffix1 = rx1.cap(6);
  QString suffix2 = rx2.cap(6);
  if (suffix1 == suffix2)
    return 0; // equal

  if ( suffix1.startsWith("alpha") ) {
    if ( suffix2.startsWith("alpha") ) {
      QString a1 = suffix1.mid(6);
      QString a2 = suffix2.mid(6);
      return QString::compare(a1, a2); // lexicographically compare
    }
    // suffix alpha preceeds any other suffix
    return -1;
  }
  if ( suffix1 == "dev" ) {
    if ( suffix2 == "dev" )
      return 0;
    // suffix dev goes after any other suffix
    return +1; // 3.X.Ydev > 3.X.Yzzzz
  }
  // suffix1 is unknown
  if ( suffix2 == "dev" || suffix2.startsWith("alpha") ) {
    // this is OK because suffix1 != suffix2
    return -klfVersionCompare(v2, v1);
  }
  // fall back to lexicographical compare
  return QString::compare(suffix1, suffix2);
}

KLF_EXPORT bool klfVersionCompareLessThan(const QString& v1, const QString& v2)
{
  return klfVersionCompare(v1,v2) < 0;
}



KLF_EXPORT bool klfEnsureDir(const QString& dir)
{
  if ( ! QDir(dir).exists() ) {
    bool r = QDir("/").mkpath(dir);
    if ( ! r ) {
      QMessageBox::critical(0, QObject::tr("Error"),
			    QObject::tr("Can't create local directory `%1' !").arg(dir));
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
    QStringList entries = d.entryList(QStringList()<<pathlist[level]);
    QStringList hitlist;
    for (k = 0; k < entries.size(); ++k) {
      newpathlist[level] = entries[k];
      hitlist << __search_find_test(root, newpathlist, level+1, limit - hitlist.size());
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
  QString expr = QDir::fromNativeSeparators(wildcard_expression);
  QStringList pathlist = expr.split("/", QString::SkipEmptyParts);
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
#if defined(Q_OS_WIN32) || defined(Q_OS_WIN64)
  static const char pathsep = ';';
#else
  static const char pathsep = ':';
#endif
  QString path = PATH;
  if (!extra_path.isEmpty())
    path += pathsep + extra_path;

  const QStringList paths = path.split(pathsep, QString::KeepEmptyParts);
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







// -----------------------------------------------------

static inline bool klf_is_hex_char(char c)
{
  return ('0' <= c && c <= '9') || ('a' <= c && c <= 'f') || ('A' <= c && c <= 'F');
}




KLF_EXPORT QByteArray klfDataToEscaped(const QByteArray& value_ba)
{
  qDebug("klfDataToEscaped: len=%d, data=`%s'", value_ba.size(), value_ba.constData());
  QByteArray data;
  int k;
  for (k = 0; k < value_ba.size(); ++k) {
    qDebug("\tdata[%d] = %x = %c", k, (uchar)value_ba[k], value_ba[k]);
    if (value_ba[k] >= 32 && value_ba[k] <= 126 && value_ba[k] != '\\') {
      // ascii-ok values, not backslash
      data += value_ba[k];
    } else if (value_ba[k] == '\\') {
      data += "\\\\";
    } else {
      data += QString("\\x%1").arg((uint)(uchar)value_ba[k], 2, 16, QChar('0')).toAscii();
    }
  }
  return data;
}

KLF_EXPORT QByteArray klfEscapedToData(const QByteArray& data)
{
  bool convertOk;
  int k;
  QByteArray value_ba;
  k = 0;
  while (k < data.size()) {
    if (data[k] != '\\') {
      value_ba += data[k];
      ++k;
      continue;
    }
    if (data[k] == '\\' && k+1 >= data.size()) {
      value_ba += '\\'; // backslash at end of data
      ++k;
      continue;
    }
    // not at end of data
    if (data[k+1] != 'x') {
      // backslash followed by something else than 'x', add that escaped 'something else'
      value_ba += data[k+1];
      k += 2; // had to skip the backslash
      continue;
    }
    // pos k points on '\\', pos k+1 points on 'x'
    if (k+3 >= data.size() || !klf_is_hex_char(data[k+2]) || !klf_is_hex_char(data[k+3])) {
      // ignore invalid escape sequence
      value_ba += data[k];
      ++k;
      continue;
    }
    // decode this char
    uchar cval = data.mid(k+2, 2).toUInt(&convertOk, 16);
    value_ba += (char)cval;
    k += 4; // advance of backslash + 'x' + 2 digits
  }
  return value_ba;
}




KLF_EXPORT QByteArray klfSaveVariantToText(const QVariant& value)
{
  QTextCodec *tc = QTextCodec::codecForLocale();

  QString s;
  QByteArray data;
  int k;

  if (!value.isValid() || value.isNull())
    return QByteArray();

  // values of value.type() are QMetaType::Type enum entries. See qt's doc.
  switch (value.type()) {
  case QMetaType::Bool:
    data = value.toBool() ? "true" : "false";
    break;
  case QMetaType::Int:
  case QMetaType::UInt:
  case QMetaType::Short:
  case QMetaType::UShort:
  case QMetaType::Long:
  case QMetaType::ULong:
  case QMetaType::LongLong:
  case QMetaType::ULongLong:
  case QMetaType::Double:
    data = value.toString().toLocal8Bit();
    break;
  case QMetaType::Char:
    {
      char c = value.value<char>();
      if (c >= 32 && c <= 126 && c != '\\')
	data = QByteArray(1, c);
      else if (c == '\\')
	data = "\\\\";
      else
	data = "\\" + QString::number(c, 16).toUpper().toAscii();
    }
  case QMetaType::QChar:
    {
      QChar c = value.toChar();
      if (tc->canEncode(c) && c != '\\')
	data = tc->fromUnicode(QString(c));
      else if (c == '\\')
	data = "\\\\";
      else
	data = "\\" + QString::number(c.unicode(), 16).toUpper().toAscii();
      break;
    }
  case QMetaType::QString:
    {
      s = value.toString();
      if (tc->canEncode(s)) {
	// replace any `\' by `\\' (ie. escape backslashes)
	data = tc->fromUnicode(s.replace("\\", "\\\\"));
      } else {
	// encode char by char, escaping as needed
	data = QByteArray("");
	for (k = 0; k < s.length(); ++k) {
	  if (tc->canEncode(s[k]))
	    data += tc->fromUnicode(s.mid(k,1));
	  else
	    data += QString("\\x%1").arg((uint)s[k].unicode(), 4, 16, QChar('0')).toAscii();
	}
      }
      break;
    }
  case QMetaType::QUrl:
    data = value.toUrl().toEncoded(); break;
  case QMetaType::QByteArray:
    {
      data = klfDataToEscaped(value.value<QByteArray>());
      break;
    }
  case QMetaType::QDate:
    data = value.value<QDate>().toString(Qt::SystemLocaleShortDate).toLocal8Bit(); break;
  case QMetaType::QTime:
    data = value.value<QTime>().toString(Qt::SystemLocaleShortDate).toLocal8Bit(); break;
  case QMetaType::QDateTime:
    data = value.value<QDateTime>().toString(Qt::SystemLocaleShortDate).toLocal8Bit(); break;
  case QMetaType::QSize:
    { QSize sz = value.toSize();
      data = QString("(%1,%2)").arg(sz.width()).arg(sz.height()).toAscii();
      break;
    }
  case QMetaType::QPoint:
    { QPoint pt = value.toPoint();
      data = QString("(%1,%2)").arg(pt.x()).arg(pt.y()).toAscii();
      break;
    }
  case QMetaType::QRect:
    { QRect r = value.toRect();
      data = QString("(%1,%2+%3,%4)").arg(r.left()).arg(r.top()).arg(r.width()).arg(r.height()).toAscii();
      break;
    }
  case QMetaType::QColor:
    { QColor c = value.value<QColor>();
      if (c.alpha() == 255)
	data = QString("(%1,%2,%3)").arg(c.red()).arg(c.green()).arg(c.blue()).toAscii();
      else
	data = QString("(%1,%2,%3,%4)").arg(c.red()).arg(c.green()).arg(c.blue()).arg(c.alpha()).toAscii();
      break;
    }
  case QMetaType::QFont:
    { QFont f = value.value<QFont>();
      data = '"' + f.family().toLocal8Bit() + '"';
      switch (f.weight()) {
      case QFont::Light: data += " Light"; break;
      case QFont::Normal: break; //data += " Normal"; break;
      case QFont::DemiBold: data += " DemiBold"; break;
      case QFont::Bold: data += " Bold"; break;
      case QFont::Black: data += " Black"; break;
      default: data += QString(" Wgt=%1").arg(f.weight()); break;
      }
      switch (f.style()) {
      case QFont::StyleNormal: break; //data += " Normal"; break;
      case QFont::StyleItalic: data += " Italic"; break;
      case QFont::StyleOblique: data += " Oblique"; break;
      default: break;
      }
      // QFontInfo is preferred, if  f  was set with a pixelSize().
      data += " " + QString::number(QFontInfo(f).pointSize()).toAscii();
      break;
    }
  case QMetaType::QVariantList:
    {
      const QList<QVariant> list = value.toList();
      data = "[";
      for (k = 0; k < list.size(); ++k) {
	QByteArray d = klfSaveVariantToText(list[k]);
	d.replace("\\", "\\\\");
	d.replace(";", "\\;");
	d.replace("[", "\\[");
	d.replace("]", "\\]");
	data += d;
	if (k < list.size()-1)
	  data += ";";
      }
      data += "]";
      break;
    }
  case QMetaType::QVariantMap:
    {
      const QMap<QString,QVariant> map = value.toMap();
      data = "{";
      bool first = true;
      for (QMap<QString,QVariant>::const_iterator it = map.begin(); it != map.end(); ++it) {
	if (!first) {
	  data += ";";
	}
	first = false;
	// prepare the pair  key=value
	QByteArray k = klfSaveVariantToText(QVariant(it.key()));
	QByteArray v = klfSaveVariantToText(it.value());
	k.replace("\\", "\\\\");
	k.replace(";", "\\;");
	k.replace("=", "\\=");
	v.replace("\\", "\\\\");
	v.replace(";", "\\;");
	data += k + "=" + v;
      }
      data += "}";
      break;
    }
  default:
    break;
  };

  // -- some other types --

  QByteArray typeName = value.typeName();

  if (typeName == "KLFStyle") {
    KLFStyle style = value.value<KLFStyle>();
    QVariantMap map;
    map["name"] = klfSaveVariantToText(style.name);
    map["fg_color"] = klfSaveVariantToText(QColor(style.fg_color));
    map["bg_color"] = klfSaveVariantToText(QColor(style.bg_color));
    map["mathmode"] = klfSaveVariantToText(style.mathmode);
    map["preamble"] = klfSaveVariantToText(style.preamble);
    map["dpi"] = klfSaveVariantToText(QVariant::fromValue(style.dpi));
    // now, save the map itself.
    // use 'return', not 'data = ', because this call to klfSaveVariantToText() is already "finalizing"
    return klfSaveVariantToText(map);
  }

  // protect data from some special sequences

  if (data.startsWith("[QVariant]") || data.startsWith("\\")) // protect this special sequence
    data = "\\"+data;

  // and provide a default encoding scheme (only machine-readable ...)

  if (data.isNull()) {
    QByteArray vdata;
    {
      QDataStream stream(&vdata, QIODevice::WriteOnly);
      stream << value;
    }
    QByteArray vdata_esc = klfDataToEscaped(vdata);
    qDebug("\tVariant value is %s, len=%d", vdata.constData(), vdata.size());
    data = QByteArray("[QVariant]");
    data += vdata_esc;
  }

  qDebug()<<"klfSaveVariantToText("<<value<<"): saved data (len="<<data.size()<<") : "<<data;
  return data;
}



KLF_EXPORT QVariant klfLoadVariantFromText(const QByteArray& stringdata, const char * dataTypeName,
					   const char *listOrMapDataTypeName)
{
  QRegExp v2rx("^\\(\\s*(-?\\d+)\\s*[,;]\\s*(-?\\d+)\\s*\\)");

  QByteArray data = stringdata; // might need slight modifications before parsing

  QVariant value;
  if (data.startsWith("[QVariant]")) {
    QByteArray vdata_esc = data.mid(strlen("[QVariant]"));
    QByteArray vdata = klfEscapedToData(vdata_esc);
    qDebug()<<"\tAbout to read raw variant from datastr="<<vdata_esc<<", ie. from data len="<<vdata.size();
    QDataStream stream(vdata);
    stream >> value;
    return value;
  }
  if (data.startsWith("\\"))
    data = data.mid(1);

  qDebug()<<"Will start loading data (len="<<data.size()<<") : "<<data;

  // now, start reading.
  int type = QVariant::nameToType(dataTypeName);
  bool convertOk = false; // in case we break; somewhere, it's (by default) because of failed convertion.
  int k;
  switch (type) {
  case QMetaType::Bool:
    {
      QByteArray lowerdata = data.trimmed().toLower();
      QChar c = QChar(lowerdata[0]);
      // true, yes, on, 1
      return QVariant::fromValue<bool>(c == 't' || c == 'y' || c == '1' || lowerdata == "on");
    }
  case QMetaType::Int:
    {
      int i = data.toInt(&convertOk);
      if (convertOk)
	return QVariant::fromValue<int>(i);
      break;
    }
  case QMetaType::UInt:
    {
      uint i = data.toUInt(&convertOk);
      if (convertOk)
	return QVariant::fromValue<uint>(i);
      break;
    }
  case QMetaType::Short:
    {
      short i = data.toShort(&convertOk);
      if (convertOk)
	return QVariant::fromValue<short>(i);
      break;
    }
  case QMetaType::UShort:
    {
      ushort i = data.toUShort(&convertOk);
      if (convertOk)
	return QVariant::fromValue<ushort>(i);
      break;
    }
  case QMetaType::Long:
    {
      long i = data.toLong(&convertOk);
      if (convertOk)
	return QVariant::fromValue<long>(i);
      break;
    }
  case QMetaType::ULong:
    {
      ulong i = data.toULong(&convertOk);
      if (convertOk)
	return QVariant::fromValue<ulong>(i);
      break;
    }
  case QMetaType::LongLong:
    {
      qlonglong i = data.toLongLong(&convertOk);
      if (convertOk)
	return QVariant::fromValue<qlonglong>(i);
      break;
    }
  case QMetaType::ULongLong:
    {
      qulonglong i = data.toULongLong(&convertOk);
      if (convertOk)
	return QVariant::fromValue<qulonglong>(i);
      break;
    }
  case QMetaType::Double:
    {
      double val = data.toDouble(&convertOk);
      if (convertOk)
	return QVariant::fromValue<double>(val);
      break;
    }
  case QMetaType::Char:
    {
      if (data[0] == '\\') {
	if (data.size() < 2)
	  break;
	if (data[1] == '\\')
	  return QVariant::fromValue<char>('\\');
	if (data.size() < 3)
	  break;
	uint c = data.mid(1).toUInt(&convertOk, 16);
	if (!convertOk)
	  break;
	convertOk = false; // reset by default convertOk to false
	if (c > 255)
	  break;
	return QVariant::fromValue<char>( (char)c );
      }
      return QVariant::fromValue<char>( (char)data[0] );
    }
  case QMetaType::QChar:
    {
      if (data[0] == '\\') {
	if (data.size() < 2)
	  break;
	if (data[1] == '\\')
	  return QVariant::fromValue<QChar>(QChar('\\'));
	if (data.size() < 3)
	  break;
	uint c = data.mid(1).toUInt(&convertOk, 16);
	if (!convertOk)
	  break;
	convertOk = false; // reset by default convertOk to false
	if (c > 255)
	  break;
	return QVariant::fromValue<QChar>( QChar(c) );
      }
      return QVariant::fromValue<QChar>( QChar(data[0]) );
    }
  case QMetaType::QString:
    {
      QString s;
      QByteArray chunk;
      k = 0;
      while (k < data.size()) {
	if (data[k] != '\\') {
	  chunk += data[k];
	  ++k;
	  continue;
	}
	if (data[k] == '\\' && k+1 >= data.size()) {
	  chunk += '\\'; // backslash at end of data
	  ++k;
	  continue;
	}
	// not at end of data
	if (data[k+1] != 'x') {
	  // backslash followed by something else than 'x', add that escaped 'something else'
	  chunk += data[k+1];
	  k += 2; // had to skip the backslash
	  continue;
	}
	// pos k points on '\\', pos k+1 points on 'x'
	int nlen = -1;
	if (k+5 < data.size() && klf_is_hex_char(data[k+2]) && klf_is_hex_char(data[k+3])
	    && klf_is_hex_char(data[k+4]) && klf_is_hex_char(data[k+5]))
	  nlen = 4; // 4-digit Unicode char
	if (k+3 < data.size() && klf_is_hex_char(data[k+2]) && klf_is_hex_char(data[k+3]))
	  nlen = 2; // 2 last digits of 4-digit unicode char
	if (nlen < 0) {
	  // bad format, ignore the escape sequence.
	  chunk += data[k];
	  ++k;
	  continue;
	}
	// decode this char
	ushort cval = data.mid(k+2, nlen).toUShort(&convertOk, 16);
	QChar ch(cval);
	// dump chunk into string, and add this char
	s += QString::fromLocal8Bit(chunk) + ch;
	// reset chunk
	chunk = QByteArray();
	// and advance the corresponding number of characters, point on fresh one
	// advance of what we read:   backslash+'x' (=2)  + number of digits
	k += 2 + nlen;
      }
      // dump remaining chunk
      s += QString::fromLocal8Bit(chunk);
      return QVariant::fromValue<QString>(s);
    }
  case QMetaType::QUrl:
    return QVariant::fromValue<QUrl>(QUrl(QString::fromLocal8Bit(data), QUrl::TolerantMode));
  case QMetaType::QByteArray:
    {
      QByteArray value_ba = klfEscapedToData(data);
      return QVariant::fromValue<QByteArray>(value_ba);
    }
  case QMetaType::QDate:
    {
      QString s = QString::fromLocal8Bit(data);
      QDate date = QDate::fromString(s, Qt::SystemLocaleShortDate);
      if (!date.isValid()) date = QDate::fromString(s, Qt::ISODate);
      if (!date.isValid()) date = QDate::fromString(s, Qt::SystemLocaleLongDate);
      if (!date.isValid()) date = QDate::fromString(s, Qt::DefaultLocaleShortDate);
      if (!date.isValid()) date = QDate::fromString(s, Qt::TextDate);
      if (!date.isValid()) date = QDate::fromString(s, "dd-MM-yyyy");
      if (!date.isValid()) date = QDate::fromString(s, "dd.MM.yyyy");
      if (!date.isValid()) date = QDate::fromString(s, "dd MM yyyy");
      if (!date.isValid()) date = QDate::fromString(s, "yyyy-MM-dd");
      if (!date.isValid()) date = QDate::fromString(s, "yyyy.MM.dd");
      if (!date.isValid()) date = QDate::fromString(s, "yyyy MM dd");
      if (!date.isValid()) date = QDate::fromString(s, "yyyyMMdd");
      if (!date.isValid())
	break;
      return QVariant::fromValue<QDate>(date);
    }
  case QMetaType::QTime:
    {
      QString s = QString::fromLocal8Bit(data);
      QTime time = QTime::fromString(s, Qt::SystemLocaleShortDate);
      if (!time.isValid()) time = QTime::fromString(s, Qt::ISODate);
      if (!time.isValid()) time = QTime::fromString(s, Qt::SystemLocaleLongDate);
      if (!time.isValid()) time = QTime::fromString(s, Qt::DefaultLocaleShortDate);
      if (!time.isValid()) time = QTime::fromString(s, Qt::TextDate);
      if (!time.isValid()) time = QTime::fromString(s, "hh:mm:ss.z");
      if (!time.isValid()) time = QTime::fromString(s, "hh:mm:ss");
      if (!time.isValid()) time = QTime::fromString(s, "hh:mm:ss AP");
      if (!time.isValid()) time = QTime::fromString(s, "hh.mm.ss");
      if (!time.isValid()) time = QTime::fromString(s, "hh.mm.ss AP");
      if (!time.isValid()) time = QTime::fromString(s, "hh mm ss");
      if (!time.isValid()) time = QTime::fromString(s, "hh mm ss AP");
      if (!time.isValid()) time = QTime::fromString(s, "hhmmss");
      if (!time.isValid())
	break;
      return QVariant::fromValue<QTime>(time);
    }
  case QMetaType::QDateTime:
    {
      QString s = QString::fromLocal8Bit(data);
      QDateTime dt = QDateTime::fromString(s, Qt::SystemLocaleShortDate);
      if (!dt.isValid()) dt = QDateTime::fromString(s, Qt::ISODate);
      if (!dt.isValid()) dt = QDateTime::fromString(s, Qt::SystemLocaleLongDate);
      if (!dt.isValid()) dt = QDateTime::fromString(s, Qt::DefaultLocaleShortDate);
      if (!dt.isValid()) dt = QDateTime::fromString(s, Qt::TextDate);
      if (!dt.isValid()) dt = QDateTime::fromString(s, "dd-MM-yyyy hh:mm:ss");
      if (!dt.isValid()) dt = QDateTime::fromString(s, "dd-MM-yyyy hh.mm.ss");
      if (!dt.isValid()) dt = QDateTime::fromString(s, "dd.MM.yyyy hh:mm:ss");
      if (!dt.isValid()) dt = QDateTime::fromString(s, "dd.MM.yyyy hh.mm.ss");
      if (!dt.isValid()) dt = QDateTime::fromString(s, "dd MM yyyy hh mm ss");
      if (!dt.isValid()) dt = QDateTime::fromString(s, "yyyy-MM-dd hh:mm:ss");
      if (!dt.isValid()) dt = QDateTime::fromString(s, "yyyy-MM-dd hh.mm.ss");
      if (!dt.isValid()) dt = QDateTime::fromString(s, "yyyy.MM.dd hh:mm:ss");
      if (!dt.isValid()) dt = QDateTime::fromString(s, "yyyy.MM.dd hh.mm.ss");
      if (!dt.isValid()) dt = QDateTime::fromString(s, "yyyy MM dd hh mm ss");
      if (!dt.isValid()) dt = QDateTime::fromString(s, "yyyyMMddhhmmss");
      if (!dt.isValid())
	break;
      return QVariant::fromValue<QDateTime>(dt);
    }
  case QMetaType::QSize:
    {
      QString s = QString::fromLocal8Bit(data.trimmed());
      if (v2rx.indexIn(s) < 0)
	break;
      QStringList vals = v2rx.capturedTexts();
      return QVariant::fromValue<QSize>(QSize(vals[1].toInt(), vals[2].toInt()));
    }
  case QMetaType::QPoint:
    {
      QString s = QString::fromLocal8Bit(data.trimmed());
      if (v2rx.indexIn(s) < 0)
	break;
      QStringList vals = v2rx.capturedTexts();
      return QVariant::fromValue<QPoint>(QPoint(vals[1].toInt(), vals[2].toInt()));
    }
  case QMetaType::QRect:
    {
      //                      1                   2           3          4                   5
      QRegExp rectrx("^\\(\\s*(-?\\d+)\\s*[,;]\\s*(-?\\d+)\\s*([+,;])\\s*(-?\\d+)\\s*[,;]\\s*(-?\\d+)\\s*\\)");
      QString s = QString::fromLocal8Bit(data.trimmed());
      if (rectrx.indexIn(s) < 0)
	break;
      QStringList vals = rectrx.capturedTexts();
      if (vals[3] == "+") {
	return QVariant::fromValue<QRect>(QRect( QPoint(vals[1].toInt(), vals[2].toInt()),
						 QSize(vals[4].toInt(), vals[5].toInt()) ));
      }
      return QVariant::fromValue<QRect>(QRect( QPoint(vals[1].toInt(), vals[2].toInt()),
					       QPoint(vals[4].toInt(), vals[5].toInt()) ));
    }
  case QMetaType::QColor:
    {
      //                     1                   2                   3           4        5
      QRegExp colrx("^\\(\\s*(-?\\d+)\\s*[,;]\\s*(-?\\d+)\\s*[,;]\\s*(-?\\d+)\\s*([,;]\\s*(-?\\d+)\\s*)?\\)");
      if (colrx.indexIn(QString::fromLocal8Bit(data.trimmed())) < 0)
	break;
      QStringList vals = colrx.capturedTexts();
      QColor color = QColor(vals[1].toInt(), vals[2].toInt(), vals[3].toInt(), 255);
      if (!vals[4].isEmpty())
	color.setAlpha(vals[5].toInt());
      return QVariant::fromValue<QColor>(color);
    }
  case QMetaType::QFont:
    {
      //               1        2
      QRegExp fontrx("^(\"?)\\s*([^\"]+)\\s*\\1"
		     //3   4                                             5
		     "(\\s+(Light|Normal|DemiBold|Bold|Black|Wgt\\s*=\\s*(\\d+)))?"
		     //6   7                        8    9
		     "(\\s+(Normal|Italic|Oblique))?(\\s+(\\d+))?");

      if (fontrx.indexIn(QString::fromLocal8Bit(data.trimmed())) < 0) {
	qDebug("\tmalformed font: `%s'", qPrintable(QString::fromLocal8Bit(data.trimmed())));
	break;
      }
      QStringList vals = fontrx.capturedTexts();
      qDebug()<<"klfLoadVariantFromText: loaded font-rx cap.Texts: "<<vals;
      QString family = vals[2].trimmed();
      QString weighttxt = vals[4];
      QString weightval = vals[5];
      QString styletxt = vals[7];
      QString ptsval = vals[9];

      int weight = -1;
      if (weighttxt == "Light") weight = QFont::Light;
      else if (weighttxt == "Normal") weight = QFont::Normal;
      else if (weighttxt == "DemiBold") weight = QFont::DemiBold;
      else if (weighttxt == "Bold") weight = QFont::Bold;
      else if (weighttxt == "Black") weight = QFont::Black;
      else if (weighttxt.startsWith("Wgt")) weight = weightval.toInt();
      

      QFont::Style style = QFont::StyleNormal;
      if (styletxt == "Normal") style = QFont::StyleNormal;
      else if (styletxt == "Italic") style = QFont::StyleItalic;
      else if (styletxt == "Oblique") style = QFont::StyleOblique;

      int pt = -1;
      if (!ptsval.isEmpty())
	pt = ptsval.toInt();

      QFont font(family, pt, weight);
      font.setStyle(style);
      return QVariant::fromValue<QFont>(font);
    }
  case QMetaType::QVariantList:
    {
      data = data.trimmed();
      if (data[0] != '[')
	break;
      if ( !data.contains(']') )
	data += ']';

      QList<QByteArray> sections;
      QByteArray chunk;
      // first, split data.  take into account escaped chars.
      // k=1 to skip '['
      k = 1;
      while (k < data.size()) {
	if (data[k] == ';') { // element separator
	  // flush chunk as a new section
	  sections.append(chunk);
	  // and start a new section
	  chunk = QByteArray();
	  ++k;
	}
	if (data[k] == '\\') {
	  if (k+1 < data.size()) { // there exists a next char
	    chunk += data[k+1];
	    k += 2;
	  } else {
	    chunk += data[k];
	    ++k;
	  }
	  continue;
	}
	if (data[k] == ']') {
	  // end of list marker.
	  // flush last chunk into sections, and break.
	  sections.append(chunk);
	  break;
	}
	// regular char, populate current chunk.
	chunk += data[k];
	++k;
      }

      // now we separated into bytearray sections. now read those into values.
      QVariantList list;
      for (k = 0; k < sections.size(); ++k) {
	QVariant val = klfLoadVariantFromText(sections[k], listOrMapDataTypeName);
	list << val;
      }

      return QVariant::fromValue<QVariantList>(list);
    }
  case QMetaType::QVariantMap:
    {
      data = data.trimmed();
      if (data[0] != '{')
	break;
      if ( !data.contains('}') )
	data += '}';

      QMap<QByteArray, QByteArray> sections;
      QByteArray chunkkey;
      QByteArray chunkvalue;
      QByteArray *curChunk = &chunkkey;
      // first, split data.  take into account escaped chars.
      // k=1 to skip '{'
      k = 1;
      while (k < data.size()) {
	if (data[k] == ';') { // separator for next pair
	  // flush chunk as a new section
	  if (curChunk == &chunkkey)
	    qWarning()<<"klfLoadVariantFromText: no '=' in pair at pos "<<k<<" in string: "<<data<<"";
	  sections[chunkkey] = chunkvalue;
	  // and start a new section
	  chunkkey = QByteArray();
	  chunkvalue = QByteArray();
	  curChunk = &chunkkey;
	  ++k;
	}
	if (data[k] == '\\') {
	  if (k+1 < data.size()) { // there exists a next char
	    *curChunk += data[k+1];
	    k += 2;
	  } else {
	    *curChunk += data[k];
	    ++k;
	  }
	  continue;
	}
	if (curChunk == &chunkkey && data[k] == '=') {
	  // currently reading key, switch to reading value
	  curChunk = &chunkvalue;
	  ++k;
	  continue;
	}
	if (data[k] == '}') {
	  // end of list marker.
	  // flush last chunk into sections, and break.
	  if (curChunk == &chunkkey)
	    qWarning()<<"klfLoadVariantFromText: no '=' in pair at pos "<<k<<" in string: "<<data<<"";
	  sections[chunkkey] = chunkvalue;
	  break;
	}
	// regular char, populate current chunk.
	*curChunk += data[k];
	++k;
      }

      QVariantMap vmap;
      QMap<QByteArray,QByteArray>::const_iterator it;
      for (it = sections.begin(); it != sections.end(); ++it) {
	QString key = klfLoadVariantFromText(it.key(), "QString").toString();
	QVariant value = klfLoadVariantFromText(it.value(), listOrMapDataTypeName);
	vmap[key] = value;
      }

      return QVariant::fromValue<QVariantMap>(vmap);
    }
  default:
    break;
  }

  QByteArray tname = dataTypeName;
  if (tname == "KLFStyle") {
    KLFStyle style;
    QVariantMap map = klfLoadVariantFromText(data, "QByteArray").toMap();
    style.name = klfLoadVariantFromText(map["name"].toByteArray(), "QString").toString();
    style.fg_color =
      klfLoadVariantFromText(map["fg_color"].toByteArray(), "QColor").value<QColor>().rgba();
    style.bg_color =
      klfLoadVariantFromText(map["bg_color"].toByteArray(), "QColor").value<QColor>().rgba();
    style.mathmode = klfLoadVariantFromText(map["mathmode"].toByteArray(), "QString").toString();
    style.preamble = klfLoadVariantFromText(map["preamble"].toByteArray(), "QString").toString();
    style.dpi = klfLoadVariantFromText(map["dpi"].toByteArray(), "int").toInt();
    return QVariant::fromValue<KLFStyle>(style);
  }

  qWarning("klfLoadVariantFromText: Can't load a %s from %s !", dataTypeName, stringdata.constData());
  return QVariant();
}
