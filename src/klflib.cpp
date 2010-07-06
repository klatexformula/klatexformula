/***************************************************************************
 *   file klflib.cpp
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
#include <QByteArray>
#include <QDataStream>
#include <QColor>
#include <QMimeData>

#include "klflib_p.h"
#include "klflib.h"





// issue a warning if no default sub-resource is set
#define KLFLIBRESOURCEENGINE_WARN_NO_DEFAULT_SUBRESOURCE(func)		\
  if ((pFeatureFlags & FeatureSubResources) && pDefaultSubResource.isNull()) { \
    qWarning("KLFLibResourceEngine::" func "(id): sub-resources are supported feature but" \
	     " no default sub-resource is specified!"); }		\




// ----------------------


KLF_EXPORT QDataStream& operator<<(QDataStream& stream, const KLFStyle& style)
{
  return stream << style.name << (quint32)style.fg_color << (quint32)style.bg_color
		<< style.mathmode << style.preamble << (quint16)style.dpi;
}
KLF_EXPORT QDataStream& operator>>(QDataStream& stream, KLFStyle& style)
{
  quint32 fg, bg;
  quint16 dpi;
  stream >> style.name;
  stream >> fg >> bg >> style.mathmode >> style.preamble >> dpi;
  style.fg_color = fg;
  style.bg_color = bg;
  style.dpi = dpi;
  return stream;
}

bool operator==(const KLFStyle& a, const KLFStyle& b)
{
  return a.name == b.name &&
    a.fg_color == b.fg_color &&
    a.bg_color == b.bg_color &&
    a.mathmode == b.mathmode &&
    a.preamble == b.preamble &&
    a.dpi == b.dpi;
}

KLF_EXPORT QString prettyPrintStyle(const KLFStyle& sty)
{
  QString s = "";
  if (sty.name != QString::null)
    s = QObject::tr("<b>Style Name</b>: %1<br>").arg(sty.name);
  return s + QObject::tr("<b>Math Mode</b>: %1<br>"
			 "<b>DPI Resolution</b>: %2<br>"
			 "<b>Foreground Color</b>: %3 <font color=\"%4\"><b>[SAMPLE]</b></font><br>"
			 "<b>Background is Transparent</b>: %5<br>"
			 "<b>Background Color</b>: %6 <font color=\"%7\"><b>[SAMPLE]</b></font><br>"
			 "<b>LaTeX Preamble:</b><br><pre>%8</pre>")
    .arg(sty.mathmode)
    .arg(sty.dpi)
    .arg(QColor(sty.fg_color).name()).arg(QColor(sty.fg_color).name())
    .arg( (qAlpha(sty.bg_color) != 255) ? QObject::tr("YES") : QObject::tr("NO") )
    .arg(QColor(sty.bg_color).name()).arg(QColor(sty.bg_color).name())
    .arg(sty.preamble)
    ;
}



// ----------------------


KLFLibEntry::KLFLibEntry(const QString& latex, const QDateTime& dt, const QImage& preview,
			 const QSize& previewsize, const QString& category, const QString& tags,
			 const KLFStyle& style)
  : KLFPropertizedObject("KLFLibEntry")
{
  initRegisteredProperties();
  setLatex(latex);
  setDateTime(dt);
  setPreview(preview);
  setPreviewSize(previewsize);
  setCategory(category);
  setTags(tags);
  setStyle(style);
}
KLFLibEntry::KLFLibEntry(const QString& latex, const QDateTime& dt, const QImage& preview,
			 const KLFStyle& style)
  : KLFPropertizedObject("KLFLibEntry")
{
  initRegisteredProperties();
  QString latexonly = stripCategoryTagsFromLatex(latex);
  QString category = categoryFromLatex(latex);
  QString tags = tagsFromLatex(latex);
  QSize previewsize = preview.size();
  setLatex(latexonly);
  setDateTime(dt);
  setPreview(preview);
  setPreviewSize(previewsize);
  setCategory(category);
  setTags(tags);
  setStyle(style);
}
KLFLibEntry::KLFLibEntry(const KLFLibEntry& copy)
  : KLFPropertizedObject("KLFLibEntry")
{
  initRegisteredProperties();
  setAllProperties(copy.allProperties());
}
KLFLibEntry::~KLFLibEntry()
{
}


int KLFLibEntry::setEntryProperty(const QString& propName, const QVariant& value)
{
  int propId = propertyIdForName(propName);
  if (propId < 0) {
    // register the property
    propId = registerProperty(propName);
    if (propId < 0)
      return -1;
  }
  // and set the property
  setProperty(propId, value);
  return propId;
}

// private
void KLFLibEntry::initRegisteredProperties()
{
  KLF_FUNC_SINGLE_RUN ;
  
  registerBuiltInProperty(Latex, "Latex");
  registerBuiltInProperty(DateTime, "DateTime");
  registerBuiltInProperty(Preview, "Preview");
  registerBuiltInProperty(PreviewSize, "PreviewSize");
  registerBuiltInProperty(Category, "Category");
  registerBuiltInProperty(Tags, "Tags");
  registerBuiltInProperty(Style, "Style");
}


// static
QString KLFLibEntry::categoryFromLatex(const QString& latex)
{
  QString s = latex.section('\n', 0, 0, QString::SectionSkipEmpty);
  if (s[0] == '%' && s[1] == ':') {
    return s.mid(2).trimmed();
  }
  return QString::null;
}
// static
QString KLFLibEntry::tagsFromLatex(const QString& latex)
{
  QString s = latex.section('\n', 0, 0, QString::SectionSkipEmpty);
  if (s[0] == '%' && s[1] == ':') {
    // category is s.mid(2);
    s = latex.section('\n', 1, 1, QString::SectionSkipEmpty);
  }
  if (s[0] == '%') {
    return s.mid(1).trimmed();
  }
  return QString::null;
}

// static
QString KLFLibEntry::stripCategoryTagsFromLatex(const QString& latex)
{
  int k = 0;
  while (k < latex.length() && latex[k].isSpace())
    ++k;
  if (k == latex.length()) return "";
  if (latex[k] == '%') {
    ++k;
    if (k == latex.length()) return "";
    //strip category and/or tag:
    if (latex[k] == ':') {
      // strip category
      while (k < latex.length() && latex[k] != '\n')
	++k;
      ++k;
      if (k >= latex.length()) return "";
      if (latex[k] != '%') {
	// there isn't any tags, just category; return rest of string
	return latex.mid(k);
      }
      ++k;
      if (k >= latex.length()) return "";
    }
    // strip tag:
    while (k < latex.length() && latex[k] != '\n')
      ++k;
    ++k;
    if (k >= latex.length()) return "";
  }
  // k is the beginnnig of the latex string
  return latex.mid(k);
}

// static
QString KLFLibEntry::latexAddCategoryTagsComment(const QString& latex, const QString& category,
						 const QString& tags)
{
  QString s;

  if (!category.isEmpty())
    s = "%: "+category+"\n";

  if (!tags.isEmpty())
    s += "% "+tags+"\n";

  s += latex;
  return s;
}


// ------------------------------------------------------------



KLFLibEntrySorter::KLFLibEntrySorter(int propId, Qt::SortOrder order)
  : pCloneOf(NULL), pPropId(propId), pOrder(order)
{
}
KLFLibEntrySorter::KLFLibEntrySorter(const KLFLibEntrySorter *clone)
  : pCloneOf(clone), pPropId(clone->pPropId), pOrder(clone->pOrder)
{
}

KLFLibEntrySorter::~KLFLibEntrySorter()
{
}

void KLFLibEntrySorter::setPropId(int propId)
{
  if (pCloneOf != NULL) {
    qWarning()<<"Attempt to setPropId() in entry sorter that is a clone of "<<pCloneOf;
    return;
  }
  pPropId = propId;
}
void KLFLibEntrySorter::setOrder(Qt::SortOrder order)
{
  if (pCloneOf != NULL) {
    qWarning()<<"Attempt to setOrder() in entry sorter that is a clone of "<<pCloneOf;
    return;
  }
  pOrder = order;
}



QString KLFLibEntrySorter::entryValue(const KLFLibEntry& entry, int propId) const
{
  if (pCloneOf != NULL)
    return pCloneOf->entryValue(entry, propId);

  // return an internal string representation of the value of the property 'propId' in libentry 'entry'

  // user friendliness. sort by date when selecting preview.
  if (propId == KLFLibEntry::Preview)
    propId = KLFLibEntry::DateTime;

  if (propId == KLFLibEntry::PreviewSize) {
    QSize s = entry.previewSize();
    // eg. "0000000280,0000000180" for 280x180
    return QString("%1,%2").arg(s.width(), 10, 10, QChar('0')).arg(s.height(), 10, 10, QChar('0'));
  }
  if (propId == KLFLibEntry::DateTime) {
    return entry.property(KLFLibEntry::DateTime).toDateTime().toString("yyyy-MM-dd+hh:mm:ss.zzz");
  }
  return entry.property(propId).toString();
}

bool KLFLibEntrySorter::compareLessThan(const KLFLibEntry& a, const KLFLibEntry& b,
					int propId, Qt::SortOrder order) const
{
  if (pCloneOf != NULL)
    return pCloneOf->compareLessThan(a, b, propId, order);

  QString as = entryValue(a, propId);
  QString bs = entryValue(b, propId);
  if (order == Qt::AscendingOrder)
    return QString::localeAwareCompare(as, bs) < 0;
  return QString::localeAwareCompare(as, bs) > 0;
}

bool KLFLibEntrySorter::operator()(const KLFLibEntry& a, const KLFLibEntry& b) const
{
  if (pCloneOf != NULL)
    return pCloneOf->operator()(a, b);

  return compareLessThan(a, b, pPropId, pOrder);
}


// ---------------------------------------------------

KLFAbstractLibEntryMimeEncoder::KLFAbstractLibEntryMimeEncoder()
{
  registerEncoder(this);
}
KLFAbstractLibEntryMimeEncoder::~KLFAbstractLibEntryMimeEncoder()
{
}
void KLFAbstractLibEntryMimeEncoder::registerEncoder(KLFAbstractLibEntryMimeEncoder *encoder)
{
  staticEncoderList.append(encoder);
}

QList<KLFAbstractLibEntryMimeEncoder*> KLFAbstractLibEntryMimeEncoder::encoderList()
{
  return staticEncoderList;
}

// static
QMimeData *KLFAbstractLibEntryMimeEncoder::createMimeData(const KLFLibEntryList& entryList,
							  QVariantMap& metaData)
{
  QMimeData *mime = new QMimeData;
  int k, j;
  for (k = 0; k < staticEncoderList.size(); ++k) {
    QStringList mimeTypeList = staticEncoderList[k]->supportedEncodingMimeTypes();
    for (j = 0; j < mimeTypeList.size(); ++j) {
      mime->setData(mimeTypeList[j],
		    staticEncoderList[k]->encodeMime(entryList, metaData, mimeTypeList[j]));
    }
  }
  return mime;
}


// static
bool KLFAbstractLibEntryMimeEncoder::canDecodeMimeData(const QMimeData *mimeData)
{
  QStringList fmts = mimeData->formats();
  int k;
  for (k = 0; k < fmts.size(); ++k) {
    if (findDecoderFor(fmts[k], false) != NULL)
      return true;
  }
  return false;
}

// static
bool KLFAbstractLibEntryMimeEncoder::decodeMimeData(const QMimeData *mimeData,
						    KLFLibEntryList *entryListPtr,
						    QVariantMap *metaDataPtr)
{
  QStringList fmts = mimeData->formats();
  int k;
  for (k = 0; k < fmts.size(); ++k) {
    KLFAbstractLibEntryMimeEncoder *decoder = findDecoderFor(fmts[k], false);
    if (decoder == NULL)
      continue;
    bool result = decoder->decodeMime(mimeData->data(fmts[k]), fmts[k], entryListPtr, metaDataPtr);
    if ( result )
      return true;
    // else continue trying
  }
  return false;
}



KLFAbstractLibEntryMimeEncoder *KLFAbstractLibEntryMimeEncoder::findEncoderFor(const QString& mimeType,
									       bool warn)
{
  int k;
  for (k = 0; k < staticEncoderList.size(); ++k)
    if (staticEncoderList[k]->supportedEncodingMimeTypes().contains(mimeType))
      return staticEncoderList[k];
  if (warn)
    qWarning()<<KLF_FUNC_NAME<<": Failed to find encoder for mime-type "<<mimeType;
  return NULL;
}

KLFAbstractLibEntryMimeEncoder *KLFAbstractLibEntryMimeEncoder::findDecoderFor(const QString& mimeType,
									       bool warn)
{
  int k;
  for (k = 0; k < staticEncoderList.size(); ++k)
    if (staticEncoderList[k]->supportedDecodingMimeTypes().contains(mimeType))
      return staticEncoderList[k];
  if (warn)
    qWarning()<<KLF_FUNC_NAME<<": Failed to find decoder for mime-type "<<mimeType;
  return NULL;
}


QList<KLFAbstractLibEntryMimeEncoder*> KLFAbstractLibEntryMimeEncoder::staticEncoderList;




// The instance of the basic encoder, that will auto-register itself
KLFLibEntryMimeEncoder __klf_lib_mime_encoder;


// ---------------------------------------------------

KLFLibResourceEngine::KLFLibResourceEngine(const QUrl& url, uint featureflags,
					   QObject *parent)
  : QObject(parent), KLFPropertizedObject("KLFLibResourceEngine"), pUrl(url),
    pFeatureFlags(featureflags), pReadOnly(false), pDefaultSubResource(QString())
{
  initRegisteredProperties();

  //  qDebug()<<"KLFLibResourceEngine::KLFLibResourceEngine("<<url<<","<<pFeatureFlags<<","
  //	  <<parent<<")";

  QStringList rdonly = pUrl.allQueryItemValues("klfReadOnly");
  if (rdonly.size() && rdonly.last() == "true") {
    if (pFeatureFlags & FeatureReadOnly)
      pReadOnly = true;
  }
  pUrl.removeAllQueryItems("klfReadOnly");

  if (pFeatureFlags & FeatureSubResources) {
    QStringList defaultsubresource = pUrl.allQueryItemValues("klfDefaultSubResource");
    if (!defaultsubresource.isEmpty()) {
      pUrl.removeAllQueryItems("klfDefaultSubResource");
      pDefaultSubResource = defaultsubresource.last();
    }
  }

}
KLFLibResourceEngine::~KLFLibResourceEngine()
{
}

void KLFLibResourceEngine::initRegisteredProperties()
{
  KLF_FUNC_SINGLE_RUN

  registerBuiltInProperty(PropTitle, "Title");
  registerBuiltInProperty(PropLocked, "Locked");
  registerBuiltInProperty(PropViewType, "ViewType");
  registerBuiltInProperty(PropAccessShared, "AccessShared");
}

QUrl KLFLibResourceEngine::url(uint flags) const
{
  QUrl url = pUrl;
  if (flags & WantUrlDefaultSubResource &&
      (pFeatureFlags & FeatureSubResources) &&
      !pDefaultSubResource.isNull()) {
    url.addQueryItem("klfDefaultSubResource", pDefaultSubResource);
  }
  if (flags & WantUrlReadOnly) {
    url.addQueryItem("klfReadOnly", pReadOnly?QString("true"):QString("false"));
  }
  return url;
}


bool KLFLibResourceEngine::canModifyData(const QString& subResource,
					 ModifyType /*modifytype*/) const
{
  return baseCanModifyStatus(true, subResource) == MS_CanModify;
}

