/***************************************************************************
 *   file klflibdbengine.cpp
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
#include <QString>
#include <QBuffer>
#include <QFile>
#include <QByteArray>
#include <QDataStream>
#include <QMessageBox>
#include <QSqlRecord>
#include <QSqlDatabase>
#include <QSqlQuery>
#include <QSqlError>

#include "klfmain.h" // KLF_DB_VERSION
#include "klflib.h"
#include "klflibdbengine.h"

#include "klflibdbengine_p.h"

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


// --------------------------------------------



KLFLibDBConnectionClassUser::KLFLibDBConnectionClassUser()
{
  pAutoDisconnectDB = false;
  pDBConnectionName = QString();
}
KLFLibDBConnectionClassUser::~KLFLibDBConnectionClassUser()
{
  if (pAutoDisconnectDB)
    QSqlDatabase::removeDatabase(pDBConnectionName);
}



// --------------------------------------------


// static
KLFLibDBEngine * KLFLibDBEngine::openUrl(const QUrl& url, QObject *parent)
{
  bool accessshared = false;

  QString datatablename = url.queryItemValue("dataTableName");
  if (datatablename.isEmpty()) {
    return NULL;
  }

  QSqlDatabase db;
  if (url.scheme() == "klf+sqlite") {
    QUrl dburl = url;
    dburl.removeAllQueryItems("dataTableName");
    dburl.removeAllQueryItems("klfReadOnly");
    accessshared = false;
    QString dburlstr = dburl.toString();
    db = QSqlDatabase::database(dburlstr);
    if ( ! db.isValid() ) {
      // connection not already open
      db = QSqlDatabase::addDatabase("QSQLITE", dburl.toString());
      db.setDatabaseName(dburl.path());
      if ( !db.open() || db.lastError().isValid() ) {
	QMessageBox::critical(0, tr("Error"),
			      tr("Unable to open library file %1 (engine: %2).\nError: %3")
			      .arg(url.path(), db.driverName(), db.lastError().text()), QMessageBox::Ok);
	return NULL;
      }
    }
  } else {
    qWarning("KLFLibDBEngine::openUrl: bad url scheme in URL\n\t%s",
	     qPrintable(url.toString()));
    return NULL;
  }

  return new KLFLibDBEngine(db, datatablename, true /*autoDisconnect*/, url, accessshared, parent);
}

// static
KLFLibDBEngine * KLFLibDBEngine::createSqlite(const QString& fileName, const QString& dtablename,
					      QObject *parent)
{
  QString datatablename = dtablename;

  bool r;

  if (QFile::exists(fileName)) {
    // fail; we want to _CREATE_ a database. use openUrl() to open an existing DB.
    return NULL;
  }
  QUrl url = QUrl::fromLocalFile(fileName);
  url.setScheme("klf+sqlite");

  if (datatablename.isEmpty())
    datatablename = "klfentries";
  
  url.addQueryItem("dataTableName", datatablename);
  
  QUrl dburl = url;
  dburl.removeAllQueryItems("dataTableName");
  dburl.removeAllQueryItems("klfReadOnly");
  QString dburlstr = dburl.toString();
  QSqlDatabase db = QSqlDatabase::database(dburlstr);
  if (!db.isValid()) {
    db = QSqlDatabase::addDatabase("QSQLITE", dburlstr);
    db.setDatabaseName(url.path());
    r = db.open(); // go open (here create) the DB
    if ( !r || db.lastError().isValid() ) {
      QMessageBox::critical(0, tr("Error"),
			    tr("Unable to create library file %1 (SQLITE database):\n"
			       "%2")
			    .arg(url.path(), db.lastError().text()), QMessageBox::Ok);
      return NULL;
    }
  }

  r = initFreshDatabase(db, datatablename);
  if ( !r ) {
    QMessageBox::critical(0, tr("Error"),
			  tr("Unable to initialize the SQLITE database file %1!").arg(url.path()));
    return NULL;
  }

  return new KLFLibDBEngine(db, datatablename, true /*autoDisconnect*/, url, false, parent);
}

