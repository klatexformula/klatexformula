/***************************************************************************
 *   file klflib.cpp
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

#include <QDebug>
#include <QBuffer>
#include <QByteArray>
#include <QDataStream>
#include <QMessageBox>
#include <QSqlRecord>
#include <QSqlDatabase>
#include <QSqlQuery>
#include <QSqlError>

#include "klflib.h"


KLFLibEntry::KLFLibEntry(const QString& latex, const QDateTime& dt, const QImage& preview,
			 const QString& category, const QString& tags, const KLFStyle& style)
  : KLFPropertizedObject("KLFLibEntry")
{
  initRegisteredProperties();
  setLatex(latex);
  setDateTime(dt);
  setPreview(preview);
  setCategory(category);
  setTags(tags);
  setStyle(style);
}
KLFLibEntry::KLFLibEntry(const KLFLibEntry& copy)
  : KLFPropertizedObject("KLFLibEntry")
{
  initRegisteredProperties();
  setAllProperties(copy.allProperties());
}
KLFLibEntry::~KLFLibEntry()
{
}



// private
void KLFLibEntry::initRegisteredProperties()
{
  static bool first_run = true;
  if ( ! first_run )
    return;
  first_run = false;

  registerBuiltInProperty(Latex, "Latex");
  registerBuiltInProperty(DateTime, "DateTime");
  registerBuiltInProperty(Preview, "Preview");
  registerBuiltInProperty(Category, "Category");
  registerBuiltInProperty(Tags, "Tags");
  registerBuiltInProperty(Style, "Style");
}


// ---------------------------------------------------


KLFLibResourceEngine::KLFLibResourceEngine(const QUrl& url, QObject *parent)
  : QObject(parent), pUrl(url)
{
}
KLFLibResourceEngine::~KLFLibResourceEngine()
{
}



// ---------------------------------------------------


QList<KLFAbstractLibEngineFactory*> KLFAbstractLibEngineFactory::pRegisteredFactories =
	 QList<KLFAbstractLibEngineFactory*>();

KLFAbstractLibEngineFactory::KLFAbstractLibEngineFactory(QObject *parent)
  : QObject(parent)
{
  registerFactory(this);
}
KLFAbstractLibEngineFactory::~KLFAbstractLibEngineFactory()
{
  unRegisterFactory(this);
}

bool KLFAbstractLibEngineFactory::canCreateResource() const
{
  return false;
}

QWidget * KLFAbstractLibEngineFactory::createPromptParametersWidget(QWidget */*parent*/,
								    Parameters /*defaultparameters*/)
{
  return NULL;
}
KLFAbstractLibEngineFactory::Parameters
/* */ KLFAbstractLibEngineFactory::retrieveParametersFromWidget(QWidget */*parent*/)
{
  return KLFAbstractLibEngineFactory::Parameters();
}
KLFLibResourceEngine *KLFAbstractLibEngineFactory::createResource(const QUrl& /*resourceurl*/,
								  KLFAbstractLibEngineFactory::Parameters
								  /*param*/)
{
  return NULL;
}




KLFAbstractLibEngineFactory *KLFAbstractLibEngineFactory::findFactoryFor(const QString& urlScheme)
{
  int k;
  // walk registered factories, and return the first that supports this scheme.
  for (k = 0; k < pRegisteredFactories.size(); ++k) {
    if (pRegisteredFactories[k]->supportedSchemes().contains(urlScheme))
      return pRegisteredFactories[k];
  }
  // no factory found
  return NULL;
}


void KLFAbstractLibEngineFactory::registerFactory(KLFAbstractLibEngineFactory *factory)
{
  if (pRegisteredFactories.indexOf(factory) != -1)
    return;
  pRegisteredFactories.append(factory);
}

void KLFAbstractLibEngineFactory::unRegisterFactory(KLFAbstractLibEngineFactory *factory)
{
  if (pRegisteredFactories.indexOf(factory) == -1)
    return;
  pRegisteredFactories.removeAll(factory);
}




// ---------------------------------------------------



static QByteArray image_data(const QImage& img, const char *format)
{
  QByteArray data;
  QBuffer buf(&data);
  buf.open(QIODevice::WriteOnly);
  img.save(&buf, format);
  buf.close();
  return data;
}

template<class T>
static QByteArray metatype_to_data(const T& object)
{
  QByteArray data;
  {
    QDataStream stream(&data, QIODevice::WriteOnly);
    stream << object;
    // force close of buffer in destroying stream
  }
  return data;
}

template<class T>
static T metatype_from_data(const QByteArray& data)
{
  T object;
  QDataStream stream(data);
  stream >> object;
  return object;
}





