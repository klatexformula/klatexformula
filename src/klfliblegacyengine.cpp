/***************************************************************************
 *   file klfliblegacyengine.cpp
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


#include <QString>
#include <QObject>
#include <QDataStream>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QMessageBox>
#include <QApplication> // qApp

#include "klflib.h"
#include "klflibview.h"
#include "klfconfig.h"
#include <klfutil.h>
#include "klfliblegacyengine.h"
#include "klfliblegacyengine_p.h"


/** \page libfmt_legacy Library Export Format Specification
 *
 * \todo (needs doc..............)
 *
 * - refer to klfliblegacyengine.cpp
 * - very basically: format is
 *   \code
 *  stream << QString("KLATEXFORMULA_LIBRARY_EXPORT") << (qint16)2 << (qint16)1
 *         << resources << library;
 *  // additionally, save our meta-data at the end (this will be ignored by previous versions
 *  // of KLF)
 *  stream << metadata;
 *   \endcode
 * - resources is a KLFLegacyData::KLFLibraryResourceList
 * - library is a KLFLegacyData::KLFLibrary
 * - all current versions of KLF write the version '2.1', it's the compatiblity version that is
 *   written, not the true version of the creating program.
 * - since 3.2, additional metadata (a QVariantMap) is appended at end of stream, will silently
 *   be ignored by previous versions of klf. Klf 3.2 does not itself make use of the meta-data,
 *   but it loads and saves it for compatibility with future versions will (as planned) will
 *   support resource and sub-resource properties, stored in this data structure.
 */





quint32 KLFLegacyData::KLFLibraryItem::MaxId = 1;


// debug

QDebug& operator<<(QDebug& s, const KLFLegacyData::KLFLibraryItem& item)
{
  return s << "KLFLegacyData::KLFLibraryItem(id="<<item.id<<"; latex="<<item.latex<<")" ;
}

QDebug& operator<<(QDebug& s, const KLFLegacyData::KLFLibraryResource& res)
{
  return s << "KLFLegacyData::KLFLibraryResource(id="<<res.id<<"; name="<<res.name<<")";
}





KLF_EXPORT QDataStream& operator<<(QDataStream& stream, const KLFLegacyData::KLFLibraryItem& item)
{
  return stream << item.id << item.datetime
      << item.latex // category and tags are included.
      << item.preview << item.style;
}

// it is important to note that the >> operator imports in a compatible way to KLF 2.0
KLF_EXPORT QDataStream& operator>>(QDataStream& stream, KLFLegacyData::KLFLibraryItem& item)
{
  stream >> item.id >> item.datetime >> item.latex >> item.preview >> item.style;
  item.category = KLFLibEntry::categoryFromLatex(item.latex);
  item.tags = KLFLibEntry::tagsFromLatex(item.latex);
  return stream;
}


KLF_EXPORT QDataStream& operator<<(QDataStream& stream, const KLFLegacyData::KLFLibraryResource& item)
{
  return stream << item.id << item.name;
}
KLF_EXPORT QDataStream& operator>>(QDataStream& stream, KLFLegacyData::KLFLibraryResource& item)
{
  return stream >> item.id >> item.name;
}


KLF_EXPORT bool operator==(const KLFLegacyData::KLFLibraryItem& a, const KLFLegacyData::KLFLibraryItem& b)
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



KLF_EXPORT bool operator<(const KLFLegacyData::KLFLibraryResource a, const KLFLegacyData::KLFLibraryResource b)
{
  return a.id < b.id;
}
KLF_EXPORT bool operator==(const KLFLegacyData::KLFLibraryResource a, const KLFLegacyData::KLFLibraryResource b)
{
  return a.id == b.id;
}

KLF_EXPORT bool resources_equal_for_import(const KLFLegacyData::KLFLibraryResource a,
					   const KLFLegacyData::KLFLibraryResource b)
{
  return a.name == b.name;
}


KLF_EXPORT QDataStream& operator<<(QDataStream& stream, const KLFLegacyData::KLFStyle& style)
{
  return stream << style.name << (quint32)style.fg_color << (quint32)style.bg_color
		<< style.mathmode << style.preamble << (quint16)style.dpi;
}
KLF_EXPORT QDataStream& operator>>(QDataStream& stream, KLFLegacyData::KLFStyle& style)
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
KLF_EXPORT bool operator==(const KLFLegacyData::KLFStyle& a, const KLFLegacyData::KLFStyle& b)
{
  return a.name == b.name &&
    a.fg_color == b.fg_color &&
    a.bg_color == b.bg_color &&
    a.mathmode == b.mathmode &&
    a.preamble == b.preamble &&
    a.dpi == b.dpi;
}



