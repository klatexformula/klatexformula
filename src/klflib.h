/***************************************************************************
 *   file klflib.h
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

#ifndef KLFLIB_H
#define KLFLIB_H

#include <QImage>
#include <QMap>
#include <QUrl>
#include <QDataStream>
#include <QDateTime>
#include <QFileInfo>

#include <klfdefs.h>
#include <klfbackend.h>

#include <klfpobj.h>
#include <klffactory.h>
#include <klfstyle.h>



class KLFProgressReporter;



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
   * when dealing in scopes larger than the running application (saved files, etc.).
   */
  enum PropertyId {
    Latex = 0, //!< The Latex Code of the equation
    DateTime, //!< The Date/Time at which the equation was evaluated
    Preview, //!< An Image Preview of equation (scaled down QImage)
    PreviewSize, //!< A cached value of the size of value in \c Preview
    Category, //!< The Category to which eq. belongs (path-style string)
    Tags, //!< Tags about the equation (string)
    Style //!< KLFStyle style used
  };

  KLFLibEntry(const QString& latex = QString(), const QDateTime& dt = QDateTime(),
	      const QImage& preview = QImage(),	const QSize& previewsize = QSize(),
	      const QString& category = QString(), const QString& tags = QString(),
	      const KLFStyle& style = KLFStyle());
  /** This constructor extracts the legacy-style category and tags from latex, and
   * stores latex with those tags stripped. */
  KLFLibEntry(const QString& latex, const QDateTime& dt, const QImage& preview,
	      const KLFStyle& style);
  KLFLibEntry(const KLFLibEntry& copy);
  virtual ~KLFLibEntry();

  inline QString latex() const { return property(Latex).toString(); }
  inline QDateTime dateTime() const { return property(DateTime).toDateTime(); }
  inline QImage preview() const { return property(Preview).value<QImage>(); }
  inline QSize previewSize() const { return property(PreviewSize).value<QSize>(); }
  inline QString category() const { return property(Category).toString(); }
  inline QString tags() const { return property(Tags).toString(); }
  inline KLFStyle style() const { return property(Style).value<KLFStyle>(); }

  inline QString latexWithCategoryTagsComments() const
  { return latexAddCategoryTagsComment(latex(), category(), tags()); }

  inline void setLatex(const QString& latex) { setProperty(Latex, latex); }
  inline void setDateTime(const QDateTime& dt) { setProperty(DateTime, dt); }
  inline void setPreview(const QImage& img) { setProperty(Preview, img); }
  inline void setPreviewSize(const QSize& sz) { setProperty(PreviewSize, sz); }
  /** \note this function normalizes category to remove any double-'/' to avoid empty sections.
   *    Equality between categories can be compared stringwise.
   *
   * See also \ref normalizeCategoryPath(). */
  inline void setCategory(const QString& s) { setProperty(Category, normalizeCategoryPath(s)); }
  inline void setTags(const QString& s) { setProperty(Tags, s); }
  inline void setStyle(const KLFStyle& style) { setProperty(Style, QVariant::fromValue(style)); }

  /** Set the property named \c name to value \c value. If the property does not
   * yet exist in the registered properties, it is registered.
   *
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
  /** Creates a latex with category and tags comments */
  static QString latexAddCategoryTagsComment(const QString& latex, const QString& category,
					     const QString& tags);
  /** Renders a category-path "pretty" by removing any double-slashes to single slashes.
   * Trailing slashes are removed. The root category is an empty string.
   *
   * When a category is set to a lib-entry with \c setCategory(), it is automatically
   * normalized.
   *
   * Returned paths may be compared string-wise for equality.
   */
  static QString normalizeCategoryPath(const QString& categoryPath);

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


  /** Describes a matching criterion for a string
   *
   * Note: The Qt::MatchWrap and Qt::MatchRecursive flags are ignored.
   */
  struct StringMatch {
    StringMatch(const QVariant& value = QVariant(), Qt::MatchFlags flags = Qt::MatchExactly)
      : mFlags(flags), mValue(value), mValueString(value.toString()) { }
    StringMatch(const StringMatch& m) : mFlags(m.mFlags), mValue(m.mValue), mValueString(m.mValueString) { }

    /** How the match should be tested (exact match, regex, contains, etc.), this is
     * a binary-OR'ed value of Qt::MatchFlag enum values.
     *
     * the Qt::MatchCaseSensitive may be set to match case sensitive.
     */
    inline const Qt::MatchFlags matchFlags() const { return mFlags; }
    /** The value that will be matched to. */
    inline const QVariant matchValue() const { return mValue; }
    /** equivalent to <tt>matchValue().toString()</tt>, however the conversion is
     * performed once and cached. */
    inline const QString matchValueString() const { return mValueString; }

  protected:
    Qt::MatchFlags mFlags;
    QVariant mValue;
    QString mValueString;
  };

  /** Discribes a matching criterion for the value of a property ID. Although not assumed
   * in this definition, property IDs will eventually refer to KLFPropertizedObject property
   * IDs, more specifically \ref KLFLibEntry property IDs.
   */
  struct PropertyMatch : public StringMatch {
    /** propId is the ID of the property which will have to match as given by \c match */
    PropertyMatch(int propId = -1, const StringMatch& match = StringMatch())
      : StringMatch(match), mPropertyId(propId) { }
    /** copy constructor */
    PropertyMatch(const PropertyMatch& other) : StringMatch(other), mPropertyId(other.mPropertyId) { }

    /** Returns the propery ID set in the constructor. */
    inline int propertyId() const { return mPropertyId; }
    
  protected:
    int mPropertyId;
  };

  /** This is a generalized condition. It can be one of several types.
   *
   * In the future, more match condition types may be added. For now, the following
   * matching condition types are supported.
   *  - property string matching (see \ref PropertyMatch)
   *  - AND of N conditions
   *  - OR of N conditions
   *
   * This class is meant for use with findEntries().
   */
  struct EntryMatchCondition
  {
    enum Type {
      MatchAllType = 0, //!< Matches all entries
      PropertyMatchType, //!< Matches a property ID with a string (with a \ref StringMatch)
      NegateMatchType, //!< Matches entries that don't match a condition
      OrMatchType, //!< entries have to match with one of a list of conditions
      AndMatchType //!< entries have to match with all given conditions
    };

    //! Get which type of condition this is
    inline Type type() const { return mType; }
    //! Relevant for type PropertyMatchType
    inline PropertyMatch propertyMatch() const { return mPropertyMatch; }
    //! Relevant for types OrMatchType and AndMatchType
    inline QList<EntryMatchCondition> conditionList() const { return mConditionList; }

    static EntryMatchCondition mkMatchAll();
    static EntryMatchCondition mkPropertyMatch(PropertyMatch pmatch);
    /** Stores \c condition in first element of conditionList(). */
    static EntryMatchCondition mkNegateMatch(const EntryMatchCondition& condition);
    static EntryMatchCondition mkOrMatch(QList<EntryMatchCondition> conditions);
    static EntryMatchCondition mkAndMatch(QList<EntryMatchCondition> conditions);

  protected:
    EntryMatchCondition(Type type) : mType(type) { }

    Type mType;

    PropertyMatch mPropertyMatch;
    QList<EntryMatchCondition> mConditionList;
  };

}

