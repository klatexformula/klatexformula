/***************************************************************************
 *   file klfexporter.h
 *   This file is part of the KLatexFormula Project.
 *   Copyright (C) 2016 by Philippe Faist
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

#ifndef KLFEXPORTER_H
#define KLFEXPORTER_H

#include <QString>
#include <QList>
#include <QStringList>
#include <QByteArray>
#include <QMap>
#include <QImage>
#include <QImageWriter>
#include <QMimeData>
#include <QTemporaryFile>
#include <QTemporaryDir>

#include <klfdefs.h>
#include <klfpobj.h>
#include <klfuserscript.h>
#include <klfbackend.h>


struct KLFExporterPrivate;

/** \brief Abstract exporter of KLF output to some given format
 *
 * This helper class can be subclassed to implement exporting klatexformula output in a given
 * data format.
 *
 * Formats are given internal names which can be chosen arbitrarily.
 * Exporters know nothing of mime types, 
 */
class KLF_EXPORT KLFExporter
{
public:
  KLFExporter();
  virtual ~KLFExporter();

  //! Name of the exporter (e.g. "pdf")
  virtual QString exporterName() const = 0;

  /** \brief Whether this exporter is suitable for saving to data file.
   *
   * Most exporters are suitable because they export the data itself in some particular
   * format.  Other exporters however may be designed only for copy-paste or drag-drop,
   * for example if they refer to a temporary file (e.g. text/uri-list).
   */
  virtual bool isSuitableForFileSave() const { return true; }

  /** \brief List of formats this exporter can export to (e.g. "pdf", "svg", "eps", "png@150dpi")
   *
   * The format name can be chosen freely, and does not have to coincide with a mime type
   * and/or filename extension(s).
   */
  virtual QStringList supportedFormats(const KLFBackend::klfOutput& klfoutput) const = 0;

  /** \brief Return TRUE if the given format is supported
   *
   *
   * The default implementation simply returns
   * <code>supportedFormats(klfoutput).contains(format)</code>, and should suffice in most
   * cases; you don't need to reimplement this function.
   *
   * However, in case you have many formats and depending on your use case, there may be a
   * more clever way of finding out whether a given format is supported than going through
   * the list of all supported formats.  In such a case you can reimplement this function.
   */
  virtual bool supports(const QString & format, const KLFBackend::klfOutput& klfoutput) const
  {
    return supportedFormats(klfoutput).contains(format);
  }

  /** \brief A huaman-readable title for this exporter and format (e.g. "PDF Vector Graphic")
   */
  virtual QString titleFor(const QString & format) = 0;

  /** \brief The filename extension suitable to save this format from this exporter
   *         (e.g. ["jpg","jpeg"])
   *
   * Subclasses should reimplement.  The base implementation returns an empty string list
   * and is only suitable for exporters which are not suitable for file saving (\ref
   * isSuitableForFileSave()).
   */
  virtual QStringList fileNameExtensionsFor(const QString & format);

  /** \brief Get the data corresponding to the given klf output
   *
   * The \a param argument may be used to specify additional parameters to the exporter.
   *
   * Subclasses should not forget to call \ref clearErrorString() to clear any previously
   * set error.
   */
  virtual QByteArray getData(const QString& format, const KLFBackend::klfOutput& klfoutput,
                             const QVariantMap& param = QVariantMap()) = 0;


  //! Returns an error string describing the last error.
  QString errorString() const;

protected:

  //! Subclasses should call this method at the beginning of getData()
  void clearErrorString();
  //! Subclasses should use this method in getData() to signify that an error occurred
  void setErrorString(const QString & errorString);

private:
  KLF_DECLARE_PRIVATE(KLFExporter) ;
};



struct KLFExporterManagerPrivate;

class KLFExporterManager
{
public:
  KLFExporterManager();
  virtual ~KLFExporterManager();

  //! The manager takes ownership of \a exporter and will delete it once the manager is destroyed.
  void registerExporter(KLFExporter *exporter);
  void unregisterExporter(KLFExporter *exporter);

  KLFExporter* exporterByName(const QString & exporterName);

  QList<KLFExporter*> exporterList();

private:
  KLF_DECLARE_PRIVATE( KLFExporterManager ) ;
};





// -----------------------------------------------------------------------------
// user script definitions:


struct KLFExportTypeUserScriptInfoPrivate;

class KLF_EXPORT KLFExportTypeUserScriptInfo : public KLFUserScriptInfo
{
public:
  KLFExportTypeUserScriptInfo(const QString& userScriptPath);
  virtual ~KLFExportTypeUserScriptInfo();
    
  struct Format : public KLFPropertizedObject
  {
    Format();
    Format(const QString& formatName_, const QString& formatTitle_, const QStringList& fileNameExtensions_);
    virtual ~Format();
    enum { FormatName = 0, FormatTitle, FileNameExtensions };
    QString formatName() const;
    QString formatTitle() const;
    QStringList fileNameExtensions() const;
  };

  enum ExportTypeProperties {
    Formats = 0,
    InputDataType,
    HasStdoutOutput,
    MimeExportProfilesXmlFile,
  };

  QStringList formats() const;
  QString inputDataType() const;
  bool hasStdoutOutput() const;
  QString mimeExportProfilesXmlFile() const;

  QList<Format> formatList() const;
  Format getFormat(const QString & formatName) const;

  QVariant klfExportTypeInfo(int propId) const;
  QVariant klfExportTypeInfo(const QString& key) const;
  QStringList klfExportTypeInfosList() const;

private:
  KLF_DECLARE_PRIVATE(KLFExportTypeUserScriptInfo) ;
};





/** \brief A stripped-down version of the latex string, suitable as a human-readable
 *         expression in plain text
 *
 * For example, \c "\vec{a}_\mathrm{xyz} = -\frac{1}{2}" becomes e.g.
 * \c "\vec{a}_{xyz} = -(1/2)".
 */
QString klfLatexToPseudoTex(QString latex);





#endif