// -------------------------------------------

// KLFLibLegacyFileDataPrivate


// static
QMap<QString,KLFLibLegacyFileDataPrivate*> KLFLibLegacyFileDataPrivate::staticFileDataObjects;



bool KLFLibLegacyFileDataPrivate::load(const QString& fnm)
{
  KLF_DEBUG_TIME_BLOCK(KLF_FUNC_NAME+"("+fnm+")") ;

  QString fname = (!fnm.isEmpty() ? fnm : filename);

  flagForceReadOnly = false;

  klfDbg("loading from file "<<fname<<" (our filename="<<filename<<")") ;

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
    legacyLibType = ExportLibraryType;
    qint16 vmaj, vmin;
    stream >> vmaj >> vmin; // these are not needed, format has not changed in .klf export files.
    stream >> resources >> library;
    if (!stream.atEnd() && stream.status() == QDataStream::Ok)
      stream >> metadata;
  } else if (s1 == "KLATEXFORMULA_LIBRARY") {
    // opening a library file (~/.klatexformula/library)
    legacyLibType = LocalLibraryType;
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
    stream >> resources >> library;
    // the meta-data cannot have been written by KLF 3.0--3.1 but we may possibly already have
    // written to this file with klf 3.2
    if (!stream.atEnd() && stream.status() == QDataStream::Ok)
      stream >> metadata;
  } else if (s1 == "KLATEXFORMULA_HISTORY") {
    // opening a post-2.0, pre-2.1 "history" file  (no "library" yet)
    legacyLibType = LocalHistoryType;
    qint16 vmaj, vmin;
    stream >> vmaj >> vmin;
    KLFLegacyData::KLFLibraryList history;
    quint32 lib_max_id;
    stream >> lib_max_id >> history;

    resources = KLFLegacyData::KLFLibraryResourceList();
    KLFLegacyData::KLFLibraryResource historyresource;
    historyresource.id = KLFLegacyData::LibResource_History;
    historyresource.name = tr("History");
    resources.append(historyresource);
    library = KLFLegacyData::KLFLibrary();
    library[historyresource] = history;
	  
  } else {
    qWarning("Error: Library file `%s' is invalid library file or corrupt!", qPrintable(fname));
    flagForceReadOnly = true;
    return false;
  }

  // update KLFLibraryItem::MaxId
  int k;
  for (k = 0; k < resources.size(); ++k) {
    KLFLegacyData::KLFLibraryList ll = library[resources[k]];
    int j;
    for (j = 0; j < ll.size(); ++j) {
      if (ll[j].id >= KLFLegacyData::KLFLibraryItem::MaxId)
	KLFLegacyData::KLFLibraryItem::MaxId = ll[j].id+1;
    }
  }
  haschanges = false;
  return true;
}

