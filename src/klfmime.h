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

  virtual QStringList keys(const KLFBackend::klfOutput * output) const = 0;
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
  virtual bool supportsKey(const QString& key, const KLFBackend::klfOutput * output) const;



  /** \brief Convenience function to look up an exporter and export the given data.
   *
   * If \c exporter is a non-empty string, that exporter is looked up, making sure it can export
   * the given type \c key, and its \ref data() function is called to export the given \c klfoutput.
   * In case of failure an empty QByteArray is returned.
   *
   * If \c exporter is an empty string, then the data given by the first exporter which supports type \c key
   * and which successfully exported the data is returned. If no exporter was found, or all exporters
   * that support type \c key failed to export the data, then an empty QByteArray is returned.
   */
  static QByteArray exportData(const QString& key, const KLFBackend::klfOutput& klfoutput,
			       const QString& exporter = QString());

  /** \brief Looks up an exporter supporting type key.
   *
   * The first exporter in the list is returned. If no exporter is found, NULL is returned.
   *
   * \warning the returned exporter may fail at exporting the requested type (for some reason), whereas
   *   there may be another exporter with the providing the same key, which, for some other reason will
   *   succeed. For this purpose it may be better to try all the exporters supporting a given key, see
   *   also \ref mimeExporterFullLookup(). */
  static KLFMimeExporter * mimeExporterLookup(const QString& key, const KLFBackend::klfOutput * output = NULL);

  /** \brief Looks up all exporters supporting the given type key.
   *
   * An empty list is returned if no exporter supports the given type \c key.
   *
   * See also \ref mimeExporterLookup().
   */
  static QList<KLFMimeExporter*> mimeExporterFullLookup(const QString& key,
							const KLFBackend::klfOutput * output = NULL);

  /** Looks up exporter \c exporter and returns the exporter, or NULL if it was not found.
   *
   * If \c key is non-empty, a check to ensure that the exporter supports the given \c key is performed
   * with \ref supportsKey(). If the exporter does not support the given \c key, NULL is returned instead. */
  static KLFMimeExporter * mimeExporterLookupByName(const QString& exporter, const QString& key = QString(),
						    const KLFBackend::klfOutput * output = NULL);

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

  KLFMimeExportProfile(const QString& pname, const QString& desc, const QList<ExportType>& exporttypes,
		       const QString& insubmenu = QString());
  KLFMimeExportProfile(const KLFMimeExportProfile& copy);


  QString profileName() const { return p_profileName; }
  QString description() const { return p_description; }

  QString inSubMenu() const { return p_inSubMenu; }

  /** List of formats to export when using this export profile. */
  inline QList<ExportType> exportTypes() const { return p_exportTypes; }
  /** Number of export types. Equivalent to <tt>exportTypes().size()</tt> */
  inline int exportTypesCount() const { return p_exportTypes.size(); }
  /** Returns export type at position \c n. \c n MUST be in the valid range \c 0 ... exportTypesCount(). */
  inline ExportType exportType(int n) const { return p_exportTypes[n]; }

  /** \brief Convience function that exports the data according to the type at the given index.
   *
   * Performs a lookup for all exporters that support the given type with \ref exporterFullLookupFor(), and
   * then tries them one by one until an exporter successfully returns exported data. We then return that
   * data.
   *
   * In case of failure, an empty QByteArray is returned.
   */
  QByteArray exportData(int n, const KLFBackend::klfOutput& output) const;

  /** Returns the KLFMimeExporter object that is responsible for exporting into the format
   * at index \c n in the exportTypes() list.
   *
   * If \c warnNotFound is TRUE, then a warning is emitted if the exporter was not found.
   *
   * See the warning in \ref KLFMimeExporert::mimeExporterLookup() and consider using the
   * function \ref exporterFullLookupFor() instead.
   */
  KLFMimeExporter * exporterLookupFor(int n, const KLFBackend::klfOutput * output = NULL,
				      bool warnNotFound = true) const;

  /** Returns the KLFMimeExporter object(s) that are responsible for exporting into the format
   * at index \c n in the exportTypes() list. If several exporters provide the same type,
   * then they are all returned provided a specific exporter has been requested in the
   * ExportType.
   *
   * If \c warnNotFound is TRUE, then a warning is emitted if the exporter was not found.
   *
   * See also \ref exporterLookupFor().
   */
  QList<KLFMimeExporter*> exporterFullLookupFor(int n, const KLFBackend::klfOutput * output = NULL,
						bool warnNotFound = true) const;

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
  QStringList availableExporterMimeTypes(const KLFBackend::klfOutput * output = NULL) const;


  static QList<KLFMimeExportProfile> exportProfileList();
  static void addExportProfile(const KLFMimeExportProfile& exportProfile);

  static KLFMimeExportProfile findExportProfile(const QString& pname);

private:

  QString p_profileName;
  QString p_description;
  QList<ExportType> p_exportTypes;
  QString p_inSubMenu;

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

  void set_possible_qt_handled_data();

  mutable QStringList pQtOwnedFormats;
};


class KLFExportTypeUserScriptInfoPrivate;

class KLF_EXPORT KLFExportTypeUserScriptInfo : public KLFUserScriptInfo
{
public:
  KLFExportTypeUserScriptInfo(const QString& scriptFileName, KLFBackend::klfSettings * settings);
  KLFExportTypeUserScriptInfo(const KLFExportTypeUserScriptInfo& copy);
  virtual ~KLFExportTypeUserScriptInfo();

  QStringList mimeTypes() const;
  QStringList outputFilenameExtensions() const;
  QStringList outputFormatDescriptions() const;
  QString inputDataType() const;

  int count() const;
  int findMimeType(const QString& mimeType) const;
  QString mimeType(int index) const;
  QString outputFilenameExtension(int index) const;
  QString outputFormatDescription(int index) const;

  bool wantStdinInput() const;
  bool hasStdoutOutput() const;

private:
  KLF_DECLARE_PRIVATE(KLFExportTypeUserScriptInfo) ;
};


class KLFExportUserScriptPrivate;

class KLF_EXPORT KLFExportUserScript
{
public:
  KLFExportUserScript(const QString& scriptFileName, KLFBackend::klfSettings * settings);
  ~KLFExportUserScript();

  KLFExportTypeUserScriptInfo info() const;

  QStringList availableMimeTypes(const KLFBackend::klfOutput * output = NULL) const;

  QByteArray getData(const QString& mimeType, const KLFBackend::klfOutput& output);

private:
  KLF_DECLARE_PRIVATE(KLFExportUserScript);
};


#endif