bool KLFLibResourceEngine::canModifyData(ModifyType modifytype) const
{
  KLFLIBRESOURCEENGINE_WARN_NO_DEFAULT_SUBRESOURCE("canModifyData")
  return canModifyData(pDefaultSubResource, modifytype);
}


bool KLFLibResourceEngine::canModifyProp(int propId) const
{
  ModifyStatus ms = baseCanModifyStatus(false);
  return ms == MS_CanModify ||
    (ms == MS_IsLocked && propId == PropLocked); // allow un-locking (!)
}
bool KLFLibResourceEngine::canRegisterProperty(const QString& /*propName*/) const
{
  return false;
}

bool KLFLibResourceEngine::hasSubResource(const QString& subResource) const
{
  if ( !(pFeatureFlags & FeatureSubResources) )
    return false;

  return subResourceList().contains(subResource);
}

QString KLFLibResourceEngine::defaultSubResource()
{
  return pDefaultSubResource;
}

bool KLFLibResourceEngine::canCreateSubResource() const
{
  return false;
}

bool KLFLibResourceEngine::canRenameSubResource(const QString& /*subResource*/) const
{
  return false;
}

QVariant KLFLibResourceEngine::subResourceProperty(const QString& /*subResource*/, int /*propId*/) const
{
  return QVariant();
}

