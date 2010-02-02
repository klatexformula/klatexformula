/***************************************************************************
 *   file klfdata.cpp
 *   This file is part of the KLatexFormula Project.
 *   Copyright (C) 2007 by Philippe Faist
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

#include <QString>
#include <QObject>
#include <QDataStream>

#include "klfdata.h"



quint32 KLFData::KLFLibraryItem::MaxId = 1;



// static method
QString KLFData::categoryFromLatex(const QString& latex)
{
  QString s = latex.section('\n', 0, 0, QString::SectionSkipEmpty);
  if (s[0] == '%' && s[1] == ':') {
    return s.mid(2).trimmed();
  }
  return QString::null;
}
// static method
QString KLFData::tagsFromLatex(const QString& latex)
{
  QString s = latex.section('\n', 0, 0, QString::SectionSkipEmpty);
  if (s[0] == '%' && s[1] == ':') {
    // category is s.mid(2);
    s = latex.section('\n', 1, 1, QString::SectionSkipEmpty);
  }
  if (s[0] == '%') {
    return s.mid(1).trimmed();
  }
  return QString::null;
}

QString KLFData::stripCategoryTagsFromLatex(const QString& latex)
{
  int k = 0;
  while (k < latex.length() && latex[k].isSpace())
    ++k;
  if (k == latex.length()) return "";
  if (latex[k] == '%') {
    ++k;
    if (k == latex.length()) return "";
    //strip category and/or tag:
    if (latex[k] == ':') {
      // strip category
      while (k < latex.length() && latex[k] != '\n')
	++k;
      ++k;
      if (k >= latex.length()) return "";
      if (latex[k] != '%') {
	// there isn't any tags, just category; return rest of string
	return latex.mid(k);
      }
      ++k;
      if (k >= latex.length()) return "";
    }
    // strip tag:
    while (k < latex.length() && latex[k] != '\n')
      ++k;
    ++k;
    if (k >= latex.length()) return "";
  }
  // k is the beginnnig of the latex string
  return latex.mid(k);
}




QString prettyPrintStyle(const KLFStyle& sty)
{
  QString s = "";
  if (sty.name != QString::null)
    s = QObject::tr("<b>Style Name</b>: %1<br>").arg(sty.name);
  return s + QObject::tr("<b>Math Mode</b>: %1<br>"
			 "<b>DPI Resolution</b>: %2<br>"
			 "<b>Foreground Color</b>: %3 <font color=\"%4\"><b>[SAMPLE]</b></font><br>"
			 "<b>Background is Transparent</b>: %5<br>"
			 "<b>Background Color</b>: %6 <font color=\"%7\"><b>[SAMPLE]</b></font><br>"
			 "<b>LaTeX Preamble:</b><br><pre>%8</pre>")
    .arg(sty.mathmode)
    .arg(sty.dpi)
    .arg(QColor(sty.fg_color).name()).arg(QColor(sty.fg_color).name())
    .arg( (qAlpha(sty.bg_color) != 255) ? QObject::tr("YES") : QObject::tr("NO") )
    .arg(QColor(sty.bg_color).name()).arg(QColor(sty.bg_color).name())
    .arg(sty.preamble)
    ;
}



QDataStream& operator<<(QDataStream& stream, const KLFStyle& style)
{
  return stream << style.name << (quint32)style.fg_color << (quint32)style.bg_color
		<< style.mathmode << style.preamble << (quint16)style.dpi;
}
QDataStream& operator>>(QDataStream& stream, KLFStyle& style)
{
  quint32 fg, bg;
  quint16 dpi;
  stream >> style.name;
  stream >> fg >> bg >> style.mathmode >> style.preamble >> dpi;
  style.fg_color = fg;
  style.bg_color = bg;
  style.dpi = dpi;
  return stream;
}

QDataStream& operator<<(QDataStream& stream, const KLFData::KLFLibraryItem& item)
{
  return stream << item.id << item.datetime
      << item.latex // category and tags are included.
      << item.preview << item.style;
}

// it is important to note that the >> operator imports in a compatible way to KLF 2.0
QDataStream& operator>>(QDataStream& stream, KLFData::KLFLibraryItem& item)
{
  stream >> item.id >> item.datetime >> item.latex >> item.preview >> item.style;
  item.category = KLFData::categoryFromLatex(item.latex);
  item.tags = KLFData::tagsFromLatex(item.latex);
  return stream;
}


QDataStream& operator<<(QDataStream& stream, const KLFData::KLFLibraryResource& item)
{
  return stream << item.id << item.name;
}
QDataStream& operator>>(QDataStream& stream, KLFData::KLFLibraryResource& item)
{
  return stream >> item.id >> item.name;
}


bool operator==(const KLFStyle& a, const KLFStyle& b)
{
  return a.name == b.name &&
    a.fg_color == b.fg_color &&
    a.bg_color == b.bg_color &&
    a.mathmode == b.mathmode &&
    a.preamble == b.preamble &&
    a.dpi == b.dpi;
}

bool operator==(const KLFData::KLFLibraryItem& a, const KLFData::KLFLibraryItem& b)
{
  return
    //    a.id == b.id &&   // don't compare IDs since they should be different.
    //    a.datetime == b.datetime &&   // same for datetime
    a.latex == b.latex &&
    /* the following is unnecessary since category/tags information is contained in .latex
      a.category == b.category &&
      a.tags == b.tags &&
    */
    //    a.preview == b.preview && // don't compare preview: it's unnecessary and overkill
    a.style == b.style;
}



bool operator<(const KLFData::KLFLibraryResource a, const KLFData::KLFLibraryResource b)
{
  return a.id < b.id;
}
bool operator==(const KLFData::KLFLibraryResource a, const KLFData::KLFLibraryResource b)
{
  return a.id == b.id;
}

bool resources_equal_for_import(const KLFData::KLFLibraryResource a, const KLFData::KLFLibraryResource b)
{
  return a.name == b.name;
}
