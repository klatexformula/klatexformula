/***************************************************************************
 *   file klflibdbengine.h
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

#ifndef KLFLIBDBENGINE_H
#define KLFLIBDBENGINE_H

#include <QSqlDatabase>
#include <QSqlQuery>

#include <klfdefs.h>
#include <klflib.h>
#include <klflibview.h> // KLFLibLocalFileSchemeGuesser

//! A utility class to automatically disconnect a database after use
/** This class basically calls \ref QSqlDatabase::removeDatabase() upon its destruction, if the
 * pAutoDisconnectDB flag is TRUE. The disconnected database name is given by the
 * \ref pDBConnectionName property, which can be set directly in sub-classes, or equivalently using
 * the \ref setDBConnectionName() member.
 * */
class KLF_EXPORT KLFLibDBConnectionClassUser {
public:
  KLFLibDBConnectionClassUser();
  virtual ~KLFLibDBConnectionClassUser();

  inline bool autoDisconnectDB() const { return pAutoDisconnectDB; }
  inline void setAutoDisconnectDB(bool autodisconnectDB) { pAutoDisconnectDB = autodisconnectDB; }
  inline QString dbConnectionName() const { return pDBConnectionName; }
  inline void setDBConnectionName(const QString& name) { pDBConnectionName = name; }
  
protected:
  bool pAutoDisconnectDB;
  QString pDBConnectionName;
};


class KLFLibDBEnginePropertyChangeNotifier;

/** Library Resource engine implementation for an (abstract) database (using Qt
 * SQL interfaces)
 *
 * For now, only SQLITE is supported. However, the class is designed to be easily adaptable
 * to add support for eg. MySQL, PostgreSQL, etc.
 *
 * Sub-resources are supported and translated to different SQLite table names, which are
 * prefixed with "t_". Sub-resources themselves must have machine-friendly names (no special
 * characters, especially the SQLite escape double-quote <tt>"</tt> character); however the
 * sub-resource titles may be fantasy.
 *
 * Sub-resource properties are also supported in a limited way
 * (only built-in properties Title and ViewType are supported).
 */
class KLF_EXPORT KLFLibDBEngine : public KLFLibResourceEngine, private KLFLibDBConnectionClassUser
{
  Q_OBJECT

public:
  /** Use this function as a constructor. Creates a KLFLibDBEngine object,
   * with QObject parent \c parent, opening the database at location \c url.
   * Returns NULL if opening the database failed.
   *
   * A non-NULL returned object was successfully connected to database.
   * */
  static KLFLibDBEngine * openUrl(const QUrl& url, QObject *parent = NULL);

  /** Use this function as a constructor. Creates a KLFLibDBEngine object,
   * with QObject parent \c parent, creating a fresh, empty SQLITE database
   * stored in file \c fileName.
   *
   * \c subresourcename is the name to give the default sub-resource. \c subresourcetitle is the
   * human title to attribute to it.
   *
   * Returns NULL if opening the database failed.
   *
   * A non-NULL returned object was successfully connected to database.
   * */
  static KLFLibDBEngine * createSqlite(const QString& fileName, const QString& subresourcename,
				       const QString& subresourcetitle, QObject *parent = NULL);


  /** Simple destructor. Disconnects the database if autoDisconnectDB was requested
   * in the constructor. */
  virtual ~KLFLibDBEngine();

  virtual uint compareUrlTo(const QUrl& other, uint interestFlags = 0xfffffff) const;

  virtual bool compareDefaultSubResourceEquals(const QString& subResourceName) const;

  virtual bool canModifyData(const QString& subRes, ModifyType modifytype) const;
  virtual bool canModifyProp(int propid) const;
  virtual bool canRegisterProperty(const QString& propName) const;

  /** True if one has supplied a valid database in the constructor or with a
   * \ref setDatabase() call. */
  virtual bool validDatabase() const;
  /** supply an open database. */
  virtual void setDatabase(const QSqlDatabase& db_connection);


  virtual QList<KLFLib::entryId> allIds(const QString& subResource);
  virtual bool hasEntry(const QString&, entryId id);
  virtual QList<KLFLibEntryWithId> entries(const QString&, const QList<KLFLib::entryId>& idList,
					   const QList<int>& wantedEntryProperties = QList<int>());
  virtual int findEntries(const QString& subResource,
			  const EntryMatchCondition& matchcondition,
			  QList<KLFLib::entryId> * entryIdList,
			  int limit = 500,
			  KLFLibEntryList *rawEntryList = NULL,
			  QList<KLFLibEntryWithId> * entryWithIdList = NULL,
			  const QList<int>& wantedEntryProperties = QList<int>());

  virtual KLFLibEntry entry(const QString& subRes, entryId id);
  virtual QList<KLFLibEntryWithId> allEntries(const QString& subRes,
					      const QList<int>& wantedEntryProperties = QList<int>());

