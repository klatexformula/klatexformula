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
#include <QTimer>

#include <klflib.h>



/** Legacy data structures for KLatexFormula
 *
 * \warning This class is OBSOLETE. It has been replaced by the new KLFLib
 *   framework.
 */
class KLF_EXPORT KLFLegacyData {
public:

  struct KLFStyle {
    KLFStyle(QString nm = QString(), unsigned long fgcol = qRgba(0,0,0,255),
	     unsigned long bgcol = qRgba(255,255,255,0),
	     const QString& mmode = QString(),
	     const QString& pre = QString(),
	     int dotsperinch = -1)
      : name(nm), fg_color(fgcol), bg_color(bgcol), mathmode(mmode), preamble(pre),
	dpi(dotsperinch) { }
    QString name; // this may not always be set, it's only important in saved style list.
    unsigned long fg_color;
    unsigned long bg_color;
    QString mathmode;
    QString preamble;
    int dpi;
  };

  // THESE VALUES MUST NOT CHANGE FROM ONE VERSION TO ANOTHER OF KLATEXFORMULA :
  enum {
    LibResource_History = 0,
    LibResource_Archive = 1,
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

    KLFLegacyData::KLFStyle style;
  };

  typedef QList<KLFLibraryItem> KLFLibraryList;
  typedef QList<KLFLibraryResource> KLFLibraryResourceList;
  typedef QMap<KLFLibraryResource, KLFLibraryList> KLFLibrary;

private:

  KLFLegacyData();
};

QDataStream& operator<<(QDataStream& stream, const KLFLegacyData::KLFStyle& style);
QDataStream& operator>>(QDataStream& stream, KLFLegacyData::KLFStyle& style);
bool operator==(const KLFLegacyData::KLFStyle& a, const KLFLegacyData::KLFStyle& b);


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




//! The Legacy Library support for the KLFLib framework
/** Implements a KLFLibResourceEngine resource engine for accessing (KLF<=3.1)-created libraries
 * (*.klf, default library files)
 *
 * Different legacy resources (in the *.klf file) are mapped to sub-resources (in KLFLibResourceEngine).
 */
class KLF_EXPORT KLFLibLegacyEngine : public KLFLibResourceSimpleEngine
{
  Q_OBJECT
public:
  /** Use this function as a constructor for a KLFLibLegacyEngine object.
   *
   * Opens the URL referenced by url and returns a pointer to a freshly instantiated
   * KLFLibLegacyEngine object, the parent of which is set to \c parent. Returns
   * NULL in case of an error.
   *
   */
  static KLFLibLegacyEngine * openUrl(const QUrl& url, QObject *parent = NULL);

  /** Use this function as a constructor. Creates a KLFLibLegacyEngine object,
   * with QObject parent \c parent, creating a fresh, empty .klf file.
   *
   * Returns NULL if creating the file failed.
   *
   * \c legacyResourceName is the name of an empty (legacy) resource (ie. sub-resource) to create
   * in the newly created file.
   *
   * A non-NULL returned object is linked to a file that was successfully created.
   * */
  static KLFLibLegacyEngine * createDotKLF(const QString& fileName, QString legacyResourceName,
					   QObject *parent = NULL);

  virtual ~KLFLibLegacyEngine();

  virtual uint compareUrlTo(const QUrl& other, uint interestFlags = 0xfffffff) const;

  virtual bool canModifyData(const QString& subRes, ModifyType modifytype) const;
  virtual bool canModifyProp(int propid) const;
  virtual bool canRegisterProperty(const QString& propName) const;

  virtual KLFLibEntry entry(const QString& resource, entryId id);
  virtual QList<KLFLibEntryWithId> allEntries(const QString& resource,
					      const QList<int>& wantedEntryProperties = QList<int>());

  virtual QStringList subResourceList() const;

public slots:

  virtual bool createSubResource(const QString& subResource, const QString& subResourceTitle);

  virtual bool save();
  virtual void setAutoSaveInterval(int intervalms);

