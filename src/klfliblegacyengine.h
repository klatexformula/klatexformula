/***************************************************************************
 *   file klfliblegacyengine.h
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

#ifndef KLFLIBLEGACYENGINE_H
#define KLFLIBLEGACYENGINE_H



#include <QDateTime>
#include <QList>
#include <QDataStream>
#include <QPixmap>
#include <QMap>
#include <QMetaType>
#include <QFile>

#include <klflib.h>



/** Legacy data structures for KLatexFormula
 *
 * \warning This class is OBSOLETE. It has been replaced by the new KLFLib
 *   framework.
 */
class KLF_EXPORT KLFLegacyData {
public:

  // THESE VALUES MUST NOT CHANGE FROM ONE VERSION TO ANOTHER OF KLATEXFORMULA :
  enum {  LibResource_History = 0, LibResource_Archive = 1,
    // user resources must be in the following range:
    LibResourceUSERMIN = 100,
    LibResourceUSERMAX = 99999
  };

  struct KLFLibraryResource {
    quint32 id;
    QString name;
  };

  struct KLFLibraryItem {
    quint32 id;
    static quint32 MaxId;

    QDateTime datetime;
    /** \note \c latex contains also information of category (first line, %: ...) and
     * tags (first/second line, after category: % ...) */
    QString latex;
    QPixmap preview;

    QString category;
    QString tags;

    KLFStyle style;
  };

  static QString categoryFromLatex(const QString& latex);
  static QString tagsFromLatex(const QString& latex);
  static QString stripCategoryTagsFromLatex(const QString& latex);

  typedef QList<KLFLibraryItem> KLFLibraryList;
  typedef QList<KLFLibraryResource> KLFLibraryResourceList;
  typedef QMap<KLFLibraryResource, KLFLibraryList> KLFLibrary;

private:

  KLFLegacyData();
};

// it is important to note that the >> operator imports in a compatible way to KLF 2.0
QDataStream& operator<<(QDataStream& stream, const KLFLegacyData::KLFLibraryItem& item);
QDataStream& operator>>(QDataStream& stream, KLFLegacyData::KLFLibraryItem& item);

QDataStream& operator<<(QDataStream& stream, const KLFLegacyData::KLFLibraryResource& item);
QDataStream& operator>>(QDataStream& stream, KLFLegacyData::KLFLibraryResource& item);

// exact matches, style included, but excluding ID and datetime
bool operator==(const KLFLegacyData::KLFLibraryItem& a, const KLFLegacyData::KLFLibraryItem& b);

// is needed for QMap : these operators compare ID only.
bool operator<(const KLFLegacyData::KLFLibraryResource a, const KLFLegacyData::KLFLibraryResource b);
bool operator==(const KLFLegacyData::KLFLibraryResource a, const KLFLegacyData::KLFLibraryResource b);
// name comparision
bool resources_equal_for_import(const KLFLegacyData::KLFLibraryResource a,
				const KLFLegacyData::KLFLibraryResource b);



//! The Legacy Library support for the new KLFLib
/** Implements a KLFLibResourceEngine resource engine for accessing (KLF<=3.1)-created libraries
 * (*.klf, default library files) */
class KLFLibLegacyEngine : public KLFLibResourceEngine
{
  Q_OBJECT
public:
  /** Use this function as a constructor for a KLFLibLegacyEngine object.
   *
   * Opens the URL referenced by url and returns a pointer to a freshly instantiated
   * KLFLibLegacyEngine object, the parent of which is set to \c parent. Returns
   * NULL in case of an error.
   *
   * Url must contain following query string:
   * - <tt>legacyResourceName=<i>name</i></tt> to specify the (legacy) resource name of
   *   the resource we're interested in the file, for the KLFLibResourceEngine API.
   *   Note: you can always access the entries in a given resource by calling
   *   .....................
   */
  static KLFLibLegacyEngine * openUrl(const QUrl& url, QObject *parent = NULL);
  /** Use this function as a constructor. Creates a KLFLibLegacyEngine object,
   * with QObject parent \c parent, creating a fresh, empty .klf file.
   *
   * Returns NULL if creating the file failed.
   *
   * \c legacyResourceName .....................................
   *
   * A non-NULL returned object was successfully connected to database.
   * */
  static KLFLibLegacyEngine * createDotKLF(const QString& fileName, QString legacyResourceName,
					   QObject *parent = NULL);

  virtual ~KLFLibLegacyEngine();

  virtual bool canModifyData(ModifyType modifytype) const;
  virtual bool canModifyProp(int propid) const;
  virtual bool canRegisterProperty(const QString& propName) const;

  virtual QString curLegacyResource() const { return pLegacyRes; }

  virtual KLFLibEntry entry(entryId id);
  virtual QList<KLFLibEntryWithId> allEntries();

  virtual QList<KLFLibEntryWithId> allEntriesInResource(const QString& resource);

  /** Lists the (legacy) resource names present in the given .klf file. This function
   * is not very optimized. (it opens and closes the resource) */
  static QStringList getLegacyResourceNames(const QUrl& url);

public slots:

  virtual void setCurrentLegacyResource(const QString& curLegacyResource);

  virtual QList<entryId> insertEntries(const KLFLibEntryList& entries);
  virtual bool changeEntries(const QList<entryId>& idlist, const QList<int>& properties,
			   const QList<QVariant>& values);
  virtual bool deleteEntries(const QList<entryId>& idlist);

  virtual bool saveAs(const QUrl& newPath);

protected:
  virtual bool saveResourceProperty(int propId, const QVariant& value);

private:
  KLFLibLegacyEngine(const QString& fname, const QString& resname, const QUrl& url,
		     bool accessshared, QObject *parent);

  QFile *pFile;
  
  QString pResourceName;


};


/** The associated factory to the KLFLibDBEngine engine. */
class KLF_EXPORT KLFLibLegacyEngineFactory : public KLFLibEngineFactory
{
  Q_OBJECT
public:
  KLFLibLegacyEngineFactory(QObject *parent = NULL);
  virtual ~KLFLibLegacyEngineFactory() { }

  virtual QStringList supportedSchemes() const;
  virtual QString schemeTitle(const QString& scheme) const ;

  virtual QWidget * createPromptUrlWidget(QWidget *parent, const QString& scheme,
					  QUrl defaultlocation = QUrl());
  virtual QUrl retrieveUrlFromWidget(const QString& scheme, QWidget *widget);

  /** Create a library engine that opens resource stored at \c location */
  virtual KLFLibResourceEngine *openResource(const QUrl& location, QObject *parent = NULL);


  virtual bool canCreateResource(const QString& /*scheme*/) const { return true; }

  virtual QWidget * createPromptCreateParametersWidget(QWidget *parent, const QString& scheme,
						       const Parameters& defaultparameters = Parameters());

  virtual Parameters retrieveCreateParametersFromWidget(const QString& scheme, QWidget *widget);

  virtual KLFLibResourceEngine *createResource(const QString& scheme, const Parameters& parameters,
					       QObject *parent = NULL);

};





#endif