//! Utility class for sorting library entry items
/**
 * This class can be used as a sorter to sort entry items.
 *
 * This class functions as follows:
 * - operator()() calls compareLessThan() with the stored propId() and order().
 * - \ref compareLessThan() in turns calls entryValue() to get string representations of the
 *   values of the property to compare for each entry, and compares the strings with
 *   \ref QString::localeAwareCompare().
 * - \ref entryValue() provides sensible string representations for the given property in the
 *   given entry.
 *
 * \note The column number may be set to be equal \c -1, to indicate an invalid sorter (maybe
 *   to indicate that a list should not be sorted). However, the functions \ref entryValue(),
 *   \ref compareLessThan() and operator()() are NOT meant to be used with an invalid \c propId.
 *   In other words, if isValid() is false (or equivalently, if \ref propId() is negative), don't
 *   attempt any sorting or comparisions.
 *
 * To customize the behaviour of this class, you may subclass it and reimplement any level
 * of its behavior, eg. you may want to just reimplement entryValue() to alter the string
 * representation of the values of some properties, or you could reimplement compareLessThan()
 * to do more sophisticated comparisions.
 *
 * \note There is no copy constructor. If there was, there would be no way of customizing the sort
 *   behavior. To use the entry sorter in contexts where you need to copy an object, use the
 *   clone mechanism with the clone constructor. (Although this should scarcely be needed ...).
 */
class KLF_EXPORT KLFLibEntrySorter
{
public:
  KLFLibEntrySorter(int propId = -1, Qt::SortOrder order = Qt::AscendingOrder);
  /** Used to "circumvent" the copy-constructor issues of reimplemented functions.
   * If an object is constructed using this constructor all calls to entryValue(),
   * compareLessThan(), and operator()() are redirected to clone's.
   *
   * Calls to setPropId() and setOrder() will fail and emit a warning.
   *
   * In particular, never pass 'this' nor 'NULL' as clone!
   */
  KLFLibEntrySorter(const KLFLibEntrySorter *clone);
  virtual ~KLFLibEntrySorter();

  inline bool isValid() const { return (pPropId >= 0); }

  //! The currently set property that will be queried in the items we're sorting
  inline int propId() const { return pPropId; }
  //! The currently set sorting order
  inline Qt::SortOrder order() const { return pOrder; }

  //! Set the KLFLibEntry property id the sort will take into account
  virtual void setPropId(int propId);
  //! Set the sort order
  virtual void setOrder(Qt::SortOrder order);

  //! Returns a string representation of the given property in the given entry
  virtual QString entryValue(const KLFLibEntry& entry, int propId) const;

  //! Compares entries two entries according to the given property and order
  /** Compares the given property of entries \c a and \c b.
   *
   * When \c order is \c Qt::Ascending, if \c a is found to be less than (and not equal) to
   * \c b, then TRUE is returned, otherwise FALSE is returned.
   *
   * If the \c order is \c Qt::Descending, if \c b is found to be less than (and not equal) to
   * \c a, then TRUE is returned, otherwise FALSE is returned.
   */
  virtual bool compareLessThan(const KLFLibEntry& a, const KLFLibEntry& b,
			       int propId, Qt::SortOrder order) const;

