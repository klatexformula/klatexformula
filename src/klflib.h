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
#include <klffactory.h>


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

KLF_EXPORT QString prettyPrintStyle(const KLFStyle& sty);

typedef QList<KLFStyle> KLFStyleList;

KLF_EXPORT QDataStream& operator<<(QDataStream& stream, const KLFStyle& style);
KLF_EXPORT QDataStream& operator>>(QDataStream& stream, KLFStyle& style);
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
  /** This constructor extracts the legacy-style category and tags from latex, and
   * stores latex with those tags stripped. */
  KLFLibEntry(const QString& latex, const QDateTime& dt, const QImage& preview,
	      const KLFStyle& style);
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


  /** Parses and returns legacy-style category comment string from latex string
   * in the form <pre>%: Category</pre> */
  static QString categoryFromLatex(const QString& latex);
  /** Parses and returns legacy-style tags comment string from latex string */
  static QString tagsFromLatex(const QString& latex);
  /** Removes legacy-style category and tags comment from latex string */
  static QString stripCategoryTagsFromLatex(const QString& latex);

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
   *
   * \note This type is signed, and may be assigned a negative value. Negative values
   *   should only indicate invalid IDs. Valid IDs are always positive.
   *
   * \note \ref KLFLibResourceEngine::entryId is a typedef of \ref KLFLib::entryId.
   *
   * */
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
 * Library resources provide entries without any particular order.
 *
 * Library entries are communicated with KLFLibEntry objects. Each entry in the resource must
 * be attributed a numerical ID, which is unique within the resource, in particular not
 * necessarily globally unique among all open resources. For engines implementing the
 * \ref FeatureSubResources feature, the attributed IDs have only to be unique within the
 * sub-resource the entry lives in.
 *
 * Entries are queried by calling for example the functions: \ref allEntries(), \ref entries(),
 * \ref entry(). See the individual function documentation for more details.
 *
 * \warning The (already somewhat complicated) API for this class might be enhanced in the
 * future to support more optimized interaction. To ensure compatibility with API changes,
 * subclass the \ref KLFLibResourceSimpleEngine instead which implements the simplest API, and
 * wraps calls to the more sophisticated functions provided by the KLFLibResourceEngine API.
 *
 * <b>Resource Properties</b>. Resources have properties (stored in a
 * KLFPropertizedObject structure, NOT in regular QObject
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
 * Subclasses may prevent read-only mode by not specifying
 * the FeatureReadOnly in the constructor. In this case the default implementation of
 * \ref setReadOnly() will return FALSE without doing anything. Also in this case, any
 * occurrence of the query item "klfReadOnly" in the URL is stripped and ignored.
 *
 * <b>Sub-Resources</b>. Sub-resources are a way of logically separating the contents of
 * a resource into separate so-called "sub-resources", like for example a library could
 * contain a sub-resource "History", "Archive" etc. Derived subclasses don't have to
 * implement subresources, they can specify if subresources are available using the
 * FeatureSubResources flag in the constructor. In classes implementing sub-resources,
 * the <i>default sub-resource</i> (queried by \ref defaultSubResource() and set by
 * \ref setDefaultSubResource()) is the resource that the \ref insertEntry(),
 * \ref changeEntries(), etc.
 * family functions will access when the variant of the function without sub-resource
 * argument is called.
 *
 * <b>Sub-Resource Properties</b>. Sub-resources, when implemented by the subclass, may
 * each have properties (the API would suggest subclasses to store these property values
 * into \ref KLFPropertizedObject's). The properties (structure+values) are NOT stored
 * by KLFLibResourceEngine itself, all the work must be done by the subclass, in particular
 * reimplementing \ref subResourceProperty() and \ref setSubResourceProperty(). Again this
 * feature is implemented by the subclass if and only if the subclass specified
 * \ref FeatureSubResourceProps in the constructor argument. In this case note also that views
 * may for example expect the resource to provide values for the sub-resource properties defined
 * by \ref SubResourceProperty.
 *
 * <b>Query Items in URL</b>. The URL may contain Query Items e.g.
 * <tt>scheme://klf.server.dom/path/to/location<b>?klfReadOnly=true</b></tt>. Here is a
 * list of recognized (universal) query items. Subclasses may choose to recognize more query
 * items, but these listed here are detected in the KLFLibResourceEngine constructor and
 * the appropriate flags are set (eg. \ref isReadOnly()):
 * - <tt>klfReadOnly=</tt>{<tt>true</tt>|<tt>false</tt>} If provided, sets the read-only
 *   flag to the given value. If not provided, the flag defaults to false (flag is directly
 *   set, bypassing the setReadOnly() function).
 * - <tt>klfDefaultSubResource=<i>sub-resource name</i></tt> If provided, specifies which
 *   sub-resource (which has to be present in the given location) should be opened by default.
 *   See \ref setDefaultSubResource().
 * .
 * Note that recognized query items (and <i>only</i> the query items recognized at the
 * KLFLibResourceEngine level) are stripped from the \ref url() as they are parsed. Subclasses
 * may choose to (independently) recognize and strip other query items from URL if it is relevant
 * to do so.
 *
 * <b>NOTES FOR SUBCLASSES</b><br>
 * - Subclasses must deal with the properties and implement them as they are meant to, and
 *   as they have been documented in their respective functions (see \ref locked(), \ref title()).
 *   For example, if a resource is \ref locked(), then \ref canModifyData() and \ref canModifyProp()
 *   must return FALSE and any attempt to modify the resource must fail.
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
  
  //! A KLFLibEntry in combination with a KLFLib::entryId
  struct KLFLibEntryWithId {
    KLFLibEntryWithId(entryId i = -1, const KLFLibEntry& e = KLFLibEntry())
      : id(i), entry(e) { }
    entryId id;
    KLFLibEntry entry;
  };

  /** List of built-in KLFPropertizedObject-properties. See
   * \ref KLFLibResourceEngine "class documentation" and \ref setResourceProperty(). */
  enum ResourceProperty {
    PropTitle = 0,
    PropLocked,
    PropViewType,
    PropAccessShared
  };

  /** List of pre-defined properties that can be applied to sub-resources when the features
   * \ref FeatureSubResources and \ref FeatureSubResourceProps are implemented. See also
   * \ref setSubResourceProperty(). */
  enum SubResourceProperty {
    SubResPropTitle = 0,
    SubResPropLocked,
    SubResPropViewType
  };

  //! Features that may or may not be implemented by subclasses
  /** These binary codes specify features that a subclass may or may not implement.
   *
   * The subclass provides the binary combination of the exact features it supports in the
   * constructor argument in \ref KLFLibResourceEngine().
   *
   * The flags can be accessed with a call to \ref supportedFeatureFlags(). They cannot be
   * modified after the constructor call.
   *
   * See this \ref KLFLibResourceEngine "class documentation" for more details.
   */
  enum ResourceFeature {
    //! Open in Read-Only mode
    /** Flag indicating that this resource engine supports opening a location in read-only mode.
     * This resource explicitely checks that we're not \ref isReadOnly() before modifying the
     * resource contents. */
    FeatureReadOnly		= 0x0001,
    //! Lock the resource
    /** Flag indicating that this resource engine supports the resource property \ref PropLocked,
     * can modify that property and checks that we're not \ref locked() before modifying the
     * resource contents.
     *
     * Resources supporting feature \ref FeatureSubResourceProps, in combination with this feature
     * (FeatureLocked) are also expected to treat properly locked sub-resources (ie. sub-resource
     * property \ref SubResPropLocked). */
    FeatureLocked		= 0x0002,
    //! Implements the \ref saveTo() function
    /** Flag indicating that this resource engine implements the saveTo() function, ie. that
     * calling it has a chance that it will not fail */
    FeatureSaveTo		= 0x0004,
    //! Data can be stored in separate sub-resources
    /** Flag indicating that this resource engine supports saving and reading data into different
     * "sub-resources". See \ref KLFLibResourceEngine "class documentation" for details. */
    FeatureSubResources		= 0x0008,
    //! Sub-Resources may be assigned properties and values
    /** Flag indicating that this resource engine supports saving and reading sub-resource
     * property values. This flag makes sense only in combination with \c FeatureSubResources.
     * Note that views may assume that implementing sub-resource properties means also providing
     * sensible values and/or loaded/stored values for the built-in sub-resource properties
     * described in the \ref SubResourceProperty enum. */
    FeatureSubResourceProps	= 0x0010,
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

  //! Query URL
  /** Returns the identifying URL of this resource.
   *
   * \note This is not quite the URL passed to the constructor. Some query items are recognized
   *   and stripped. See the \ref KLFLibResourceEngine "Class Documentation" for more information.
   */
  virtual QUrl url() const { return pUrl; }

  //! Query read-only state
  /** See \ref KLFLibResourceEngine "Class Documentation" for more details. */
  virtual bool isReadOnly() const { return pReadOnly; }

  //! The human-set title of this resource
  /** See \ref setTitle(). */
  virtual QString title() const { return KLFPropertizedObject::property(PropTitle).toString(); }

  //! Is this resource is locked?
  /** If the resource is locked (property \ref PropLocked), no changes can be in principle made,
   * except <tt>setLocked(false)</tt> (of course...).
   *
   * Note: the subclass is responsible to check in functions such as insertEntry(), that the
   * resource is not locked and not read-only etc. (checked all-in-one with \ref canModifyData()).
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

  enum ModifyType {
    AllActionsData = 0,
    UnknownModification = 0,
    InsertData,
    ChangeData,
    DeleteData
  };

  /** Returns TRUE here if it is possible to modify the resource's data in the way described
   * by \c modifytype (see \ref ModifyType enum).
   *
   * Engines supporting feature \ref FeatureSubResources should return TRUE or FALSE whether
   * it is possible to perform the requested action in the given sub-resource \c subResource.
   * Other engines should ignore the \c subResource parameter.
   *
   * For engines supporting all of \ref FeatureSubResources, \ref FeatureSubResourceProps, and
   * \ref FeatureLocked the default implementation checks whether the sub-resource is locked
   * by querying \ref subResourceProperty(\ref SubResPropLocked).
   *
   * \c modifytype must be one of AllActionsData, InsertData, ChangeData, DeleteData.
   *
   * The base implementation returns TRUE only if the current resource is not read-only (see
   * \ref isReadOnly()) and is not locked (see \ref locked()). Subclasses should reimplement
   * this function to add more sophisticated behaviour, eg. checking that an open file is not
   * read-only, or that a DB user was granted the correct rights, etc.
   *
   * \warning subclasses should call the base implementation to e.g. take into account
   * eg. the \ref locked() property. If the base implementation returns FALSE, then the
   * subclasses should also return FALSE. */
  virtual bool canModifyData(const QString& subResource, ModifyType modifytype) const;

  /** This function is provided for convenience for resource not implementing
   * \ref FeatureSubResources, or Views relying on the default sub-resource.
   *
   * \warning Subclasses should normally not reimplement this function, but rather the
   *   canModifyData(const QString&, ModifyType) function, even if they do not support
   *   sub-resources!
   *
   * The default implementation of this function (which should be sufficient in a vast majority
   * of cases) calls
   * \code
   *  canModifyData(defaultSubResource(), modifytype)
   * \endcode
   * as one would expect.
   */
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

  /** Returns TRUE if this resource supports sub-resources and contains a sub-resource
   * named \c subResource.
   *
   * The default implementation checks that the \ref FeatureSubResources is supported, and
   * that \ref subResourceList() contains \c subResource (with exact string match).
   *
   * If subclasses have a more optimized method to check this, they may (but need not
   * necessarily) reimplement this function to optimize it.
   */
  virtual bool hasSubResource(const QString& subResource) const;

  /** Returns a list of sub-resources in this resource. This function is relevant only
   * if the \ref FeatureSubResources is implemented. */
  virtual QStringList subResourceList() const { return QStringList(); }

  /** Returns the default sub-resource, ie. the sub-resource to access if eg. the variant
   * of insertEntry() without the sub-resource argument is called.  */
  virtual QString defaultSubResource();

  /** Returns TRUE if we can create a new sub-resource in this resource.
   *
   * This function can be reimplemented if the resource supports feature \ref FeatureSubResources.
   *
   * The default implementation returns FALSE.
   */
  virtual bool canCreateSubResource() const;

  /** Returns TRUE if we can give the sub-resource \c subResource a new name. More specifically,
   * returns TRUE if \ref renameSubResource() on that sub-resource has chances to succeed.
   *
   * This function can be reimplemented if the resource supports feature \ref FeatureSubResources.
   *
   * The default implementation returns FALSE.
   */
  virtual bool canRenameSubResource(const QString& subResource) const;


  /** Queries properties of sub-resources. The default implementation returns an empty
   * QVariant(). Test the \ref supportedFeatureFlags() for \ref FeatureSubResourceProps to
   * see if this feature is implemented. In particular, subclasses should specify whether
   * they implement this feature by passing the correct feature flags. */
  virtual QVariant subResourceProperty(const QString& subResource, int propId) const;

  /** Returns a list of sub-resource properties that are supported by this resource.
   *
   * This function makes sense to be reimplemented and called only if this resource supports
   * features \ref FeatureSubResources and \ref FeatureSubResourceProps.
   *
   * The default implementation returns an empty list.
   */
  virtual QList<int> subResourcePropertyIdList() const { return QList<int>(); }

  /** Returns a property name for the sub-resource property \c propId.
   *
   * The name should be significant but machine-compatible (in particular, NOT translated) as
   * it could be used to identify the property for example to save it possibly...
   *
   * the default implementation returns reasonable names for \c SubResPropTitle,
   * \c SubResPropLocked, and \c SubResPropViewType, and returns the \c propId in a string
   * for other IDs (\ref QString::number()).
   *
   * This function makes sense to be called only for engines supporting
   * \ref FeatureSubResourceProps.
   */
  virtual QString subResourcePropertyName(int propId) const;

  /** Returns TRUE if the property \c propId in sub-resource \c subResource can be modified.
   *
   * This function should not be called or reimplemented for classes not supporting feature
   * \ref FeatureSubResourceProps.
   *
   * The default implementation provides a basic functionality based on \ref baseCanModifyStatus().
   * If the modifiable status of the sub-resource \c subResource is \ref MS_CanModify, or if the
   * status is \ref MS_IsLocked but the \c propId we want to modify is \ref SubResPropLocked, this
   * function returns TRUE. In other cases it returns FALSE.
   *
   * In short, it behaves as you would expect it to: protect against modifications but allow
   * un-locking of a locked sub-resource.
   */
  virtual bool canModifySubResourceProperty(const QString& subResource, int propId) const;

  //! Query an entry in this resource
  /** Returns the entry (in the sub-resource \c subResource) corresponding to ID \c id, or an
   * empty KLFLibEntry() if the \c id is not valid.
   *
   * \note Subclasses must reimplement this function to behave as described here.
   *
   * For resources implementing \ref FeatureSubResources, the \c subResource argument specifies
   * the sub-resource in which the entry should be fetched. Classes not implementing this feature
   * should ignore this parameter.
   *  */
  virtual KLFLibEntry entry(const QString& subResource, entryId id) = 0;

  //! Query an entry in this resource
  /** Returns the entry (for classes implementing the \ref FeatureSubResources, queries the default
   * sub-resource) corresponding to ID \c id, or an empty KLFLibEntry() if the \c id is not valid.
   *
   * The default implementation calls \ref entry(const QString& subResource, entryId id) with
   * the default subresource as argument. See \ref setDefaultSubResource().
   */
  virtual KLFLibEntry entry(entryId id);

  //! Query the existence of an entry in this resource
  /** Returns TRUE if an entry with entry ID \c id exists in this resource, in the sub-resource
   * \c subResource, or FALSE otherwise.
   *
   * \note Subclasses must reimplement this function to behave as described here.
   */
  virtual bool hasEntry(const QString& subResource, entryId id) = 0;

  //! Query the existence of an entry in this resource
  /** Returns TRUE if an entry with entry ID \c id exists in this resource or FALSE otherwise.
   * Classes implementing the \ref FeatureSubResources will query the default sub-resource, see
   * \ref setDefaultSubResource().
   *
   * The default implementation calls \ref hasEntry(const QString& subResource, entryId id) with
   * the default subresource as argument. See \ref setDefaultSubResource().
   */
  virtual bool hasEntry(entryId id);

  //! Query multiple entries in this resource
  /** Returns a list of \ref KLFLibResourceEngine::KLFLibEntryWithId "KLFLibEntryWithId" s, that
   * is a list of KLFLibEntry-ies with their
   * corresponding IDs, exactly corresponding to the requested entries given in idList. The same
   * order of entries in the returned list as in the specified \c idList is garanteed. For classes
   * implementing sub-resources (\ref FeatureSubResources), the sub-resource \c subResource is
   * queried.
   *
   * \note Subclasses must reimplement this function to behave as described here.
   * */
  virtual QList<KLFLibEntryWithId> entries(const QString& subResource,
					   const QList<KLFLib::entryId>& idList) = 0;

  //! Query multiple entries in this resource
  /** Returns a list of \ref KLFLibEntryWithId's, that is a list of KLFLibEntry-ies with their
   * corresponding IDs, exactly corresponding to the requested entries given in idList. The same
   * order of entries in the returned list as in the specified \c idList is garanteed. For classes
   * implementing sub-resources (\ref FeatureSubResources), the default sub-resource is
   * queried, see \ref setDefaultSubResource().
   *
   * The default implementation calls
   * \ref entries(const QString& subResource, const QList<KLFLib::entryId>& idList) with
   * the default subresource as argument. See \ref setDefaultSubResource().
   */
  virtual QList<KLFLibEntryWithId> entries(const QList<KLFLib::entryId>& idList);

  //! Query all entries in this resource
  /** Returns all the entries in this library resource (in sub-resource \c subResource if
   * \ref FeatureSubResources is supported) with their corresponding IDs.
   *
   * \note Subclasses must reimplement this function to behave as described here.
   */
  virtual QList<KLFLibEntryWithId> allEntries(const QString& subResource) = 0;

  //! Query all entries in this resource
  /** Returns all the entries in this library resource (in default sub-resource if
   * \ref FeatureSubResources is supported) with their corresponding IDs.
   *
   * The default implementation calls \ref allEntries(const QString& subResource) with
   * the default subresource as argument. See \ref setDefaultSubResource().
   */
  virtual QList<KLFLibEntryWithId> allEntries();


  
signals:
  //! Emitted when data has changed
  /** This signal is emitted whenever data changes in the model (eg. due to an \ref insertEntries()
   * function call).
   *
   * The parameter \c subResource is the sub-resource in which the change was observed.
   *
   * The parameter \c modificationType is the type of modification that occured. It is one
   * of \ref InsertData, \ref ChangeData, \ref DeleteData, or \ref UnknownModification. (In
   * particular, sub-classes should not emit other modification types). The \c int type is
   * used for compatibility in Qt's SIGNAL-SLOT mechanism.
   *
   * An \c UnknownModification change means either the library resource changed completely,
   * or simply the backend does not wish to privide any information on which entries changed.
   * In any case, the receiver should consider all previously read data from the resource as
   * out of date and refresh all.
   *
   * The entries that were changed are given in the argument \c entryIdList.
   */
  void dataChanged(const QString& subResource, int modificationType,
		   const QList<KLFLib::entryId>& entryIdList);

  //! Emitted when the default sub-resource changes.
  void defaultSubResourceChanged(const QString& newDefaultSubResource);
  
  //! Emitted when a resource property changes.
  /** \param propId the ID of the property that changed. */
  void resourcePropertyChanged(int propId);
  
  //! Emitted when a sub-resource property changes.
  /** This function should only be emitted by engines supporting feature \ref FeatureSubResourceProps
   * as well as \ref FeatureSubResources.
   *
   * \param subResource the affected sub-resource name
   * \param propId the ID of the property that changed
   */
  void subResourcePropertyChanged(const QString& subResource, int propId);
  
public slots:

  //! set a new resource title for this library resource
  /** Stores the new resource title. The title is a human string to display eg. in the
   * tabs of the \ref KLFLibBrowser library browser.
   *
   * This function calls directly \ref setResourceProperty().
   *
   * The default implementation should suffice; subclass \ref saveResourceProperty()
   * for actually saving the new value into the resource backend data, and for more control
   * over setting properties, or possibly subclass \ref setResourceProperty() for even more
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
   * The default implementation should suffice; subclass \ref saveResourceProperty()
   * for actually saving the new value into the resource backend data, and for more control
   * over setting properties, or possibly subclass \ref setResourceProperty() for even more
   * control.
   */
  virtual bool setLocked(bool locked);

  /** Store the given view type in the ViewType property.
   *
   * Calls directly \ref setResourceProperty().
   *
   * The default implementation should suffice; subclass \ref saveResourceProperty()
   * for actually saving the new value into the resource backend data, and for more control
   * over setting properties, or possibly subclass \ref setResourceProperty() for even more
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

  //! Set the default sub-resource
  /** When calling the variants of insertEntry(), changeEntry(), etc. that do not take a
   * sub-resource argument, and when this resource implements \ref FeatureSubResources, then
   * calls to those functions will access the <i>default sub-resource</i>, which can be
   * set with this function. 
   *
   * This function should be used only with subclasses implementing \ref FeatureSubResources.
   *
   * The default implementation remembers \c subResource to be the new \ref defaultSubResource() and
   * emits \ref defaultSubResourceChanged(). It does nothing if \c subResource is already the default
   * sub-resource.
   *
   * \warning Reimplementing this function has a drawback: it will not be called with the argument
   * given in the constructor (impossible to call reimplemented virtual methods in base constructor).
   * Instead, the sub-class has to explicitely call its own implementation in its own constructor
   * to ensure full proper initialization.
   */
  virtual void setDefaultSubResource(const QString& subResource);

  //! Create a new sub-resource
  /** If they implement the feature \ref FeatureSubResources, subclasses may reimplement this
   * function to create a new sub-resource named \c subResource, with human title \c subResourceTitle.
   *
   * \c subResourceTitle should be ignored for resources not implementing the \ref FeatureSubResourceProps
   * feature.
   *
   * \returns TRUE for success or FALSE for failure.
   *
   * Subclasses should reimplement this function to provide functionality if they wish. The default
   * implementation does nothing and returns FALSE.
   *
   * \note engines not implementing sub-resource properties should also subclass this function and
   *   not the other one, without the \c subResourceTitle parameter. This is because internally this
   *   function is ultimately called in all cases.
   */
  virtual bool createSubResource(const QString& subResource, const QString& subResourceTitle);

  //! Create a new sub-resource
  /** This function is provided for convenience. It calls
   * \code
   *  createSubResource(subResource, QString());
   * \endcode
   * and returns the same result.
   *
   * In other words, call this function if you don't want to provide a resource title, but subclasses
   * should re-implement the _other_ function to make sure full functionality is achieved (in particular
   * also engines not implementing sub-resource properties, which will then simply ignore the title
   * argument). This is because internally the other function is ultimately called in all cases.
   *
   * Again, the default implementation of this function sould be largely sufficient in most cases
   * and thus in principle needs not be reimplemented.
   */
  virtual bool createSubResource(const QString& subResource);

  /** Rename the sub-resource \c oldSubResourceName to \c newSubResourceName.
   *
   * \note we are talking about the resource <i>name</i> (eg. the table name in the database),
   *   not the <i>title</i> (stored as a sub-resource property, for engines supporting
   *   \ref FeatureSubResourceProps).
   *
   * Returns TRUE for success and FALSE for failure.
   *
   * Subclasses may reimplement this function to provide functionality. The default implementation
   * does nothing and returns FALSE.
   *
   * \note Subclasses should also adjust the current defaultSubResource() if that is the one that
   *   was precisely renamed. Use a call to \ref setDefaultSubResource().
   */
  virtual bool renameSubResource(const QString& oldSubResourceName, const QString& newSubResourceName);


  /** Sets the given sub-resource property of sub-resource \c subResource to the value \c value,
   * if the operation is possible and legal.
   *
   * \returns TRUE for success, FALSE for failure.
   *
   * The default implementation does nothing and returns FALSE.
   */
  virtual bool setSubResourceProperty(const QString& subResource, int propId, const QVariant& value);


  //! Insert an entry into this resource
  /** This function is provided for convenience. Inserts the entry \c entry into the
   * subresource \c subResource of this resource. Resources not implementing the sub-resources
   * feature (\ref FeatureSubResources) should ignore the subResource parameter.
   *
   * API users should use the version of \ref insertEntry(const KLFLibEntry& entry) when dealing
   * with resources that do not implement the \ref FeatureSubResources feature.
   *
   * This function is already implemented as shortcut for calling the list version. Subclasses
   * need in principle NOT reimplement this function. */
  virtual entryId insertEntry(const QString& subResource, const KLFLibEntry& entry);

  //! Insert an entry into this resource
  /** This function is provided for convenience. Inserts the entry \c entry into this resource.
   *
   * Use this function for resources not supporting sub-resources (but subclasses still need
   * to reimplement the other function, ignoring the sub-resources argument), or use this
   * function when you explicitely want to use the default sub-resource.
   *
   * A reasonable default implementation is provided. Subclasses are not required to reimplement
   * this function.
   */
  virtual entryId insertEntry(const KLFLibEntry& entry);

  //! Insert new entries in this resource
  /** Inserts the given library entries (given as a \ref KLFLibEntry list) into this resource
   * and returns a list of the IDs that were attributed to the new entries (in the same
   * order as the given list). An individual ID of -1 means failure for that specific entry;
   * an empty list returned means a general failure, except if \c entrylist is empty.
   *
   * The entries are inserted into sub-resource \c subResource, if sub-resources are supported
   * (\ref FeatureSubResources). Otherwise the \c subResource parameter should be ignored
   * by subclasses.
   *
   * This function should be reimplemented by subclasses to actually save the new entries.
   * The reimplementation should make sure that the operation is permitted (eg. by checking
   * that \ref canModifyData() "canModifyData(InsertData)" is true, and should behave as
   * described above.
   *
   * Subclasses should then emit the \ref dataChanged() signal, and return a success/failure
   * code.
   */
  virtual QList<entryId> insertEntries(const QString& subResource, const KLFLibEntryList& entrylist) = 0;

  //! Insert new entries in this resource
  /** This function is provided for convenience. Inserts the entries \c entrylist into this resource.
   *
   * Use this function for resources not supporting sub-resources (but subclasses still need
   * to reimplement the other function, ignoring the sub-resources argument), or use this
   * function when you explicitely want to use the default sub-resource.
   *
   * A reasonable default implementation is provided. Subclasses are not required to reimplement
   * this function.
   */
  virtual QList<entryId> insertEntries(const KLFLibEntryList& entrylist);

  //! Change some entries in this resource.
  /** The entries specified by the ids \c idlist are modified. The properties given
   * in \c properties (which should be KLFLibEntry property IDs) are to be set to the respective
   * value in \c values for all the given entries in \c idlist. \c properties and \c values must
   * obviously be of same size.
   *
   * This function affects entries in sub-resource \c subResource for engines supporting feature
   * \ref FeatureSubResources.
   *
   * A return value of TRUE indicates general success and FALSE indicates a failure.
   *
   * This function should be reimplemented by subclasses to actually save the modified entries.
   * The reimplementation should make sure that the operation is permitted (eg. by checking
   * that \ref canModifyData() "canModifyData(ChangeData)" is true, and should behave as described
   * above.
   *
   * Subclasses should then emit the \ref dataChanged() signal, and return a success/failure
   * code.
   */
  virtual bool changeEntries(const QString& subResource, const QList<entryId>& idlist,
			     const QList<int>& properties, const QList<QVariant>& values) = 0;

  //! Change some entries in this resource
  /** This function is provided for convenience. The default implementation, which should largely
   * suffice in most cases, calls the other changeEntries() function, with the default sub-resource
   * \c defaultSubResource() as first argument.
   *
   * Use this function for resources not supporting sub-resources (but subclasses still need
   * to reimplement the other function, ignoring the sub-resources argument), or use this
   * function when you explicitely want to use the default sub-resource.
   */
  virtual bool changeEntries(const QList<entryId>& idlist, const QList<int>& properties,
			     const QList<QVariant>& values);

  //! Delete some entries in this resource.
  /** The entries specified by the ids \c idlist are deleted.
   *
   * A return value of TRUE indicates general success and FALSE indicates a failure.
   *
   * This function affects entries in sub-resource \c subResource for engines supporting feature
   * \ref FeatureSubResources.
   *
   * This function should be reimplemented by subclasses to actually delete the entries.
   * The reimplementation should make sure that the operation is permitted (eg. by checking
   * that \ref canModifyData() "canModifyData(DeleteData)" is true, and should behave as described
   * above.
   *
   * Subclasses should then emit the \ref dataChanged() signal, and return a success/failure
   * code.
   */
  virtual bool deleteEntries(const QString& subResource, const QList<entryId>& idlist) = 0;

  //! Delete some entries in this resource
  /** This function is provided for convenience. The default implementation, which should largely
   * suffice in most cases, calls the other deleteEntries() function, with the default sub-resource
   * \c defaultSubResource() as first argument.
   *
   * Use this function for resources not supporting sub-resources (but subclasses still need
   * to reimplement the other function, ignoring the sub-resources argument), or use this
   * function when you explicitely want to use the default sub-resource.
   */
  virtual bool deleteEntries(const QList<entryId>& idList);

  /** If the \ref FeatureSaveTo is supported (passed to the constructor), reimplement this
   * function to save the resource data in the new path specified by \c newPath.
   *
   * The \c newPath must be garanteed to have same schema as the previous url.
   */
  virtual bool saveTo(const QUrl& newPath);

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

  /** Whether it's possible to modify \a something (data, property value, etc.)
   */
  enum ModifyStatus {
    MS_CanModify = 0, //!< It's possible to modify that \a something
    MS_IsLocked = 1, //!< That \a something is resource-locked (ie., the only possible action is to unlock it)
    MS_SubResLocked = 2, //!< That \a something is sub-resource-locked
    MS_NotModifiable = 3 //!< That \a something is read-only or not modifiable for another reason
  };

  //! can modify data in resource (base common tests only)
  /** Don't call this function directly, use \ref canModifyData(), \ref canModifyProp(),
   * and \ref canModifySubResourceProperty() instead.
   *
   * The latter and their reimplementations may call this function.
   *
   * This function can be reimplemented to take into account more factors (eg. a file being writable).
   *
   * \returns one value of the \ref ModifyStatus enum. See that documentation for more info.
   *
   * All supporting-feature checks are made correspondingly as needed. If nothing is supported
   * (read-only, locked, sub-resource properties) by default data is modifiable.
   */
  virtual ModifyStatus baseCanModifyStatus(bool inSubResource,
					   const QString& subResource = QString()) const;

private:
  void initRegisteredProperties();

  QUrl pUrl;
  uint pFeatureFlags;
  bool pReadOnly;

  QString pDefaultSubResource;
};


Q_DECLARE_METATYPE(KLFLibResourceEngine::KLFLibEntryWithId)
  ;


KLF_EXPORT QDataStream& operator<<(QDataStream& stream,
				   const KLFLibResourceEngine::KLFLibEntryWithId& entrywid);
KLF_EXPORT QDataStream& operator>>(QDataStream& stream,
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

  // these functions are implemented using the base functions above.

  virtual bool hasEntry(const QString&, entryId id);
  virtual QList<KLFLibEntryWithId> entries(const QString&, const QList<KLFLib::entryId>& idList);

};



/** An abstract factory class for opening resources identified by their URL, and creating
 * objects of the currect subclass of KLFLibResourceEngine.
 *
 * See also \ref KLFLibResourceEngine, \ref KLFLibDBEngine, \ref KLFLibLegacyEngine.
 * More about factory common functions in \ref KLFFactoryManager documentation.
 */
class KLF_EXPORT KLFLibEngineFactory : public QObject, public KLFFactoryBase
{
  Q_OBJECT
public:
  /** A generalized way of passing arbitrary parameters for creating
   * new resources or saving resources to new locations.
   *
   */
  typedef QMap<QString,QVariant> Parameters;

  enum SchemeFunctions {
    FuncOpen   = 0x01, //!< Open Resources
    FuncCreate = 0x02, //!< Create New Resources
    FuncSaveTo = 0x04  //!< Save Resources to new locations
  };

  /** Constructs an engine factory and automatically regisers it. */
  KLFLibEngineFactory(QObject *parent = NULL);
  /** Destroys this engine factory and unregisters it. */
  virtual ~KLFLibEngineFactory();

  /** \brief A list of supported URL schemes this factory can open.
   *
   * If two factories provide a common scheme name, only the last instantiated is used;
   * the consequent ones will be ignored for that scheme name. See
   * \ref KLFFactoryBase::supportedTypes() and \ref KLFFactoryManager::findFactoryFor() */
  virtual QStringList supportedTypes() const = 0;

  /** \brief What this factory is capable of doing
   *
   * Informs the caller of what functionality this factory provides for the given \c scheme.
   *
   * The \c FuncOpen must be provided in every factory.
   *
   * \returns a bitwise-OR of flags defined in the \ref SchemeFunctions enum.
   *
   * The default implementation returns \c FuncOpen. */
  virtual uint schemeFunctions(const QString& scheme) const;

  /** Should return a human (short) description of the given scheme (which is one
   * returned by \ref supportedTypes()) */
  virtual QString schemeTitle(const QString& scheme) const = 0;

  /** Returns the widget type that should be used to present to user to "open" or "create" or
   * "save" a resource of the given \c scheme.
   *
   * For example, both \c "klf+sqlite" and \c "klf+legacy" schemes could return a "LocalFile"
   * widget that prompts to open/create/save-as a file. */
  virtual QString correspondingWidgetType(const QString& scheme) const = 0;

  /** Instantiate a library engine that opens resource stored at \c location. The resource
   * engine should be constructed as a child of object \c parent.
   */
  virtual KLFLibResourceEngine *openResource(const QUrl& location, QObject *parent = NULL) = 0;

  //! Create a new resource of given type and parameters
  /** Create the new resource, with the given settings. This function opens the resource and
   * returns the KLFLibResourceEngine object, which is instantiated as child of \c parent.
   *
   * The parameters' structure are defined for each widget type. That is, if the
   * \ref correspondingWidgetType() for a given scheme is eg. \c "LocalFile", then the
   * parameters are defined by whatever the \c "LocalFile" widget sets. For example
   * \c "LocalFile" documents its parameters in class \ref KLFLibBasicWidgetFactory :
   * parameter \c "Filename" contains a QString with the entered local file name.
   * Additional parameters may also be set by KLFLibCreateResourceDlg itself (eg.
   * default sub-resource name/title, TODO)
   *
   * The default implementation of this function does nothing and returns \c NULL. To enable
   * creating resources, reimplement \ref schemeFunctions() to return also \ref FuncCreate
   * and reimplement this function.
   */
  virtual KLFLibResourceEngine *createResource(const QString& scheme, const Parameters& parameters,
					       QObject *parent = NULL);

  //! Save the given resource to a new location
  /** Save the resource \c resource to the new location given by \c newLocation.
   *
   * The caller should garantee that \c resource is a resource engine of a scheme supported by
   * this factory.
   *
   * \note The resource a the new location is not opened. \c resource will still continue pointing
   *   to the previous location.
   *
   * For example, an \c "klf+sqlite" Sqlite database may choose to simply copy the database file
   * to the new location. This example is implemented in \ref KLFLibDBEngine::saveTo().
   *
   * \returns TRUE for success or FALSE for failure.
   *
   * The default implementation of this function does nothing and returns false. To enable
   * creating resources, reimplement \ref schemeFunctions() to return also \ref FuncCreate
   * and reimplement this function.
   */
  virtual bool saveResourceTo(KLFLibResourceEngine *resource, const QUrl& newLocation);




  /** Finds the last registered factory that should be able to open URL \c url (determined by
   * the URL's scheme) and returns a pointer to that factory.
   *
   * This function is provided for convenience; it is equivalent to
   * \code findFactoryFor(url.scheme()) \endcode
   */
  static KLFLibEngineFactory *findFactoryFor(const QUrl& url);

  /** Finds the last registered factory that should be able to open URLs with scheme \c urlScheme
   * and returns a pointer to that factory. */
  static KLFLibEngineFactory *findFactoryFor(const QString& urlScheme);

  /** Returns a concatenated list of all schemes that all registered factories support */
  static QStringList allSupportedSchemes();

  /** Finds the good factory for URL \c location, and opens the resource using that factory.
   * The created resource will be a child of \c parent. */
  static KLFLibResourceEngine *openURL(const QUrl& location, QObject *parent = NULL);

  /** Opens resource designated by \c url, and then lists the subresources. An empty list
   * is returned if the resource cannot be opened, or if the resource does not support
   * sub-resources.
   *
   * The resource is immedately closed after reading the list of sub-resources.
   *
   * This function:
   * - Finds the relevant factory with \ref findFactoryFor()
   * - Opens the resource with \ref openResource() using that factory
   * - fails if the resource does not support \ref KLFLibResourceEngine::FeatureSubResources
   * - lists the sub-resources
   * - closes the resource
   */
  static QStringList listSubResources(const QUrl& url);

  /** Opens resource designated by \c url, and then lists the subresources with as key
   * the resource name and as value the sub-resource title (if sub-resource properties
   * are supported, or an empty string if not). An empty map is returned if the resource
   * cannot be opened, or if the resource does not support sub-resources.
   *
   * This function works in a very similar way to \ref listSubResources().
   *
   * The resource is immedately closed after reading the list of sub-resources.
   */
  static QMap<QString,QString> listSubResourcesWithTitles(const QUrl& url);

private:
  static KLFFactoryManager pFactoryManager;
};


//! Create Associated Widgets to resources for Open/Create/Save actions
/**
 * Widget-types are associated User Interface widgets with the actions
 * "Open Resource", "Create Resource" and "Save Resource To New Location". The
 * typical example would be to show a file dialog to enter a file name for
 * file-based resources (eg. Sqlite database).
 *
 * Widget-types are not directly one-to-one mapped to url schemes, because
 * for example multiple schemes can be accessed with the same open resource
 * parameters (eg. a file name for both "klf+sqlite" and "klf+legacy"). Thus
 * each scheme tells with <i>Widget-Type</i> it requires to associate with
 * "open" or "create new" or "save to" actions (eg. <tt>wtype=="LocalFile"</tt>
 * or similar logical name).
 */
class KLFLibWidgetFactory : public QObject, public KLFFactoryBase
{
  Q_OBJECT
public:
  /** A generalized way of passing arbitrary parameters for creating
   * new resources.
   *
   * See \ref KLFLibEngineFactory::createResource().
   *
   * Some parameters have special meaning to the system; see
   * \ref retrieveCreateParametersFromWidget().
   */
  typedef KLFLibEngineFactory::Parameters Parameters;
  
  /** Simple constructor. Pass the parent object of this factory as parameter. This
   * can be the application object \c qApp for example. */
  KLFLibWidgetFactory(QObject *parent);

  /** Finds the factory in the list of registered factories that can handle
   * the widget-type \c wtype. */
  static KLFLibWidgetFactory * findFactoryFor(const QString& wtype);

  /** Returns a concatenated list of all supported widget types all registered factories
   * support. */
  static QStringList allSupportedWTypes();


  //! List the supported widget types that this factory can create
  /** eg. "LocalFile", "HostPortUserPass", ... */
  virtual QStringList supportedTypes() const = 0;

  //! The human-readable label for this type of input
  /** Returns a human-readable label that describes the kind of input the widget type
   * \c wtype provides (eg. \c "Local File", \c "Remote Database Connection", etc.)
   */
  virtual QString widgetTypeTitle(const QString& wtype) const = 0;

  /** Create a widget of logical type \c wtype (eg. "LocalFile", ...) that will prompt to user
   * for various data needed to open some given kind of library resource. It could be a file
   * selection widget, or a text entry for a hostname, etc. The widget should present data
   * corresponding to \c defaultlocation as default location if that parameter is non-empty.
   *
   * The widget should have a <tt>void readyToOpen(bool isReady)</tt> signal that is emitted
   * to synchronize the enabled state of the "open" button.
   *
   * The create widget should be a child of \c wparent. */
  virtual QWidget * createPromptUrlWidget(QWidget *wparent, const QString& wtype,
					  QUrl defaultlocation = QUrl()) = 0;

  /** Get the URL edited by user, that are stored in user-interface widgets in \c widget GUI.
   * \c widget is never other than a QWidget returned by \ref createPromptUrlWidget(). */
  virtual QUrl retrieveUrlFromWidget(const QString& wtype, QWidget *widget) = 0;

  /** \returns whether this widget type (\c wtype) can create user-interface widgets to create
   * new resources (eg. a new library sqlite file, etc.) */
  virtual bool hasCreateWidget(const QString& wtype) const;

  /** create a widget that will prompt to user the different settings for the new resource
   * that is about to be created.  Do not reimplement this function if hasCreateWidget()
   * returns false. Information to which URL to store the resource should be included
   * in these parameters!
   *
   * The created widget should be child of \c wparent. */
  virtual QWidget *createPromptCreateParametersWidget(QWidget *wparent, const QString& wtype,
						      const Parameters& defaultparameters = Parameters());
  /** Get the parameters edited by user, that are stored in \c widget GUI.
   *
   * Some special parameters are recognized by the system:
   * * <tt>param["klfScheme"]=<i>url-scheme</i></tt> is MANDATORY: it tells the caller which kind
   *   of scheme the resource should be.
   * * <tt>param["klfRetry"]=true</tt> will cause the dialog not to exit but re-prompt user to
   *   possibly change his input (could result from user clicking "No" in a "Overwrite?"
   *   dialog presented in a possible reimplementation of this function).
   * * <tt>param["klfCancel"]=true</tt> will cause the "create resource" process to be cancelled.
   * * Not directly recognized by the system, but a common standard: <tt>param["klfDefaultSubResource"]</tt>
   *   is the name of the default sub-resource to create if the resource supports sub-resources,
   *   and <tt>param["klfDefaultSubResourceTitle"]</tt> its title, if the resource supports sub-resource
   *   properties.
   *
   * If an empty Parameters() is returned, it is also interpreted as a cancel operation.
   * */
  virtual Parameters retrieveCreateParametersFromWidget(const QString& wtype, QWidget *widget);

  /** Returns TRUE if this widget type (\c wtype) can create user-interface widgets to save an
   * existing open resource as a new name (eg. to an other (new) library sqlite file, etc.) */
  virtual bool hasSaveToWidget(const QString& wtype) const;
  /** Creates a widget to prompt the user to save the resource \c resource to a different location,
   * by default \c defaultUrl.
   *
   * The created widget should be child of \c wparent. */
  virtual QWidget *createPromptSaveToWidget(QWidget *wparent, const QString& wtype,
					    KLFLibResourceEngine *resource, const QUrl& defaultUrl);
  /** Returns the URL to "save to copy", from the data entered by user in the widget \c widget,
   * which was previously returned by \ref createPromptSaveToWidget(). */
  virtual QUrl retrieveSaveToUrlFromWidget(const QString& wtype, QWidget *widget);


private:
  static KLFFactoryManager pFactoryManager;
};



/** Returns the file path represented in \c url, interpreted as an (absolute) path to
 * a local file.
 *
 * Under windows, this ensures that there is no slash preceeding the drive letter, eg.
 * fixes "/C:/..." to "C:/...", but keeps forward-slashes.
 */
KLF_EXPORT QString urlLocalFilePath(const QUrl& url);




#endif