QString KLFLibResourceEngine::subResourcePropertyName(int propId) const
{
  switch (propId) {
  case SubResPropTitle:
    return QLatin1String("Title");
  case SubResPropLocked:
    return QLatin1String("Locked");
  case SubResPropViewType:
    return QLatin1String("ViewType");
  default:
    ;
  }
  return QString::number(propId);
}

bool KLFLibResourceEngine::canModifySubResourceProperty(const QString& subResource, int propId) const
{
  ModifyStatus ms = baseCanModifyStatus(true, subResource);
  return ms == MS_CanModify ||
    (ms == MS_SubResLocked && propId == SubResPropLocked); // allow sub-resource un-locking
}

bool KLFLibResourceEngine::setTitle(const QString& title)
{
  return setResourceProperty(PropTitle, title);
}
bool KLFLibResourceEngine::setLocked(bool setlocked)
{
  // if locked feature is supported, setResourceProperty().
  // immediately return FALSE otherwise.
  if (pFeatureFlags & FeatureLocked) {
    return setResourceProperty(PropLocked, setlocked);
  }
  return false;
}

bool KLFLibResourceEngine::setViewType(const QString& viewType)
{
  return setResourceProperty(PropViewType, viewType);
}

bool KLFLibResourceEngine::setReadOnly(bool readonly)
{
  if ( !(pFeatureFlags & FeatureReadOnly) )
    return false;

  pReadOnly = readonly;
  return true;
}