// private
KLFLibDBEngine::KLFLibDBEngine(const QSqlDatabase& db, const QString& tablename, bool autodisconnect,
			       const QUrl& url, bool accessshared, QObject *parent)
  : KLFLibResourceSimpleEngine(url, FeatureReadOnly|FeatureLocked, parent)
{
  pDataTableName = "t_"+tablename;

  pAutoDisconnectDB = autodisconnect;

  // load some read-only properties in memory (these are NOT stored in the DB)
  KLFPropertizedObject::setProperty(PropAccessShared, accessshared);

  setDatabase(db);
  readResourceProperty(-1); // read all resource properties from DB

  if ( ! db.tables().contains("t_"+tablename) ) {
    if (isReadOnly() || ((supportedFeatureFlags()&FeatureLocked) && locked())) {
      // cannot create given dataTableName... will generate an error
      qWarning()<<"KLFLibDBEngine::KLFLibDBEngine: cannot create data table "<<pDataTableName
		<<"in read-only/locked mode!";
    } else {
      createFreshDataTable(db, tablename);
    }
  }

  KLFLibDBEnginePropertyChangeNotifier *dbNotifier = dbPropertyNotifierInstance(db.connectionName());
  connect(dbNotifier, SIGNAL(resourcePropertyChanged(int)),
	  this, SLOT(resourcePropertyUpdate(int)));
  dbNotifier->ref();
}

KLFLibDBEngine::~KLFLibDBEngine()
{
  pDBConnectionName = pDB.connectionName();
  KLFLibDBEnginePropertyChangeNotifier *dbNotifier = dbPropertyNotifierInstance(pDBConnectionName);
  if (dbNotifier->deRef() && pAutoDisconnectDB) {
    pDB.close();
    pAutoDisconnectDB = true;
  } else {
    pAutoDisconnectDB = false;
  }
}

bool KLFLibDBEngine::canModifyData(ModifyType mt) const
{
  if ( !KLFLibResourceEngine::canModifyData(mt) )
    return false;

  /** \todo TODO: check if file is writable (SQLITE3), if permissions on the database
   * are granted (MySQL, PgSQL, etc.) */
  return true;
}

bool KLFLibDBEngine::canModifyProp(int propId) const
{
  return KLFLibResourceEngine::canModifyProp(propId);
}
bool KLFLibDBEngine::canRegisterProperty(const QString& /*propName*/) const
{
  // we can register properties if we can write them
  return canModifyProp(-1);
}

bool KLFLibDBEngine::validDatabase() const
{
  return pDB.isOpen();
}

void KLFLibDBEngine::setDatabase(const QSqlDatabase& db)
{
  pDB = db;
}

// private
bool KLFLibDBEngine::saveResourceProperty(int propId, const QVariant& value)
{
  QString propName = propertyNameForId(propId);
  if ( propName.isEmpty() )
    return false;

  {
    QSqlQuery q = QSqlQuery(pDB);
    q.prepare("DELETE FROM klf_properties WHERE name = ?");
    q.addBindValue(propName);
    bool r = q.exec();
    if ( !r || q.lastError().isValid() ) {
      qWarning()<<"KLFLibDBEngine::setRes.Property("<<propId<<","<<value<<"): can't DELETE!\n\t"
		<<q.lastError().text()<<"\n\tSQL="<<q.lastQuery();
      return false;
    }
  }
  {
    QSqlQuery q = QSqlQuery(pDB);
    q.prepare("INSERT INTO klf_properties (name,value) VALUES (?,?)");
    q.bindValue(0, propName);
    q.bindValue(1, convertVariantToDBData(value));
    if ( ! q.exec() || q.lastError().isValid() ) {
      qWarning()<<"KLFLibDBEngine::setRes.Property("<<propId<<","<<value<<"): can't INSERT!\n\t"
		<<q.lastError().text()<<"\n\tSQL="<<q.lastQuery();
      return false;
    }
  }
  KLFPropertizedObject::setProperty(propId, value);
  dbPropertyNotifierInstance(pDB.connectionName())->notifyResourcePropertyChanged(propId);
  // will be emitted by own slot from above call
  //  emit resourcePropertyChanged(propId);
  return true;
}

