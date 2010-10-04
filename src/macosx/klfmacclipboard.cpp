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


void __klf_add_macosx_type_rules(const QString& fname, const QDomElement& e)
{
  QDomNode en;
  for (en = e.firstChild(); ! en.isNull(); en = en.nextSibling() ) {
    if ( en.isNull() || en.nodeType() != QDomNode::ElementNode )
      continue;
    QDomElement ee = en.toElement();
    if ( en.nodeName() == "translate" ) {
      QDomNodeList mimetypetags = ee.elementsByTagName("mime-type");
      if (mimetypetags.size() != 1) {
	qWarning()<<KLF_FUNC_NAME<<": in XML file "<<fname<<", macosx type translate rule:"
	  " exactly ONE <mime-type> tag must be present.";
	continue;
      }
      QDomNodeList macflavortags = ee.elementsByTagName("mac-flavor");
      if (macflavortags.size() != 1) {
	qWarning()<<KLF_FUNC_NAME<<": in XML file "<<fname<<", macosx type translate rule:"
	  " exactly ONE <mac-flavor> tag must be present.";
	continue;
      }
      QString mimetype = mimetypetags.at(0).toElement().text().trimmed();
      QString macflavortype = macflavortags.at(0).toElement().text().trimmed();
      // and register this rule
      KLFMacPasteboardMime::TranslateRule rule(mimetype, macflavortype);
      KLFMacPasteboardMime::addTranslateTypeRule(rule);
      continue;
    }
    qWarning()<<KLF_FUNC_NAME<<": in XML file "<<fname<<": bad node "<<en.nodeName()
	      <<" in <add-macosx-type-rules>";
    continue;
  }
}



// See the following URL for some mime types:
//   http://developer.apple.com/library/mac/#documentation/Miscellaneous/Reference/UTIRef/Articles/System-DeclaredUniformTypeIdentifiers.html

// static
QList<KLFMacPasteboardMime::TranslateRule> KLFMacPasteboardMime::staticTranslateTypeRules =
  QList<KLFMacPasteboardMime::TranslateRule>()
//       a few built-in translation rules
  << KLFMacPasteboardMime::TranslateRule("image/png", "public.png")
  << KLFMacPasteboardMime::TranslateRule("image/jpeg", "public.jpeg")
  << KLFMacPasteboardMime::TranslateRule("image/gif", "com.compuserve.gif")
  << KLFMacPasteboardMime::TranslateRule("image/bmp", "com.microsoft.bmp")
  << KLFMacPasteboardMime::TranslateRule("application/pdf", "com.adobe.pdf")
  << KLFMacPasteboardMime::TranslateRule("application/postscript", "com.adobe.postscript")
  << KLFMacPasteboardMime::TranslateRule("application/eps", "com.adobe.encapsulated-postscript")
  << KLFMacPasteboardMime::TranslateRule("image/eps", "com.adobe.encapsulated-postscript")
  ;

// static
void KLFMacPasteboardMime::addTranslateTypeRule(const TranslateRule& rule)
{
  klfDbg("adding rule "<<rule.mimetype<<" <-> "<<rule.macflavor) ;
  staticTranslateTypeRules.prepend(rule);
}



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
  klfDbg("mime="<<mime<<" flav="<<flav) ;

  return KLF_DEBUG_TEE( mimeFor(flav) == mime ) ;
}

QList<QByteArray> KLFMacPasteboardMime::convertFromMime(const QString& mime, QVariant data,
							QString flav)
{
  KLF_DEBUG_BLOCK(KLF_FUNC_NAME) ;
  klfDbg("mime="<<mime<<" flav="<<flav) ;

  if (!canConvert(mime, flav)) {
    klfDbg("can't convert.") ;
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
  if (data.count() > 1) {
    qWarning()<<KLF_FUNC_NAME<<": Can't handle data.size()>1, in conversion from "<<mime
	      <<" to "<<flav;
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

  // query our translation rules
  int k;
  for (k = 0; k < staticTranslateTypeRules.size(); ++k) {
    if (mime == staticTranslateTypeRules[k].mimetype)
      return KLF_DEBUG_TEE( staticTranslateTypeRules[k].macflavor );
  }

  klfDbg("unknown...") ;
  return QString();
}

QString KLFMacPasteboardMime::mimeFor(QString flav)
{
  KLF_DEBUG_BLOCK(KLF_FUNC_NAME) ;
  klfDbg("flav="<<flav) ;

  // query our translation rules
  int k;
  for (k = 0; k < staticTranslateTypeRules.size(); ++k) {
    if (flav == staticTranslateTypeRules[k].macflavor)
      return KLF_DEBUG_TEE( staticTranslateTypeRules[k].mimetype );
  }

  klfDbg("unknown...") ;
  return QString();
}


