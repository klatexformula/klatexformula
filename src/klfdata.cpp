/***************************************************************************
 *   file klfdata.cpp
 *   This file is part of the KLatexFormula Project.
 *   Copyright (C) 2007 by Philippe Faist
 *   philippe.faist@bluewin.ch
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

#include "klfdata.h"



Q_UINT32 KLFData::KLFHistoryItem::MaxId = 1;


QDataStream& operator<<(QDataStream& stream, const KLFData::KLFStyle& style)
{
  return stream << style.name << (Q_UINT32)style.fg_color << (Q_UINT32)style.bg_color
		<< style.mathmode << style.preamble << (Q_UINT16)style.dpi;
}
QDataStream& operator>>(QDataStream& stream, KLFData::KLFStyle& style)
{
  Q_UINT32 fg, bg;
  Q_UINT16 dpi;
  stream >> style.name;
  stream >> fg >> bg >> style.mathmode >> style.preamble >> dpi;
  style.fg_color = fg;
  style.bg_color = bg;
  style.dpi = dpi;
  return stream;
}

QDataStream& operator<<(QDataStream& stream, const KLFData::KLFHistoryItem& item)
{
  return stream << item.id << item.datetime << item.latex << item.preview << item.style;
}

QDataStream& operator>>(QDataStream& stream, KLFData::KLFHistoryItem& item)
{
  return stream >> item.id >> item.datetime >> item.latex >> item.preview >> item.style;
}

bool operator==(const KLFData::KLFStyle& a, const KLFData::KLFStyle& b)
{
  return a.name == b.name &&
    a.fg_color == b.fg_color &&
    a.bg_color == b.bg_color &&
    a.mathmode == b.mathmode &&
    a.preamble == b.preamble &&
    a.dpi == b.dpi;
}

bool operator==(const KLFData::KLFHistoryItem& a, const KLFData::KLFHistoryItem& b)
{
  return
    //    a.id == b.id &&   // don't compare IDs since they should be different.
    //    a.datetime == b.datetime &&   // same for datetime
    a.latex == b.latex &&
    //    a.preview == b.preview && // don't compare preview: it's unnecessary
    a.style == b.style;
}
