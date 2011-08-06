/***************************************************************************
 *   file klflibdbengine.cpp
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

#include <QDebug>
#include <QApplication>  // qApp
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

#include <klfguiutil.h>
#include "klflib.h"
#include "klflibview.h"
#include "klflibdbengine.h"
#include "klflibdbengine_p.h"




/** \page libfmt_klfdb Library Database Format Specification
 *
 * \todo (needs doc..............)
 *
 * - refer to klflibdbengine.cpp
 * - is a real, valid sqlite3 database
 * - table structure:
 *   - klf_dbmetainfo  (id INTEGER PRIMARY KEY, name TEXT, value BLOB) stores database-specific
 *     information
 *      - created by klf version (name=<tt>klf_version</tt>, value=<i>klf-version</i>)
 *      - database version (name=<tt>klf_dbversion</tt>, value=<tt>1</tt>) currently, db version
 *        is <tt>1</tt>.
 *   - klf_subresprops (id INTEGER PRIMARY KEY, pid INTEGER, subresource TEXT, pvalue BLOB) stores
 *     sub-resource properties
 *      - properties are stored with <tt>pid</tt> = sub-property ID (eg.
 *        KLFLibResourceEngine::SubResPropTitle), <tt>subresource</tt> = the sub-resource whose
 *        given property has this given value, <tt>value</tt> = the value of the sub-resource property
 *   - klf_properties (id INTEGER PRIMARY KEY, name TEXT, value BLOB)
 *     stores resource properties
 *     - properties are stored with <tt>name</tt> = resource property name (see KLFPropertizedObject in
 *       klftools)
 *   - t_<i>subresource_name</i> (id INTEGER PRIMARY KEY, Latex TEXT, DateTime TEXT, Preview BLOB,
 *     PreviewSize TEXT, Category TEXT, Tags TEXT, Style BLOB) (possibly more columns as more entry
 *     properties are added) stores the entries themselves. Each column stores a KLFLibEntry property.
 *     - Latex is stored as a string
 *     - DateTime is stored as an integer (QDateTime::toTime_t())
 *     - Preview is stored as PNG data (blob)
 *     - Category and Tags are stored as strings
 *     - PreviewSize is stored as a 64-bit integer in the format <tt>(width << 32) | height</tt>
 *     - Any other property that is integer type (incl. bool) will be stored as an integer
 *     - Any other property will be stored as <tt>[<i>TypeName</i>]</tt> and (possibly binary) data
 *       for that property value. QImage data is stored as PNG (ie. <tt>[QImage]<i>png-data</i></tt>),
 *       QString's as <tt>[QString]<i>the raw string</i></tt> as string format, QByteArray's
 *       as <tt>[QByteArray]<i>the raw data</i></tt> as a blob, QDateTime's as
 *       <tt>[QDateTime]<i>integer-epoch</i></tt> as for the DateTime property, and all other types
 *       as <tt>[<i>TypeName</i>]</tt> and the binary data resulting from a QDataStream save of the
 *       QVariant value (this includes KLFStyle).
 *
 */



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
KLFLibDBEngine * KLFLibDBEngine::openUrl(const QUrl& givenurl, QObject *parent)
{
  bool accessshared = false;

  QUrl url = givenurl;

  if (url.hasQueryItem("klfDefaultSubResource")) {
    QString defaultsubres = url.queryItemValue("klfDefaultSubResource");
    // force lower-case default sub-resource
    url.removeAllQueryItems("klfDefaultSubResource");
    url.addQueryItem("klfDefaultSubResource", defaultsubres.toLower());
  }

  QSqlDatabase db;
  if (url.scheme() == "klf+sqlite") {
    QUrl dburl = url;
    dburl.removeAllQueryItems("klfDefaultSubResource");
    dburl.removeAllQueryItems("klfReadOnly");
    accessshared = false;
    QString dburlstr = dburl.toString();
    QString path = klfUrlLocalFilePath(dburl);
    if (dburlstr.isEmpty() || !QFile::exists(path)) {
      QMessageBox::critical(0, tr("Error"),
			    tr("Database file <b>%1</b> does not exist.").arg(path));
      return NULL;
    }
    db = QSqlDatabase::database(dburlstr);
    if ( ! db.isValid() ) {
      // connection not already open
      db = QSqlDatabase::addDatabase("QSQLITE", dburl.toString());
      db.setDatabaseName(path);
      if ( !db.open() || db.lastError().isValid() ) {
	QMessageBox::critical(0, tr("Error"),
			      tr("Unable to open library file \"%1\" (engine: \"%2\").\nError: %3")
			      .arg(path, db.driverName(), db.lastError().text()), QMessageBox::Ok);
	return NULL;
      }
    }
  } else {
    qWarning("KLFLibDBEngine::openUrl: bad url scheme in URL\n\t%s",
	     qPrintable(url.toString()));
    return NULL;
  }

  return new KLFLibDBEngine(db, true /*autoDisconnect*/, url, accessshared, parent);
}

// static
KLFLibDBEngine * KLFLibDBEngine::createSqlite(const QString& fileName, const QString& sresname,
					      const QString& srestitle, QObject *parent)
{
  KLF_DEBUG_BLOCK(KLF_FUNC_NAME) ;
  klfDbgSt("fileName="<<fileName<<", sresname="<<sresname<<", srestitle="<<srestitle) ;

  QString subresname = sresname;
  QString subrestitle = srestitle;

  bool r;

  if (QFile::exists(fileName)) {
    // fail; we want to _CREATE_ a database. use openUrl() to open an existing DB.
    // To overwrite, erase the file first.
    return NULL;
  }
  QUrl url = QUrl::fromLocalFile(fileName);
  url.setScheme("klf+sqlite");

  QString dburlstr = url.toString();
  QSqlDatabase db = QSqlDatabase::database(dburlstr);
  if (!db.isValid()) {
    db = QSqlDatabase::addDatabase("QSQLITE", dburlstr);
    QString path = klfUrlLocalFilePath(url);
    db.setDatabaseName(path);
    r = db.open(); // go open (here create) the DB
    if ( !r || db.lastError().isValid() ) {
      QMessageBox::critical(0, tr("Error"),
			    tr("Unable to create library file %1 (SQLITE database):\n"
			       "%2")
			    .arg(path, db.lastError().text()), QMessageBox::Ok);
      return NULL;
    }
  }

  if (subresname.isEmpty()) {
    subresname = "table1";
    if (subrestitle.isEmpty()) {
      subrestitle = "Table 1";
    }
  }
  if (subrestitle.isEmpty())
    subrestitle = subresname;

  url.addQueryItem("klfDefaultSubResource", subresname);
  if (subresname.contains("\"")) {
    // SQLite table name cannot contain double-quote char (itself is used to escape the table name!)
    qWarning()<<KLF_FUNC_NAME<<"\" character is not allowed in SQLITE database tables (<-> library sub-resources).";
    return NULL;
  }

  r = initFreshDatabase(db);
  if ( r ) {
    // and create default table
    r = createFreshDataTable(db, subresname);
  }
  if ( !r ) {
    QMessageBox::critical(0, tr("Error"),
			  tr("Unable to initialize the SQLITE database file %1!").arg(url.path()));
    return NULL;
  }

  KLFLibDBEngine *res = new KLFLibDBEngine(db, true /*autoDisconnect*/, url, false, parent);

  r = res->setSubResourceProperty(subresname, SubResPropTitle, subrestitle);
  if ( ! r ) {
    qWarning()<<"Failed to create table named "<<subresname<<"!";
    delete res;
    return NULL;
  }

  return res;
}

