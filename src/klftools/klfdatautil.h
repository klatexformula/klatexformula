/***************************************************************************
 *   file klfdatautil.h
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

#ifndef KLFDATAUTIL_H
#define KLFDATAUTIL_H

#include <QString>
#include <QStringList>
#include <QDomElement>

#include <klffactory.h>

/** Escapes every character in \c data that is not in the range 32-126 (included) as
 * \\xHH whith HH the hex code of the character. Backslashes are replaced by double-backslashes.
 */
KLF_EXPORT QByteArray klfDataToEscaped(const QByteArray& data);

/** Performs the exact inverse of \ref klfDataToEscaped().
 *
 * Also understands standard short C escape sequences such as \c '\n', \c '\0', and \c '\t'
 */
KLF_EXPORT QByteArray klfEscapedToData(const QByteArray& escaped);


/** Saves the variant \c value into a string, stored in Local 8-bit encoding text in QByteArray.
 * The saved string is both human and machine readable, ie. the exact string can be recast again
 * to a variant with \ref klfLoadVariantFromText().
 *
 * This function is aware of various QVariant formats, however maybe not all of them. The unknown
 * formats are stored machine-readable only, by sending the variant in a datastream, and protecting
 * the special characters from encoding artifacts (ascii chars only, proper escape sequences).
 *
 * If \c saveListAndMapsAsXML is FALSE (the default), then variant-lists and -maps are saved in
 * a format like \c "[element-1,element-2,...]" or \c "{key1=value1,key2=value2,....}", assuming
 * that all elements are of the same QVariant type. If \c saveListAndMapsAsXML is TRUE, then
 * variant lists and maps are saved with klfSaveVariantListToXML() and klfSaveVariantMapToXML(),
 * which enables you to save arbitrary combination of types.
 *
 * \note Note however that the saved string does NOT save the type. The data type must be known
 *   when loading the value. See \ref klfLoadVariantFromText().
 *
 * If \c savedType is not NULL, then the exact type of the variant that was saved is reported, including
 * a type specifier if the type is a registered KLFSpecifyableType subclass.
 * */
KLF_EXPORT QByteArray klfSaveVariantToText(const QVariant& value, bool saveListAndMapsAsXML = false,
					   QByteArray * savedType = NULL);

/** Loads the value stored in \c string into a variant of data type \c dataTypeName. The string
 * is parsed and the returned variant will by of the given type name, or invalid if the string
 * could not be parsed.
 *
 * \note The data type must be known when loading the value. It can in general not be guessed by
 * looking at \c string.
 *
 * Example use: to save/store settings values in QSettings in a human-read/writable format.
 *
 * If \c dataTypeName is a variant list or map, then please specify the type of the values in
 * the list or map (this function assumes all objects in list or map have the same type). As
 * a special case, you can pass \c "XML" to load the list or map data with klfLoadVariantListFromXML()
 * or klfLoadVariantMapFromXML(), which enables you to save arbitrary combination of types.
 *
 * See also \ref klfSaveVariantToText().
 *
 * If dataTypeName contains a '/' (ie is of the form "KLFEnumType/a:b:c", then the string after the
 * slash is treated as a specification for the given KLFSpecifyableType.
 */
KLF_EXPORT QVariant klfLoadVariantFromText(const QByteArray& string, const char * dataTypeName,
					   const char *listOrMapTypeName = NULL);


//! Lossless save of full map to XML with type information
KLF_EXPORT QDomElement klfSaveVariantMapToXML(const QVariantMap& vmap, QDomElement xmlNode);
//! Load a map saved with klfSaveVariantMapToXML()
KLF_EXPORT QVariantMap klfLoadVariantMapFromXML(const QDomElement& xmlNode);

//! Lossless save of full list to XML with type information
KLF_EXPORT QDomElement klfSaveVariantListToXML(const QVariantList& vlist, QDomElement xmlNode);
//! Load a list saved with klfSaveVariantListToXML()
KLF_EXPORT QVariantList klfLoadVariantListFromXML(const QDomElement& xmlNode);






class KLFAbstractPropertizedObject;

/** \brief Inherit this class to implement a custom saver for KLFAbstractPropertizedObject<i></i>s
 *
 * \note All formats must be explicitly recognizable; for binary formats you must add a "magic" header.
 *   This is important so that klfLoad() does not need to know the format in advance.
 */
class KLFAbstractPropertizedObjectSaver : public KLFFactoryBase
{
public:
  KLFAbstractPropertizedObjectSaver();
  virtual ~KLFAbstractPropertizedObjectSaver();

  virtual QStringList supportedTypes() const = 0;

  virtual QString recognizeDataFormat(const QByteArray& data) const = 0;

  virtual QByteArray save(const KLFAbstractPropertizedObject * obj, const QString& format) = 0;
  virtual bool load(const QByteArray& data, KLFAbstractPropertizedObject * obj, const QString& format) = 0;

  static KLFAbstractPropertizedObjectSaver * findRecognizedFormat(const QByteArray& data, QString * format = NULL);
  static KLFAbstractPropertizedObjectSaver * findSaverFor(const QString& format);
private:
  static KLFFactoryManager pFactoryManager;
};



KLF_EXPORT QByteArray klfSave(const KLFAbstractPropertizedObject * obj, const QString& = "XML");

/**
 * If \c format is an empty string, then the format will be guessed from data.
 */
KLF_EXPORT bool klfLoad(const QByteArray& data, KLFAbstractPropertizedObject * obj, const QString& format = QString());





#endif