// static
KLFLibDBEngine * KLFLibDBEngine::openUrl(const QUrl& url, QObject *parent)
{
  QSqlDatabase db;
  if (url.scheme() == "klf+sqlite") {
    db = QSqlDatabase::addDatabase("QSQLITE");
    db.setDatabaseName(url.path());
  } else {
    qWarning("KLFLibDBEngine::KLFLibDBEngine: bad url scheme in URL\n\t%s",
	     qPrintable(url.toString()));
    return NULL;
  }

  QString datatablename = url.encodedQueryItemValue("dataTableName");
  if (datatablename.isEmpty())
    datatablename = "klfentries";

  if (!db.open()) {
    QMessageBox::critical(0, tr("Error"),
			  tr("Unable to open library file %1 (engine: %2).")
			  .arg(url.path(), db.driverName()), QMessageBox::Ok);
    return NULL;
  }

  return new KLFLibDBEngine(db, datatablename, true /*autoDisconnect*/, url, parent);
}

// private
KLFLibDBEngine::KLFLibDBEngine(const QSqlDatabase& db, const QString& tablename, bool autodisconnect,
			       const QUrl& url, QObject *parent)
  : KLFLibResourceEngine(url, parent), pAutoDisconnectDB(autodisconnect), pDataTableName(tablename)
{
  setDatabase(db);
}

KLFLibDBEngine::~KLFLibDBEngine()
{
  if (pAutoDisconnectDB) {
    pDB.close();
    QSqlDatabase::removeDatabase(pDB.connectionName());
  }
}

bool KLFLibDBEngine::validDatabase() const
{
  return pDB.isOpen();
}

void KLFLibDBEngine::setDatabase(QSqlDatabase db)
{
  pDB = db;
}



// private
QMap<int,int> KLFLibDBEngine::detectEntryColumns(const QSqlQuery& q)
{
  const QSqlRecord rec = q.record();
  QMap<int,int> cols;
  KLFLibEntry e; // dummy entry to test property names
  QList<int> props = e.registeredPropertyIdList();
  int k;
  for (k = 0; k < props.size(); ++k) {
    cols[props[k]] = rec.indexOf(e.propertyNameForId(props[k]));
  }
  return cols;
}
// private
KLFLibEntry KLFLibDBEngine::readEntry(const QSqlQuery& q, QMap<int,int> col)
{
  // and actually read the result and return it
  KLFLibEntry entry;
  entry.setLatex(q.value(col[KLFLibEntry::Latex]).toString());
  entry.setDateTime(QDateTime::fromTime_t(q.value(col[KLFLibEntry::DateTime]).toInt()));
  QImage img; img.loadFromData(q.value(col[KLFLibEntry::Preview]).toByteArray());
  entry.setPreview(img);
  entry.setCategory(q.value(col[KLFLibEntry::Category]).toString());
  entry.setTags(q.value(col[KLFLibEntry::Tags]).toString());
  entry.setStyle(metatype_from_data<KLFStyle>(q.value(col[KLFLibEntry::Style]).toByteArray()));
  return entry;
}



KLFLibEntry KLFLibDBEngine::entry(entryId id)
{
  if ( ! validDatabase() )
    return KLFLibEntry();

  QSqlQuery q = QSqlQuery(pDB);
  q.prepare("SELECT * FROM klfentries WHERE id = ?");
  q.addBindValue(id);
  q.exec();
  q.next();

  if (q.size() == 0) {
    qWarning("KLFLibDBEngine::entry: id=%d cannot be found!", id);
    return KLFLibEntry();
  }
  if (q.size() != 1) {
    qWarning("KLFLibDBEngine::entry: %d results returned for id %d!", q.size(), id);
    return KLFLibEntry();
  }

  // read the result and return it
  KLFLibEntry e = readEntry(q, detectEntryColumns(q));
  q.finish();
  return e;
}


QList<KLFLibResourceEngine::KLFLibEntryWithId> KLFLibDBEngine::allEntries()
{
  if ( ! validDatabase() )
    return QList<KLFLibResourceEngine::KLFLibEntryWithId>();

  QSqlQuery q = QSqlQuery(pDB);
  q.prepare("SELECT * FROM klfentries");
  q.setForwardOnly(true);
  q.exec();

  QList<KLFLibEntryWithId> entryList;

  int colId = q.record().indexOf("id");
  QMap<int,int> cols = detectEntryColumns(q);
  while (q.next()) {
    KLFLibEntryWithId e;
    e.id = q.value(colId).toInt();
    e.entry = readEntry(q, cols);
    entryList << e;
  }
  q.finish();

  return entryList;
}


// private
QVariant KLFLibDBEngine::convertVariantToDBData(const QVariant& value) const
{
  QVariant data = value;
  // setup data in proper format if needed
  if (data.type() == QVariant::Image)
    data = QVariant::fromValue<QByteArray>(image_data(data.value<QImage>(), "PNG"));
  if (data.type() == QVariant::nameToType("KLFStyle"))
    data = QVariant::fromValue<QByteArray>(metatype_to_data(data.toByteArray()));
  return data;
}