// private
KLFLibDBEngine::KLFLibDBEngine(const QSqlDatabase& db, bool autodisconnect,
			       const QUrl& url, bool accessshared, QObject *parent)
  : KLFLibResourceEngine(url, FeatureReadOnly|FeatureLocked|FeatureSubResources
			 |FeatureSubResourceProps, parent)
{
  pAutoDisconnectDB = autodisconnect;

  // load some read-only properties in memory (these are NOT stored in the DB)
  KLFPropertizedObject::setProperty(PropAccessShared, accessshared);

  setDatabase(db);
  readResourceProperty(-1); // read all resource properties from DB

  readDbMetaInfo();
  QStringList subres = subResourceList();
  int k;
  for (k = 0; k < subres.size(); ++k)
    readAvailColumns(subres[k]);

  KLFLibDBEnginePropertyChangeNotifier *dbNotifier = dbPropertyNotifierInstance(db.connectionName());
  connect(dbNotifier, SIGNAL(resourcePropertyChanged(int)),
	  this, SLOT(resourcePropertyUpdate(int)));
  connect(dbNotifier, SIGNAL(subResourcePropertyChanged(const QString&, int)),
	  this, SLOT(subResourcePropertyUpdate(const QString&, int)));

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

// private
bool KLFLibDBEngine::tableExists(const QString& subResource) const
{
  return pDB.tables().contains(dataTableName(subResource), Qt::CaseInsensitive);
}

// static
QString KLFLibDBEngine::dataTableName(const QString& subResource)
{
  return "t_"+subResource.toLower();
}
// static
QString KLFLibDBEngine::quotedDataTableName(const QString& subResource)
{
  QString dtname = dataTableName(subResource);
  dtname.replace('"', "\"\"");
  return '"' + dtname + '"';
}


uint KLFLibDBEngine::compareUrlTo(const QUrl& other, uint interestFlags) const
{
  // we can only test for these flags (see doc for KLFLibResourceEngine::compareUrlTo())
  interestFlags = interestFlags & (KlfUrlCompareBaseEqual);
  // and we have to compare sub-resources case-insensitive (SQL table names)
  interestFlags |= klfUrlCompareFlagIgnoreQueryItemValueCase;

  return klfUrlCompare(url(), other, interestFlags);
}

bool KLFLibDBEngine::canModifyData(const QString& subResource, ModifyType mt) const
{
  if ( !KLFLibResourceEngine::canModifyData(subResource, mt) )
    return false;
  if ( !validDatabase() )
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
  KLF_DEBUG_TIME_BLOCK(KLF_FUNC_NAME) ;

  KLF_ASSERT_CONDITION( validDatabase() , "Database connection not valid!" ,
			return false; ) ;

  QString propName = propertyNameForId(propId);
  if ( propName.isEmpty() )
    return false;

  if ( KLFPropertizedObject::property(propId) == value )
    return true;

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

void KLFLibDBEngine::subResourcePropertyUpdate(const QString& subResource, int propId)
{
  emit subResourcePropertyChanged(subResource, propId);
}

void KLFLibDBEngine::readResourceProperty(int propId)
{
  KLF_ASSERT_CONDITION( validDatabase() , "Database connection not valid!" ,
			return ) ;

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
    int propId = propertyIdForName(propname);
    if (!propertyNameRegistered(propname)) {
      if (!canRegisterProperty(propname))
	continue;
      KLFPropertizedObject::registerProperty(propname);
    }
    QVariant propvalue = convertVariantFromDBData(q.value(1));
    klfDbg( "Setting property `"<<propname<<"' (id #"<<propId<<") to "<<propvalue<<"" ) ;
    KLFPropertizedObject::setProperty(propId, propvalue);
    emit resourcePropertyChanged(propId);
  }
}

void KLFLibDBEngine::readDbMetaInfo()
{
  QSqlQuery q = QSqlQuery(pDB);
  q.prepare("SELECT name,value FROM klf_dbmetainfo");
  bool r = q.exec();
  if ( !r || q.lastError().isValid() ) {
    qWarning()<<KLF_FUNC_NAME<<": unable to fetch DB meta-info: "<<q.lastError().text();
    return;
  }
  while (q.next()) {
    QString name = q.value(0).toString();
    QString version = q.value(1).toString();
    if (name == QLatin1String("klf_dbversion")) {
      pDBVersion = version.toInt();
    }
  }
}
void KLFLibDBEngine::readAvailColumns(const QString& subResource)
{
  QSqlRecord rec = pDB.record(dataTableName(subResource));
  QStringList columns;
  int k;
  for (k = 0; k < rec.count(); ++k)
    columns << rec.fieldName(k);

  pDBAvailColumns[subResource] = columns;
}



// private
QStringList KLFLibDBEngine::columnNameList(const QString& subResource, const QList<int>& entryPropList,
					   bool wantIdFirst)
{
  QStringList cols;
  KLFLibEntry dummy; // to get prop name
  int k;
  for (k = 0; k < entryPropList.size(); ++k) {
    QString col = dummy.propertyNameForId(entryPropList[k]);
    if (pDBAvailColumns[subResource].contains(col))
      cols << col;
    else if (entryPropList[k] == KLFLibEntry::PreviewSize) // previewsize not available, use preview
      cols << "Preview";
  }
  if (entryPropList.size() == 0) {
    cols << "*";
  }
  if (wantIdFirst && (!cols.size() || cols[0] != "id") )
    cols.prepend("id");

  return cols;
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
      klfDbg( "Registering property "<<propName ) ;
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
    int propId = entry.propertyIdForName(cols[k]);
    QVariant value = dbReadEntryPropertyValue(q.value(k), propId);
    entry.setEntryProperty(cols[k], value);
  }
  //  klfDbg( ": cols="<<cols.join(",") ) ;
  //  klfDbg( ": read entry="<<entry<<" previewsize="<<entry.property(KLFLibEntry::PreviewSize)<<"; it's valid="<<entry.property(KLFLibEntry::PreviewSize).toSize().isValid()<<"; preview=... /null="<<entry.property(KLFLibEntry::Preview).value<QImage>().isNull() ) ;
  // add a preview size if necessary
  const QImage& preview = entry.property(KLFLibEntry::Preview).value<QImage>();
  if (!preview.isNull()) {
    const QSize& s = entry.property(KLFLibEntry::PreviewSize).toSize();
    if (!s.isValid() || s != preview.size()) {
      klfDbg( ": missing or incorrect preview size set to "<<entry.preview().size() ) ;
      entry.setPreviewSize(entry.preview().size());
    }
  }      
  return entry;
}


