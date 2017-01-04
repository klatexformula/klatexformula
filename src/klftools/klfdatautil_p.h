/***************************************************************************
 *   file klfdatautil_p.h
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

/** \file
 * This header contains (in principle private) auxiliary classes for
 * library routines defined in klfdatautil.cpp */

#ifndef KLFDATAUTIL_P_H
#define KLFDATAUTIL_P_H

#include <QByteArray>
#include <QBuffer>

#include <string.h>
#include <klfutil.h>

static QByteArray compressed_xml_magic = QByteArray("qCompressedXML\0",
						    strlen("qCompressedXML")+1); // _WITH_ '\0'
static QByteArray binary_magic = QByteArray("BinaryVariantMap");
static QByteArray textvariantmap_header = QByteArray("TextVariantMap:");

class KLFBaseFormatsPropertizedObjectSaver : public KLFAbstractPropertizedObjectSaver
{
public:
  KLFBaseFormatsPropertizedObjectSaver()
  {
  }

  QStringList supportedTypes() const
  {
    return QStringList() << QLatin1String("XML") << QLatin1String("CompressedXML")
			 << QLatin1String("Binary") << QLatin1String("TextVariantMap");
  }
  QString recognizeDataFormat(const QByteArray& data) const
  {
    KLF_DEBUG_BLOCK(KLF_FUNC_NAME) ;
    klfDbg("data="<<data) ;
    { // try to recognize XML
      int j, k;
      for (k = 0; k < data.size() && QChar(data[k]).isSpace(); ++k)
	;
      QStringList testheaders = klfMkList<QString>(QLatin1String("<?xml"), QLatin1String("<!DOCTYPE"));
      for (j = 0; j < testheaders.size(); ++j) {
	if (testheaders[j].compare(QString::fromLatin1(data.mid(k, testheaders[j].size())),
				   Qt::CaseInsensitive) == 0) { // found header
	  return KLF_DEBUG_TEE( QLatin1String("XML") );
	}
      }
    }
    { // try to recognize Compressed XML
      if (data.startsWith(compressed_xml_magic)) {
	return KLF_DEBUG_TEE( QLatin1String("CompressedXML") );
      }
    }
    { // try to recognize Binary
      QDataStream stream(data);
      stream.setVersion(QDataStream::Qt_4_4);
      QByteArray header;
      stream >> header;
      if (header == binary_magic) {
	return KLF_DEBUG_TEE( QLatin1String("Binary") );
      }
    }
    { // try to recognize TextVariantMap
      if (data.startsWith(textvariantmap_header)) {
	return KLF_DEBUG_TEE( QLatin1String("TextVariantMap") );
      }
    }
    // failed to recognize type
    return QString();
  }
  QByteArray save(const KLFAbstractPropertizedObject * obj, const QString& format)
  {
    KLF_DEBUG_BLOCK(KLF_FUNC_NAME) ;
    QVariantMap propdata = obj->allProperties();
    klfDbg("saving object of kind " << obj->objectKind() << " with properties = "
           << propdata << " using format " << format );
    if (format == QLatin1String("XML")) {
      QString s = obj->objectKind();
      QDomDocument xmldoc(s);
      QDomElement root = xmldoc.createElement(s);
      xmldoc.appendChild(root);
      (void)klfSaveVariantMapToXML(propdata, root);
      return xmldoc.toByteArray(-1);
    } else if (format == QLatin1String("CompressedXML")) {
      QByteArray data = compressed_xml_magic;
      data.append(qCompress(save(obj, QLatin1String("XML"))));
      return data;
    } else if (format == QLatin1String("Binary")) {
      QByteArray b;
      {
        QBuffer buf(&b);
        buf.open(QIODevice::WriteOnly);
        QDataStream stream(&buf);
        stream.setVersion(QDataStream::Qt_4_4);
        stream << binary_magic << propdata;
      }
      klfDbg("binary data is " << b) ;
      return b;
    } else if (format == QLatin1String("TextVariantMap")) {
      QByteArray data;
      // see if all values are of the same type, and is a simple type (i.e., not map or list)
      const char * typ = NULL;
      bool allok = true;
      for (QVariantMap::const_iterator it = propdata.begin(); it != propdata.end(); ++it) {
	if (typ == NULL) {
	  typ = it.value().typeName();
	}
	if (!strcmp(typ, "QVariantMap") || !strcmp(typ, "QVariantList")) {
	  allok = false;
	  break; // don't save complex types as inline-text...
	}
	if (strcmp(typ, it.value().typeName()) != 0) {
	  // different types
	  allok = false;
	  break;
	}
      }
      data.append(textvariantmap_header);
      data.append(obj->objectKind().toLatin1());
      if (typ == NULL) {
	// empty variant map
	data.append("{}");
	return data;
      }
      if (allok) {
	// don't need to save as XML, we can save as the given type
	data.append("["); data.append(typ); data.append("]");
	data.append(klfSaveVariantToText(propdata, false));
	return data;
      }
      // fallback to saving as XML
      data.append("[+XML]");
      data.append(klfSaveVariantToText(propdata, true));
      return data;
    } else {
      qWarning()<<KLF_FUNC_NAME<<": Unknown format `"<<format<<"'";
      return QByteArray();
    }
    qWarning()<<KLF_FUNC_NAME<<": Should never reach this point in function!";
    return QByteArray();
  }
  bool load(const QByteArray& data, KLFAbstractPropertizedObject * obj, const QString& format)
  {
    KLF_DEBUG_BLOCK(KLF_FUNC_NAME) ;
    klfDbg("Loading object properties from data " << data << " in format " << format) ;
    if (format == QLatin1String("XML")) {
      QString s = obj->objectKind();
      QDomDocument xmldoc(s);
      bool result = xmldoc.setContent(data);
      KLF_ASSERT_CONDITION(result, "Failed to read wrapper XML in load()", return false; ) ;
 
      QDomElement el = xmldoc.documentElement();
      KLF_ASSERT_CONDITION( el.nodeName() == s,
			    "Wrong XML root node in XML wrapper for load(): "<<el.nodeName() , return false; ) ;
      QVariantMap data = klfLoadVariantMapFromXML(el);
      // now set all the properties
      return obj->setAllProperties(data);
    } else if (format == QLatin1String("CompressedXML")) {
      KLF_ASSERT_CONDITION(data.startsWith(compressed_xml_magic),
			   "Data is not in compressed XML format! Bad header!", return false;) ;
      int magiclen = compressed_xml_magic.size();
      return load(qUncompress((const uchar*)data.constData() + magiclen, data.size() - magiclen),
		  obj, QLatin1String("XML"));
    } else if (format == QLatin1String("Binary")) {
      QDataStream stream(data);
      stream.setVersion(QDataStream::Qt_4_4);
      QByteArray header;
      stream >> header;
      KLF_ASSERT_CONDITION(header == binary_magic, "Data is not 'Binary' format! Bad header!", return false; ) ;
      QVariantMap vmap;
      stream >> vmap;
      klfDbg("read variant map: " << vmap) ;
      // now set all the properties
      return obj->setAllProperties(vmap);
    } else if (format == QLatin1String("TextVariantMap")) {
      klfDbg("Reading a TextVariantMap encoded variant map.") ;
      QVariantMap props;
      // determine which method we used (empty, inline, xml)
      int k = 0;
      KLF_ASSERT_CONDITION(data.startsWith(textvariantmap_header),
			   "Data is not of format TextVariantMap! Bad Header!", return false; );
      k = textvariantmap_header.size();
      // read object kind
      int okindidx = k+1;
      while (okindidx < data.size() && data[okindidx] != '[' && data[okindidx] != '{') {
	++okindidx;
      }
      // object kind
      QByteArray okind = data.mid(k, okindidx-k);
      KLF_ASSERT_CONDITION(okind == obj->objectKind().toLatin1(),
			   "Trying to load wrong object kind: "<<okind, return false; ) ;
      k = okindidx;
      if (k+1 < data.size() && data[k] == '{' && data[k+1] == '}') {
	// empty property map
        klfDbg("Empty variant map.") ;
	props = QVariantMap();
      } else {
	KLF_ASSERT_CONDITION(k < data.size() && (data[k] == '['),
			     "Malformed data: expected '[' with type name.", return false; ) ;
	int k2 = data.indexOf(']', k+1);
	KLF_ASSERT_CONDITION(k2 > k+1, "Cannot find matching ']' for type name !", return false; ) ;
	QByteArray typ = data.mid(k+1, k2-(k+1));
	if (typ == "+XML") {
	  // read from XML data
	  props = klfLoadVariantFromText(data.mid(k2+1), "QVariantMap", "XML").toMap();
	} else {
	  props = klfLoadVariantFromText(data.mid(k2+1), "QVariantMap", typ.constData()).toMap();
	}
        klfDbg("variant map: okind="<<okind<<", typ="<<typ<<", props="<<props) ;
      }
      // now set these properties
      return obj->setAllProperties(props);
    } else {
      qWarning()<<KLF_FUNC_NAME<<": Unknown format `"<<format<<"'";
      return false;
    }
  }
};



#endif
