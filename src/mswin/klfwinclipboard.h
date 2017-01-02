/***************************************************************************
 *   file klfwinclipboard.h
 *   This file is part of the KLatexFormula Project.
 *   Copyright (C) 2016 by Philippe Faist
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


#ifndef KLFWINCLIPBOARD_H
#define KLFWINCLIPBOARD_H

#include <QWinMime>

#include <klfdefs.h>


struct KLFWinClipboardPrivate;

class KLFWinClipboard : public QWinMime
{
public:
  KLFWinClipboard();
  virtual ~KLFWinClipboard();
  virtual bool canConvertFromMime(const FORMATETC &formatetc, const QMimeData *mimeData) const;
  virtual bool canConvertToMime(const QString &mimeType, IDataObject *pDataObj) const;
  virtual bool convertFromMime(const FORMATETC &formatetc, const QMimeData *mimeData,
                               STGMEDIUM *pmedium) const;
  virtual QVariant convertToMime(const QString &mimeType, IDataObject *pDataObj,
                                 QVariant::Type preferredType) const;
  virtual QVector<FORMATETC> formatsForMime(const QString &mimeType, const QMimeData *mimeData) const;
  virtual QString mimeForFormat(const FORMATETC &formatetc) const;
private:
  KLF_DECLARE_PRIVATE(KLFWinClipboard) ;
};


#endif