KLFLibResourceEngine::entryId KLFLibDBEngine::insertEntry(const KLFLibEntry& entry)
{
  int k;
  if ( ! validDatabase() )
    return -1;

  QList<int> propids = entry.registeredPropertyIdList();
  QStringList props;
  QStringList questionmarks;
  for (k = 0; k < propids.size(); ++k) {
    props << entry.propertyNameForId(propids[k]);
    questionmarks << "?";
  }

  QSqlQuery q = QSqlQuery(pDB);
  q.prepare("INSERT INTO klfentries (" + props.join(",") + ") "
	    " VALUES (" + questionmarks.join(",") + ")");
  for (k = 0; k < propids.size(); ++k) {
    QVariant data = convertVariantToDBData(entry.property(propids[k]));
    // and add a corresponding bind value for sql query
    q.addBindValue(data);
  }
  q.exec();
  q.finish();

  QVariant v_id = q.lastInsertId();
  if ( ! v_id.isValid() )
    return -1;
  return v_id.toInt();
}


bool KLFLibDBEngine::changeEntry(const QList<entryId>& idlist, const QList<int>& properties,
				 const QList<QVariant>& values)
{
  if ( ! validDatabase() )
    return false;

  if ( properties.size() != values.size() ) {
    qWarning("KLFLibDBEngine::changeEntry(): properties' and values' sizes mismatch!");
    return false;
  }

  KLFLibEntry e; // dummy 
  QStringList updatepairs;
  int k;
  for (k = 0; k < properties.size(); ++k) {
    updatepairs << (e.propertyNameForId(properties[k]) + " = ?");
  }
  QStringList questionmarks_ids;
  for (k = 0; k < idlist.size(); ++k) questionmarks_ids << "?";
  // prepare query
  QSqlQuery q = QSqlQuery(pDB);
  q.prepare("UPDATE klfentries SET " + updatepairs.join(", ") + "  WHERE id IN (" +
	    questionmarks_ids.join(", ") + ")");
  for (k = 0; k < properties.size(); ++k) {
    q.addBindValue(convertVariantToDBData(values[k]));
  }
  for (k = 0; k < idlist.size(); ++k)
    q.addBindValue(idlist[k]);
  bool r = q.exec();
  q.finish();

  qDebug() << "UPDATE: SQL="<<q.lastQuery()<<"; boundvalues="<<q.boundValues().values();
  if ( !r || q.lastError().isValid() ) {
    qWarning() << "SQL UPDATE Error: "<<q.lastError();
    return false;
  }

  qDebug() << "Wrote Entry change to ids "<<idlist;

  return true;
}

bool KLFLibDBEngine::deleteEntry(entryId id)
{
  if ( ! validDatabase() )
    return false;

  QSqlQuery q = QSqlQuery(pDB);
  q.prepare("DELETE FROM klfentries WHERE id = ?");
  q.addBindValue(id);
  bool r = q.exec();
  q.finish();

  if ( !r || q.lastError().isValid())
    return false;   // an error was set

  return true;  // no error
}

/*
// static
bool KLFLibDBEngine::initFreshDatabase(QSqlDatabase db)
{
  if ( ! db.isOpen() )
    return false;

  QStringList sql;
  sql << "DROP TABLE klfentries";
  // the CREATE TABLE statements should be adapted to the database type (sqlite, mysql, pgsql) since
  // syntax is different...
  sql << "CREATE TABLE klfentries (id INTEGER PRIMARY KEY, Latex TEXT, DateTime INTEGER, "
    "       Preview BLOB, Category TEXT, Tags TEXT, Style BLOB)";
  sql << "CREATE TABLE klf_metainfo (id INTEGER PRIMARY KEY, propName TEXT, propValue BLOB)";
  sql << "INSERT INTO klf_metainfo (propName, propValue) VALUES ('klf_version', '" KLF_VERSION_STRING "')";
  sql << "INSERT INTO klf_metainfo (propName, propValue) VALUES ('klf_dbversion', '" KLF_DB_VERSION "')";

  int k;
  for (k = 0; k < sql.size(); ++k) {
    QSqlQuery query(sql[k], db);
    if (query.lastError().isValid()) {
      qWarning("initFreshDatabase(): SQL Error: %s\nSQL=%s", qPrintable(query.lastError().text()),
	       qPrintable(sql[k]));
    }
    return false;
  }

  return true;
}
*/


// ------------------------------------

KLFLibDBEngineFactory::KLFLibDBEngineFactory(QObject *parent)
  : KLFAbstractLibEngineFactory(parent)
{
}

QStringList KLFLibDBEngineFactory::supportedSchemes() const
{
  return QStringList() << QLatin1String("klf+sqlite");
}

KLFLibResourceEngine *KLFLibDBEngineFactory::openResource(const QUrl& location, QObject *parent)
{
  return KLFLibDBEngine::openUrl(location, parent);
}


