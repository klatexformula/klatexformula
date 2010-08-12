/***************************************************************************
 *   file klflibview.cpp
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


#include <QApplication>
#include <QDebug>
#include <QImage>
#include <QString>
#include <QDataStream>
#include <QMessageBox>
#include <QAbstractItemModel>
#include <QModelIndex>
#include <QPainter>
#include <QStyle>
#include <QVBoxLayout>
#include <QStackedWidget>
#include <QComboBox>
#include <QHeaderView>
#include <QTextDocument>
#include <QTextCursor>
#include <QTextCharFormat>
#include <QListView>
#include <QMenu>
#include <QAction>
#include <QEvent>
#include <QDropEvent>
#include <QDragEnterEvent>
#include <QDragMoveEvent>
#include <QStandardItemModel>
#include <QItemDelegate>

#include <ui_klflibopenresourcedlg.h>
#include <ui_klflibrespropeditor.h>
#include <ui_klflibnewsubresdlg.h>

#include "klfconfig.h"
#include "klflibview.h"
#include "klfguiutil.h"

#include "klflibview_p.h"


// #ifndef KLF_DEBUG
// // Qt doesn't provide this operator out of debug mode, however we need it to feed
// // into qWarning()<< ...
// QDebug& operator<<(QDebug& dbg, const QModelIndex &index)
// {
//   return dbg << "QModelIndex(row="<<index.row()<<";data="<<index.internalPointer()<<")";
// }
// #endif


// ---------------------------------------------------

/** \internal */
static QImage transparentify_image(const QImage& img, qreal factor)
{
  // set the image opacity to factor (by multiplying each alpha value by factor)
  QImage img2 = img.convertToFormat(QImage::Format_ARGB32_Premultiplied);
  int k, j;
  for (k = 0; k < img.height(); ++k) {
    for (j = 0; j < img.width(); ++j) {
      QRgb c = img2.pixel(j, k);
      img2.setPixel(j, k, qRgba(qRed(c),qGreen(c),qBlue(c),(int)(factor*qAlpha(c))));
    }
  }
  return img2;
}


/** \internal */
static QImage autocrop_image(const QImage& img, int alpha_threshold = 0)
{
  // crop transparent borders
  int x, y;
  int min_x = -1, max_x = -1, min_y = -1, max_y = -1;
  // so first find borders
  for (x = 0; x < img.width(); ++x) {
    for (y = 0; y < img.height(); ++y) {
      if (qAlpha(img.pixel(x,y)) - alpha_threshold > 0) {
	// opaque pixel: include it.
	if (min_x == -1 || min_x > x) min_x = x;
	if (max_x == -1 || max_x < x) max_x = x;
	if (min_y == -1 || min_y > y) min_y = y;
	if (max_y == -1 || max_y < y) max_y = y;
      }
    }
  }
  return img.copy(QRect(QPoint(min_x, min_y), QPoint(max_x, max_y)));
}

/** \internal */
static float color_distinguishable_distance(QRgb a, QRgb b, bool aPremultiplied = false) {
  static const float C_r = 11.f,   C_g = 16.f,   C_b = 5.f;
  static const float C_avg = (C_r + C_g + C_b) / 3.f;

  // (?) really-> NO?  ON SECOND THOUGHT, REMOVE THIS FACTOR
  //  // * drkfactor reduces distances for dark colors, accounting for the fact that the eye
  //  //   distinguishes less dark colors than light colors
  //  // * 0 <= qGray(...) <= 255
  //  // * drkfactor <= 1 -> reducing factor
  //  float drkfactor = 1 - (qGray(b)/1000.f);
  static const float drkfactor = 1;

  float alpha = qAlpha(a)/255.f;
  QRgb m;
  if (aPremultiplied)
    m = qRgb((int)(qRed(a)+(1-alpha)*qRed(b)),
	     (int)(qGreen(a)+(1-alpha)*qGreen(b)),
	     (int)(qBlue(a)+(1-alpha)*qBlue(b)));
  else
    m = qRgb((int)(alpha*qRed(a)+(1-alpha)*qRed(b)),
	     (int)(alpha*qGreen(a)+(1-alpha)*qGreen(b)),
	     (int)(alpha*qBlue(a)+(1-alpha)*qBlue(b)));

  float dst = qMax( qMax(C_r*abs(qRed(m) - qRed(b)), C_g*abs(qGreen(m) - qGreen(b))),
		    C_b*abs(qBlue(m) - qBlue(b)) ) * drkfactor / C_avg;
  // klfDbg("a="<<klfFmt("%#010x", a).data()<<" qRed(a)="<<qRed(a)<<" b="<<klfFmt("%#010x", b).data()
  //	 <<" m="<<klfFmt("%#010x", m)<<"drkfactor="<<drkfactor<<" a/alpha="<<alpha<<" => distance="<<dst) ;
  return dst;
}


/** \internal */
static bool image_is_distinguishable(const QImage& imgsrc, QColor background, float threshold)
{
  int fmt = imgsrc.format();
  bool apremultiplied;
  QImage img;
  switch (fmt) {
  case QImage::Format_ARGB32: img = imgsrc; apremultiplied = false; break;
  case QImage::Format_ARGB32_Premultiplied: img = imgsrc; apremultiplied = true; break;
  default:
   img = imgsrc.convertToFormat(QImage::Format_ARGB32);
   apremultiplied = false;
   break;
  }
  QRgb bg = background.rgb();
  // crop transparent borders
  int x, y;
  for (x = 0; x < img.width(); ++x) {
    for (y = 0; y < img.height(); ++y) {
      //      klfDbg("src/format="<<imgsrc.format()<<"; thisformat="<<img.format()
      //	     <<" Testing pixel at ("<<x<<","<<y<<") pixel="<<klfFmt("%#010x", img.pixel(x,y))) ;
      float dist = color_distinguishable_distance(img.pixel(x,y), bg, apremultiplied);
      if (dist > threshold) {
	// ok we have one pixel at least we can distinguish.
	return true;
      }
    }
  }
  return false;
}



// -------------------------------------------------------

KLFAbstractLibView::KLFAbstractLibView(QWidget *parent)
  : QWidget(parent), pResourceEngine(NULL)
{
}

void KLFAbstractLibView::setResourceEngine(KLFLibResourceEngine *resource)
{
  if (pResourceEngine)
    disconnect(pResourceEngine, 0, this, 0);
  pResourceEngine = resource;
  connect(pResourceEngine, SIGNAL(dataChanged(const QString&, int, const QList<KLFLib::entryId>&)),
	  this, SLOT(updateResourceData(const QString&, int, const QList<KLFLib::entryId>&)));
  connect(pResourceEngine, SIGNAL(resourcePropertyChanged(int)),
	  this, SLOT(updateResourceProp(int)));
  connect(pResourceEngine, SIGNAL(defaultSubResourceChanged(const QString&)),
	  this, SLOT(updateResourceDefaultSubResourceChanged(const QString&)));
  updateResourceEngine();
}

void KLFAbstractLibView::updateResourceDefaultSubResourceChanged(const QString& /*subResource*/)
{
  updateResourceEngine();
}

void KLFAbstractLibView::wantMoreCategorySuggestions()
{
  emit moreCategorySuggestions(getCategorySuggestions());
}

QList<QAction*> KLFAbstractLibView::addContextMenuActions(const QPoint&)
{
  return QList<QAction*>();
}



// -------------------------------------------------------

QList<KLFLibViewFactory*> KLFLibViewFactory::pRegisteredFactories =
	 QList<KLFLibViewFactory*>();


KLFLibViewFactory::KLFLibViewFactory(const QStringList& viewTypeIdentifiers,
						     QObject *parent)
  : QObject(parent), pViewTypeIdentifiers(viewTypeIdentifiers)
{
  registerFactory(this);
}
KLFLibViewFactory::~KLFLibViewFactory()
{
  unRegisterFactory(this);
}


QString KLFLibViewFactory::defaultViewTypeIdentifier()
{
  if (pRegisteredFactories.size() > 0)
    return pRegisteredFactories[0]->pViewTypeIdentifiers.first();
  return QString();
}

KLFLibViewFactory *KLFLibViewFactory::findFactoryFor(const QString& viewTypeIdentifier)
{
  if (viewTypeIdentifier.isEmpty()) {
    if (pRegisteredFactories.size() > 0)
      return pRegisteredFactories[0]; // first registered factory is default
    return NULL;
  }
  int k;
  // walk registered factories, and return the first that supports this scheme.
  for (k = 0; k < pRegisteredFactories.size(); ++k) {
    if (pRegisteredFactories[k]->viewTypeIdentifiers().contains(viewTypeIdentifier))
      return pRegisteredFactories[k];
  }
  // no factory found
  return NULL;
}

QStringList KLFLibViewFactory::allSupportedViewTypeIdentifiers()
{
  QStringList list;
  int k;
  for (k = 0; k < pRegisteredFactories.size(); ++k)
    list << pRegisteredFactories[k]->viewTypeIdentifiers();
  return list;
}


void KLFLibViewFactory::registerFactory(KLFLibViewFactory *factory)
{
  KLF_ASSERT_NOT_NULL( factory, "Attempt to register NULL factory!", return )
    ;
  // WARNING: THIS FUNCTION IS CALLED FROM CONSTRUCTOR. NO VIRTUAL METHOD CALLS!
  if (factory->pViewTypeIdentifiers.size() == 0) {
    qWarning("KLFLibViewFactory::registerFactory: factory must provide at least one view type!");
    return;
  }
  if (pRegisteredFactories.indexOf(factory) != -1) // already registered
    return;
  pRegisteredFactories.append(factory);
}

void KLFLibViewFactory::unRegisterFactory(KLFLibViewFactory *factory)
{
  if (pRegisteredFactories.indexOf(factory) == -1)
    return;
  pRegisteredFactories.removeAll(factory);
}








// -------------------------------------------------------

//   MODEL CACHE OBJECT

QDebug& operator<<(QDebug& dbg, const KLFLibModelCache::NodeId& n)
{
  const char * skind =
    ( (n.kind == KLFLibModelCache::EntryKind) ? "EntryKind" :
      ((n.kind == KLFLibModelCache::CategoryLabelKind) ? "CategoryLabelKind" :
       "*InvalidKind*") );
  return dbg.nospace() << "NodeId("<<skind<<"+"<<n.index<<")";
}
QDebug& operator<<(QDebug& dbg, const KLFLibModelCache::EntryNode& en)
{
  return dbg << "EntryNode(entryid="<<en.entryid<<"; entry/latex="<<en.entry.latex()<<"; parent="
	     <<en.parent<<")";
}
QDebug& operator<<(QDebug& dbg, const KLFLibModelCache::CategoryLabelNode& cn)
{
  return dbg << "CategoryLabelNode(label="<<cn.categoryLabel<<";fullCategoryPath="<<cn.fullCategoryPath
	     <<"; parent="<<cn.parent<<")";
}

bool KLFLibModelCache::KLFLibModelSorter::operator()(const NodeId& a, const NodeId& b)
{
  if ( ! a.valid() || ! b.valid() )
    return false;

  if ( groupCategories ) {
    // category kind always smaller than entry kind in category grouping mode
    if ( a.kind != EntryKind && b.kind == EntryKind ) {
      return true;
    } else if ( a.kind == EntryKind && b.kind != EntryKind ) {
      return false;
    }
    if ( a.kind != EntryKind && b.kind != EntryKind ) {
      if ( ! (a.kind == CategoryLabelKind && b.kind == CategoryLabelKind) ) {
	qWarning("KLFLibModelSorter::operator(): Bad node kinds!! a: %d and b: %d",
		 a.kind, b.kind);
	return false;
      }
      // when grouping sub-categories, always sort the categories *ascending*
      return QString::localeAwareCompare(cache->getCategoryLabelNodeRef(a).categoryLabel,
					 cache->getCategoryLabelNodeRef(b).categoryLabel)  < 0;
    }
    // both are entrykind
    return entrysorter->operator()(cache->getEntryNodeRef(a).entry, cache->getEntryNodeRef(b).entry);
  }

  int entryProp = entrysorter->propId();
  int sortorderfactor = (entrysorter->order() == Qt::AscendingOrder) ? 1 : -1;
  QString nodevaluea = cache->nodeValue(a, entryProp);
  QString nodevalueb = cache->nodeValue(b, entryProp);
  return sortorderfactor*QString::localeAwareCompare(nodevaluea, nodevalueb) < 0;
}



// ---

void KLFLibModelCache::rebuildCache()
{
  uint flavorFlags = pModel->pFlavorFlags;

  KLF_DEBUG_TIME_BLOCK(KLF_FUNC_NAME) ; qDebug("\tflavorFlags=%#010x", flavorFlags);
  int k;

  QModelIndexList persistentIndexes = pModel->persistentIndexList();
  QList<KLFLibModel::PersistentId> persistentIndexIds = pModel->persistentIdList(persistentIndexes);

  // clear cache first
  pEntryCache.clear();
  pCategoryLabelCache.clear();
  CategoryLabelNode root = CategoryLabelNode();
  root.fullCategoryPath = "/";
  root.categoryLabel = "/";
  // root category label MUST ALWAYS (in every display flavor) occupy index 0 in category
  // label cache
  pCategoryLabelCache.append(root);

  QList<int> wantedProps = minimalistEntryPropIds();
  QList<KLFLibResourceEngine::KLFLibEntryWithId> everything = pModel->pResource->allEntries(wantedProps);
  for (k = 0; k < everything.size(); ++k) {
    klfDbgT( "Adding entry id="<<everything[k].id<<"; entry="
	    <<everything[k].entry ) ;
    EntryNode e;
    e.entryid = everything[k].id;
    e.minimalist = true;
    e.entry = everything[k].entry;
    pEntryCache.append(e);
    NodeId entryindex;
    entryindex.kind = EntryKind;
    entryindex.index = pEntryCache.size()-1;
    treeInsertEntry(entryindex, false);
  }

  QModelIndexList newPersistentIndexes = pModel->newPersistentIndexList(persistentIndexIds);
  // and refresh all persistent indexes
  pModel->changePersistentIndexList(persistentIndexes, newPersistentIndexes);

  fullDump(); // DEBUG

  emit pModel->reset();

  klfDbg( " end of func" ) ;
}


QModelIndex KLFLibModelCache::createIndexFromId(NodeId nodeid, int row, int column)
{
  if ( ! nodeid.valid() || nodeid == NodeId::rootNode())
    return QModelIndex();

  // if row is -1, then we need to find the row of the item
  if ( row < 0 ) {
    row = getNodeRow(nodeid);
  }

  // make sure all elements have been "fetched" up to this row

  klfDbg( ": nodeid="<<nodeid<<"; row="<<row<<"; col="<<column ) ;

  // get the parent node
  Node node = getNode(nodeid);
  NodeId parentid = node.parent;
  if (!parentid.valid())
    parentid = NodeId::rootNode();

  // if we cache getNode(parentid) make sure to keep a reference! it changes!
  while ( getNode(parentid).numDisplayFetched <= row && canFetchMore(parentid) ) {
    klfDbgT(": need to fetch more numDisplayFetched="
	   <<getNode(parentid).numDisplayFetched<<"<row="<<row<<" !");
    fetchMore(parentid, pModel->fetchBatchCount());
  }

  // create & return the index
  return pModel->createIndex(row, column, nodeid.universalId());
}


KLFLibModelCache::NodeId KLFLibModelCache::getNodeForIndex(const QModelIndex& index)
{
  if ( ! index.isValid() )
    return NodeId();
  return NodeId::fromUID(index.internalId());
}

KLFLibModelCache::Node KLFLibModelCache::getNode(NodeId nodeid)
{
  if (!nodeid.valid())
    return CategoryLabelNode();
  Node &n = getNodeRef(nodeid);
  return n;
}
KLFLibModelCache::Node& KLFLibModelCache::getNodeRef(NodeId nodeid)
{
  if (!nodeid.valid()) {
    qWarning()<<"KLFLibModelCache::getNodeRef: Invalid Node Id: "<<nodeid;
    return pInvalidEntryNode;
  }
  if (nodeid.kind == EntryKind) {
    if (nodeid.index < 0 || nodeid.index >= pEntryCache.size()) {
      qWarning()<<"KLFLibModelCache::getNodeRef: Invalid Entry Node Id: "<<nodeid<<" : index out of range!";
      return pInvalidEntryNode;
    }
    return pEntryCache[nodeid.index];
  } else if (nodeid.kind == CategoryLabelKind) {
    if (nodeid.index < 0 || nodeid.index >= pCategoryLabelCache.size()) {
      qWarning()<<"KLFLibModelCache::getNodeRef: Invalid Category Label Node Id: "<<nodeid
		<<" : index out of range!";
      return pInvalidEntryNode;
    }
    return pCategoryLabelCache[nodeid.index];
  }
  qWarning("KLFLibModelCache::getNodeRef(): Invalid kind: %d (index=%d)\n", nodeid.kind, nodeid.index);
  return pInvalidEntryNode;
}
KLFLibModelCache::EntryNode& KLFLibModelCache::getEntryNodeRef(NodeId nodeid, bool enforceNotMinimalist)
{
  if (!nodeid.valid() || nodeid.kind != EntryKind ||
      nodeid.index < 0 || nodeid.index >= pEntryCache.size()) {
    qWarning()<<"KLFLibModelCache::getEntryNodeRef: Invalid Entry Node "<<nodeid<<"!";
    return pEntryCache[0];
  }
  if (enforceNotMinimalist && pEntryCache[nodeid.index].minimalist)
    ensureNotMinimalist(nodeid);

  return pEntryCache[nodeid.index];
}

KLFLibModelCache::CategoryLabelNode& KLFLibModelCache::getCategoryLabelNodeRef(NodeId nodeid)
{
  if (!nodeid.valid() || nodeid.kind != CategoryLabelKind ||
      nodeid.index < 0 || nodeid.index >= pCategoryLabelCache.size()) {
    qWarning()<<"KLFLibModelCache::getCat.LabelNode: Invalid Category Label Node "<<nodeid<<"!";
    return pCategoryLabelCache[0];
  }
  return pCategoryLabelCache[nodeid.index];
}

int KLFLibModelCache::getNodeRow(NodeId node)
{
  if ( ! node.valid() )
    return -1;
  if ( node == NodeId::rootNode() )
    return 0;

  Node n = getNode(node);

  NodeId pparentid = n.parent;
  if ( !pparentid.valid() ) {
    // shouldn't happen (!?!), only parentless item should be root node !
    qWarning()<<"Found parentless non-root node !";
    return 0;
  }
  Node pparent = getNode(pparentid);
  // find child in parent
  int k;
  for (k = 0; k < pparent.children.size(); ++k)
    if (pparent.children[k] == node)
      return k;

  // search failed
  qWarning("KLFLibModelCache: Unable to get node row: parent-child one-way broken!!");
  return -1;
}

void KLFLibModelCache::ensureNotMinimalist(NodeId p, int countdown)
{
  KLF_DEBUG_BLOCK(KLF_FUNC_NAME) ;
  // fetch & complete some minimalist entry(ies)
  NodeId n;
  // prepare some entry IDs to fetch
  QMap<KLFLib::entryId, NodeId> wantedIds;
  if (countdown < 0)
    countdown = pModel->pFetchBatchCount;
  //  n = prevNode(p);
  //  if (!n.valid())
  n = p;
  for (; n.valid() && countdown-- > 0; n = nextNode(n)) {
    if (n.kind == CategoryLabelKind) {
      ++countdown; // don't count category labels
      continue;
    }
    if (n.kind == EntryKind) {
      EntryNode en = getEntryNodeRef(n);
      if (en.minimalist) // if this entry is "minimalist", update it
	wantedIds[en.entryid] = n;
      continue;
    }
    qWarning()<<KLF_FUNC_NAME<<": Got unknown kind="<<n.kind;
  }
  // fetch the required entries
  QList<KLFLibResourceEngine::KLFLibEntryWithId> updatedentries =
    pModel->pResource->entries(wantedIds.keys(), QList<int>()); // fetch all properties
  int k;
  for (k = 0; k < updatedentries.size(); ++k) {
    KLFLib::entryId eid = updatedentries[k].id;
    if ( ! wantedIds.contains(eid) ) {
      qWarning()<<KLF_FUNC_NAME<<" got unrequested updated entry ID ?! id="<<eid;
      continue;
    }
    NodeId nid = wantedIds[eid];
    pEntryCache[nid.index].entry = updatedentries[k].entry;
    pEntryCache[nid.index].minimalist = false;
  }
  klfDbg( ": updated entries "<<wantedIds.keys() ) ;
}

