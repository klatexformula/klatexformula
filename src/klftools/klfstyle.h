/***************************************************************************
 *   file klfstyle.h
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

#ifndef KLFSTYLE_H
#define KLFSTYLE_H

#include <QList>
#include <QDataStream>

#include <klfbackend.h>


/** \brief A Formula Style (collection of properties)
 *
 * Structure containing forground color, bg color, mathmode, preamble, etc.
 */
struct KLF_EXPORT KLFStyle {
  /** \brief a structure memorizing parameters for bbox expansion
   *
   * Stores how much to expand (EPS) BBox in each of top, right, bottom, and left directions,
   * in units of Postscript Points.
   *
   * BBox expansion is done in KLFBackend::getLatexFormula() to add margins on the sides of the
   * resulting image.
   *
   * Type is stored as \c double for now, however the backend requires integer values. This is to
   * allow for future improvement of klfbackend to accept float values.
   */
  struct KLF_EXPORT BBoxExpand {
    BBoxExpand(double t = -1, double r = -1, double b = -1, double l = -1)
      : top(t), right(r), bottom(b), left(l)  { }
    BBoxExpand(const BBoxExpand& c) : top(c.top), right(c.right), bottom(c.bottom), left(c.left) { }

    inline bool valid() const { return top >= 0 && right >= 0 && bottom >= 0 && left >= 0; }

    double top;
    double right;
    double bottom;
    double left;
    inline const BBoxExpand& operator=(const BBoxExpand& other)
    { top = other.top; right = other.right; bottom = other.bottom; left = other.left;  return *this; }
    inline bool operator==(const BBoxExpand& x) const
    { return top == x.top && right == x.right && bottom == x.bottom && left == x.left; }
  };

  KLFStyle(QString nm = QString(), unsigned long fgcol = qRgba(0,0,0,255),
	   unsigned long bgcol = qRgba(255,255,255,0),
	   const QString& mmode = QString(),
	   const QString& pre = QString(),
	   int dotsperinch = -1)
    : name(nm), fg_color(fgcol), bg_color(bgcol), mathmode(mmode), preamble(pre),
      dpi(dotsperinch)
  {
  }

  KLFStyle(const KLFBackend::klfInput& input)
    : name(), fg_color(input.fg_color), bg_color(input.bg_color), mathmode(input.mathmode),
      preamble(input.preamble), dpi(input.dpi), overrideBBoxExpand()
  {
  }

  KLFStyle(const KLFStyle& o)
    : name(o.name), fg_color(o.fg_color), bg_color(o.bg_color), mathmode(o.mathmode),
      preamble(o.preamble), dpi(o.dpi), overrideBBoxExpand(o.overrideBBoxExpand)
  {
  }

  QString name; ///< this may not always be set, it's only important in saved style list.
  unsigned long fg_color;
  unsigned long bg_color;
  QString mathmode;
  QString preamble;
  int dpi;
  BBoxExpand overrideBBoxExpand;

  inline const KLFStyle& operator=(const KLFStyle& o) {
    name = o.name; fg_color = o.fg_color; bg_color = o.bg_color; mathmode = o.mathmode;
    preamble = o.preamble; dpi = o.dpi; overrideBBoxExpand = o.overrideBBoxExpand;
    return *this;
  }
};

Q_DECLARE_METATYPE(KLFStyle)
  ;

typedef QList<KLFStyle> KLFStyleList;

KLF_EXPORT QDataStream& operator<<(QDataStream& stream, const KLFStyle& style);
KLF_EXPORT QDataStream& operator>>(QDataStream& stream, KLFStyle& style);
// exact matches
KLF_EXPORT bool operator==(const KLFStyle& a, const KLFStyle& b);




#endif