void KLFLibResourceEngine::setDefaultSubResource(const QString& subResource)
{
  if (pDefaultSubResource == subResource)
    return;

  pDefaultSubResource = subResource;
  emit defaultSubResourceChanged(subResource);
}

bool KLFLibResourceEngine::setSubResourceProperty(const QString& /*subResource*/, int /*propId*/,
						  const QVariant& /*value*/)
{
  return false;
}

bool KLFLibResourceEngine::createSubResource(const QString& /*subResource*/,
					     const QString& /*subResourceTitle*/)
{
  return false;
}
bool KLFLibResourceEngine::createSubResource(const QString& subResource)
{
  return createSubResource(subResource, QString());
}
bool KLFLibResourceEngine::renameSubResource(const QString& /*old*/, const QString& /*new*/)
{
  return false;
}


KLFLibEntry KLFLibResourceEngine::entry(entryId id)
{
  KLFLIBRESOURCEENGINE_WARN_NO_DEFAULT_SUBRESOURCE("entry");
  return entry(pDefaultSubResource, id);
}
bool KLFLibResourceEngine::hasEntry(entryId id)
{
  KLFLIBRESOURCEENGINE_WARN_NO_DEFAULT_SUBRESOURCE("hasEntry");
  return hasEntry(pDefaultSubResource, id);
}
QList<KLFLibResourceEngine::KLFLibEntryWithId>
/* */ KLFLibResourceEngine::entries(const QList<KLFLib::entryId>& idList,
				    const QList<int>& wantedEntryProperties)
{
  KLFLIBRESOURCEENGINE_WARN_NO_DEFAULT_SUBRESOURCE("entries");
  return entries(pDefaultSubResource, idList, wantedEntryProperties);
}