bool KLFLibModelCache::canFetchMore(NodeId parentId)
{
  if (pIsFetchingMore)
    return false;

  if (!parentId.valid())
    parentId = NodeId::rootNode();
  const Node& node = getNodeRef(parentId);
  if (node.numDisplayFetched < node.children.size())
    return true;
  return false;
}
void KLFLibModelCache::fetchMore(NodeId n, int fetchBatchCount)
{
  KLF_DEBUG_BLOCK(KLF_FUNC_NAME); klfDbg( "\t parentId: n="<<n<<"; valid="<<n.valid() ) ;

  if (pIsFetchingMore)
    return;
  pIsFetchingMore = true;

  // see function doxygen doc for nIndex param info.

  if (!n.valid())
    n = NodeId::rootNode();


  Node& node = getNodeRef(n);
  klfDbg( "\t -> n="<<n<<"; node: kind="<<node.kind<<", numDisplayFetched="
	  <<node.numDisplayFetched ) ;

  if (node.numDisplayFetched >= node.children.size()) {
    pIsFetchingMore = false;
    return;
  }

  QModelIndex parent = createIndexFromId(n, -1, 0);

  // don't exceed children size
  int fetchNum = qMin(fetchBatchCount, node.children.size() - node.numDisplayFetched);
  klfDbg( "; children/size="<<node.children.size()<<"; batchCount="<<fetchBatchCount
	  <<"; numDisplayFetched="<<node.numDisplayFetched<<"; => fetchNum="<<fetchNum ) ;
  pModel->beginInsertRows(parent, node.numDisplayFetched, node.numDisplayFetched+fetchNum-1);
  node.numDisplayFetched += fetchNum;
  klfDbg( ": increased node.numDisplayFetched by "<<fetchNum<<" to "<<node.numDisplayFetched ) ;
  pModel->endInsertRows();

  pIsFetchingMore = false;
}



void KLFLibModelCache::updateData(const QList<KLFLib::entryId>& entryIdList, int modifyType)
{
  klfDbg( "modifyType="<<modifyType<<" entryIdList="<<entryIdList ) ;

  if (modifyType == KLFLibResourceEngine::UnknownModification) {
    rebuildCache();
    return;
  }

  if (entryIdList.size() > 10 &&
      (entryIdList.size() > pEntryCache.size()/3 || entryIdList.size() > 100)) {
    // too big a modification, just rebuild the cache
    rebuildCache();
    return;
  }

  switch (modifyType) {
  case KLFLibResourceEngine::InsertData:
    {	// entries inserted
      QList<KLFLibResourceEngine::KLFLibEntryWithId> entryList = pModel->pResource->entries(entryIdList);
      int k;
      for (k = 0; k < entryList.size(); ++k) {
	EntryNode en;
	en.entryid = entryList[k].id;
	en.minimalist = false;
	en.entry = entryList[k].entry;
	pEntryCache.append(en);      // append the entry to cache
	NodeId n = NodeId(EntryKind, pEntryCache.size()-1);
	treeInsertEntry(n);
	qDebug("%s: entry ID %d inserted", KLF_FUNC_NAME, entryIdList[k]);
      }
      break;
    }
  case KLFLibResourceEngine::ChangeData:
    { // entry moved in category tree or just changed
      QList<KLFLibResourceEngine::KLFLibEntryWithId> entryList = pModel->pResource->entries(entryIdList);
      int k;
      for (k = 0; k < entryIdList.size(); ++k) {
	qDebug("%s: modifying entry ID=%d.", KLF_FUNC_NAME, entryIdList[k]);
	NodeId n = findEntryId(entryIdList[k]);
	KLFLibEntry oldentry = pEntryCache[n.index].entry;
	KLFLibEntry newentry = entryList[k].entry;
	pEntryCache[n.index].entry = newentry;
	// the modified entry may have a different category, move it if needed
	if (newentry.category() != oldentry.category()) {
	  pModel->startLayoutChange();
	  treeRemoveEntry(n, false); // remove it from its position in tree
	  // klfDbg( "\tremoved entry. dump:\n"
	  //         <<"\t Entry Cache="<<pEntryCache<<"\n\t CategoryLabelCache = "<<pCategoryLabelCache ) ;
	  // dumpNodeTree(NodeId::rootNode());
	  // revalidate the removed entry
	  pEntryCache[n.index].entryid = entryIdList[k];
	  // and add it again at the (new) correct position (automatically positioned!)
	  treeInsertEntry(n, false);
	  pModel->endLayoutChange();
	  QModelIndex idx = createIndexFromId(n, -1, 0);
	  emit pModel->dataChanged(idx, idx);
	} else {
	  // just some data change
	  QModelIndex idx = createIndexFromId(n, -1, 0);
	  emit pModel->dataChanged(idx, idx);
	}
      }
      break;
    }
  case KLFLibResourceEngine::DeleteData:
    { // entry removed
      int k;
      for (k = 0; k < entryIdList.size(); ++k) {
	qDebug("%s: deleting entry ID=%d.", KLF_FUNC_NAME, entryIdList[k]);
	// entry deleted
	NodeId n = findEntryId(entryIdList[k]);
	treeRemoveEntry(n);
      }
      break;
    }
  default:
    {
      qWarning()<<KLF_FUNC_NAME<<": Bad modify-type parameter: "<<modifyType;
      rebuildCache();
      return;
    }
  }

  //  updateCacheSetupModel();
  klfDbg( ": udpated; full tree dump:" ) ;
  fullDump();
}


void KLFLibModelCache::treeInsertEntry(const NodeId& n, bool notifyQtApi)
{
  KLF_DEBUG_TIME_BLOCK(KLF_FUNC_NAME) ;

  klfDbg( "n="<<n<<",notifyQtApi="<<notifyQtApi<<"" ) ;

  if (n.kind != EntryKind) {
    qWarning()<<"KLFLibModelCache::treeInsertEntry("<<n<<"): bad kind!";
    return;
  }
  if (n.index < 0 || n.index >= pEntryCache.size()) {
    qWarning()<<KLF_FUNC_NAME
	      <<"Requested to insert entry to cache tree which is NOT A VALID ENTRY INDEX: "<<n;
    return;
  }

  // first find the appropriate category label parent

  EntryNode e = pEntryCache[n.index];
  QString category = e.entry.category();
  // fix category: remove any double-/ to avoid empty sections. (goal: ensure that join(split(c))==c )
  category = category.split('/', QString::SkipEmptyParts).join("/");
  
  // parse its catelements and remember that in the category list cache (that is useful only to
  // suggest known categories to user at given occasions)
  QStringList catelements = category.split('/', QString::SkipEmptyParts);
  for (int kl = 0; kl < catelements.size(); ++kl) {
    QString c = QStringList(catelements.mid(0,kl+1)).join("/");
    if (!pCatListCache.contains(c)) {
      pCatListCache.insert(qLowerBound(pCatListCache.begin(), pCatListCache.end(), c), c);
    }
  }

  IndexType catindex;
  if (pModel->displayType() == KLFLibModel::LinearList) {
    // add as child of root element
    catindex = 0;
  } else if (pModel->displayType() == KLFLibModel::CategoryTree) {
    // find/create category label node (creating the full tree if needed)
    catindex = cacheFindCategoryLabel(catelements, true, notifyQtApi);
  } else {
    qWarning("Flavor Flags have unknown display type! flavorFlags=%#010x", pModel->pFlavorFlags);
    catindex = 0;
  }

  NodeId parentid = NodeId(CategoryLabelKind, catindex);
  pEntryCache[n.index].parent = parentid; // set the parent

  // now we determined the parent of the new entry in the category tree, we will actually
  // insert the item according to current sort instructions.
  Node& parentref = getNodeRef(parentid);
  QModelIndex parentidx = createIndexFromId(parentid, -1, 0);
  KLFLibModelSorter srt =
    KLFLibModelSorter(this, pModel->pEntrySorter, pModel->pFlavorFlags & KLFLibModel::GroupSubCategories);
  QList<NodeId> & childlistref = parentref.children;
  int insertPos;
  if (pLastSortPropId < 0) {
    insertPos = childlistref.size(); // no sorting, just append the item
  } else {
    // qLowerBound returns an iterator. subtract begin() to get absolute index
    insertPos = qLowerBound(childlistref.begin(), childlistref.end(), n, srt) - childlistref.begin();
  }
  // now remove the temporarily appended item to insertPos
  CategoryLabelNode &catLabelNodeRef = getCategoryLabelNodeRef(parentid);

  // and insert it again at the required spot
  if (notifyQtApi) {
    if (catLabelNodeRef.numDisplayFetched < insertPos) {
      pModel->beginInsertRows(parentidx, catLabelNodeRef.numDisplayFetched, insertPos+1);
      catLabelNodeRef.numDisplayFetched = insertPos+1;
    } else {
      pModel->beginInsertRows(parentidx, insertPos, insertPos);
      catLabelNodeRef.numDisplayFetched++;
    }
  }
  qDebug("\tinserting (%d,%d) at pos %d in category '%s'", n.kind, n.index, insertPos,
	 qPrintable(catLabelNodeRef.fullCategoryPath));
  childlistref.insert(insertPos, n);

  if (notifyQtApi)
    pModel->endInsertRows();
}

void KLFLibModelCache::treeRemoveEntry(const NodeId& nodeid, bool notifyQtApi)
{
  KLF_DEBUG_TIME_BLOCK(KLF_FUNC_NAME) ;

  klfDbg( "KLFLibModelCache::treeRemoveEntry("<<nodeid<<","<<notifyQtApi<<")" ) ;
  NodeId n = nodeid;
  // keep it in cache to keep indexes valid, just remove its reference.
  // then remove all empty categories above the removed item.
  // also invalidate the entry so findEntryIdList() doesn't find it.
  if (nodeid.kind == EntryKind) {
    getEntryNodeRef(nodeid).entryid = -1;
  }
  NodeId parentid;
  bool willRemoveParent;
  do {
    parentid = getNode(n).parent;
    if (parentid.kind != CategoryLabelKind) {
      qWarning()<<"KLFLibModelCache::treeRemoveEntry("<<n<<"): Bad parent node kind: "<<parentid.kind<<"!";
      return;
    }
    QModelIndex parent = createIndexFromId(parentid, -1, 0);
    int childIndex = pCategoryLabelCache[parentid.index].children.indexOf(n);
    if (childIndex < 0) {
      qWarning()<<"KLFLibModelCache::treeRemoveEntry("<<n<<"): !!?! bad child-parent relation, can't find "<<n
		<<" in child list "<<pCategoryLabelCache[parentid.index].children<<"; full dump:\n"
		<<"\tEntryCache = "<<pEntryCache<<"\n"
		<<"\tCat.Lbl.Cache = "<<pCategoryLabelCache;
      return;
    }
    if (notifyQtApi)
      pModel->beginRemoveRows(parent, childIndex, childIndex);
    //    // CHANGE OF STRATEGY: EMPTY CATEGORIES ARE LEFT INTACT, THEY SHOULD BE REMOVED EXPLICITELY
    //    willRemoveParent = false;
    willRemoveParent = parentid.valid() && getNode(parentid).children.size() <= 1;
    if (n.kind == CategoryLabelKind) {
      // an unlinked category label should have its parent set to invalid.
      pCategoryLabelCache[n.index].parent = NodeId();
      // small optimization feature: mark that we should look for invalid category labels when inserting
      // new category labels.
      pCategoryLabelCacheContainsInvalid = true;
    }
    pCategoryLabelCache[parentid.index].children.removeAt(childIndex);
    if (pCategoryLabelCache[parentid.index].numDisplayFetched > childIndex)
      pCategoryLabelCache[parentid.index].numDisplayFetched--;
    if (notifyQtApi)
      pModel->endRemoveRows();
    n = parentid;
  } while (willRemoveParent);
}




KLFLibModelCache::IndexType KLFLibModelCache::cacheFindCategoryLabel(QStringList catelements,
								     bool createIfNotExists,
								     bool notifyQtApi)
{
  klfDbg(  "cacheFindCategoryLabel(catelmnts="<<catelements<<", createIfNotExists="<<createIfNotExists
	   <<", notifyQtApi="<<notifyQtApi<<")" ) ;

  QString catelpath = catelements.join("/");

  int i;
  for (i = 0; i < pCategoryLabelCache.size(); ++i) {
    if (pCategoryLabelCache[i].parent.valid() &&
	pCategoryLabelCache[i].fullCategoryPath == catelpath) {
      // found the valid category label
      return i;
    }
  }
  if (catelements.isEmpty())
    return 0; // index of root category label

  // if we haven't found the correct category, we may need to create it if requested by
  // caller. If not, return failure immediately
  if ( ! createIfNotExists )
    return -1;

  if (pModel->displayType() != KLFLibModel::CategoryTree) {
    qWarning("cacheFindCategoryLabel: but not in a category tree display type (flavor flags=%#010x)",
	     pModel->pFlavorFlags);
    return 0;
  }

  QStringList catelementsparent = catelements.mid(0, catelements.size()-1);
  IndexType parent_index = cacheFindCategoryLabel(catelementsparent, true, notifyQtApi);
  // now create this last category label
  IndexType this_index;
  if (pCategoryLabelCacheContainsInvalid) {
    // find invalid category label to overwrite (but not root node of course) ...
    for (this_index = 1; this_index < pCategoryLabelCache.size() &&
	   pCategoryLabelCache[this_index].parent.valid(); ++this_index)
      ;
  } else {
    this_index = pCategoryLabelCache.size();
  }
  // ... or append
  if (this_index == pCategoryLabelCache.size()) {
    pCategoryLabelCacheContainsInvalid = false;
    pCategoryLabelCache.append(CategoryLabelNode());
  }
  
  KLFLibModelCache::KLFLibModelSorter srt = 
    KLFLibModelCache::KLFLibModelSorter(this, pModel->pEntrySorter,
					pModel->pFlavorFlags & KLFLibModel::GroupSubCategories);

  // the category label node to add
  CategoryLabelNode c;
  c.fullCategoryPath = catelpath;
  c.categoryLabel = catelements.last(); // catelements is non-empty, see above
  pCategoryLabelCache[this_index] = c;
  int insertPos;
  CategoryLabelNode & parentCatLabelNodeRef = pCategoryLabelCache[parent_index];
  QList<NodeId> & childlistref = parentCatLabelNodeRef.children;
  if (pLastSortPropId < 0) {
    insertPos = childlistref.size(); // no sorting, just append the item
  } else {
    // qLowerBound returns an iterator. subtract begin() to get absolute index
    insertPos = qLowerBound(childlistref.begin(), childlistref.end(),
			    NodeId(CategoryLabelKind, this_index), srt) - childlistref.begin();
  }
  klfDbg( ": About to insert rows in "<<NodeId(CategoryLabelKind, parent_index)
	  <<" at position "<<insertPos ) ;
  if (notifyQtApi) {
    QModelIndex parentidx = createIndexFromId(NodeId(CategoryLabelKind, parent_index), -1, 0);
    if (parentCatLabelNodeRef.numDisplayFetched < insertPos) {
      pModel->beginInsertRows(parentidx, parentCatLabelNodeRef.numDisplayFetched, insertPos+1);
      parentCatLabelNodeRef.numDisplayFetched = insertPos+1;
    } else {
      pModel->beginInsertRows(parentidx, insertPos, insertPos);
      parentCatLabelNodeRef.numDisplayFetched++;
    }
  }
  qDebug("%s: Inserting this_index=%d in parent_index=%d 's children", KLF_FUNC_NAME, this_index,
	 parent_index);

  childlistref.insert(insertPos, NodeId(CategoryLabelKind, this_index));
  pCategoryLabelCache[this_index].parent = NodeId(CategoryLabelKind, parent_index);

  if (notifyQtApi)
    pModel->endInsertRows();
  
  // and return the created category label index
  return this_index;
}

QString KLFLibModelCache::nodeValue(NodeId n, int entryProperty)
{
  // return an internal string representation of the value of the property 'entryProperty' in node ptr.
  // or the category title if node is a category

  if (!n.valid() || n.isRoot())
    return QString();
  if (entryProperty < 0) {
    qWarning()<<KLF_FUNC_NAME<<": invalid entry property ID : "<<entryProperty;
    return QString();
  }
  if (n.kind == EntryKind) {
    if (entryProperty == KLFLibEntry::Preview)
      entryProperty = KLFLibEntry::DateTime; // user friendliness. sort by date when selecting preview.

    EntryNode en = getEntryNodeRef(n);
    // don't update minimalist nodes (assume minimalist info is enough)
    return pModel->entrySorter()->entryValue(en.entry, entryProperty);
  }
  if (n.kind == CategoryLabelKind)
    return getCategoryLabelNodeRef(n).categoryLabel;

  qWarning("nodeValue(): Bad Item Kind: %d", n.kind);
  return QString();
}

// private
void KLFLibModelCache::sortCategory(NodeId category, KLFLibModelSorter *sorter, bool rootCall)
{
  bool requireSimpleReverse = false;

  // some optimizations based on the current sorting
  if (sorter->entrySorter()->propId() == pLastSortPropId) {
    // already sorting according to that column
    if (sorter->entrySorter()->order() == pLastSortOrder) {
      // exact same sorting required, already done.
      return;
    } else {
      // opposite sorting (eg. Ascending instead of Descending)
      // -> reverse child list
      requireSimpleReverse = true;
    }
  }

  int k;
  // sort this category's children

  // This works both in LinearList and in CategoryTree display types. (In LinearList
  // display type, call this function on root category node)

  if (category.kind != CategoryLabelKind)
    return;
  if (category.index < 0 || category.index >= pCategoryLabelCache.size())
    return;

  if (sorter->entrySorter()->propId() < 0)
    return; // no sorting required

  QList<NodeId>& childlistref = pCategoryLabelCache[category.index].children;
  if (requireSimpleReverse) {
    // reverse the child list (but not category list if flavor is to group them)
    int N = childlistref.size();
    int firstEntryInd = 0;
    if (pModel->pFlavorFlags & KLFLibModel::GroupSubCategories) {
      for (firstEntryInd = 0; firstEntryInd < N &&
	     childlistref[firstEntryInd].kind != EntryKind; ++firstEntryInd)
	;
      // in case the GroupSubCategories flag is false, firstEntryInd is zero -> reverse all
    }
    // swap all entries, skipping the grouped sub-categories
    for (k = 0; k < (N-firstEntryInd)/2; ++k)
      qSwap(childlistref[firstEntryInd+k], childlistref[N-k-1]);
  } else {
    qSort(childlistref.begin(), childlistref.end(), *sorter); // normal sort
  }

  // and sort all children's children
  for (k = 0; k < childlistref.size(); ++k)
    if (childlistref[k].kind == CategoryLabelKind)
      sortCategory(childlistref[k], sorter, false /* not root call */);

  if (rootCall) {
    pLastSortPropId = sorter->entrySorter()->propId();
    pLastSortOrder = sorter->entrySorter()->order();
  }
}


KLFLibModelCache::NodeId KLFLibModelCache::nextNode(NodeId n)
{
  //  klfDbg( "KLFLibModelCache::nextNode("<<n<<"): full tree dump:" ) ;
  //  fullDump();
    
  if (!n.valid()) {
    // start with root category (but don't return the root category itself)
    n = NodeId::rootNode();
  }
  // walk children, if any
  Node nn = getNode(n);
  if (nn.children.size() > 0) {
    return nn.children[0];
  }
  // no children, find next sibling.
  NodeId parentid;
  while ( (parentid = nn.parent).valid() ) {
    Node parent = getNode(parentid);
    int row = getNodeRow(n);
    if (row+1 < parent.children.size()) {
      // there is a next sibling: return it
      return parent.children[row+1];
    }
    // no next sibling, so go up the tree
    n = parentid;
    nn = parent;
  }
  // last entry passed
  return NodeId();
}

KLFLibModelCache::NodeId KLFLibModelCache::prevNode(NodeId n)
{
  if (!n.valid() || n.isRoot()) {
    // look for last node in tree.
    return lastNode(NodeId()); // NodeId() is accepted by lastNode.
  }
  // find previous sibling
  NodeId parentId = getNode(n).parent;
  Node parent = getNode(parentId);
  int row = getNodeRow(n);
  if (row > 0) {
    // there is a previous sibling: find its last node
    return lastNode(parent.children[row-1]);
  }
  // there is no previous sibling: so we can return the parent
  // except if parent is the root node, in which case we return an invalid NodeId.
  if (parentId == NodeId::rootNode())
    return NodeId();

  return parentId;
}

KLFLibModelCache::NodeId KLFLibModelCache::lastNode(NodeId n)
{
  if (!n.valid())
    n = NodeId::rootNode(); // root category

  Node nn = getNode(n);
  if (nn.children.size() == 0)
    return n; // no children: n is itself the "last node"

  // children: return last node of last child.
  return lastNode(nn.children[nn.children.size()-1]);
}


