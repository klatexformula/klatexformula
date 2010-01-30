/***************************************************************************
 *   file klfdata.h
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

#ifndef KLFDATA_H
#define KLFDATA_H

#include <QDateTime>
#include <QList>
#include <QDataStream>
#include <QPixmap>
#include <QMap>

#include <klfdefs.h>



/** \brief A Formula Style (collection of properties)
 *
 * Structure containing forground color, bg color, mathmode, preamble, etc.
 */
struct KLFStyle {
  KLFStyle(QString nm = QString(), unsigned long fgcol = qRgba(0,0,0,255),
	   unsigned long bgcol = qRgba(255,255,255,0),
	   const QString& mmode = QString(),
	   const QString& pre = QString(),
	   int dotsperinch = -1)
    : name(nm), fg_color(fgcol), bg_color(bgcol), mathmode(mmode), preamble(pre),
      dpi(dotsperinch) { }
  KLFStyle(const KLFStyle& o)
    : name(o.name), fg_color(o.fg_color), bg_color(o.bg_color), mathmode(o.mathmode),
      preamble(o.preamble), dpi(o.dpi) { }

  QString name; // this may not always be set, it's only important in saved style list.
  unsigned long fg_color;
  unsigned long bg_color;
  QString mathmode;
  QString preamble;
  int dpi;

  const KLFStyle& operator=(const KLFString& other) {
    name = o.name; fg_color = o.fg_color; bg_color = o.bg_color; mathmode = o.mathmode;
    preamble = o.preamble; dpi = o.dpi;
    return *this;
  }
};


Q_DECLARE_METATYPE(MyStruct)
  ;

static QString prettyPrintStyle(const KLFStyle& sty);

typedef QList<KLFStyle> KLFStyleList;

QDataStream& operator<<(QDataStream& stream, const KLFStyle& style);
QDataStream& operator>>(QDataStream& stream, KLFStyle& style);
// exact matches
bool operator==(const KLFStyle& a, const KLFStyle& b);




/** Data structures for KLatexFormula
 * \author Philippe Faist &lt;philippe.faist at bluewin.ch&gt;
 */
class KLF_EXPORT KLFData {
public:

  // THESE VALUES MUST NOT CHANGE FROM ONE VERSION TO ANOTHER OF KLATEXFORMULA :
  enum {  LibResource_History = 0, LibResource_Archive = 1,
    // user resources must be in the following range:
    LibResourceUSERMIN = 100,
    LibResourceUSERMAX = 99999
  };

  struct KLFLibraryResource {
    quint32 id;
    QString name;
  };

  struct KLFLibraryItem {
    quint32 id;
    static quint32 MaxId;

    QDateTime datetime;
    /** \note \c latex contains also information of category (first line, %: ...) and
     * tags (first/second line, after category: % ...) */
    QString latex;
    QPixmap preview;

    QString category;
    QString tags;

    KLFStyle style;
  };

  static QString categoryFromLatex(const QString& latex);
  static QString tagsFromLatex(const QString& latex);
  static QString stripCategoryTagsFromLatex(const QString& latex);

  typedef QList<KLFLibraryItem> KLFLibraryList;
  typedef QList<KLFLibraryResource> KLFLibraryResourceList;
  typedef QMap<KLFLibraryResource, KLFLibraryList> KLFLibrary;

private:

  KLFData();
};



// it is important to note that the >> operator imports in a compatible way to KLF 2.0
QDataStream& operator<<(QDataStream& stream, const KLFData::KLFLibraryItem& item);
QDataStream& operator>>(QDataStream& stream, KLFData::KLFLibraryItem& item);

QDataStream& operator<<(QDataStream& stream, const KLFData::KLFLibraryResource& item);
QDataStream& operator>>(QDataStream& stream, KLFData::KLFLibraryResource& item);

// exact matches, style included, but excluding ID and datetime
bool operator==(const KLFData::KLFLibraryItem& a, const KLFData::KLFLibraryItem& b);

// is needed for QMap : these operators compare ID only.
bool operator<(const KLFData::KLFLibraryResource a, const KLFData::KLFLibraryResource b);
bool operator==(const KLFData::KLFLibraryResource a, const KLFData::KLFLibraryResource b);
// name comparision
bool resources_equal_for_import(const KLFData::KLFLibraryResource a, const KLFData::KLFLibraryResource b);

#endif
