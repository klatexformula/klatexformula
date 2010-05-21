/***************************************************************************
 *   file klfliblegacyengine.cpp
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


#include <QString>
#include <QObject>
#include <QDataStream>
#include <QFile>
#include <QFileInfo>
#include <QMessageBox>

#include "klflib.h"
#include "klflibview.h"
#include "klfliblegacyengine.h"
#include "klfliblegacyengine_p.h"

quint32 KLFLegacyData::KLFLibraryItem::MaxId = 1;



QDataStream& operator<<(QDataStream& stream, const KLFLegacyData::KLFLibraryItem& item)
{
  return stream << item.id << item.datetime
      << item.latex // category and tags are included.
      << item.preview << item.style;
}

// it is important to note that the >> operator imports in a compatible way to KLF 2.0
QDataStream& operator>>(QDataStream& stream, KLFLegacyData::KLFLibraryItem& item)
{
  stream >> item.id >> item.datetime >> item.latex >> item.preview >> item.style;
  item.category = KLFLibEntry::categoryFromLatex(item.latex);
  item.tags = KLFLibEntry::tagsFromLatex(item.latex);
  return stream;
}


QDataStream& operator<<(QDataStream& stream, const KLFLegacyData::KLFLibraryResource& item)
{
  return stream << item.id << item.name;
}
QDataStream& operator>>(QDataStream& stream, KLFLegacyData::KLFLibraryResource& item)
{
  return stream >> item.id >> item.name;
}


bool operator==(const KLFLegacyData::KLFLibraryItem& a, const KLFLegacyData::KLFLibraryItem& b)
{
  return
    //    a.id == b.id &&   // don't compare IDs since they should be different.
    //    a.datetime == b.datetime &&   // same for datetime
    a.latex == b.latex &&
    /* the following is unnecessary since category/tags information is contained in .latex
      a.category == b.category &&
      a.tags == b.tags &&
    */
    //    a.preview == b.preview && // don't compare preview: it's unnecessary and overkill
    a.style == b.style;
}



bool operator<(const KLFLegacyData::KLFLibraryResource a, const KLFLegacyData::KLFLibraryResource b)
{
  return a.id < b.id;
}
bool operator==(const KLFLegacyData::KLFLibraryResource a, const KLFLegacyData::KLFLibraryResource b)
{
  return a.id == b.id;
}

bool resources_equal_for_import(const KLFLegacyData::KLFLibraryResource a,
				const KLFLegacyData::KLFLibraryResource b)
{
  return a.name == b.name;
}


QDataStream& operator<<(QDataStream& stream, const KLFLegacyData::KLFStyle& style)
{
  return stream << style.name << (quint32)style.fg_color << (quint32)style.bg_color
		<< style.mathmode << style.preamble << (quint16)style.dpi;
}
QDataStream& operator>>(QDataStream& stream, KLFLegacyData::KLFStyle& style)
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
bool operator==(const KLFLegacyData::KLFStyle& a, const KLFLegacyData::KLFStyle& b)
{
  return a.name == b.name &&
    a.fg_color == b.fg_color &&
    a.bg_color == b.bg_color &&
    a.mathmode == b.mathmode &&
    a.preamble == b.preamble &&
    a.dpi == b.dpi;
}



// -------------------------------------------


// static
KLFLibLegacyEngine * KLFLibLegacyEngine::openUrl(const QUrl& url, QObject *parent)
{
  QString fname = url.path();
  if ( ! QFileInfo(fname).isReadable() ) {
    qWarning("KLFLibLegacyEngine::openUrl(): file %s does not exist!", qPrintable(fname));
    return NULL;
  }

  if (url.scheme() != "klf+legacy") {
    qWarning("KLFLibLegacyEngine::openUrl(): unsupported scheme %s!", qPrintable(url.scheme()));
    return NULL;
  }

  QString legresname;
  if (url.hasQueryItem("klfDefaultSubResource"))
    legresname = url.queryItemValue("klfDefaultSubResource");

  return new KLFLibLegacyEngine(fname, legresname, url, parent);
}