QList<KLFLibResourceEngine::KLFLibEntryWithId>
/* */ KLFLibResourceEngine::allEntries(const QList<int>& wantedEntryProperties)
{
  KLFLIBRESOURCEENGINE_WARN_NO_DEFAULT_SUBRESOURCE("allEntries");
  return allEntries(pDefaultSubResource, wantedEntryProperties);
}
QList<KLFLib::entryId> KLFLibResourceEngine::allIds()
{
  KLFLIBRESOURCEENGINE_WARN_NO_DEFAULT_SUBRESOURCE("allIds");
  return allIds(pDefaultSubResource);
}




KLFLibResourceEngine::entryId KLFLibResourceEngine::insertEntry(const QString& subResource,
								const KLFLibEntry& entry)
{
  QList<entryId> ids = insertEntries(subResource, KLFLibEntryList() << entry);
  if (ids.size() == 0)
    return -1;

  return ids[0];
}
KLFLibResourceEngine::entryId KLFLibResourceEngine::insertEntry(const KLFLibEntry& entry)
{ 
  KLFLIBRESOURCEENGINE_WARN_NO_DEFAULT_SUBRESOURCE("insertEntry");
  return insertEntry(pDefaultSubResource, entry);
}
QList<KLFLibResourceEngine::entryId> KLFLibResourceEngine::insertEntries(const KLFLibEntryList& entrylist)
{
  KLFLIBRESOURCEENGINE_WARN_NO_DEFAULT_SUBRESOURCE("insertEntries");
  return insertEntries(pDefaultSubResource, entrylist);
}

