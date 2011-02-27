/***************************************************************************
 *   file klfmacclipboard.h
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


#ifndef KLFMACCLIPBOARD_H
#define KLFMACCLIPBOARD_H

#include <QMacPasteboardMime>

#include <QDomElement>


class KLFMacPasteboardMime : public QMacPasteboardMime
{
public:
  KLFMacPasteboardMime();
  virtual ~KLFMacPasteboardMime();

  QString convertorName() { return QLatin1String("KLFMacPasteboardMime"); }

  bool canConvert(const QString& mime, QString flav);
  QList<QByteArray> convertFromMime(const QString& mime, QVariant data, QString flav);
  QVariant convertToMime(const QString& mime, QList<QByteArray> data, QString flav);
  QString flavorFor(const QString& mime);
  QString mimeFor(QString flav);


  struct TranslateRule
  {
    TranslateRule(QString mime, QString flav) : mimetype(mime), macflavor(flav) { }
    QString mimetype;
    QString macflavor;
  };


  static void addTranslateTypeRule(const TranslateRule& rule);

private:
  static QList<TranslateRule> staticTranslateTypeRules;

};



void __klf_init_the_macpasteboardmime();


void __klf_add_macosx_type_rules(const QString& xmlfname, const QDomElement& element);


#endif
