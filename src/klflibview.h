/***************************************************************************
 *   file klflibview.h
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

#ifndef KLFLIBVIEW_H
#define KLFLIBVIEW_H

#include <QAbstractItemModel>
#include <QAbstractItemDelegate>
#include <QMimeData>
#include <QEvent>
#include <QWidget>
#include <QDialog>
#include <QAbstractButton>
#include <QTreeView>
#include <QItemSelectionModel>
#include <QTextDocument>
#include <QTextCharFormat>
#include <QStandardItemModel>
#include <QListView>

#include <klfdefs.h>
#include <klflib.h>
#include <klfsearchbar.h>
#include <klfguiutil.h>


namespace KLFLib {
  enum RestoreMode {
    RestoreLatex = 0x0001,
    RestoreStyle = 0x0002,

    RestoreLatexAndStyle = RestoreLatex|RestoreStyle,
    RestoreAll = 0xFFFF
  };
};


//! A view widget to display a library resource's contents
/** A base API for a widget that will display a KLFLibResourceEngine's contents.
 * For example one could create a QTreeView and display the contents in there
 * (that's what KLFLibDefaultView does...).
 *
 * \note This class subclasses QWidget because it makes uses of signals and slots.
 *   Before editing this file to change that, think before removing that inheritance
 *   or muting it to a QObject (subclasses will eventually want to inherit QWidget),
 *   no multiple QObject inheritance please!
 *
 * \note Subclasses should emit the \ref resourceDataChanged() signal AFTER they
 *   refresh after a resource data change.
 *
 * Note: The operationStartReportingProgress() signal is purposed for when long update
 * operations are necessary. It is to be used by subclasses exactly like in
 * KLFLibResourceEngine.
 *
 * \warning The design of this class with a \ref setResourceEngine() function makes it
 *   unsafe to blindly assume a non-NULL resource engine pointer at all times. Check
 *   the pointer before using it! See \ref validResourceEngine().
 *
 * If the reimplementation of this view can be searched with a KLFSearchBar, reimplement
 * searchable() to return a KLFSearchable object for this view.
 */
class KLF_EXPORT KLFAbstractLibView : public QWidget
{
  Q_OBJECT
public:
  KLFAbstractLibView(QWidget *parent);
  virtual ~KLFAbstractLibView() { }

  virtual KLFLibResourceEngine * resourceEngine() const { return pResourceEngine; }

  //! Display Resource URL. <i>NOT exactly like KLFLibResourceEngine::url() !</i>
  /** Returns an URL describing exactly what is shown to user, based on the resource
   * engine's URL.
   *
   * \warning This does not simply return <tt>resourceEngine()->url()</tt>. It returns
   *   in the URL exactly the information that is displayed to user. For example, a
   *   view that can handle displaying all sub-resources will NOT include the default
   *   sub-resource in its URL, however a view displaying only the default sub-resource
   *   will include a "klfDefaultSubResource" query item to distinguish it from another
   *   view acting on the same base URL but displaying a different sub-resource.
   */
  virtual QUrl url() const = 0;

  //! Compare this resource view's URL to another URL
  /** Compares the URL of the resource we are viewing with the URL \c other, and
   * returns an OR-ed combination of enum \ref KlfUrlCompareFlag values of URL-comparision
   * tests that have turned out to be true (see \ref KlfUrlCompareFlag for a list
   * of tests).
   *
   * This function is supposed to answer to the following questions, each a condition
   * as whether to return a URL comparation flag or not:
   * - flag \c KlfUrlCompareEqual: does this view show the same information as a view
   *   whose URL would be \c other ?
   * - flag \c KlfUrlCompareLessSpecific: does this view show _MORE_ information than
   *   the \c other view ("less specific information" <=> "shows more") ?
   * - flag \c KLFUrlCompareMoreSpecific: does this view show _LESS_ information than
   *   the \c other view ?
   * - flag \c KLFUrlComapreBaseEqual: does this view show the same resource as the
   *   \c other view, but possibly does not the same specific information, eg. not the
   *   same sub-resource.
   *
   * The \c interestFlags is a binary OR'ed value of KlfUrlCompareFlag values of tests
   * to be performed. Any flag that is set in \c interestFlags indicates that the return
   * value of this function, when binary-AND'ed with that flag, is the result (T or F)
   * of the test the flag stands for. However, if a flag is not set in \c interestFlags,
   * its state in the return value by this function is undefined.
   *
   * See also klfUrlCompare().
   */
  virtual uint compareUrlTo(const QUrl& other, uint interestFlags = 0xFFFFFFFF) const = 0;


  //! Returns TRUE if a non-\c NULL resource engine has been set
  inline bool validResourceEngine() const { return pResourceEngine != NULL; }

  virtual void setResourceEngine(KLFLibResourceEngine *resource);

  /** Subclasses should return a list of entries that have been selected by the user. */
  virtual KLFLibEntryList selectedEntries() const = 0;

  /** Subclasses should return a list of resource-entry-IDs that have been selected by
   * the user. */
  virtual QList<KLFLib::entryId> selectedEntryIds() const = 0;

  /** Subclasses may add items to the context menu by returning them in this function.
   * \param pos is the position relative to widget where the menu was requested.
   *
   * \note The view itself does not handle context menus. This function is provided
   *   for whichever class uses this view to add these actions when that class creates
   *   a context menu.
   *
   * The default implementation returns an empty list. */
  virtual QList<QAction*> addContextMenuActions(const QPoint& pos);