bool KLFLibResourceEngine::changeEntries(const QList<entryId>& idlist, const QList<int>& properties,
					 const QList<QVariant>& values)
{
  KLFLIBRESOURCEENGINE_WARN_NO_DEFAULT_SUBRESOURCE("changeEntries");
  return changeEntries(pDefaultSubResource, idlist, properties, values);
}

bool KLFLibResourceEngine::deleteEntries(const QList<entryId>& idList)
{
  KLFLIBRESOURCEENGINE_WARN_NO_DEFAULT_SUBRESOURCE("deleteEntries");
  return deleteEntries(pDefaultSubResource, idList);
}



bool KLFLibResourceEngine::saveTo(const QUrl&)
{
  // not implemented by default. Subclasses must reimplement
  // to support this feature.
  return false;
}

QVariant KLFLibResourceEngine::resourceProperty(const QString& name) const
{
  return KLFPropertizedObject::property(name);
}

bool KLFLibResourceEngine::loadResourceProperty(const QString& propName, const QVariant& value)
{
  int propId = propertyIdForName(propName);
  if (propId < 0) {
    if (!canRegisterProperty(propName))
      return false;
    // register the property
    propId = registerProperty(propName);
    if (propId < 0)
      return false;
  }
  // finally set the property
  return setResourceProperty(propId, value);
}

bool KLFLibResourceEngine::setResourceProperty(int propId, const QVariant& value)
{
  if ( ! KLFPropertizedObject::propertyIdRegistered(propId) )
    return false;

  if ( !canModifyProp(propId) ) {
    return false;
  }

  bool result = saveResourceProperty(propId, value);
  if (!result) // operation not permitted or failed
    return false;

  // operation succeeded: set KLFPropertizedObject-based property.
  KLFPropertizedObject::setProperty(propId, value);
  emit resourcePropertyChanged(propId);
  return true;
}