QList<KLFLib::entryId> KLFLibModelCache::entryIdList(const QModelIndexList& indexlist)
{
  // ORDER OF RESULT IS NOT GARANTEED != entryIdForIndexList()

  QList<KLFLib::entryId> idList;
  int k;
  QList<NodeId> nodeIds;
  // walk all indexes and get their IDs
  for (k = 0; k < indexlist.size(); ++k) {
    NodeId n = getNodeForIndex(indexlist[k]);
    if ( !n.valid() || n.kind != EntryKind)
      continue;
    if ( nodeIds.contains(n) ) // duplicate, probably for another column
      continue;
    nodeIds << n;
    idList << getEntryNodeRef(n).entryid;
  }
  return idList;
}



QList<KLFLib::entryId> KLFLibModelCache::entryIdForIndexList(const QModelIndexList& indexlist)
{
  // ORDER OF RESULT GARANTEED  != entryIdList()

  QList<KLFLib::entryId> eidlist;
  int k;
  for (k = 0; k < indexlist.size(); ++k) {
    NodeId node = getNodeForIndex(indexlist[k]);
    if ( !node.valid() || node.kind != EntryKind ) {
      eidlist << (KLFLib::entryId) -1;
      continue;
    }
    eidlist << getEntryNodeRef(node).entryid;
  }
  return eidlist;
}
QModelIndexList KLFLibModelCache::findEntryIdList(const QList<KLFLib::entryId>& eidlist)
{
  klfDbg( ": eidlist="<<eidlist ) ;
  int k;
  int count = 0;
  QModelIndexList indexlist;
  // pre-fill index list
  for (k = 0; k < eidlist.size(); ++k)
    indexlist << QModelIndex();

  // walk entry list
  for (k = 0; k < pEntryCache.size(); ++k) {
    int i = eidlist.indexOf(pEntryCache[k].entryid);
    if (i >= 0) {
      indexlist[i] = createIndexFromId(NodeId(EntryKind, k), -1, 0);
      if (++count == eidlist.size())
	break; // found 'em all
    }
  }
  return indexlist;
}

KLFLibModelCache::NodeId KLFLibModelCache::findEntryId(KLFLib::entryId eId)
{
  int k;
  for (k = 0; k < pEntryCache.size(); ++k)
    if (pEntryCache[k].entryid == eId)
      return NodeId(EntryKind, k);

  return NodeId();
}


void KLFLibModelCache::fullDump()
{
#ifdef KLF_DEBUG
  int k;
  qDebug("---------------------------------------------------------");
  qDebug("------------- FULL CACHE DUMP ---------------------------");
  qDebug("---------------------------------------------------------");
  qDebug(" ");
  qDebug("Entry cache dump:");
  for (k = 0; k < pEntryCache.size(); ++k)
    qDebug()<<"#"<<k<<": "<<pEntryCache[k];
  qDebug(" ");
  qDebug("Category Label cache dump: ");
  for (k = 0; k < pCategoryLabelCache.size(); ++k)
    qDebug()<<"#"<<k<<": "<<pCategoryLabelCache[k];
  qDebug(" ");
  dumpNodeTree(NodeId::rootNode());
  qDebug(" ");
  qDebug("---------------------------------------------------------");
  qDebug("---------------------------------------------------------");
  qDebug("---------------------------------------------------------");
#endif
}

void KLFLibModelCache::dumpNodeTree(NodeId node, int indent)
{
#ifdef KLF_DEBUG

  if (indent == 0) {
    qDebug(" ");
    qDebug("---------------- NODE TREE DUMP -------------------------");
  }
  char sindent[] = "                                                                                + ";
  if (indent < (signed)strlen(sindent))
    sindent[indent] = '\0';

  if (!node.valid())
    qDebug() << sindent << "(Invalid Node)";

  EntryNode en;
  CategoryLabelNode cn;
  switch (node.kind) {
  case EntryKind:
    en = getEntryNodeRef(node);
    qDebug() << sindent << node <<"\n"<<sindent<<"\t\t"<<en;
    break;
  case CategoryLabelKind:
    cn = getCategoryLabelNodeRef(node);
    qDebug() << sindent << node << "\n"<<sindent<<"\t\t"<<cn;
    break;
  default:
    qDebug() << sindent << node << "\n"<<sindent<<"\t\t*InvalidNodeKind*(kind="<<node.kind<<")";
  }

  if (node.valid()) {
    Node n = getNode(node);
    int k;
    for (k = 0; k < n.children.size(); ++k) {
      dumpNodeTree(getNode(node).children[k], indent+4);
    }
  }

  if (indent == 0) {
    qDebug("---------------------------------------------------------");
    qDebug("---------------------------------------------------------");
  }

#endif
}




// -------------------------------------------------------

//    MODEL ITSELF



KLFLibModel::KLFLibModel(KLFLibResourceEngine *engine, uint flavorFlags, QObject *parent)
  : QAbstractItemModel(parent), pFlavorFlags(flavorFlags)
{
  // set the default value of N items per batch
  // the initial default value is ridicuously small because fetchMore() is called during startup
  // sequence too many times (in my opinion). the batch count is increased once the widget is
  // shown, see KLFLibDefaultView::showEvent().
  setFetchBatchCount(10);

  // by default, sort according to DateTime, recent first
  pEntrySorter = new KLFLibEntrySorter(KLFLibEntry::DateTime, Qt::DescendingOrder);

  pCache = new KLFLibModelCache(this);

  setResource(engine);
  // DON'T CONNECT SIGNALs FROM RESOURCE ENGINE HERE, we are informed from
  // the view. This is because of the KLFAbstractLibView API.
  //  connect(engine, SIGNAL(dataChanged(...)), this, SLOT(updateData(...)));

}

KLFLibModel::~KLFLibModel()
{
  delete pCache;
  if (pEntrySorter)
    delete pEntrySorter;
}

void KLFLibModel::setResource(KLFLibResourceEngine *resource)
{
  pResource = resource;
  updateCacheSetupModel();
}

QUrl KLFLibModel::url() const
{
  if (pResource == NULL) {
    qWarning()<<KLF_FUNC_NAME<<": resource is NULL!";
    return QUrl();
  }
  if ((pResource->supportedFeatureFlags() & KLFLibResourceEngine::FeatureSubResources)  ==  0)
    return pResource->url();

  // return URL with the default sub-resource (as we're displaying that one only)
  return pResource->url(KLFLibResourceEngine::WantUrlDefaultSubResource);
}



void KLFLibModel::setFlavorFlags(uint flags, uint modify_mask)
{
  if ( (flags & modify_mask) == (pFlavorFlags & modify_mask) )
    return; // no change
  uint other_flags = pFlavorFlags & ~modify_mask;
  pFlavorFlags = flags | other_flags;

  // stuff that needs update
  if (modify_mask & DisplayTypeMask) {
    updateCacheSetupModel();
  }
  if (modify_mask & GroupSubCategories) {
    int col = columnForEntryPropertyId(pEntrySorter->propId());
    Qt::SortOrder ord = pEntrySorter->order();
    // force full re-sort
    sort(-1, ord); // by first setting to unsorted
    sort(col, ord); // and then re-sorting correctly
  }
}
uint KLFLibModel::flavorFlags() const
{
  return pFlavorFlags;
}

void KLFLibModel::prefetch(const QModelIndexList& indexes) const
{
  int k;
  for (k = 0; k < indexes.size(); ++k) {
    if (!indexes[k].isValid())
      continue;
    KLFLibModelCache::NodeId p = pCache->getNodeForIndex(indexes[k]);
    if (!p.valid() || p.isRoot()) {
      klfDbg("Invalid index: indexes[k].row="<<indexes[k].row());
      continue;
    }
    pCache->ensureNotMinimalist(p);
  }
}

QVariant KLFLibModel::data(const QModelIndex& index, int role) const
{
  //  KLF_DEBUG_TIME_BLOCK(KLF_FUNC_NAME) ;
  //  klfDbg( "\tindex="<<index<<"; role="<<role ) ;

  KLFLibModelCache::NodeId p = pCache->getNodeForIndex(index);
  if (!p.valid() || p.isRoot())
    return QVariant();

  // if this node is not yet visible, hide it...
  KLFLibModelCache::Node thisNode = pCache->getNode(p);
  KLFLibModelCache::NodeId parent = thisNode.parent;
  KLFLibModelCache::Node parentNode = pCache->getNode(parent);
  if (index.row() >= parentNode.numDisplayFetched)
    return QVariant();

  if (role == ItemKindItemRole)
    return QVariant::fromValue<int>(p.kind);

  if (ItemKind(p.kind) == EntryKind) {
    // --- GET ENTRY DATA ---
    const KLFLibModelCache::EntryNode& ep = pCache->getEntryNodeRef(p);

    if (role == EntryContentsTypeItemRole)
      return entryColumnContentsPropertyId(index.column());
    if (role == EntryIdItemRole)
      return QVariant::fromValue<KLFLib::entryId>(ep.entryid);

    if (role == Qt::ToolTipRole || role == Qt::DisplayRole) { // current contents
      int propId = entryColumnContentsPropertyId(index.column());
      if (propId == KLFLibEntry::Preview)
	propId = KLFLibEntry::Tags;
      role = entryItemRole(propId);
      // role readjusted, continue below to return correct data
    }

    // now, only custom roles are recognized.
    if (role < Qt::UserRole)
      return QVariant();

    //    klfDbg( "(): role="<<role ) ;

    KLFLibEntry entry = ep.entry;

    if ( ! pCache->minimalistEntryPropIds().contains(entryPropIdForItemRole(role)) &&
	 ep.minimalist) {
      // we are requesting for a property which is not included in minimalist property set
      pCache->ensureNotMinimalist(p);
    }

    if (role == entryItemRole(KLFLibEntry::Latex))
      return entry.latex();
    if (role == entryItemRole(KLFLibEntry::DateTime))
      return entry.dateTime();
    if (role == entryItemRole(KLFLibEntry::Category))
      return entry.category();
    if (role == entryItemRole(KLFLibEntry::Tags))
      //      return klf_debug_tee( entry.tags() );
      return entry.tags();
    if (role == entryItemRole(KLFLibEntry::PreviewSize))
      return entry.previewSize();

    entry = ep.entry; // now the full entry

    if (role == FullEntryItemRole)
      return QVariant::fromValue(entry);

    if (role == entryItemRole(KLFLibEntry::Preview))
      return QVariant::fromValue<QImage>(entry.preview());
    if (role == entryItemRole(KLFLibEntry::Style))
      return QVariant::fromValue(entry.style());
    // by default
    return QVariant();
  }
  else if (ItemKind(p.kind) == CategoryLabelKind) {
    // --- GET CATEGORY LABEL DATA ---
    const KLFLibModelCache::CategoryLabelNode& cp = pCache->getCategoryLabelNodeRef(p);
    if (role == Qt::ToolTipRole)
      return cp.fullCategoryPath;
    if (role == CategoryLabelItemRole)
      return cp.categoryLabel;
    if (role == FullCategoryPathItemRole)
      return cp.fullCategoryPath;
    // by default
    return QVariant();
  }

  // default
  return QVariant();
}
Qt::ItemFlags KLFLibModel::flags(const QModelIndex& index) const
{
  Qt::ItemFlags flagdropenabled = 0;
  if ( index.column() == 0 &&
       (pResource->canModifyData(KLFLibResourceEngine::InsertData) ||
	pResource->canModifyData(KLFLibResourceEngine::ChangeData)) )
    flagdropenabled = Qt::ItemIsDropEnabled;

  if (!index.isValid())
    return flagdropenabled;

  KLFLibModelCache::NodeId p = pCache->getNodeForIndex(index);
  if (!p.valid())
    return 0; // ?!!?
  if (p.kind == KLFLibModelCache::EntryKind)
    return Qt::ItemIsSelectable | Qt::ItemIsEnabled | Qt::ItemIsDragEnabled;
  if (p.kind == KLFLibModelCache::CategoryLabelKind)
    return Qt::ItemIsEnabled | flagdropenabled;

  qWarning()<<KLF_FUNC_NAME<<": bad item kind! index-row="<<index.row()<<"; p="<<p;

  // by default (should never happen)
  return 0;
}
bool KLFLibModel::hasChildren(const QModelIndex &parent) const
{
  if (parent.column() > 0)
    return false;

  KLFLibModelCache::NodeId p = pCache->getNodeForIndex(parent);
  if (!p.valid() || p.isRoot()) {
    // invalid index -> interpret as root node
    p = KLFLibModelCache::NodeId::rootNode();
  }

  return pCache->getNode(p).children.size() > 0;
}
QVariant KLFLibModel::headerData(int section, Qt::Orientation orientation, int role) const
{
  if (role == Qt::FontRole) {
    return qApp->font();
  }
  if (role == Qt::SizeHintRole && orientation == Qt::Horizontal) {
    switch (entryColumnContentsPropertyId(section)) {
    case KLFLibEntry::Preview:
      return QSize(280,30);
    case KLFLibEntry::Latex:
      return QSize(350,30);
    case KLFLibEntry::Category:
    case KLFLibEntry::Tags:
      return QSize(200,30);
    default:
      return QVariant();
    }
  }
  if (role == Qt::DisplayRole) {
    if (orientation == Qt::Horizontal)
      return KLFLibEntry().propertyNameForId(entryColumnContentsPropertyId(section));
    return QString("-");
  }
  return QVariant();
}

bool KLFLibModel::hasIndex(int row, int column, const QModelIndex &parent) const
{
  // better implementation in the future
  return index(row, column, parent).isValid();
}

QModelIndex KLFLibModel::index(int row, int column, const QModelIndex &parent) const
{
  const KLFLibModelCache::CategoryLabelNode& cat_p =
    pCache->getCategoryLabelNodeRef(KLFLibModelCache::NodeId::rootNode()); // root node
  KLFLibModelCache::CategoryLabelNode p = cat_p;

  if (parent.isValid()) {
    KLFLibModelCache::NodeId pp = KLFLibModelCache::NodeId::fromUID(parent.internalId());
    if (pp.kind != KLFLibModelCache::CategoryLabelKind)
      return QModelIndex();
    if (pp.valid())
      p = pCache->getCategoryLabelNodeRef(pp);
  }
  if (row < 0 || row >= p.children.size() || column < 0 || column >= columnCount(parent))
    return QModelIndex();
  klfDbgT(": row="<<row<<"; column="<<column<<"; parent="<<parent);
  return pCache->createIndexFromId(p.children[row], row, column);
}
QModelIndex KLFLibModel::parent(const QModelIndex &index) const
{
  KLFLibModelCache::NodeId p = pCache->getNodeForIndex(index);
  if ( !p.valid() ) // invalid index
    return QModelIndex();
  KLFLibModelCache::Node n = pCache->getNode(p);
  if ( ! n.parent.valid() ) // invalid parent (should never happen!)
    return QModelIndex();
  klfDbgT(": requesting parent of index "<<index);
  return pCache->createIndexFromId(n.parent, -1 /* figure out row */, 0);
}
int KLFLibModel::rowCount(const QModelIndex &parent) const
{
  if (parent.column() > 0)
    return 0;

  KLFLibModelCache::NodeId p = pCache->getNodeForIndex(parent);
  if (!p.valid())
    p = KLFLibModelCache::NodeId::rootNode();

  KLFLibModelCache::Node n = pCache->getNode(p);
  klfDbg( " parent="<<parent<<"; count="<<n.numDisplayFetched<<"; numchildren="
	  <<n.children.size() ) ;

  // make sure also that numDisplayFetched is not more than children.size()
  return qMin(n.children.size(), n.numDisplayFetched);
}

int KLFLibModel::columnCount(const QModelIndex & /*parent*/) const
{
  return 5;
}
int KLFLibModel::entryColumnContentsPropertyId(int column) const
{
  // WARNING: ANY CHANGES MADE HERE MUST BE REPEATED IN THE NEXT FUNCTION
  switch (column) {
  case 0:
    return KLFLibEntry::Preview;
  case 1:
    return KLFLibEntry::Tags;
  case 2:
    return KLFLibEntry::Category;
  case 3:
    return KLFLibEntry::Latex;
  case 4:
    return KLFLibEntry::DateTime;
  default:
    return -1;
  }
}
int KLFLibModel::columnForEntryPropertyId(int entryPropertyId) const
{
  // WARNING: ANY CHANGES MADE HERE MUST BE REPEATED IN THE PREVIOUS FUNCTION
  switch (entryPropertyId) {
  case KLFLibEntry::Preview:
    return 0;
  case KLFLibEntry::Tags:
    return 1;
  case KLFLibEntry::Category:
    return 2;
  case KLFLibEntry::Latex:
    return 3;
  case KLFLibEntry::DateTime:
    return 4;
  default:
    return -1;
  }
}


bool KLFLibModel::canFetchMore(const QModelIndex& parent) const
{
  //  KLF_DEBUG_BLOCK(KLF_FUNC_NAME); klfDbg( "\t parent="<<parent ) ;

  KLFLibModelCache::NodeId n = pCache->getNodeForIndex(parent);
  if (!n.valid())
    n = KLFLibModelCache::NodeId::rootNode();

  return pCache->canFetchMore(pCache->getNodeForIndex(parent));
}
void KLFLibModel::fetchMore(const QModelIndex& parent)
{
  pCache->fetchMore(pCache->getNodeForIndex(parent), pFetchBatchCount);
}


Qt::DropActions KLFLibModel::supportedDropActions() const
{
  return Qt::CopyAction|Qt::MoveAction;
}

QStringList KLFLibModel::mimeTypes() const
{
  return QStringList() << "application/x-klf-internal-lib-move-entries"
		       << KLFAbstractLibEntryMimeEncoder::allEncodingMimeTypes();
}
QMimeData *KLFLibModel::mimeData(const QModelIndexList& indlist) const
{
  // in the future, this will serve when dragging a category label, to redifine the index
  // list as containing all the child entries.
  QModelIndexList indexes = indlist;

  // get all entries
  KLFLibEntryList entries;
  QList<KLFLib::entryId> entryids;
  QList<KLFLibModelCache::NodeId> entriesnodeids;
  int k;
  for (k = 0; k < indexes.size(); ++k) {
    KLFLibModelCache::NodeId n = pCache->getNodeForIndex(indexes[k]);
    if (!n.valid() || n.isRoot())
      continue;
    if (n.kind != KLFLibModelCache::EntryKind)
      continue;
    if (entriesnodeids.contains(n))
      continue; // skip duplicates (for ex. for other model column indexes)
    const KLFLibModelCache::EntryNode& en = pCache->getEntryNodeRef(n);
    entries << pResource->entry(en.entryid);
    entryids << en.entryid;
    entriesnodeids << n;
  }

  //  klfDbg( "KLFLibModel::mimeData: Encoding entries "<<entries ) ;

  // some meta-data properties
  QVariantMap vprop;
  QUrl myurl = url();
  vprop["Url"] = myurl; // originating URL

  QMimeData *mimedata = KLFAbstractLibEntryMimeEncoder::createMimeData(entries, vprop);

  QByteArray internalmovedata;

  // application/x-klf-internal-lib-move-entries
  { QDataStream imstr(&internalmovedata, QIODevice::WriteOnly);
    imstr.setVersion(QDataStream::Qt_4_4);
    imstr << vprop << entryids;
  }
  mimedata->setData("application/x-klf-internal-lib-move-entries", internalmovedata);

  return mimedata;
}

// private
bool KLFLibModel::dropCanInternal(const QMimeData *mimedata)
{
  if ( ! mimedata->hasFormat("application/x-klf-internal-lib-move-entries") ||
       ! pResource->canModifyData(KLFLibResourceEngine::ChangeData))
    return false;

  QByteArray imdata = mimedata->data("application/x-klf-internal-lib-move-entries");
  QDataStream imstr(imdata);
  imstr.setVersion(QDataStream::Qt_4_4);
  QVariantMap vprop;
  imstr >> vprop;
  QUrl theirurl = vprop.value("Url").toUrl();
  QUrl oururl = url();
  bool ok = (oururl.toEncoded() == theirurl.toEncoded());
  klfDbg( "drag originated from "<<theirurl<<"; we are "<<oururl<<"; OK="<<ok ) ;
  return ok;
}

