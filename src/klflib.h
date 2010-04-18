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
#include <QDataStream>
#include <QDateTime>
#include <QSqlDatabase>
#include <QSqlQuery>

//#include <klfdata.h>  // for KLFStyle
#include <klfpobj.h>



/** \brief A Formula Style (collection of properties)
 *
 * Structure containing forground color, bg color, mathmode, preamble, etc.
 */
struct KLFStyle {
  KLFStyle(QString nm = QString(), unsigned long fgcol = qRgba(0,0,0,255),
	   unsigned long bgcol = qRgba(255,255,255,0),
	   const QString& mmode = QString(),
	   const QString& pre = QString(),
	   int dotsperinch = -1)
    : name(nm), fg_color(fgcol), bg_color(bgcol), mathmode(mmode), preamble(pre),
      dpi(dotsperinch) { }
  KLFStyle(const KLFStyle& o)
    : name(o.name), fg_color(o.fg_color), bg_color(o.bg_color), mathmode(o.mathmode),
      preamble(o.preamble), dpi(o.dpi) { }

  QString name; // this may not always be set, it's only important in saved style list.
  unsigned long fg_color;
  unsigned long bg_color;
  QString mathmode;
  QString preamble;
  int dpi;

  const KLFStyle& operator=(const KLFStyle& o) {
    name = o.name; fg_color = o.fg_color; bg_color = o.bg_color; mathmode = o.mathmode;
    preamble = o.preamble; dpi = o.dpi;
    return *this;
  }
};

Q_DECLARE_METATYPE(KLFStyle)
  ;

QString prettyPrintStyle(const KLFStyle& sty);

typedef QList<KLFStyle> KLFStyleList;

QDataStream& operator<<(QDataStream& stream, const KLFStyle& style);
QDataStream& operator>>(QDataStream& stream, KLFStyle& style);
// exact matches
bool operator==(const KLFStyle& a, const KLFStyle& b);




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

  /** Set the property named \c name to value \c value. If the property does not
   * yet exist in the registered properties, it is registered.
   * \returns -1 for error, or the property ID that was successfully (maybe registered
   * and) set.
   */
  int setEntryProperty(const QString& propName, const QVariant& value);

private:
  
  void initRegisteredProperties();
};

Q_DECLARE_METATYPE(KLFLibEntry)
  ;

typedef QList<KLFLibEntry> KLFLibEntryList;


//! Contains general definitions to be used anywhere in the KLFLib* framework
namespace KLFLib {
  //! An entry ID
  /** The type of the ID of an entry in a library resource (see \ref KLFLibResourceEngine)
   * \note \ref KLFLibResourceEngine::entryId is a typedef of \ref KLFLib::entryId. */
  typedef qint32 entryId;
}


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
 * \warning I may want to change the API for this class in a future version of KLatexFormula,
 *   to perform more specific entry querying, and to avoid all items having to be pre-loaded.
 *   To ensure this simple API will remain available after such a rewrite, subclass the
 *   \ref KLFLibResourceSimpleEngine class instead: for now, it is functionless, but in the
 *   future it will (if I actually do the API change) provide an interface between the new
 *   API (in the background) while continuing to provide the current API for subclassing.
 *
 * Resources have properties (stored in a KLFPropertizedObject structure, NOT in regular QObject
 * properties) that ARE STORED IN THE RESOURCE DATA ITSELF. Built-in properties are listed
 * in the \ref ResourceProperty enum. Some properties may be implemented read-only
 * (eg. \ref PropAccessShared) in which case saveResourceProperty() should return FALSE.
 *
 * The view used to display this resource is stored in the "ViewType" resource property (and
 * thus should be stored in the backend data like other properties).
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
 * Note that recognized query items (and <i>only</i> the recognized query items) are stripped
 * from the \ref url() as they are parsed.
 *
 * <b>NOTES FOR SUBCLASSES</b><br>
 * - Subclasses must deal with the properties and implement them as they are meant to, and
 *   as they have been documented in their respective functions (see \ref locked(), \ref title()).
 *   For example, if a resource is \ref locked(), then \ref canModify() must return FALSE and
 *   any attempt to modify the resource must fail.
 * - Subclasses must reimplement \ref saveResourceProperty() to save the new property value
 *   to the backend data, or return FALSE to cancel the resource change. Calls to
 *   \ref setLocked(), \ref setTitle() or setViewType() all call in turn \ref setResourceProperty(),
 *   the default implementation of which is usually sufficient, which calls in turn
 *   \ref saveResourceProperty().
 * - See also docs for \ref setResourceProperty() and \ref saveResourceProperty().
 * - The \ref resourcePropertyChanged() is emitted in the default implementation of
 *   \ref setResourceProperty().
 */