KLFLibResourceEngine::ModifyStatus
/* */ KLFLibResourceEngine::baseCanModifyStatus(bool inSubResource, const QString& subResource) const
{
  if (pFeatureFlags & FeatureLocked) {
    if (locked())
      return MS_IsLocked;
    if (inSubResource &&
	(pFeatureFlags & FeatureSubResources) &&
	(pFeatureFlags & FeatureSubResourceProps)) {
      if (subResourceProperty(subResource, SubResPropLocked).toBool())
	return MS_SubResLocked;
    }
  }

  if (pFeatureFlags & FeatureReadOnly) {
    if (isReadOnly())
      return MS_NotModifiable;
  }

  return MS_CanModify;
}

// -----

QDataStream& operator<<(QDataStream& stream, const KLFLibResourceEngine::KLFLibEntryWithId& entrywid)
{
  return stream << entrywid.id << entrywid.entry;
}
QDataStream& operator>>(QDataStream& stream, KLFLibResourceEngine::KLFLibEntryWithId& entrywid)
{
  return stream >> entrywid.id >> entrywid.entry;
}



// ---------------------------------------------------

QList<KLFLib::entryId> KLFLibResourceSimpleEngine::allIds(const QString& subResource)
{
  QList<KLFLib::entryId> idList;
  QList<KLFLibResourceEngine::KLFLibEntryWithId> elist = allEntries(subResource);
  int k;
  for (k = 0; k < idList.size(); ++k)
    idList << elist[k].id;
  return idList;
}

bool KLFLibResourceSimpleEngine::hasEntry(const QString& subResource, entryId id)
{
  /** \bug ............... BUG/TODO .......... concept problem here */
  return entry(subResource, id).latex().size();
}

QList<KLFLibResourceEngine::KLFLibEntryWithId>
/* */ KLFLibResourceSimpleEngine::entries(const QString& subResource, const QList<KLFLib::entryId>& idList,
					  const QList<int>& /*wantedEntryProperties*/)
{
  QList<KLFLibEntryWithId> elist;
  int k;
  for (k = 0; k < idList.size(); ++k)
    elist << KLFLibEntryWithId(idList[k], entry(subResource, idList[k]));
  return elist;
}

QList<KLFLibResourceEngine::KLFLibEntryWithId>
/* */ KLFLibResourceSimpleEngine::findEntries(const QString& subResource,
					      const QMap<int,QVariant>& propertyValues,
					      const QList<int>& wantedEntryProperties)
{
  // unoptimized lookup
  QList<KLFLibEntryWithId> allEList = allEntries(subResource);
  QList<KLFLibEntryWithId> foundEList;
  int k;
  for (k = 0; k < allEList.size(); ++k) {
    bool ok = true;
    for (QMap<int,QVariant>::const_iterator it = propertyValues.begin();
	 ok && it != propertyValues.end(); ++it) {
      if (allEList[k].entry.property(it.key()) != it.value())
	ok = false;
    }
    if (ok)
      foundEList << allEList[k];
  }
  return foundEList;
}

// ---------------------------------------------------

// static
KLFFactoryManager KLFLibEngineFactory::pFactoryManager;


KLFLibEngineFactory::KLFLibEngineFactory(QObject *parent)
  : QObject(parent), KLFFactoryBase(&pFactoryManager)
{
}
KLFLibEngineFactory::~KLFLibEngineFactory()
{
}

uint KLFLibEngineFactory::schemeFunctions(const QString& /*scheme*/) const
{
  return FuncOpen;
}

KLFLibEngineFactory *KLFLibEngineFactory::findFactoryFor(const QUrl& url)
{
  return findFactoryFor(url.scheme());
}

KLFLibEngineFactory *KLFLibEngineFactory::findFactoryFor(const QString& urlscheme)
{
  return dynamic_cast<KLFLibEngineFactory*>(pFactoryManager.findFactoryFor(urlscheme));
}

QStringList KLFLibEngineFactory::allSupportedSchemes()
{
  return pFactoryManager.allSupportedTypes();
}

KLFLibResourceEngine *KLFLibEngineFactory::createResource(const QString& /*scheme*/,
							  const Parameters& /*param*/,
							  QObject */*parent*/)
{
  return NULL;
}

bool KLFLibEngineFactory::saveResourceTo(KLFLibResourceEngine */*resource*/, const QUrl& /*newLocation*/)
{
  return false;
}