  virtual QList<entryId> insertEntries(const QString& subResource, const KLFLibEntryList& entries);
  virtual bool changeEntries(const QString& subResource, const QList<entryId>& idlist,
			     const QList<int>& properties, const QList<QVariant>& values);
  virtual bool deleteEntries(const QString& subResource, const QList<entryId>& idlist);

  virtual bool saveTo(const QUrl& newPath);

protected:
  virtual bool saveResourceProperty(int propId, const QVariant& value);

private:
  KLFLibLegacyEngine(const QString& fileName, const QString& resname, const QUrl& url, QObject *parent);

  /** if pCloneOf is non-NULL, EVERY FUNCTION CALL redirects the call to the clone original.
   *
   * \bug A clone-based approach was taken to avoid multiple engines accessing the same file. A better approach
   *   would have been to have one instance of a KLFLibLegacyFileData (eg. QObject-subclass) containing the
   *   filename, KLFLibLegacyData::KLFLibrary etc. data, lib type etc. and all resources that want to connect
   *   to this file read/write that (shared) data. With a refcount one can make the object destroy itself after
   *   the last one using it was destroyed. */
  KLFLibLegacyEngine *pCloneOf;
  /** a list of objects whose pCloneOf points to me. */
  QList<KLFLibLegacyEngine*> pMyClones;

  /** copies all data from the clone to this object, making this object independant.
   * this is called by the master clone's destructor */
  void takeOverFromClone();

  static QMap<QString,KLFLibLegacyEngine*> staticLegacyEngineInstances;

  enum LegacyLibType { LocalHistoryType = 1, LocalLibraryType, ExportLibraryType };

  int findResourceName(const QString& resname);
  int getReservedResourceId(const QString& resourceName, int defaultId);

  // Resource data is data that has to be copied when taking over from a master clone
  // If you add a property here, make sure it is copied in takeOverFromClone().
  // BEGIN RESOURCE DATA ->

  QString pFileName;

  KLFLegacyData::KLFLibrary pLibrary;
  KLFLegacyData::KLFLibraryResourceList pResources;

  LegacyLibType pLegacyLibType;

  QTimer *pAutoSaveTimer;

  // <- END RESOURCE DATA

  static KLFLibEntry toLibEntry(const KLFLegacyData::KLFLibraryItem& item);
  static KLFLegacyData::KLFLibraryItem toLegacyLibItem(const KLFLibEntry& entry);
  static KLFLegacyData::KLFStyle toLegacyStyle(const KLFStyle& style);
  static KLFStyle toStyle(const KLFLegacyData::KLFStyle& oldstyle);

  static bool loadLibraryFile(const QString& fname, KLFLegacyData::KLFLibraryResourceList *reslist,
			      KLFLegacyData::KLFLibrary *lib, LegacyLibType *libType);
  static bool saveLibraryFile(const QString& fname, const KLFLegacyData::KLFLibraryResourceList& reslist,
			      const KLFLegacyData::KLFLibrary& lib, LegacyLibType legacyLibType);
};



class KLF_EXPORT KLFLibLegacyLocalFileSchemeGuesser : public QObject, public KLFLibLocalFileSchemeGuesser
{
public:
  KLFLibLegacyLocalFileSchemeGuesser(QObject *parent) : QObject(parent) { }

  QString guessScheme(const QString& fileName) const;
};


/** The associated factory to the KLFLibDBEngine engine. */
class KLF_EXPORT KLFLibLegacyEngineFactory : public KLFLibEngineFactory
{
  Q_OBJECT
public:
  KLFLibLegacyEngineFactory(QObject *parent = NULL);
  virtual ~KLFLibLegacyEngineFactory() { }

  virtual QStringList supportedTypes() const;
  virtual QString schemeTitle(const QString& scheme) const ;

  virtual uint schemeFunctions(const QString& scheme) const;

  virtual QString correspondingWidgetType(const QString& scheme) const;

  /** Create a library engine that opens resource stored at \c location */
  virtual KLFLibResourceEngine *openResource(const QUrl& location, QObject *parent = NULL);

  virtual KLFLibResourceEngine *createResource(const QString& scheme, const Parameters& parameters,
					       QObject *parent = NULL);
};





#endif