class KLF_EXPORT KLFLibResourceEngine : public QObject, public KLFPropertizedObject
{
  Q_OBJECT
public:
  typedef KLFLib::entryId entryId;
  
  struct KLFLibEntryWithId {
    KLFLibEntryWithId(entryId i = -1, const KLFLibEntry& e = KLFLibEntry())
      : id(i), entry(e) { }
    entryId id;
    KLFLibEntry entry;
  };

  /** List of built-in KLFPropertizedObject-properties. */
  enum ResourceProperty {
    PropTitle = 0,
    PropLocked,
    PropViewType,
    PropAccessShared
  };

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
  /** If the resource is locked (property \ref PropLocked), no changes can be in principle made,
   * except <tt>setLocked(false)</tt> (of course...).
   *
   * The default implementation has some limited support for the locked property: see
   * \ref setLocked(). */
  virtual bool locked() const { return KLFPropertizedObject::property(PropLocked).toBool(); }

  //! The (last) View Type used to display this resource
  virtual QString viewType() const
  { return KLFPropertizedObject::property(PropViewType).toString(); }

  //! If the resource is accessed by many clients
  /** Whether the resource is meant to be shared by many clients (eg. a remote database access)
   * or if it is local and we're the only one to use it (eg. a local sqlite file).
   *
   * This property may be used by views to decide if they can store meta-information about the
   * view state in the resource itself (icon positions in icon view mode for example). */
  virtual bool accessShared() const
  { return KLFPropertizedObject::property(PropAccessShared).toBool(); }

  //! Get the value of a resource property
  /** Returns the value of the given resource property. If the property is not registered,
   * returns an invalid QVariant with a warning.
   *
   * For built-in resource properties, consider using the simpler \ref locked(), \ref title()
   * and \ref viewType() methods (for example). */
  virtual QVariant resourceProperty(const QString& name) const;

  enum ModifyType { AllActionsData, InsertData, ChangeData, DeleteData };
  /** Subclasses should return TRUE here if it is possible to modify the resource's data in
   * the way described by \c modifytype. Return for ex. false when opening a read-only file.
   *
   * \warning subclasses should call the base implementation to e.g. take into account
   * the \ref locked() property. If the base implementation returns FALSE, then the
   * subclasses should also return FALSE. */
  virtual bool canModifyData(ModifyType modifytype) const;

  /** Subclasses should return TRUE here if it is possible to modify the resource's property
   * identified by \c propId. Return for ex. false when opening a read-only file.
   *
   * \warning subclasses should call the base implementation to e.g. take into account
   * the \ref locked() property and \ref isReadOnly() state. If the base implementation
   * returns FALSE, then the subclasses should also return FALSE.
   *
   * If \c propId is negative, then a more general "Can we generally speaking modify properties
   * right now?" is returned.
   *
   * \note The base implementation returns TRUE if the resource is locked but if \c propId is
   * \ref PropLocked (to be able to un-lock the resource!) */
  virtual bool canModifyProp(int propId) const;

  /** Subclasses should return TRUE if they can handle a new resource property that they
   * (maybe) haven't yet heard of, and if they can handle saving and loading the property
   * as a \ref QVariant to and from the backend data.
   * 
   * Call \ref loadResourceProperty() to actually register the property and save a value
   * to it. */
  virtual bool canRegisterProperty(const QString& propName) const;

  /** Suggested library view widget to view this resource with optimal user experience :-) .
   * Return QString() for default view. */
  virtual QString suggestedViewTypeIdentifier() const { return QString(); }

  /** Returns the entry corresponding to ID \c id, or an empty KLFLibEntry() if the \c id
   * is not valid. */
  virtual KLFLibEntry entry(entryId id) = 0;
  /** Returns all the entries in this library resource, with their corresponding IDs. */
  virtual QList<KLFLibEntryWithId> allEntries() = 0;
  
signals:
  //! Emitted when a resource property changes.
  void resourcePropertyChanged(int propId);
  
  //! Emitted when data has changed
  /** This signal is emitted whenever data changes in the model (eg. due to an \ref insertEntries()
   * function call).
   *
   * The entries that were changed are given in the argument \c entryIdList. An empty list
   * means either the library resource changed completely, or simply the backend does not wish
   * to privide any information on which entries changed. In any case, the receiver should
   * consider all previously read data from the resource as out of date and refresh all.
   */
  void dataChanged(const QList<KLFLib::entryId>& entryIdList);
  
public slots:

  //! set a new resource title for this library resource
  /** Stores the new resource title. The title is a human string to display eg. in the
   * tabs of the \ref KLFLibBrowser library browser.
   *
   * This function calls directly \ref setResourceProperty().
   *
   * The default implementation should suffice; subclass \ref savePropertyResource()
   * for actually saving the new value into the resource backend data, and for more control
   * over setting properties, or possibly subclass \ref setPropertyResource() for even more
   * control.
   *  */
  virtual bool setTitle(const QString& title);

  //! Set the resource to be locked
  /** See \ref locked() for more doc.
   *
   * If the feature flags do not contain \ref FeatureLocked, then this function returns
   * immediately FALSE. Otherwise, this function calls \ref setResourceProperty() with
   * arguments \ref PropLocked and the new value.
   *
   *
   * The default implementation should suffice; subclass \ref savePropertyResource()
   * for actually saving the new value into the resource backend data, and for more control
   * over setting properties, or possibly subclass \ref setPropertyResource() for even more
   * control.
   */
  virtual bool setLocked(bool locked);

  /** Store the given view type in the ViewType property.
   *
   * Calls directly \ref setResourceProperty().
   *
   * The default implementation should suffice; subclass \ref savePropertyResource()
   * for actually saving the new value into the resource backend data, and for more control
   * over setting properties, or possibly subclass \ref setPropertyResource() for even more
   * control.
   * */
  virtual bool setViewType(const QString& viewType);

  //! Set the resource to be read-only or not
  /** If the \ref FeatureReadOnly was given to the constructor, The base implementation sets
   * the read-only flag to \c readonly (this is then returned by \ref isReadOnly()). The
   * base implementation does nothing and returns FALSE if the feature \ref FeatureReadOnly is
   * not supported.
   */
  virtual bool setReadOnly(bool readonly);

  //! (shortcut for the list version)
  /** This function is already implemented as shortcut for calling the list version. Subclasses
   * need in principle NOT reimplement this function. */
  virtual entryId insertEntry(const KLFLibEntry& entry);

  //! Insert new entries in this resource.
  /** Inserts the given library entries (given as a \ref KLFLibEntry list) into this resource
   * and returns a list of the IDs that were attributed to the new entries (in the same
   * order as the given list). An individual ID of -1 means failure for that specific entry;
   * an empty list returned means a general failure, except if \c entrylist is empty.
   *
   * This function should be reimplemented by subclasses to actually save the new entries.
   * The reimplementation should make sure that the operation is permitted (eg. by checking
   * that \ref canModifyData(InsertData) is true, and should behave as described above.
   *
   * Subclasses should then emit the \ref dataChanged() signal, and return a success/failure
   * code.
   */
  virtual QList<entryId> insertEntries(const KLFLibEntryList& entrylist) = 0;
  //! Change some entries in this resource.
  /** The entries specified by the ids \c idlist are modified. The properties given
   * in \c properties (which should be KLFLibEntry property IDs) are to be set to the respective
   * value in \c values for all the given entries in \c idlist. \c properties and \c values must
   * obviously be of same size.
   *
   * A return value of TRUE indicates general success and FALSE indicates a failure.
   *
   * This function should be reimplemented by subclasses to actually save the modified entries.
   * The reimplementation should make sure that the operation is permitted (eg. by checking
   * that \ref canModifyData(ChangeData) is true, and should behave as described above.
   *
   * Subclasses should then emit the \ref dataChanged() signal, and return a success/failure
   * code.
   */
  virtual bool changeEntries(const QList<entryId>& idlist, const QList<int>& properties,
			     const QList<QVariant>& values) = 0;
  //! Delete some entries in this resource.
  /** The entries specified by the ids \c idlist are deleted.
   *
   * A return value of TRUE indicates general success and FALSE indicates a failure.
   *
   * This function should be reimplemented by subclasses to actually delete the entries.
   * The reimplementation should make sure that the operation is permitted (eg. by checking
   * that \ref canModify(DeleteData) is true, and should behave as described above.
   *
   * Subclasses should then emit the \ref dataChanged() signal, and return a success/failure
   * code.
   */
  virtual bool deleteEntries(const QList<entryId>& idlist) = 0;

  /** If the \ref FeatureSaveAs is supported (passed to the constructor), reimplement this
   * function to save the resource data in the new path specified by \c newPath.
   *
   * The \c newPath must be garanteed to have same schema as the previous url.
   *
   * \bug ................... API to define: this function is called 'saveAs' however, I think
   *   its implementation should keep the old path open (easier to implement). What to do? rename
   *   to saveAsCopy() ? open new path ? keep as is and clearly document ?
   * */
  virtual bool saveAs(const QUrl& newPath);

