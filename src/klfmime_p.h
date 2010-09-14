/***************************************************************************
 *   file klfmime_p.h
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

#ifndef KLFMIME_P_H
#define KLFMIME_P_H

#include <QObject>

#include "klfmime.h"

/** \file
 * This header contains (in principle private) auxiliary classes for
 * library routines defined in klfmime.cpp */



/** KLFMimeExporter implementation to export all known built-in image formats, including
 * - all Qt-supported image formats
 * - all KLFBackend-generated formats, namely EPS (image/eps, application/eps, application/postscript),
 *   PDF (application/pdf)
 * - additionally, OpenOffice.org draw object
 *   (\c application/x-openoffice-drawing;windows_formatname="Drawing Format")
 */
class KLF_EXPORT KLFMimeExporterImage : public QObject, public KLFMimeExporter
{
  Q_OBJECT
public:
  KLFMimeExporterImage(QObject *parent) : QObject(parent) { }
  virtual ~KLFMimeExporterImage() { }

  virtual QString exporterName() const { return QString::fromLatin1("KLFMimeExporterImage"); }

  virtual QStringList keys() const;
  virtual QByteArray data(const QString& key, const KLFBackend::klfOutput& klfoutput);

  virtual QString windowsFormatName(const QString& key) const;

private:
  static QMap<QString,QByteArray> imageFormats;
};

/** KLFMimeExporter implementation for exporting \c "text/x-moz-url" and \c "text/uri-list"
 * to a temporary PNG file
 */
class KLF_EXPORT KLFMimeExporterUrilist : public QObject, public KLFMimeExporter
{
  Q_OBJECT
public:
  KLFMimeExporterUrilist(QObject *parent) : QObject(parent) { }
  virtual ~KLFMimeExporterUrilist() { }

  virtual QString exporterName() const { return QString::fromLatin1("KLFMimeExporterUrilist"); }

  virtual QStringList keys() const;
  virtual QByteArray data(const QString& key, const KLFBackend::klfOutput& klfoutput);

  virtual QString windowsFormatName(const QString& key) const;

private:
  static QMap<qint64,QString> tempFilesForImageCacheKey;
};


/** Wrapper class to export all formats registered in KLFAbstractLibEntryMimeExporter,
 * including all additional added encoders (eg. from plugins)
 */
class KLF_EXPORT KLFMimeExporterLibFmts : public QObject, public KLFMimeExporter
{
  Q_OBJECT
public:
  KLFMimeExporterLibFmts(QObject *parent) : QObject(parent) { }
  virtual ~KLFMimeExporterLibFmts() { }

  virtual QString exporterName() const { return QString::fromLatin1("KLFMimeExporterLibFmts"); }

  virtual QStringList keys() const;
  virtual QByteArray data(const QString& key, const KLFBackend::klfOutput& klfoutput);

};




/** Alien equations ;) */
class KLFMimeExporterGlowImage : public QObject, public KLFMimeExporter
{
  Q_OBJECT
public:
  KLFMimeExporterGlowImage(QObject *parent) : QObject(parent) { }

  virtual QString exporterName() const { return QString::fromLatin1("KLFMimeExporterGlowImage"); }

  virtual QStringList keys() const;
  virtual QByteArray data(const QString& key, const KLFBackend::klfOutput& klfoutput);

};


#endif