KLFLibResourceEngine *KLFLibEngineFactory::openURL(const QUrl& url, QObject *parent)
{
  KLFLibEngineFactory *factory = findFactoryFor(url.scheme());
  if ( factory == NULL ) {
    qWarning()<<"KLFLibEngineFactory::openURL("<<url<<"): No suitable factory found!";
    return NULL;
  }
  return factory->openResource(url, parent);
}

// static
QMap<QString,QString> KLFLibEngineFactory::listSubResourcesWithTitles(const QUrl& urlbase)
{
  QUrl url = urlbase;
  url.addQueryItem("klfReadOnly", "true");
  KLFLibResourceEngine *resource = openURL(url, NULL); // NULL parent
  if ( resource == NULL ) {
    qWarning()<<"KLFLibEngineFactory::listSubResources("<<url<<"): Unable to open resource!";
    return QMap<QString,QString>();
  }
  if ( !(resource->supportedFeatureFlags() & KLFLibResourceEngine::FeatureSubResources) ) {
    qWarning()<<"KLFLibEngineFactory::listSubResources("<<url<<"): Resource does not support sub-resources!";
    return QMap<QString,QString>();
  }
  QStringList subreslist = resource->subResourceList();
  int k;
  QMap<QString,QString> subresmap;
  for (k = 0; k < subreslist.size(); ++k) {
    if (resource->supportedFeatureFlags() & KLFLibResourceEngine::FeatureSubResourceProps)
      subresmap[subreslist[k]]
	= resource->subResourceProperty(subreslist[k],
					KLFLibResourceEngine::SubResPropTitle).toString();
    else
      subresmap[subreslist[k]] = QString();
  }
  delete resource;
  return subresmap;
}

// static
QStringList KLFLibEngineFactory::listSubResources(const QUrl& urlbase)
{
  return listSubResourcesWithTitles(urlbase).keys();
}


// ---------------------------------------------------


// static
KLFFactoryManager KLFLibWidgetFactory::pFactoryManager;

KLFLibWidgetFactory::KLFLibWidgetFactory(QObject *parent)
  : QObject(parent), KLFFactoryBase(&pFactoryManager)
{
}

// static
KLFLibWidgetFactory *KLFLibWidgetFactory::findFactoryFor(const QString& wtype)
{
  return dynamic_cast<KLFLibWidgetFactory*>(pFactoryManager.findFactoryFor(wtype));
}

// static
QStringList KLFLibWidgetFactory::allSupportedWTypes()
{
  return pFactoryManager.allSupportedTypes();
}


bool KLFLibWidgetFactory::hasCreateWidget(const QString& /*scheme*/) const
{
  return false;
}

QWidget * KLFLibWidgetFactory::createPromptCreateParametersWidget(QWidget */*parent*/,
									  const QString& /*scheme*/,
									  const Parameters& /*par*/)
{
  return NULL;
}
KLFLibWidgetFactory::Parameters
/* */ KLFLibWidgetFactory::retrieveCreateParametersFromWidget(const QString& /*scheme*/,
								      QWidget */*parent*/)
{
  return Parameters();
}

bool KLFLibWidgetFactory::hasSaveToWidget(const QString& /*scheme*/) const
{
  return false;
}
QWidget *KLFLibWidgetFactory::createPromptSaveToWidget(QWidget */*parent*/,
						       const QString& /*scheme*/,
						       KLFLibResourceEngine* /*resource*/,
						       const QUrl& /*defaultUrl*/)
{
  return NULL;
}
QUrl KLFLibWidgetFactory::retrieveSaveToUrlFromWidget(const QString& /*scheme*/,
						      QWidget */*widget*/)
{
  return QUrl();
}


// -----------------------------------

KLF_EXPORT QString urlLocalFilePath(const QUrl& url)
{
#ifdef Q_OS_WIN32
  QString p = url.path();
  if (p.startsWith("/"))
    p = p.mid(1);
  return p;
#else
  return url.path();
#endif
}