QList<KLFLib::entryId> KLFLibDBEngine::allIds(const QString& subResource)
{
  KLF_ASSERT_CONDITION( validDatabase() , "Database connection not valid!" ,
			return QList<KLFLib::entryId>() ) ;

  QSqlQuery q = QSqlQuery(pDB);
  q.prepare(QString("SELECT id FROM %1").arg(quotedDataTableName(subResource)));
  q.setForwardOnly(true);
  bool r = q.exec();
  if ( ! r || q.lastError().isValid() ) {
    qWarning("KLFLibDBEngine::allIds: Error fetching IDs!\n"
	     "SQL Error: %s", qPrintable(q.lastError().text()));
    return QList<KLFLib::entryId>();
  }
  QList<KLFLib::entryId> idlist;
  while (q.next()) {
    idlist << q.value(0).toInt();
  }
  return idlist;
}
bool KLFLibDBEngine::hasEntry(const QString& subResource, entryId id)
{
  KLF_ASSERT_CONDITION( validDatabase() , "Database connection not valid!" ,
			return false ) ;

  QSqlQuery q = QSqlQuery(pDB);
  q.prepare(QString("SELECT id FROM %1 WHERE id = ?").arg(quotedDataTableName(subResource)));
  q.addBindValue(id);
  bool r = q.exec();
  if ( ! r || q.lastError().isValid() ) {
    qWarning("KLFLibDBEngine::hasEntry: Error!\n"
	     "SQL Error: %s", qPrintable(q.lastError().text()));
    return false;
  }
  if (q.next())
    return true;
  return false;
}
QList<KLFLibResourceEngine::KLFLibEntryWithId>
/* */ KLFLibDBEngine::entries(const QString& subResource, const QList<KLFLib::entryId>& idList,
			      const QList<int>& wantedEntryProperties)
{
  KLF_DEBUG_BLOCK(KLF_FUNC_NAME); klfDbg( "\t: subResource="<<subResource<<"; idlist="<<idList ) ;
  KLF_ASSERT_CONDITION( validDatabase() , "Database connection not valid!" ,
			return QList<KLFLibEntryWithId>() ) ;
  if (idList.isEmpty())
    return QList<KLFLibEntryWithId>();

  QStringList cols = columnNameList(subResource, wantedEntryProperties, true);
  if (cols.contains("*")) {
    cols = QStringList();
    cols << "id" // first column is ID.
	 << pDBAvailColumns[subResource];
  }

  QSqlQuery q = QSqlQuery(pDB);
  q.prepare(QString("SELECT %1 FROM %2 WHERE id = ?").arg(cols.join(","),
							  quotedDataTableName(subResource)));

  KLFProgressReporter progr(0, idList.size(), this);
  if (!thisOperationProgressBlocked())
    emit operationStartReportingProgress(&progr, tr("Fetching items from library database ..."));

  QList<KLFLibEntryWithId> eList;

  int k;
  for (k = 0; k < idList.size(); ++k) {
    if (k % 10 == 0)
      progr.doReportProgress(k);

    q.bindValue(0, idList[k]);
    bool r = q.exec();
    if ( !r || q.lastError().isValid() ) {
      klfDbg( " SQL Error, sql="<<q.lastQuery()<<"; boundvalues="<<q.boundValues() ) ;
      qWarning("KLFLibDBEngine::entries: Error\n"
	       "SQL Error (?): %s", qPrintable(q.lastError().text()));
      continue;
    }
    if ( !q.next() ) {
      klfDbg( ": id="<<idList[k]<<" does not exist in DB." ) ;
      KLFLibEntryWithId e; e.entry = KLFLibEntry(); e.id = -1;
      eList << e;
      continue;
    }
    // get the entry
    KLFLibEntryWithId e;
    e.entry = readEntry(q, cols);
    e.id = q.value(0).toInt();
    eList << e;
  }

  progr.doReportProgress(idList.size());

  return eList;
}


static QString escape_sql_data_string(QString s)
{
  s.replace("'", "''");
  return "'" + s + "'";
}

// note: does not enclose expression in parens
static QString make_like_condition(QString field, QString val, bool wildbefore, bool wildafter, bool casesensitive)
{
  if (casesensitive) { // use GLOB for case sensitive match
    QString globval = val;
    if (wildbefore)
      globval.prepend("*");
    if (wildafter)
      globval.append("*");
    // we cannot escape special chars in GLOB -> use glob in conjunction with like
    return field+" GLOB "+escape_sql_data_string(globval)+"  AND  "
      + make_like_condition(field, val, wildbefore, wildafter, false);
  } else {
    // use LIKE for case-insensitive match
    val.replace("%", "\\%");
    val.replace("_", "\\_");
    val.replace("\\", "\\\\");
    val = escape_sql_data_string(val);
    if (wildbefore)
      val.prepend("%");
    if (wildafter)
      val.append("%");
    return field+" LIKE '"+val+"' ESCAPE '\\' ";
  }
}



static QString make_sql_condition(const KLFLib::EntryMatchCondition m, QVariantList *placeholders,
				  bool *haspostsqlcondition, KLFLib::EntryMatchCondition *postsqlcondition)
{
  /** \bug ........... LARGELY UNTESTED ..........................
   */
  *haspostsqlcondition = false;

  if (m.type() == KLFLib::EntryMatchCondition::MatchAllType) {
    return "1";
  }
  if (m.type() == KLFLib::EntryMatchCondition::PropertyMatchType) {
    QString condition;
    KLFLib::PropertyMatch pm = m.propertyMatch();
    KLFLibEntry dummyentry;
    QString field = dummyentry.propertyNameForId(pm.propertyId());
    condition += "(";
    uint f = pm.matchFlags();
    switch ( f & 0xFF ) { // the match type
    case Qt::MatchExactly:
      condition += field+" = ?";
      if ((f & Qt::CaseSensitive) == 0)
	condition += " COLLATE NOCASE";
      if (f & Qt::MatchFixedString)
	placeholders->append(pm.matchValueString());
      else
	placeholders->append(pm.matchValueString());
      if (pm.matchValueString().isEmpty()) // allow this field to be sql-NULL
	condition += " OR "+field+" IS NULL";
      break;
    case Qt::MatchContains:
      condition += make_like_condition(field, pm.matchValueString(), true, true, (f & Qt::CaseSensitive));
      break;
    case Qt::MatchStartsWith:
      condition += make_like_condition(field, pm.matchValueString(), true, false, (f & Qt::CaseSensitive));
      break;
    case Qt::MatchEndsWith:
      condition += make_like_condition(field, pm.matchValueString(), false, true, (f & Qt::CaseSensitive));
      break;
    case Qt::MatchRegExp:
      // sqlite does not support regexp natively
      *haspostsqlcondition = true;
      *postsqlcondition = m;
      condition += "1";
      break;
    case Qt::MatchWildcard:
      if (f & Qt::CaseSensitive) {
	condition += field+" GLOB  ? ";
      } else {
	condition += " lower("+field+") GLOB lower(?) ";
      }
      placeholders->append(pm.matchValueString());
      break;
    default:
      qWarning()<<KLF_FUNC_NAME<<": unknown property match type flags: "<<f ;
      return "0";
    }
    condition += ")";
    return condition;
  }
  if (m.type() == KLFLib::EntryMatchCondition::NegateMatchType) {
    if (m.conditionList().size() == 0) {
      qWarning()<<KLF_FUNC_NAME<<": condition list is empty for NOT match type!";
      return "0";
    }
    KLFLib::EntryMatchCondition postm = KLFLib::EntryMatchCondition::mkMatchAll(); // has to be initialized to sth..
    QString c = "(NOT " + make_sql_condition(m.conditionList()[0], placeholders,
					     haspostsqlcondition, &postm) ;
    if (*haspostsqlcondition) {
      *postsqlcondition = KLFLib::EntryMatchCondition::mkNegateMatch(postm);
    }
    return c;
  }
  if (m.type() == KLFLib::EntryMatchCondition::OrMatchType ||
      m.type() == KLFLib::EntryMatchCondition::AndMatchType) {
    static const char *w_and = " AND ";
    static const char *w_or  = " OR ";
    const char * word = (m.type() == KLFLib::EntryMatchCondition::AndMatchType) ? w_and : w_or ;
    QList<KLFLib::EntryMatchCondition> clist = m.conditionList();
    if (clist.isEmpty())
      return "1";
    int k;
    QString str;
    QList<KLFLib::EntryMatchCondition> postconditionlist;
    for (k = 0; k < clist.size(); ++k) {
      if (k > 0)
	str += word;

      KLFLib::EntryMatchCondition thispostm = KLFLib::EntryMatchCondition::mkMatchAll(); // init to sth...
      bool thishaspostsql;
      QString c = make_sql_condition(m.conditionList()[0], placeholders,
				     &thishaspostsql, &thispostm) ;
      if (thishaspostsql) {
	postconditionlist.append(thispostm);
      }
      str += c;
    } // for
    if (postconditionlist.size()) {
      *haspostsqlcondition = true;
      *postsqlcondition = (m.type() == KLFLib::EntryMatchCondition::OrMatchType)
	?  KLFLib::EntryMatchCondition::mkOrMatch(postconditionlist)
	:  KLFLib::EntryMatchCondition::mkOrMatch(postconditionlist) ;
    }
    return str;
  }
  qWarning()<<KLF_FUNC_NAME<<": unknown entry match condition type: "<<m.type();
  return "0";
}

