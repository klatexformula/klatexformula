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

#include <klfdefs.h>
#include <klflib.h>



class KLF_EXPORT KLFLibDBConnectionClassUser {
public:
  KLFLibDBConnectionClassUser();
  virtual ~KLFLibDBConnectionClassUser();
  
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
class KLF_EXPORT KLFLibDBEngine : public KLFLibResourceSimpleEngine, private KLFLibDBConnectionClassUser
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

  virtual bool canModifyData(ModifyType modifytype) const;
  virtual bool canModifyProp(int propid) const;
  virtual bool canRegisterProperty(const QString& propName) const;

  /** True if one has supplied a valid database in the constructor or with a
   * \ref setDatabase() call. */
  virtual bool validDatabase() const;
  /** supply an open database. */
  virtual void setDatabase(const QSqlDatabase& db_connection);

  virtual KLFLibEntry entry(const QString& subRes, entryId id);
  virtual QList<KLFLibEntryWithId> allEntries(const QString& subRes);

public slots:

  virtual bool createSubResource(const QString& subResource, const QString& subResourceTitle);

  virtual QList<entryId> insertEntries(const QString& subRes, const KLFLibEntryList& entries);
  virtual bool changeEntries(const QString& subRes, const QList<entryId>& idlist,
			     const QList<int>& properties, const QList<QVariant>& values);
  virtual bool deleteEntries(const QString& subRes, const QList<entryId>& idlist);

  virtual bool saveAs(const QUrl& newPath);

protected:
  virtual bool saveResourceProperty(int propId, const QVariant& value);
  virtual bool setSubResourceProperty(const QString& subResource, int propId, const QVariant& value);

private slots:
  /** Called by our instance of KLFLibDBEnginePropertyChangeNotifier that tells us when
   * other class instances using the same connection change the properties. */
  void resourcePropertyUpdate(int propId);

  //! Read given resource property from DB
  /** If \c propId == -1, all properties are (re-)read. */
  void readResourceProperty(int propId);


private:
  KLFLibDBEngine(const QSqlDatabase& db, bool autoDisconnectDB, const QUrl& url,
		 bool accessshared, QObject *parent);

  QSqlDatabase pDB;
  
  QStringList detectEntryColumns(const QSqlQuery& q);
  KLFLibEntry readEntry(const QSqlQuery& q, const QStringList& columns);

  QVariant convertVariantToDBData(const QVariant& value) const;
  QVariant convertVariantFromDBData(const QVariant& dbdata) const;
  QVariant encaps(const char *ts, const QString& data) const;
  QVariant encaps(const char *ts, const QByteArray& data) const;
  QVariant decaps(const QString& string) const;
  QVariant decaps(const QByteArray& data) const;

  /** Inserts columns into datatable that don't exist for each extra registered property,
   * in sub-resource subResource. */
  bool ensureDataTableColumnsExist(const QString& subResource);

  /** Initializes a fresh database, without any sub-resource. */
  static bool initFreshDatabase(QSqlDatabase db);
  /** Creates and initializes a fresh data table. It should not yet exist. \c subresource should
   * NOT contain the leading \c "t_" prefix. */
  static bool createFreshDataTable(QSqlDatabase db, const QString& subresource);

  bool tableExists(const QString& subResource);

  static QString dataTableName(const QString& subResource);
  static QString quotedDataTableName(const QString& subResource);

  static QMap<QString,KLFLibDBEnginePropertyChangeNotifier*> pDBPropertyNotifiers;
  static KLFLibDBEnginePropertyChangeNotifier *dbPropertyNotifierInstance(const QString& dbname);
};



/** The associated factory to the KLFLibDBEngine engine. */
class KLF_EXPORT KLFLibDBEngineFactory : public KLFLibEngineFactory
{
  Q_OBJECT
public:
  KLFLibDBEngineFactory(QObject *parent = NULL);
  virtual ~KLFLibDBEngineFactory() { }

  /** Should return a list of supported URL schemes this factory can open */
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
