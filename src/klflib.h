/***************************************************************************
 *   file klflib.h
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

#ifndef KLFLIB_H
#define KLFLIB_H

#include <QImage>
#include <QMap>
#include <QUrl>
#include <QSqlDatabase>
#include <QSqlQuery>

#include <klfdata.h>  // for KLFStyle and legacy library
#include <klfpobj.h>

/** \brief An entry (single formula) in the library
 */
class KLF_EXPORT KLFLibEntry : public KLFPropertizedObject {
public:
  /** \note The numeric IDs don't have to be preserved from one version of KLF to
   * another, since they are nowhere stored. Properties are always stored by name
   * when dealing in scopes larger than the running application (saved files, etc.). */
  enum PropertyId {
    Latex, //!< The Latex Code of the equation
    DateTime, //!< The Date/Time at which the equation was evaluated
    Preview, //!< An Image Preview of equation (scaled down QImage)
    Category, //!< The Category to which eq. belongs (path-style string)
    Tags, //!< Tags about the equation (string)
    Style //!< KLFStyle style used
  };

  KLFLibEntry(const QString& latex = QString(), const QDateTime& dt = QDateTime(),
	      const QImage& preview = QImage(),	const QString& category = QString(),
	      const QString& tags = QString(), const KLFStyle& style = KLFStyle());
  KLFLibEntry(const KLFLibEntry& copy);
  virtual ~KLFLibEntry();

  inline QString latex() const { return property(Latex).toString(); }
  inline QDateTime dateTime() const { return property(DateTime).toDateTime(); }
  inline QImage preview() const { return property(Preview).value<QImage>(); }
  inline QString category() const { return property(Category).toString(); }
  inline QString tags() const { return property(Tags).toString(); }
  inline KLFStyle style() const { return property(Style).value<KLFStyle>(); }

  inline void setLatex(const QString& latex) { setProperty(Latex, latex); }
  inline void setDateTime(const QDateTime& dt) { setProperty(DateTime, dt); }
  inline void setPreview(const QImage& img) { setProperty(Preview, img); }
  inline void setCategory(const QString& s) { setProperty(Category, s); }
  inline void setTags(const QString& s) { setProperty(Tags, s); }
  inline void setStyle(const KLFStyle& style) { setProperty(Style, QVariant::fromValue(style)); }

private:

  void initRegisteredProperties();
};

Q_DECLARE_METATYPE(KLFLibEntry)
  ;

class KLF_EXPORT KLFLibResourceEngine : public QObject
{
  Q_OBJECT
public:
  typedef qint32 entryId;

  struct KLFLibEntryWithId {
    entryId id;
    KLFLibEntry entry;
  };

  KLFLibResourceEngine(const QUrl& url, QObject *parent = NULL);
  virtual ~KLFLibResourceEngine();

  virtual QUrl url() const { return pUrl; }

  /** Suggested library view widget to view this resource with optimal user experience :-) .
   * Return QString() for default view. */
  virtual QString suggestedViewTypeIdentifier() const { return QString(); }

  virtual KLFLibEntry entry(entryId id) = 0;
  virtual QList<KLFLibEntryWithId> allEntries() = 0;

  virtual entryId insertEntry(const KLFLibEntry& entry) = 0;
  virtual bool deleteEntry(const entryId& id) = 0;

protected:

  QUrl pUrl;
};



/** An abstract factory class for opening resources identified by their URL.
 */
class KLF_EXPORT KLFAbstractLibEngineFactory : public QObject
{
  Q_OBJECT
public:
  typedef QMap<QString,QVariant> Parameters;

  /** Constructs an engine factory and automatically regisers it. */
  KLFAbstractLibEngineFactory(QObject *parent = NULL);
  /** Destroys this engine factory and unregisters it. */
  virtual ~KLFAbstractLibEngineFactory();

  /** Should return a list of supported URL schemes this factory can open */
  virtual QStringList supportedSchemes() const = 0;

  /** Create a library engine that opens resource stored at \c location. The resource
   * engine should be constructed as a child of object \c parent. */
  virtual KLFLibResourceEngine *openResource(const QUrl& location, QObject *parent = NULL) = 0;

  /** \returns whether this factory can be used to create new resources (eg. a new library
   * sqlite resource, etc.) */
  virtual bool canCreateResource() const;
  /** create a widget that will prompt to user the different settings for the new resource
   * that is about to be created.  Do not reimplement this function if canCreateResource()
   * returns false. */
  virtual QWidget * createPromptParametersWidget(QWidget *parent,
						 Parameters defaultparameters = Parameters());
  /** get the parameters edited by user, that are stored in \c widget GUI. */
  virtual Parameters retrieveParametersFromWidget(QWidget *widget);
  /** Actuall create the new resource, with the given settings. Save the resource to location
   * \c newResourceUrl */
  virtual KLFLibResourceEngine *createResource(const QUrl& newResourceUrl, Parameters parameters);

  /** Returns the factory that can handle the URL scheme \c urlScheme, or NULL if no such
   * factory exists (ie. has been registered). */
  static KLFAbstractLibEngineFactory *findFactoryFor(const QString& urlScheme);

private:
  static void registerFactory(KLFAbstractLibEngineFactory *factory);
  static void unRegisterFactory(KLFAbstractLibEngineFactory *factory);

  static QList<KLFAbstractLibEngineFactory*> pRegisteredFactories;
};




/** Library Resource engine implementation for an (abstract) database (using Qt
 * SQL interfaces) */
class KLF_EXPORT KLFLibDBEngine : public KLFLibResourceEngine
{
  Q_OBJECT
public:
  /** Use this function as a constructor. Creates a KLFLibDBEngine object,
   * with QObject parent \c parent, opening the database at location \c url.
   * Returns NULL if opening the database failed.
   *
   * A non-NULL returned object was successfully connected to database. */
  static KLFLibDBEngine * openUrl(const QUrl& url, QObject *parent = NULL);
  virtual ~KLFLibDBEngine();

  /** True if one has supplied a valid database in the constructor or with a
   * \ref setDatabse() call. */
  bool validDatabase() const;
  /** supply an open database. */
  void setDatabase(QSqlDatabase db_connection);

  virtual KLFLibEntry entry(entryId id);
  virtual QList<KLFLibEntryWithId> allEntries();

  virtual entryId insertEntry(const KLFLibEntry& entry);
  virtual bool deleteEntry(const entryId& id);

  static bool initFreshDatabase(QSqlDatabase db);

private:
  KLFLibDBEngine(const QSqlDatabase& db, const QUrl& url, QObject *parent = NULL);

  QSqlDatabase pDB;

  QMap<int,int> detectEntryColumns(const QSqlQuery& q);
  KLFLibEntry readEntry(const QSqlQuery& q, QMap<int,int> columns);
};

/** The associated factory to the KLFLibDBEngine engine. */
class KLF_EXPORT KLFLibDBEngineFactory : public KLFAbstractLibEngineFactory
{
  Q_OBJECT
public:
  KLFLibDBEngineFactory(QObject *parent = NULL);
  virtual ~KLFLibDBEngineFactory() { }

  /** Should return a list of supported URL schemes this factory can open */
  virtual QStringList supportedSchemes() const;

  /** Create a library engine that opens resource stored at \c location */
  virtual KLFLibResourceEngine *openResource(const QUrl& location, QObject *parent = NULL);

};


// class KLF_EXPORT KLFLibLegacyLibraryEngine : public KLFLibResourceEngine
// {
//   // ..........
// };




#endif