void KLFLibDBEngine::resourcePropertyUpdate(int propId)
{
  readResourceProperty(propId);
}
void KLFLibDBEngine::readResourceProperty(int propId)
{
  QString sqlstr = "SELECT name,value FROM klf_properties";
  QString propName;
  if (propId >= 0) {
    if (!propertyIdRegistered(propId)) {
      qWarning()<<"Can't read un-registered resource property "<<propId<<" !";
      return;
    }
    sqlstr += " WHERE name = ?";
    propName = propertyNameForId(propId);
  }
  // read resource properties from database
  QSqlQuery q = QSqlQuery(pDB);
  q.prepare("SELECT name,value FROM klf_properties");
  q.exec();
  while (q.next()) {
    QString propname = q.value(0).toString();
    QVariant propvalue = convertVariantFromDBData(q.value(1));
    qDebug()<<"Setting property `"<<propname<<"' to "<<propvalue<<"";
    if (!propertyNameRegistered(propname)) {
      if (!canRegisterProperty(propname))
	continue;
      KLFPropertizedObject::registerProperty(propname);
    }
    int propId = propertyIdForName(propname);
    KLFPropertizedObject::setProperty(propId, propvalue);
    emit resourcePropertyChanged(propId);
  }
}




// private
QStringList KLFLibDBEngine::detectEntryColumns(const QSqlQuery& q)
{
  const QSqlRecord rec = q.record();
  QStringList cols;
  KLFLibEntry dummy; // to get prop ID and possibly register property
  int k;
  for (k = 0; k < rec.count(); ++k) {
    QString propName = rec.fieldName(k);
    int propId = dummy.propertyIdForName(propName);
    if (propId < 0 && propName != "id") { // don't register field "id"
      qDebug()<<"Registering property "<<propName;
      dummy.setEntryProperty(propName, QVariant()); // register property.
    }

    cols << propName;
  }
  return cols;
}
// private
KLFLibEntry KLFLibDBEngine::readEntry(const QSqlQuery& q, const QStringList& cols)
{
  // and actually read the result and return it
  KLFLibEntry entry;
  int k;
  for (k = 0; k < cols.size(); ++k) {
    QVariant v = q.value(k);
    if (cols[k] == "id")
      continue;
    /** \bug DEBUG/TODO/BUG .... HERE REMOVE THE FOLLOWING SPECIAL CASES LATER (these
     * are needed only for my old test db's... */
    bool hasTypeInfo = (v.toByteArray()[0] == '[');
    if (!hasTypeInfo && cols[k] == "DateTime")
      entry.setDateTime(QDateTime::fromString(q.value(k).toString(), Qt::ISODate));
    else if (!hasTypeInfo && cols[k] == "Preview") {
      QImage img; img.loadFromData(q.value(k).toByteArray());
      entry.setPreview(img);
    } else if (!hasTypeInfo && cols[k] == "KLFStyle") {
      entry.setStyle(metatype_from_data<KLFStyle>(q.value(k).toByteArray()));
    } else {
      entry.setEntryProperty(cols[k], convertVariantFromDBData(q.value(k)));
    }
  }
  return entry;
}



