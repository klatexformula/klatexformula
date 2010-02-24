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
#include <QWidget>
#include <QDialog>
#include <QTreeView>

#include <klfdefs.h>
#include <klflib.h>

namespace KLFLib {
  enum RestoreMode {
    RestoreLatex = 0x0001,
    RestoreStyle = 0x0002,

    RestoreLatexAndStyle = RestoreLatex|RestoreStyle
  };
};


class KLF_EXPORT KLFAbstractLibView : public QWidget
{
  Q_OBJECT
public:
  KLFAbstractLibView(QWidget *parent);
  virtual ~KLFAbstractLibView() { }

  virtual KLFLibResourceEngine * resourceEngine() const { return pResourceEngine; }

  virtual void setResourceEngine(KLFLibResourceEngine *resource);

  virtual KLFLibEntryList selectedEntries() const = 0;

signals:
  void requestRestore(const KLFLibEntry& entry, uint restoreflags = KLFLib::RestoreLatexAndStyle);
  void requestRestoreStyle(const KLFStyle& style);

  void entriesSelected(const KLFLibEntryList& entries);
  /** Subclasses should emit this signal with lists of categories they come accross,
   * so that the editor can suggest these as completions upon editing category */
  void moreCategorySuggestions(const QStringList& categorylist);

public slots:
  virtual void updateView();
  virtual void updateResourceView() = 0;
  virtual void updateResourceProp() = 0;
  virtual void updateResourceData() = 0;
  virtual bool writeEntryProperty(int property, const QVariant& value) = 0;
  bool writeEntryCategory(const QString& category)
  { return writeEntryProperty(KLFLibEntry::Category, category); }
  bool writeEntryTags(const QString& tags)
  { return writeEntryProperty(KLFLibEntry::Tags, tags); }
  virtual bool deleteSelected(bool requireConfirm = true) = 0;
  virtual bool insertEntries(const KLFLibEntryList& entries) = 0;
  bool insertEntry(const KLFLibEntry& entry) { return insertEntries(KLFLibEntryList() << entry); }

  virtual bool searchFind(const QString& queryString, bool forward = true) = 0;
  virtual bool searchFindNext(bool forward) = 0;
  virtual void searchAbort() = 0;

  virtual void restore(uint restoreFlags = KLFLib::RestoreLatexAndStyle) = 0;
  void restoreWithStyle() { restore(KLFLib::RestoreLatexAndStyle); }
  void restoreLatexOnly() { restore(KLFLib::RestoreLatex); }

  /** Called by the owner of the view. This function fetches category suggestions
   * (by calling the virtual getCategorySuggestions() reimplemented by subclasses)
   * and emits the signal moreCategorySuggestions().
   *
   * Subclasses need not reimplement this function. */
  virtual void wantMoreCategorySuggestions();

protected:
  virtual QStringList getCategorySuggestions() = 0;

private:
  KLFLibResourceEngine *pResourceEngine;
};


// -----------------

class KLF_EXPORT KLFAbstractLibViewFactory : public QObject
{
  Q_OBJECT
public:
  KLFAbstractLibViewFactory(QObject *parent = NULL);
  virtual ~KLFAbstractLibViewFactory();

  /** An identifier for this view type (eg. "LibModel+CategoryTree" or whatever)  */
  virtual QString viewTypeIdentifier() const = 0;
  /** A translated string to be shown to user (in a choice box for ex.)
   * (eg. <tt>tr("Tree View")</tt>) */
  virtual QString viewTypeTitle() const = 0;

  /** \returns Whether this factory can create its view widget for the given engine.
   *
   * This function may return false, for example if this widget factory creates a specialized
   * widgets that can only work with a given engine. */
  virtual bool canCreateLibView(KLFLibResourceEngine *engine) = 0;

  /** Create a library view with the given widget \c parent. The view should reflect the contents
   * given by the resource engine \c resourceEngine . */
  virtual KLFAbstractLibView * createLibView(QWidget *parent, KLFLibResourceEngine *resourceEngine) = 0;


  /** Returns the factory that can handle the URL scheme \c urlScheme, or NULL if no such
   * factory exists (ie. has been registered). */
  static KLFAbstractLibViewFactory *findFactoryFor(const QString& viewTypeIdentifier);

private:
  static void registerFactory(KLFAbstractLibViewFactory *factory);
  static void unRegisterFactory(KLFAbstractLibViewFactory *factory);

  static QList<KLFAbstractLibViewFactory*> pRegisteredFactories;

};