// static
KLFLibLegacyEngine * KLFLibLegacyEngine::createDotKLF(const QString& fileName, QString legacyResourceName,
						      QObject *parent)
{
  QString lrname = legacyResourceName;
  if (QFile::exists(fileName)) {
    // fail; we want to _CREATE_ a .klf file. Erase file before calling this function to overwrite.
    return NULL;
  }
  QUrl url = QUrl::fromLocalFile(fileName);
  url.setScheme("klf+legacy");

  if (lrname.isEmpty())
    lrname = "Resource"; // default name...?
  url.addQueryItem("legacyResourceName", lrname);

  // create the .klf file
  KLFLegacyData::KLFLibrary lib = KLFLegacyData::KLFLibrary();
  KLFLegacyData::KLFLibraryResourceList reslist = KLFLegacyData::KLFLibraryResourceList();
  bool r = saveLibraryFile(fileName, reslist, lib, ExportLibraryType);
  if (!r)
    return false;

  return new KLFLibLegacyEngine(fileName, lrname, url, parent);
}

// private, static
bool KLFLibLegacyEngine::loadLibraryFile(const QString& fname, KLFLegacyData::KLFLibraryResourceList *reslist,
					 KLFLegacyData::KLFLibrary *lib, LegacyLibType *libType)
{
  QFile fimp(fname);
  if ( ! fimp.open(QIODevice::ReadOnly) ) {
    qWarning("Unable to open library file %s!", qPrintable(fname));
    return false;
  }
  QDataStream stream(&fimp);
  // Qt3-compatible stream input
  stream.setVersion(QDataStream::Qt_3_3);
  QString s1;
  stream >> s1;
  if (s1 == "KLATEXFORMULA_LIBRARY_EXPORT") {
    // opening an export file (*.klf)
    *libType = ExportLibraryType;
    qint16 vmaj, vmin;
    stream >> vmaj >> vmin; // these are not needed, format has not changed in .klf export files.
    stream >> *reslist >> *lib;
  } else if (s1 == "KLATEXFORMULA_LIBRARY") {
    // opening a library file (~/.klatexformula/library)
    *libType = LocalLibraryType;
    qint16 vmaj, vmin;
    stream >> vmaj >> vmin;
    if (vmaj <= 2) {
      stream.setVersion(QDataStream::Qt_3_3);
    } else {
      qint16 version;
      stream >> version;
      stream.setVersion(version);
    }
    quint32 lib_max_id;
    stream >> lib_max_id; // will not be used...
    // now read the library itself.
    stream >> *reslist >> *lib;
  } else if (s1 == "KLATEXFORMULA_HISTORY") {
    // opening a post-2.0, pre-2.1 "history" file  (no "library" yet)
    *libType = LocalHistoryType;
    qint16 vmaj, vmin;
    stream >> vmaj >> vmin;
    KLFLegacyData::KLFLibraryList history;
    quint32 lib_max_id;
    stream >> lib_max_id >> history;

    *reslist = KLFLegacyData::KLFLibraryResourceList();
    KLFLegacyData::KLFLibraryResource historyresource;
    historyresource.id = KLFLegacyData::LibResource_History;
    historyresource.name = tr("History");
    reslist->append(historyresource);
    *lib = KLFLegacyData::KLFLibrary();
    lib->operator[](historyresource) = history;
	  
  } else {
    qWarning("Error: Library file `%s' is invalid library file or corrupt!", qPrintable(fname));
    return false;
  }

  // update KLFLibraryItem::MaxId
  int k;
  for (k = 0; k < reslist->size(); ++k) {
    KLFLegacyData::KLFLibraryList ll = lib->operator[](reslist->operator[](k));
    int j;
    for (j = 0; j < ll.size(); ++j) {
      if (ll[j].id >= KLFLegacyData::KLFLibraryItem::MaxId)
	KLFLegacyData::KLFLibraryItem::MaxId = ll[j].id+1;
    }
  }

  return true;
}