  /** Saves the current GUI state (eg. column widths and order, etc.) */
  virtual QVariantMap saveGuiState() const = 0;

  /** Restores the state described in \c state (which was previously, possibly in another
   * session, returned by \ref saveGuiState()) */
  virtual bool restoreGuiState(const QVariantMap& state) = 0;

  /** Subclasses should reimplement this function to return a list of all known categories
   * to suggest to the user, eg. in a completion list for an editable combo box to edit
   * categories. */
  virtual QStringList getCategorySuggestions() = 0;

  virtual KLFSearchable * searchable() { return NULL; }

signals:
  /** Is emitted (by subclasses) when a latex entry is selected to be restored (eg. the entry was
   * double-clicked).
   *
   * \param entry is the data to restore
   * \param restoreflags provides information on which part of \c entry to restore. See the
   *   possible flags in \ref KLFLib::RestoreMode.
   */
  void requestRestore(const KLFLibEntry& entry, uint restoreflags = KLFLib::RestoreLatexAndStyle);
  /** Is emitted (by subclasses) when the view wants the main application (or the framework that uses
   * this class) to restore the given style \c style. No latex is to be restored.
   */
  void requestRestoreStyle(const KLFStyle& style);

  /** Subclasses must emit this signal AFTER they have refreshed. */
  void resourceDataChanged(const QList<KLFLib::entryId>& entryIdList);

  /** Emitted when the selection has changed, eg. user selected or deselected some entries. */
  void entriesSelected(const KLFLibEntryList& entries);

  /** Subclasses should emit this signal with lists of categories they come accross,
   * so that the editor can suggest these as completions upon editing category */
  void moreCategorySuggestions(const QStringList& categorylist);

  /** Emitted by subclasses to announce the beginning of a long operation during which progress
   * will be reported through the given \c progressReporter.
   *
   * Should be used in the same way as KLFLibResourceEngine::operationStartReportingProgress().
   */
  void operationStartReportingProgress(KLFProgressReporter *progressReporter, const QString& descriptiveText);

public slots:
  virtual void updateResourceEngine() = 0;
  virtual void updateResourceProp(int propId) = 0;
  virtual void updateResourceData(const QString& subres, int modifyType,
				  const QList<KLFLib::entryId>& entryIdList) = 0;
  /** Default implementation calls updateResourceEngine() */
  virtual void updateResourceDefaultSubResourceChanged(const QString& newSubResource);

  /** Subclasses should reimplement to update the given property to the given value on all
   * library entries that are selected by the user. (They have to actually perform the change
   * in the resource, eg with KLFLibResourceEngine::changeEntries()).
   *
   * \note The view does not have to garantee that selection is preserved.
   */
  //  virtual bool writeEntryProperty(int property, const QVariant& value) = 0;

  /* * Provides a reasonable default implementation that should suit for most purposes. */
  //  virtual bool writeEntryCategory(const QString& category)
  //  { return writeEntryProperty(KLFLibEntry::Category, category); }

  /* * Provides a reasonable default implementation that should suit for most purposes. */
  //  virtual bool writeEntryTags(const QString& tags)
  //  { return writeEntryProperty(KLFLibEntry::Tags, tags); }

  //  virtual bool deleteSelected(bool requireConfirm = true);

  //   virtual bool insertEntries(const KLFLibEntryList& entries);

  /* * Provides a reasonable default implementation that should suit for most purposes. */
  //   virtual bool insertEntry(const KLFLibEntry& entry)
  //   { return insertEntries(KLFLibEntryList() << entry); }

  /** Subclasses must reimplement to select the given entries in the view.
   *
   * \returns TRUE on success. If the operation was not possible (eg. one ID is invalid/not
   * displayed, view does not support multiple selections whilst idList.size()>1, etc.) then
   * this function should return FALSE.
   */
  virtual bool selectEntries(const QList<KLFLib::entryId>& idList) = 0;

  /** Collects the necessary information and emits \ref requestRestore() */
  virtual void restore(uint restoreFlags = KLFLib::RestoreLatexAndStyle) = 0;
  /** Provides a reasonable default implementation that should suit for most purposes. */
  virtual void restoreWithStyle() { restore(KLFLib::RestoreLatexAndStyle); }
  /** Provides a reasonable default implementation that should suit for most purposes. */
  virtual void restoreLatexOnly() { restore(KLFLib::RestoreLatex); }

  /** Called by the owner of the view. This function fetches category suggestions
   * (by calling the virtual getCategorySuggestions() reimplemented by subclasses)
   * and emits the signal moreCategorySuggestions().
   *
   * Subclasses need not reimplement this function. */
  virtual void wantMoreCategorySuggestions();

private:
  KLFLibResourceEngine *pResourceEngine;
};


// -----------------

class KLF_EXPORT KLFLibViewFactory : public QObject
{
  Q_OBJECT
public:
  KLFLibViewFactory(const QStringList& viewTypeIdentifiers, QObject *parent = NULL);
  virtual ~KLFLibViewFactory();

  /** A list of view type identifiers that this factory can create.
   *
   * Individual view widget types are identified by their "view type identifiers". They
   * are not meant to be human-readable (eg. "LibModel+CategoryTree" or whatever)  */
  virtual QStringList viewTypeIdentifiers() { return pViewTypeIdentifiers; }