int KLFLibDBEngine::query(const QString& subResource, const Query& query, QueryResult *result)
{
  KLF_DEBUG_BLOCK(KLF_FUNC_NAME);
  klfDbg( "\t: subResource="<<subResource<<"; query="<<query ) ;

  KLF_ASSERT_CONDITION( validDatabase() , "Database connection not valid!" ,
			return -1 ) ;

  QStringList cols = columnNameList(subResource, query.wantedEntryProperties, true);

  QString sql;
  // prepare SQL string.
  sql = QString("SELECT %1 FROM %2 ").arg(cols.join(","), quotedDataTableName(subResource));
  QVariantList placeholders;
  bool haspostsqlcondition = false;
  KLFLib::EntryMatchCondition postsqlcondition = KLFLib::EntryMatchCondition::mkMatchAll();
  QString wherecond = make_sql_condition(query.matchCondition, &placeholders, &haspostsqlcondition,
					 &postsqlcondition);
  sql += " WHERE "+wherecond;

  /** \bug. ................ postsqlcondition is NOT implemented ............. */
  if (haspostsqlcondition) {
    // fallback on very rudimentary search
    klfDbg("You are using a feature that is not natively implemented in KLFLibDBEngine: falling back to "
	   "rudimentary and slow implementation!");
    return KLFLibResourceSimpleEngine::queryImpl(this, subResource, query, result);
  }

  if (query.orderPropId != -1) {
    sql += " ORDER BY "+KLFLibEntry().propertyNameForId(query.orderPropId)+" ";
    sql += (query.orderDirection==Qt::AscendingOrder)?"ASC ":"DESC ";
  }

  if (query.limit != -1) {
    sql += " LIMIT "+QString::number(query.skip+query.limit);
  }

  klfDbg("Built query: SQL="<<sql<<"; placeholders="<<placeholders) ;

  QSqlQuery q = QSqlQuery(pDB);
  q.prepare(sql);
  q.setForwardOnly(true);
  int k;
  for (k = 0; k < placeholders.size(); ++k)
    q.bindValue(k, placeholders[k]);

  // and exec the query
  bool r = q.exec();
  if ( !r || q.lastError().isValid() ) {
    qWarning()<<KLF_FUNC_NAME<<"SQL Error: "<<qPrintable(q.lastError().text())
	      <<"\nSql was="<<sql<<"; bound values="<<q.boundValues();
    return -1;
  }

  // retrieve the entries

  cols = detectEntryColumns(q);

  int N = q.size();
  if (N == -1)
    N = 100;
  else
    N -= query.skip;
  KLFProgressReporter progr(0, N, this);
  if (!thisOperationProgressBlocked())
    emit operationStartReportingProgress(&progr, tr("Querying items from library database ..."));

  // skip the first 'query.skip' entries
  int skipped = 0;
  bool ok = true;
  while (skipped < query.skip && (ok = q.next()))
    ++skipped;
  klfDbg("skipped "<<skipped<<" entries.") ;

  // warning: Qt crashes on two consequent failing q.next() calls, if forward-only mode is enabled.

  int count = 0;
  while (ok && q.next()) {
    if (count % 10 == 0 && count < N) {
      // emit every 10 items, without exceeding what maximum we gave
      progr.doReportProgress(count);
    }

    KLFLibEntryWithId e;
    e.id = q.value(0).toInt(); // column 0 is 'id', see \ref columnNameList()
    e.entry = readEntry(q, cols);

    if (result->fillFlags & QueryResult::FillEntryIdList)
      result->entryIdList << e.id;
    if (result->fillFlags & QueryResult::FillRawEntryList)
      result->rawEntryList << e.entry;
    if (result->fillFlags & QueryResult::FillEntryWithIdList)
      result->entryWithIdList << e;
    ++count;
  }

  progr.doReportProgress(count);
  klfDbg("got "<<count<<" entries.") ;
  return count;
}
QList<QVariant> KLFLibDBEngine::queryValues(const QString& subResource, int entryPropId)
{
  KLF_DEBUG_BLOCK(KLF_FUNC_NAME);
  klfDbg( "\t: subResource="<<subResource<<"; entryPropId="<<entryPropId ) ;

  KLF_ASSERT_CONDITION( validDatabase() , "Database connection not valid!" ,
			return QList<QVariant>() ) ;

  if (!pDBAvailColumns.contains(subResource) || !hasSubResource(subResource)) {
    qWarning()<<KLF_FUNC_NAME<<": bad sub-resource: "<<subResource;
    return QVariantList();
  }

  KLFLibEntry dummye;
  QString pname;
  if (!dummye.propertyIdRegistered(entryPropId)) {
    qWarning()<<KLF_FUNC_NAME<<": Invalid property ID "<<entryPropId;
    return QVariantList();
  }
  pname = dummye.propertyNameForId(entryPropId);
  if (!pDBAvailColumns[subResource].contains(pname)) {
    qWarning()<<KLF_FUNC_NAME<<": property "<<pname<<" is not available in tables for sub-res "<<subResource
	      <<" (avail are "<<pDBAvailColumns[subResource]<<")";
    return QVariantList();
  }

  QString sql = "SELECT DISTINCT "+pname+" FROM "+quotedDataTableName(subResource);

  QSqlQuery q = QSqlQuery(pDB);
  q.prepare(sql);
  q.setForwardOnly(true);
  bool r = q.exec();
  if ( !r || q.lastError().isValid() ) {
    qWarning()<<KLF_FUNC_NAME<<"SQL Error: "<<qPrintable(q.lastError().text())
	      <<"\nSQL was: "<<sql;
    return QVariantList();
  }

  QVariantList list;
  while (q.next()) {
    list << dbReadEntryPropertyValue(q.value(0), entryPropId);
    klfDbg("adding value "<<list.last().toString()) ;
  }

  return list;
}



