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
 */
class KLF_EXPORT KLFLibDBEngine : public KLFLibResourceSimpleEngine, private KLFLibDBConnectionClassUser
{
  Q_OBJECT

  Q_PROPERTY(QString dataTableName READ dataTableName)
public:
  /** Use this function as a constructor. Creates a KLFLibDBEngine object,
   * with QObject parent \c parent, opening the database at location \c url.
   * Returns NULL if opening the database failed.
   *
   * Url must contain following query strings:
   * - <tt>dataTableName=<i>tablename</i></tt> to specify the data table name
   *   in the DB
   *
   * \note if the \c dataTableName does not exist, the corresponding table is
   *   created, if the \c url is to be opened in non-readonly mode and the
   *   database is not locked.
   *
   * A non-NULL returned object was successfully connected to database.
   * */
  static KLFLibDBEngine * openUrl(const QUrl& url, QObject *parent = NULL);
  /** Use this function as a constructor. Creates a KLFLibDBEngine object,
   * with QObject parent \c parent, creating a fresh, empty SQLITE database
   * stored in file \c fileName.
   *
   * Returns NULL if opening the database failed.
   *
   * \param datatablename defaults to \c "klfentries" if an empty string is given.
   *  \c datatablename should NOT contain a leading \c "t_" prefix.
   *
   * A non-NULL returned object was successfully connected to database.
   * */
  static KLFLibDBEngine * createSqlite(const QString& fileName, const QString& datatablename,
				       QObject *parent = NULL);


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

  /** Returns the data table name, WITHOUT the leading \c "t_" prefix. */
  virtual QString dataTableName() const { return pDataTableName.mid(2); }

  virtual QString displayTitle() const { return QString("%1 - %2").arg(title(), dataTableName()); }

  virtual KLFLibEntry entry(entryId id);
  virtual QList<KLFLibEntryWithId> allEntries();

  /** Lists the data tables present in the given database. This function
   * is not very optimized. (it opens and closes the resource) */
  static QStringList getDataTableNames(const QUrl& url);

public slots:

  virtual QList<entryId> insertEntries(const KLFLibEntryList& entries);
  virtual bool changeEntries(const QList<entryId>& idlist, const QList<int>& properties,
			   const QList<QVariant>& values);
  virtual bool deleteEntries(const QList<entryId>& idlist);
  virtual bool saveAs(const QUrl& newPath);

protected:
  virtual bool saveResourceProperty(int propId, const QVariant& value);

private slots:
  void resourcePropertyUpdate(int propId);

  //! If \c propId == -1, all properties are (re-)read.
  void readResourceProperty(int propId);


private:
  /** \note \c tablename does NOT contain a leading \c "t_" prefix. */
  KLFLibDBEngine(const QSqlDatabase& db, const QString& tablename, bool autoDisconnectDB,
		 const QUrl& url, bool accessshared, QObject *parent);

  QSqlDatabase pDB;
  
  QString pDataTableName; ///< \note WITH leading \c "t_" prefix.

  QStringList detectEntryColumns(const QSqlQuery& q);
  KLFLibEntry readEntry(const QSqlQuery& q, const QStringList& columns);

  QVariant convertVariantToDBData(const QVariant& value) const;
  QVariant convertVariantFromDBData(const QVariant& dbdata) const;
  QVariant encaps(const char *ts, const QString& data) const;
  QVariant encaps(const char *ts, const QByteArray& data) const;
  QVariant decaps(const QString& string) const;
  QVariant decaps(const QByteArray& data) const;

  /** Inserts columns into datatable that don't exist for each extra registered property */
  bool ensureDataTableColumnsExist();

  /** Initializes a fresh database. \c datatablename should NOT contain the leading
   * \c "t_" prefix. */
  static bool initFreshDatabase(QSqlDatabase db, const QString& datatablename);
  /** Creates and initializes a fresh data table. It should not yet exist. \c datatablename should
   * NOT contain the leading \c "t_" prefix. */
  static bool createFreshDataTable(QSqlDatabase db, const QString& datatablename);

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