bool KLFLibModel::dropMimeData(const QMimeData *mimedata, Qt::DropAction action, int row,
			       int column, const QModelIndex& parent)
{
  klfDbg(  "Drop data: action="<<action<<" row="<<row<<" col="<<column
	   << " parent="<<parent ) ;

  if (action == Qt::IgnoreAction)
    return true;
  if (action != Qt::CopyAction)
    return false;

  if ( ! (mimedata->hasFormat("application/x-klf-internal-lib-move-entries") &&
	  pResource->canModifyData(KLFLibResourceEngine::ChangeData)) &&
       ! (KLFAbstractLibEntryMimeEncoder::canDecodeMimeData(mimedata) &&
	  pResource->canModifyData(KLFLibResourceEngine::InsertData)) ) {
    // cannot (change items with application/x-klf-internal-lib-move-entries) AND
    // cannot (insert items with any supported format, eg. application/x-klf-libentries)
    return false;
  }

  if (column > 0)
    return false;

  // imdata, imstr : "*i*nternal *m*ove" ; ldata, lstr : "entry *l*ist"
  bool useinternalmove = dropCanInternal(mimedata);
  if (useinternalmove) {
    klfDbg(  "Dropping application/x-klf-internal-lib-move-entries" ) ;
    if ( !(pFlavorFlags & CategoryTree) )
      return false;

    QByteArray imdata = mimedata->data("application/x-klf-internal-lib-move-entries");
    QDataStream imstr(imdata);
    imstr.setVersion(QDataStream::Qt_4_4);
    QList<KLFLib::entryId> idlist;
    QVariantMap vprop;
    imstr >> vprop >> idlist;
    // get the category we moved to
    KLFLibModelCache::NodeId pn = pCache->getNodeForIndex(parent);
    if (!pn.valid()) {
      pn = KLFLibModelCache::NodeId::rootNode();
    }
    if (ItemKind(pn.kind) != CategoryLabelKind) {
      qWarning()<<"Dropped in a non-category index! (kind="<<pn.kind<<")";
      return false;
    }
    const KLFLibModelCache::CategoryLabelNode& cn = pCache->getCategoryLabelNodeRef(pn);
    // move, not copy: change the selected entries to the new category.
    QString newcategory = cn.fullCategoryPath;
    if (newcategory.endsWith("/"))
      newcategory.chop(1);

    bool r = pResource->changeEntries(idlist, QList<int>() << KLFLibEntry::Category,
				      QList<QVariant>() << QVariant(newcategory));
    klfDbg( "Accepted drop of type application/x-klf-internal-lib-move-entries. Res="<<r<<"\n"
	    <<"ID list is "<<idlist<<" new category is "<<newcategory ) ;
    if (!r) {
      return false;
    }
    // dataChanged() is emitted in changeEntries()
    return true;
  }

  klfDbg( "Dropping entry list." ) ;

  KLFLibEntryList elist;
  QVariantMap vprop;
  bool res = KLFAbstractLibEntryMimeEncoder::decodeMimeData(mimedata, &elist, &vprop);
  if ( ! res ) {
    qWarning()<<KLF_FUNC_NAME<<": Drop: Can't decode mime data! provided types="
	      <<mimedata->formats();
    QMessageBox::warning(NULL, tr("Drop Error", "[[message box title]]"),
			 tr("Error dropping data."));
    return false;
  }

  if ( elist.isEmpty() )
    return true; // nothing to drop

  // insert list, regardless of parent (no category change)
  res = insertEntries(elist);
  klfDbg( "Dropped entry list "<<elist<<". Originating URL="
	  <<(vprop.contains("Url")?vprop["Url"]:"(none)")<<". result="<<res ) ;
  if (!res)
    return false;

  // dataChanged()/rowsInserted()/... are emitted in insertEntries()
  return true;
}

uint KLFLibModel::dropFlags(QDragMoveEvent *event, QAbstractItemView *view)
{
  const QMimeData *mdata = event->mimeData();
  if (dropCanInternal(mdata)) {
    if ( !(pFlavorFlags & CategoryTree) ) {
      return 0; // will NOT accept an internal move not in a category tree.
      //           (note: icon moves not handled in KLFLibModel; intercepted in view.)
    }
    // we're doing an internal drop in category tree:
    QModelIndex dropOnIndex = view->indexAt(event->pos());
    if (pResource->canModifyData(KLFLibResourceEngine::ChangeData) &&
	(!dropOnIndex.isValid() || dropOnIndex.column() == 0) )
      return DropWillAccept|DropWillMove|DropWillCategorize;
    /** \todo ....... drop not accepted on background (categorize to root) */
    return 0;
  }
  if (KLFAbstractLibEntryMimeEncoder::canDecodeMimeData(mdata) &&
      pResource->canModifyData(KLFLibResourceEngine::InsertData))
    return DropWillAccept;
  return 0;
}

QImage KLFLibModel::dragImage(const QModelIndexList& indexes)
{
  if (indexes.isEmpty())
    return QImage();

  const int MAX = 5;
  const QSize s1 = 0.8*klfconfig.UI.labelOutputFixedSize;
  const QPointF delta(18.0, 20.0);
  QList<QImage> previewlist;
  QList<KLFLibModelCache::NodeId> alreadydone;
  int k, j;
  int iS = indexes.size();
  int n = qMin(1+MAX, iS);
  for (j = 0, k = 0; k < iS && j < n; ++k) {
    KLFLibModelCache::NodeId n = pCache->getNodeForIndex(indexes[iS-k-1]); // reverse order
    if (!n.valid() || n.kind != KLFLibModelCache::EntryKind)
      continue;
    if (alreadydone.contains(n))
      continue;
    const KLFLibModelCache::EntryNode& en = pCache->getEntryNodeRef(n, true);
    alreadydone << n;
    previewlist << en.entry.preview().scaled(s1, Qt::KeepAspectRatio, Qt::SmoothTransformation);
    ++j;
  }
  if (previewlist.isEmpty())
    return QImage();

  int N = qMin(MAX, previewlist.size());
  QSize s2 = (s1 + (N-1)*pointToSizeF(delta)).toSize();
  QImage image(s2, QImage::Format_ARGB32_Premultiplied);
  image.fill(qRgba(0,0,0,0));
  {
    QPainter p(&image);
    QPointF P(0,0);
    QPointF lastimgbr;
    for (k = 0; k < N; ++k) {
      // and add this image
      QImage i = transparentify_image(previewlist[k], 0.7);
      p.drawImage(P, i);
      // p.drawRect(QRectF(P, s1));
      lastimgbr = P+sizeToPointF(i.size());
      P += delta;
    }
    if (k == MAX) {
      p.setCompositionMode(QPainter::CompositionMode_DestinationOver);
      p.setPen(QPen(QColor(0,0,0),2.0));
      QPointF br = lastimgbr;
      p.drawPoint(br - QPointF(5,5));
      p.drawPoint(br - QPointF(10,5));
      p.drawPoint(br - QPointF(15,5));
    }
  }

  return autocrop_image(image);
}


bool KLFLibModel::isDesendantOf(const QModelIndex& child, const QModelIndex& ancestor)
{
  if (!child.isValid())
    return false;

  return child.parent() == ancestor || isDesendantOf(child.parent(), ancestor);
}

QStringList KLFLibModel::categoryList() const
{
  return pCache->categoryListCache();
}

void KLFLibModel::updateData(const QList<KLFLib::entryId>& entryIdList, int modifyType)
{
  pCache->updateData(entryIdList, modifyType);
}

QModelIndex KLFLibModel::walkNextIndex(const QModelIndex& cur)
{
  KLFLibModelCache::NodeId nextnode = pCache->nextNode(pCache->getNodeForIndex(cur));
  //  klfDbg( "KLFLibModel::walkNextIndex: got next node=NodeId("<<nextnode.kind<<","<<nextnode.index<<")" ) ;
  return pCache->createIndexFromId(nextnode, -1, 0);
}
QModelIndex KLFLibModel::walkPrevIndex(const QModelIndex& cur)
{
  KLFLibModelCache::NodeId prevnode = pCache->prevNode(pCache->getNodeForIndex(cur));
  return pCache->createIndexFromId(prevnode, -1, 0);
}

KLFLib::entryId KLFLibModel::entryIdForIndex(const QModelIndex& index) const
{
  return entryIdForIndexList(QModelIndexList() << index) [0];
}

QModelIndex KLFLibModel::findEntryId(KLFLib::entryId eid) const
{
  return findEntryIdList(QList<KLFLib::entryId>() << eid) [0];
}

QList<KLFLib::entryId> KLFLibModel::entryIdForIndexList(const QModelIndexList& indexlist) const
{
  return pCache->entryIdForIndexList(indexlist);
}
QModelIndexList KLFLibModel::findEntryIdList(const QList<KLFLib::entryId>& eidlist) const
{
  return pCache->findEntryIdList(eidlist);
}


void KLFLibModel::setEntrySorter(KLFLibEntrySorter *entrySorter)
{
  if (pEntrySorter == entrySorter)
    return;
  if (entrySorter == NULL) {
    qWarning()<<KLF_FUNC_NAME<<": NULL entrySorter given!";
  }

  if (pEntrySorter)
    delete pEntrySorter;
  pEntrySorter = entrySorter;
}


QModelIndex KLFLibModel::searchFind(const QString& queryString, const QModelIndex& fromIndex,
				    bool forward)
{
  klfDbg( " s="<<queryString<<" from "<<fromIndex<<" forward="<<forward ) ;
  pSearchAborted = false;
  pSearchString = queryString;
  pSearchCurNode = fromIndex;
  return searchFindNext(forward);
}
QModelIndex KLFLibModel::searchFindNext(bool forward)
{
  KLF_DEBUG_TIME_BLOCK(KLF_FUNC_NAME) ;
  pSearchAborted = false;
  if (pSearchString.isEmpty())
    return QModelIndex();

  QTime t;

  KLFLibModelCache::NodeId curNode = pCache->getNodeForIndex(pSearchCurNode);

  // search nodes
  KLFLibModelCache::NodeId (KLFLibModelCache::*stepfunc)(KLFLibModelCache::NodeId) =
    forward
    ? &KLFLibModelCache::nextNode
    : &KLFLibModelCache::prevNode;

  // presence of capital letter switches case sensitivity on (like (X)Emacs)
  Qt::CaseSensitivity cs = Qt::CaseInsensitive;
  if (pSearchString.contains(QRegExp("[A-Z]")))
    cs = Qt::CaseSensitive;

  bool found = false;
  while ( ! found &&
	  (curNode = (pCache->*stepfunc)(curNode)).valid() ) {
    if ( pCache->nodeValue(curNode, KLFLibEntry::Latex).contains(pSearchString, cs) ||
    	 pCache->nodeValue(curNode, KLFLibEntry::Tags).contains(pSearchString, cs) ) {
      found = true;
    }
    if (t.elapsed() > 150) {
      pSearchCurNode = pCache->createIndexFromId(curNode, -1, 0);
      qApp->processEvents();
      if (pSearchAborted)
	break;
      t.restart();
    }
  }
  pSearchCurNode = pCache->createIndexFromId(curNode, -1, 0);
  if (found) {
    klfDbg( "found "<<pSearchString<<" at "<<pSearchCurNode ) ;
    return pSearchCurNode;
  }
  return QModelIndex();
}

void KLFLibModel::searchAbort()
{
  pSearchAborted = true;
}

bool KLFLibModel::changeEntries(const QModelIndexList& indexlist, int propId, const QVariant& value)
{
  if ( indexlist.size() == 0 )
    return true; // no change...

  QList<KLFLib::entryId> idList = pCache->entryIdList(indexlist);
  klfDbg( ": Changing entries "<<idList<<"; indexlist="<<indexlist ) ;
  //  klfDbg( "full dump:" ) ;
  //  pCache->fullDump();
  bool r = pResource->changeEntries(idList, QList<int>() << propId, QList<QVariant>() << value);
  //  klfDbg( ": entries changed, full dump:" ) ;
  //  pCache->fullDump();

  // the resource will emit dataChanged() to the view's updateResourceData() for the model update

  return r;
}

bool KLFLibModel::insertEntries(const KLFLibEntryList& entries)
{
  if (entries.size() == 0)
    return true; // nothing to do...

  KLFLibEntryList elist = entries;
  /*  int k;
  for (k = 0; k < entries.size(); ++k) {
    // strip any position property from the entry
    if (elist[k].property("IconView_IconPosition").isValid())
      elist[k].setEntryProperty("IconView_IconPosition", QVariant());
  }*/

  QList<KLFLib::entryId> list = pResource->insertEntries(elist);

  // will be automatically called via the call to resoruce->(modify-data)()!
  //  updateCacheSetupModel();

  if ( list.size() == 0 || list.contains(-1) )
    return false; // error for at least one entry
  return true;
}

bool KLFLibModel::deleteEntries(const QModelIndexList& indexlist)
{
  if ( indexlist.size() == 0 )
    return true; // no change...

  QList<KLFLib::entryId> idList = pCache->entryIdList(indexlist);
  if ( pResource->deleteEntries(idList) ) {
    // will be automatically called via the call to resoruce->(modify-data)()!
    //    updateCacheSetupModel();
    return true;
  }
  return false;
}

void KLFLibModel::completeRefresh()
{
  updateCacheSetupModel();
}


void KLFLibModel::redoSort()
{
  startLayoutChange();

  KLFLibModelCache::KLFLibModelSorter srt = 
    KLFLibModelCache::KLFLibModelSorter(pCache, pEntrySorter, pFlavorFlags & KLFLibModel::GroupSubCategories);

  pCache->sortCategory(KLFLibModelCache::NodeId::rootNode(), &srt);

  endLayoutChange();
}

void KLFLibModel::sort(int column, Qt::SortOrder order)
{
  pEntrySorter->setPropId(entryColumnContentsPropertyId(column));
  pEntrySorter->setOrder(order);

  redoSort();
}



QList<KLFLibModel::PersistentId> KLFLibModel::persistentIdList(const QModelIndexList& persistentIndexes)
{
  KLF_DEBUG_TIME_BLOCK(KLF_FUNC_NAME) ;
  // save persistent indexes
  QList<PersistentId> persistentIndexIds;
  int k;
  for (k = 0; k < persistentIndexes.size(); ++k) {
    PersistentId id;
    KLFLibModelCache::NodeId n = pCache->getNodeForIndex(persistentIndexes[k]);
    if (!n.valid()) {
      id.kind = (ItemKind)-1;
    } else {
      id.kind = n.kind;
      if (ItemKind(id.kind) == EntryKind)
	id.entry_id = pCache->getEntryNodeRef(n).entryid;
      else if (ItemKind(id.kind) == CategoryLabelKind)
	id.categorylabel_fullpath = pCache->getCategoryLabelNodeRef(n).fullCategoryPath;
      else
	qWarning("KLFLibModel::persistentIdList: Bad Node kind: %d!!", id.kind);
    }
    id.column = persistentIndexes[k].column();
    persistentIndexIds << id;
  }
  return persistentIndexIds;
}
QModelIndexList KLFLibModel::newPersistentIndexList(const QList<PersistentId>& persistentIndexIds)
{
  KLF_DEBUG_TIME_BLOCK(KLF_FUNC_NAME) ;
  QModelIndexList newPersistentIndexes;
  int k;
  for (k = 0; k < persistentIndexIds.size(); ++k) {
    // find node in new layout
    PersistentId id = persistentIndexIds[k];
    QModelIndex index;
    if (ItemKind(id.kind) == EntryKind) {
      QModelIndexList ilist = pCache->findEntryIdList(QList<KLFLib::entryId>() << id.entry_id);
      index = ilist[0];
    } else if (id.kind == CategoryLabelKind) {
      int idx = pCache->cacheFindCategoryLabel(id.categorylabel_fullpath.split('/'));
      KLFLibModelCache::NodeId nodeId(KLFLibModelCache::CategoryLabelKind, idx);
      index = pCache->createIndexFromId(nodeId, -1, 0);
    } else {
      qWarning("newPersistentIndexList: bad persistent id node kind! :%d", id.kind);
    }

    newPersistentIndexes << index;
  }
  return newPersistentIndexes;
}


void KLFLibModel::startLayoutChange()
{
  // first notify anyone who may be interested
  emit layoutAboutToBeChanged();
  // save persistent indexes
  pLytChgIndexes = persistentIndexList();
  pLytChgIds = persistentIdList(pLytChgIndexes);
}
void KLFLibModel::endLayoutChange()
{
  QModelIndexList newpindexes = newPersistentIndexList(pLytChgIds);
  changePersistentIndexList(pLytChgIndexes, newpindexes);
  // and notify of layout change
  emit layoutChanged();
}


void KLFLibModel::updateCacheSetupModel()
{
  pCache->rebuildCache();
}


// --------------------------------------------------------



KLFLibViewDelegate::KLFLibViewDelegate(QObject *parent)
  : QAbstractItemDelegate(parent), pSelModel(NULL), pTheTreeView(NULL)
{
  pAutoBackgroundItems = true;
}
KLFLibViewDelegate::~KLFLibViewDelegate()
{
}

QWidget * KLFLibViewDelegate::createEditor(QWidget */*parent*/,
					   const QStyleOptionViewItem& /*option*/,
					   const QModelIndex& /*index*/) const
{
  return 0;
}
bool KLFLibViewDelegate::editorEvent(QEvent */*event*/, QAbstractItemModel */*model*/,
				     const QStyleOptionViewItem& /*option*/,
				     const QModelIndex& /*index*/)
{
  return false;
}
/** \internal */
class _klf_block_progress_blocker
{
  KLFLibResourceEngine *res;
public:
  _klf_block_progress_blocker(KLFLibResourceEngine *r) : res(r)
  {
    res->blockProgressReporting(true);
  }
  ~_klf_block_progress_blocker()
  {
    res->blockProgressReporting(false);
  }
};
void KLFLibViewDelegate::paint(QPainter *painter, const QStyleOptionViewItem& option,
			       const QModelIndex& index) const
{
  KLF_DEBUG_TIME_BLOCK(KLF_FUNC_NAME) ;
  klfDbg( "\tindex="<<index<<"; rect="<<option.rect ) ;
  PaintPrivate pp;
  pp.p = painter;
  pp.option = &option;
  pp.innerRectImage = QRect(option.rect.topLeft()+QPoint(2,2), option.rect.size()-QSize(4,4));
  pp.innerRectText = QRect(option.rect.topLeft()+QPoint(4,2), option.rect.size()-QSize(8,4));

#ifdef Q_WS_MAC
  // block progress reporting on MAC to avoid repaint recursions
  _klf_block_progress_blocker(qobject_cast<KLFLibModel*>(const_cast<QAbstractItemModel*>(index.model()))->resource());
#endif

  painter->save();

  QPen pen = painter->pen();
  pp.isselected = (option.state & QStyle::State_Selected);
  QPalette::ColorGroup cg = option.state & QStyle::State_Enabled
    ? QPalette::Normal : QPalette::Disabled;
  if (cg == QPalette::Normal && !(option.state & QStyle::State_Active))
    cg = QPalette::Inactive;
  if (pp.isselected) {
    pp.background = option.palette.brush(cg, QPalette::Highlight);
    painter->setPen(option.palette.color(cg, QPalette::HighlightedText));
    painter->fillRect(option.rect, pp.background);
  } else {
    pp.background = pAutoBackgroundColor.isValid()
      ? pAutoBackgroundColor
      : option.palette.brush(cg, QPalette::Base);
    painter->setPen(option.palette.color(cg, QPalette::Text));
  }

  int kind = index.data(KLFLibModel::ItemKindItemRole).toInt();
  if (kind == KLFLibModel::EntryKind)
    paintEntry(&pp, index);
  else if (kind == KLFLibModel::CategoryLabelKind)
    paintCategoryLabel(&pp, index);

  if (option.state & QStyle::State_HasFocus) {
    QStyleOptionFocusRect o;
    o.QStyleOption::operator=(option);
    o.rect = option.rect;
    o.state |= QStyle::State_KeyboardFocusChange;
    o.state |= QStyle::State_Item;
    QPalette::ColorGroup cg = (option.state & QStyle::State_Enabled)
                              ? QPalette::Normal : QPalette::Disabled;
    o.backgroundColor = option.palette.color(cg, (option.state & QStyle::State_Selected)
                                             ? QPalette::Highlight : QPalette::Window);
    const QWidget *w = 0;
    if (const QStyleOptionViewItemV3 *v3 = qstyleoption_cast<const QStyleOptionViewItemV3 *>(&option))
      w = v3->widget;
    QStyle *style = w ? w->style() : QApplication::style();
    style->drawPrimitive(QStyle::PE_FrameFocusRect, &o, painter, w);
  }

  painter->restore();
}

