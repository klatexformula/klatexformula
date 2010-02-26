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
 *
 * Stores Latex code, Date/Time of evaluation, A preview image, A Category String,
 * A Tags String, and a Style in a KLFPropertizedObject-based object.
 *
 * This object can be used as a normal value (ie. it has copy constructor, assignment
 * operator and default constructor).
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

typedef QList<KLFLibEntry> KLFLibEntryList;


/** \brief An abstract resource engine
 *
 * This class is the base of all resource types. Subclasses (e.g. KLFLibDBEngine) implement
 * the actual access to the data, while this class defines the base API that will be used
 * by e.g. the library browser (through a KLFAbstractLibView subclass) to access the data.
 *
 * Subclasses may choose to implement various features specified by the \ref ResourceFeature
 * enum. The supported features must be passed (binary OR-ed) to the constructor.
 *
 * Library entries are communicated with KLFLibEntry objects. Each entry in the resource must
 * be attributed a numerical ID (unique within the resource, not necessarily globally unique
 * among all open resources). Entries are queried by calling \ref allEntries() to get all the
 * entries with their IDs, or \ref entry() if a specific entry (whose ID is already known) is
 * requested. IDs are attributed by the resource engine subclasses, depending of course on the
 * backend (eg. a database backend storage could use the ID in the database table).
 *
 * Resources have properties (stored in a KLFPropertizedObject structure, NOT in regular QObject
 * properties) that ARE STORED IN THE RESOURCE DATA ITSELF. Built-in properties are listed
 * in the \ref ResourceProperty enum.
 *
 * <b>Note on URL</b>. The URL is not a resource property as it is not stored in the resource
 * itself. However the resource engine instance knows about the url it is manipulating
 * (because subclasses pass the relevant url as parameter in the constructor). However, the
 * URL itself blindly stored and reprovided when requested; it is not used by
 * KLFLibResourceEngine itself. Small exception: in the constructor, some query items are
 * recognized and stripped from the URL. See below.
 *
 * <b>Read-Only Mode</b>. The read-only mode differs from the locked property in that the
 * mode is activated by the accessor only and is NOT stored in the resource data eg. on disk
 * (in contrast to the locked property). It is up to SUBCLASSES to enforce this flag and
 * prevent data from being modified if isReadOnly() is TRUE.<br>
 * <b>Preventing READ-ONLY</b>. Subclasses may prevent read-only mode by not specifying
 * the FeatureReadOnly in the constructor. In this case the default implementation of
 * \ref setReadOnly() will return FALSE without doing anything. Also in this case, any
 * occurrence of the query item "klfReadOnly" in the URL is stripped and ignored.
 *
 * <b>Query Items in URL</b>. The URL may contain Query Items e.g.
 * <tt>scheme://klf.server.dom/path/to/location<b>?klfReadOnly=true</b></tt>. Here is a
 * list of recognized (universal) query items. Subclasses may choose to recognize more query
 * items, but these listed here are detected in the KLFLibResourceEngine constructor and
 * the appropriate flags are set (eg. \ref isReadOnly()):
 * - <tt>klfReadOnly=<i>{</i>true<i>|</i>false<i>}</i></tt> If provided, sets the read-only
 *   flag to the given value. If not provided, the flag defaults to false (flag is directly
 *   set, bypassing the setReadOnly() function).
 * .
 * Note that [only] recognized query items are stripped from the \ref url() as they are parsed.
 *
 * <b>NOTES FOR SUBCLASSES</b><br>
 * - Subclasses must deal with the properties and implement them as they are meant to. For
 *   example, if a resource is \ref locked(), then \ref canModify() must return FALSE and
 *   any attempt to modify the resource must fail.
 * - See also docs for \ref setTitle(), \ref setLocked().
 * - The \ref resourcePropertyChanged() signal must be emitted by subclasses when they change
 *   any resource property.
 */