bool KLFLibLegacyFileDataPrivate::save(const QString& fnm)
{
  KLF_DEBUG_TIME_BLOCK(KLF_FUNC_NAME+"("+fnm+")") ;

  if (flagForceReadOnly) {
    qWarning()<<KLF_FUNC_NAME<<": attempt to save a forced-read-only file "<<fnm<<" !";
    return false;
  }

  QString fname = (!fnm.isEmpty() ? fnm : filename) ;
  klfDbg(" saving to file "<<fname<<" a "<<legacyLibType<<"-type library with N="<<resources.size()
	 <<" resources (our filename="<<filename<<")") ;
  QFile fsav(fname);
  if ( ! fsav.open(QIODevice::WriteOnly) ) {
    qWarning("Can't write to file %s!", qPrintable(fname));
    QMessageBox::critical(NULL, tr("Error"), tr("Can't write to file %1").arg(fname));
    return false;
  }
  QDataStream stream(&fsav);

  // WE'RE WRITING KLF 2.1 compatible output, so we write as being version 2.1
  // This also implies that we write with QDataStream version "Qt 3.3"

  // write Qt 3.3-compatible data
  stream.setVersion(QDataStream::Qt_3_3);

  LegacyLibType llt = legacyLibType;

  // however, for old versions of klatexformula:
  QString cfname = canonicalFilePath(fname);
  if (legacyLibType == ExportLibraryType &&
      (cfname == canonicalFilePath(klfconfig.homeConfigDir+"/library") ||
       (cfname.startsWith(canonicalFilePath(QDir::homePath()+"/.kde")) &&
	(cfname.endsWith("/library") || cfname.endsWith("/history"))))) {
    // the file name is a legacy library or history.
    if (QFileInfo(fsav).fileName() == "history")
      llt = LocalHistoryType;
    if (QFileInfo(fsav).fileName() == "library")
      llt = LocalLibraryType;
    klfDbg("adjusted legacy lib type to save to type "<<llt<<" instead of "<<legacyLibType) ;
  }

  switch (llt) {
  case LocalHistoryType:
    {
      KLFLegacyData::KLFLibraryList liblist;
      if (resources.size() == 0) {
	liblist = KLFLegacyData::KLFLibraryList(); // no resources !
      } else {
	liblist = library[resources[0]];
      }
      if (resources.size() > 1) {
	qWarning("%s: Saving an old \"history\" resource. Only one resource can be saved, "
		 "it will be the first: %s", KLF_FUNC_NAME, qPrintable(resources[0].name));
	QMessageBox::warning(NULL, tr("Warning"),
			     tr("Saving an old \"history\" resource. Only one resource can be saved, "
				"it will be the first: %1").arg(resources[0].name));
      }
      // find history resource in our
      stream << QString("KLATEXFORMULA_HISTORY") << (qint16)2 << (qint16)0
	     << (quint32)KLFLegacyData::KLFLibraryItem::MaxId << liblist;
      break;
    }
  case LocalLibraryType:
    {
      stream << QString("KLATEXFORMULA_LIBRARY") << (qint16)2 << (qint16)1;
      // don't save explicitely QDataStream version: we're writing KLF 2.1-compatible;
      // version explicitely saved only since KLF >= 3.x.
      stream << (quint32)KLFLegacyData::KLFLibraryItem::MaxId << resources << library;
      // additionally, save our meta-data at the end (this will be ignored by previous versions
      // of KLF)
      stream << metadata;
      break;
    }
  case ExportLibraryType:
  default:
    if (llt != ExportLibraryType) {
      qWarning("%s: bad library type %d! Falling back to '.klf'-library-export type",
	       KLF_FUNC_NAME, llt);
    }

    stream << QString("KLATEXFORMULA_LIBRARY_EXPORT") << (qint16)2 << (qint16)1
	   << resources << library;
    // additionally, save our meta-data at the end (this will be ignored by previous versions
    // of KLF)
    stream << metadata;

    klfDbg("saved export-library type. resource count: "<<resources.size()) ;
#ifdef KLF_DEBUG
    if (resources.size()) {
      KLFLegacyData::KLFLibraryList llll = ((const KLFLegacyData::KLFLibrary)library)[resources[0]];
      klfDbg("in first resource wrote "<<llll.size()<<" items.");
    }
#endif
    break;
  }

  if (fnm.isEmpty() || canonicalFilePath(fnm) == canonicalFilePath(filename))
    haschanges = false; // saving to the reference file, not a copy
  return true;
}


int KLFLibLegacyFileDataPrivate::getReservedResourceId(const QString& resname, int defaultId)
{
  if ( ! QString::localeAwareCompare(resname, "History") ||
       ! QString::localeAwareCompare(resname, qApp->translate("KLFMainWin", "History")) ||
       ! QString::compare(resname, "History", Qt::CaseInsensitive) ||
       ! QString::compare(resname, qApp->translate("KLFMainWin", "History"), Qt::CaseInsensitive) )
    return KLFLegacyData::LibResource_History;
  if ( ! QString::localeAwareCompare(resname, "Archive") ||
       ! QString::localeAwareCompare(resname, qApp->translate("KLFMainWin", "Archive")) ||
       ! QString::compare(resname, "Archive", Qt::CaseInsensitive) ||
       ! QString::compare(resname, qApp->translate("KLFMainWin", "Archive"), Qt::CaseInsensitive) )
    return KLFLegacyData::LibResource_Archive;

  return defaultId;
}

