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
  if (__klf_the_macpasteboardmime == NULL) {
    __klf_the_macpasteboardmime = new KLFMacPasteboardMime();
  }
}


// additionally : see
// http://developer.apple.com/library/mac/#documentation/Miscellaneous/Reference/UTIRef/Articles/System-DeclaredUniformTypeIdentifiers.html
//com.adobe.pdf
//com.adobe.postscript
//com.adobe.encapsulated-postscript

#define FLAV_PS  "com.adobe.postscript"
#define FLAV_EPS "com.adobe.encapsulated-postscript"
#define FLAV_PDF "com.adobe.pdf"



KLFMacPasteboardMime::KLFMacPasteboardMime()
  : QMacPasteboardMime(MIME_ALL)
{
  klfDbg("constructor.") ;
#if QT_VERSION >= 0x040600
  qRegisterDraggedTypes(QStringList()<<FLAV_PS<<FLAV_EPS<<FLAV_PDF);
#endif
}

KLFMacPasteboardMime::~KLFMacPasteboardMime()
{
  klfDbg("destructor.") ;
}


bool KLFMacPasteboardMime::canConvert(const QString& mime, QString flav)
{
  KLF_DEBUG_BLOCK(KLF_FUNC_NAME) ;
  klfDbg("mime="<<mime<<" flav="<<flav) ;

  return KLF_DEBUG_TEE( mimeFor(flav) == mime ) ;

  klfDbg("can't convert.") ;
  return false;
}

QList<QByteArray> KLFMacPasteboardMime::convertFromMime(const QString& mime, QVariant data, QString flav)
{
  KLF_DEBUG_BLOCK(KLF_FUNC_NAME) ;
  klfDbg("mime="<<mime<<" flav="<<flav) ;

  if (canConvert(mime, flav)) {
    QByteArray d = data.toByteArray();
    klfDbg("returning data "<<data<<" as bytearray: length="<<d.size()<<" for conversion from "
	   <<mime<<" to "<<flav) ;
    /** \bug .... this message fprintf() is for debug only... */
    fprintf(stderr, "DEBUG: mac/convertFromMime: mime=%s, flav=%s, data.length=%d\n",
	    qPrintable(mime), qPrintable(flav), d.size());

    return QList<QByteArray>() << d;
  }

  klfDbg("can't convert.") ;
  return QList<QByteArray>();
}

QVariant KLFMacPasteboardMime::convertToMime(const QString& mime, QList<QByteArray> data, QString flav)
{
  KLF_DEBUG_BLOCK(KLF_FUNC_NAME) ;
  klfDbg("mime="<<mime<<" flav="<<flav) ;
  if (data.count() > 1) {
    qWarning()<<KLF_FUNC_NAME<<": Can't handle data.size()>1, in conversion from "<<mime<<" to "<<flav;
  }
  if (data.count() <= 0) {
    klfDbg("no data.");
    return QByteArray();
  }

  klfDbg("data size="<<data[0].length()) ;
  return data[0];
}

QString KLFMacPasteboardMime::flavorFor(const QString& mime)
{
  KLF_DEBUG_BLOCK(KLF_FUNC_NAME) ;
  klfDbg("mime="<<mime) ;
  if (mime == "image/jpeg")
    return KLF_DEBUG_TEE("public.jpeg");
  if (mime == "image/png")
    return KLF_DEBUG_TEE("public.png");
  if (mime == "application/postscript")
    return KLF_DEBUG_TEE(FLAV_PS);
  if (mime == "application/eps")
    return KLF_DEBUG_TEE(FLAV_EPS);
  if (mime == "image/eps")
    return KLF_DEBUG_TEE(FLAV_EPS);
  if (mime == "application/pdf")
    return KLF_DEBUG_TEE(FLAV_PDF);

  klfDbg("unknown...") ;
  return QString();
}

QString KLFMacPasteboardMime::mimeFor(QString flav)
{
  KLF_DEBUG_BLOCK(KLF_FUNC_NAME) ;
  klfDbg("flav="<<flav) ;
  if (flav == "public.jpeg")
    return KLF_DEBUG_TEE("image/jpeg");
  if (flav == "public.png")
    return KLF_DEBUG_TEE("image/png");
  if (flav == FLAV_PS)
    return KLF_DEBUG_TEE("application/postscript");
  if (flav == FLAV_EPS)
    return KLF_DEBUG_TEE("application/eps");
  if (flav == FLAV_PDF)
    return KLF_DEBUG_TEE("application/pdf");

  klfDbg("unknown...") ;
  return QString();
}




