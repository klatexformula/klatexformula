/***************************************************************************
 *   file klfstyle.cpp
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

#include <QColor>

#include "klfstyle.h"

/** \bug BUG/TODO: save user-script in style. Think: full path, only basename, ... ? */


KLF_EXPORT QDataStream& operator<<(QDataStream& stream, const KLFStyle::BBoxExpand& b)
{
  return stream << b.top << b.right << b.bottom << b.left;
}

KLF_EXPORT QDataStream& operator>>(QDataStream& stream, KLFStyle::BBoxExpand& b)
{
  return stream >> b.top >> b.right >> b.bottom >> b.left;
}


KLF_EXPORT QDataStream& operator<<(QDataStream& stream, const KLFStyle& style)
{
  // yes, QIODevice inherits QObject and we can use dynamic properties...
  QString compat_klfversion = stream.device()->property("klfDataStreamAppVersion").toString();
  if (klfVersionCompare(compat_klfversion, "3.1") <= 0) {
    return stream << style.name << (quint32)style.fg_color << (quint32)style.bg_color
		  << style.mathmode << style.preamble << (quint16)style.dpi;
  } else if (klfVersionCompare(compat_klfversion, "3.3") < 0) {
    return stream << style.name << (quint32)style.fg_color << (quint32)style.bg_color
		  << style.mathmode << style.preamble << (quint16)style.dpi
		  << style.overrideBBoxExpand;
  } else {
    return stream << style.name << (quint32)style.fg_color << (quint32)style.bg_color
		  << style.mathmode << style.preamble << (quint16)style.dpi
      		  << style.overrideBBoxExpand << style.userScript;
  }
}
KLF_EXPORT QDataStream& operator>>(QDataStream& stream, KLFStyle& style)
{
  QString compat_klfversion = stream.device()->property("klfDataStreamAppVersion").toString();
  if (klfVersionCompare(compat_klfversion, "3.1") <= 0) {
    quint32 fg, bg;
    quint16 dpi;
    stream >> style.name;
    stream >> fg >> bg >> style.mathmode >> style.preamble >> dpi;
    style.fg_color = fg;
    style.bg_color = bg;
    style.dpi = dpi;
    return stream;
  } else if (klfVersionCompare(compat_klfversion, "3.3") < 0) {
    quint32 fg, bg;
    quint16 dpi;
    stream >> style.name;
    stream >> fg >> bg >> style.mathmode >> style.preamble >> dpi;
    style.fg_color = fg;
    style.bg_color = bg;
    style.dpi = dpi;
    stream >> style.overrideBBoxExpand;
    return stream;
  } else {
    quint32 fg, bg;
    quint16 dpi;
    stream >> style.name;
    stream >> fg >> bg >> style.mathmode >> style.preamble >> dpi;
    style.fg_color = fg;
    style.bg_color = bg;
    style.dpi = dpi;
    stream >> style.overrideBBoxExpand;
    stream >> style.userScript;
    return stream;
  }
}

KLF_EXPORT bool operator==(const KLFStyle& a, const KLFStyle& b)
{
  return a.name == b.name &&
    a.fg_color == b.fg_color &&
    a.bg_color == b.bg_color &&
    a.mathmode == b.mathmode &&
    a.preamble == b.preamble &&
    a.dpi == b.dpi &&
    a.overrideBBoxExpand == b.overrideBBoxExpand &&
    a.userScript == b.userScript;
}