void KLFLibViewDelegate::paintEntry(PaintPrivate *p, const QModelIndex& index) const
{
  KLF_DEBUG_TIME_BLOCK(KLF_FUNC_NAME) ;
  uint fl = PTF_HighlightSearch;
  if (index.parent() == pSearchIndex.parent() && index.row() == pSearchIndex.row())
    fl |= PTF_HighlightSearchCurrent;

  switch (index.data(KLFLibModel::EntryContentsTypeItemRole).toInt()) {
  case KLFLibEntry::Latex:
    // paint Latex String
    paintText(p, index.data(KLFLibModel::entryItemRole(KLFLibEntry::Latex)).toString(), fl);
    break;
  case KLFLibEntry::Preview:
    // paint Latex Equation
    {
      QImage img = index.data(KLFLibModel::entryItemRole(KLFLibEntry::Preview)).value<QImage>();
      QImage img2 = img.scaled(p->innerRectImage.size(), Qt::KeepAspectRatio, Qt::SmoothTransformation);
      if (p->isselected)
	img2 = transparentify_image(img2, 0.85);
      QPoint pos = p->innerRectImage.topLeft()
	+ QPoint(0, (p->innerRectImage.height()-img2.height()) / 2);
      if (pAutoBackgroundItems) {
	// draw image on different background if it can't be "distinguished" from default background
	// (eg. a transparent white formula)
	klfDbg( " BG Brush is "<<p->background ) ;
	QColor bgcolor = p->background.color();
	QList<QColor> bglista, bglistb, bglist;
	// 	bglist << bgcolor
	// 	       << QColor(255,255,255)
	// 	       << QColor(220,220,220)
	// 	       << QColor(180,190,190)
	// 	       << QColor(200,200,200)
	// 	       << QColor(150,150,150)
	// 	       << QColor(100,100,100)
	// 	       << QColor(50,50,50)
	// 	       << QColor(0,0,0);
	bglist << bgcolor; // first try: default color (!)
	bglista = bglistb = bglist;
	int count;
	for (count = 0; count < 5; ++count) // suggest N (ever) darker colors
	  bglista << bglista.last().darker(105+count*2);
	for (count = 0; count < 5; ++count) // and N (ever) lighter colors
	  bglistb << bglistb.last().lighter(105+count*2);
	// build the full list, and always provide white, and black to be sure
	bglist << bglista.mid(1) << bglistb.mid(1) << QColor(255,255,255) << QColor(0,0,0);
	klfDbg( "alt. bg list is "<<bglist );
	int k;
	for (k = 0; k < bglist.size(); ++k) {
	  //	  klfDbg( "calling image_is_distinguishable on entry with latex="
	  //		  <<index.data(KLFLibModel::entryItemRole(KLFLibEntry::Latex)).toString() );
	  bool distinguishable = image_is_distinguishable(img2, bglist[k], 20); // 30
	  if ( distinguishable )
	    break; // got distinguishable color
	}
	// if the background color is not the default one, fill the background with that color
	if (k > 0 && k < bglist.size())
	  p->p->fillRect(QRect(pos, img2.size()), QBrush(bglist[k]));
      }
      // and draw the equation
      p->p->drawImage(pos, img2);
      break;
    }
  case KLFLibEntry::Category:
    // paint Category String
    paintText(p, index.data(KLFLibModel::entryItemRole(KLFLibEntry::Category)).toString(), fl);
    break;
  case KLFLibEntry::Tags:
    // paint Tags String
    paintText(p, index.data(KLFLibModel::entryItemRole(KLFLibEntry::Tags)).toString(), fl);
    break;
  case KLFLibEntry::DateTime:
    // paint DateTime String
    { QLocale loc = klfconfig.UI.locale;
      paintText(p, loc.toString(index.data(KLFLibModel::entryItemRole(KLFLibEntry::DateTime))
				.toDateTime(), QLocale::LongFormat), fl);
      break;
    }
  default:
    qDebug("KLFLibViewDelegate::paintEntry(): Got bad contents type %d !",
	   index.data(KLFLibModel::EntryContentsTypeItemRole).toInt());
    // nothing to show !
    //    painter->fillRect(option.rect, QColor(128,0,0));
  }
}

void KLFLibViewDelegate::paintCategoryLabel(PaintPrivate *p, const QModelIndex& index) const
{
  KLF_DEBUG_TIME_BLOCK(KLF_FUNC_NAME) ;
  //  klfDbg( "index="<<index<<"; pTheTreeView="<<pTheTreeView<<"; treeview->isexpanded="
  //	  <<(pTheTreeView?(pTheTreeView->isExpanded(index)?"true":"false"):"(N/A)") );
  //  klfDbg( "index has selected desc. ?="<<indexHasSelectedDescendant(index) );

  if (index.column() > 0)  // paint only on first column
    return;

  //  klfDbg( "index="<<index<<"; pSearchIndex="<<pSearchIndex ) ;

  uint fl = PTF_HighlightSearch;
  if (index.parent() == pSearchIndex.parent() && index.row() == pSearchIndex.row())
    fl |= PTF_HighlightSearchCurrent;
  if ( pTheTreeView != NULL && !pTheTreeView->isExpanded(index) &&
       indexHasSelectedDescendant(index) ) { // not expanded, and has selected child
    fl |= PTF_SelUnderline;
  }
  //   if ( sizeHint(*p->option, index).width() > p->option->rect.width() ) {
  //     // force rich text edit to handle too-wide texts
  //     fl |= PTF_ForceRichTextRender;
  //   }

  if (fl & PTF_HighlightSearchCurrent) {
    // paint line as if it were selected.
    QPen pen = p->p->pen();
    QPalette::ColorGroup cg = p->option->state & QStyle::State_Enabled
      ? QPalette::Normal : QPalette::Disabled;
    if (cg == QPalette::Normal && !(p->option->state & QStyle::State_Active))
      cg = QPalette::Inactive;
    p->p->fillRect(p->option->rect, p->option->palette.brush(cg, QPalette::Highlight));
    p->p->setPen(p->option->palette.color(cg, QPalette::HighlightedText));
  }

  // paint Category Label
  paintText(p, index.data(KLFLibModel::CategoryLabelItemRole).toString(), fl);
}

QDebug& operator<<(QDebug& d, const KLFLibViewDelegate::ColorRegion& c)
{
  return d << "ColorRegion["<<c.start<<"->+"<<c.len<<"]";
}

void KLFLibViewDelegate::paintText(PaintPrivate *p, const QString& text, uint flags) const
{
  KLF_DEBUG_TIME_BLOCK(KLF_FUNC_NAME) ;

  QColor textcol = p->option->palette.color(QPalette::Text);
  QColor textcoltransp = textcol; textcoltransp.setAlpha(0);
  QGradientStops borderfadelingrstops;
  borderfadelingrstops << QGradientStop(0.0f,  textcoltransp)
		       << QGradientStop(0.1f, textcol)
		       << QGradientStop(0.9f, textcol)
		       << QGradientStop(1.0f,  textcoltransp);
  QLinearGradient borderfadelingr(p->innerRectText.topLeft(), p->innerRectText.bottomLeft());
  borderfadelingr.setStops(borderfadelingrstops);
  borderfadelingr.setCoordinateMode(QGradient::LogicalMode);
  QPen borderfadepen = QPen(QBrush(borderfadelingr), 1.0f);

  int drawnTextWidth;
  int drawnBaseLineY;
  if ( (pSearchString.isEmpty() || !(flags&PTF_HighlightSearch) ||
	text.indexOf(pSearchString, 0, Qt::CaseInsensitive) == -1) &&
       !(flags & PTF_SelUnderline) &&
       !(flags & PTF_ForceRichTextRender) ) {
    // no formatting required, use fast method.
    QSize s = QFontMetrics(p->option->font).size(0, text);
    drawnTextWidth = s.width();
    drawnBaseLineY = (int)(p->innerRectText.bottom() - 0.5f*(p->innerRectText.height()-s.height()));
    p->p->setPen(borderfadepen);
    p->p->drawText(p->innerRectText, Qt::AlignLeft|Qt::AlignVCenter, text);
  } else {
    // formatting required.
    // build a list of regions to highlight
    QList<ColorRegion> c;
    QTextCharFormat f_highlight;
    if (flags & PTF_HighlightSearchCurrent)
      f_highlight.setBackground(QColor(0,255,0,80));
    f_highlight.setForeground(QColor(128,0,0));
    f_highlight.setFontItalic(true);
    f_highlight.setFontWeight(QFont::Bold);
    int i = -1, ci, j;
    if (!pSearchString.isEmpty()) {
      while ((i = text.indexOf(pSearchString, i+1, Qt::CaseInsensitive)) != -1)
	c << ColorRegion(f_highlight, i, pSearchString.length());
    }
    qSort(c);
    //    klfDbg( "searchstr="<<pSearchString<<"; label "<<text<<": c is "<<c ) ;
    QTextDocument textDocument;
    textDocument.setDefaultFont(p->option->font);
    QTextCursor cur(&textDocument);
    QList<ColorRegion> appliedfmts;
    for (i = ci = 0; i < text.length(); ++i) {
      //      klfDbg( "Processing char "<<text[i]<<"; i="<<i<<"; ci="<<ci ) ;
      if (ci >= c.size() && appliedfmts.size() == 0) {
	// insert all remaining text (no more formatting needed)
	cur.insertText(Qt::escape(text.mid(i)), QTextCharFormat());
	break;
      }
      while (ci < c.size() && c[ci].start == i) {
	appliedfmts.append(c[ci]);
	++ci;
      }
      QTextCharFormat f;
      for (j = 0; j < appliedfmts.size(); ++j) {
	if (i >= appliedfmts[j].start + appliedfmts[j].len) {
	  // this format no longer applies
	  appliedfmts.removeAt(j);
	  --j;
	  continue;
	}
	f.merge(appliedfmts[j].fmt);
      }
      cur.insertText(Qt::escape(text[i]), f);
    }

    QSizeF s = textDocument.size();
    QRectF textRect = p->innerRectText;
    // note: setLeft changes width, not right.
    textRect.setLeft(textRect.left()-4); // some offset because of qt's rich text drawing ..?
    // textRect is now correct
    // draw a selection underline, if needed
    if (flags & PTF_SelUnderline) {
       QColor h1 = p->option->palette.color(QPalette::Highlight);
       QColor h2 = h1;
       h1.setAlpha(0);
       h2.setAlpha(220);
       QLinearGradient gr(0.f, 0.f, 0.f, 1.f);
       gr.setCoordinateMode(QGradient::ObjectBoundingMode);
       gr.setColorAt(0.f, h1);
       gr.setColorAt(1.f, h2);
       QBrush selbrush(gr);
       p->p->save();
       //       p->p->setClipRect(p->option->rect);
       p->p->fillRect(QRectF(textRect.left(), textRect.bottom()-10,
			     textRect.width(), 10), selbrush);
       p->p->restore();
    }
    // check the text size
    drawnTextWidth = (int)s.width();
    if (s.width() > textRect.width()) { // sorry, too large
      s.setWidth(textRect.width());
    }
    p->p->save();
    p->p->setClipRect(textRect);
    p->p->setPen(borderfadepen);
    p->p->translate(textRect.topLeft());
    p->p->translate( QPointF( 0, //textRect.width() - s.width(),
			      textRect.height() - s.height()) / 2.f );
    textDocument.drawContents(p->p);
    p->p->restore();
    drawnBaseLineY = (int)(textRect.bottom() - (textRect.height() - s.height()) / 2.f);
  }

  // draw arrow if text too large

  if (drawnTextWidth > p->innerRectText.width()) {
    // draw small arrow indicating more text
    //      c.setAlpha(80);
    p->p->save();
    p->p->translate(p->option->rect.right()-2, drawnBaseLineY-2);
    p->p->setPen(textcol);
    p->p->drawLine(0, 0, -16, 0);
    p->p->drawLine(0, 0, -2, +2);
    p->p->restore();
  }
}

void KLFLibViewDelegate::setEditorData(QWidget */*editor*/, const QModelIndex& /*index*/) const
{
}
void KLFLibViewDelegate::setModelData(QWidget */*editor*/, QAbstractItemModel */*model*/,
				      const QModelIndex& /*index*/) const
{
}
QSize KLFLibViewDelegate::sizeHint(const QStyleOptionViewItem& option, const QModelIndex& index) const
{
  KLF_DEBUG_TIME_BLOCK(KLF_FUNC_NAME) ;
  klfDbg( "\tindex="<<index ) ;
  int kind = index.data(KLFLibModel::ItemKindItemRole).toInt();
  if (kind == KLFLibModel::EntryKind) {
    int prop = -1;
    switch (index.data(KLFLibModel::EntryContentsTypeItemRole).toInt()) {
    case KLFLibEntry::Latex: prop = KLFLibEntry::Latex;
    case KLFLibEntry::Category: prop = (prop > 0) ? prop : KLFLibEntry::Category;
    case KLFLibEntry::Tags: prop = (prop > 0) ? prop : KLFLibEntry::Tags;
      return QFontMetrics(option.font)
	.size(0, index.data(KLFLibModel::entryItemRole(prop)).toString())+QSize(4,2);
    case KLFLibEntry::DateTime:
      { QLocale loc = klfconfig.UI.locale;
	return QFontMetrics(option.font)
	  .size(0, loc.toString(index.data(KLFLibModel::entryItemRole(KLFLibEntry::DateTime))
				.toDateTime(), QLocale::LongFormat) )+QSize(4,2);
      }
    case KLFLibEntry::Preview:
      // sizeHint() is called for all entries, also those that are not visible. So no time-consuming
      // operations and don't make model update its minimalist entries.
      // return index.data(KLFLibModel::entryItemRole(KLFLibEntry::Preview)).value<QImage>().size()
      //                                                                                  +QSize(4,4);
      //      return klfconfig.UI.labelOutputFixedSize + QSize(4,4);
      return index.data(KLFLibModel::entryItemRole(KLFLibEntry::PreviewSize)).value<QSize>() + QSize(4,4);
    default:
      return QSize();
    }
  } else if (kind == KLFLibModel::CategoryLabelKind) {
    return QFontMetrics(option.font)
      .size(0, index.data(KLFLibModel::CategoryLabelItemRole).toString())+QSize(4,2);
  } else {
    qWarning("KLFLibItemViewDelegate::sizeHint(): Bad Item kind: %d\n", kind);
    return QSize();
  }
}
void KLFLibViewDelegate::updateEditorGeometry(QWidget */*editor*/,
					      const QStyleOptionViewItem& /*option*/,
					      const QModelIndex& /*index*/) const
{
}


bool KLFLibViewDelegate::indexHasSelectedDescendant(const QModelIndex& parent) const
{
  KLF_DEBUG_TIME_BLOCK(KLF_FUNC_NAME) ;
  klfDbg( "\t parent="<<parent ) ;
  QTime tm; tm.start();
  return func_indexHasSelectedDescendant(parent, tm, 250);
}

bool KLFLibViewDelegate::selectionIntersectsIndexChildren(const QItemSelection& selection,
							  const QModelIndex& parent) const
{
  KLF_DEBUG_TIME_BLOCK(KLF_FUNC_NAME) ;
  klfDbg( ": selection size is "<<selection.size() ) ;
  int k;
  for (k = 0; k < selection.size(); ++k) {
    if (selection[k].parent() != parent)
      continue;
    if (selection[k].isValid()) // selection range not empty
      return true;
  }
  return false;
}

bool KLFLibViewDelegate::func_indexHasSelectedDescendant(const QModelIndex& parent, const QTime& timer,
							 int timeLimitMs) const
{
  KLF_DEBUG_TIME_BLOCK(KLF_FUNC_NAME) ;
  if (!parent.isValid()) {
    qWarning()<<KLF_FUNC_NAME<<": parent is invalid!";
    return false;
  }
  if (selectionIntersectsIndexChildren(pSelModel->selection(), parent)) {
    klfDbg( ": selection under index parent="<<parent ) ;
    return true;
  }
  const QAbstractItemModel *model = parent.model();
  if (model == NULL) {
    qWarning()<<KLF_FUNC_NAME<<": parent has NULL model!";
    return false;
  }

  int k;
  QModelIndex idx;
  for (k = 0; k < model->rowCount(parent); ++k) {
    idx = model->index(k, 0, parent);

    if (!idx.isValid() || !model->hasChildren(idx))
      continue;
    if (func_indexHasSelectedDescendant(idx, timer, timeLimitMs))
      return true;
    // stop this function if it is taking too long.
    if (timer.elapsed() > timeLimitMs)
      return false;

  }
  return false;
  //   // simple, dumb way: walk selection list.
  //   int k;
  //   QModelIndexList sel = pSelModel->selectedIndexes();
  //   KLFLibModel *model = (KLFLibModel*)index.model();
  //   for (k = 0; k < sel.size(); ++k) {
  //     if (model->isDesendantOf(sel[k], index)) {
  //       // this selected item is child of our index
  //       return true;
  //     }
  //   }
  //   return false;
}




// -------------------------------------------------------

QDebug& operator<<(QDebug& dbg, const KLFLibResourceEngine::KLFLibEntryWithId& e)
{
  return dbg <<"KLFLibEntryWithId(id="<<e.id<<";"<<e.entry.category()<<","<<e.entry.tags()<<","
	     <<e.entry.latex()<<")";
}


KLFLibDefaultView::KLFLibDefaultView(QWidget *parent, ViewType view)
  : KLFAbstractLibView(parent), pViewType(view)
{
  KLF_DEBUG_TIME_BLOCK(KLF_FUNC_NAME) ;

  pModel = NULL;

  pIconViewLockAction = NULL;
  pIconViewRelayoutAction = NULL;

  pEventFilterNoRecurse = false;

  QVBoxLayout *lyt = new QVBoxLayout(this);
  lyt->setMargin(0);
  lyt->setSpacing(0);
  pDelegate = new KLFLibViewDelegate(this);

  KLFLibDefTreeView *treeView = NULL;
  KLFLibDefListView *listView = NULL;
  switch (pViewType) {
  case IconView:
    listView = new KLFLibDefListView(this);
    klfDbg( "Created list view." ) ;
    listView->setViewMode(QListView::IconMode);
    listView->setSpacing(15);
    listView->setMovement(QListView::Free);
    //    listView->setFlow(QListView::LeftToRight);
    //    listView->setFlow(QListView::TopToBottom);
    listView->setFlow((QListView::Flow)klfconfig.LibraryBrowser.iconViewFlow);
    listView->setResizeMode(QListView::Adjust);
    klfDbg( "prepared list view." ) ;
    pView = listView;
    break;
  case CategoryTreeView:
  case ListTreeView:
  default:
    treeView = new KLFLibDefTreeView(this);
    treeView->setSortingEnabled(true);
    treeView->setIndentation(16);
    treeView->setAllColumnsShowFocus(true);
    //    treeView->setUniformRowHeights(true);
    pDelegate->setTheTreeView(treeView);
    //    connect(treeView, SIGNAL(expanded(const QModelIndex&)),
    //	    this, SLOT(slotExpanded(const QModelIndex&)));
    //    connect(treeView, SIGNAL(collapsed(const QModelIndex&)),
    //	    this, SLOT(slotCollapsed(const QModelIndex&)));
    pView = treeView;
    break;
  };

  lyt->addWidget(pView);

  //  pView->setViewport(new KLFNoRecurseRepaintWidget(this));

  pView->setSelectionBehavior(QAbstractItemView::SelectRows);
  pView->setSelectionMode(QAbstractItemView::ExtendedSelection);
  pView->setDragEnabled(true);
  pView->setDragDropMode(QAbstractItemView::DragDrop);
  pView->setDragDropOverwriteMode(false);
  pView->setVerticalScrollMode(QAbstractItemView::ScrollPerPixel);
  pView->setHorizontalScrollMode(QAbstractItemView::ScrollPerPixel);
  pView->setItemDelegate(pDelegate);
  pView->viewport()->installEventFilter(this);
  pView->installEventFilter(this);
  installEventFilter(this);

  connect(pView, SIGNAL(clicked(const QModelIndex&)),
	  this, SLOT(slotViewItemClicked(const QModelIndex&)));
  connect(pView, SIGNAL(doubleClicked(const QModelIndex&)),
	  this, SLOT(slotEntryDoubleClicked(const QModelIndex&)));
}
KLFLibDefaultView::~KLFLibDefaultView()
{
}

QUrl KLFLibDefaultView::url() const
{
  if (pModel == NULL) {
    qWarning()<<KLF_FUNC_NAME<<": model is NULL!";
    return QUrl();
  }
  return pModel->url();
}

bool KLFLibDefaultView::event(QEvent *event)
{
  return KLFAbstractLibView::event(event);
}
bool KLFLibDefaultView::eventFilter(QObject *object, QEvent *event)
{
  return KLFAbstractLibView::eventFilter(object, event);
}

KLFLibEntryList KLFLibDefaultView::selectedEntries() const
{
  QModelIndexList selectedindexes = selectedEntryIndexes();
  KLFLibEntryList elist;
  // fill the entry list with our selected entries
  int k;
  for (k = 0; k < selectedindexes.size(); ++k) {
    if ( selectedindexes[k].data(KLFLibModel::ItemKindItemRole) != KLFLibModel::EntryKind )
      continue;
    KLFLibEntry entry = selectedindexes[k].data(KLFLibModel::FullEntryItemRole).value<KLFLibEntry>();
    klfDbg(  "selection list: adding item [latex="<<entry.latex()<<"; tags="<<entry.tags()<<"]" ) ;
    elist << entry;
  }
  return elist;
}