  //! Set a resource property to the given value
  /** This function calls in turn:
   * - \ref canModifyProp() to check whether the property can be modified
   * - \ref saveResourceProperty() to check whether the operation is permitted (the value
   *   is acceptable, or any other check the subclass would want to perform) and to save
   *   the given value to the backend data;
   * - \ref KLFPropertizedObject::setProperty() to save the property locally;
   * - emits \ref resourcePropertyChanged() to notify other classes of the property change.
   *
   * \note This function fails immediately if \c propId is not a valid registered
   *   property ID, or if canModifyProp() returns FALSE.
   * */
  virtual bool setResourceProperty(int propId, const QVariant& value);

  //! Set the given property to the given value
  /** Very similar to calling \ref setResourceProperty() with the respective property
   * ID (see \ref KLFPropertizedObject::propertyIdForName()), except the property is
   * registered if it does't exist.
   *
   * The property is registered if it doesn't exist yet and the \ref canRegisterProperty()
   * returns TRUE.
   *
   * Once the property has been possibly registered, setResourceProperty() is called.
   */
  virtual bool loadResourceProperty(const QString& propName, const QVariant& value);


protected:

  //! Save a resource property to the backend resource data.
  /** Subclasses should reimplement this function to save the value of the given resource
   * property to the knew given value.
   *
   * If the new value is inacceptable, or if the operation is not permitted, the subclass
   * should return FALSE, in which case the resource property will not be changed.
   *
   */
  virtual bool saveResourceProperty(int propId, const QVariant& value) = 0;

private:
  void initRegisteredProperties();

  QUrl pUrl;
  uint pFeatureFlags;
  bool pReadOnly;
};


Q_DECLARE_METATYPE(KLFLibResourceEngine::KLFLibEntryWithId)
  ;


QDataStream& KLF_EXPORT operator<<(QDataStream& stream,
				   const KLFLibResourceEngine::KLFLibEntryWithId& entrywid);
QDataStream& KLF_EXPORT operator>>(QDataStream& stream,
				   KLFLibResourceEngine::KLFLibEntryWithId& entrywid);


/** \brief Provides a simple API for reading library resources.
 *
 * Right now already KLFLibResourceEngine requires a simple API subclassing for reading
 * resources. However in the future I may wish to change that API for providing more specific
 * entry querying (by category / by any property type / ....) to avoid full caching of all
 * entries (which is for ex. not useful when displaying a huge library resource in a category
 * tree). In the event of an API modification, this class will still provide the simple
 * approach with \ref allEntries() and \ref entry().
 *
 */
class KLF_EXPORT KLFLibResourceSimpleEngine : public KLFLibResourceEngine
{
  Q_OBJECT
public:
  KLFLibResourceSimpleEngine(const QUrl& url, uint supportedfeatureflags, QObject *parent = NULL)
    : KLFLibResourceEngine(url, supportedfeatureflags, parent)
  {
  }
  virtual ~KLFLibResourceSimpleEngine() { }

  /** Returns the entry corresponding to ID \c id, or an empty KLFLibEntry() if the \c id
   * is not valid. */
  virtual KLFLibEntry entry(entryId id) = 0;
  /** Returns all the entries in this library resource, with their corresponding IDs. */
  virtual QList<KLFLibEntryWithId> allEntries() = 0;

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
   * <tt>param["klfRetry"]=true</tt> will cause the dialog not to exit but re-prompt user to
   * possibly change his input (could result from user clicking "No" in a "Overwrite?"
   * dialog presented in a reimplementation of this function).
   * <tt>param["klfCancel"]=true</tt> will cause the `create resource' process to be cancelled.
   *
   * If an empty Parameters() is returned, it is also interpreted as a cancel operation.
   * */
  virtual Parameters retrieveCreateParametersFromWidget(const QString& scheme, QWidget *widget);
  /** Actually create the new resource, with the given settings. Open the resource and return
   * the KLFLibResourceEngine object. */
  virtual KLFLibResourceEngine *createResource(const QString& scheme, const Parameters& parameters,
					       QObject *parent = NULL);


  virtual bool canResourceSaveAs(const QString& scheme) const;
  virtual QWidget *createPromptSaveAsWidget(QWidget *parent, const QString& scheme,
					    const QUrl& defaultUrl);
  virtual QUrl retrieveSaveAsUrlFromWidget(const QString& scheme, QWidget *widget);


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





#endif