  /** A translated string to be shown to user (in a choice box for ex.) for
   * the given view widget type. (eg. <tt>tr("Tree View")</tt>) */
  virtual QString viewTypeTitle(const QString& viewTypeIdent) const = 0;

  /** \returns Whether this factory can create the given view widget for the given engine.
   *
   * This function may return false, for example if this widget factory creates a specialized
   * kind of widget that can only work with a given engine. */
  virtual bool canCreateLibView(const QString& viewTypeIdent, KLFLibResourceEngine *engine) = 0;

  /** Create a library view with the given widget \c parent. The view should reflect the contents
   * given by the resource engine \c resourceEngine . */
  virtual KLFAbstractLibView * createLibView(const QString& viewTypeIdent, QWidget *parent,
					     KLFLibResourceEngine *resourceEngine) = 0;

  /** Returns the default view type identifier. Create this view if you don't have any idea
   * which view you prefer.
   *
   * This actually returns the first view type identifier of the first registered factory. */
  static QString defaultViewTypeIdentifier();

  /** Returns the factory that can handle the URL scheme \c urlScheme, or NULL if no such
   * factory exists (ie. has been registered). */
  static KLFLibViewFactory *findFactoryFor(const QString& viewTypeIdentifier);

  /** Returns a combined list of all view type identifiers that the installed factories support.
   * ie. returns a list of all view type idents. we're capable of creating. */
  static QStringList allSupportedViewTypeIdentifiers();
  /** Returns the full list of installed factories. */
  static QList<KLFLibViewFactory*> allFactories() { return pRegisteredFactories; }

private:
  QStringList pViewTypeIdentifiers;

  static void registerFactory(KLFLibViewFactory *factory);
  static void unRegisterFactory(KLFLibViewFactory *factory);

  static QList<KLFLibViewFactory*> pRegisteredFactories;

};


// --------------------



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
 *
 * The widget type is given by the
 * \ref KLFLibEngineFactory::correspondingWidgetType "Engine Factory".
 *
 * See also \ref KLFLibBasicWidgetFactory and \ref KLFLibEngineFactory.
 */