// -----------------


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
  KLFLibModel(KLFLibResourceEngine *resource, uint flavorFlags = LinearList|GroupSubCategories,
	      QObject *parent = NULL);
  virtual ~KLFLibModel();

  enum ItemKind { EntryKind, CategoryLabelKind };
  enum {
    ItemKindItemRole = Qt::UserRole+800,
    EntryContentsTypeItemRole,
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
  static inline int entryItemRole(int propertyId) { return Qt::UserRole+810+propertyId; }
  
  virtual void setResource(KLFLibResourceEngine *resource);

  virtual KLFLibResourceEngine * resource() { return pResource; }

  enum FlavorFlag {
    LinearList = 0x0001,
    CategoryTree = 0x0002,
    DisplayTypeMask = 0x000f,

    GroupSubCategories = 0x1000
  };
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

  virtual QVariant data(const QModelIndex& index, int role) const;
  virtual Qt::ItemFlags flags(const QModelIndex& index) const;
  virtual bool hasChildren(const QModelIndex &parent) const;
  virtual QVariant headerData(int section, Qt::Orientation orientation,
			      int role = Qt::DisplayRole) const;
  virtual bool hasIndex(int row, int column,
			const QModelIndex &parent = QModelIndex()) const;
  virtual QModelIndex index(int row, int column,
			    const QModelIndex &parent = QModelIndex()) const;
  virtual QModelIndex parent(const QModelIndex &index) const;
  virtual int rowCount(const QModelIndex &parent = QModelIndex()) const;
  virtual int columnCount(const QModelIndex &parent = QModelIndex()) const;

  virtual void sort(int column, Qt::SortOrder order = Qt::AscendingOrder);

  virtual int entryColumnContentsPropertyId(int column) const;
  virtual int columnForEntryPropertyId(int entryPropertyId) const;

  virtual QStringList categoryList() const;

  virtual void updateData();

public slots:

  virtual QModelIndex searchFind(const QString& queryString, bool forward = true);
  virtual QModelIndex searchFindNext(bool forward);

  virtual bool changeEntries(const QModelIndexList& items, int property, const QVariant& value);
  virtual bool insertEntries(const KLFLibEntryList& entries);
  virtual bool deleteEntries(const QModelIndexList& items);

private:

  KLFLibResourceEngine *pResource;

  unsigned int pFlavorFlags;

  typedef qint32 IndexType;
  struct NodeId {
    NodeId(ItemKind k = EntryKind, IndexType i = -1) : kind(k), index(i)  { }
    bool valid() const { return index >= 0; }
    ItemKind kind;
    IndexType index;
  };
  struct PersistentId {
    ItemKind kind;
    KLFLibResourceEngine::entryId entry_id;
    QString categorylabel_fullpath;
  };
  struct Node {
    Node() : parent(NodeId()), children(QList<NodeId>()) { }
    NodeId parent;
    QList<NodeId> children;
    virtual int nodeKind() const = 0;
  };
  struct EntryNode : public Node {
    EntryNode() : entryid(-1), entry() { }
    virtual int nodeKind() const { return EntryKind; }
    KLFLibResourceEngine::entryId entryid;
    KLFLibEntry entry;
  };
  struct CategoryLabelNode : public Node {
    CategoryLabelNode() : categoryLabel(), fullCategoryPath()  { }
    virtual int nodeKind() const { return CategoryLabelKind; }
    //! The last element in \ref fullCategoryPath eg. "General Relativity"
    QString categoryLabel;
    //! The full category path of this category eg. "Physics/General Relativity"
    QString fullCategoryPath;
  };
  /** QVector is used since the QModelIndex s uses pointer to individual
   * nodes */
  typedef QVector<EntryNode> EntryCache;
  /** QVector is used since the QModelIndex s uses pointer to individual
   * nodes */
  typedef QVector<CategoryLabelNode> CategoryLabelCache;

  EntryCache pEntryCache;
  CategoryLabelCache pCategoryLabelCache;

  /** If row is negative, it will be looked up automatically. */
  QModelIndex createIndexFromId(NodeId nodeid, int row, int column) const;
  /** If row is negative, it will be looked up automatically. */
  QModelIndex createIndexFromPtr(Node *node, int row, int column) const;
  Node * getNodeForIndex(const QModelIndex& index) const;
  Node * getNode(NodeId nodeid) const;
  NodeId getNodeId(Node *node) const;
  /** get the row of \c nodeid in its parent. if you already have a pointer to the 
   * \ref Node with id \c nodeid, pass it to \c node pointer (slight optimization).
   * Otherwise pass NULL pointer and getNodeRow() will figure out the pointer with
   * a call to getNode().  */
  int getNodeRow(NodeId nodeid, Node *node = NULL) const;
  inline int getNodeRow(Node * node) const { return getNodeRow(getNodeId(node), node); }

  void updateCacheSetupModel();

  IndexType cacheFindCategoryLabel(QString category, bool createIfNotExists = false);

  void dumpNodeTree(Node *node, int indent = 0) const;

  class KLF_EXPORT KLFLibModelSorter
  {
  public:
    KLFLibModelSorter(KLFLibModel *m, int entry_prop, Qt::SortOrder sort_order, bool groupcategories)
      : model(m), entryProp(entry_prop), sortOrderFactor( (sort_order == Qt::AscendingOrder) ? -1 : 1),
	groupCategories(groupcategories) { }
    bool operator()(const NodeId& a, const NodeId& b);
  private:
    KLFLibModel *model;
    int entryProp;
    int sortOrderFactor;
    bool groupCategories;
  };
  QString nodeValue(Node *ptr, int entryProperty);
  void sortCategory(CategoryLabelNode *categoryPtr, int column, Qt::SortOrder order);

  int lastSortColumn;
  Qt::SortOrder lastSortOrder;

  /** Walks the whole tree returning all the nodes one after the other, in the following order:
   * if \c n has children, first child is returned; otherwise next sibling is returned.
   *
   * This function returns all nodes in the order they would be displayed in a tree view.
   *
   * Returns \c NULL after last node. Returns first node in tree if \c NULL is given as
   * paremeter. */
  Node * nextNode(Node *n);
  /** Same as \ref nextNode() but the walk is performed in the opposite direction.
   *
   * This function returns all nodes in the inverse order they would be displayed in a tree view. In
   * particular, it returns a parent node after having explored its children. */
  Node * prevNode(Node *n);
  /** Returns the last node in tree defined by node \c n.
   *
   * If \c n has children, returns last child of the last child of the last child etc. If \c n does
   * not have children, it is itself returned.
   *
   * If \c NULL is given, the root node is assumed. */
  Node * lastNode(Node *n);

  QList<KLFLibResourceEngine::entryId> entryIdList(const QModelIndexList& indexlist) const;
  
  QString pSearchString;
  Node *pSearchCurNode;
};

