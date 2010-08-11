/***************************************************************************
 *   file klfmime.h
 *   This file is part of the KLatexFormula Project.
 *   Copyright (C) 2009 by Philippe Faist
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

#ifndef KLFMIME_H
#define KLFMIME_H

#include <QString>
#include <QList>
#include <QStringList>
#include <QByteArray>
#include <QMap>
#include <QImage>
#include <QImageWriter>
#include <QTemporaryFile>

#include <klfdefs.h>
#include <klfbackend.h>



class KLF_EXPORT KLFMimeExporter : public QObject
{
  Q_OBJECT
public:
  KLFMimeExporter(QObject *parent) : QObject(parent) { }
  virtual ~KLFMimeExporter() { }

  virtual QStringList keys() const = 0;
  virtual QByteArray data(const QString& key, const KLFBackend::klfOutput& klfoutput) = 0;

  /** \brief Shortcut function (do not reimplement in subclasses)
   *
   * \returns true if key is in \ref keys() list. */
  virtual bool supportsKey(const QString& key) const;

  /** Looks up an exporter supporting type \c key. The first exporter in the list
   * is returned. If no exporter is found, NULL is returned. */
  static KLFMimeExporter * mimeExporterLookup(const QString& key);
  
  static QList<KLFMimeExporter *> mimeExporterList();
  /** Adds the instance \c exporter to the internal list of exporters. Ownership of the
   * object is NOT transferred.
   *
   * If \c overrides is TRUE, then prepends the exporter to the list (will be looked
   * up first), otherwise appends it to list. */
  static void addMimeExporter(KLFMimeExporter *exporter, bool overrides = true);

private:
  static void initMimeExporterList();
  static QList<KLFMimeExporter*> p_mimeExporterList;
};


class KLF_EXPORT KLFMimeExportProfile
{
public:
  KLFMimeExportProfile(const QString& pname, const QString& desc, const QStringList& mtypes,
		       const QStringList& wintypes);
  KLFMimeExportProfile(const KLFMimeExportProfile& copy);

  QString profileName() const { return p_profileName; }
  QString description() const { return p_description; }
  QStringList mimeTypes() const { return p_mimeTypes; }

  /** Windows Clipboard Formats to show for each mime type (respectively).
   *
   * An empty string should cause the mime type name to be used. */
  QStringList respectiveWinTypes() const { return p_respectiveWinTypes; }

  /** Returns the k-th element in respectiveWinTypes if non-empty, otherwise
   * the k-th element in mimeTypes. */
  QString respectiveWinType(int k) const;

  static QList<KLFMimeExportProfile> exportProfileList();
  static void addExportProfile(const KLFMimeExportProfile& exportProfile);

  static KLFMimeExportProfile findExportProfile(const QString& pname);

private:

  QString p_profileName;
  QString p_description;
  QStringList p_mimeTypes;
  QStringList p_respectiveWinTypes;

  static void ensureLoadedExportProfileList();
  static void loadFromXMLFile(const QString& fname);
  static QList<KLFMimeExportProfile> p_exportProfileList;
};

/** KLFMimeExporter implementation to export all known built-in image formats, including
 * - all Qt-supported image formats
 * - all KLFBackend-generated formats, namely EPS (image/eps, application/eps, application/postscript),
 *   PDF (application/pdf)
 * - additionally, OpenOffice.org draw object
 *   (\c application/x-openoffice-drawing;windows_formatname="Drawing Format")
 */
class KLF_EXPORT KLFMimeExporterImage : public KLFMimeExporter
{
  Q_OBJECT
public:
  KLFMimeExporterImage(QObject *parent) : KLFMimeExporter(parent) { }
  virtual ~KLFMimeExporterImage() { }

  virtual QStringList keys() const;
  virtual QByteArray data(const QString& key, const KLFBackend::klfOutput& klfoutput);

private:
  static QMap<QString,QByteArray> imageFormats;
};

/** KLFMimeExporter implementation for exporting \c "text/x-moz-url" and \c "text/uri-list"
 * to a temporary PNG file
 */
class KLF_EXPORT KLFMimeExporterUrilist : public KLFMimeExporter
{
  Q_OBJECT
public:
  KLFMimeExporterUrilist(QObject *parent) : KLFMimeExporter(parent) { }
  virtual ~KLFMimeExporterUrilist() { }

  virtual QStringList keys() const;
  virtual QByteArray data(const QString& key, const KLFBackend::klfOutput& klfoutput);

private:
  static QMap<QByteArray,QString> tempFilesForImageMD5;
};


/** Wrapper class to export all formats registered in KLFAbstractLibEntryMimeExporter,
 * including all additional added encoders (eg. from plugins)
 */
class KLF_EXPORT KLFMimeExporterLibFmts : public KLFMimeExporter
{
  Q_OBJECT
public:
  KLFMimeExporterLibFmts(QObject *parent) : KLFMimeExporter(parent) { }
  virtual ~KLFMimeExporterLibFmts() { }

  virtual QStringList keys() const;
  virtual QByteArray data(const QString& key, const KLFBackend::klfOutput& klfoutput);
};





#endif