class KLF_EXPORT KLFLibResourceEngine : public QObject, public KLFPropertizedObject
{
  Q_OBJECT
public:
  typedef qint32 entryId;

  struct KLFLibEntryWithId {
    entryId id;
    KLFLibEntry entry;
  };

  /** List of built-in KLFPropertizedObject-properties. */
  enum ResourceProperty { PropTitle = 0, PropLocked };

  enum ResourceFeature {
    FeatureReadOnly = 0x0001,
    FeatureLocked = 0x0002,
    FeatureSaveAs = 0x0004
  };

  KLFLibResourceEngine(const QUrl& url, uint supportedfeatureflags, QObject *parent = NULL);
  virtual ~KLFLibResourceEngine();

  //! List of features supported by this resource engine
  /** Use this function to probe whether this resource instance supports a specific
   * feature. For example a property editor widget might want to know if this resource
   * supportes the \ref FeatureLocked feature, to enable or disable the corresponding
   * check box.
   *
   * Do not reimplement in subclasses, simply pass the feature flags to the constructor.
   * */
  virtual uint supportedFeatureFlags() const { return pFeatureFlags; }

  virtual QUrl url() const { return pUrl; }
  virtual bool isReadOnly() const { return pReadOnly; }

  //! The human-set title of this resource
  /** See \ref setTitle(). */
  virtual QString title() const { return KLFPropertizedObject::property(PropTitle).toString(); }
  //! Is this resource is locked?
  /** If the resource is locked (property \ref PropLocked), no changes can be in principle made.
   *
   * \note It is up to subclasses of KLFLibResourceEngine to enforce that locked resources are
   *   not modified and that \ref canModify() returns FALSE. */
  virtual bool locked() const { return KLFPropertizedObject::property(PropLocked).toBool(); }

  /** Subclasses should return TRUE here if, in principle, it is possible to modify the
   * resource's data. Return for ex. false when opening a read-only file.
   *
   * \warning subclasses should call the base implementation to e.g. take into account
   * the \ref locked() property. If the base implementation returns FALSE, then the
   * subclasses should also return FALSE. */
  virtual bool canModify() const;

  /** This function should be used instead of \ref canModify() to test whether the
   * Locked property can be modified, to prevent a set locked property from preventing
   * itself from being unset (!) */
  virtual bool canUnLock() const;

  /** Suggested library view widget to view this resource with optimal user experience :-) .
   * Return QString() for default view. */
  virtual QString suggestedViewTypeIdentifier() const { return QString(); }

  virtual KLFLibEntry entry(entryId id) = 0;
  virtual QList<KLFLibEntryWithId> allEntries() = 0;

signals:
  void resourcePropertyChanged(int propId);
  void dataChanged();

public slots:

  /** Store the title and set property PropTitle to the set title. Return
   * value TRUE indicates success. This function may return FALSE if changing the title is
   * not permitted, or the new title is not acceptable (for some given standard...).
   *
   * Subclasses should store the new value in the resource data (e.g. on disk), set the
   * PropLocked KLFPropertizedObject-based property, then emit resourcePropertyChanged().   */
  virtual bool setTitle(const QString& title) = 0;

  //! Set the resource to be locked
  /** See \ref locked() for more doc.
   *
   * If you wish to implement resource locking, this function should be reimplemented to
   * lock or unlock the resource. If the feature is globally not supported, you can leave
   * the default implementation return FALSE.
   *
   * The function may return FALSE for failure or if the operation is not permitted in
   * a special case.
   *
   * Subclasses should store the new value in the resource data (e.g. on disk), set the
   * PropLocked KLFPropertizedObject-based property, then emit resourcePropertyChanged()
   * and return TRUE for success.
   */
  virtual bool setLocked(bool locked);

  //! Set the resource to be read-only or not
  /** The base implementation sets the read-only flag to \c readonly (this is then returned
   * by \ref isReadOnly()) if the \ref FeatureReadOnly was given to the constructor; does
   * nothing and returns FALSE otherwise.
   */
  virtual bool setReadOnly(bool readonly);