QList<QAction*> KLFLibDefaultView::addContextMenuActions(const QPoint& /*pos*/)
{
  QList<QAction*> actionList = QList<QAction*>() << pCommonActions;
  if (pViewType == IconView) {
    actionList << pIconViewActions;
  }
  if (pViewType == CategoryTreeView || pViewType == ListTreeView) {
    actionList << pShowColumnActions;
  }
  return actionList;
}

bool KLFLibDefaultView::canMoveIcons() const
{
  return false;
  //  KLFLibResourceEngine *e = resourceEngine();
  //  if (e == NULL)
  //    return false;
  //  if (e->propertyNameRegistered("IconView_IconPositionsLocked") &&
  //      e->resourceProperty("IconView_IconPositionsLocked").toBool())
  //    return false;
  //  if (!e->accessShared() && e->locked())
  //    return false;
  //  return true;
}


QVariantMap KLFLibDefaultView::saveGuiState() const
{
  QVariantMap vst;
  if (pViewType == CategoryTreeView || pViewType == ListTreeView) {
    QTreeView *tv = qobject_cast<QTreeView*>(pView);
    KLF_ASSERT_NOT_NULL( tv, "Tree View is NULL in view type "<<pViewType<<" !!", return QVariantMap() )
      ;
    vst["ColumnsState"] = QVariant::fromValue<QByteArray>(tv->header()->saveState());
  }
  if (pViewType == IconView) {
    /*    KLFLibDefListView *lv = qobject_cast<KLFLibDefListView*>(pView);
    KLF_ASSERT_NOT_NULL( lv, "KLFLibDefListView List View is NULL in view type "<<pViewType<<" !!",
			 return QVariantMap() )
      ;
    if ( resourceEngine()->accessShared() ) {
      // access is shared, so we store icon position info in Gui state instead
      // of in the resource data itself.
      QMap<KLFLibResourceEngine::entryId,QPoint> iconpositions = lv->allIconPositions();
      QMap<KLFLibResourceEngine::entryId,QPoint>::const_iterator it;
      QVariantList vEntryIds; // will hold entryId's in qint32 format
      QVariantList vPositions; // will hold QPoint's
      for (it = iconpositions.begin(); it != iconpositions.end(); ++it) {
	vEntryIds << QVariant::fromValue<qint32>(it.key());
	vPositions << QVariant::fromValue<QPoint>(it.value());
      }
      vst["IconPositionsEntryIdList"] = QVariant::fromValue<QVariantList>(vEntryIds);
      vst["IconPositionsPositionList"] = QVariant::fromValue<QVariantList>(vPositions);
      } */
  }
  return vst;
}
bool KLFLibDefaultView::restoreGuiState(const QVariantMap& vstate)
{
  if (pViewType == CategoryTreeView || pViewType == ListTreeView) {
    QByteArray colstate = vstate["ColumnsState"].toByteArray();
    QTreeView *tv = qobject_cast<QTreeView*>(pView);
    KLF_ASSERT_NOT_NULL( tv, "Tree View is NULL in view type"<<pViewType<<"!!", return false )
      ;
    tv->header()->restoreState(colstate);
  }
  if (pViewType == IconView) {
    /*    KLFLibDefListView *lv = qobject_cast<KLFLibDefListView*>(pView);
    KLF_ASSERT_NOT_NULL( lv, "KLFLibDefListView List View is NULL in view type "<<pViewType<<" !!",
			 return false )
      ;
    if ( resourceEngine()->accessShared() ) {
      // access is shared, so we store icon position info in Gui state instead
      // of in the resource data itself.
      QVariantList vEntryIds = vstate["IconPositionsEntryIdList"].toList();
      QVariantList vPositions = vstate["IconPositionsPositionList"].toList();
      QMap<KLFLibResourceEngine::entryId,QPoint> iconpositions;
      int k;
      for (k = 0; k < vEntryIds.size() && k < vPositions.size(); ++k) {
	iconpositions[vEntryIds[k].value<qint32>()] = vPositions[k].value<QPoint>();
      }
      lv->loadIconPositions(iconpositions);
      } */
  }
  return true;
}

QModelIndex KLFLibDefaultView::currentVisibleIndex() const
{
  QModelIndex index;
  if (pViewType == IconView) {
    KLFLibDefListView *lv = qobject_cast<KLFLibDefListView*>(pView);
    KLF_ASSERT_NOT_NULL( lv, "KLFLibDefListView List View is NULL in view type "<<pViewType<<" !!",
			 return QModelIndex() )
      ;
    index = lv->curVisibleIndex();
  } else if (pViewType == CategoryTreeView || pViewType == ListTreeView) {
    KLFLibDefTreeView *tv = qobject_cast<KLFLibDefTreeView*>(pView);
    KLF_ASSERT_NOT_NULL( tv, "KLFLibDefTreeView List View is NULL in view type "<<pViewType<<" !!",
			 return QModelIndex() )
      ;
    index = tv->curVisibleIndex();
  } else {
    index = QModelIndex();
  }
  return index;
}


#ifdef KLF_DEBUG_USE_MODELTEST
#include <modeltest.h>
#endif

void KLFLibDefaultView::updateResourceEngine()
{
  KLF_DEBUG_TIME_BLOCK(KLF_FUNC_NAME) ;

  int k;
  KLFLibResourceEngine *resource = resourceEngine();
  if (resource == NULL) {
    pModel = NULL;
    pView->setModel(NULL);
    for (k = 0; k < pShowColumnActions.size(); ++k)
      delete pShowColumnActions[k];
    pShowColumnActions.clear();
    if (pIconViewLockAction) {
      delete pIconViewLockAction;
      pIconViewLockAction = NULL;
    }
    if (pIconViewRelayoutAction) {
      delete pIconViewRelayoutAction;
      pIconViewRelayoutAction = NULL;
    }
  }

  //  klfDbg(  "KLFLibDefaultView::updateResourceEngine: All items:\n"<<resource->allEntries() ) ;
  uint model_flavor = 0;
  switch (pViewType) {
  case IconView:
  case ListTreeView:
    model_flavor = KLFLibModel::LinearList;
    break;
  case CategoryTreeView:
  default:
    model_flavor = KLFLibModel::CategoryTree;
    break;
  };

  if (klfconfig.LibraryBrowser.groupSubCategories)
    model_flavor |= KLFLibModel::GroupSubCategories;

  pModel = new KLFLibModel(resource, model_flavor, this);
  pView->setModel(pModel);

#ifdef KLF_DEBUG_USE_MODELTEST
  new ModelTest(pModel, this);
#endif

  if (pViewType == IconView) {
    qobject_cast<KLFLibDefListView*>(pView)->modelInitialized();
  } else {
    qobject_cast<KLFLibDefTreeView*>(pView)->modelInitialized();
  }

  // get informed about selections
  QItemSelectionModel *s = pView->selectionModel();
  connect(s, SIGNAL(selectionChanged(const QItemSelection&, const QItemSelection&)),
	  this, SLOT(slotViewSelectionChanged(const QItemSelection&, const QItemSelection&)));

  connect(pModel, SIGNAL(dataChanged(const QModelIndex&, const QModelIndex&)),
	  this, SLOT(slotResourceDataChanged(const QModelIndex&, const QModelIndex&)));

  // delegate wants to know more about selections...
  pDelegate->setSelectionModel(s);

  QAction *selectAllAction = new QAction(tr("Select All", "[[menu action]]"), this);
  selectAllAction->setShortcut(QKeySequence::SelectAll);
  connect(selectAllAction, SIGNAL(triggered()), this, SLOT(slotSelectAll()));
  QAction *refreshAction = new QAction(tr("Refresh", "[[menu action]]"), this);
  refreshAction->setShortcut(QKeySequence::Refresh);
  connect(refreshAction, SIGNAL(triggered()), pModel, SLOT(completeRefresh()));
  pCommonActions = QList<QAction*>() << selectAllAction << refreshAction;

  if (pViewType == IconView) {
    klfDbg( "About to prepare iconview." ) ;
    //    KLFLibDefListView *lv = qobject_cast<KLFLibDefListView*>(pView);
    /*    if ( ! resource->propertyNameRegistered("IconView_IconPositionsLocked") )
	  resource->loadResourceProperty("IconView_IconPositionsLocked", false);
    */
    pIconViewRelayoutAction = new QAction(tr("Relayout All Icons", "[[menu action]]"), this);
    connect(pIconViewRelayoutAction, SIGNAL(triggered()), this, SLOT(slotRelayoutIcons()));
    pIconViewLockAction = new QAction(tr("Lock Icon Positions", "[[menu action]]"), this);
    pIconViewLockAction->setCheckable(true);
    connect(pIconViewLockAction, SIGNAL(toggled(bool)), this, SLOT(slotLockIconPositions(bool)));
    connect(pIconViewLockAction, SIGNAL(toggled(bool)),
	    pIconViewRelayoutAction, SLOT(setDisabled(bool)));
    bool iconposlocked = true;
      //      = resource->resourceProperty("IconView_IconPositionsLocked").toBool();
    pIconViewLockAction->setChecked(iconposlocked);
    //     bool iconposlockenabled
    //       = resource->propertyNameRegistered("IconView_IconPositionsLocked") &&
    //       /* */ resource->canModifyProp(resource->propertyIdForName("IconView_IconPositionsLocked"));
    //     pIconViewLockAction->setEnabled(iconposlockenabled);
    // pIconViewRelayoutAction->setEnabled(canMoveIcons());

    pIconViewActions = QList<QAction*>() << pIconViewRelayoutAction ;/* << pIconViewLockAction; */
  }
  if (pViewType == CategoryTreeView || pViewType == ListTreeView) {
    QTreeView *treeView = qobject_cast<QTreeView*>(pView);
    // select some columns to show
    treeView->setColumnHidden(pModel->columnForEntryPropertyId(KLFLibEntry::Preview), false);
    treeView->setColumnHidden(pModel->columnForEntryPropertyId(KLFLibEntry::Latex), true);
    treeView->setColumnHidden(pModel->columnForEntryPropertyId(KLFLibEntry::Tags), false);
    treeView->setColumnHidden(pModel->columnForEntryPropertyId(KLFLibEntry::Category), true);
    treeView->setColumnHidden(pModel->columnForEntryPropertyId(KLFLibEntry::DateTime), true);
    // optimize column sizes
    for (k = 0; k < pModel->columnCount(); ++k)
      treeView->resizeColumnToContents(k);
    treeView->setColumnWidth(0, 35+treeView->columnWidth(0));
    // and provide a menu to show/hide these columns
    int col;
    QMenu *colMenu = new QMenu(this);
    for (col = 0; col < pModel->columnCount(); ++col) {
      QString title = pModel->headerData(col, Qt::Horizontal, Qt::DisplayRole).toString();
      QAction *a;
      a = new QAction(title, this);
      a->setProperty("klfModelColumn", col);
      a->setCheckable(true);
      a->setChecked(!treeView->isColumnHidden(col));
      connect(a, SIGNAL(toggled(bool)), this, SLOT(slotShowColumnSenderAction(bool)));
      colMenu->addAction(a);
    }
    QAction *menuAction = new QAction(tr("Show/Hide Columns", "[[menu with sub-menu]]"), this);
    menuAction->setMenu(colMenu);
    pShowColumnActions = QList<QAction*>() << menuAction;
    // expand root items if they contain little number of children
    // or if there are little number of root items
    int numRootItems = pModel->rowCount(QModelIndex());
    for (k = 0; k < pModel->rowCount(QModelIndex()); ++k) {
      QModelIndex i = pModel->index(k, 0, QModelIndex());
      if (pModel->rowCount(i) < 6 || numRootItems < 4)
	treeView->expand(i);
    }
  }

  updateResourceProp(-1); // update all with respect to resource changes..
  updateResourceOwnData(QList<KLFLib::entryId>());

  wantMoreCategorySuggestions();
}

void KLFLibDefaultView::updateResourceData(const QString& subRes, int modifyType,
					   const QList<KLFLib::entryId>& entryIdList)
{
  KLF_ASSERT_NOT_NULL( pModel , "Model is NULL!" , return )
    ;
  if (subRes != resourceEngine()->defaultSubResource())
    return;
  pModel->updateData(entryIdList, modifyType);
  // update our own data (icon positions)
  updateResourceOwnData(entryIdList);
}
void KLFLibDefaultView::updateResourceOwnData(const QList<KLFLib::entryId>& /*entryIdList*/)
{
  /** \todo ..................... OPTIMIZE THIS FUNCTION not to do a full refresh */
  klfDbg( KLF_FUNC_NAME ) ;
  if (pViewType == IconView) {
    /* KLFLibDefListView *lv = qobject_cast<KLFLibDefListView*>(pView);
       KLF_ASSERT_NOT_NULL( lv, "KLFLibDefListView List View is NULL in view type "<<pViewType<<" !!",
       return )
       ;
       if ( ! lv->isSavingIconPositions() ) {
      // Important: do NOT update if the update was generated because an icon position was
      // saved by the view!

      // load the icon positions stored in the resource's entries as custom properties.
      int k;
      QList<KLFLibResourceEngine::KLFLibEntryWithId> allentries = pModel->resource()->allEntries();
      QMap<KLFLib::entryId, QPoint> readIconPositions;
      for (k = 0; k < allentries.size(); ++k) {
	QVariant pos = allentries[k].entry.property("IconView_IconPosition");
	if (pos.isValid() && pos.canConvert<QPoint>())
	  readIconPositions[allentries[k].id] = pos.value<QPoint>();
      }
      // update the view
      // call this function now. It will detect and possibly delay the actual icon position loading
      // until widget polishing time, if needed.
      lv->loadIconPositions(readIconPositions);
      } */
  }
}
void KLFLibDefaultView::updateResourceProp(int propId)
{
  klfDbg( "propId="<<propId ) ;

  // if locked property was modified, refresh
  // Lock Icon Positions & Relayout Icons according to whether we store that info
  // in resource or in view state...

  KLF_ASSERT_NOT_NULL( resourceEngine() , "Resource Engine is NULL, skipping !" , return ) ;

  //  if ( propId == -1 || propId == KLFLibResourceEngine::PropLocked ) {
  //     if (pIconViewRelayoutAction != NULL)
  //       pIconViewRelayoutAction->setEnabled(canMoveIcons());
  //     if (pIconViewLockAction != NULL) {
  //       int propId = resourceEngine()->propertyIdForName("IconView_IconPositionsLocked");
  //       bool canmodify = resourceEngine()->canModifyProp(propId);
  //       if ( ! canmodify )
  // 	pIconViewLockAction->setChecked(!canMoveIcons());
  //       pIconViewLockAction->setEnabled(canmodify);
  //       if (canmodify) {
  // 	bool locked = pModel->resource()->resourceProperty("IconView_IconPositionsLocked").toBool();
  // 	pIconViewLockAction->setChecked(locked);
  //       }
  //     }
  //  }
}

void KLFLibDefaultView::showEvent(QShowEvent *event)
{
  if (pModel)
    pModel->setFetchBatchCount(100);
}



QStringList KLFLibDefaultView::getCategorySuggestions()
{
  return pModel->categoryList();
}

bool KLFLibDefaultView::writeEntryProperty(int property, const QVariant& value)
{
  return pModel->changeEntries(selectedEntryIndexes(), property, value);
}
bool KLFLibDefaultView::deleteSelected(bool requireConfirm)
{
  QModelIndexList sel = selectedEntryIndexes();
  if (sel.size() == 0)
    return true;
  if (requireConfirm) {
    QMessageBox::StandardButton res
      = QMessageBox::question(this, tr("Delete?"),
			      tr("Delete %n selected item(s) from resource \"%1\"?", "", sel.size())
			      .arg(pModel->resource()->title()),
			      QMessageBox::Yes|QMessageBox::Cancel, QMessageBox::Cancel);
    if (res != QMessageBox::Yes)
      return false; // abort action
  }
  return pModel->deleteEntries(sel);
}
bool KLFLibDefaultView::insertEntries(const KLFLibEntryList& entries)
{
  return pModel->insertEntries(entries);
}



void KLFLibDefaultView::restore(uint restoreflags)
{
  QModelIndexList sel = selectedEntryIndexes();
  if (sel.size() != 1) {
    qWarning("KLFLibDefaultView::restoreWithStyle(): Cannot restore: %d items selected.", sel.size());
    return;
  }

  KLFLibEntry e = sel[0].data(KLFLibModel::FullEntryItemRole).value<KLFLibEntry>();
  
  emit requestRestore(e, restoreflags);
}

void KLFLibDefaultView::showColumns(int propIdColumn, bool show)
{
  if ( ! pView->inherits("QTreeView") ) {
    qWarning("KLFLibDefaultView::showColumns(%d,%s): Resource view for %s: view does not inherit QTreeView!",
	     propIdColumn, show?"[show]":"[hide]", qPrintable(resourceEngine()->url().toString()));
    return;
  }
  int colNo = pModel->columnForEntryPropertyId(propIdColumn);
  qobject_cast<QTreeView*>(pView)->setColumnHidden(colNo, show);
}

void KLFLibDefaultView::sortBy(int propIdColumn, Qt::SortOrder sortorder)
{
  if ( ! pView->inherits("QTreeView") ) {
    qWarning("KLFLibDefaultView::showBy(%d,%s): Resource view for %s: view does not inherit QTreeView!",
	     propIdColumn, (sortorder == Qt::AscendingOrder)?"[Ascending]":"[Descending]" ,
	     qPrintable(resourceEngine()->url().toString()));
    return;
  }
  QTreeView * tree = qobject_cast<QTreeView*>(pView);
  int colNo = pModel->columnForEntryPropertyId(propIdColumn);
  if (colNo < 0 || colNo >= pModel->columnCount()) {
    qWarning("KLFLibDefaultView::showBy(%d,%s): column number %d is not valid or hidden!",
	     propIdColumn, (sortorder == Qt::AscendingOrder)?"[Ascending]":"[Descending]", colNo);
    return;
  }
  tree->sortByColumn(colNo, sortorder);
}


void KLFLibDefaultView::slotSelectAll(const QModelIndex& parent, bool rootCall)
{
  // this function requires to fetch all items in parent!
  KLFDelayedPleaseWaitPopup *pleaseWait = NULL;
  if (rootCall) {
    pleaseWait = new KLFDelayedPleaseWaitPopup(tr("Fetching and selecting all, please wait ..."), this);
    pleaseWait->setDisableUi(true);
    pleaseWait->setDelay(500);
  }

  while (pModel->canFetchMore(parent)) {
    pModel->fetchMore(parent);
    if (pleaseWait != NULL)
      pleaseWait->process();
  }

  if (rootCall)
    pView->selectionModel()->blockSignals(true);
  int k;
  QModelIndex topLeft = pModel->index(0, 0, parent);
  QModelIndex bottomRight = pModel->index(pModel->rowCount(parent)-1,
					  pModel->columnCount(parent)-1, parent);
  pView->selectionModel()->select(QItemSelection(topLeft, bottomRight), QItemSelectionModel::Select);
  if ( ! pModel->hasChildren(parent) )
    return;

  QTime t; t.start();
  for (k = 0; k < pModel->rowCount(parent); ++k) {
    QModelIndex child = pModel->index(k, 0, parent);
    slotSelectAll(child, false);
    if (t.elapsed() > 500) {
      qApp->processEvents();
      if (pleaseWait != NULL)
	pleaseWait->process();
      t.restart();
    }
  }
  if (rootCall) {
    pView->selectionModel()->blockSignals(false);
    emit entriesSelected(selectedEntries());
    delete pleaseWait;
  }
}

void KLFLibDefaultView::slotRelayoutIcons()
{
  //   if (pViewType != IconView || !pView->inherits("KLFLibDefListView")) {
  //     return;
  //   }
  //   if (pModel->resource()->resourceProperty("IconView_IconPositionsLocked").toBool())
  //     return;
  KLFLibDefListView *lv = qobject_cast<KLFLibDefListView*>(pView);
  // force a re-layout
  lv->forceRelayout();
}
void KLFLibDefaultView::slotLockIconPositions(bool locked)
{
  //  if (locked == pModel->resource()->resourceProperty("IconView_IconPositionsLocked").toBool())
  //    return; // no change

  /*  klfDbg( "Locking icon positions to "<<locked ) ;
      bool r = pModel->resource()->loadResourceProperty("IconView_IconPositionsLocked",
      QVariant::fromValue(locked));
      if (!r) {
      qWarning()<<"Can't lock icon positions!";
      return;
      }
      pIconViewLockAction->setChecked(locked);
  */
}




bool KLFLibDefaultView::searchFind(const QString& queryString, bool forward)
{
  QModelIndex fromIndex = currentVisibleIndex();
  // take one index above in case it's partially shown
  fromIndex = pModel->walkPrevIndex(fromIndex);
  QModelIndex i = pModel->searchFind(queryString, fromIndex, forward);
  pDelegate->setSearchString(queryString);
  searchFound(i);
  return i.isValid();
}