  /** This is a wrapper that calls \ref compareLessThan() with the previously set
   * propId() and order().
   */
  virtual bool operator()(const KLFLibEntry& a, const KLFLibEntry& b) const;


private:
  const KLFLibEntrySorter * pCloneOf;

  int pPropId;
  Qt::SortOrder pOrder;
};



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
 * The API in this class for accessing data is pretty elaborate. You may consider to subclass
 * \ref KLFLibResourceSimpleEngine, which requires you to implement a simpler API, but at the
 * price of not being able to take profit of the optimization opportunities given by the
 * more complicated API.
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
 * <b>query Items in URL</b>. The URL may contain query items e.g.
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
 * - lengthy operations should emit the signal \ref operationStartReportingProgress() with a suitable
 *   KLFProgressReporter as necessary to inform callers of progress of operations that can take some
 *   time. This is recommended, but subclasses aren't forced to comply (for laziness' sake for ex...).
 *   However, subclasses should test ONCE per operation whether \ref thisOperationProgressBlocked() and
 *   should not emit progress in that case.
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
   * \ref setSubResourceProperty().
   *
   * \note These properties, by contrast to resource properties, have fixed numerical IDs, since
   *   sub-resource properties are not backed by a KLFPropertizedObject object. */
  enum SubResourceProperty {
    SubResPropTitle    = 0,
    SubResPropLocked   = 1,
    SubResPropViewType = 2
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

  /**
   *
   * \note If the URL \c url needs to be normalized in some way (eg. convert some query item
   * values to lower case, ...), then you need to implement a static function, eg.
   * \code
   * static KLFMyEngine * openURL(const QUrl& url) {
   *   QUrl normalized = url;
   *   // ... normalize URL ...
   *   return new KLFMyEngine(normalized, ...);
   * }
   * KLFMyEngine::KLFMyEngine(const QUrl& url, ...) : KLFLibResourceEngine(url,...)
   * {
   *   // ...
   * }
   * \endcode
   * which will pass the normalized URL here. You cannot change the URL once it has been
   * passed to the constructor. Note however that the stored URL has some elements stripped,
   * as described in the class documentation \ref KLFLibResourceEngine, which you can
   * change via the API (eg. setDefaultSubResource()).
   */
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

  //! Format options one can give to \ref url()
  enum WantUrlFormatFlag {
    //! Add a query item for default sub-res. as \c "klfDefaultSubResource"
    WantUrlDefaultSubResource = 0x01,
    //! Add a query item for read-only status, as \c "klfReadOnly"
    WantUrlReadOnly = 0x02
  };
  //! query URL
  /** Returns the identifying URL of this resource.
   *
   * \note This is not quite the URL passed to the constructor. Some query items are recognized
   *   and stripped. See the \ref KLFLibResourceEngine "Class Documentation" for more information.
   *
   * Flags may be specified as to which query items we would like to add to URL (eg. default
   * sub-resource, read-only status). see \ref WantUrlFormatFlag.
   *
   * See also \ref KLFAbstractLibView::url(). That function gives also information on what part
   * of the resource it displays.
   */
  virtual QUrl url(uint flags = 0x0) const;

  //! Compare this resource's URL with another's
  /** Compares the URL of this resource with \c other, and returns a binary OR'ed value of
   * KlfUrlCompareFlag enum values of tests that have proved true (see KlfUrlCompareFlag
   * for a list of URL-comparision tests in klfutil.h in klftools).
   *
   * This function should return the following flag, if its corresponding tests turn out
   * to be true. This function should NOT return any other flag that is not listed here.
   * - \c KLFUrlComapreBaseEqual should be set if this resource shares its data with the resource
   *   given by URL \c other. Default sub-resources are to be ignored.
   * 
   * The \c interestFlags is a binary OR'ed value of KlfUrlCompareFlag values of tests
   * to be performed. Any flag that is set in \c interestFlags indicates that the return
   * value of this function, when binary-AND'ed with that flag, is the result (T or F)
   * of the test the flag stands for. However, if a flag is not set in \c interestFlags,
   * its state in the return value by this function is undefined.
   *
   * See also klfUrlCompare().
   */
  virtual uint compareUrlTo(const QUrl& other, uint interestFlags = 0xfffffff) const = 0;

  //! query read-only state
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
   *
   * Engines that have case-insensitive resource names (eg. SQL tables) must reimplement
   * this function to do a case-insensitive test.
   */
  virtual bool hasSubResource(const QString& subResource) const;

  /** Returns a list of sub-resources in this resource. This function is relevant only
   * if the \ref FeatureSubResources is implemented. */
  virtual QStringList subResourceList() const { return QStringList(); }

  /** Returns the default sub-resource, ie. the sub-resource to access if eg. the variant
   * of insertEntry() without the sub-resource argument is called.
   *
   * This is relevant only if the engine supports feature \ref FeatureSubResources.
   */
  virtual QString defaultSubResource() const;

  /** \brief Compare our sub-resource name to another
   *
   * Returns TRUE if our default sub-resource name equals \c subResource, FALSE otherwise.
   *
   * The default implementation tests for string equality (case sensitive). If sub-resources
   * in the reimplemented engine are case-insensitive, then reimplement this function to
   * compare string case-insensitive.
   */
  virtual bool compareDefaultSubResourceEquals(const QString& subResourceName) const;

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

  /** Returns TRUE if we can completely delete sub-resource \c subResource from the resource.
   *
   * If this function returns FALSE, a subsequent call to deleteSubResoruce() is certain
   * to fail.
   *
   * The default implementation returns FALSE.
   */
  virtual bool canDeleteSubResource(const QString& subResource) const;

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

  //! query an entry in this resource
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

  //! query an entry in this resource
  /** Returns the entry (for classes implementing the \ref FeatureSubResources, queries the default
   * sub-resource) corresponding to ID \c id, or an empty KLFLibEntry() if the \c id is not valid.
   *
   * The default implementation calls \ref entry(const QString& subResource, entryId id) with
   * the default subresource as argument. See \ref setDefaultSubResource().
   */
  virtual KLFLibEntry entry(entryId id);

  //! query the existence of an entry in this resource
  /** Returns TRUE if an entry with entry ID \c id exists in this resource, in the sub-resource
   * \c subResource, or FALSE otherwise.
   *
   * \note Subclasses must reimplement this function to behave as described here.
   */
  virtual bool hasEntry(const QString& subResource, entryId id) = 0;

  //! query the existence of an entry in this resource
  /** Returns TRUE if an entry with entry ID \c id exists in this resource or FALSE otherwise.
   * Classes implementing the \ref FeatureSubResources will query the default sub-resource, see
   * \ref setDefaultSubResource().
   *
   * The default implementation calls \ref hasEntry(const QString& subResource, entryId id) with
   * the default subresource as argument. See \ref setDefaultSubResource().
   */
  virtual bool hasEntry(entryId id);

  //! query multiple entries in this resource
  /** Returns a list of \ref KLFLibResourceEngine::KLFLibEntryWithId "KLFLibEntryWithId" s, that
   * is a list of KLFLibEntry-ies with their corresponding IDs, exactly corresponding to the
   * requested entries given in idList. The same order of entries in the returned list as in the
   * specified \c idList is garanteed. For classes implementing sub-resources
   * (\ref FeatureSubResources), the sub-resource \c subResource is queried.
   *
   * The KLFLibEntry objects are populated only of the required \c wantedEntryProperties, which is
   * a list of IDs of KLFLibEntry properties (the KLFPropertizedObject-kind properties) that are
   * set in the KLFLibEntry object. The other fields are left undefined. If the property
   * list is empty (by default), then all properties are fetched and set.
   *
   * If in the list an ID is given which does not exist in the resource, a corresponding
   * KLFLibEntryWithId entry is returned in which the \c entry is set to an empty \c KLFLibEntry()
   * and the \c id is set to \c -1.
   *
   * If \c idList is empty, then this function returns an empty list.
   *
   * \note Subclasses must reimplement this function to behave as described here.
   * */
  virtual QList<KLFLibEntryWithId> entries(const QString& subResource,
					   const QList<KLFLib::entryId>& idList,
					   const QList<int>& wantedEntryProperties = QList<int>()) = 0;

  //! query multiple entries in this resource
  /** Returns a list of \ref KLFLibEntryWithId's, that is a list of KLFLibEntry-ies with their
   * corresponding IDs, exactly corresponding to the requested entries given in idList. The same
   * order of entries in the returned list as in the specified \c idList is garanteed. For classes
   * implementing sub-resources (\ref FeatureSubResources), the default sub-resource is
   * queried, see \ref setDefaultSubResource().
   *
   * The KLFLibEntry objects are populated only of the required \c wantedEntryProperties, which is
   * a list of IDs of KLFLibEntry properties (the KLFPropertizedObject-kind properties) that are
   * set in the KLFLibEntry object. The other fields are left undefined. If the property
   * list is empty (by default), then all properties are fetched and set.
   *
   * If in the list an ID is given which does not exist in the resource, a corresponding
   * KLFLibEntryWithId entry is returned in which the \c entry is set to an empty \c KLFLibEntry()
   * and the \c id is set to \c -1.
   *
   * If \c idList is empty, then this function returns an empty list.
   *
   * The default implementation calls
   * \ref entries(const QString& subResource, const QList<KLFLib::entryId>& idList, const QList<int>&)
   * with the default subresource as argument. See \ref setDefaultSubResource().
   */
  virtual QList<KLFLibEntryWithId> entries(const QList<KLFLib::entryId>& idList,
					   const QList<int>& wantedEntryProperties = QList<int>());


  /** \brief A structure that describes a query for query()
   *
   * The following properties should be adjusted (by direct access) before calling query().
   *
   * \c matchcondition is an \ref KLFLib::EntryMatchCondition struct that tells which properties have to be
   * matched, how, and to what value. Only entries that match the match condition set in \c query will
   * be returned by query(). Note that the match condition itself may be a complex condition, like an OR
   * and AND tree of property matching conditions with strings or regexps. See \ref KLFLib::EntryMatchCondition,
   * and \ref KLFLib::PropertyMatch for more info. The idea is that engines can translate such conditions into,
   * eg. a SQL WHERE condition for optimized entry queries.
   *
   * The default match condition (set automatically in constructor) matches all entries.
   *
   * The first \c skip results will be ignored, and the first returned result will be the <tt>skip</tt>'th
   * entry (that is counting from 0; or the <tt>skip+1</tt>'th, counting more intuitively from 1). By default,
   * \c skip is zero, so no entries are skipped. The entries must be skipped \a after they have been sorted.
   * The query() function's return value (entry count) does not include the skipped entries.
   *
   * A \c limit may be set to limit the number of returned results (default is \c -1, meaning no limit).
   *
   * \c orderPropId specifies along which KLFLibEntry property ID the items should be ordered. This
   * can be \c -1 to specify that elements should not be ordered; their order will then be undefined.
   * Default value: \c -1.
   *
   * \c orderDirection specifies in which direction the elements should be ordered. This can be
   * Qt::AscendingOrder or Qt::DescendingOrder (lesser value first or greater value first).
   *
   * \c wantedEntryProperties is a list of properties the lists should be filled with. Elements in the
   * entryWithIdList and rawEntryList lists will only have those properties listed in
   * \c wantedEntryProperties set. The other properties are undefined (some implementations may decide
   * to ignore this optimization). An empty list (which is the default) indicates that all entry
   * properties have to be set.
   */
  struct Query
  {
    /** Default constructor. sets reasonable default values as documented in class doc. */
    Query()
      : matchCondition(KLFLib::EntryMatchCondition::mkMatchAll()),
	skip(0),
	limit(-1),
	orderPropId(-1),
	orderDirection(Qt::AscendingOrder),
	wantedEntryProperties(QList<int>())
    {
    }

    KLFLib::EntryMatchCondition matchCondition;
    int skip;
    int limit;
    int orderPropId;
    Qt::SortOrder orderDirection;
    QList<int> wantedEntryProperties;
  };

  /** \brief A structure that will hold the result of a query() query.
   *
   * This class will contain the entry ID list, the raw entry list and the entry-with-id
   * list of the entries that matched the query() query this object was given to.
   *
   * \c fillFlags may specify which of the aforementioned lists are to be filled (those that are
   * not needed by the caller don't have to be filled, this saves time). You may pass those flags
   * to the constructor.
   *
   * Once the \c fillFlags adjusted, pass a pointer to this object to the query() function to
   * retrieve results.
   *
   * \warning The lists in this object are not garanteed to be cleared at the beginning of
   *   query(). If you recycle this object to call query() a second time, be sure to clean this
   *   object first.
   */
  struct QueryResult
  {
    enum Flags { FillEntryIdList = 0x01, FillRawEntryList = 0x02, FillEntryWithIdList = 0x04 };

    /** Constructor. Sets \c fillFlags as given, and sets reasonable default values for the other
     * members. */
    QueryResult(uint fill_flags = 0x00)  : fillFlags(fill_flags)   {  }
    uint fillFlags;

    QList<KLFLib::entryId> entryIdList;
    KLFLibEntryList rawEntryList;
    QList<KLFLibEntryWithId> entryWithIdList;
  };


  //! query entries in this resource with specified property values
  /** Returns a list of all entries in this resource (and in sub-resource \c subResource
   * for the engines supporting this feature) that match all the required matches specifed
   * in \c matchcondition.
   *
   * \note the subclass is responsible for providing a functional implementation of this function.
   *   Subclasses that don't want to pass much time to implement this feature can resort to calling
   *   the static method \ref KLFLibResourceSimpleEngine::queryImpl() that provides a functional
   *   (but not very optimized) default implementation; the
   *   KLFLibResourceSimpleEngine::testMatchConditionImpl() function may prove useful for testing match
   *   conditions; and the utilities KLFLibEntrySorter and KLFLibResourceSimpleEngine::QueryResultListSorter
   *   might prove useful for sorting entries in conjuction with qLowerBound().
   *
   * \c query is a \ref Query object that describes the query. See \ref Query for details.
   *
   * \c result is a \ref QueryResult structure that will be filled with the queried data. See
   * \ref QueryResult for details.
   *
   * The matches are returned in the following way into the \c result structure:
   *   - if the appropriate flag in \c result is set, then <tt>result.entryIdList</tt> is set to a list
   *     of all entry IDs that matched
   *   - if the appropriate flag in \c result is set, then <tt>result.rawEntryList</tt> is set to a list
   *     of all entries that matched, stored as KLFLibEntry objects. Only the requested properties
   *     are populated (specified by <tt>query.wantedEntryProperties</tt>)
   *   - if the appropriate flag in \c result is set, then <tt>result.entryWithIdList</tt> is set
   *     to a list of all entries, with their corresponding entry ID, that matched (similar to the
   *     return value of allEntries()). Only the requested properties are populated (as specified
   *     by <tt>result.wantedEntryProperties</tt>.)
   *
   * If the \c orderPropId member in \c query is not \c -1, then the returned results (in all
   * to-be-filled lists) are ordered according to the entry property ID \c orderPropId, in the order
   * specified by \c orderDirection.
   *
   * \returns the number of items in the resource that matched. If a \c limit was set in \c query,
   *   then at most that many results are returned. \c 0 is returned if no match was found. \c -1
   *   can be returned to signify an error (eg. invalid regexp, i/o error, etc.).
   *
   * The KLFLibEntry objects are populated only of the required \c wantedEntryProperties as set in
   * \c query (see \ref Query), which is a list of IDs of KLFLibEntry properties (the
   * KLFPropertizedObject-kind properties) that are set in the KLFLibEntry object. If the property
   * list is empty (by default), then all properties are fetched and set. Note that if this list
   * is not empty, then the properties not in the list are undefined: they may be uninitialized,
   * set to null/invalid, or filled eg. by an implementation that ignores \c wantedEntryProperties.
   *
   * \note it is possible to specify a property ID in the match condition that isn't given in
   *   \c wantedEntryProperties, and reimplementations must handle this. <i>Reason: even if
   *   this may seem inconsistent, it can be easily implemented in some examples of engines (SQL
   *   condition, ...) and can easily be worked around in other engines by adding the requested
   *   property to the wanted property list. Example: listing entries by date/time order, without
   *   necessarily displaying the date/time.</i>
   */
  virtual int query(const QString& subResource, const Query& query, QueryResult *result) = 0;


  /** \brief List all distinct values that a property takes in all entries
   *
   * \return A list of all (distinct) values that a given property in all the entries of sub-resource
   *   \c subResource takes.
   *
   * For example, KLFLibModel uses this function in category tree mode to get the category tree,
   * by passing \ref KLFLibEntry::Category as \c entryPropId.
   *
   * The order of the elements within the returned list is not defined.
   *
   * In mathematical terms, if $f_{pid}(entry)$ is the value of property ID \c pid of the entry \c entry,
   * then this function returns the \a range (=set of all reached values) of $f_{pid}$.
   *
   * KLFLibResourceSimpleEngine::queryValuesImpl() offers a very simple non-optimized implementation
   * of this function that you can use in your resource engine subclass if you don't want to do this
   * yourself. That function is automatically re-implemented if you subclass KLFLibResourceSimpleEngine.
   */
  virtual QList<QVariant> queryValues(const QString& subResource, int entryPropId) = 0;


  //! Returns all IDs in this resource (and this sub-resource)
  /** Returns a list of the ID of each entry in this resource.
   *
   * If sub-resources are supported, then returns only IDs in the given \c subResource.
   * Otherwise, the \c subResource parameter should be ignored.
   */
  virtual QList<KLFLib::entryId> allIds(const QString& subResource) = 0;

  //! Returns all IDs in this resource (and the default sub-resource)
  /** Returns a list of the ID of each entry in this resource.
   *
   * If sub-resources are supported, then returns only IDs in the default sub-resource
   * given by \ref defaultSubResource().
   *
   * The base implementation calls \ref allIds(const QString&) with the default sub-resource
   * as parameter. Subclasses need not reimplement this function, but rather
   * \ref allIds(const QString&) instead.
   */
  virtual QList<KLFLib::entryId> allIds();

  //! query all entries in this resource
  /** Returns all the entries in this library resource (in sub-resource \c subResource if
   * \ref FeatureSubResources is supported) with their corresponding IDs.
   *
   * \note Subclasses must reimplement this function to behave as described here.
   */
  virtual QList<KLFLibEntryWithId> allEntries(const QString& subResource,
					      const QList<int>& wantedEntryProperties = QList<int>()) = 0;

  //! query all entries in this resource
  /** Returns all the entries in this library resource (in default sub-resource if
   * \ref FeatureSubResources is supported) with their corresponding IDs.
   *
   * The default implementation calls
   * \ref allEntries(const QString& subResource, const QList<int>&) with the default subresource
   * as argument. See \ref setDefaultSubResource().
   */
  virtual QList<KLFLibEntryWithId> allEntries(const QList<int>& wantedEntryProperties = QList<int>());


  //! Specifies that the next operation (only) should not report progress
  void blockProgressReportingForNextOperation();

  //! (Un)Blocks generally progress reporting
  void blockProgressReporting(bool block);
  
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
  /** This signal should only be emitted by engines supporting feature \ref FeatureSubResourceProps
   * as well as \ref FeatureSubResources.
   *
   * \param subResource the affected sub-resource name
   * \param propId the ID of the property that changed
   */
  void subResourcePropertyChanged(const QString& subResource, int propId);

  //! Emitted when a sub-resource is created
  /** This signal should be emitted by engines supporting feature \ref FeatureSubResources, at
   * the end of a (successful) \ref createSubResource() call.\
   */
  void subResourceCreated(const QString& newSubResource);

  //! Emitted when a sub-resource is renamed
  /** This signal should be emitted by engines supporting feature \ref FeatureSubResources, at
   * the end of a (successful) \ref renameSubResource() call.\
   */
  void subResourceRenamed(const QString& oldSubResourceName,
			  const QString& newSubResourceName);

  //! Emitted when a sub-resource is deleted
  /** This signal should be emitted by engines supporting feature \ref FeatureSubResources, at
   * the end of a (successful) \ref deleteSubResource() call.
   */
  void subResourceDeleted(const QString& subResource);


  /** Emitted at the beginning of a long operation during which progress will be reported
   * by emission of KLFProgressReporter::progress() of the given object \c progressReporter.
   *
   * \param progressReporter is the object that will emit its progress() at regular intervals
   *   to inform caller of operation progress.
   * \param descriptiveText is some text describing the nature of the operation in progress
   *
   * Subclasses have to use KLFProgressReporter directly. It's very simple to use, example:
   * \code
   * int do_some_long_computations(...) {
   *   int num_of_steps = ... ;
   *   KLFProgressReporter progressReporter(0, num_of_steps, this);
   *   // ...
   *   int current_step = 0;
   *   for ( ... many iterations ... ) {
   *     progressReporter.doReportProgress(current_step++);
   *
   *     .... // perform a long computation iteration
   *
   *   }
   *   // finally emit the last progress value
   *   progressReporter.doReportProgress(num_of_steps);
   *   ...
   *   return some_result;
   * }
   * \endcode
   *
   * The signal operationStartReportingProgress() is suitable to use in conjunction with
   * a KLFProgressDialog.
   */
  void operationStartReportingProgress(KLFProgressReporter *progressReporter,
				       const QString& descriptiveText);



public slots:

  //! set a new resource title for this library resource
  /** Stores the new resource title. The title is a human string to display eg. in the
   * tabs of the KLFLibBrowser library browser. [KLFLibBrowser:  see klfapp library]
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
   * Subclasses should not forget to emit \ref subResourceRenamed() after a successful sub-resource
   * rename.
   *
   * \note Subclasses should also adjust the current defaultSubResource() if that is the one that
   *   was precisely renamed. Use a call to \ref setDefaultSubResource().
   */
  virtual bool renameSubResource(const QString& oldSubResourceName, const QString& newSubResourceName);

  /** Delete the given sub-resource
   *
   * Returns TRUE for success and FALSE for failure.
   *
   * Subclasses may reimplement this function to provide functionality. The default implementation
   * does nothing and returns FALSE.
   *
   * Subclasses should not forget to emit \ref subResourceDeleted() after the sub-resource
   * has been deleted (ie. upon a successful execution).
   *
   * \note Subclasses should also change the current defaultSubResource() if that is the one that
   *   was precisely deleted. Use a call to \ref setDefaultSubResource().
   */
  virtual bool deleteSubResource(const QString& subResource);


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
   *
   * \bug ........ THIS FUNCTION IS NOT COMPLETELY SUPPORTED IN KLFLibBrowser. ....... It also
   *   needs a "concept clarification", ie. what this is supposed to do exactly and how it is
   *   supposed to be used.
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
  /** External classes: Don't call this function directly, use \ref canModifyData(), \ref canModifyProp(),
   * and \ref canModifySubResourceProperty() instead.
   *
   * Internal classes: this is useful for reimplementations of canModifyData(), canModifyProp(), etc.
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

  bool thisOperationProgressBlocked() const;

private:
  void initRegisteredProperties();

  QUrl pUrl;
  uint pFeatureFlags;
  bool pReadOnly;

  QString pDefaultSubResource;

  mutable bool pProgressBlocked;
  bool pThisOperationProgressBlockedOnly;

  KLF_DEBUG_DECLARE_REF_INSTANCE( QFileInfo(url().path()).fileName()+":"+defaultSubResource()  ) ;
};


Q_DECLARE_METATYPE(KLFLibResourceEngine::KLFLibEntryWithId)
  ;


KLF_EXPORT QDataStream& operator<<(QDataStream& stream,
				   const KLFLibResourceEngine::KLFLibEntryWithId& entrywid);
KLF_EXPORT QDataStream& operator>>(QDataStream& stream,
				   KLFLibResourceEngine::KLFLibEntryWithId& entrywid);


/** \brief Provides a simple API for reading library resources.
 *
 * This class provides default implementations of some pure virtual methods of
 * \ref KLFLibResourceEngine, which call other member functions. The goal is to make life
 * simpler to create a resource engine, where access speed is not a major concern.
 *
 * For example, KLFLibResourceEngine::allIds() is pure virtual. Normally it can be implemented
 * to be faster than allEntries(), depending on the engine backend. However the functionality
 * can as well easily be achieved by calling KLFLibResourceEngine::allEntries() and returning
 * just a list with all the IDs, at the price of losing optimization.
 *
 * This class provides non-optimized default implementations for allIds() (as given above),
 * hasEntry(), entries(), and query(), based on the data returned by allEntries() and
 * entry()
 *
 * Bear in mind that optimizing one or more of those functions is still possible, by
 * reimplementing them (!)
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

  // these functions are implemented using the other base functions.

  virtual QList<KLFLib::entryId> allIds(const QString& subResource);
  virtual bool hasEntry(const QString&, entryId id);
  virtual QList<KLFLibEntryWithId> entries(const QString&, const QList<KLFLib::entryId>& idList,
					   const QList<int>& wantedEntryProperties = QList<int>());

  /** \brief Helper class to sort entries into a \ref KLFLibResourceEngine::QueryResult.
   */
  class QueryResultListSorter
  {
    KLFLibEntrySorter *mSorter;
    QueryResult *mResult;
    uint fillflags;
    bool reference_is_rawentrylist;
  public:
    /** Build an QueryResultListSorter object, that should sort entries according to
     * \c sorter. See also \ref KLFLibEntrySorter.
     *
     * \c sorter must not be NULL.
     */
    QueryResultListSorter(KLFLibEntrySorter *sorter, QueryResult *result);

    /*    QueryResultListSorter(const QueryResultListSorter& other); */

    /** \brief Compares \ref KLFLibEntry'ies */
    inline bool operator()(const KLFLibEntry& a, const KLFLibEntry& b)
    { return mSorter->operator()(a, b); }

    /** \brief Compares \ref KLFLibResourceEngine::KLFLibEntryWithId's */
    inline bool operator()(const KLFLibEntryWithId& a, const KLFLibEntryWithId& b)
    { return mSorter->operator()(a.entry, b.entry); }

    /** Returns the number of entries there are in the lists (ignoring those lists left empty, of
     * course). */
    int numberOfEntries();

    /** Inserts the entry-with-id \c entrywid, into the appropriate lists in the \c result that
     * was given to the constructor, such that the lists are ordered according to the sorter set
     * in the constructor.
     *
     * By \a appropriate we mean the lists for which the fill flags are set in the QueryResult object.
     *
     * If the set sorter's sorting property ID is \c -1, then the elements are simply
     * appended to the appropriate lists; sorting is disabled in this case.
     *
     * When sorting is enabled, this function assumes that the lists in \c result are sorted
     * appropriately. This is naturally the case if you only use this function to build the lists.
     * In other terms, don't call this function if you already added non-sorted items into the
     * list(s).
     *
     * \note if the fill flags include neither the raw entry list, nor the entry-with-id list,
     *   then the raw entry list is also filled, as it is not possible to just compare bare entry
     *   IDs (!)
     */
    void insertIntoOrderedResult(const KLFLibEntryWithId& entry);
  };

  virtual int query(const QString& subResource, const Query& query, QueryResult *result);

  virtual QList<QVariant> queryValues(const QString& subResource, int entryPropId);


  /** A basic implementation of query() based on matching the results of
   * <tt>resource->allEntries()</tt>. */
  static int queryImpl(KLFLibResourceEngine *resource, const QString& subResource,
		       const Query& query, QueryResult *result);

  /** A basic implementation of queryValues() based on looking at the results of
   * <tt>resource->allEntries()</tt> */
  static QList<QVariant> queryValuesImpl(KLFLibResourceEngine *resource, const QString& subResource,
					 int entryPropId);


  /** A simple entry condition tester. */
  static bool testEntryMatchConditionImpl(const KLFLib::EntryMatchCondition& condition,
					  const KLFLibEntry& libentry);
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
   * \c "LocalFile" documents its parameters in
   * \ref KLFLibBasicWidgetFactory::retrieveCreateParametersFromWidget(), eg. parameter
   * \c "Filename" contains a QString with the entered local file name.
   *
   * Additional parameters may also be set by KLFLibCreateResourceDlg itself (eg.
   * default sub-resource name/title, .........TODO............)
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



// -------------------------


class QMimeData;

//! Helper class to encode an entry list as mime data (abstract interface)
class KLF_EXPORT KLFAbstractLibEntryMimeEncoder
{
public:
  KLFAbstractLibEntryMimeEncoder();
  virtual ~KLFAbstractLibEntryMimeEncoder();

  //! A list of mime types this class can encode
  virtual QStringList supportedEncodingMimeTypes() const = 0;
  //! A list of mime types this class can decode
  virtual QStringList supportedDecodingMimeTypes() const = 0;

  virtual QByteArray encodeMime(const KLFLibEntryList& entryList, const QVariantMap& metaData,
				const QString& mimeType) const = 0;

  virtual bool decodeMime(const QByteArray& data, const QString& mimeType,
			  KLFLibEntryList *entryList, QVariantMap *metaData) const = 0;


  static QStringList allEncodingMimeTypes();
  static QStringList allDecodingMimeTypes();
  //! Creates a QMetaData with all known registered encoding mime types
  static QMimeData *createMimeData(const KLFLibEntryList& entryList, const QVariantMap& metaData);
  static bool canDecodeMimeData(const QMimeData *mimeData);
  static bool decodeMimeData(const QMimeData *mimeData, KLFLibEntryList *entryList,
			     QVariantMap *metaData);

  static KLFAbstractLibEntryMimeEncoder *findEncoderFor(const QString& mimeType,
							bool warnIfNotFound = true);
  static KLFAbstractLibEntryMimeEncoder *findDecoderFor(const QString& mimeType,
							bool warnIfNotFound = true);
  static QList<KLFAbstractLibEntryMimeEncoder*> encoderList();
private:

  static void registerEncoder(KLFAbstractLibEntryMimeEncoder *encoder);

  static QList<KLFAbstractLibEntryMimeEncoder*> staticEncoderList;
};





#ifdef KLF_DEBUG
#include <QDebug>
KLF_EXPORT QDebug& operator<<(QDebug& dbg, const KLFLib::StringMatch& smatch);
KLF_EXPORT QDebug& operator<<(QDebug& dbg, const KLFLib::PropertyMatch& pmatch);
KLF_EXPORT QDebug& operator<<(QDebug& dbg, const KLFLib::EntryMatchCondition& c);
KLF_EXPORT QDebug& operator<<(QDebug& dbg, const KLFLibResourceEngine::Query& q);
KLF_EXPORT QDebug& operator<<(QDebug& dbg, const KLFLibResourceEngine::KLFLibEntryWithId& ewid);
#endif




#endif

