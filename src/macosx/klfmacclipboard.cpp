/***************************************************************************
 *   file klfmacclipboard.cpp
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

#include <QMacPasteboardMime>
#include <QTextCodec>
#include <QDebug>

#include <klfdefs.h>

#include "klfmime.h"
#include "klfmacclipboard.h"



KLFMacPasteboardMime::KLFMacPasteboardMime()
  : QMacPasteboardMime(MIME_ALL)
{
  klfDbg("constructor.") ;
}

KLFMacPasteboardMime::~KLFMacPasteboardMime()
{
  klfDbg("destructor.") ;
}


bool KLFMacPasteboardMime::canConvert(const QString& mime, QString flav)
{
  KLF_DEBUG_BLOCK(KLF_FUNC_NAME) ;
  Q_UNUSED(flav);
  klfDbg("mime="<<mime<<" flav="<<flav) ;

  if (flav.startsWith("com.trolltech.anymime.")) {
    // Qt internal mac UTI (flavor)
    return false;
  }

  const QString prefix = QLatin1String(KLF_MIME_PROXY_MAC_FLAVOR_PREFIX) ;
  return mime.startsWith(prefix) && (mime.mid(prefix.size()) == flav);
}

QList<QByteArray> KLFMacPasteboardMime::convertFromMime(const QString& mime, QVariant data,
							QString flav)
{
  KLF_DEBUG_BLOCK(KLF_FUNC_NAME) ;
  klfDbg("mime="<<mime<<" flav="<<flav) ;

  if (!canConvert(mime, flav)) {
    klfDbg("Can't convert!") ;
    return QList<QByteArray>();
  }

  QByteArray d = data.toByteArray();
  klfDbg("returning data "<<data<<" as bytearray: length="<<d.size()<<" for conversion from "
	 <<mime<<" to "<<flav) ;

  return QList<QByteArray>() << d;
}

QVariant KLFMacPasteboardMime::convertToMime(const QString& mime, QList<QByteArray> data,
					     QString flav)
{
  KLF_DEBUG_BLOCK(KLF_FUNC_NAME) ;
  klfDbg("mime="<<mime<<" flav="<<flav) ;

  if (!canConvert(mime, flav)) {
    klfDbg("Can't convert!") ;
    return QVariant();
  }

  if (data.count() > 1) {
    klfWarning("Can't handle data.size()>1 (data.size()="<<data.size()<<"), in conversion from "
               << mime << " to " << flav) ;
    return QVariant();
  }
  if (data.count() <= 0) {
    klfDbg("no data.");
    return QVariant();
  }

  klfDbg("data size="<<data[0].length()) ;
  return data[0];
}

QString KLFMacPasteboardMime::flavorFor(const QString& mime)
{
  KLF_DEBUG_BLOCK(KLF_FUNC_NAME) ;
  klfDbg("mime="<<mime) ;

  if (mime.startsWith(KLF_MIME_PROXY_MAC_FLAVOR_PREFIX)) {
    return mime.mid(QLatin1String(KLF_MIME_PROXY_MAC_FLAVOR_PREFIX).size());
  }

  klfDbg("Mime type is not a KLF mac flavor proxy.") ;
  return QString();
}

QString KLFMacPasteboardMime::mimeFor(QString flav)
{
  KLF_DEBUG_BLOCK(KLF_FUNC_NAME) ;
  klfDbg("flav="<<flav) ;

  if (flav.startsWith("com.trolltech.anymime.")) {
    // Qt internal UTI (flavor)
    return QString();
  }

  return QString::fromLatin1(KLF_MIME_PROXY_MAC_FLAVOR_PREFIX) + flav;
}