KLFLibEntry KLFLibDBEngine::entry(entryId id)
{
  if ( ! validDatabase() )
    return KLFLibEntry();

  QSqlQuery q = QSqlQuery(pDB);
  q.prepare("SELECT * FROM `"+pDataTableName+"` WHERE id = ?");
  q.addBindValue(id);
  bool r = q.exec();

  if ( !r || q.lastError().isValid() || q.size() == 0) {
    qWarning("KLFLibDBEngine::entry: id=%d cannot be found!\n"
	     "SQL Error (?): %s", id, qPrintable(q.lastError().text()));
    return KLFLibEntry();
  }
  //   if (q.size() != 1) {
  //     qWarning("KLFLibDBEngine::entry: %d results returned for id %d!", q.size(), id);
  //     return KLFLibEntry();
  //   }
  if ( ! q.next() ) {
    qWarning("KLFLibDBEngine::entry(): no entry available!\n"
	     "SQL=\"%s\" (?=%d)", qPrintable(q.lastQuery()), id);
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
  q.prepare("SELECT * FROM `"+pDataTableName+"`");
  q.setForwardOnly(true);
  q.exec();

  QList<KLFLibEntryWithId> entryList;

  int colId = q.record().indexOf("id");
  QStringList cols = detectEntryColumns(q);
  while (q.next()) {
    KLFLibEntryWithId e;
    e.id = q.value(colId).toInt();
    e.entry = readEntry(q, cols);
    entryList << e;
  }

  return entryList;
}


// private
QVariant KLFLibDBEngine::convertVariantToDBData(const QVariant& value) const
{
  // setup data in proper format if needed

  if (!value.isValid())
    return QVariant();

  int t = value.type();
  const char *ts = value.typeName();
  if (t == QVariant::Int || t == QVariant::UInt || t == QVariant::LongLong || t == QVariant::ULongLong ||
      t == QVariant::Double)
    return value; // value is OK

  // these types are to be converted to string
  if (t == QVariant::String)
    return encaps(ts, value.toString());
  if (t == QVariant::Bool)
    return encaps(ts, value.toBool() ? QString("true") : QString("false"));

  // these types are to be converted by byte array
  if (t == QVariant::ByteArray)
    return encaps(ts, value.toByteArray());
  if (t == QVariant::DateTime)
    return encaps(ts, value.value<QDateTime>().toString(Qt::ISODate).toLatin1());
  if (t == QVariant::Image)
    return encaps(ts, image_data(value.value<QImage>(), "PNG"));
  if (t == QVariant::Point) {
    QPoint p = value.value<QPoint>();
    return encaps(ts, QByteArray()+"("+QString::number(p.x()).toLatin1()+","
		  +QString::number(p.y()).toLatin1()+")");
  }

  // any other case: convert metatype to QByteArray.
  QByteArray valuedata;
  { QDataStream stream(&valuedata, QIODevice::WriteOnly);
    stream.setVersion(QDataStream::Qt_4_4);
    stream << value; }
  return encaps(ts, valuedata);
}

QVariant KLFLibDBEngine::encaps(const char *ts, const QString& data) const
{
  return QVariant::fromValue<QString>(QString("[")+ts+"]"+data);
}
QVariant KLFLibDBEngine::encaps(const char *ts, const QByteArray& data) const
{
  QByteArray edata;
  edata.append("[");
  edata.append(ts);
  edata.append("]");
  edata.append(data);
  return QVariant::fromValue<QByteArray>(edata);
}
QVariant KLFLibDBEngine::convertVariantFromDBData(const QVariant& dbdata) const
{
  if ( !dbdata.isValid() )
    return QVariant();

  int t = dbdata.type();
  if (t == QVariant::Int || t == QVariant::UInt || t == QVariant::LongLong || t == QVariant::ULongLong ||
      t == QVariant::Double)
    return dbdata; // value is OK

  if (t == QVariant::String)
    return decaps(dbdata.toString());
  if (t == QVariant::ByteArray)
    return decaps(dbdata.toByteArray());

  qWarning()<<"Unexpected DB data variant found: "<<dbdata;
  return QVariant();
}
QVariant KLFLibDBEngine::decaps(const QString& sdata) const
{
  return decaps(sdata.toUtf8());
}
QVariant KLFLibDBEngine::decaps(const QByteArray& data) const
{
  //  qDebug()<<"decaps(): "<<data;
  int k;
  if (!data.size())
    return QVariant();
  if (data[0] != '[')
    return QVariant::fromValue<QString>(QString::fromUtf8(data));
  for (k = 1; k < data.size() && data[k] != ']'; ++k)  ;
  if (k >= data.size()) {
    qWarning()<<"KLFLibDBEngine::decaps(QB.A.): bad data:"<<data;
    return QVariant();
  }
  const QByteArray typenam = data.mid(1, k-1);
  const QByteArray valuedata = data.mid(k+1);

  if (typenam == "bool") {
    QString svaluedata = QString::fromUtf8(valuedata);
    return QVariant::fromValue<bool>(svaluedata[0].toLower() == 't' ||
				     svaluedata[0].toLower() == 'y' ||
				     svaluedata.toInt() != 0);
  }
  if (typenam == "QString")
    return QVariant::fromValue<QString>(QString::fromUtf8(valuedata));
  if (typenam == "QByteArray")
    return QVariant::fromValue<QByteArray>(valuedata);
  if (typenam == "QDateTime")
    return QDateTime::fromString(QString::fromLatin1(valuedata), Qt::ISODate);
  if (typenam == "QPoint") {
    static QRegExp rx("\\s*\\(\\s*(-?\\d+)\\s*,\\s*(-?\\d+)\\s*\\)");
    if ( !rx.exactMatch(valuedata) ) {
      qDebug()<<"nomatch!";
      return QPoint(-1,-1);
    }
    QStringList cap = rx.capturedTexts();
    if (cap.size() < 3) {
      qDebug()<<"Bad cap "<<cap;
      return QPoint(-1,-1);
    }
    QPoint p(cap[1].toInt(), cap[2].toInt());
    return p;
  }
  if (typenam == "QImage") {
    QImage img;
    img.loadFromData(valuedata);
    return QVariant::fromValue<QImage>(img);
  }

  // OTHERWISE, load from datastream save:

  QVariant value;
  { QDataStream stream(valuedata);
    stream.setVersion(QDataStream::Qt_4_4);
    stream >> value; }
  return value;
}


bool KLFLibDBEngine::ensureDataTableColumnsExist()
{
  KLFLibEntry dummy;
  QSqlRecord rec = pDB.record(pDataTableName);
  QStringList propNameList = dummy.registeredPropertyNameList();
  int k;
  bool failed = false;
  for (k = 0; k < propNameList.size(); ++k) {
    if (rec.contains(propNameList[k]))
      continue;
    QSqlQuery sql(pDB);
    sql.prepare("ALTER TABLE `"+pDataTableName+"` ADD COLUMN "+propNameList[k]+" BLOB");
    bool r = sql.exec();
    if (!r || sql.lastError().isValid()) {
      qWarning()<<"KLFLibDBEngine::ensureDataTableColumnsExist: Can't add column "<<propNameList[k]<<"!\n"
		<<sql.lastError().text()<<" SQL="<<sql.lastQuery();
      failed = true;
    }
  }

  return !failed;
}


// --


QList<KLFLibResourceEngine::entryId> KLFLibDBEngine::insertEntries(const KLFLibEntryList& entrylist)
{
  int k, j;
  if ( ! validDatabase() )
    return QList<entryId>();
  if ( entrylist.size() == 0 )
    return QList<entryId>();

  if ( !canModifyData(InsertData) )
    return QList<entryId>();

  KLFLibEntry e; // dummy object to test for properties
  QList<int> propids = e.registeredPropertyIdList();
  QStringList props;
  QStringList questionmarks;
  for (k = 0; k < propids.size(); ++k) {
    props << e.propertyNameForId(propids[k]);
    questionmarks << "?";
  }

  QList<entryId> insertedIds;

  ensureDataTableColumnsExist();

  QSqlQuery q = QSqlQuery(pDB);
  q.prepare("INSERT INTO `" + pDataTableName + "` (" + props.join(",") + ") "
	    " VALUES (" + questionmarks.join(",") + ")");
  qDebug()<<"INSERT query: "<<q.lastQuery();
  // now loop all entries, and exec the query with appropriate bound values
  for (j = 0; j < entrylist.size(); ++j) {
    //    qDebug()<<"New entry to insert.";
    for (k = 0; k < propids.size(); ++k) {
      QVariant data = convertVariantToDBData(entrylist[j].property(propids[k]));
      // and add a corresponding bind value for sql query
      qDebug()<<"Binding value "<<k<<": "<<data;
      q.bindValue(k, data);
    }
    // and exec the query with these bound values
    bool r = q.exec();
    if ( ! r || q.lastError().isValid() ) {
      qWarning()<<"INSERT failed! SQL Error: "<<q.lastError().text()<<"\n\tSQL="<<q.lastQuery();
      insertedIds << -1;
    } else {
      QVariant v_id = q.lastInsertId();
      if ( ! v_id.isValid() )
	insertedIds << -2;
      else
	insertedIds << v_id.toInt();
    }
  }

  emit dataChanged(insertedIds);
  return insertedIds;
}


bool KLFLibDBEngine::changeEntries(const QList<entryId>& idlist, const QList<int>& properties,
				   const QList<QVariant>& values)
{
  if ( ! validDatabase() )
    return false;
  if ( ! canModifyData(ChangeData) )
    return false;

  if ( properties.size() != values.size() ) {
    qWarning("KLFLibDBEngine::changeEntry(): properties' and values' sizes mismatch!");
    return false;
  }

  if ( idlist.size() == 0 )
    return true; // no items to change

  qDebug()<<"KLFLibDBEngine::changeEntries: funcional tests passed; idlist="<<idlist<<" props="
	  <<properties<<" vals="<<values;

  ensureDataTableColumnsExist();

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
  q.prepare("UPDATE `"+pDataTableName+"` SET " + updatepairs.join(", ") + "  WHERE id IN (" +
	    questionmarks_ids.join(", ") + ")");
  for (k = 0; k < properties.size(); ++k) {
    q.addBindValue(convertVariantToDBData(values[k]));
  }
  for (k = 0; k < idlist.size(); ++k)
    q.addBindValue(idlist[k]);
  bool r = q.exec();

  //  qDebug() << "UPDATE: SQL="<<q.lastQuery()<<"; boundvalues="<<q.boundValues().values();
  if ( !r || q.lastError().isValid() ) {
    qWarning() << "SQL UPDATE Error: "<<q.lastError()<<"\nWith SQL="<<q.lastQuery()
	       <<";\n and bound values="<<q.boundValues();
    return false;
  }

  //  qDebug() << "Wrote Entry change to ids "<<idlist;
  emit dataChanged(idlist);
  return true;
}

bool KLFLibDBEngine::deleteEntries(const QList<entryId>& idlist)
{
  if ( ! validDatabase() )
    return false;
  if (idlist.size() == 0)
    return true; // nothing to do
  if ( ! canModifyData(DeleteData) )
    return false;

  int k;
  QStringList qmarks_ids;
  for (k = 0; k < idlist.size(); ++k)
    qmarks_ids << "?";

  QSqlQuery q = QSqlQuery(pDB);
  q.prepare("DELETE FROM `"+pDataTableName+"` WHERE id IN ("+qmarks_ids.join(", ")+")");
  for (k = 0; k < idlist.size(); ++k)
    q.addBindValue(idlist[k]);
  bool r = q.exec();
  q.finish();

  emit dataChanged(idlist);

  if ( !r || q.lastError().isValid())
    return false;   // an error was set

  return true;  // no error
}

bool KLFLibDBEngine::saveAs(const QUrl& newPath)
{
  if (newPath.scheme() != "klf+sqlite") {
    qWarning()<<"KLFLibDBEngine::saveAs("<<newPath<<"): Bad scheme!";
    return false;
  }
  if (!newPath.host().isEmpty()) {
    qWarning()<<"KLFLibDBEngine::saveAs("<<newPath<<"): Expected empty host!";
    return false;
  }
  return QFile::copy(url().path(), newPath.path());
}

// static
bool KLFLibDBEngine::initFreshDatabase(QSqlDatabase db, const QString& dtname)
{
  if ( ! db.isOpen() )
    return false;

  // the CREATE TABLE statements should be adapted to the database type (sqlite, mysql, pgsql) since
  // syntax is slightly different...
  QStringList sql;
  sql << "CREATE TABLE klf_properties (id INTEGER PRIMARY KEY, name TEXT, value BLOB)";
  sql << "INSERT INTO klf_properties (name, value) VALUES ('Title', 'New Resource')";
  sql << "INSERT INTO klf_properties (name, value) VALUES ('Locked', 'false')";
  sql << "CREATE TABLE klf_dbmetainfo (id INTEGER PRIMARY KEY, name TEXT, value BLOB)";
  sql << "INSERT INTO klf_dbmetainfo (name, value) VALUES ('klf_version', '" KLF_VERSION_STRING "')";
  sql << "INSERT INTO klf_dbmetainfo (name, value) VALUES ('klf_dbversion', '"+
    QString::number(KLF_DB_VERSION)+"')";

  int k;
  for (k = 0; k < sql.size(); ++k) {
    QSqlQuery query(db);
    query.prepare(sql[k]);
    bool r = query.exec();
    if ( !r || query.lastError().isValid() ) {
      qWarning()<<"initFreshDatabase(): SQL Error: "<<query.lastError().text()<<"\n"
		<<"SQL="<<sql[k];
      return false;
    }
  }

  return createFreshDataTable(db, dtname);
}
// static
bool KLFLibDBEngine::createFreshDataTable(QSqlDatabase db, const QString& dtname)
{
  QString datatablename = "t_"+dtname;
  if ( ! db.isOpen() )
    return false;

  QSqlQuery query(db);
  query.prepare(QString("")+
		"CREATE TABLE `"+datatablename+"` (id INTEGER PRIMARY KEY, Latex TEXT, DateTime TEXT, "
		"       Preview BLOB, Category TEXT, Tags TEXT, Style BLOB)");
  bool r = query.exec();
  if ( !r || query.lastError().isValid() ) {
    qWarning()<<"createFreshDataTable(): SQL Error: "<<query.lastError().text()<<"\n"
	      <<"SQL="<<query.lastQuery();
    return false;
  }

  return true;
}



QStringList KLFLibDBEngine::getDataTableNames(const QUrl& url)
{
  KLFLibDBEngine *resource = openUrl(url, NULL);
  QStringList tables = resource->pDB.tables(QSql::Tables);
  delete resource;
  // filter out t_<tablename> tables
  int k;
  QStringList dataTableNames;
  for (k = 0; k < tables.size(); ++k)
    if (tables[k].startsWith("t_"))
      dataTableNames << tables[k].mid(2);
  return dataTableNames;
}



QMap<QString,KLFLibDBEnginePropertyChangeNotifier*> KLFLibDBEngine::pDBPropertyNotifiers
/* */ = QMap<QString,KLFLibDBEnginePropertyChangeNotifier*>();

// static
KLFLibDBEnginePropertyChangeNotifier *KLFLibDBEngine::dbPropertyNotifierInstance(const QString& dbname)
{
  if (!pDBPropertyNotifiers.contains(dbname))
    pDBPropertyNotifiers[dbname] = new KLFLibDBEnginePropertyChangeNotifier(dbname, qApp);
  return pDBPropertyNotifiers[dbname];
}

// ------------------------------------


KLFLibDBEngineFactory::KLFLibDBEngineFactory(QObject *parent)
  : KLFLibEngineFactory(parent)
{
}

QStringList KLFLibDBEngineFactory::supportedSchemes() const
{
  return QStringList() << QLatin1String("klf+sqlite");
}

QString KLFLibDBEngineFactory::schemeTitle(const QString& scheme) const
{
  if (scheme == QLatin1String("klf+sqlite"))
    return tr("Local Library File");
  return QString();
}


KLFLibResourceEngine *KLFLibDBEngineFactory::openResource(const QUrl& location, QObject *parent)
{
  return KLFLibDBEngine::openUrl(location, parent);
}

QWidget * KLFLibDBEngineFactory::createPromptUrlWidget(QWidget *parent, const QString& scheme,
						       QUrl defaultlocation)
{
  if (scheme == QLatin1String("klf+sqlite")) {
    KLFLibSqliteOpenWidget *w = new KLFLibSqliteOpenWidget(parent);
    w->setUrl(defaultlocation);
    return w;
  }
  return NULL;
}

QUrl KLFLibDBEngineFactory::retrieveUrlFromWidget(const QString& scheme, QWidget *widget)
{
  if (scheme == "klf+sqlite") {
    if (widget == NULL || !widget->inherits("KLFLibSqliteOpenWidget")) {
      qWarning("KLFLibDBEngineFactory::retrieveUrlFromWidget(): Bad Widget provided!");
      return QUrl();
    }
    return qobject_cast<KLFLibSqliteOpenWidget*>(widget)->url();
  } else {
    qWarning()<<"Bad scheme: "<<scheme;
    return QUrl();
  }
}


QWidget *KLFLibDBEngineFactory::createPromptCreateParametersWidget(QWidget *parent,
								   const QString& scheme,
								   const Parameters& defaultparameters)
{
  if (scheme == QLatin1String("klf+sqlite")) {
    KLFLibSqliteCreateWidget *w = new KLFLibSqliteCreateWidget(parent);
    w->setUrl(defaultparameters["Url"].toUrl());
    return w;
  }
  return NULL;
}

KLFLibEngineFactory::Parameters
/* */ KLFLibDBEngineFactory::retrieveCreateParametersFromWidget(const QString& scheme,
								QWidget *widget)
{
  if (scheme == QLatin1String("klf+sqlite")) {
    if (widget == NULL || !widget->inherits("KLFLibSqliteCreateWidget")) {
      qWarning("KLFLibDBEngineFactory::retrieveUrlFromWidget(): Bad Widget provided!");
      return Parameters();
    }
    KLFLibSqliteCreateWidget *w = qobject_cast<KLFLibSqliteCreateWidget*>(widget);
    Parameters p;
    QString filename = w->fileName();
    if (QFile::exists(filename) && !w->confirmedOverwrite()) {
      QMessageBox::StandardButton result =
	QMessageBox::warning(widget, tr("Overwrite?"),
			     tr("The specified file already exists. Overwrite it?"),
			     QMessageBox::Yes|QMessageBox::No|QMessageBox::Cancel,
			     QMessageBox::No);
      if (result == QMessageBox::No) {
	p["klfRetry"] = true;
	return p;
      } else if (result == QMessageBox::Cancel) {
	return Parameters();
      }
      // remove the previous file, because otherwise KLFLibDBEngine will fail.
      bool r = QFile::remove(filename);
      if ( !r ) {
	QMessageBox::critical(widget, tr("Error"), tr("Failed to overwrite the file %1.")
			      .arg(filename));
	return Parameters();
      }
    }

    p["Filename"] = w->fileName();
    p["dataTableName"] = "klfentries";
    return p;
  }

  return Parameters();
}

KLFLibResourceEngine *KLFLibDBEngineFactory::createResource(const QString& scheme,
							    const Parameters& parameters,
							    QObject *parent)
{
  if (scheme == QLatin1String("klf+sqlite")) {
    if ( !parameters.contains("Filename") || !parameters.contains("dataTableName") ) {
      qWarning()
	<<"KLFLibDBEngineFactory::createResource: bad parameters. They do not contain `Filename' or\n"
	"`dataTableName': "<<parameters;
      return NULL;
    }
    return KLFLibDBEngine::createSqlite(parameters["Filename"].toString(),
					parameters["dataTableName"].toString(), parent);
  }
  return NULL;
}