class KLF_EXPORT KLFLibWidgetFactory : public QObject, public KLFFactoryBase
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
  /** eg. \c "LocalFile", ... */
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
   * The widget should have a <tt>void readyToOpen(bool isReady)</tt> signal, that is emitted
   * to synchronize the enabled state of the "open" button. The widget should also have a
   * \c "readyToOpen" Qt property (static or dynamic, doesn't matter), that can be retrieved
   * with QObject::property(), that contains the current status of if the widget is ready to
   * open.
   *
   * The create widget should be a child of \c wparent.
   */
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
   * - <tt>param["klfScheme"]=<i>url-scheme</i></tt> is MANDATORY: it tells the caller which kind
   *   of scheme the resource should be.
   * - <tt>param["klfRetry"]=true</tt> will cause the dialog not to exit but re-prompt user to
   *   possibly change his input (could result from user clicking "No" in a "Overwrite?"
   *   dialog presented in a possible reimplementation of this function).
   * - <tt>param["klfCancel"]=true</tt> will cause the "create resource" process to be cancelled.
   * - Not directly recognized by the system, but a common standard: <tt>param["klfDefaultSubResource"]</tt>
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





// -----------------

class KLFLibModelCache;

/** \brief Model for Item-Views displaying a library resource's contents
 *
 * The Model can morph into different forms, for simulating various common & useful
 * displays (chronological list (history), category/tags tree (archive), maybe icons
 * in the future, ...).
 *
 */
class KLF_EXPORT KLFLibModel : public QAbstractItemModel
{
  Q_OBJECT
public:
  enum FlavorFlag {
    LinearList = 0x0001,
    IconViewList = LinearList,
    CategoryTree = 0x0002,
    DisplayTypeMask = 0x000f,

    GroupSubCategories = 0x1000
  };

  KLFLibModel(KLFLibResourceEngine *resource, uint flavorFlags = LinearList|GroupSubCategories,
	      QObject *parent = NULL);
  virtual ~KLFLibModel();

  enum ItemKind { EntryKind, CategoryLabelKind };
  enum {
    ItemKindItemRole = Qt::UserRole+768, // = 800 in Qt 4.4, nice in debugging messages ;-)
    EntryContentsTypeItemRole,
    EntryIdItemRole,
    FullEntryItemRole,
    CategoryLabelItemRole,
    FullCategoryPathItemRole
  };

  /** For example use
   * \code
   *  model->data(index, KLFLibModel::entryItemRole(KLFLibEntry::Latex)).toString()
   * \endcode
   * to get LaTeX string for model index \c index.
   */
  static inline int entryItemRole(int propertyId) { return (Qt::UserRole+788) + propertyId; } // = 820+propId
  /** inverse operation of \ref entryItemRole */
  static inline int entryPropIdForItemRole(int role)  { return role - (Qt::UserRole+788); } // = role - 820
  
  virtual void setResource(KLFLibResourceEngine *resource);

  virtual KLFLibResourceEngine * resource() { return pResource; }

  virtual QUrl url() const;

  /** sets the flavor flags given by \c flags. Only flags masked by \c modify_mask
   * are affected. Examples:
   * \code
   *  // Display type set to LinearList. GroupSubCategories is unchanged.
   *  m->setFlavorFlags(KLFLibModel::LinearList, KLFLibModel::DisplayTypeMask);
   *  // Set, and respectively unset the group sub-categories flag (no change to other flags)
   *  m->setFlavorFlags(KLFLibModel::GroupSubCategories, KLFLibModel::GroupSubCategories);
   *  m->setFlavorFlags(0, KLFLibModel::GroupSubCategories);
   * \endcode  */
  virtual void setFlavorFlags(uint flags, uint modify_mask = 0xffffffff);
  virtual uint flavorFlags() const;
  inline uint displayType() const { return flavorFlags() & DisplayTypeMask; }

  /** ensures that the cache nodes of the given index list are not 'minimalist' */
  virtual void prefetch(const QModelIndexList& index) const;
  virtual QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const;
  virtual Qt::ItemFlags flags(const QModelIndex& index) const;
  virtual bool hasChildren(const QModelIndex &parent = QModelIndex()) const;
  virtual QVariant headerData(int section, Qt::Orientation orientation,
			      int role = Qt::DisplayRole) const;
  virtual bool hasIndex(int row, int column,
			const QModelIndex &parent = QModelIndex()) const;
  virtual QModelIndex index(int row, int column,
			    const QModelIndex &parent = QModelIndex()) const;
  virtual QModelIndex parent(const QModelIndex &index) const;
  virtual int rowCount(const QModelIndex &parent = QModelIndex()) const;
  virtual int columnCount(const QModelIndex &parent = QModelIndex()) const;

  virtual bool canFetchMore(const QModelIndex& parent) const;
  virtual void fetchMore(const QModelIndex& parent);

  virtual Qt::DropActions supportedDropActions() const;

  virtual QStringList mimeTypes() const;
  virtual QMimeData *mimeData(const QModelIndexList& indexes) const;

  virtual bool dropMimeData(const QMimeData *data, Qt::DropAction action, int row,
			    int column, const QModelIndex& parent);

  enum { DropWillAccept = 0x0001,
	 DropWillCategorize = 0x0002,
	 DropWillMove = 0x0004 };
  virtual uint dropFlags(QDragMoveEvent *event, QAbstractItemView *view);

  virtual QImage dragImage(const QModelIndexList& indexes);

  virtual int entryColumnContentsPropertyId(int column) const;
  virtual int columnForEntryPropertyId(int entryPropertyId) const;

  virtual bool isDesendantOf(const QModelIndex& child, const QModelIndex& ancestor);

  virtual QStringList categoryList() const;

  virtual void updateData(const QList<KLFLib::entryId>& entryIdList, int modifyType);

  //! Call repeatedly to walk all indexes (once each exactly, first column only)
  virtual QModelIndex walkNextIndex(const QModelIndex& cur);
  //! Call repeatedly to walk all indexes in model in reverse order
  virtual QModelIndex walkPrevIndex(const QModelIndex& cur);

  virtual KLFLib::entryId entryIdForIndex(const QModelIndex& index) const;
  virtual QModelIndex findEntryId(KLFLib::entryId eid) const;
  virtual QList<KLFLib::entryId> entryIdForIndexList(const QModelIndexList& indexlist) const;
  virtual QModelIndexList findEntryIdList(const QList<KLFLib::entryId>& eidlist) const;

  virtual int fetchBatchCount() const { return pFetchBatchCount; }


  //! notify the model that the entrySorter() settings were changed, and we need to re-sort.
  virtual void redoSort();

  //! change the entrySorter accordingly and re-sort the model.
  virtual void sort(int column, Qt::SortOrder order = Qt::AscendingOrder);

  //! The current KLFLibEntrySorter that sorts our items
  virtual KLFLibEntrySorter * entrySorter() { return pEntrySorter; }

  /** \warning The model will take ownership of the sorter and deleted in the destructor. */
  virtual void setEntrySorter(KLFLibEntrySorter *entrySorter);

signals:
  /** Announces the beginning of a long operation  (used for updates in updateData()) */
  void operationStartReportingProgress(KLFProgressReporter *progressReporter, const QString& descriptiveText);

public slots:

  virtual QModelIndex searchFind(const QString& queryString, const QModelIndex& fromIndex
				 = QModelIndex(), bool forward = true);
  virtual QModelIndex searchFindNext(bool forward);
  virtual void searchAbort();

  //   virtual bool changeEntries(const QModelIndexList& items, int property, const QVariant& value);
  //   virtual bool insertEntries(const KLFLibEntryList& entries);
  //   virtual bool deleteEntries(const QModelIndexList& items);

  virtual void completeRefresh();

  /** how many items to fetch at a time when fetching preview and style (non-minimalist) */
  virtual void setFetchBatchCount(int count) { pFetchBatchCount = count; }

private:

  friend class KLFLibModelCache;

  KLFLibResourceEngine *pResource;

  unsigned int pFlavorFlags;

  int pFetchBatchCount;

  KLFLibModelCache *pCache;

  KLFLibEntrySorter *pEntrySorter;

  struct PersistentId {
    int kind;
    KLFLib::entryId entry_id;
    QString categorylabel_fullpath;
    int column;
  };
  friend QDebug& operator<<(QDebug&, const PersistentId&);

  QList<PersistentId> persistentIdList(const QModelIndexList& persistentindexlist);
  QModelIndexList newPersistentIndexList(const QList<PersistentId>& persistentidlist);

  void startLayoutChange(bool withQtLayoutChangedSignal = true);
  void endLayoutChange(bool withQtLayoutChangedSignal = true);

  QModelIndexList pLytChgIndexes;
  QList<PersistentId> pLytChgIds;

  void updateCacheSetupModel();

  QString pSearchString;
  QModelIndex pSearchCurNode;
  bool pSearchAborted;

  bool dropCanInternal(const QMimeData *data);

  KLF_DEBUG_DECLARE_ASSIGNABLE_REF_INSTANCE() ;
};



// -----------------


class KLF_EXPORT KLFLibViewDelegate : public QAbstractItemDelegate
{
  Q_OBJECT

  Q_PROPERTY(QSize previewSize READ previewSize WRITE setPreviewSize) ;
public:
  /** Create a view delegate for displaying a KLFLibModel.
   * \param parent the (QObject-)parent of this object.
   */
  KLFLibViewDelegate(QObject *parent);
  virtual ~KLFLibViewDelegate();

  inline QSize previewSize() const { return pPreviewSize; }

  virtual QWidget *createEditor(QWidget *parent, const QStyleOptionViewItem& option,
				const QModelIndex& index) const;
  virtual bool editorEvent(QEvent *event,QAbstractItemModel *model, const QStyleOptionViewItem& option,
			   const QModelIndex& index);
  virtual void paint(QPainter *painter, const QStyleOptionViewItem& option, const QModelIndex& index) const;
  virtual void setEditorData(QWidget *editor, const QModelIndex& index) const;
  virtual void setModelData(QWidget *editor, QAbstractItemModel *model, const QModelIndex& index) const;
  virtual QSize sizeHint(const QStyleOptionViewItem& option, const QModelIndex& index) const;
  virtual void updateEditorGeometry(QWidget *editor, const QStyleOptionViewItem& option,
				    const QModelIndex& index) const;

  virtual void setSearchString(const QString& s) { pSearchString = s; }
  virtual void setSearchIndex(const QModelIndex& index) { pSearchIndex = index; }
  virtual void setSelectionModel(QItemSelectionModel *sm) { pSelModel = sm; }
  /** If the delegate paints items in a QTreeView, then pass a pointer to it here to display nice
   * selection markings under non-expanded tree items. Pass \c NULL to unset any previously set
   * QTreeView pointer.
   *
   * By default, the internal tree view pointer is set to NULL. */
  virtual void setTheTreeView(QTreeView *theTreeView) { pTheTreeView = theTreeView; }
  //  virtual void setIndexExpanded(const QModelIndex& index, bool isexpanded) {
  //    pExpandedIndexes[QPersistentModelIndex(index)] = isexpanded;
  //  }

  virtual bool autoBackgroundItems() const { return pAutoBackgroundItems; }
  virtual void setAutoBackgroundItems(bool autoBgItems) { pAutoBackgroundItems = autoBgItems; }
  virtual QColor autoBackgroundColor() const { return pAutoBackgroundColor; }
  virtual void setAutoBackgroundColor(const QColor& autoBgColor) { pAutoBackgroundColor = autoBgColor; }

public slots:
  void setPreviewSize(const QSize& psize) { pPreviewSize = psize; }

protected:
  struct PaintPrivate {
    QPainter *p;
    QBrush background;
    const QStyleOptionViewItem *option;
    bool isselected;
    QRect innerRectText;
    QRect innerRectImage;
  };

  virtual void paintEntry(PaintPrivate *p, const QModelIndex& index) const;
  virtual void paintCategoryLabel(PaintPrivate *p, const QModelIndex& index) const;

  enum { PTF_HighlightSearch        = 0x0001,
	 PTF_HighlightSearchCurrent = 0x0002,
	 PTF_SelUnderline           = 0x0004,
	 PTF_ForceRichTextRender    = 0x0008,
	 PTF_FontLarge              = 0x0010,
	 PTF_FontTT                 = 0x0020
  };
  virtual void paintText(PaintPrivate *p, const QString& text, uint flags = PTF_HighlightSearch) const;

  virtual bool indexHasSelectedDescendant(const QModelIndex& parent) const;
  virtual bool selectionIntersectsIndexChildren(const QItemSelection& selection,
						const QModelIndex& parent) const;
  /** implements the core of \ref indexHasSelectedDescendant. use that instead. */
  virtual bool func_indexHasSelectedDescendant(const QModelIndex& parent, const QTime& timer,
					       int timeLimitMs) const;

private:
  QString pSearchString;
  QModelIndex pSearchIndex;
  QItemSelectionModel *pSelModel;
  QTreeView *pTheTreeView; //!< warning: this is possibly NULL! see \ref setTheTreeView()

  QSize pPreviewSize;

  bool pAutoBackgroundItems;
  QColor pAutoBackgroundColor;

  //  QMap<QPersistentModelIndex, bool> pExpandedIndexes;

  struct ColorRegion {
    ColorRegion(QTextCharFormat f = QTextCharFormat(), int s = -1, int l = 0)
      : fmt(f), start(s), len(l) { }
    QTextCharFormat fmt; int start; int len;
    bool operator<(const ColorRegion& other) const {
      return start < other.start;
    }
  };
  friend QDebug& operator<<(QDebug&, const ColorRegion&);
};

// -----------------

/** An implementation of the KLFAbstractLibView viwer to view library resource contents in
 * so-called Category, List or Icon view modes.
 */
class KLF_EXPORT KLFLibDefaultView : public KLFAbstractLibView, public KLFSearchable
{
  Q_OBJECT
  Q_PROPERTY(bool autoBackgroundItems READ autoBackgroundItems WRITE setAutoBackgroundItems) ;
  Q_PROPERTY(QColor autoBackgroundColor READ autoBackgroundColor WRITE setAutoBackgroundColor) ;

  Q_PROPERTY(QListView::Flow iconViewFlow READ iconViewFlow WRITE setIconViewFlow) ;

  Q_PROPERTY(QSize previewSize READ previewSize WRITE setPreviewSize) ;

public:
  enum ViewType { CategoryTreeView, ListTreeView, IconView };
  KLFLibDefaultView(QWidget *parent, ViewType viewtype = CategoryTreeView);
  virtual ~KLFLibDefaultView();

  virtual QUrl url() const;
  virtual uint compareUrlTo(const QUrl& other, uint interestFlags = 0xFFFFFFFF) const;

  inline QSize previewSize() const { return pDelegate->previewSize(); }

  bool groupSubCategories() const { return pGroupSubCategories; }

  virtual bool event(QEvent *e);
  virtual bool eventFilter(QObject *o, QEvent *e);

  virtual KLFLibEntryList selectedEntries() const;
  virtual QList<KLFLib::entryId> selectedEntryIds() const;

  ViewType viewType() const { return pViewType; }

  virtual QList<QAction*> addContextMenuActions(const QPoint& pos);

  virtual QVariantMap saveGuiState() const;
  virtual bool restoreGuiState(const QVariantMap& state);

  //! The first index that is currently visible in the current scrolling position
  virtual QModelIndex currentVisibleIndex() const;

  bool autoBackgroundItems() const { return pDelegate->autoBackgroundItems(); }
  QColor autoBackgroundColor() const { return pDelegate->autoBackgroundColor(); }

  QListView::Flow iconViewFlow() const;

  virtual QStringList getCategorySuggestions();

  virtual KLFSearchable * searchable() { return this; }

public slots:
  //   virtual bool writeEntryProperty(int property, const QVariant& value);
  //   virtual bool deleteSelected(bool requireConfirmation = true);
  //   virtual bool insertEntries(const KLFLibEntryList& entries);
  virtual bool selectEntries(const QList<KLFLib::entryId>& idList);

  virtual bool searchFind(const QString& queryString, bool forward = true);
  virtual bool searchFindNext(bool forward);
  virtual void searchAbort();

  virtual void restore(uint restoreflags = KLFLib::RestoreLatexAndStyle);

  virtual void showColumns(int propIdColumn, bool show);
  virtual void sortBy(int propIdColumn, Qt::SortOrder sortorder);

  /** Selects all items in the view, fetching all necessary items (this can be slow). If \c expandItems
   * is TRUE, then all items in this view (category labels) are expanded (no effect if this view is
   * not a tree view). */
  virtual void slotSelectAll(bool expandItems = false);
  virtual void slotRefresh();
  virtual void slotRelayoutIcons();

  void setPreviewSize(const QSize& size) { pDelegate->setPreviewSize(size); }

  void setAutoBackgroundItems(bool on) { pDelegate->setAutoBackgroundItems(on); }
  void setAutoBackgroundColor(const QColor& c) { pDelegate->setAutoBackgroundColor(c); }

  /** \brief Sets the icon view flow, see \ref QListView::Flow.
   *
   * Has no effect if our view type is not icon view. */
  void setIconViewFlow(QListView::Flow flow);

  /** \warning This function takes effect upon the next change of resource engine, ie the next
   * call of \ref KLFAbstractLibView::setResourceEngine() */
  void setGroupSubCategories(bool yesOrNo) { pGroupSubCategories = yesOrNo; }

  void updateDisplay();

protected:
  virtual void updateResourceEngine();
  virtual void updateResourceProp(int propId);
  virtual void updateResourceData(const QString& subRes, int modifyType,
				  const QList<KLFLib::entryId>& entryIdList);
  virtual void updateResourceOwnData(const QList<KLFLib::entryId>& entryIdList);

  virtual void showEvent(QShowEvent *event);

  enum SelectAllFlags { ExpandItems = 0x01, NoSignals = 0x02 } ;

protected slots:
  void slotViewSelectionChanged(const QItemSelection& selected, const QItemSelection& deselected);

  /** Selects all children of \c parent (by default a QModelIndex(), so this function selects
   * all items).
   *
   * selectFlags is an OR'ed combination of SelectAllFlags. If ExpandItems is set then all items
   * in the view are expanded (no effect if the
   * view is not a tree view).
   */
  virtual void slotSelectAll(const QModelIndex& parent, uint selectFlags);
  
  void slotViewItemClicked(const QModelIndex& index);
  void slotEntryDoubleClicked(const QModelIndex& index);

  void slotShowColumnSenderAction(bool showCol);

  // called from model
  void slotResourceDataChanged(const QModelIndex& topLeft, const QModelIndex& botRight);

  void slotPreviewSizeFromActionSender();
  void slotPreviewSizeActionsRefreshChecked();

private:
  ViewType pViewType;
  QAbstractItemView *pView;
  KLFLibViewDelegate *pDelegate;
  KLFLibModel *pModel;

  bool pGroupSubCategories;

  QList<QAction*> pCommonActions;
  QList<QAction*> pShowColumnActions;
  QMenu *pPreviewSizeMenu;
  QList<QAction*> pIconViewActions;

  QList<QAction*> pViewActionsWithShortcut;

  bool pEventFilterNoRecurse;

  QModelIndexList selectedEntryIndexes() const;

  bool func_selectAll(const QModelIndex& parent, uint flags, QTime *tm, KLFDelayedPleaseWaitPopup *pleaseWait);

private slots:
  void searchFound(const QModelIndex& i);

protected:
  KLF_DEBUG_DECLARE_REF_INSTANCE( QFileInfo(url().path()).fileName()+":"
				  +(resourceEngine()?resourceEngine()->defaultSubResource():"[NULL]")
				  +"|viewtype="+QString::number(pViewType)  ) ;
};

// -----------------

class KLF_EXPORT KLFLibDefaultViewFactory : public KLFLibViewFactory
{
  Q_OBJECT
public:
  KLFLibDefaultViewFactory(QObject *parent = NULL);
  virtual ~KLFLibDefaultViewFactory() { }

  virtual QString viewTypeTitle(const QString& viewTypeIdent) const;

  virtual bool canCreateLibView(const QString& /*viewTypeIdent*/,
				KLFLibResourceEngine */*engine*/) { return true; }

  virtual KLFAbstractLibView * createLibView(const QString& viewTypeIdent, QWidget *parent,
					     KLFLibResourceEngine *resourceEngine);
};



// -----------------

namespace Ui {
  class KLFLibOpenResourceDlg;
  class KLFLibCreateResourceDlg;
  class KLFLibResPropEditor;
  class KLFLibNewSubResDlg;
};

/** \brief Dialog prompting user to choose a resource and a sub-resource to open. */
class KLF_EXPORT KLFLibOpenResourceDlg : public QDialog
{
  Q_OBJECT
public:
  KLFLibOpenResourceDlg(const QUrl& defaultlocation = QUrl(), QWidget *parent = 0);
  virtual ~KLFLibOpenResourceDlg();

  virtual QUrl url() const;

  static QUrl queryOpenResource(const QUrl& defaultlocation = QUrl(), QWidget *parent = 0);

protected slots:

  virtual void updateReadyToOpenFromSender(bool isready);
  virtual void updateReadyToOpen();

protected:
  virtual QUrl retrieveRawUrl() const;

private:
  Ui::KLFLibOpenResourceDlg *pUi;
  QAbstractButton *btnGo;
};

// --


class KLF_EXPORT KLFLibCreateResourceDlg : public QDialog
{
  Q_OBJECT
public:
  typedef KLFLibEngineFactory::Parameters Parameters;

  KLFLibCreateResourceDlg(const QString& defaultWtype, QWidget *parent = 0);
  virtual ~KLFLibCreateResourceDlg();

  virtual Parameters getCreateParameters() const;

  static KLFLibResourceEngine *createResource(const QString& defaultWtype, QObject *resourceParent,
					      QWidget *parent = 0);

public slots:

  virtual void accept();
  virtual void reject();

protected slots:

  virtual void updateReadyToCreateFromSender(bool isready);
  virtual void updateReadyToCreate();

private:
  Ui::KLFLibOpenResourceDlg *pUi;
  QAbstractButton *btnGo;

  Parameters pParam;
};


// --

class KLF_EXPORT KLFLibResPropEditor : public QWidget
{
  Q_OBJECT
public:
  KLFLibResPropEditor(KLFLibResourceEngine *resource, QWidget *parent = 0);
  virtual ~KLFLibResPropEditor();

public slots:
  bool apply();

protected slots:
  void slotResourcePropertyChanged(int propId);
  void slotSubResourcePropertyChanged(const QString& subResource, int propId);
  void on_btnAdvanced_toggled(bool on);
  void advPropEdited(QStandardItem *item);
  void advSubResPropEdited(QStandardItem *item);
  void on_cbxSubResource_currentIndexChanged(int newSubResItemIndex);

  void updateResourceProperties();
  void updateSubResourceProperties();
  void updateSubResources(const QString& curSubResource = QString());

private:
  KLFLibResourceEngine *pResource;
  bool pSuppSubRes;
  bool pSuppSubResProps;
  Ui::KLFLibResPropEditor *U;
  QStandardItemModel *pPropModel;
  QStandardItemModel *pSubResPropModel;

  QString curSubResource() const;
};

class KLF_EXPORT KLFLibResPropEditorDlg : public QDialog
{
  Q_OBJECT
public:
  KLFLibResPropEditorDlg(KLFLibResourceEngine *resource, QWidget *parent = 0);
  virtual ~KLFLibResPropEditorDlg();

public slots:
  void applyAndClose();
  void cancelAndClose();

private:
  KLFLibResPropEditor *pEditor;
};




class KLF_EXPORT KLFLibNewSubResDlg : public QDialog
{
  Q_OBJECT
public:
  KLFLibNewSubResDlg(KLFLibResourceEngine *resource, QWidget *parent = 0);
  virtual ~KLFLibNewSubResDlg();

  QString newSubResourceName() const;
  QString newSubResourceTitle() const;

  /** Prompt to create a sub-resource in resource \c resource. Then actually create the
   * sub-resource and return the name of the sub-resource that was created.
   *
   * Returns a null string in case of error or if the operation was canceled.
   */
  static QString createSubResourceIn(KLFLibResourceEngine *resource, QWidget *parent = 0);

  /** Choose a nice internal name for the given title. Only "nice" characters will be
   * used in the return value, namely \c "[A-Za-z0-9_]".
   *
   * If \c title only consists of allowed characters, it is returned unchanged. */
  static QString makeSubResInternalName(const QString& title);

private slots:
  void on_txtTitle_textChanged(const QString& text);
  void on_txtName_textChanged(const QString& text);

private:
  Ui::KLFLibNewSubResDlg *u;

  bool isAutoName;
};





/** \brief Interface for guessing file schemes
 *
 * This class provides the basic interface for customizing known local file types, and
 * guessing their corresponding schemes.
 *
 * To add a scheme guesser, just reimplement this function and create an instance of it.
 * It will register automatically.
 *
 * To query [all guessers instances] the scheme to use for a filename, use
 * \ref KLFLibBasicWidgetFactory::guessLocalFileScheme().
 */
class KLF_EXPORT KLFLibLocalFileSchemeGuesser
{
public:
  KLFLibLocalFileSchemeGuesser();
  virtual ~KLFLibLocalFileSchemeGuesser();

  //! Guess the appropriate scheme for handling the given file
  /** Reimplentations of this function must guess what scheme fileName is to be opened
   * with.
   *
   * By \a scheme we mean the URL scheme, ie. the scheme that the correct subclass of
   * \ref KLFLibEngineFactory reports being capable of opening (eg. \c "klf+sqlite").
   *
   * In reimplementations of this function, first the filename extension should be checked. If
   * it is not known, then the file can be peeked into for magic headers.
   *
   * If the scheme cannot be guessed, then the reimplementation should return an empty string.
   *
   * \note the \c fileName does not necessarily exist. (keep that in mind before reporting
   *   an error that you can't open the file to read a magic header). In that case, a
   *   simple test should be performed on the file extension.
   */
  virtual QString guessScheme(const QString& fileName) const = 0;
};



//! Provides some basic UIs to access resources
/**
 * Provides the following widget types for opening/creating/saving resources:
 *  - Local file (\c "LocalFile"). Don't forget to add new file types with
 *    \ref addLocalFileType() (this can be done e.g. in other engine factories'
 *    constructor).
 *  - planned, not yet implemented: remote DB connection with hostname/user/pass
 *    information collecting (tentative name \c "RemoteHostUserPass").
 *
 * \note Sub-resources are handled in \ref KLFLibOpenResourceDlg.
 *
 * \todo TODO: remote connections to eg. DB ..........
 */
class KLF_EXPORT KLFLibBasicWidgetFactory : public KLFLibWidgetFactory
{
  Q_OBJECT
public:
  //! A known local file type for \c KLFLibBasicWidgetFactory-created widgets
  struct LocalFileType {
    QString scheme; //!< eg. \c "klf+sqlite"
    QString filepattern; //!< eg. \c "*.klf.db"
    QString filter; //!< eg. \c "Local Library Database File (*.klf.db)"
  };

  KLFLibBasicWidgetFactory(QObject *parent = NULL);
  virtual ~KLFLibBasicWidgetFactory();

  virtual QStringList supportedTypes() const;

  virtual QString widgetTypeTitle(const QString& wtype) const;

  virtual QWidget * createPromptUrlWidget(QWidget *parent, const QString& scheme,
					  QUrl defaultlocation = QUrl());
  virtual QUrl retrieveUrlFromWidget(const QString& scheme, QWidget *widget);

  virtual bool hasCreateWidget(const QString& /*wtype*/) const { return true; }

  /** See \ref KLFLibWidgetFactory.
   *
   * Default parameters that can be given in \c defaultparameters:
   * - \c "Url" (type QUrl): the URL to start with
   */
  virtual QWidget * createPromptCreateParametersWidget(QWidget *parent, const QString& scheme,
						       const Parameters& defaultparameters = Parameters());

  /** The parameters returned by this function depends on the \c wtype.
   *
   * <b>Widget-type \c "LocalFile"</b>
   * - \c "Filename" : the selected local file name
   * - \c "klfRetry", \c "klfScheme" as documented in
   *   \ref KLFLibWidgetFactory::retrieveCreateParametersFromWidget().
   */
  virtual Parameters retrieveCreateParametersFromWidget(const QString& wtype, QWidget *widget);


  /** This function should be called for example in KLFLibEngineFactory subclasses' constructor
   * to inform this widget factory of local file types that are known by the various engine
   * factories. This is then eg. used to provide a useful filter choice in file dialogs.
   */
  static void addLocalFileType(const LocalFileType& fileType);
  static QList<LocalFileType> localFileTypes();

  /** Queries all the instantiated KLFLibLocalFileSchemeGuesser objects to see if one can recognize
   * the file \c fileName. The first scheme match found is returned. An empty QString is returned
   * if no guesser succeeded to recognize \c fileName.
   */
  static QString guessLocalFileScheme(const QString& fileName);

protected:
  static QList<LocalFileType> pLocalFileTypes;
  static QList<KLFLibLocalFileSchemeGuesser*> pSchemeGuessers;

  friend class KLFLibLocalFileSchemeGuesser;

  /** This function adds a scheme guesser, ie. a functional sub-class of
   * \ref KLFLibLocalFileSchemeGuesser. The instance is NOT deleted after use.
   * \c schemeguesser could for example also sub-class QObject and set \c qApp as parent.
   */
  static void addLocalFileSchemeGuesser(KLFLibLocalFileSchemeGuesser *schemeguesser);

  static void removeLocalFileSchemeGuesser(KLFLibLocalFileSchemeGuesser *schemeguesser);

};





#endif