// -----------------

class KLF_EXPORT KLFLibViewDelegate : public QAbstractItemDelegate
{
  Q_OBJECT
public:
  KLFLibViewDelegate(QObject *parent);
  virtual ~KLFLibViewDelegate();

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

protected:
  virtual void paintEntry(QPainter *painter, const QStyleOptionViewItem& option,
			  const QModelIndex& index) const;
  virtual void paintCategoryLabel(QPainter *painter, const QStyleOptionViewItem& option,
				  const QModelIndex& index) const;

private:
  /** Temporary variable paint() sets for subcalls to paintEntry() and paintCategoryLabel() */
  mutable QRect innerRectText, innerRectImage;
  /** Temporary variable paint() sets for subcalls to paintEntry() and paintCategoryLabel() */
  mutable bool isselected;

  QString pSearchString;
};

// -----------------

class KLF_EXPORT KLFLibDefaultView : public KLFAbstractLibView
{
  Q_OBJECT
public:
  KLFLibDefaultView(QWidget *parent);
  virtual ~KLFLibDefaultView();

  virtual KLFLibEntryList selectedEntries() const;

public slots:
  virtual bool writeEntryProperty(int property, const QVariant& value);
  virtual bool deleteSelected(bool requireConfirmation = true);
  virtual bool insertEntries(const KLFLibEntryList& entries);

  virtual bool searchFind(const QString& queryString, bool forward = true);
  virtual bool searchFindNext(bool forward);
  virtual void searchAbort();

  virtual void restore(uint restoreflags = KLFLib::RestoreLatexAndStyle);

protected:
  virtual void updateResourceView();
  virtual void updateResourceProp();
  virtual void updateResourceData();
  virtual QStringList getCategorySuggestions();

protected slots:
  void slotViewSelectionChanged(const QItemSelection& selected, const QItemSelection& deselected);
  void slotEntryDoubleClicked(const QModelIndex& index);

private:
  QTreeView *pTreeView;
  KLFLibViewDelegate *pDelegate;
  KLFLibModel *pModel;

  QModelIndexList selectedEntryIndexes() const;

private slots:
  void searchFound(const QModelIndex& i);
};

// -----------------

class KLF_EXPORT KLFLibDefaultViewFactory : public KLFAbstractLibViewFactory
{
  Q_OBJECT
public:
  KLFLibDefaultViewFactory(QObject *parent = NULL) : KLFAbstractLibViewFactory(parent) { }

  virtual QString viewTypeIdentifier() const { return "default"; }
  virtual QString viewTypeTitle() const { return tr("Default View"); }

  virtual bool canCreateLibView(KLFLibResourceEngine */*engine*/) { return true; }

  virtual KLFAbstractLibView * createLibView(QWidget *parent, KLFLibResourceEngine *resourceEngine);
};



// -----------------

namespace Ui {
  class KLFLibOpenResourceDlg;
  class KLFLibResPropEditor;
};

class KLF_EXPORT KLFLibOpenResourceDlg : public QDialog
{
  Q_OBJECT
public:
  KLFLibOpenResourceDlg(const QUrl& defaultlocation = QUrl(), QWidget *parent = 0);
  virtual ~KLFLibOpenResourceDlg();

  virtual QUrl url() const;

  static QUrl queryOpenResource(const QUrl& defaultlocation = QUrl(), QWidget *parent = 0);

private:
  Ui::KLFLibOpenResourceDlg *pUi;
};


class KLFLibResPropEditor : public QWidget
{
  Q_OBJECT
public:
  KLFLibResPropEditor(KLFLibResourceEngine *resource, QWidget *parent = 0);
  virtual ~KLFLibResPropEditor();

public slots:
  bool apply();

private:
  KLFLibResourceEngine *pResource;
  Ui::KLFLibResPropEditor *pUi;
};

class KLFLibResPropEditorDlg : public QDialog
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


#endif