KLFLibEntry KLFLibDBEngine::entry(const QString& subResource, entryId id)
{
  KLF_ASSERT_CONDITION( validDatabase() , "Database connection not valid!" ,
			return KLFLibEntry() ) ;

  QSqlQuery q = QSqlQuery(pDB);
  q.prepare(QString("SELECT * FROM %1 WHERE id = ?").arg(quotedDataTableName(subResource)));
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


QList<KLFLibResourceEngine::KLFLibEntryWithId>
/* */ KLFLibDBEngine::allEntries(const QString& subResource, const QList<int>& wantedEntryProperties)
{
  KLF_ASSERT_CONDITION( validDatabase() , "Database connection not valid!" ,
			return QList<KLFLibEntryWithId>() ) ;

  QStringList cols = columnNameList(subResource, wantedEntryProperties, true);

  QSqlQuery q = QSqlQuery(pDB);
  q.prepare(QString("SELECT %1 FROM %2 ORDER BY id ASC").arg(cols.join(","), quotedDataTableName(subResource)));
  q.setForwardOnly(true);
  bool r = q.exec();
  if ( ! r || q.lastError().isValid() ) {
    qWarning()<<KLF_FUNC_NAME<<": SQL ERROR. Sql="<<q.lastQuery()<<"\n\tError: "<<q.lastError().text()
	      <<"\nbound values: "<<q.boundValues();
    return QList<KLFLibEntryWithId>();
  }

  QList<KLFLibEntryWithId> entryList;

  cols = detectEntryColumns(q);

  int count = q.size();

  KLFProgressReporter progr(0, count, this);
  if (!thisOperationProgressBlocked())
    emit operationStartReportingProgress(&progr, tr("Fetching items from library database ..."));

  int n = 0;
  while (q.next()) {
    if (n % 10 == 0) // emit every 10 items
      progr.doReportProgress(n++);
    KLFLibEntryWithId e;
    e.id = q.value(0).toInt(); // column 0 is 'id', see \ref columnNameList()
    e.entry = readEntry(q, cols);
    entryList << e;
  }

  progr.doReportProgress(count);

  return entryList;
}


bool KLFLibDBEngine::compareDefaultSubResourceEquals(const QString& subResourceName) const
{
  return QString::compare(defaultSubResource(), subResourceName, Qt::CaseInsensitive) == 0;
}


bool KLFLibDBEngine::canCreateSubResource() const
{
  return baseCanModifyStatus(false) == MS_CanModify;
}

bool KLFLibDBEngine::canDeleteSubResource(const QString& subResource) const
{
  if (baseCanModifyStatus(true, subResource) == MS_CanModify)
    if (tableExists(subResource) && subResourceList().size() > 1)
      return true;

  return false;
}

QVariant KLFLibDBEngine::subResourceProperty(const QString& subResource, int propId) const
{
  KLF_ASSERT_CONDITION( validDatabase() , "Database connection not valid!" ,
			return QVariant() ) ;

  QSqlQuery q = QSqlQuery(pDB);
  q.prepare("SELECT pvalue FROM klf_subresprops WHERE lower(subresource) = lower(?) AND pid = ?");
  q.addBindValue(QVariant::fromValue<QString>(subResource));
  q.addBindValue(QVariant::fromValue<int>(propId));
  int r = q.exec();
  if ( !r || q.lastError().isValid() ) {
    qWarning()<<"KLFLibDBEngine::subResourceProperty("<<subResource<<","<<propId<<"): SQL Error: "
	      <<q.lastError().text() << "\n\t\tSQL: "<<q.lastQuery()<<"\n\t\tBound values: "<<q.boundValues();
    klfDbg("DB: "<<pDB.connectionName());
    return QVariant();
  }
  //klfDbg( "KLFLibDBEngine::subRes.Prop.(): SQL="<<q.lastQuery()<<"; vals="<<q.boundValues() ) ;
  //qDebug("KLFLibDBEngine::subResourceProperty: Got size %d, valid=%d", q.size(), (int)q.isValid());
  if ( !q.next() ) {
    // return some default values, at least to ensure the correct data type
    if (propId == SubResPropLocked)
      return QVariant(false);
    if (propId == SubResPropViewType)
      return QVariant("");
    if (propId == SubResPropTitle)
      return QVariant("");
    return QVariant();
  }
  return convertVariantFromDBData(q.value(0));
}


bool KLFLibDBEngine::hasSubResource(const QString& subRes) const
{
  return tableExists(subRes);
}

QStringList KLFLibDBEngine::subResourceList() const
{
  KLF_ASSERT_CONDITION( validDatabase() , "Database connection not valid!" ,
			return QStringList() ) ;

  QStringList allTables = pDB.tables();
  QStringList subreslist;
  int k;
  for (k = 0; k < allTables.size(); ++k) {
    if (allTables[k].startsWith("t_"))
      subreslist << allTables[k].mid(2);
  }
  return subreslist;
}


bool KLFLibDBEngine::setSubResourceProperty(const QString& subResource, int propId, const QVariant& value)
{
  KLF_DEBUG_TIME_BLOCK(KLF_FUNC_NAME) ;

  KLF_ASSERT_CONDITION( validDatabase() , "Database connection not valid!" ,
			return false ) ;

  if ( !canModifyProp(-1) )
    return false;
  if ( subResourceProperty(subResource, SubResPropLocked).toBool() && propId != SubResPropLocked )
    return false; // still allow us to unlock the sub-resource (!)

  klfDbg( ": setting sub-resource property "<<propId<<" to "<<value<<" in sub-res "
	  <<subResource ) ;

  if ( subResourceProperty(subResource, propId) == value ) {
    klfDbg("property already has the requested value "<<value<<".");
    return true;
  }

  {
    QSqlQuery q = QSqlQuery(pDB);
    q.prepare("DELETE FROM klf_subresprops WHERE lower(subresource) = lower(?) and pid = ?");
    q.bindValue(0, subResource);
    q.bindValue(1, propId);
    bool r = q.exec();
    if ( !r || q.lastError().isValid() ) {
      qWarning()<<"KLFLibDBEngine::setSubRes.Prop.("<<subResource<<","<<propId<<","<<value<<"):"
		<<" can't DELETE!\n\t"<<q.lastError().text()<<"\n\tBound values="<<q.boundValues();
      return false;
    }
  }
  {
    QSqlQuery q = QSqlQuery(pDB);
    q.prepare("INSERT INTO klf_subresprops (subresource,pid,pvalue) VALUES (?,?,?)");
    q.bindValue(0, subResource);
    q.bindValue(1, propId);
    q.bindValue(2, convertVariantToDBData(value));
    if ( ! q.exec() || q.lastError().isValid() ) {
      qWarning()<<"KLFLibDBEngine::setSubRes.Prop.("<<subResource<<","<<propId<<","<<value<<"):"
		<<" can't INSERT!\n\t"<<q.lastError().text()<<"\n\tBound values="<<q.boundValues();
      return false;
    }
  }

  dbPropertyNotifierInstance(pDB.connectionName())
    ->notifySubResourcePropertyChanged(subResource, propId);
  // will be emitted by own slot from above call
  //  qDebug("DBEngine::setSubResourceProperty: Emitting signal!");
  //  emit subResourcePropertyChanged(subResource, propId);

  return true;
}



QVariant KLFLibDBEngine::dbMakeEntryPropertyValue(const QVariant& entryval, int propertyId)
{
  if (propertyId == KLFLibEntry::Latex)
    return QVariant::fromValue<QString>(entryval.toString());
  if (propertyId == KLFLibEntry::DateTime)
    return QVariant::fromValue<qulonglong>(entryval.toDateTime().toTime_t());
  if (propertyId == KLFLibEntry::Preview)
    return QVariant::fromValue<QByteArray>(image_data(entryval.value<QImage>(), "PNG"));
  if (propertyId == KLFLibEntry::Category)
    return QVariant::fromValue<QString>(entryval.toString());
  if (propertyId == KLFLibEntry::Tags)
    return QVariant::fromValue<QString>(entryval.toString());
  if (propertyId == KLFLibEntry::PreviewSize) {
    QSize s = entryval.value<QSize>();
    return QVariant::fromValue<qulonglong>( (((qulonglong)s.width()) <<         32)  |
					    (((qulonglong)s.height()) & 0xFFFFFFFF) );
  }
  // otherwise, return a generic encapsulation
  return convertVariantToDBData(entryval);
}
QVariant KLFLibDBEngine::dbReadEntryPropertyValue(const QVariant& dbdata, int propertyId)
{
  if (propertyId == KLFLibEntry::Latex)
    return dbdata.toString();
  if (propertyId == KLFLibEntry::DateTime)
    return QVariant::fromValue<QDateTime>(QDateTime::fromTime_t(dbdata.toULongLong()));
  if (propertyId == KLFLibEntry::Preview) {
    QImage img;
    img.loadFromData(dbdata.toByteArray(), "PNG");
    return QVariant::fromValue<QImage>(img);
  }
  if (propertyId == KLFLibEntry::Category)
    return dbdata.toString();
  if (propertyId == KLFLibEntry::Tags)
    return dbdata.toString();
  if (propertyId == KLFLibEntry::PreviewSize) {
    qulonglong val = dbdata.toULongLong();
    int w = (int)((val>>32) & 0xFFFFFFFF) ;
    int h = (int)(val       & 0xFFFFFFFF) ;
    return QVariant::fromValue<QSize>(QSize(w, h));
  }
  // otherwise, return the generic decapsulation
  return convertVariantFromDBData(dbdata);
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
      t == QVariant::Double || t == QVariant::Bool)
    return value; // value is OK

  // these types are to be converted to string
  if (t == QVariant::String)
    return encaps(ts, value.toString());

  // these types are to be converted by byte array
  if (t == QVariant::ByteArray)
    return encaps(ts, value.toByteArray());
  if (t == QVariant::DateTime)
    return encaps(ts, value.value<QDateTime>().toString(Qt::ISODate).toLatin1());
  if (t == QVariant::Image)
    return encaps(ts, image_data(value.value<QImage>(), "PNG"));

  // any other case: convert metatype to QByteArray.
  QByteArray valuedata;
  { QBuffer buf(&valuedata);
    buf.open(QIODevice::WriteOnly);
    QDataStream stream(&buf);
    stream.setVersion(QDataStream::Qt_4_4);
    /** \todo: TAKE CARE OF STREAM VERSION !!! */
    buf.setProperty("klfDataStreamAppVersion", "3.3");
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
      t == QVariant::Double || t == QVariant::Bool)
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
  //  klfDbg( "decaps(): "<<data ) ;
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
    QString svaluedata = QString::fromUtf8(valuedata).trimmed();
    return QVariant::fromValue<bool>(svaluedata[0] != '0' ||
				     (svaluedata != "1" && (svaluedata[0].toLower() == 't' ||
							    svaluedata[0].toLower() == 'y' ||
							    svaluedata.toInt() != 0)) );
}
  if (typenam == "QString")
    return QVariant::fromValue<QString>(QString::fromUtf8(valuedata));
  if (typenam == "QByteArray")
    return QVariant::fromValue<QByteArray>(valuedata);
  if (typenam == "QDateTime")
    return QDateTime::fromString(QString::fromLatin1(valuedata), Qt::ISODate);
  if (typenam == "QImage") {
    QImage img;
    img.loadFromData(valuedata);
    return QVariant::fromValue<QImage>(img);
  }

  // OTHERWISE, load from datastream save:

  QVariant value;
  { QByteArray vdata = valuedata;
    QBuffer buf(&vdata);
    buf.open(QIODevice::ReadOnly);
    QDataStream stream(&buf);
    stream.setVersion(QDataStream::Qt_4_4);
    /** \todo: TAKE CARE OF STREAM VERSION !!! */
    buf.setProperty("klfDataStreamAppVersion", "3.3");
    stream >> value; }
  return value;
}