  virtual bool canCreateSubResource() const;
  virtual bool canRenameSubResource() const { return false; }

  virtual QVariant subResourceProperty(const QString& subResource, int propId) const;

  virtual QList<int> subResourcePropertyIdList() const
  { return QList<int>() << SubResPropTitle << SubResPropViewType << SubResPropLocked; }

  virtual bool hasSubResource(const QString& subRes) const;
  virtual QStringList subResourceList() const;

public slots:

  virtual bool createSubResource(const QString& subResource, const QString& subResourceTitle);

  virtual QList<entryId> insertEntries(const QString& subRes, const KLFLibEntryList& entries);
  virtual bool changeEntries(const QString& subRes, const QList<entryId>& idlist,
			     const QList<int>& properties, const QList<QVariant>& values);
  virtual bool deleteEntries(const QString& subRes, const QList<entryId>& idlist);

  virtual bool saveTo(const QUrl& newPath);

  virtual bool setSubResourceProperty(const QString& subResource, int propId, const QVariant& value);

protected:
  virtual bool saveResourceProperty(int propId, const QVariant& value);

private slots:
  /** Called by our instance of KLFLibDBEnginePropertyChangeNotifier that tells us when
   * other class instances using the same connection change the properties. */
  void resourcePropertyUpdate(int propId);
  /** Called by our instance of KLFLibDBEnginePropertyChangeNotifier that tells us when
   * other class instances using the same connection change the properties. */
  void subResourcePropertyUpdate(const QString& subResource, int propId);

  //! Read given resource property from DB
  /** If \c propId == -1, all properties are (re-)read. */
  void readResourceProperty(int propId);

  void readDbMetaInfo();

  void readAvailColumns(const QString& subResource);

private:
  KLFLibDBEngine(const QSqlDatabase& db, bool autoDisconnectDB, const QUrl& url,
		 bool accessshared, QObject *parent);

  QSqlDatabase pDB;

  int pDBVersion;

  QMap<QString,QStringList> pDBAvailColumns;
  
  QStringList columnNameList(const QString& subResource, const QList<int>& entryPropList,
			     bool wantIdFirst = true);
  QStringList detectEntryColumns(const QSqlQuery& q);
  KLFLibEntry readEntry(const QSqlQuery& q, const QStringList& columns);

  QVariant convertVariantToDBData(const QVariant& value) const;
  QVariant convertVariantFromDBData(const QVariant& dbdata) const;
  QVariant encaps(const char *ts, const QString& data) const;
  QVariant encaps(const char *ts, const QByteArray& data) const;
  QVariant decaps(const QString& string) const;
  QVariant decaps(const QByteArray& data) const;

  bool ensureDataTableColumnsExist(const QString& subResource, const QStringList& columnList);
  /** Inserts columns into datatable that don't exist for each extra registered property,
   * in sub-resource subResource. */
  bool ensureDataTableColumnsExist(const QString& subResource);

  /** Initializes a fresh database, without any sub-resource. */
  static bool initFreshDatabase(QSqlDatabase db);
  /** Creates and initializes a fresh data table. It should not yet exist. \c subresource should
   * NOT contain the leading \c "t_" prefix. */
  static bool createFreshDataTable(QSqlDatabase db, const QString& subresource);

  bool tableExists(const QString& subResource) const;

  static QString dataTableName(const QString& subResource);
  static QString quotedDataTableName(const QString& subResource);

  static QMap<QString,KLFLibDBEnginePropertyChangeNotifier*> pDBPropertyNotifiers;
  static KLFLibDBEnginePropertyChangeNotifier *dbPropertyNotifierInstance(const QString& dbname);
};



class KLF_EXPORT KLFLibDBLocalFileSchemeGuesser : public QObject, public KLFLibLocalFileSchemeGuesser
{
public:
  KLFLibDBLocalFileSchemeGuesser(QObject *parent) : QObject(parent) { }

  QString guessScheme(const QString& fileName) const;
};


/** The associated \ref KLFLibEngineFactory factory to the \ref KLFLibDBEngine resource engine. */
class KLF_EXPORT KLFLibDBEngineFactory : public KLFLibEngineFactory
{
  Q_OBJECT
public:
  KLFLibDBEngineFactory(QObject *parent = NULL);
  virtual ~KLFLibDBEngineFactory() { }

  virtual QStringList supportedTypes() const;
  virtual QString schemeTitle(const QString& scheme) const ;
  virtual uint schemeFunctions(const QString& scheme) const ;

  virtual QString correspondingWidgetType(const QString& scheme) const;
  virtual KLFLibResourceEngine *openResource(const QUrl& location, QObject *parent = NULL);

  virtual KLFLibResourceEngine *createResource(const QString& scheme, const Parameters& parameters,
					       QObject *parent = NULL);
};



#endif