// private, static
bool KLFLibLegacyEngine::saveLibraryFile(const QString& fname,
					 const KLFLegacyData::KLFLibraryResourceList& reslist,
					 const KLFLegacyData::KLFLibrary& lib,
					 LegacyLibType libType)
{
  QFile fsav(fname);
  if ( ! fsav.open(QIODevice::WriteOnly) ) {
    qWarning("Can't write to file %s!", qPrintable(fname));
    return false;
  }
  QDataStream stream(&fsav);
  // write Qt 3.3-compatible data
  stream.setVersion(QDataStream::Qt_3_3);

  // WE'RE WRITING KLF 2.1 compatible output, so we write as being version 2.1
  switch (libType) {
  case LocalHistoryType:
    {
      KLFLegacyData::KLFLibraryList liblist;
      if (reslist.size() == 0) {
	liblist = KLFLegacyData::KLFLibraryList(); // no resources !
      } else {
	liblist = lib[reslist[0]];
      }
      if (reslist.size() > 1) {
	qWarning("KLFLibLegacyEngine::saveLibraryFile: Saving an old \"history\" resource. Only "
		 "one resource can be saved, it will be the first: %s", qPrintable(reslist[0].name));
      }
      // find history resource in our
      stream << QString("KLATEXFORMULA_HISTORY") << (qint16)2 << (qint16)0
	     << (quint32)KLFLegacyData::KLFLibraryItem::MaxId << liblist;
      break;
    }
  case LocalLibraryType:
    {
      stream << QString("KLATEXFORMULA_LIBRARY") << (qint16)2 << (qint16)1;
      // don't save explicitely QDataStream version: we're writing <= KLF 2.x
      // version explicitely saved since KLF >= 3.x.
      stream << (quint32)KLFLegacyData::KLFLibraryItem::MaxId << reslist << lib;
      break;
    }
  case ExportLibraryType:
    stream << QString("KLATEXFORMULA_LIBRARY_EXPORT") << (qint16)2 << (qint16)1
	   << reslist << lib;
    break;
  default:
    qWarning("KLFLibLegacyEngine::saveLibraryFile: bad library type %d!", libType);
    stream << QString("KLATEXFORMULA_LIBRARY_EXPORT") << (qint16)2 << (qint16)1
	   << reslist << lib;
  }
}

// private
int KLFLibLegacyEngine::findResourceName(const QString& resname)
{
  int k;
  for (k = 0; k < pResources.size() && pResources[k].name != resname; ++k)
    ;
  return k < pResources.size() ? k : -1;
}

// private
KLFLibLegacyEngine::KLFLibLegacyEngine(const QString& fileName, const QString& resname, const QUrl& url,
				       QObject *parent)
  : KLFLibResourceSimpleEngine(url, FeatureReadOnly|FeatureSaveTo|FeatureSubResources, parent)
{
  // load library
  pFileName = fileName;
  loadLibraryFile(fileName, &pResources, &pLibrary, &pLegacyLibType);

  // add at least one resource (baptized resname) if the library is empty
  if (pResources.isEmpty()) {
    // create a resource
    KLFLegacyData::KLFLibraryResource res;
    res.id = KLFLegacyData::LibResourceUSERMAX-1;
    res.name = resname;
    pResources << res;
    // and initialize pLibrary to contain this one empty resource.
    pLibrary.clear();
    pLibrary[res] = KLFLegacyData::KLFLibraryList();
  }

  KLFPropertizedObject::setProperty(PropAccessShared, QVariant::fromValue(false));
  KLFPropertizedObject::setProperty(PropTitle, QFileInfo(fileName).baseName());

  pAutoSaveTimer = new QTimer(this);
  connect(pAutoSaveTimer, SIGNAL(timeout()), this, SLOT(save()));

  if (!isReadOnly())
    setAutoSaveInterval(180000); // by default: 3 minutes

}

KLFLibLegacyEngine::~KLFLibLegacyEngine()
{
  if (!isReadOnly())
    save();
}

bool KLFLibLegacyEngine::canModifyData(const QString& subResouce, ModifyType modifytype) const
{
  if ( ! KLFLibResourceEngine::canModifyData(subResouce, modifytype) )
    return false;
  /// \todo TODO: check if file is writable, etc. ..............

  return true;
}

bool KLFLibLegacyEngine::canModifyProp(int propid) const
{
  return false;
}