bool KLFLibDefaultView::searchFindNext(bool forward)
{
  QModelIndex i = pModel->searchFindNext(forward);
  searchFound(i);
  return i.isValid();
}

void KLFLibDefaultView::searchAbort()
{
  pModel->searchAbort();
  pDelegate->setSearchString(QString());
  pDelegate->setSearchIndex(QModelIndex());
  pView->viewport()->update(); // repaint widget to update search underline

  // don't un-select the found index...
  //  searchFound(QModelIndex());
}

// private
void KLFLibDefaultView::searchFound(const QModelIndex& i)
{
  pDelegate->setSearchIndex(i);
  if ( ! i.isValid() ) {
    pView->scrollTo(QModelIndex(), QAbstractItemView::PositionAtTop);
    // unselect all
    pView->selectionModel()->setCurrentIndex(QModelIndex(), QItemSelectionModel::Clear);
    return;
  }
  pView->scrollTo(i, QAbstractItemView::EnsureVisible);
  if (pViewType == CategoryTreeView) {
    // if tree view, expand item
    qobject_cast<QTreeView*>(pView)->expand(i);
  }
  // select the item
  pView->selectionModel()->setCurrentIndex(i,
					   QItemSelectionModel::ClearAndSelect|
					   QItemSelectionModel::Rows);
  pView->update(i);
}


void KLFLibDefaultView::slotViewSelectionChanged(const QItemSelection& /*selected*/,
						 const QItemSelection& /*deselected*/)
{
  pView->viewport()->update();
  
  emit entriesSelected(selectedEntries());
}

void KLFLibDefaultView::slotResourceDataChanged(const QModelIndex& topLeft,
						const QModelIndex& bottomRight)
{
  QItemSelectionRange rng = QItemSelectionRange(topLeft, bottomRight);
  QModelIndexList indexes = rng.indexes();
  QList<KLFLib::entryId> eids = pModel->entryIdForIndexList(indexes);
  emit resourceDataChanged(eids);
}


QItemSelection KLFLibDefaultView::fixSelection(const QModelIndexList& selidx)
{
  QModelIndexList moreselidx;
  QItemSelection moresel;

  int k, j;
  for (k = 0; k < selidx.size(); ++k) {
    if (selidx[k].isValid() &&
	selidx[k].data(KLFLibModel::ItemKindItemRole).toInt() == KLFLibModel::CategoryLabelKind &&
	selidx[k].column() == 0) {
      // a category is selected -> select all its children
      const QAbstractItemModel *model = selidx[k].model();
      int nrows = model->rowCount(selidx[k]);
      int ncols = model->columnCount(selidx[k]);
      if (pViewType == CategoryTreeView) {
	// if tree view, expand item
	qobject_cast<QTreeView*>(pView)->expand(selidx[k]);
      }
      for (j = 0; j < nrows; ++j) {
	switch (selidx[k].child(j,0).data(KLFLibModel::ItemKindItemRole).toInt()) {
	case KLFLibModel::CategoryLabelKind:
	  moreselidx.append(selidx[k].child(j,0));
	  // no break; proceed to entrykind->
	case KLFLibModel::EntryKind:
	  moresel.append(QItemSelectionRange(selidx[k].child(j, 0),
					     selidx[k].child(j, ncols-1)));
	  break;
	default: ;
	}
	
      }
    }
  }

  if (moresel.isEmpty()) {
    return QItemSelection();
  }

  QItemSelection s = moresel;
  QItemSelection morefixed = fixSelection(moreselidx);
  s.merge(morefixed, QItemSelectionModel::Select);
  return s;

}

void KLFLibDefaultView::slotViewItemClicked(const QModelIndex& index)
{
  if (index.column() != 0)
    return;
  QItemSelection fixed = fixSelection( QModelIndexList() << index);
  pView->selectionModel()->select(fixed, QItemSelectionModel::Select);
}
void KLFLibDefaultView::slotEntryDoubleClicked(const QModelIndex& index)
{
  if (index.data(KLFLibModel::ItemKindItemRole).toInt() != KLFLibModel::EntryKind)
    return;
  emit requestRestore(index.data(KLFLibModel::FullEntryItemRole).value<KLFLibEntry>(),
		      KLFLib::RestoreLatex|KLFLib::RestoreStyle);
}

// void KLFLibDefaultView::slotExpanded(const QModelIndex& index)
// {
//   pDelegate->setIndexExpanded(index, true);
// }
// void KLFLibDefaultView::slotCollapsed(const QModelIndex& index)
// {
//   pDelegate->setIndexExpanded(index, false);
// }

void KLFLibDefaultView::slotShowColumnSenderAction(bool showCol)
{
  QObject *a = sender();
  if (a == NULL)
    return;

  if ( ! pView->inherits("QTreeView") )
    return;
  int colNo = a->property("klfModelColumn").toInt();
  qobject_cast<QTreeView*>(pView)->setColumnHidden(colNo, !showCol);
}




QModelIndexList KLFLibDefaultView::selectedEntryIndexes() const
{
  QModelIndexList selection = pView->selectionModel()->selectedIndexes();
  QModelIndexList entryindexes;
  int k;
  QList<KLFLib::entryId> doneEntryIds;
  // walk selection and strip all non-zero column number indexes
  for (k = 0; k < selection.size(); ++k) {
    KLFLib::entryId eid = selection[k].data(KLFLibModel::EntryIdItemRole).toInt();
    int iPos = qLowerBound(doneEntryIds.begin(), doneEntryIds.end(), eid) - doneEntryIds.begin();
    if ( iPos < doneEntryIds.size() && doneEntryIds[iPos] == eid ) // already in list
      continue;
    doneEntryIds.insert(iPos, eid);
    entryindexes << selection[k];
  }
  return entryindexes;
}



// ------------

static QStringList defaultViewTypeIds = QStringList()<<"default"<<"default+list"<<"default+icons";


KLFLibDefaultViewFactory::KLFLibDefaultViewFactory(QObject *parent)
  : KLFLibViewFactory(defaultViewTypeIds, parent)
{
}


QString KLFLibDefaultViewFactory::viewTypeTitle(const QString& viewTypeIdent) const
{
  if (viewTypeIdent == "default")
    return tr("Category Tree View");
  if (viewTypeIdent == "default+list")
    return tr("List View");
  if (viewTypeIdent == "default+icons")
    return tr("Icon View");

  return QString();
}


KLFAbstractLibView * KLFLibDefaultViewFactory::createLibView(const QString& viewTypeIdent,
							     QWidget *parent,
							     KLFLibResourceEngine *resourceEngine)
{
  KLFLibDefaultView::ViewType v = KLFLibDefaultView::CategoryTreeView;
  if (viewTypeIdent == "default")
    v = KLFLibDefaultView::CategoryTreeView;
  else if (viewTypeIdent == "default+list")
    v = KLFLibDefaultView::ListTreeView;
  else if (viewTypeIdent == "default+icons")
    v = KLFLibDefaultView::IconView;

  KLFLibDefaultView *view = new KLFLibDefaultView(parent, v);
  view->setResourceEngine(resourceEngine);
  return view;
}



// ------------


KLFLibOpenResourceDlg::KLFLibOpenResourceDlg(const QUrl& defaultlocation, QWidget *parent)
  : QDialog(parent)
{
  pUi = new Ui::KLFLibOpenResourceDlg;
  pUi->setupUi(this);

  // add a widget for all supported widgets
  QStringList wtypeList = KLFLibWidgetFactory::allSupportedWTypes();
  int k;
  for (k = 0; k < wtypeList.size(); ++k) {
    qDebug("KLFLibOpenRes.Dlg::[constr.]() Adding widget for wtype %s", qPrintable(wtypeList[k]));
    KLFLibWidgetFactory *factory
      = KLFLibWidgetFactory::findFactoryFor(wtypeList[k]);
    QWidget *openWidget = factory->createPromptUrlWidget(pUi->stkOpenWidgets, wtypeList[k],
							 defaultlocation);
    pUi->stkOpenWidgets->insertWidget(k, openWidget);
    pUi->cbxType->insertItem(k, factory->widgetTypeTitle(wtypeList[k]),
			     QVariant::fromValue(wtypeList[k]));

    connect(openWidget, SIGNAL(readyToOpen(bool)), this, SLOT(updateReadyToOpenFromSender(bool)));
  }

  /** \todo .... BUG/TODO ........ only scheme part of defaultlocation URL is considered. */

  KLFLibEngineFactory *efactory = KLFLibEngineFactory::findFactoryFor(defaultlocation.scheme());
  QString defaultwtype;
  if (efactory == NULL) {
    if (!defaultlocation.isEmpty())
      qWarning()<<"No Factory for URL "<<defaultlocation<<"'s scheme!";
  } else {
    defaultwtype = efactory->correspondingWidgetType(defaultlocation.scheme());
  }
  if (defaultwtype.isEmpty()) {
    pUi->cbxType->setCurrentIndex(0);
    pUi->stkOpenWidgets->setCurrentIndex(0);
  } else {
    pUi->cbxType->setCurrentIndex(k = wtypeList.indexOf(defaultwtype));
    pUi->stkOpenWidgets->setCurrentIndex(k);
  }

  btnGo = pUi->btnBar->button(QDialogButtonBox::Open);

  connect(pUi->cbxType, SIGNAL(activated(int)), this, SLOT(updateReadyToOpen()));
  updateReadyToOpen();
}

KLFLibOpenResourceDlg::~KLFLibOpenResourceDlg()
{
  delete pUi;
}

QUrl KLFLibOpenResourceDlg::rawUrl() const
{
  int k = pUi->cbxType->currentIndex();
  QString wtype = pUi->cbxType->itemData(k).toString();
  KLFLibWidgetFactory *factory
    = KLFLibWidgetFactory::findFactoryFor(wtype);
  return factory->retrieveUrlFromWidget(wtype, pUi->stkOpenWidgets->widget(k));
}

QUrl KLFLibOpenResourceDlg::url() const
{
  QUrl url = rawUrl();
  if (url.isEmpty()) {
    // empty url means cancel open
    return QUrl();
  }
  if (pUi->chkReadOnly->isChecked())
    url.addQueryItem("klfReadOnly", "true");
  if (pUi->cbxSubResource->count())
    url.addQueryItem("klfDefaultSubResource",
		     pUi->cbxSubResource->itemData(pUi->cbxSubResource->currentIndex()).toString());
  klfDbg( "Got URL: "<<url ) ;
  return url;
}

void KLFLibOpenResourceDlg::updateReadyToOpenFromSender(bool isready)
{
  QObject *w = sender();
  // use the widget itself to store the value
  w->setProperty("__klflibopenresourcedlg_readyToOpen", isready);
  updateReadyToOpen();
}
void KLFLibOpenResourceDlg::updateReadyToOpen()
{
  QWidget *w = pUi->stkOpenWidgets->currentWidget();
  if (w == NULL)
    return;
  QVariant v = w->property("__klflibopenresourcedlg_readyToOpen");
  // and update button enabled
  bool haveValidLocation = !v.isValid() || v.toBool();
  btnGo->setEnabled(haveValidLocation);
  // and propose choice of sub-resources
  pUi->cbxSubResource->setEnabled(true);
  pUi->cbxSubResource->clear();
  if (haveValidLocation) {
    QMap<QString,QString> subResMap = KLFLibEngineFactory::listSubResourcesWithTitles(rawUrl());
    if (subResMap.isEmpty()) {
      pUi->cbxSubResource->setEnabled(false);
    } else {
      for (QMap<QString,QString>::const_iterator it = subResMap.begin(); it != subResMap.end(); ++it) {
	QString subres = it.key();
	QString subrestitle = it.value();
	if (subrestitle.isEmpty())
	  subrestitle = subres;
	pUi->cbxSubResource->addItem(subrestitle, QVariant(subres));
      }
    }
  } else {
    pUi->cbxSubResource->setEnabled(false);
  }
}

// static
QUrl KLFLibOpenResourceDlg::queryOpenResource(const QUrl& defaultlocation, QWidget *parent)
{
  KLFLibOpenResourceDlg dlg(defaultlocation, parent);
  int result = dlg.exec();
  if (result != QDialog::Accepted)
    return QUrl();
  QUrl url = dlg.url();
  return url;
}



// ---------------------


KLFLibCreateResourceDlg::KLFLibCreateResourceDlg(const QString& defaultWtype, QWidget *parent)
  : QDialog(parent)
{
  pUi = new Ui::KLFLibOpenResourceDlg;
  pUi->setupUi(this);

  pUi->lblMain->setText(tr("Create New Library Resource", "[[dialog label title]]"));
  setWindowTitle(tr("Create New Library Resource", "[[dialog window title]]"));
  pUi->chkReadOnly->hide();

  pUi->cbxSubResource->setEnabled(true);
  pUi->cbxSubResource->setEditable(true);
  pUi->cbxSubResource->setEditText(tr("SubResource1"));

  pUi->btnBar->setStandardButtons(QDialogButtonBox::Save|QDialogButtonBox::Cancel);
  btnGo = pUi->btnBar->button(QDialogButtonBox::Save);

  int defaultIndex = 0;
  // add a widget for all supported widget-types
  QStringList wtypeList = KLFLibWidgetFactory::allSupportedWTypes();
  int k;
  for (k = 0; k < wtypeList.size(); ++k) {
    KLFLibWidgetFactory *factory
      = KLFLibWidgetFactory::findFactoryFor(wtypeList[k]);
    QWidget *createResWidget =
      factory->createPromptCreateParametersWidget(pUi->stkOpenWidgets, wtypeList[k],
						  Parameters());
    pUi->stkOpenWidgets->insertWidget(k, createResWidget);
    pUi->cbxType->insertItem(k, factory->widgetTypeTitle(wtypeList[k]),
			     QVariant::fromValue(wtypeList[k]));

    if (wtypeList[k] == defaultWtype)
      defaultIndex = k;

    connect(createResWidget, SIGNAL(readyToCreate(bool)),
	    this, SLOT(updateReadyToCreateFromSender(bool)));
  }

  pUi->cbxType->setCurrentIndex(defaultIndex);
  pUi->stkOpenWidgets->setCurrentIndex(defaultIndex);

  connect(pUi->cbxType, SIGNAL(activated(int)), this, SLOT(updateReadyToCreate()));
  updateReadyToCreate();
}
KLFLibCreateResourceDlg::~KLFLibCreateResourceDlg()
{
  delete pUi;
}

KLFLibEngineFactory::Parameters KLFLibCreateResourceDlg::getCreateParameters() const
{
  int k = pUi->cbxType->currentIndex();
  QString wtype = pUi->cbxType->itemData(k).toString();
  KLFLibWidgetFactory *factory
    = KLFLibWidgetFactory::findFactoryFor(wtype);
  Parameters p = factory->retrieveCreateParametersFromWidget(wtype, pUi->stkOpenWidgets->widget(k));
  p["klfWidgetType"] = wtype;
  p["klfDefaultSubResource"] = pUi->cbxSubResource->currentText();
  return p;
}


void KLFLibCreateResourceDlg::accept()
{
  const Parameters p = getCreateParameters();
  if (p == Parameters() || p["klfCancel"].toBool() == true) {
    QDialog::reject();
    return;
  }
  if (p["klfRetry"].toBool() == true)
    return; // ignore accept, reprompt user

  // then by default accept the event
  // set internal parameter cache to the given parameters, they
  // will be recycled by createResource() instead of being re-queried
  pParam = p;
  QDialog::accept();
}
void KLFLibCreateResourceDlg::reject()
{
  QDialog::reject();
}
 
void KLFLibCreateResourceDlg::updateReadyToCreateFromSender(bool isready)
{
  QObject *w = sender();
  w->setProperty("__klflibcreateresourcedlg_readyToCreate", isready);
  updateReadyToCreate();
}
void KLFLibCreateResourceDlg::updateReadyToCreate()
{
  QWidget *w = pUi->stkOpenWidgets->currentWidget();
  if (w == NULL) return;
  QVariant v = w->property("__klflibcreateresourcedlg_readyToCreate");
  // and update button enabled
  btnGo->setEnabled(!v.isValid() || v.toBool());
}

// static
KLFLibResourceEngine *KLFLibCreateResourceDlg::createResource(const QString& defaultWtype,
							      QObject *resourceParent, QWidget *parent)
{
  KLFLibCreateResourceDlg dlg(defaultWtype, parent);
  int result = dlg.exec();
  if (result != QDialog::Accepted)
    return NULL;

  Parameters p = dlg.pParam; // we have access to this private member
  QString scheme = p["klfScheme"].toString();

  if (scheme.isEmpty()) {
    qWarning()<<"KLFLibCr.Res.Dlg::createRes.(): Widget Type "<<p["klfWidgetType"]
	      <<" failed to announce what scheme it wanted in p[\"klfScheme\"]!";
    return NULL;
  }

  KLFLibEngineFactory * factory = KLFLibEngineFactory::findFactoryFor(scheme);
  KLF_ASSERT_NOT_NULL( factory ,
		       qPrintable(QString("Couldn't find factory for scheme %1 ?!?").arg(scheme)),
		       return NULL )
    ;

  KLFLibResourceEngine *resource = factory->createResource(scheme, p, resourceParent);
  return resource;
}


// ---------------------


KLFLibResPropEditor::KLFLibResPropEditor(KLFLibResourceEngine *res, QWidget *parent)
  : QWidget(parent)
{
  U = new Ui::KLFLibResPropEditor;
  U->setupUi(this);

  QPalette pal = U->txtUrl->palette();
  pal.setColor(QPalette::Base, pal.color(QPalette::Window));
  pal.setColor(QPalette::Background, pal.color(QPalette::Window));
  U->txtUrl->setPalette(pal);

  if (res == NULL)
    qWarning("KLFLibResPropEditor: NULL Resource! Expect CRASH!");

  pResource = res;

  pSuppSubRes =
    (pResource->supportedFeatureFlags() & KLFLibResourceEngine::FeatureSubResources);
  pSuppSubResProps =
    (pResource->supportedFeatureFlags() & KLFLibResourceEngine::FeatureSubResourceProps) ;

  connect(pResource, SIGNAL(resourcePropertyChanged(int)),
	  this, SLOT(slotResourcePropertyChanged(int)));
  connect(pResource, SIGNAL(subResourcePropertyChanged(const QString&, int)),
	  this, SLOT(slotSubResourcePropertyChanged(const QString&, int)));

  pPropModel = new QStandardItemModel(this);
  pPropModel->setColumnCount(2);
  pPropModel->setHorizontalHeaderLabels(QStringList() << tr("Name") << tr("Value"));
  U->tblProperties->setModel(pPropModel);
  U->tblProperties->setItemDelegate(new QItemDelegate(this));

  connect(pPropModel, SIGNAL(itemChanged(QStandardItem *)),
	  this, SLOT(advPropEdited(QStandardItem *)));

  pSubResPropModel = new QStandardItemModel(this);
  pSubResPropModel->setColumnCount(2);
  pSubResPropModel->setHorizontalHeaderLabels(QStringList() << tr("Name") << tr("Value"));
  U->tblSubResProperties->setModel(pSubResPropModel);
  U->tblSubResProperties->setItemDelegate(new QItemDelegate(this));

  connect(pSubResPropModel, SIGNAL(itemChanged(QStandardItem *)),
	  this, SLOT(advSubResPropEdited(QStandardItem *)));

  U->frmAdvanced->setShown(U->btnAdvanced->isChecked());

  // perform full refresh (resource properties)
  updateResourceProperties();
  // preform full refresh (sub-resources)
  updateSubResources(pResource->defaultSubResource());
  // perform full refresh (sub-resource properties)
  updateSubResourceProperties();

  connect(U->btnApply, SIGNAL(clicked()), this, SLOT(apply()));
}

KLFLibResPropEditor::~KLFLibResPropEditor()
{
  delete U;
}