  //! (shortcut for the list version)
  /** This function is already implemented as shortcut for calling the list version. Subclasses
   * need in principle NOT reimplement this function. */
  virtual entryId insertEntry(const KLFLibEntry& entry);

  virtual QList<entryId> insertEntries(const KLFLibEntryList& entrylist) = 0;
  virtual bool changeEntries(const QList<entryId>& idlist, const QList<int>& properties,
			   const QList<QVariant>& values) = 0;
  virtual bool deleteEntries(const QList<entryId>& idlist) = 0;

  /** If the \ref FeatureSaveAs is supported (passed to the constructor), reimplement this
   * function to save the resource data in the new path specified by \c newPath.
   * The \c newPath is garanteed to have same schema as the previous url. */
  virtual bool saveAs(const QUrl& newPath);

protected:


private:
  void initRegisteredProperties();

  QUrl pUrl;
  uint pFeatureFlags;
  bool pReadOnly;
};



/** An abstract factory class for opening resources identified by their URL.
 */
class KLF_EXPORT KLFLibEngineFactory : public QObject
{
  Q_OBJECT
public:
  /** A generalized way of passing arbitrary parameters for creating
   * new resources.
   *
   * Some parameters have special meaning to the system; see
   * \ref retrieveCreateParametersFromWidget().
   */
  typedef QMap<QString,QVariant> Parameters;

  /** Constructs an engine factory and automatically regisers it. */
  KLFLibEngineFactory(QObject *parent = NULL);
  /** Destroys this engine factory and unregisters it. */
  virtual ~KLFLibEngineFactory();

  /** Should return a list of supported URL schemes this factory can open.
   * Two factories should NOT provide common scheme names, or bugs like one factory
   * being be shadowed by the other may appear. */
  virtual QStringList supportedSchemes() const = 0;

  /** Should return a human (short) description of the given scheme (which is one
   * returned by \ref supportedSchemes()) */
  virtual QString schemeTitle(const QString& scheme) const = 0;

  /** create a widget that will prompt to user to open a resource of given \c scheme.
   * It could be a file selection widget, or a text entry for a hostname or etc. The
   * widget should be initialized to \c defaultlocation if the latter is non-empty.
   *
   * The widget should have a <tt>readyToOpen(bool isReady)</tt> signal that is emitted
   * to synchronize the enabled state of the "open" button. */
  virtual QWidget * createPromptUrlWidget(QWidget *parent, const QString& scheme,
					  QUrl defaultlocation = QUrl()) = 0;

  /** get the url edited by user, that are stored in \c widget GUI. \c widget is a
   * QWidget returned by \ref createPromptUrlWidget(). */
  virtual QUrl retrieveUrlFromWidget(const QString& scheme, QWidget *widget) = 0;

  /** Create a library engine that opens resource stored at \c location. The resource
   * engine should be constructed as a child of object \c parent. */
  virtual KLFLibResourceEngine *openResource(const QUrl& location, QObject *parent = NULL) = 0;

  /** \returns whether this factory can be used to create new resources (eg. a new library
   * sqlite resource, etc.) */
  virtual bool canCreateResource(const QString& scheme) const;
  /** create a widget that will prompt to user the different settings for the new resource
   * that is about to be created.  Do not reimplement this function if canCreateResource()
   * returns false. Information to which URL to store the resource should be included
   * in these parameters! */
  virtual QWidget *createPromptCreateParametersWidget(QWidget *parent, const QString& scheme,
						      const Parameters& defaultparameters = Parameters());
  /** get the parameters edited by user, that are stored in \c widget GUI.
   *
   * Some special parameters are recognized by the system:
   * <tt>param["retry"]=true</tt> will cause the dialog not to exit but re-prompt user to
   * possibly change his input (could result from user clicking "No" in a "Overwrite?"
   * dialog presented in a reimplementation of this function).
   * <tt>param["cancel"]=true</tt> will cause the `create resource' process to be cancelled.
   *
   * If an empty Parameters() is returned, it is also interpreted as a cancel operation.
   * */
  virtual Parameters retrieveCreateParametersFromWidget(const QString& scheme, QWidget *widget);
  /** Actually create the new resource, with the given settings. Open the resource and return
   * the KLFLibResourceEngine object. */
  virtual KLFLibResourceEngine *createResource(const QString& scheme, const Parameters& parameters,
					       QObject *parent = NULL);


