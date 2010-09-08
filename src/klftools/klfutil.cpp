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
#include <QIcon>
#include <QColor>
#include <QBrush>
#include <QPushButton>
#include <QApplication>
#include <QDesktopWidget>
#include <QDomDocument>
#include <QTextFormat>

#include "klfutil.h"
#include "klfstyle.h"



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
  QList<QPair<QString,QString> > qitems = url.queryItems();
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
  u1.setQueryItems(QList<QPair<QString,QString> >());
  u2.setQueryItems(QList<QPair<QString,QString> >());

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


// -----------------------------------------------------



static inline bool klf_is_hex_char(char c)
{
  return ('0' <= c && c <= '9') || ('a' <= c && c <= 'f') || ('A' <= c && c <= 'F');
}



#define KLF_BRUSH_STYLE(sty)			\
  { Qt::sty##Pattern, #sty }

static struct { int brushStyle; const char *style; } klf_brush_styles[] = {
  { Qt::NoBrush, "NoBrush" },
  { Qt::SolidPattern, "" },
  { Qt::SolidPattern, "Solid" },
  KLF_BRUSH_STYLE(Dense1),
  KLF_BRUSH_STYLE(Dense2),
  KLF_BRUSH_STYLE(Dense3),
  KLF_BRUSH_STYLE(Dense4),
  KLF_BRUSH_STYLE(Dense5),
  KLF_BRUSH_STYLE(Dense6),
  KLF_BRUSH_STYLE(Dense7),
  KLF_BRUSH_STYLE(Hor),
  KLF_BRUSH_STYLE(Ver),
  KLF_BRUSH_STYLE(Cross),
  KLF_BRUSH_STYLE(BDiag),
  KLF_BRUSH_STYLE(FDiag),
  KLF_BRUSH_STYLE(DiagCross),
  { -1, NULL }
};



#define KLF_TEXT_FORMAT_FORMAT(fmt)		\
  { QTextFormat::fmt##Format, #fmt "Format" }

static struct { int formatId; const char *format; } klf_text_format_formats[] = {
  KLF_TEXT_FORMAT_FORMAT(Invalid),
  KLF_TEXT_FORMAT_FORMAT(Block),
  KLF_TEXT_FORMAT_FORMAT(Char),
  KLF_TEXT_FORMAT_FORMAT(List),
  KLF_TEXT_FORMAT_FORMAT(Table),
  KLF_TEXT_FORMAT_FORMAT(Frame),
  KLF_TEXT_FORMAT_FORMAT(User),
  { -100, NULL }
};


#define KLF_TEXT_FORMAT_PROP(p, type)		\
  { QTextFormat::p, #p, #type }

static struct { int propId; const char *key; const char *type; } klf_text_format_props[] = {
  KLF_TEXT_FORMAT_PROP(ForegroundBrush, QBrush),
  KLF_TEXT_FORMAT_PROP(BackgroundBrush, QBrush),
  KLF_TEXT_FORMAT_PROP(FontFamily, QString),
  KLF_TEXT_FORMAT_PROP(FontPointSize, int),
  KLF_TEXT_FORMAT_PROP(FontWeight, int),
  KLF_TEXT_FORMAT_PROP(FontItalic, bool),
  KLF_TEXT_FORMAT_PROP(TextUnderlineStyle, int),
  // add more keys for short-hands
  { QTextFormat::ForegroundBrush, "FG", "QBrush" },
  { QTextFormat::BackgroundBrush, "BG", "QBrush" },

  { -1, NULL, NULL }
};

static struct { const char * keyword; int propId; QVariant fixed_value; } klf_text_format_keywords[] = {
    { "NORMALWEIGHT", QTextFormat::FontWeight, QVariant(QFont::Normal) },
    { "BOLD", QTextFormat::FontWeight, QVariant(QFont::Bold) },
    { "NORMALSTYLE", QTextFormat::FontItalic, QVariant(false) },
    { "ITALIC", QTextFormat::FontItalic, QVariant(true) },

    { NULL, -1, QVariant() }
};




KLF_EXPORT QByteArray klfDataToEscaped(const QByteArray& value_ba)
{
  qDebug("klfDataToEscaped: len=%d, data=`%s'", value_ba.size(), value_ba.constData());
  QByteArray data;
  int k;
  for (k = 0; k < value_ba.size(); ++k) {
    //    qDebug("\tdata[%d] = %x = %c", k, (uchar)value_ba[k], value_ba[k]);
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


static QByteArray encaps_list(const QList<QByteArray>& list)
{
  QByteArray data = "[";
  for (int k = 0; k < list.size(); ++k) {
    QByteArray d = list[k];
    d.replace("\\", "\\\\");
    d.replace(";", "\\;");
    d.replace("[", "\\[");
    d.replace("]", "\\]");
    data += d;
    if (k < list.size()-1)
      data += ";";
  }
  data += "]";
  return data;
}

// if 'ignore_empty_values' is TRUE, then the '=' sign is omitted with the value in a section is empty.
static QByteArray encaps_map(const QList<QPair<QByteArray,QByteArray> >& sections, bool ignore_empty_values = false)
{
  QByteArray data;
  data = "{";
  bool first_item = true;
  int k;
  for (k = 0; k < sections.size(); ++k) {
    if (!first_item) {
      data += ";";
    }
    first_item = false;
    QByteArray key = sections[k].first;
    QByteArray val = sections[k].second;
    // prepare the pair  key=value
    key.replace("\\", "\\\\");
    key.replace(";", "\\;");
    key.replace("=", "\\=");
    val.replace("\\", "\\\\");
    val.replace(";", "\\;");
    if (val.isEmpty() && ignore_empty_values)
      data += key;
    else
      data += key + "=" + val;
  }
  data += "}";
  return data;
}


static QList<QByteArray> decaps_list(const QByteArray& ba_data)
{
  klfDbg("decaps_list, data="<<ba_data);
  QByteArray data = ba_data.trimmed();
  if (data[0] != '[')
    return QList<QByteArray>();

  QList<QByteArray> sections;
  QByteArray chunk;
  // first, split data.  take into account escaped chars.
  // k=1 to skip '['
  int k = 1;
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
      chunk = "";
      break;
    }
    // regular char, populate current chunk.
    chunk += data[k];
    ++k;
  }
  if (!chunk.isEmpty()) {
    // missing ']' at end, tolerate this by adding the unfinished chunk to sections
    sections.append(chunk);
  }

  klfDbg("sections="<<sections);

  return sections;
}

static QList<QPair<QByteArray,QByteArray> > decaps_map(const QByteArray& ba_data, bool allow_empty_values = false)
{
  QByteArray data = ba_data.trimmed();
  if (data[0] != '{')
    return QList<QPair<QByteArray,QByteArray> >();
  if ( !data.contains('}') )
    data += '}';
	
  QList<QPair<QByteArray, QByteArray> > sections;
  QByteArray chunkkey;
  QByteArray chunkvalue;
  QByteArray *curChunk = &chunkkey;
  // first, split data.  take into account escaped chars.
  // k=1 to skip '{'
  int k = 1;
  while (k < data.size()) {
    if (data[k] == ';') { // separator for next pair
      // flush chunk as a new section
      if (!allow_empty_values && curChunk == &chunkkey)
	qWarning()<<KLF_FUNC_NAME<<": no '=' in pair at pos "<<k<<" in string: "<<data<<"";
      sections << QPair<QByteArray,QByteArray>(chunkkey, chunkvalue);
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
      if (!allow_empty_values && curChunk == &chunkkey)
	qWarning()<<"klfLoadVariantFromText: no '=' in pair at pos "<<k<<" in string: "<<data<<"";
      sections << QPair<QByteArray,QByteArray>(chunkkey, chunkvalue);
      break;
    }
    // regular char, populate current chunk.
    *curChunk += data[k];
    ++k;
  }
  return sections;
}



// returns root node. get the document with  root.ownerDocument()
static QDomElement make_xml_wrapper(const QString& rootname)
{
  QDomDocument xmldoc(rootname);
  QDomElement root = xmldoc.createElement(rootname);
  xmldoc.appendChild(root);
  return root;
}

static QDomElement parse_xml_wrapper(const QByteArray& xmldata, const QString& shouldBeRootName)
{
  QDomDocument xmldoc(shouldBeRootName);
  bool result = xmldoc.setContent(xmldata);
  KLF_ASSERT_CONDITION(result, "Failed to read wrapper XML for klfLoadVariantFromText()",
		       return QDomElement() ) ;

  QDomElement el = xmldoc.documentElement();
  KLF_ASSERT_CONDITION( el.nodeName() == shouldBeRootName,
		        "Wrong XML root node in wrapper for klfLoadVariantFromText(): "
			<<el.nodeName() ,  ) ;
  return el;
}

KLF_EXPORT QByteArray klfSaveVariantToText(const QVariant& value, bool saveListAndMapsAsXML)
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
  case QMetaType::QStringList:
    {
      const QStringList list = value.toStringList();
      QList<QByteArray> sections;
      int k;
      for (k = 0; k < list.size(); ++k) {
	sections.append(klfDataToEscaped(list[k].toUtf8()));
      }
      data = encaps_list(sections);
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
      data = QString("(%1 %2)").arg(sz.width()).arg(sz.height()).toAscii();
      break;
    }
  case QMetaType::QPoint:
    { QPoint pt = value.toPoint();
      data = QString("(%1 %2)").arg(pt.x()).arg(pt.y()).toAscii();
      break;
    }
  case QMetaType::QRect:
    { QRect r = value.toRect();
      data = QString("(%1 %2+%3x%4)").arg(r.left()).arg(r.top()).arg(r.width()).arg(r.height()).toAscii();
      break;
    }
  case QMetaType::QColor:
    { QColor c = value.value<QColor>();
      if (c.alpha() == 255)
	data = QString("(%1 %2 %3)").arg(c.red()).arg(c.green()).arg(c.blue()).toAscii();
      else
	data = QString("(%1 %2 %3 %4)").arg(c.red()).arg(c.green()).arg(c.blue()).arg(c.alpha()).toAscii();
      break;
    }
  case QMetaType::QFont:
    { QFont f = value.value<QFont>();
      data = "'" + f.family().toLocal8Bit() + "'";
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
  case QMetaType::QBrush:
    { QBrush b = value.value<QBrush>();
      if (!b.matrix().isIdentity())
	break; // forget about saving complex brushes here
      int bstyle = b.style();
      // find index in our brush style enum
      int k;
      bool found_style = false;
      for (k = 0; klf_brush_styles[k].brushStyle >= 0 && klf_brush_styles[k].style != NULL; ++k) {
	if (klf_brush_styles[k].brushStyle == bstyle) {
	  found_style = true;
	  break;
	}
      }
      if (!found_style) {
	// didn't find this style, this is a complex brush. Need to save it via a datastream.
	break;
      }
      // found brush style. This is a simple brush with just a style and a color.
      data = "(";
      data += klf_brush_styles[k].style;
      if (strlen(klf_brush_styles[k].style))
	data += " ";
      QColor c = b.color();
      data += QString("%1 %2 %3 %4").arg(c.red()).arg(c.green()).arg(c.blue()).arg(c.alpha());
      data += ")";
      break;
    }
  case QMetaType::QTextFormat:
    {
      QTextFormat tf = value.value<QTextFormat>();
      const QMap<int,QVariant> props = tf.properties();

      QList<QPair<QByteArray,QByteArray> > sections;

      // first find the QTextFormat type.
      int k;
      for (k = 0; klf_text_format_formats[k].format != NULL; ++k)
	if (klf_text_format_formats[k].formatId == tf.type())
	  break;
      if (klf_text_format_formats[k].format == NULL) {
	// didn't find format, something is bound to go wrong, so fall back
	// on Qt's datastream saving.
	data = QByteArray();
	break;
      }
      // found format. This will be the first (value-less) section.
      sections << QPair<QByteArray,QByteArray>(klf_text_format_formats[k].format, QByteArray());

      QMap<int,QVariant>::const_iterator it;
      for (it = props.begin(); it != props.end(); ++it) {
	int propId = it.key();
	QVariant propValue = it.value();
	// Add data for this property.

	// first look to see if a keyword is already known to be available
	for (k = 0; klf_text_format_keywords[k].keyword != NULL; ++k)
	  if (klf_text_format_keywords[k].propId == propId &&
	      klf_text_format_keywords[k].fixed_value == propValue)
	    break;
	const char *kw = klf_text_format_keywords[k].keyword;
	if (kw != NULL) {
	  // found a keyword for this property-value pair
	  QByteArray key = kw;
	  sections << QPair<QByteArray,QByteArray>(kw, QByteArray());
	  continue;
	}

	// now look to see if we can name the property
	for (k = 0; klf_text_format_props[k].key != NULL; ++k)
	  if (klf_text_format_props[k].propId == propId)
	    break;
	if (klf_text_format_props[k].key != NULL) {
	  // make sure the variant has the advertised type
	  if ( !strcmp(klf_text_format_props[k].type, propValue.typeName()) ) {
	    // found the property in our list of common properties
	    QByteArray key = klf_text_format_props[k].key;
	    QByteArray value = klfSaveVariantToText(propValue, true); // resort to XML for lists/maps...
	    sections << QPair<QByteArray,QByteArray>(key, value);
	    continue;
	  } else {
	    qWarning()<<KLF_FUNC_NAME<<": QTextFormat property "<<klf_text_format_props[k].key
		      <<" 's type is `"<<propValue.typeName()<<"' which is not the known type: "
		      <<klf_text_format_props[k].type;
	  }
	}

	// this property is unknown to us. store it as we can.
	QByteArray key = QString::number(propId).toLatin1();
	QByteArray value;
	value = QByteArray("[")+propValue.typeName()+"]"+klfSaveVariantToText(propValue, true);
      }
      data = encaps_map(sections, true);
      break;
    }
  case QMetaType::QVariantList:
    {
      const QList<QVariant>& list = value.toList();
      if (saveListAndMapsAsXML) {
	QDomElement el = make_xml_wrapper("variant-list");
	el = klfSaveVariantListToXML(list, el);
	data = el.ownerDocument().toByteArray(0);
      } else {
	QList<QByteArray> sections;
	for (k = 0; k < list.size(); ++k) {
	  sections << klfSaveVariantToText(list[k]);
	}
	data = encaps_list(sections);
      }
      break;
    }
  case QMetaType::QVariantMap:
    {
      const QMap<QString,QVariant>& map = value.toMap();
      if (saveListAndMapsAsXML) {
	QDomElement el = make_xml_wrapper("variant-map");
	el = klfSaveVariantMapToXML(map, el);
	data = el.ownerDocument().toByteArray(0);
      } else {
	QList<QPair<QByteArray, QByteArray> > sections;
	for (QMap<QString,QVariant>::const_iterator it = map.begin(); it != map.end(); ++it) {
	  QByteArray k = klfSaveVariantToText(QVariant(it.key()));
	  QByteArray v = klfSaveVariantToText(it.value());
	  sections << QPair<QByteArray,QByteArray>(k, v);
	}
	data = encaps_map(sections);
      }
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

  // and provide a default encoding scheme in case no one up to now was able to
  // format the data (this format is only machine-readable ...)

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

  klfDbg( "klfSaveVariantToText("<<value<<"): saved data (len="<<data.size()<<") : "<<data ) ;
  return data;
}




KLF_EXPORT QVariant klfLoadVariantFromText(const QByteArray& stringdata, const char * dataTypeName,
					   const char *listOrMapDataTypeName)
{
  KLF_DEBUG_TIME_BLOCK(KLF_FUNC_NAME) ;

  // SOME REGULAR EXPRESSIONS

#define RX_INT "-?\\d+"
#define RX_COORD_SEP "\\s*(?:[,;]|\\s)\\s*" // note: non-capturing parenthesis
#define RX_SIZE_SEP "\\s*(?:[,;x]|\\s)\\s*" // note: non-capturing parenthesis

  //                     1                           2
  QRegExp v2rx("^\\(?\\s*(" RX_INT ")" RX_COORD_SEP "(" RX_INT ")\\s*\\)?");
  static const int V2RX_X = 1, V2RX_Y = 2;

  //                     1                          2
  QRegExp szrx("^\\(?\\s*(" RX_INT ")" RX_SIZE_SEP "(" RX_INT ")\\s*\\)?");
  static const int SZRX_W = 1, SZRX_H = 2;

  //                       1                           2
  QRegExp rectrx("^\\(?\\s*(" RX_INT ")" RX_COORD_SEP "(" RX_INT ")"
		 //                       3
		 "(?:" RX_COORD_SEP "|\\s*([+])\\s*)"
		 //4                                5         6
		 "(" RX_INT ")(?:"RX_COORD_SEP"|\\s*([x])\\s*)(" RX_INT ")\\s*\\)?");
  static const int RECTRX_X1 = 1, RECTRX_Y1 = 2, RECTRX_MIDDLESEP_PLUS = 3,
    RECTRX_X2orW = 4, RECTRX_LASTSEP_X = 5, RECTRX_Y2orH = 6;

  //                                1                     2                     3
  QRegExp colrx("^(?:rgba?)?\\(?\\s*(\\d+)" RX_COORD_SEP "(\\d+)" RX_COORD_SEP "(\\d+)"
		//4               5
		"(" RX_COORD_SEP "(\\d+))?\\s*\\)?", Qt::CaseInsensitive);
  static const int COLRX_R = 1, COLRX_G = 2, COLRX_B = 3, COLRX_MAYBE_ALPHA = 4, COLRX_A = 5;

  //                                       1                                2                     3
  QRegExp brushrx("^(?:q?brush)?\\(?\\s*(?:([A-Za-z_]\\w*)" RX_COORD_SEP ")?(\\d+)" RX_COORD_SEP "(\\d+)"
		  //            4         5               6
		  RX_COORD_SEP "(\\d+)"  "("RX_COORD_SEP "(\\d+))?" "\\s*\\)?", Qt::CaseInsensitive);
  static const int BRUSHRX_STYLE = 1, BRUSHRX_R = 2, BRUSHRX_G = 3, BRUSHRX_B = 4, BRUSHRX_A = 6;

  //               1           2
  QRegExp fontrx("^([\"']?)\\s*(.+)\\s*\\1"
		 //3   4                                             5
		 "(\\s+(Light|Normal|DemiBold|Bold|Black|Wgt\\s*=\\s*(\\d+)))?"
		 //6   7                        8    9
		 "(\\s+(Normal|Italic|Oblique))?(\\s+(\\d+))?$");
  fontrx.setMinimal(true); // don't match Light|Normal|DemiBold|... etc as part of font name
  static const int FONTRX_FAMILY = 2, FONTRX_WEIGHT_TEXT = 4, FONTRX_WEIGHT_VALUE = 5,
    FONTRX_STYLE_TEXT = 7, FONTRX_POINTSIZE = 9;


  // START DECODING TEXT

  QByteArray data = stringdata; // might need slight modifications before parsing

  QVariant value;
  if (data.startsWith("[QVariant]")) {
    QByteArray vdata_esc = data.mid(strlen("[QVariant]"));
    QByteArray vdata = klfEscapedToData(vdata_esc);
    klfDbg( "\tAbout to read raw variant from datastr="<<vdata_esc<<", ie. from data len="<<vdata.size() ) ;
    QDataStream stream(vdata);
    stream >> value;
    return value;
  }
  if (data.startsWith("\\"))
    data = data.mid(1);

  klfDbg( "Will start loading a `"<<dataTypeName<<"' from data (len="<<data.size()<<") : "<<data ) ;

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
  case QMetaType::QStringList:
    {
      QList<QByteArray> sections = decaps_list(data);

      // now we separated into bytearray sections. now read those into values.
      QStringList list;
      for (k = 0; k < sections.size(); ++k) {
	list << QString::fromUtf8(klfEscapedToData(sections[k]));
      }

      return QVariant::fromValue<QStringList>(list);
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
      if (szrx.indexIn(s) < 0)
	break;
      QStringList vals = szrx.capturedTexts();
      return QVariant::fromValue<QSize>(QSize(vals[SZRX_W].toInt(), vals[SZRX_H].toInt()));
    }
  case QMetaType::QPoint:
    {
      QString s = QString::fromLocal8Bit(data.trimmed());
      if (v2rx.indexIn(s) < 0)
	break;
      QStringList vals = v2rx.capturedTexts();
      return QVariant::fromValue<QPoint>(QPoint(vals[V2RX_X].toInt(), vals[V2RX_Y].toInt()));
    }
  case QMetaType::QRect:
    {
      QString s = QString::fromLocal8Bit(data.trimmed());
      if (rectrx.indexIn(s) < 0)
	break;
      QStringList vals = rectrx.capturedTexts();
      if (vals[RECTRX_MIDDLESEP_PLUS] == "+" || vals[RECTRX_LASTSEP_X] == "x") {
	return QVariant::fromValue<QRect>(QRect( QPoint(vals[RECTRX_X1].toInt(), vals[RECTRX_Y1].toInt()),
						 QSize(vals[RECTRX_X2orW].toInt(), vals[RECTRX_Y2orH].toInt()) ));
      }
      return QVariant::fromValue<QRect>(QRect( QPoint(vals[RECTRX_X1].toInt(), vals[RECTRX_Y1].toInt()),
					       QPoint(vals[RECTRX_X2orW].toInt(), vals[RECTRX_Y2orH].toInt()) ));
    }
  case QMetaType::QColor:
    {
      QString colstr = QString::fromLocal8Bit(data.trimmed());
      // try our regexp
      if (colrx.indexIn(colstr) < 0) {
	klfDbg("color "<<colstr<<" does not match regexp="<<colrx.pattern()<<", trying named...") ;
	// try a named color
	QColor color;  color.setNamedColor(colstr);
	// if we got a valid color, yepee
	if (color.isValid())
	  return color;
	break;
      }
      // our regexp matched
      QStringList vals = colrx.capturedTexts();
      QColor color = QColor(vals[COLRX_R].toInt(), vals[COLRX_G].toInt(), vals[COLRX_B].toInt(), 255);
      if (!vals[COLRX_MAYBE_ALPHA].isEmpty())
	color.setAlpha(vals[COLRX_A].toInt());
      return QVariant::fromValue<QColor>(color);
    }
  case QMetaType::QFont:
    {
      if (fontrx.indexIn(QString::fromLocal8Bit(data.trimmed())) < 0) {
	klfDbg("malformed font: "<<data);
	break;
      }
      QStringList vals = fontrx.capturedTexts();
      klfDbg("parsing font: data="<<data<<"; captured texts are: "<<vals );

      QString family = vals[FONTRX_FAMILY].trimmed();
      QString weighttxt = vals[FONTRX_WEIGHT_TEXT];
      QString weightval = vals[FONTRX_WEIGHT_VALUE];
      QString styletxt = vals[FONTRX_STYLE_TEXT];
      QString ptsval = vals[FONTRX_POINTSIZE];

      int weight = QFont::Normal;
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
  case QMetaType::QBrush:
    {
      if (brushrx.indexIn(QString::fromLocal8Bit(data.trimmed())) < 0) {
	klfDbg("malformed brush text: "<<data) ;
	break;
      }
      QStringList vals = brushrx.capturedTexts();
      QString style = vals[BRUSHRX_STYLE];
      // find brush style
      int k;
      bool style_found = false;
      for (k = 0; klf_brush_styles[k].brushStyle >= 0 && klf_brush_styles[k].style != NULL; ++k) {
	if (klf_brush_styles[k].style == style) {
	  style_found = true;
	  break;
	}
      }
      if (!style_found) {
	klfDbg("Can't find style"<<style<<" in brush style list!");
	break;
      }
      int qbrush_style = klf_brush_styles[k].brushStyle;
      // read the color and construct QBrush.
      QColor c = QColor(vals[BRUSHRX_R].toInt(),  vals[BRUSHRX_G].toInt(),
			vals[BRUSHRX_B].toInt());
      if (!vals[BRUSHRX_A].isEmpty())
	c.setAlpha(vals[BRUSHRX_A].toInt());
      return QBrush(c, static_cast<Qt::BrushStyle>(qbrush_style));
    }
  case QMetaType::QTextFormat:
    {
      int k;
      QList<QPair<QByteArray,QByteArray> > sections = decaps_map(data, true);
      if (sections.isEmpty()) {
	klfDbg("Invalid QTextFormat data.") ;
	break;
      }
      QPair<QByteArray,QByteArray> firstSection = sections.takeFirst();
      QString fmttype = QString::fromLatin1(firstSection.first);
      // find the format in our list
      for (k = 0; klf_text_format_formats[k].format != NULL; ++k)
	if (QString::compare(fmttype, QLatin1String(klf_text_format_formats[k].format),
			     Qt::CaseInsensitive) == 0)
	  break;
      if (klf_text_format_formats[k].format == NULL) {
	klfDbg("QTextFormat: Invalid format type: "<<fmttype) ;
	break;
      }
      int qtextformat_type = klf_text_format_formats[k].formatId;

      // now decode the list of properties
      QTextFormat textformat(qtextformat_type);
      QList<QPair<QByteArray,QByteArray> >::const_iterator it;
      for (it = sections.begin(); it != sections.end(); ++it) {
	QByteArray key = (*it).first.trimmed();
	QByteArray value = (*it).second;
	klfDbg("QTextFormat: considering property pair key="<<key<<"; value="<<value) ;
	// see if the key is a keyword
	for (k = 0; klf_text_format_keywords[k].keyword != NULL; ++k)
	  if (QString::compare(QLatin1String(klf_text_format_keywords[k].keyword),
			       key, Qt::CaseInsensitive) == 0)
	    break;
	if (klf_text_format_keywords[k].keyword != NULL) {
	  // this is a keyword.
	  klfDbg("QTextFormat: is keyword, propId="<<klf_text_format_keywords[k].propId<<", fixed_value="
		 <<klf_text_format_keywords[k].fixed_value) ;
	  textformat.setProperty(klf_text_format_keywords[k].propId,
				 klf_text_format_keywords[k].fixed_value);
	  continue;
	}
	// see if the key is a known property name
	for (k = 0; klf_text_format_props[k].key != NULL; ++k)
	  if (QString::compare(QLatin1String(klf_text_format_props[k].key),
			       key, Qt::CaseInsensitive) == 0)
	    break;
	if (klf_text_format_props[k].key != NULL) {
	  klfDbg("QTextFormat: is known property of type "<<klf_text_format_props[k].type) ;
	  // load property propId, of type type
	  QVariant vval = klfLoadVariantFromText(value, klf_text_format_props[k].type, "XML");
	  textformat.setProperty(klf_text_format_props[k].propId, vval);
	  continue;
	}
	// load generally-saved qvariant property

	bool tointok = true;
	int propid = key.toInt(&tointok);
	if (!tointok) {
	  qWarning()<<KLF_FUNC_NAME<<": QTextFormat bad format for general property key=value pair; "
		    <<"key is not a numerical property ID, nor is it a known property name.";
	}

	klfDbg("QTextFormat: property is not a known one. propid="<<propid) ;

	// trim space beginning of string
	while (value.size() && QChar(value[0]).isSpace())
	  value.remove(0, 1);
	int i;
	if (value.isEmpty() || !value.startsWith("[") || ((i = value.indexOf(']')) == -1)) {
	  qWarning().nospace()<<KLF_FUNC_NAME<<": QTextFormat bad format for general property, value does "
			      <<"not begin with \"[type-name]\".";
	  continue;
	}
	QByteArray typenm = value.mid(1, i-1);
	QByteArray valuedata = value.mid(i+1);
	QVariant vval = klfLoadVariantFromText(valuedata, typenm);
	klfDbg("setting generalized property "<<propid<<" to value "<<vval) ;
	textformat.setProperty(propid, vval);
      }
      return textformat;
    }
  case QMetaType::QVariantList:
    {
      if (listOrMapDataTypeName == QLatin1String("XML")) {
	QDomElement el = parse_xml_wrapper(data, "variant-list");
	return klfLoadVariantListFromXML(el);
      } else {
	QList<QByteArray> sections = decaps_list(data);

	// now we separated into bytearray sections. now read those into values.
	QVariantList list;
	for (k = 0; k < sections.size(); ++k) {
	  QVariant val = klfLoadVariantFromText(sections[k], listOrMapDataTypeName);
	list << val;
	}

	return QVariant::fromValue<QVariantList>(list);
      }
    }
  case QMetaType::QVariantMap:
    {
      if (listOrMapDataTypeName == QLatin1String("XML")) {
	QDomElement el = parse_xml_wrapper(data, "variant-map");
	return klfLoadVariantMapFromXML(el);
      } else {
	const QList<QPair<QByteArray,QByteArray> > sections = decaps_map(data);
	QVariantMap vmap;
	QList<QPair<QByteArray,QByteArray> >::const_iterator it;
	for (it = sections.begin(); it != sections.end(); ++it) {
	  QString key = klfLoadVariantFromText((*it).first, "QString").toString();
	  QVariant value = klfLoadVariantFromText((*it).second, listOrMapDataTypeName);
	  vmap[key] = value;
	}
	return QVariant::fromValue<QVariantMap>(vmap);
      }
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





// ----------------------------------------------------





KLF_EXPORT QDomElement klfSaveVariantMapToXML(const QVariantMap& vmap, QDomElement baseNode)
{
  QDomDocument doc = baseNode.ownerDocument();

  for (QVariantMap::const_iterator it = vmap.begin(); it != vmap.end(); ++it) {
    QString key = it.key();
    QVariant value = it.value();
    
    QDomElement pairNode = doc.createElement("pair");
    // * key
    QDomElement keyNode = doc.createElement("key");
    QDomText keyText = doc.createTextNode(key);
    keyNode.appendChild(keyText);
    pairNode.appendChild(keyNode);
    // * value data
    QDomElement vdataNode = doc.createElement("value");
    QString vtype = QLatin1String(value.typeName());
    vdataNode.setAttribute(QLatin1String("type"), vtype);
    if (vtype == "QVariantMap") {
      vdataNode = klfSaveVariantMapToXML(value.toMap(), vdataNode);
    } else if (vtype == "QVariantList") {
      vdataNode = klfSaveVariantListToXML(value.toList(), vdataNode);
    } else {
      QDomText vdataText = doc.createTextNode(QString::fromLocal8Bit(klfSaveVariantToText(value)));
      vdataNode.appendChild(vdataText);
    }
    pairNode.appendChild(vdataNode);
    // now append this pair to our list
    baseNode.appendChild(pairNode);
  }
  return baseNode;
}

KLF_EXPORT QVariantMap klfLoadVariantMapFromXML(const QDomElement& xmlNode)
{
  KLF_DEBUG_BLOCK(KLF_FUNC_NAME) ;

  QVariantMap vmap;

  QDomNode n;
  for (n = xmlNode.firstChild(); ! n.isNull(); n = n.nextSibling()) {
    QDomElement e = n.toElement(); // try to convert the node to an element.
    if ( e.isNull() || n.nodeType() != QDomNode::ElementNode )
      continue;
    if ( e.nodeName() != "pair" ) {
      qWarning("%s: ignoring unexpected tag `%s'!\n", KLF_FUNC_NAME, qPrintable(e.nodeName()));
      continue;
    }
    // read this pair
    QString key;
    QByteArray valuetype;
    QByteArray valuedata;
    QDomElement valueNode;
    QDomNode nn;
    for (nn = e.firstChild(); ! nn.isNull(); nn = nn.nextSibling()) {
      klfDbg("inside <pair>: read node "<<nn.nodeName()) ;
      QDomElement ee = nn.toElement();
      if ( ee.isNull() || nn.nodeType() != QDomNode::ElementNode )
	continue;
      if ( ee.nodeName() == "key" ) {
	key = ee.text();
	continue;
      }
      if ( ee.nodeName() == "value" ) {
	// "local 8-bit"  because klfLoadVariantFromText() assumes local 8-bit encoding
	valueNode = ee;
	valuedata = ee.text().toLocal8Bit();
	valuetype = ee.attribute("type").toLatin1();
	continue;
      }
      qWarning("%s: ignoring unexpected tag `%s' in <pair>!\n", KLF_FUNC_NAME,
	       qPrintable(ee.nodeName()));
    }
    QVariant value;
    if (valuetype == "QVariantMap") {
      value = QVariant::fromValue<QVariantMap>(klfLoadVariantMapFromXML(valueNode));
    } else if (valuetype == "QVariantList") {
      value = QVariant::fromValue<QVariantList>(klfLoadVariantListFromXML(valueNode));
    } else {
      value = klfLoadVariantFromText(valuedata, valuetype.constData());
    }
    // set this value in our variant map
    vmap[key] = value;
  }
  return vmap;
}


KLF_EXPORT QDomElement klfSaveVariantListToXML(const QVariantList& vlist, QDomElement baseNode)
{
  QDomDocument doc = baseNode.ownerDocument();

  for (QVariantList::const_iterator it = vlist.begin(); it != vlist.end(); ++it) {
    QVariant value = *it;

    QDomElement elNode = doc.createElement(QLatin1String("item"));
    QString vtype = QString::fromLatin1(value.typeName()); // "Latin1" encoding by convention
    //                                        because type names do not have any special chars
    elNode.setAttribute(QLatin1String("type"), vtype);
    if (vtype == "QVariantMap") {
      elNode = klfSaveVariantMapToXML(value.toMap(), elNode);
    } else if (vtype == "QVariantList") {
      elNode = klfSaveVariantListToXML(value.toList(), elNode);
    } else {
      QDomText vdataText = doc.createTextNode(QString::fromLocal8Bit(klfSaveVariantToText(value)));
      elNode.appendChild(vdataText);
    }
    // now append this pair to our list
    //klfDbg( "... appending node!" ) ;
    baseNode.appendChild(elNode);
  }

  return baseNode;
}

KLF_EXPORT QVariantList klfLoadVariantListFromXML(const QDomElement& xmlNode)
{
  KLF_DEBUG_BLOCK(KLF_FUNC_NAME) ;

  QVariantList vlist;

  QDomNode n;
  for (n = xmlNode.firstChild(); ! n.isNull(); n = n.nextSibling()) {
    QDomElement e = n.toElement(); // try to convert the node to an element.
    if ( e.isNull() || n.nodeType() != QDomNode::ElementNode )
      continue;
    if ( e.nodeName() != QLatin1String("item") ) {
      qWarning("%s: ignoring unexpected tag `%s'!\n", KLF_FUNC_NAME, qPrintable(e.nodeName()));
      continue;
    }

    QString vtype = e.attribute(QLatin1String("type"));

    QVariant value;
    if (vtype == QLatin1String("QVariantMap")) {
      value = QVariant::fromValue<QVariantMap>(klfLoadVariantMapFromXML(e));
    } else if (vtype == QLatin1String("QVariantList")) {
      value = QVariant::fromValue<QVariantList>(klfLoadVariantListFromXML(e));
    } else {
      value = klfLoadVariantFromText(e.text().toLocal8Bit(), vtype.toLatin1().constData());
    }

    // set this value in our variant map
    vlist << value;
  }
  return vlist;
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