bool KLFLibResPropEditor::apply()
{
  bool res = true;

  // set the default sub-resource
  if (pResource->supportedFeatureFlags() & KLFLibResourceEngine::FeatureSubResources)
    pResource->setDefaultSubResource(curSubResource());

  bool srislocked = false;
  if (pSuppSubRes && pSuppSubResProps)
    srislocked = pResource->subResourceProperty(curSubResource(),
						KLFLibResourceEngine::SubResPropLocked).toBool();

  bool lockmodified = (pResource->locked() != U->chkLocked->isChecked());
  bool srlockmodified = false;
  if (pSuppSubRes && pSuppSubResProps && srislocked != U->chkSubResLocked->isChecked()) {
    srlockmodified = true;
  }
  bool wantunlock = lockmodified && !U->chkLocked->isChecked();
  bool srwantunlock = srlockmodified && !U->chkSubResLocked->isChecked();
  bool wantlock = lockmodified && !wantunlock;
  bool srwantlock = srlockmodified && !srwantunlock;
  bool titlemodified = (pResource->title() != U->txtTitle->text());
  bool subrestitlemodified = false;
  if (pSuppSubRes && pSuppSubResProps &&
      pResource->subResourceProperty(curSubResource(), KLFLibResourceEngine::SubResPropTitle).toString()
      != U->txtSubResTitle->text()) {
    subrestitlemodified = true;
  }

  klfDbg( ": lockmodif="<<lockmodified<<"; srlockmodified="<<srlockmodified
	  <<"; wantunlock="<<wantunlock<<"; srwantunlock="<<srwantunlock<<"; wantlock="<<wantlock
	  <<"; srwantlock="<<srwantlock<<"; titlemodif="<<titlemodified
	  <<"; srtitlemodif="<<subrestitlemodified ) ;

  if ( (pResource->locked() && !lockmodified) ||
       (srislocked && !srlockmodified) ) {
    // no intent to modify locked state of locked resource
    if (titlemodified || subrestitlemodified) {
      QMessageBox::critical(this, tr("Error"), tr("Can't rename a locked resource!"));
      return false;
    } else {
      return true; // nothing to apply
    }
  }
  if (wantunlock) {
    if ( ! pResource->setLocked(false) ) { // unlock resource before other modifications
      res = false;
      QMessageBox::critical(this, tr("Error"), tr("Failed to unlock resource."));
    }
  }
  if (srwantunlock) {
    if ( ! pResource->setSubResourceProperty(curSubResource(), KLFLibResourceEngine::SubResPropLocked,
					     false) ) { // unlock sub-resource before other modifications
      res = false;
      QMessageBox::critical(this, tr("Error"), tr("Failed to unlock sub-resource \"%1\".")
			    .arg(curSubResource()));
    }
  }

  QString newTitle = U->txtTitle->text();
  QString newSubResTitle = U->txtSubResTitle->text();

  if ( titlemodified && ! pResource->setTitle(newTitle) ) {
    res = false;
    QMessageBox::critical(this, tr("Error"), tr("Failed to rename resource."));
  }

  if ( subrestitlemodified &&
       ! pResource->setSubResourceProperty(curSubResource(),
					   KLFLibResourceEngine::SubResPropTitle,
					   newSubResTitle) ) {
    res = false;
    QMessageBox::critical(this, tr("Error"), tr("Failed to rename sub-resource \"%1\".")
			  .arg(curSubResource()));
  }

  if (wantlock) {
    if ( ! pResource->setLocked(true) ) { // lock resource after other modifications
      res = false;
      QMessageBox::critical(this, tr("Error"), tr("Failed to lock resource."));
    }
  }
  if (srwantlock) {
    if ( ! pResource->setSubResourceProperty(curSubResource(), KLFLibResourceEngine::SubResPropLocked,
					     true) ) { // lock resource after other modifications
      res = false;
      QMessageBox::critical(this, tr("Error"), tr("Failed to lock sub-resource \"%1\".")
			    .arg(curSubResource()));
    }
  }

  return res;
}

void KLFLibResPropEditor::on_btnAdvanced_toggled(bool on)
{
  // show/hide advanced controls
  U->frmAdvanced->setShown(on);
  if (U->frmAdvanced->isVisible()) {
    // adjust size of columns
    int w = width() / 3;
    U->tblProperties->setColumnWidth(0, w);
    U->tblProperties->setColumnWidth(1, w);
    U->tblSubResProperties->setColumnWidth(0, w);
    U->tblSubResProperties->setColumnWidth(1, w);
  }
  update();
  adjustSize();
  if (parentWidget()) {
    klfDbg( "Parent widget is "<<parentWidget() ) ;
    parentWidget()->update();
    parentWidget()->adjustSize();
  }
}

void KLFLibResPropEditor::updateSubResources(const QString& curSubRes)
{
  klfDbg( "KLFLibResPropEditor::updateSubResources("<<curSubRes<<")" ) ;
  if ( pSuppSubRes ) {
    U->cbxSubResource->blockSignals(true);
    U->cbxSubResource->clear();
    QStringList subResList = pResource->subResourceList();
    int k;
    int curSubResIndex = 0;
    for (k = 0; k < subResList.size(); ++k) {
      QString title
	= pResource->subResourceProperty(subResList[k], KLFLibResourceEngine::SubResPropTitle).toString();
      if (title.isEmpty())
	title = subResList[k];
      U->cbxSubResource->addItem(title, subResList[k]);
      if (subResList[k] == curSubRes)
	curSubResIndex = k;
    }
    klfDbg( "KLFLibResPropEditor::updateSubResources("<<curSubRes<<") : setting cur index="<<curSubResIndex ) ;
    U->cbxSubResource->setCurrentIndex(curSubResIndex);
    U->cbxSubResource->blockSignals(false);
    if ( pSuppSubResProps ) {
      U->wSubResProps->show();
      U->txtSubResTitle->setEnabled(true);
      U->chkSubResLocked->setEnabled(true);
      slotSubResourcePropertyChanged(curSubResource(), -2);
      U->txtSubResTitle->setEnabled(pResource
				    ->canModifySubResourceProperty(curSubResource(),
								   KLFLibResourceEngine::SubResPropTitle));
      U->txtSubResTitle->setText(pResource->subResourceProperty(curSubResource(),
								KLFLibResourceEngine::SubResPropTitle)
				 .toString());
      U->chkSubResLocked->setEnabled(pResource
				     ->canModifySubResourceProperty(curSubResource(),
								    KLFLibResourceEngine::SubResPropLocked));
      U->chkSubResLocked->setChecked(pResource->subResourceProperty(curSubResource(),
								    KLFLibResourceEngine::SubResPropLocked)
				     .toBool());
    } else {
      U->wSubResProps->hide();
      U->chkSubResLocked->setEnabled(false);
      U->txtSubResTitle->setText("");
      U->txtSubResTitle->setEnabled(false);
    }
  } else {
    U->cbxSubResource->clear();
    U->cbxSubResource->setEnabled(false);
    U->wSubResProps->hide();
    U->chkSubResLocked->setEnabled(false);
    U->txtSubResTitle->setText("");
    U->txtSubResTitle->setEnabled(false);
  }
}

void KLFLibResPropEditor::advPropEdited(QStandardItem *item)
{
  klfDbg( "advPropEdited("<<item<<")" ) ;
  QVariant value = item->data(Qt::EditRole);
  int propId = item->data(Qt::UserRole).toInt();
  bool r = pResource->setResourceProperty(propId, value);
  if ( ! r ) {
    QMessageBox::critical(this, tr("Error"),
			  tr("Failed to set resource property \"%1\".")
			  .arg(pResource->propertyNameForId(propId)));
  }
  // slotResourcePropertyChanged will be called.
}

void KLFLibResPropEditor::slotResourcePropertyChanged(int /*propId*/)
{
  // perform full update
  updateResourceProperties();
  updateSubResources();
}
void KLFLibResPropEditor::updateResourceProperties()
{
  pPropModel->setRowCount(0);
  int k;
  QStringList props = pResource->registeredPropertyNameList();
  QPalette pal = U->tblSubResProperties->palette();
  for (k = 0; k < props.size(); ++k) {
    QString prop = props[k];
    int propId = pResource->propertyIdForName(prop);
    QVariant val = pResource->resourceProperty(prop);
    QStandardItem *i1 = new QStandardItem(prop);
    i1->setEditable(false);
    QStandardItem *i2 = new QStandardItem(val.toString());
    bool editable = pResource->canModifyProp(propId);
    i2->setEditable(editable);
    QPalette::ColorGroup cg = editable ? QPalette::Active : QPalette::Disabled;
    i2->setForeground(pal.brush(cg, QPalette::Text));
    i2->setBackground(pal.brush(cg, QPalette::Base));
    i2->setData(val, Qt::EditRole);
    i2->setData(propId, Qt::UserRole); // user data is property ID
    pPropModel->appendRow(QList<QStandardItem*>() << i1 << i2);
  }

  U->txtTitle->setText(pResource->title());
  U->txtTitle->setEnabled(pResource->canModifyProp(KLFLibResourceEngine::PropTitle));
  QString surl = pResource->url().toString();
  if (surl.endsWith("?"))
    surl.chop(1);
  U->txtUrl->setText(surl);
  U->chkLocked->setChecked(pResource->locked());
  U->chkLocked->setEnabled(pResource->canModifyProp(KLFLibResourceEngine::PropLocked));
}

QString KLFLibResPropEditor::curSubResource() const
{
  return U->cbxSubResource->itemData(U->cbxSubResource->currentIndex()).toString();
}

void KLFLibResPropEditor::advSubResPropEdited(QStandardItem *item)
{
  klfDbg( "item="<<item<<"" ) ;
  QVariant value = item->data(Qt::EditRole);
  int propId = item->data(Qt::UserRole).toInt();
  bool r = pResource->setSubResourceProperty(curSubResource(), propId, value);
  if ( ! r ) {
    QMessageBox::critical(this, tr("Error"),
			  tr("Failed to set sub-resource \"%1\"'s property \"%2\".")
			  .arg(curSubResource(), propId));
  }
  // slotSubResourcePropertyChanged will be called.
}
void KLFLibResPropEditor::on_cbxSubResource_currentIndexChanged(int newSubResItemIndex)
{
  slotSubResourcePropertyChanged(curSubResource(), -2);
  U->txtSubResTitle->setText(pResource->subResourceProperty(curSubResource(),
							    KLFLibResourceEngine::SubResPropTitle)
			     .toString());
  U->chkSubResLocked->setChecked(pResource->subResourceProperty(curSubResource(),
								KLFLibResourceEngine::SubResPropLocked)
				 .toBool());
}

void KLFLibResPropEditor::slotSubResourcePropertyChanged(const QString& subResource, int subResPropId)
{
  if (subResource != curSubResource())
    return;

  if (subResPropId != -2) {
    // REALLY full refresh
    updateSubResources(curSubResource());
    // will call this function again, with subResPropId==-2
    // so return here.
    return;
  }

  if ( ! pSuppSubResProps )
    return;

  // perform full update of sub-res prop list
  updateSubResourceProperties();
}
void KLFLibResPropEditor::updateSubResourceProperties()
{
  // full update of model data
  pSubResPropModel->setRowCount(0);
  QPalette pal = U->tblSubResProperties->palette();
  int k;
  QList<int> props = pResource->subResourcePropertyIdList();
  for (k = 0; k < props.size(); ++k) {
    int propId = props[k];
    QVariant val = pResource->subResourceProperty(curSubResource(), propId);
    QStandardItem *i1 = new QStandardItem(pResource->subResourcePropertyName(propId));
    i1->setEditable(false);
    QStandardItem *i2 = new QStandardItem(val.toString());
    bool editable = pResource->canModifySubResourceProperty(curSubResource(), propId);
    i2->setEditable(editable);
    QPalette::ColorGroup cg = editable ? QPalette::Active : QPalette::Disabled;
    i2->setForeground(pal.brush(cg, QPalette::Text));
    i2->setBackground(pal.brush(cg, QPalette::Base));
    i2->setData(val, Qt::EditRole);
    i2->setData(propId, Qt::UserRole); // user data is property ID
    pSubResPropModel->appendRow(QList<QStandardItem*>() << i1 << i2);
  }
}





// -------------


KLFLibResPropEditorDlg::KLFLibResPropEditorDlg(KLFLibResourceEngine *resource, QWidget *parent)
  : QDialog(parent)
{
  QVBoxLayout *lyt = new QVBoxLayout(this);

  pEditor = new KLFLibResPropEditor(resource, this);
  lyt->addWidget(pEditor);

  QDialogButtonBox *btns = new QDialogButtonBox(QDialogButtonBox::Ok|QDialogButtonBox::Cancel,
						Qt::Horizontal, this);
  lyt->addWidget(btns);

  connect(btns->button(QDialogButtonBox::Ok), SIGNAL(clicked()),
	  this, SLOT(applyAndClose()));
  //  connect(btns->button(QDialogButtonBox::Apply), SIGNAL(clicked()),
  //	  pEditor, SLOT(apply()));
  connect(btns->button(QDialogButtonBox::Cancel), SIGNAL(clicked()),
	  this, SLOT(cancelAndClose()));
  
  setModal(false);
  setWindowTitle(pEditor->windowTitle());
  setAttribute(Qt::WA_DeleteOnClose, true);
}

KLFLibResPropEditorDlg::~KLFLibResPropEditorDlg()
{
}

void KLFLibResPropEditorDlg::applyAndClose()
{
  if (pEditor->apply()) {
    accept();
  }
}

void KLFLibResPropEditorDlg::cancelAndClose()
{
  reject();
}


// ---------------------------------------------------------------

KLFLibNewSubResDlg::KLFLibNewSubResDlg(KLFLibResourceEngine *resource, QWidget *parent)
  : QDialog(parent), isAutoName(true)
{
  u = new Ui::KLFLibNewSubResDlg;
  u->setupUi(this);

  u->lblNoTitle->hide();

  uint fl = resource->supportedFeatureFlags();
  if ( (fl & KLFLibResourceEngine::FeatureSubResources) == 0 ) {
    u->lblName->setEnabled(false);
    u->txtName->setEnabled(false);
    u->lblTitle->setEnabled(false);
    u->txtTitle->setEnabled(false);
    u->btns->button(QDialogButtonBox::Ok)->setEnabled(false);
  } else if ( (fl & KLFLibResourceEngine::FeatureSubResourceProps) == 0) {
    u->lblTitle->setEnabled(false);
    u->txtTitle->setEnabled(false);
    u->lblNoTitle->show();
  }

  u->lblResource->setText(resource->title());
}

KLFLibNewSubResDlg::~KLFLibNewSubResDlg()
{
}

QString KLFLibNewSubResDlg::newSubResourceName() const
{
  return u->txtName->text();
}

QString KLFLibNewSubResDlg::newSubResourceTitle() const
{
  return u->txtTitle->text();
}

void KLFLibNewSubResDlg::on_txtTitle_textChanged(const QString& text)
{
  if (isAutoName) {
    QString nm = text;
    // replace "string of words" into "stringOfWords"
    QRegExp rx("(\\s|-)[a-z]");
    int i;
    while ((i = rx.indexIn(nm,i+1)) >= 0) {
      nm.replace(i, 2, nm[i+1].toUpper());
      --i; // effectively removed one char, compensate
    }
    nm.replace(QRegExp("\\s"), "");
    nm.replace(QRegExp("[^A-Za-z0-9_]"), "_");
    u->txtName->blockSignals(true);
    u->txtName->setText(nm);
    u->txtName->blockSignals(false);
  }
}

void KLFLibNewSubResDlg::on_txtName_textChanged(const QString& text)
{
  if (!text.isEmpty())
    isAutoName = false; // manually set name
  else
    isAutoName = true; // user erased name, so auto-name again
}

QString KLFLibNewSubResDlg::createSubResourceIn(KLFLibResourceEngine *resource, QWidget *parent)
{
  if ( (resource->supportedFeatureFlags() & KLFLibResourceEngine::FeatureSubResources) == 0 ) {
    qWarning("KLFLibNewSubResDlg::createSubResourceIn: can't create sub-resource in resource not "
	     "supporting sub-resources (!) : %s", qPrintable(resource->url().toString()));
    return QString();
  }
  KLFLibNewSubResDlg d(resource, parent);
  int r = d.exec();
  if (r != QDialog::Accepted)
    return QString();
  QString name = d.newSubResourceName();
  QString title = d.newSubResourceTitle();

  bool result = resource->createSubResource(name, title);
  if (!result)
    return QString();

  return name;
}

// ---------------------------------------------------------------

KLFLibLocalFileSchemeGuesser::KLFLibLocalFileSchemeGuesser()
{
  KLFLibBasicWidgetFactory::addLocalFileSchemeGuesser(this);
}
KLFLibLocalFileSchemeGuesser::~KLFLibLocalFileSchemeGuesser()
{
  KLFLibBasicWidgetFactory::removeLocalFileSchemeGuesser(this);
}

// ---------------------------------------------------------------

KLFLibBasicWidgetFactory::KLFLibBasicWidgetFactory(QObject *parent)
  :  KLFLibWidgetFactory(parent)
{
}
KLFLibBasicWidgetFactory::~KLFLibBasicWidgetFactory()
{
}


QStringList KLFLibBasicWidgetFactory::supportedTypes() const
{
  return QStringList() << QLatin1String("LocalFile");
}


QString KLFLibBasicWidgetFactory::widgetTypeTitle(const QString& wtype) const
{
  if (wtype == QLatin1String("LocalFile"))
    return tr("Local File");
  return QString();
}



QWidget * KLFLibBasicWidgetFactory::createPromptUrlWidget(QWidget *parent, const QString& wtype,
							  QUrl defaultlocation)
{
  if (wtype == QLatin1String("LocalFile")) {
    KLFLibLocalFileOpenWidget *w = new KLFLibLocalFileOpenWidget(parent, pLocalFileTypes);
    w->setUrl(defaultlocation);
    return w;
  }
  return NULL;
}

QUrl KLFLibBasicWidgetFactory::retrieveUrlFromWidget(const QString& wtype, QWidget *widget)
{
  if (wtype == "LocalFile") {
    if (widget == NULL || !widget->inherits("KLFLibLocalFileOpenWidget")) {
      qWarning("KLFLibBasicWidgetFactory::retrieveUrlFromWidget(): Bad Widget provided!");
      return QUrl();
    }
    return qobject_cast<KLFLibLocalFileOpenWidget*>(widget)->url();
  } else {
    qWarning()<<"KLFLibB.W.Factory::retrieveUrlFromWidget(): Bad widget type: "<<wtype;
    return QUrl();
  }
}


QWidget *KLFLibBasicWidgetFactory::createPromptCreateParametersWidget(QWidget *parent,
								      const QString& wtype,
								      const Parameters& defaultparameters)
{
  if (wtype == QLatin1String("LocalFile")) {
    KLFLibLocalFileCreateWidget *w = new KLFLibLocalFileCreateWidget(parent, pLocalFileTypes);
    w->setUrl(defaultparameters["Url"].toUrl());
    return w;
  }
  return NULL;
}

KLFLibWidgetFactory::Parameters
/* */ KLFLibBasicWidgetFactory::retrieveCreateParametersFromWidget(const QString& scheme,
								   QWidget *widget)
{
  if (scheme == QLatin1String("LocalFile")) {
    if (widget == NULL || !widget->inherits("KLFLibLocalFileCreateWidget")) {
      qWarning("KLFLibBasicWidgetFactory::retrieveUrlFromWidget(): Bad Widget provided!");
      return Parameters();
    }
    KLFLibLocalFileCreateWidget *w = qobject_cast<KLFLibLocalFileCreateWidget*>(widget);
    Parameters p;
    QString filename = w->selectedFName();
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
      // remove the previous file, because otherwise KLFLib*Engine may fail on existing files.
      /** \bug DEBUG/TODO ........... remove comment next line for debugging safety blocking
       *    \ref QFile::remove() ...... */
      bool r = false; //bool r = QFile::remove(filename); DEBUG SAFETY
      if ( !r ) {
	QMessageBox::critical(widget, tr("Error"), tr("Failed to overwrite the file %1.")
			      .arg(filename));
	return Parameters();
      }
    }

    p["Filename"] = filename;
    p["klfScheme"] = w->selectedScheme();
    return p;
  }

  return Parameters();
}

// static
void KLFLibBasicWidgetFactory::addLocalFileType(const LocalFileType& f)
{
  pLocalFileTypes << f;
}

// static
QList<KLFLibBasicWidgetFactory::LocalFileType> KLFLibBasicWidgetFactory::localFileTypes()
{
  return pLocalFileTypes;
}


// static
QString KLFLibBasicWidgetFactory::guessLocalFileScheme(const QString& fileName)
{
  int k;
  for (k = 0; k < pSchemeGuessers.size(); ++k) {
    QString s = pSchemeGuessers[k]->guessScheme(fileName);
    if (!s.isEmpty())
      return s;
  }
  return QString();
}

// static
void KLFLibBasicWidgetFactory::addLocalFileSchemeGuesser(KLFLibLocalFileSchemeGuesser *schemeguesser)
{
  pSchemeGuessers << schemeguesser;
}

// static
void KLFLibBasicWidgetFactory::removeLocalFileSchemeGuesser(KLFLibLocalFileSchemeGuesser *schemeguesser)
{
  pSchemeGuessers.removeAll(schemeguesser);
}



// static
QList<KLFLibBasicWidgetFactory::LocalFileType> KLFLibBasicWidgetFactory::pLocalFileTypes ;
// static
QList<KLFLibLocalFileSchemeGuesser*> KLFLibBasicWidgetFactory::pSchemeGuessers ;

