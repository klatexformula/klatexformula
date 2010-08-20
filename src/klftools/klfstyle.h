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
      preamble(input.preamble), dpi(input.dpi)
  {
  }

  KLFStyle(const KLFStyle& o)
    : name(o.name), fg_color(o.fg_color), bg_color(o.bg_color), mathmode(o.mathmode),
      preamble(o.preamble), dpi(o.dpi)
  {
  }

  QString name; ///< this may not always be set, it's only important in saved style list.
  unsigned long fg_color;
  unsigned long bg_color;
  QString mathmode;
  QString preamble;
  int dpi;

  inline const KLFStyle& operator=(const KLFStyle& o) {
    name = o.name; fg_color = o.fg_color; bg_color = o.bg_color; mathmode = o.mathmode;
    preamble = o.preamble; dpi = o.dpi;
    return *this;
  }
};

Q_DECLARE_METATYPE(KLFStyle)
  ;

KLF_EXPORT QString prettyPrintStyle(const KLFStyle& sty);

typedef QList<KLFStyle> KLFStyleList;

KLF_EXPORT QDataStream& operator<<(QDataStream& stream, const KLFStyle& style);
KLF_EXPORT QDataStream& operator>>(QDataStream& stream, KLFStyle& style);
// exact matches
KLF_EXPORT bool operator==(const KLFStyle& a, const KLFStyle& b);




#endif
