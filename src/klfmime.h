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
#include <klfuserscript.h>
#include <klfbackend.h>

#include <klfexporter.h>

/** \brief An export profile grouping several MIME types and associated exporters to
 *         generate the corresponding data
 */
class KLF_EXPORT KLFMimeExportProfile
{
public:

  struct ExportType {
    struct DefaultQtFormat {
      DefaultQtFormat(const QString& qtType_ = QString(), const QStringList& onlyOnWs_ = QStringList())
        : qtType(qtType_), onlyOnWs(onlyOnWs_) { }
      QString qtType;
      QStringList onlyOnWs;
    };
    ExportType(const QString & exporterName_ = QString(),
               const QString & exporterFormat_ = QString(),
               const QList<DefaultQtFormat>& defaultQtFormats_ = QList<DefaultQtFormat>(),
               const QStringList& mimeTypes_ = QStringList(),
               const QStringList& winFormats_ = QStringList(),
               const QStringList& macFlavors_ = QStringList())
      : exporterName(exporterName_), exporterFormat(exporterFormat_),
        defaultQtFormats(defaultQtFormats_),
        mimeTypes(mimeTypes_), winFormats(winFormats_), macFlavors(macFlavors_)
    { }
    QString exporterName;
    QString exporterFormat;
    QList<DefaultQtFormat> defaultQtFormats;
    QList<DefaultQtFormat> defaultQtFormatsOnThisWs() const;
    QStringList mimeTypes;
    QStringList winFormats;
    QStringList macFlavors;
  };

  KLFMimeExportProfile(const QString& pname, const QString& desc,
                       const QList<ExportType>& exporttypes,
		       const QString& insubmenu = QString());

  // construct an invalid profile, with null name and description
  KLFMimeExportProfile();

  QString profileName() const { return p_profileName; }
  QString description() const { return p_description; }

  QString inSubMenu() const { return p_inSubMenu; }

  /** List of formats to export when using this export profile. */
  inline QList<ExportType> exportTypes() const { return p_exportTypes; }

  /** Number of export types. Equivalent to <tt>exportTypes().size()</tt> */
  inline int exportTypesCount() const { return p_exportTypes.size(); }

  /** Returns export type at position \c n. \c n MUST be in the valid range \c 0 ... exportTypesCount(). */
  inline ExportType exportType(int n) const { return p_exportTypes[n]; }

  /** A list of all the available mime types provided by this profile.
   *
   * This is the collected list of all mime types of all export types for which an
   * exporter exists in \a exporterManager which can export the \a output to.
   */
  QStringList collectedAvailableMimeTypes(KLFExporterManager * exporterManager,
                                          const KLFBackend::klfOutput & output) const;

  /** A list of all the available windows formats provided by this profile.
   *
   * This is the collected list of all windows formats of all export types for which an
   * exporter exists in \a exporterManager which can export the \a output to.
   */
  QStringList collectedAvailableWinFormats(KLFExporterManager * exporterManager,
                                          const KLFBackend::klfOutput & output) const;

  /** A list of all the available mac flavors provided by this profile.
   *
   * This is the collected list of all mac flavors of all export types for which an
   * exporter exists in \a exporterManager which can export the \a output to.
   */
  QStringList collectedAvailableMacFlavors(KLFExporterManager * exporterManager,
                                           const KLFBackend::klfOutput & output) const;

  /** \brief The export type index of the first export type with an exporter and format
   *         capable of generating the given mime type
   * 
   * If a MIME type appears several times in a same profile, the first occurrence in an
   * available export type is used. An export type is unavailable if the exporter name
   * doesn't exist (e.g., if a user script can't be executed).
   *
   * Returns \a -1 if no such MIME type can be found.
   */
  int indexOfAvailableMimeType(const QString & mimeType,
                               KLFExporterManager * exporterManager,
                               const KLFBackend::klfOutput & klfoutput) const;

  /** \brief The export type index of the first export type with an exporter and format
   *         capable of generating the given windows format
   * 
   * If a windows format type appears several times in a same profile, the first occurrence in an
   * available export type is used. An export type is unavailable if the exporter name
   * doesn't exist (e.g., if a user script can't be executed).
   *
   * Returns \a -1 if no such windows format can be found.
   */
  int indexOfAvailableWinFormat(const QString & winFormat,
                               KLFExporterManager * exporterManager,
                               const KLFBackend::klfOutput & klfoutput) const;

  /** \brief The export type index of the first export type with an exporter and format
   *         capable of generating the given mac flavor
   * 
   * If a mac flavor appears several times in a same profile, the first occurrence in an
   * available export type is used. An export type is unavailable if the exporter name
   * doesn't exist (e.g., if a user script can't be executed).
   *
   * Returns \a -1 if no such mac flavor can be found.
   */
  int indexOfAvailableMacFlavor(const QString & macFlavor,
                               KLFExporterManager * exporterManager,
                               const KLFBackend::klfOutput & klfoutput) const;


  /** \brief Load profiles from an XML description file
   *
   * The file should be an XML file of the format found in the \c
   * "conf/mime_export_profiles.d/xxx.xml" directory.
   */
  static QList<KLFMimeExportProfile> loadProfilesFromXmlFile(const QString & xmlFile);

private:

  QString p_profileName;
  QString p_description;
  QList<ExportType> p_exportTypes;
  QString p_inSubMenu;
};

struct KLFMimeExportProfileManagerPrivate;

class KLFMimeExportProfileManager
{
public:
  KLFMimeExportProfileManager();
  ~KLFMimeExportProfileManager();

  QList<KLFMimeExportProfile> exportProfileList();
  void addExportProfile(const KLFMimeExportProfile& exportProfile);
  void addExportProfiles(const QList<KLFMimeExportProfile> & initialProfileList);

  KLFMimeExportProfile findExportProfile(const QString& pname);

  static QList<KLFMimeExportProfile> loadKlfExportProfileList();

private:
  KLF_DECLARE_PRIVATE(KLFMimeExportProfileManager) ;
};



struct KLFMimeDataPrivate;

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
  KLFMimeData(const KLFMimeExportProfile& exportProfile,
              KLFExporterManager * exporterManager,
              const KLFBackend::klfOutput& output);
  virtual ~KLFMimeData();

  QStringList formats() const;

  void transmitAllData();

  static QList<KLFMimeData*> activeMimeDataInstances();

protected:
  QVariant retrieveData(const QString &mimetype, QVariant::Type type) const;

private:
  KLF_DECLARE_PRIVATE( KLFMimeData ) ;
};





#define KLF_MIME_PROXY_MAC_FLAVOR_PREFIX "application/x-klf-proxymacflavor-"
#define KLF_MIME_PROXY_WIN_FORMAT_PREFIX "application/x-klf-proxywinformat-"




/*
struct KLFExportTypeUserScriptInfoPrivate;

class KLF_EXPORT KLFExportTypeUserScriptInfo : public KLFUserScriptInfo
{
public:
  KLFExportTypeUserScriptInfo(const QString& scriptFileName, KLFBackend::klfSettings * settings);
  KLFExportTypeUserScriptInfo(const KLFExportTypeUserScriptInfo& copy);
  KLFExportTypeUserScriptInfo(const KLFUserScriptInfo& copy);
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


struct KLFExportUserScriptPrivate;

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
*/



#endif