int KLFLibLegacyFileDataPrivate::findResourceName(const QString& resname)
{
  int k;
  for (k = 0; k < resources.size() && resources[k].name != resname; ++k)
    ;
  return k < resources.size() ? k : -1;
}






// -------------------------------------------


// static
KLFLibLegacyEngine * KLFLibLegacyEngine::openUrl(const QUrl& url, QObject *parent)
{
  KLF_DEBUG_BLOCK(KLF_FUNC_NAME) ;

  QString fname = klfUrlLocalFilePath(url);
  if (fname.isEmpty()) {
    klfDbgSt("Requested empty fname.") ;
    return NULL;
  }
  if ( ! QFileInfo(fname).isReadable() ) {
    qWarning("%s: file %s does not exist!", KLF_FUNC_NAME, qPrintable(fname));
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
KLFLibLegacyEngine * KLFLibLegacyEngine::createDotKLF(const QString& fname, QString legacyResourceName,
						      QObject *parent)
{
  QString fileName = KLFLibLegacyFileDataPrivate::canonicalFilePath(fname);

  QString lrname = legacyResourceName;
  if (QFile::exists(fileName)) {
    // fail; we want to _CREATE_ a .klf file. Erase file before calling this function to overwrite.
    return NULL;
  }

  if (fileName.isEmpty()) {
    qWarning()<<KLF_FUNC_NAME<<": file name "<<fileName<<" is empty!";
    return NULL;
  }
  //  if (!QFileInfo(QFileInfo(fileName).absolutePath()).isWritable()) {
  //    qWarning()<<KLF_FUNC_NAME<<": containing directory "<<QFileInfo(fileName).absolutePath()<<" is not writable.";
  //    return NULL;
  //  }

  QUrl url = QUrl::fromLocalFile(fileName);
  url.setScheme("klf+legacy");

  if (lrname.isEmpty())
    lrname = tr("Default Resource"); // default name...?
  url.addQueryItem("klfDefaultSubResource", lrname);

  klfDbgSt("fileName="<<fileName<<"; canonical file path="<<QFileInfo(fileName).canonicalFilePath()
	   <<"; legacyResourceName="<<legacyResourceName);

  return new KLFLibLegacyEngine(fileName, lrname, url, parent);
}


// private
KLFLibLegacyEngine::KLFLibLegacyEngine(const QString& fileName, const QString& resname, const QUrl& url,
				       QObject *parent)
  : KLFLibResourceSimpleEngine(url, FeatureReadOnly|FeatureLocked|FeatureSaveTo|FeatureSubResources, parent)
{
  // get the data object
  d = KLFLibLegacyFileDataPrivate::instanceFor(fileName, !isReadOnly());
  if (d == NULL) {
    qWarning()<<KLF_FUNC_NAME<<": Couldn't get KLFLibLegacyFileDataPrivate instance for "<<fileName<<"! Expect Crash!";
    return;
  }
  d->ref();

  connect(d, SIGNAL(resourcePropertyChanged(int)), this, SLOT(updateResourceProperty(int)));

  updateResourceProperty(-1);

  // add at least one resource (baptized resname) if the library is empty
  if (d->resources.isEmpty()) {
    // create a resource
    KLFLegacyData::KLFLibraryResource res;
    res.name = resname;
    res.id = d->getReservedResourceId(resname, KLFLegacyData::LibResourceUSERMAX - 1);
    d->resources << res;
    // and initialize pLibrary to contain this one empty resource.
    d->library.clear();
    d->library[res] = KLFLegacyData::KLFLibraryList();
    d->haschanges = true;
  }

  setReadOnly(isReadOnly() || d->flagForceReadOnly);

  klfDbg("Opened KLFLibLegacyEngine resource `"<<fileName<<"': d="<<d<<"; resources="<<d->resources
	 <<" (me="<<this<<", d="<<d<<")\n") ;
}

KLFLibLegacyEngine::~KLFLibLegacyEngine()
{
  KLF_DEBUG_BLOCK(KLF_FUNC_NAME) ;

  KLF_ASSERT_NOT_NULL( d , "d is NULL!" , return ) ;

  if ( ! d->deref() ) {
    klfDbg("last reference to the private liblegacyenginedataprivate object d="<<d<<", "
	   "saving(?) and deleting. haschanges="<<d->haschanges) ;
    klfDbg("resources dump:\n"<<d->resources<<"\nlibrary:\n"<<d->library) ;
    if (d->haschanges)
      d->save();
    delete d;
  }
}

uint KLFLibLegacyEngine::compareUrlTo(const QUrl& other, uint interestFlags) const
{
  // we can only test for these flags (see doc for KLFLibResourceEngine::compareUrlTo())
  interestFlags = interestFlags & (KlfUrlCompareBaseEqual);

  return klfUrlCompare(url(), other, interestFlags);
}


bool KLFLibLegacyEngine::canModifyData(const QString& subResource, ModifyType modifytype) const
{
  if ( ! KLFLibResourceEngine::canModifyData(subResource, modifytype) ) {
    klfDbg("base cannot modify resource engine...") ;
    return false;
  }

  KLF_ASSERT_NOT_NULL( d , "d is NULL!" , return false ) ;

#ifndef Q_WS_WIN
  // seems like windows doesn't like to test directories to be writable ...?

  if ( QFile::exists(d->fileName())  // depending on whether the file itself exists, check if
       ?  ! QFileInfo(d->fileName()).isWritable() // file itself writable
       :  ! QFileInfo(QFileInfo(d->fileName()).absolutePath()).isWritable() ) { // or containing dir writable
    return false;
  }
#endif

  return true;
}

bool KLFLibLegacyEngine::canModifyProp(int propid) const
{
  // all resource properties are stored in QVariantMap
  return KLFLibResourceEngine::canModifyProp(propid);
}

bool KLFLibLegacyEngine::canRegisterProperty(const QString& propName) const
{
  // all resource properties are stored in QVariantMap
  return canModifyProp(-1);
}

KLFLibEntry KLFLibLegacyEngine::entry(const QString& resource, entryId id)
{
  KLF_ASSERT_NOT_NULL( d , "d is NULL!" , return KLFLibEntry() ) ;

  int index = d->findResourceName(resource);
  if (index < 0)
    return KLFLibEntry();

  KLFLegacyData::KLFLibraryList ll = d->library[d->resources[index]];
  // find entry with id 'id'
  int k;
  for (k = 0; k < ll.size() && ll[k].id != (quint32)id; ++k)
    ;
  if (k == ll.size()) {
    return KLFLibEntry();
  }

  return d->toLibEntry(ll[k]);
}



QList<KLFLibResourceEngine::KLFLibEntryWithId>
/* */ KLFLibLegacyEngine::allEntries(const QString& resource, const QList<int>& wantedEntryProperties)
{
  KLF_ASSERT_NOT_NULL( d , "d is NULL!" , return QList<KLFLibEntryWithId>() ) ;

  int rindex = d->findResourceName(resource);
  if (rindex < 0)
    return QList<KLFLibEntryWithId>();

  QList<KLFLibEntryWithId> entryList;
  KLFLegacyData::KLFLibraryList ll = d->library[d->resources[rindex]];
  int k;
  for (k = 0; k < ll.size(); ++k) {
    KLFLibEntryWithId e;
    e.entry = d->toLibEntry(ll[k]);
    e.id = ll[k].id;
    entryList << e;
  }
  return entryList;
}


QStringList KLFLibLegacyEngine::subResourceList() const
{
  KLF_ASSERT_NOT_NULL( d , "d is NULL!" , return QStringList() ) ;

  QStringList list;
  int k;
  for (k = 0; k < d->resources.size(); ++k)
    list << d->resources[k].name;
  return list;
}

bool KLFLibLegacyEngine::canCreateSubResource() const
{
  // canModifyData is not sensitive to thses arguments...
  return canModifyData(QString(), ChangeData);
}
bool KLFLibLegacyEngine::canRenameSubResource(const QString& subResource) const
{
  return canModifyData(subResource, ChangeData);
}
bool KLFLibLegacyEngine::canDeleteSubResource(const QString& subResource) const
{
  return subResource.length() && hasSubResource(subResource) &&
    canModifyData(subResource, DeleteData) && subResourceList().size() > 1;
}

bool KLFLibLegacyEngine::createSubResource(const QString& subResource,
					   const QString& subResourceTitle)
{
  KLF_ASSERT_NOT_NULL( d , "d is NULL!" , return false ) ;

  quint32 newId = KLFLegacyData::LibResourceUSERMIN;
  bool ok = true;
  int k;
  while (!ok && newId <= KLFLegacyData::LibResourceUSERMAX) {
    for (k = 0; k < d->resources.size() && d->resources[k].id != newId; ++k)
      ;
    if (k == d->resources.size())
      ok = true;
    ++newId;
  }
  if (newId == KLFLegacyData::LibResourceUSERMAX) {
    qWarning()<<KLF_FUNC_NAME<<"("<<subResource<<",..): no new ID could be found (!?!)";
    return false;
  }
  KLFLegacyData::KLFLibraryResource res;
  res.name = subResource;
  res.id = d->getReservedResourceId(subResource, newId);

  d->resources.push_back(res);
  d->library[res] = KLFLegacyData::KLFLibraryList();
  d->haschanges = true;

  emit subResourceCreated(subResource);

  return true;
}

bool KLFLibLegacyEngine::renameSubResource(const QString& subResource, const QString& subResourceNewName)
{
  KLF_ASSERT_NOT_NULL( d , "d is NULL!" , return false ) ;
  int rindex = d->findResourceName(subResource);
  if (rindex < 0) {
    qWarning()<<KLF_FUNC_NAME<<": can't find sub-resource "<<subResource<<" in our data.";
    return false;
  }
  KLFLegacyData::KLFLibraryResource & resref = d->resources[rindex];
  // remove from library lists, keep the list
  KLFLegacyData::KLFLibraryList liblist = d->library.take(resref);

  // modify the resource data as requested
  resref.name = subResourceNewName;
  // see if the name we gave this resource is a 'reserved' name, eg. "History", or "Archive", that
  // have specific resource IDs.
  int possibleNewId = d->getReservedResourceId(subResource, -1);
  if (possibleNewId != -1)
    resref.id = possibleNewId;

  // re-insert into library list
  d->library[resref] = liblist;

  emit subResourceRenamed(subResource, subResourceNewName);
  return true;
}

bool KLFLibLegacyEngine::deleteSubResource(const QString& subResource)
{
  KLF_DEBUG_BLOCK(KLF_FUNC_NAME) ;
  klfDbg("sub-resource: "<<subResource) ;

  KLF_ASSERT_NOT_NULL( d , "d is NULL!" , return false ) ;
  if (!canDeleteSubResource(subResource)) {
    klfDbg("Cannot delete sub-resource "<<subResource) ;
    return false;
  }
  int rindex = d->findResourceName(subResource);
  if (rindex < 0) {
    qWarning()<<KLF_FUNC_NAME<<": can't find sub-resource "<<subResource<<" in our data.";
    return false;
  }

  // delete the data from our resource list and library

  KLFLegacyData::KLFLibraryResource res = d->resources.takeAt(rindex);
  // remove from library lists, keep the list
  d->library.remove(res);

  emit subResourceDeleted(subResource);
  return true;
}


bool KLFLibLegacyEngine::save()
{
  klfDbg( "() our url="<<url()<<"." ) ;
  if (isReadOnly()) {
    qWarning("KLFLibLegacyEngine::save: resource is read-only!");
    return false;
  }
  KLF_ASSERT_NOT_NULL( d , "d is NULL!" , return false ) ;

  return d->save();
}

void KLFLibLegacyEngine::setAutoSaveInterval(int intervalms)
{
  d->autoSaveTimer->stop();
  if (intervalms > 0) {
    d->autoSaveTimer->setInterval(intervalms);
    d->autoSaveTimer->start();
  }
}

QList<KLFLibResourceEngine::entryId> KLFLibLegacyEngine::insertEntries(const QString& subResource,
								       const KLFLibEntryList& entrylist)
{
  klfDbg("subResource="<<subResource<<"; entrylist="<<entrylist) ;

  KLF_ASSERT_NOT_NULL( d , "d is NULL!" , return QList<entryId>() ) ;

  if ( entrylist.size() == 0 )
     return QList<entryId>();
  if (!canModifyData(subResource, InsertData)) {
    klfDbg("cannot modify data.") ;
    return QList<entryId>();
  }

  int index = d->findResourceName(subResource);
  if (index < 0) {
    klfDbg("cannot find sub-resource: "<<subResource) ;
    return QList<entryId>();
  }

  QList<entryId> newIds;

  int k;
  for (k = 0; k < entrylist.size(); ++k) {
    KLFLegacyData::KLFLibraryItem item = d->toLegacyLibItem(entrylist[k]);
    d->library[d->resources[index]] << item;
    newIds << item.id;
  }
  d->haschanges = true;

  emit dataChanged(subResource, InsertData, newIds);

  klfDbg("finished inserting items. d="<<d<<"; dumping resources:\n"<<d->resources
	 <<"\nand library:\n"<<d->library) ;

  return newIds;
}

bool KLFLibLegacyEngine::changeEntries(const QString& subResource, const QList<entryId>& idlist,
				       const QList<int>& properties, const QList<QVariant>& values)
{
  if (idlist.size() == 0)
    return true;
  if (!canModifyData(subResource, ChangeData))
    return false;

  KLF_ASSERT_NOT_NULL( d , "d is NULL!" , return false ) ;

  int index = d->findResourceName(subResource);
  if (index < 0)
    return false;

  const KLFLegacyData::KLFLibraryList& ll = d->library[d->resources[index]];

  bool success = true;

  int k;
  for (k = 0; k < idlist.size(); ++k) {
    int libindex;
    int j;
    // find the entry
    for (libindex = 0; libindex < ll.size() && ll[libindex].id != (quint32)idlist[k]; ++libindex)
      ;
    if (libindex == ll.size()) {
      qWarning()<<KLF_FUNC_NAME<<": Can't find entry with id "<<idlist[k];
      success = false;
      continue;
    }
    // modify this entry as requested.
    for (j = 0; j < properties.size(); ++j) {
      switch (properties[j]) {
      case KLFLibEntry::Latex:
	// remember that entry.latex has redundancy for category+tags in the form "%: ...\n% ...\n<latex>"
	{ QString curcategory = d->library[d->resources[index]][libindex].category;
	  QString curtags = d->library[d->resources[index]][libindex].tags;
	  d->library[d->resources[index]][libindex].latex =
	    KLFLibEntry::latexAddCategoryTagsComment(values[j].toString(), curcategory, curtags);
	  break;
	}
      case KLFLibEntry::DateTime:
	d->library[d->resources[index]][libindex].datetime = values[j].toDateTime();
	break;
      case KLFLibEntry::Preview:
	d->library[d->resources[index]][libindex].preview = QPixmap::fromImage(values[j].value<QImage>());
	break;
      case KLFLibEntry::Category:
	// remember that entry.latex has redundancy for category+tags in the form "%: ...\n% ...\n<latex>"
	{ QString curlatex =
	    KLFLibEntry::stripCategoryTagsFromLatex(d->library[d->resources[index]][libindex].latex);
	  QString newcategory = values[j].toString();
	  QString curtags = d->library[d->resources[index]][libindex].tags;
	  d->library[d->resources[index]][libindex].latex =
	    KLFLibEntry::latexAddCategoryTagsComment(curlatex, newcategory, curtags);
	  d->library[d->resources[index]][libindex].category = newcategory;
	  break;
	}
      case KLFLibEntry::Tags:
	// remember that entry.latex has redundancy for category+tags in the form "%: ...\n% ...\n<latex>"
	{ QString curlatex =
	    KLFLibEntry::stripCategoryTagsFromLatex(d->library[d->resources[index]][libindex].latex);
	  QString curcategory = d->library[d->resources[index]][libindex].category;
	  QString newtags = values[j].toString();
	  d->library[d->resources[index]][libindex].latex =
	    KLFLibEntry::latexAddCategoryTagsComment(curlatex, curcategory, newtags);
	  d->library[d->resources[index]][libindex].tags = newtags;
	  break;
	}
	break;
      case KLFLibEntry::Style:
	d->library[d->resources[index]][libindex].style = d->toLegacyStyle(values[j].value<KLFStyle>());
	break;
      default:
	qWarning()<<KLF_FUNC_NAME<<": Cannot set arbitrary property "<<propertyNameForId(properties[j])
		  <<"!";
	success = false;
	break;
      }
    }
  }

#ifdef KLF_DEBUG
  klfDbg( ": Changed entries. Dump:" ) ;
  const KLFLegacyData::KLFLibraryList& ll2 = d->library[d->resources[index]];
  int kl;
  for (kl = 0; kl < ll2.size(); ++kl)
    klfDbg( "\t#"<<kl<<": "<<ll2[kl].latex<<" - "<<ll2[kl].category ) ;
#endif

  d->haschanges = true;

  emit dataChanged(subResource, ChangeData, idlist);

  return success;
}

bool KLFLibLegacyEngine::deleteEntries(const QString& subResource, const QList<entryId>& idlist)
{
  KLF_ASSERT_NOT_NULL( d , "d is NULL!" , return false ) ;

  if (idlist.isEmpty())
    return true;
  if (!canModifyData(subResource, DeleteData))
    return false;

  int index = d->findResourceName(subResource);
  if (index < 0)
    return false;

  // now remove the requested entries

  KLFLegacyData::KLFLibraryList *ll = & d->library[d->resources[index]];
  bool success = true;
  int k;
  for (k = 0; k < idlist.size(); ++k) {
    int j;
    for (j = 0; j < ll->size() && ll->operator[](j).id != (quint32)idlist[k]; ++j)
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

  d->haschanges = true;

  emit dataChanged(subResource, DeleteData, idlist);

  return success;
}

bool KLFLibLegacyEngine::saveTo(const QUrl& newPath)
{
  KLF_ASSERT_NOT_NULL( d , "d is NULL!" , return false ) ;

  if (newPath.scheme() == "klf+legacy") {
    return d->save(klfUrlLocalFilePath(newPath));
  }
  return false;
}

bool KLFLibLegacyEngine::saveResourceProperty(int propId, const QVariant& value)
{
  KLF_DEBUG_BLOCK(KLF_FUNC_NAME) ;

  QVariantMap m = d->metadata["ResProps"].toMap();

  QString propName = propertyNameForId(propId);
  if ( propName.isEmpty() )
    return false;

  if (m.contains(propName) && m[propName] == value)
    return true; // nothing to do

  m[propName] = value;

  d->metadata["ResProps"] = QVariant(m);
  d->haschanges = true;

  d->emitResourcePropertyChanged(propId);

  return true;
}

void KLFLibLegacyEngine::updateResourceProperty(int propId)
{
  KLF_DEBUG_BLOCK(KLF_FUNC_NAME) ;
  klfDbg("property id="<<propId) ;

  // need KLFPropertizedObject:: explicit scopes to disambiguate from QObject's functions

  const QMap<QString,QVariant> resprops = d->metadata["ResProps"].toMap();

  if (propId < 0) {
    // read and update all the properties
    KLFPropertizedObject::setAllProperties(resprops);
    // set some default values for some properties if they have not been set
    if (!KLFPropertizedObject::property(PropLocked).isValid())
      KLFPropertizedObject::setProperty(PropLocked, QVariant(false));
    if (!KLFPropertizedObject::property(PropAccessShared).isValid())
      KLFPropertizedObject::setProperty(PropAccessShared, QVariant::fromValue(false));
    if (!KLFPropertizedObject::property(PropTitle).isValid())
      KLFPropertizedObject::setProperty(PropTitle, QFileInfo(d->fileName()).baseName());
  } else {
    QString propName = KLFPropertizedObject::propertyNameForId(propId);
    KLFPropertizedObject::setProperty(propId, resprops[propName]);
  }
  emit resourcePropertyChanged(propId);
}


// ------------------------------------


QString KLFLibLegacyLocalFileSchemeGuesser::guessScheme(const QString& fileName) const
{
  klfDbg("file "<<fileName);

  if (fileName.endsWith(".klf")) {
    klfDbg("has .klf extension.") ;
    return QLatin1String("klf+legacy");
  }

  QFile f(fileName);
  if ( ! f.open(QIODevice::ReadOnly) ) {
    qWarning()<<KLF_FUNC_NAME<<": Can't open file: "<<fileName;
    return QString();
  }
  QDataStream stream(&f);
  // Qt3-compatible stream input
  stream.setVersion(QDataStream::Qt_3_3);
  QString s1;
  stream >> s1;
  klfDbg("read line: got magic "<<s1);
  if (s1 == QLatin1String("KLATEXFORMULA_LIBRARY_EXPORT") ||
      s1 == QLatin1String("KLATEXFORMULA_LIBRARY") ||
      s1 == QLatin1String("KLATEXFORMULA_HISTORY"))
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
    return tr("KLatexFormula Library Export File");
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