bool KLFLibLegacyEngine::canRegisterProperty(const QString& propName) const
{
  return false;
}

KLFLibEntry KLFLibLegacyEngine::entry(const QString& resource, entryId id)
{
  int index = findResourceName(resource);
  if (index < 0)
    return KLFLibEntry();

  KLFLegacyData::KLFLibraryList ll = pLibrary[pResources[index]];
  // find entry with id 'id'
  int k;
  for (k = 0; k < ll.size() && ll[k].id != id; ++k)
    ;
  if (k == ll.size()) {
    return KLFLibEntry();
  }

  return toLibEntry(ll[k]);
}

// private
KLFLegacyData::KLFStyle KLFLibLegacyEngine::toLegacyStyle(const KLFStyle& style)
{
  KLFLegacyData::KLFStyle oldstyle;
  oldstyle.name = style.name;
  oldstyle.fg_color = style.fg_color;
  oldstyle.bg_color = style.bg_color;
  oldstyle.mathmode = style.mathmode;
  oldstyle.preamble = style.preamble;
  oldstyle.dpi = style.dpi;
  return oldstyle;
}
// private
KLFStyle KLFLibLegacyEngine::toStyle(const KLFLegacyData::KLFStyle& oldstyle)
{
  KLFStyle style;
  style.name = oldstyle.name;
  style.fg_color = oldstyle.fg_color;
  style.bg_color = oldstyle.bg_color;
  style.mathmode = oldstyle.mathmode;
  style.preamble = oldstyle.preamble;
  style.dpi = oldstyle.dpi;
  return style;
}

// private
KLFLibEntry KLFLibLegacyEngine::toLibEntry(const KLFLegacyData::KLFLibraryItem& item)
{
  return KLFLibEntry(item.latex, item.datetime, item.preview.toImage(), item.category, item.tags,
		     toStyle(item.style));
}
// private
KLFLegacyData::KLFLibraryItem KLFLibLegacyEngine::toLegacyLibItem(const KLFLibEntry& entry)
{
  KLFLegacyData::KLFLibraryItem item;
  item.id = KLFLegacyData::KLFLibraryItem::MaxId++;
  item.latex = entry.latex();
  item.category = entry.category();
  item.tags = entry.tags();
  item.preview = QPixmap::fromImage(entry.preview());
  item.datetime = entry.dateTime();
  item.style = toLegacyStyle(entry.style());
  return item;
}


QList<KLFLibResourceEngine::KLFLibEntryWithId> KLFLibLegacyEngine::allEntries(const QString& resource)
{
  int rindex = findResourceName(resource);
  if (rindex < 0)
    return QList<KLFLibEntryWithId>();

  QList<KLFLibEntryWithId> entryList;
  KLFLegacyData::KLFLibraryList ll = pLibrary[pResources[rindex]];
  int k;
  for (k = 0; k < ll.size(); ++k) {
    KLFLibEntryWithId e;
    e.entry = toLibEntry(ll[k]);
    e.id = ll[k].id;
    entryList << e;
  }
  return entryList;
}


QStringList KLFLibLegacyEngine::subResourceList() const
{
  QStringList list;
  int k;
  for (k = 0; k < pResources.size(); ++k)
    list << pResources[k].name;
  return list;
}

bool KLFLibLegacyEngine::createSubResource(const QString& subResource, const QString& /*subResourceTitle*/)
{
  int newId = KLFLegacyData::LibResourceUSERMIN;
  bool ok = true;
  int k;
  while (!ok && newId <= KLFLegacyData::LibResourceUSERMAX) {
    for (k = 0; k < pResources.size() && pResources[k].id != newId; ++k)
      ;
    if (k == pResources.size())
      ok = true;
    ++newId;
  }
  if (newId == KLFLegacyData::LibResourceUSERMAX) {
    qWarning()<<"KLFLibLegacyEngine::createSubResource("<<subResource<<",..): no new ID could be found (!?!)";
    return false;
  }
  KLFLegacyData::KLFLibraryResource res;
  res.name = subResource;
  res.id = newId;

  pResources.push_back(res);
  pLibrary[res] = KLFLegacyData::KLFLibraryList();
  return true;
}


