/***************************************************************************
 *   file klfmacclipboard.cpp
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

#include <QMacPasteboardMime>
#include <QDebug>

#include <klfdefs.h>

#include "klfmime.h"
#include "klfmacclipboard.h"



static KLFMacPasteboardMime *__klf_the_macpasteboardmime = NULL;

void __klf_init_the_macpasteboardmime()
{
  if (__klf_the_macpasteboardmime == NULL)
    __klf_the_macpasteboardmime = new KLFMacPasteboardMime();
}


// additionally : see
// http://developer.apple.com/library/mac/#documentation/Miscellaneous/Reference/UTIRef/Articles/System-DeclaredUniformTypeIdentifiers.html
//com.adobe.pdf
//com.adobe.postscript
//com.adobe.encapsulated-postscript


KLFMacPasteboardMime::KLFMacPasteboardMime()
  : QMacPasteboardMime(MIME_ALL)
{
  klfDbg("constructor.") ;
}


bool KLFMacPasteboardMime::canConvert(const QString& mime, QString flav)
{
  klfDbg("mime="<<mime<<" flav="<<flav) ;
  if (mime == "image/png" && flav == "public.png")
    return true;
  if (mime == "image/jpeg" && flav == "public.jpeg")
    return true;
  if (mime == "application/postscript" && flav == "public.ps")
    return true;
  klfDbg("can't convert.") ;
  return false;
}
QList<QByteArray> KLFMacPasteboardMime::convertFromMime(const QString& mime, QVariant data, QString flav)
{
  klfDbg("mime="<<mime<<" flav="<<flav) ;
  if (mime == "image/png" && flav == "public.png") {
    QByteArray d = data.toByteArray();
    klfDbg("returning data "<<data<<" as bytearray: length="<<d.size()) ;
    return QList<QByteArray>() << d;
  }
  if (mime == "image/jpeg" && flav == "public.jpeg")
    return QList<QByteArray>() << data.toByteArray();
  klfDbg("can't convert.") ;
  return QList<QByteArray>();
}
QVariant KLFMacPasteboardMime::convertToMime(const QString& mime, QList<QByteArray> data, QString flav)
{
  klfDbg("mime="<<mime<<" flav="<<flav) ;
  if (data.size() > 1)
    qWarning()<<KLF_FUNC_NAME<<": can't handle multiple member data";

  klfDbg("data size="<<data[0].length()) ;
  return data[0];
}
QString KLFMacPasteboardMime::flavorFor(const QString& mime)
{
  klfDbg("mime="<<mime) ;
  if (mime == "image/jpeg")
    return "public.jpeg";
  if (mime == "image/png")
    return "public.png";

  klfDbg("unknown...") ;
  return QString();
}
QString KLFMacPasteboardMime::mimeFor(QString flav)
{
  klfDbg("flav="<<flav) ;
  if (flav == "public.jpeg")
    return "image/jpeg";
  if (flav == "public.png")
    return "image/png";

  klfDbg("unknown...") ;
  return QString();
}




