/***************************************************************************
 *   file klfmime.h
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

#ifndef KLFMIME_H
#define KLFMIME_H

#include <QString>
#include <QList>
#include <QStringList>
#include <QByteArray>
#include <QMap>
#include <QImage>
#include <QImageWriter>
#include <QMimeData>
#include <QTemporaryFile>

#include <klfdefs.h>
#include <klfbackend.h>


/** \brief A helper class to export KLF output to other applications
 *
 * This helper class can be subclassed to implement exporting klatexformula output in a given
 * data format.
 *
 * Under Windows, mime types are translated to a windows format name returned by \c windowsFormatName().
 * Under Mac OS X, mime types are translated using KLFMacPasteboardMime, a QMacPasteboardMime subclass.
 *
 * \note The mime-type \c "application/x-qt-image" has a special meaning. This key may be returned by
 *   keys() to indicate that a generic image data is to be exported in the native Qt-supported formats.
 *   When the data for that mime-type is requested, the image data should be returned in raw PNG, JPEG
 *   or BMP as a QByteArray, and then when this data is added to a QMimeData, QMimeData::setImageData()
 *   with the returned data will be used.
 */
class KLF_EXPORT KLFMimeExporter
{
public:
  KLFMimeExporter() { }
  virtual ~KLFMimeExporter() { }

  virtual QString exporterName() const = 0;

  virtual QStringList keys() const = 0;
  virtual QByteArray data(const QString& key, const KLFBackend::klfOutput& klfoutput) = 0;

  /* \brief the MS Windows Format Name for the given mime-type \c key.
   *
   * The default implementation just returns \c key. You should reimplement
   * this function to return a useful standard name.
   */
  virtual QString windowsFormatName(const QString& key) const { return key; }

  /** \brief Shortcut function (do not reimplement in subclasses)
   *
   * \returns true if key is in \ref keys() list. */
  virtual bool supportsKey(const QString& key) const;

  /** Looks up an exporter supporting type \c key. The first exporter in the list
   * is returned. If no exporter is found, NULL is returned. */
  static KLFMimeExporter * mimeExporterLookup(const QString& key);

  /** Looks up exporter \c exporter and returns the exporter, or NULL if it was not found.
   *
   * If \c key is non-empty, a check to ensure that the exporter supports the given \c key is performed
   * with \ref supportsKey(). If the exporter does not support the given \c key, NULL is returned instead. */
  static KLFMimeExporter * mimeExporterLookupByName(const QString& exporter, const QString& key = QString());
  
  static QList<KLFMimeExporter *> mimeExporterList();
  /** Adds the instance \c exporter to the internal list of exporters.
   *
   * If \c overrides is TRUE, then prepends the exporter to the list (will be looked
   * up first), otherwise appends it to list. */
  static void registerMimeExporter(KLFMimeExporter *exporter, bool overrides = true);
  static void unregisterMimeExporter(KLFMimeExporter *exporter);

private:
  static void initMimeExporterList();
  static QList<KLFMimeExporter*> p_mimeExporterList;
};

/** \brief An export profile grouping several mime types
 */
class KLF_EXPORT KLFMimeExportProfile
{
public:
  struct ExportType {
    ExportType(const QString& mime = QString(), const QString& win = QString(), const QString& exp = QString())
      : mimetype(mime), wintype(win), exporter(exp)  { }
    QString mimetype;
    QString wintype; //!< May be left empty, to indicate to use KLFMimeExporter::windowsFormatName()
    QString exporter; //!< May be left empty
  };

  KLFMimeExportProfile(const QString& pname, const QString& desc, const QList<ExportType>& exporttypes);
  KLFMimeExportProfile(const KLFMimeExportProfile& copy);


  QString profileName() const { return p_profileName; }
  QString description() const { return p_description; }

  /** List of formats to export when using this export profile. */
  inline QList<ExportType> exportTypes() const { return p_exportTypes; }
  /** Number of export types. Equivalent to <tt>exportTypes().size()</tt> */
  inline int exportTypesCount() const { return p_exportTypes.size(); }
  /** Returns export type at position \c n. \c n MUST be in the valid range \c 0 ... exportTypesCount(). */
  inline ExportType exportType(int n) const { return p_exportTypes[n]; }

  /** Returns the KLFMimeExporter object that is responsible for exporting into the format
   * at index \c n in the exportTypes() list.
   *
   * If \c warnNotFound is TRUE, then a warning is emitted if the exporter was not found. */
  KLFMimeExporter * exporterLookupFor(int n, bool warnNotFound = true) const;

  /** A list of mime types to export when using this profile.
   *
   * This is equivalent to building a list of all ExportType::mimetype members of the return
   * value of exportTypes(). */
  QStringList mimeTypes() const;

  /** Returns the index of the given mimeType in the export list, or \c -1 if not found. */
  int indexOfMimeType(const QString& mimeType) const;

  /** Windows Clipboard Formats to show for each mime type (respectively).
   *
   * This is equivalent to building a list of all return values of respectiveWinType(int)
   * for all integers ranging from \c 0 to exportTypesCount(). */
  QStringList respectiveWinTypes() const;

  /** Returns the k-th element in respectiveWinTypes. If that element is empty,
   * queries the correct mime-type exporter for a windows format name with
   * KLFMimeExporter::windowsFormatName().
   */
  QString respectiveWinType(int k) const;


  /** Returns a list of mime types, for which we garantee that (at least at the time
   * of calling this function), KLFMimeExporter::mimeExporterLookup(mimetype) will
   * not return NULL.
   */
  QStringList availableExporterMimeTypes() const;


  static QList<KLFMimeExportProfile> exportProfileList();
  static void addExportProfile(const KLFMimeExportProfile& exportProfile);

  static KLFMimeExportProfile findExportProfile(const QString& pname);

private:

  QString p_profileName;
  QString p_description;
  QList<ExportType> p_exportTypes;

  static void ensureLoadedExportProfileList();
  static void loadFromXMLFile(const QString& fname);
  static QList<KLFMimeExportProfile> p_exportProfileList;
};



/** A QMimeData subclass for Copy and Drag operations in KLFMainWin, that supports delayed
 * data processing, ie. that actually creates the requested data only on drop or paste, and
 * not when the operation is initiated.
 *
 * This function can be used as a regular QMimeData object to copy or drag any
 * KLFBackend::klfOutput data, with a given export profile.
 */
class KLF_EXPORT KLFMimeData : public QMimeData
{
  Q_OBJECT
public:
  KLFMimeData(const QString& exportProfileName, const KLFBackend::klfOutput& output);
  virtual ~KLFMimeData();

  QStringList formats() const;

protected:
  QVariant retrieveData(const QString &mimetype, QVariant::Type type) const;

private:
  KLFMimeExportProfile pExportProfile;
  KLFBackend::klfOutput pOutput;

  void set_possible_qt_image_data();

  mutable QStringList pQtOwnedFormats;
};





#endif