bool KLFLibLegacyEngine::save()
{
  if (isReadOnly()) {
    qWarning("KLFLibLegacyEngine::save: resource is read-only!");
    return false;
  }
  return saveLibraryFile(pFileName, pResources, pLibrary, pLegacyLibType);
}

void KLFLibLegacyEngine::setAutoSaveInterval(int intervalms)
{
  pAutoSaveTimer->stop();
  if (intervalms > 0) {
    pAutoSaveTimer->setInterval(intervalms);
    pAutoSaveTimer->setSingleShot(false);
    pAutoSaveTimer->start();
  }
}

QList<KLFLibResourceEngine::entryId> KLFLibLegacyEngine::insertEntries(const QString& subResource,
								       const KLFLibEntryList& entrylist)
{
  if ( entrylist.size() == 0 )
     return QList<entryId>();
  if (!canModifyData(subResource, InsertData))
    return QList<entryId>();

  int index = findResourceName(subResource);
  if (index < 0)
    return QList<entryId>();

  QList<entryId> newIds;

  int k;
  for (k = 0; k < entrylist.size(); ++k) {
    KLFLegacyData::KLFLibraryItem item = toLegacyLibItem(entrylist[k]);
    pLibrary[pResources[index]] << item;
    newIds << item.id;
  }

  emit dataChanged(subResource, InsertData, newIds);

  return newIds;
}
bool KLFLibLegacyEngine::changeEntries(const QString& subResource, const QList<entryId>& idlist,
				       const QList<int>& properties, const QList<QVariant>& values)
{
  if (idlist.size() == 0)
    return true;
  if (!canModifyData(subResource, ChangeData))
    return false;

  int index = findResourceName(subResource);
  if (index < 0)
    return false;

  const KLFLegacyData::KLFLibraryList& ll = pLibrary[pResources[index]];

  bool success = true;

  int k;
  for (k = 0; k < idlist.size(); ++k) {
    int libindex;
    int j;
    // find the entry
    for (libindex = 0; libindex < ll.size() && ll[libindex].id != idlist[k]; ++libindex)
      ;
    if (libindex == ll.size()) {
      qWarning()<<"KLFLibLegacyEngine::changeEntries: Can't find entry with id "<<idlist[k];
      success = false;
      continue;
    }
    // modify this entry as requested.
    for (j = 0; j < properties.size(); ++j) {
      switch (properties[j]) {
      case KLFLibEntry::Latex:
	pLibrary[pResources[index]][libindex].latex = values[j].toString();
	break;
      case KLFLibEntry::DateTime:
	pLibrary[pResources[index]][libindex].datetime = values[j].toDateTime();
	break;
      case KLFLibEntry::Preview:
	pLibrary[pResources[index]][libindex].preview = QPixmap::fromImage(values[j].value<QImage>());
	break;
      case KLFLibEntry::Category:
	pLibrary[pResources[index]][libindex].category = values[j].toString();
	break;
      case KLFLibEntry::Tags:
	pLibrary[pResources[index]][libindex].tags = values[j].toString();
	break;
      case KLFLibEntry::Style:
	pLibrary[pResources[index]][libindex].style = toLegacyStyle(values[j].value<KLFStyle>());
	break;
      default:
	qWarning("KLFLibLegacyEngine::changeEntries: Cannot set arbitrary property '%s'.",
		 qPrintable(propertyNameForId(properties[j])));
	success = false;
	break;
      }
    }
  }

  emit dataChanged(subResource, ChangeData, idlist);

  return success;
}

bool KLFLibLegacyEngine::deleteEntries(const QString& subResource, const QList<entryId>& idlist)
{
  if (idlist.isEmpty())
    return true;
  if (!canModifyData(subResource, DeleteData))
    return false;

  int index = findResourceName(subResource);
  if (index < 0)
    return false;

  // now remove the requested entries

  KLFLegacyData::KLFLibraryList *ll = & pLibrary[pResources[index]];
  bool success = true;
  int k;
  for (k = 0; k < idlist.size(); ++k) {
    int j;
    for (j = 0; j < ll->size() && ll->operator[](j).id != idlist[k]; ++j)
      ;
    if (j == ll->size()) {
      qWarning("KLFLibLegacyEngine::deleteEntries: Can't find ID %d in library list in current resource.",
	       idlist[k]);
      success = false;
      continue;
    }
    // remove this entry from list
    ll->removeAt(j);
  }

  emit dataChanged(subResource, DeleteData, idlist);

  return success;
}