bool KLFLibDBEngine::ensureDataTableColumnsExist(const QString& subResource, const QStringList& columnList)
{
  QSqlRecord rec = pDB.record(dataTableName(subResource));
  int k;
  bool failed = false;
  for (k = 0; k < columnList.size(); ++k) {
    if (columnList[k] == "*") // in case a superfluous '*' remained in a 'cols' stringlist... in eg. entries()
      continue;
    if (rec.contains(columnList[k]))
      continue;
    QSqlQuery sql = QSqlQuery(pDB);
    sql.prepare("ALTER TABLE "+quotedDataTableName(subResource)+" ADD COLUMN "+columnList[k]+" BLOB");
    bool r = sql.exec();
    if (!r || sql.lastError().isValid()) {
      qWarning()<<"KLFLibDBEngine::ensureDataTableColumnsExist("<<subResource<<"): Can't add column "
		<<columnList[k]<<"!\n"<<sql.lastError().text()<<" SQL="<<sql.lastQuery();
      failed = true;
    }
  }

  readAvailColumns(subResource);

  return !failed;
}
bool KLFLibDBEngine::ensureDataTableColumnsExist(const QString& subResource)
{
  KLFLibEntry dummy;
  QSqlRecord rec = pDB.record(dataTableName(subResource));
  QStringList propNameList = dummy.registeredPropertyNameList();
  return ensureDataTableColumnsExist(subResource, propNameList);
}


// --


bool KLFLibDBEngine::deleteSubResource(const QString& subResource)
{
  KLF_ASSERT_CONDITION( validDatabase() , "Database connection not valid!" ,
			return false ) ;

  if (!canDeleteSubResource(subResource))
    return false;

  QSqlQuery q = QSqlQuery(pDB);
  q.prepare(QString("DROP TABLE %1").arg(quotedDataTableName(subResource)));
  int r = q.exec();
  if ( !r || q.lastError().isValid() ) {
    qWarning()<<KLF_FUNC_NAME<<"("<<subResource<<"): SQL Error: "
	      <<q.lastError().text() << "\n\tSQL="<<q.lastQuery() ;
    return false;
  }

  // all ok
  emit subResourceDeleted(subResource);

  if (subResource == defaultSubResource()) {
    QString newDefaultSubResource;
    if (subResourceList().size() >= 1)
      newDefaultSubResource = subResourceList()[0];
    else
      newDefaultSubResource = QString();
    setDefaultSubResource(newDefaultSubResource);
    emit defaultSubResourceChanged(newDefaultSubResource);
  }
  return true;
}