  /** Returns the factory that can handle the URL scheme \c urlScheme, or NULL if no such
   * factory exists (ie. has been registered). */
  static KLFLibEngineFactory *findFactoryFor(const QString& urlScheme);

  /** Returns a combined list of all schemes all factories support. ie. returns a list of
   * all schemes we're capable of opening. */
  static QStringList allSupportedSchemes();
  /** Returns the full list of installed factories. */
  static QList<KLFLibEngineFactory*> allFactories() { return pRegisteredFactories; }

private:
  static void registerFactory(KLFLibEngineFactory *factory);
  static void unRegisterFactory(KLFLibEngineFactory *factory);

  static QList<KLFLibEngineFactory*> pRegisteredFactories;
};





/** Library Resource engine implementation for an (abstract) database (using Qt
 * SQL interfaces)
 */
class KLF_EXPORT KLFLibDBEngine : public KLFLibResourceEngine
{
  Q_OBJECT

  Q_PROPERTY(bool autoDisconnectDB READ autoDisconnectDB) ;
  Q_PROPERTY(QString dataTableName READ dataTableName)
public:
  /** Use this function as a constructor. Creates a KLFLibDBEngine object,
   * with QObject parent \c parent, opening the database at location \c url.
   * Returns NULL if opening the database failed.
   *
   * Url may contain following query strings:
   * - <tt>dataTableName=<i>tablename</i></tt> to specify the data table name
   *   in the DB
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
   *
   * A non-NULL returned object was successfully connected to database.
   * */
  static KLFLibDBEngine * createSqlite(const QString& fileName, const QString& datatablename,
				       QObject *parent = NULL);


  /** Simple destructor. Disconnects the database if the autoDisconnectDB() property was
   * set. */
  virtual ~KLFLibDBEngine();

  bool canModify() const;

  /** True if one has supplied a valid database in the constructor or with a
   * \ref setDatabase() call. */
  bool validDatabase() const;
  /** supply an open database. */
  void setDatabase(QSqlDatabase db_connection);

  virtual bool autoDisconnectDB() const { return pAutoDisconnectDB; }
  virtual QString dataTableName() const { return pDataTableName; }

  virtual KLFLibEntry entry(entryId id);
  virtual QList<KLFLibEntryWithId> allEntries();

public slots:

  virtual bool setTitle(const QString& title);
  virtual bool setLocked(bool locked);

  virtual QList<entryId> insertEntries(const KLFLibEntryList& entries);
  virtual bool changeEntries(const QList<entryId>& idlist, const QList<int>& properties,
			   const QList<QVariant>& values);
  virtual bool deleteEntries(const QList<entryId>& idlist);

  static bool initFreshDatabase(QSqlDatabase db, const QString& datatablename);

private:
  KLFLibDBEngine(const QSqlDatabase& db, const QString& tablename,
		 bool autoDisconnectDB, const QUrl& url, QObject *parent = NULL);

  QSqlDatabase pDB;
  
  bool pAutoDisconnectDB;
  QString pDataTableName;

  QMap<int,int> detectEntryColumns(const QSqlQuery& q);
  KLFLibEntry readEntry(const QSqlQuery& q, QMap<int,int> columns);

  QVariant convertVariantToDBData(const QVariant& value) const;

  bool setResourceProperty(int propId, const QVariant& value);
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


// class KLF_EXPORT KLFLibLegacyLibraryEngine : public KLFLibResourceEngine
// {
//   // ..........
// };




#endif