bool KLFLibLegacyEngine::saveTo(const QUrl& newPath)
{
  if (newPath.scheme() == "klf+legacy") {
    return saveLibraryFile(newPath.path(), pResources, pLibrary, ExportLibraryType);
  }
  return false;
}

bool KLFLibLegacyEngine::saveResourceProperty(int propId, const QVariant& value)
{
  return false; // can't change anything.
}


// ------------------------------------


QString KLFLibLegacyLocalFileSchemeGuesser::guessScheme(const QString& fileName) const
{
  if (fileName.endsWith(".klf"))
    return QLatin1String("klf+legacy");

  QFile f(fileName);
  if ( ! f.open(QIODevice::ReadOnly) ) {
    return QString();
  }
  QDataStream stream(&f);
  // Qt3-compatible stream input
  stream.setVersion(QDataStream::Qt_3_3);
  QString s1;
  stream >> s1;
  if (s1 == "KLATEXFORMULA_LIBRARY_EXPORT" || s1 == "KLATEXFORMULA_LIBRARY" ||
      s1 == "KLATEXFORMULA_HISTORY")
    return QLatin1String("klf+legacy");

  return QString();
}

// ------------------------------------


KLFLibLegacyEngineFactory::KLFLibLegacyEngineFactory(QObject *parent)
  : KLFLibEngineFactory(parent)
{
  KLFLibBasicWidgetFactory::LocalFileType f;
  f.scheme = QLatin1String("klf+legacy");
  f.filepattern = QLatin1String("*.klf");
  f.filter = QString("%1 (%2)").arg(schemeTitle(f.scheme), f.filepattern);
  KLFLibBasicWidgetFactory::addLocalFileType(f);
  new KLFLibLegacyLocalFileSchemeGuesser(this);
}

QStringList KLFLibLegacyEngineFactory::supportedTypes() const
{
  return QStringList() << QLatin1String("klf+legacy");
}

QString KLFLibLegacyEngineFactory::schemeTitle(const QString& scheme) const
{
  if (scheme == QLatin1String("klf+legacy"))
    return tr("KLatexFormula 3.1 Library Export File");
  return QString();
}


uint KLFLibLegacyEngineFactory::schemeFunctions(const QString& scheme) const
{
  if (scheme == QLatin1String("klf+legacy"))
    return FuncOpen|FuncCreate;
  return 0;
}

QString KLFLibLegacyEngineFactory::correspondingWidgetType(const QString& scheme) const
{
  if (scheme == QLatin1String("klf+legacy"))
    return QLatin1String("LocalFile");
  return 0;
}


KLFLibResourceEngine *KLFLibLegacyEngineFactory::openResource(const QUrl& location, QObject *parent)
{
  return KLFLibLegacyEngine::openUrl(location, parent);
}

KLFLibResourceEngine *KLFLibLegacyEngineFactory::createResource(const QString& scheme,
								const Parameters& parameters,
								QObject *parent)
{
  QString defsubres = parameters["klfDefaultSubResource"].toString();
  if (defsubres.isEmpty())
    defsubres = QLatin1String("Table1");

  if (scheme == QLatin1String("klf+legacy")) {
    if ( !parameters.contains("Filename") ) {
      qWarning()
	<<"KLFLibLegacyEngineFactory::createResource: bad parameters. They do not contain `Filename': "
	<<parameters;
      return NULL;
    }
    return KLFLibLegacyEngine::createDotKLF(parameters["Filename"].toString(),
					    defsubres, parent);
  }
  qWarning()<<"KLFLibLegacyEngineFactory::createResource("<<scheme<<","<<parameters<<","<<parent<<"):"
	    <<"Bad scheme!";
  return NULL;
}