bool KLFLibDBEngine::createSubResource(const QString& subResource, const QString& subResourceTitle)
{

  KLF_ASSERT_CONDITION( validDatabase() , "Database connection not valid!" ,
			return false ) ;

  if ( subResource.isEmpty() )
    return false;

  if ( subResourceList().contains(subResource) ) {
    qWarning()<<"KLFLibDBEngine::createSubResource: Sub-Resource "<<subResource<<" already exists!";
    return false;
  }

  bool r = createFreshDataTable(pDB, subResource);
  if (!r)
    return false;
  QString title = subResourceTitle;
  if (title.isEmpty())
    title = subResource;
  r = setSubResourceProperty(subResource, SubResPropTitle, QVariant(title));
  if (!r)
    return false;

  // success
  emit subResourceCreated(subResource);

  return true;
}


QList<KLFLibResourceEngine::entryId> KLFLibDBEngine::insertEntries(const QString& subres,
								   const KLFLibEntryList& entrylist)
{
  int k, j;

  KLF_ASSERT_CONDITION( validDatabase() , "Database connection not valid!" ,
			return QList<KLFLibResourceEngine::entryId>() ) ;

  klfDbg("subres="<<subres<<"; entrylist="<<entrylist) ;

  if ( entrylist.size() == 0 ) {
    return QList<entryId>();
  }

  if ( !canModifyData(subres, InsertData) ) {
    klfDbg("can't modify data.") ;
    return QList<entryId>();
  }

  if ( !tableExists(subres) ) {
    qWarning()<<KLF_FUNC_NAME<<": Sub-Resource "<<subres<<" does not exist.";
    return QList<entryId>();
  }

  KLFLibEntry e; // dummy object to test for properties
  QList<int> propids = e.registeredPropertyIdList();
  QStringList props;
  QStringList questionmarks;
  for (k = 0; k < propids.size(); ++k) {
    props << e.propertyNameForId(propids[k]);
    questionmarks << "?";
  }

  QList<entryId> insertedIds;

  ensureDataTableColumnsExist(subres);

  KLFProgressReporter progr(0, entrylist.size(), this);
  if (!thisOperationProgressBlocked())
    emit operationStartReportingProgress(&progr, tr("Inserting items into library database ..."));

  QSqlQuery q = QSqlQuery(pDB);
  q.prepare("INSERT INTO " + quotedDataTableName(subres) + " (" + props.join(",") + ") "
	    " VALUES (" + questionmarks.join(",") + ")");
  klfDbg( "INSERT query: "<<q.lastQuery() ) ;
  // now loop all entries, and exec the query with appropriate bound values
  for (j = 0; j < entrylist.size(); ++j) {
    if (j % 10 == 0) // emit every 10 items
      progr.doReportProgress(j);
    //    klfDbg( "New entry to insert." ) ;
    for (k = 0; k < propids.size(); ++k) {
      QVariant data = dbMakeEntryPropertyValue(entrylist[j].property(propids[k]), propids[k]);
      // and add a corresponding bind value for sql query
      klfDbg( "Binding value "<<k<<": "<<data ) ;
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

  // make sure the last signal is emitted as specified by KLFLibResourceEngine doc (needed
  // for example to close progress dialog!)
  progr.doReportProgress(entrylist.size());

  emit dataChanged(subres, InsertData, insertedIds);
  return insertedIds;
}


bool KLFLibDBEngine::changeEntries(const QString& subResource, const QList<entryId>& idlist,
				   const QList<int>& properties, const QList<QVariant>& values)
{
  if ( ! validDatabase() )
    return false;
  if ( ! canModifyData(subResource, ChangeData) )
    return false;

  if ( !tableExists(subResource) ) {
    qWarning()<<KLF_FUNC_NAME<<": Sub-Resource "<<subResource<<" does not exist.";
    return false;
  }

  if ( properties.size() != values.size() ) {
    qWarning("KLFLibDBEngine::changeEntry(): properties' and values' sizes mismatch!");
    return false;
  }

  if ( idlist.size() == 0 )
    return true; // no items to change

  klfDbg( "KLFLibDBEngine::changeEntries: funcional tests passed; idlist="<<idlist<<" props="
	  <<properties<<" vals="<<values ) ;

  ensureDataTableColumnsExist(subResource);

  KLFLibEntry e; // dummy 
  QStringList updatepairs;
  int k;
  for (k = 0; k < properties.size(); ++k) {
    updatepairs << (e.propertyNameForId(properties[k]) + " = ?");
  }
  // prepare query
  QSqlQuery q = QSqlQuery(pDB);
  q.prepare(QString("UPDATE %1 SET %2 WHERE id = ?")
	    .arg(quotedDataTableName(subResource), updatepairs.join(",")));
  for (k = 0; k < properties.size(); ++k) {
    q.bindValue(k, dbMakeEntryPropertyValue(values[k], properties[k]));
  }
  const int idBindValueNum = k;

  KLFProgressReporter progr(0, idlist.size(), this);
  if (!thisOperationProgressBlocked())
    emit operationStartReportingProgress(&progr, tr("Changing entries in database ..."));

  bool failed = false;
  for (k = 0; k < idlist.size(); ++k) {
    if (k % 10 == 0)
      progr.doReportProgress(k);

    q.bindValue(idBindValueNum, idlist[k]);
    bool r = q.exec();
    if ( !r || q.lastError().isValid() ) {
      qWarning() << "SQL UPDATE Error: "<<q.lastError().text()<<"\nWith SQL="<<q.lastQuery()
		 <<";\n and bound values="<<q.boundValues();
      failed = true;
    }
  }

  progr.doReportProgress(idlist.size());

  emit dataChanged(subResource, ChangeData, idlist);

  return !failed;
}

bool KLFLibDBEngine::deleteEntries(const QString& subResource, const QList<entryId>& idlist)
{
  if ( ! validDatabase() )
    return false;
  if (idlist.size() == 0)
    return true; // nothing to do
  if ( ! canModifyData(subResource, DeleteData) )
    return false;

  if ( !tableExists(subResource) ) {
    qWarning()<<KLF_FUNC_NAME<<": Sub-Resource "<<subResource<<" does not exist.";
    return false;
  }

  int k;
  bool failed = false;

  QString sql = QString("DELETE FROM %1 WHERE id = ?").arg(quotedDataTableName(subResource));

  klfDbg("sql is "<<sql<<", idlist is "<<idlist) ;

  QSqlQuery q = QSqlQuery(pDB);
  q.prepare(sql);

  KLFProgressReporter progr(0, idlist.size(), this);
  if (!thisOperationProgressBlocked())
    emit operationStartReportingProgress(&progr, tr("Removing entries from database ..."));

  for (k = 0; k < idlist.size(); ++k) {
    if (k % 10 == 0)
      progr.doReportProgress(k);

    q.bindValue(0, idlist[k]);
    bool r = q.exec();
    if ( !r || q.lastError().isValid() ) {
      qWarning()<<KLF_FUNC_NAME<<": Sql error: "<<q.lastError().text();
      failed = true;
      continue;
    }
  }

  progr.doReportProgress(idlist.size());

  emit dataChanged(subResource, DeleteData, idlist);

  return !failed;
}

bool KLFLibDBEngine::saveTo(const QUrl& newPath)
{
  if (newPath.scheme() == QLatin1String("klf+sqlite") && url().scheme() == QLatin1String("klf+sqlite")) {
    if (!newPath.host().isEmpty()) {
      qWarning()<<"KLFLibDBEngine::saveTo("<<newPath<<"): Expected empty host!";
      return false;
    }
    return QFile::copy(klfUrlLocalFilePath(url()), klfUrlLocalFilePath(newPath));
  }
  qWarning()<<"KLFLibDBEngine::saveTo("<<newPath<<"): Bad scheme!";
  return false;
}

// static
bool KLFLibDBEngine::initFreshDatabase(QSqlDatabase db)
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
    QString::number(1)+"')";
  sql << "CREATE TABLE klf_subresprops (id INTEGER PRIMARY KEY, pid INTEGER, subresource TEXT, pvalue BLOB)";

  int k;
  for (k = 0; k < sql.size(); ++k) {
    QSqlQuery query(db);
    query.prepare(sql[k]);
    bool r = query.exec();
    if ( !r || query.lastError().isValid() ) {
      qWarning()<<"KLFLibDBEngine::initFreshDatabase(): SQL Error: "<<query.lastError().text()<<"\n"
		<<"SQL="<<sql[k];
      return false;
    }
  }
  return true;
}

// static
bool KLFLibDBEngine::createFreshDataTable(QSqlDatabase db, const QString& subres)
{
  qDebug("KLFLibDBEngine::createFreshDataTable(.., '%s')", qPrintable(subres));
  QString datatablename = dataTableName(subres);
  if ( ! db.isOpen() ) {
    qWarning("KLFLibDBEngine::createFreshDataTable(..,%s): DB is not open!", qPrintable(subres));
    return false;
  }

  if ( db.tables().contains(datatablename) ) {
    qWarning("KLFLibDBEngine::createFreshDataTable(..,%s): table %s exists!", qPrintable(subres),
	     qPrintable(datatablename));
    return false;
  }
  QString qdtname = quotedDataTableName(subres);


  QSqlQuery query(db);
  query.prepare(QString("")+
		"CREATE TABLE "+qdtname+" (id INTEGER PRIMARY KEY, Latex TEXT, DateTime TEXT, "
		"       Preview BLOB, PreviewSize TEXT, Category TEXT, Tags TEXT, Style BLOB)");
  bool r = query.exec();
  if ( !r || query.lastError().isValid() ) {
    qWarning()<<"createFreshDataTable(): SQL Error: "<<query.lastError().text()<<"\n"
	      <<"SQL="<<query.lastQuery();
    return false;
  }

  return true;
}


/*
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
*/



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

#define MAGIC_SQLITE_HEADER_LEN 16

QString KLFLibDBLocalFileSchemeGuesser::guessScheme(const QString& fileName) const
{
  // refer to  http://www.sqlite.org/fileformat.html  (section 2.2.1)  for more info
  const char magic_sqlite_header[MAGIC_SQLITE_HEADER_LEN]
    = { 0x53, 0x51, 0x4c, 0x69, 0x74, 0x65, 0x20, 0x66,
	0x6f, 0x72, 0x6d, 0x61, 0x74, 0x20, 0x33, 0x00 }; // "SQLite format 3"
  char header[MAGIC_SQLITE_HEADER_LEN];

  klfDbg("guessing scheme of "<<fileName) ;

  if (fileName.endsWith(".klf.db"))
    return QLatin1String("klf+sqlite");

  QFile f(fileName);
  if ( ! f.open(QIODevice::ReadOnly) ) {
    klfDbg("Yikes, can't read file "<<fileName);
    return QString();
  }

  int len = f.read(header, MAGIC_SQLITE_HEADER_LEN);

  if (len < MAGIC_SQLITE_HEADER_LEN) {
    klfDbg("Nope-can't read header.");
    return QString(); // error reading
  }

  if (!strncmp(header, magic_sqlite_header, MAGIC_SQLITE_HEADER_LEN)) {
    klfDbg("Yep, it's klf-sqlite!");
    return QLatin1String("klf+sqlite");
  }

  klfDbg("Nope-bad header.");
  return QString();
}

// ------------------------------------


KLFLibDBEngineFactory::KLFLibDBEngineFactory(QObject *parent)
  : KLFLibEngineFactory(parent)
{
  KLFLibBasicWidgetFactory::LocalFileType f;
  f.scheme = QLatin1String("klf+sqlite");
  f.filepattern = QLatin1String("*.klf.db");
  f.filter = QString("%1 (%2)").arg(schemeTitle(f.scheme), f.filepattern);
  KLFLibBasicWidgetFactory::addLocalFileType(f);
  new KLFLibDBLocalFileSchemeGuesser(this);
}

QStringList KLFLibDBEngineFactory::supportedTypes() const
{
  return QStringList() << QLatin1String("klf+sqlite");
}

QString KLFLibDBEngineFactory::schemeTitle(const QString& scheme) const
{
  if (scheme == QLatin1String("klf+sqlite"))
    return tr("Local Library Database File");
  return QString();
}
uint KLFLibDBEngineFactory::schemeFunctions(const QString& scheme) const
{
  uint flags = FuncOpen;
  if (scheme == QLatin1String("klf+sqlite"))
    flags |= FuncCreate;
  else
    qWarning()<<"KLFLibDBEngineFactory::schemeFunctions: Bad scheme: "<<scheme;
  return flags;
}

QString KLFLibDBEngineFactory::correspondingWidgetType(const QString& scheme) const
{
  if (scheme == QLatin1String("klf+sqlite"))
    return QLatin1String("LocalFile");
  return QString();
}


KLFLibResourceEngine *KLFLibDBEngineFactory::openResource(const QUrl& location, QObject *parent)
{
  return KLFLibDBEngine::openUrl(location, parent);
}


KLFLibResourceEngine *KLFLibDBEngineFactory::createResource(const QString& scheme,
							    const Parameters& parameters, QObject *parent)
{
  QString defsubres = parameters["klfDefaultSubResource"].toString();
  QString defsubrestitle = parameters["klfDefaultSubResourceTitle"].toString();
  if (defsubres.isEmpty())
    defsubres = "entries";
  if (defsubrestitle.isEmpty())
    defsubrestitle = tr("Default Table", "[[default sub-resource title]]");

  // ensure that the sub-resource (internal) name conforms to standard characters
  defsubres = KLFLibNewSubResDlg::makeSubResInternalName(defsubres);

  if ( !parameters.contains("Filename") ) {
    qWarning()
      <<"KLFLibLegacyEngineFactory::createResource: bad parameters. They do not contain `Filename': "
      <<parameters;
  }

  if (scheme == QLatin1String("klf+sqlite"))
    return KLFLibDBEngine::createSqlite(parameters["Filename"].toString(), defsubres,
					defsubrestitle, parent);
  qWarning()<<"KLFLibDBEngineFactory::createResource("<<scheme<<","<<parameters<<","<<parent<<"):"
	    <<"Bad scheme!";
  return NULL;
}
