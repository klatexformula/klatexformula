/***************************************************************************
 *   file klfdata.h
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

#ifndef KLFDATA_H
#define KLFDATA_H

#include <qdatetime.h>
#include <qvaluelist.h>
#include <qdatastream.h>
#include <qpixmap.h>

/** Data for KLatexFormula
 * \author Philippe Faist &lt;philippe.faist@bluewin.ch&gt;
 */
class KLFData {
public:

  struct KLFStyle {
    QString name; // this may not always be set, it's only important in saved style list.
    unsigned long fg_color;
    unsigned long bg_color;
    QString mathmode;
    QString preamble;
    int dpi;
  };

  struct KLFHistoryItem {
    Q_UINT32 id;
    static Q_UINT32 MaxId;

    QDateTime datetime;
    QString latex;
    QPixmap preview;
    KLFStyle style;
  };


  typedef QValueList<KLFStyle> KLFStyleList;
  typedef QValueList<KLFHistoryItem> KLFHistoryList;


private:

  KLFData();
};


QDataStream& operator<<(QDataStream& stream, const KLFData::KLFStyle& style);
QDataStream& operator>>(QDataStream& stream, KLFData::KLFStyle& style);

QDataStream& operator<<(QDataStream& stream, const KLFData::KLFHistoryItem& item);
QDataStream& operator>>(QDataStream& stream, KLFData::KLFHistoryItem& item);


#endif
