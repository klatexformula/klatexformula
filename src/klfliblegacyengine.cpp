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
  if (!url.hasQueryItem("legacyResourceName")) {
    qWarning()<<"Please specify a 'legacyResourceName' query item in URL.";
    return NULL;
  }
  QString legresname = url.queryItemValue("legacyResourceName");

  QString fname = url.path();
  if ( ! QFileInfo(fname).isReadable() ) {
    qWarning("KLFLibLegacyEngine::openUrl(): file %s does not exist!", qPrintable(fname));
    return NULL;
  }

  if (url.scheme() != "klf+legacy") {
    qWarning("KLFLibLegacyEngine::openUrl(): unsupported scheme %s!", qPrintable(url.scheme()));
    return NULL;
  }

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
  bool r = saveLibraryFile(fileName, reslist, lib);
  if (!r)
    return false;

  return new KLFLibLegacyEngine(fileName, lrname, url, parent);
}

// private, static
bool KLFLibLegacyEngine::loadLibraryFile(const QString& fname, KLFLegacyData::KLFLibraryResourceList *reslist,
					 KLFLegacyData::KLFLibrary *lib)
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
    qint16 vmaj, vmin;
    stream >> vmaj >> vmin; // these are not needed, format has not changed in .klf export files.
    stream >> *reslist >> *lib;
  } else if (s1 == "KLATEXFORMULA_LIBRARY") {
    // opening a library file (~/.klatexformula/library)
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
bool KLFLibLegacyEngine::saveLibraryFile(const QString& fname, const KLFLegacyData::KLFLibraryResourceList& reslist,
					 const KLFLegacyData::KLFLibrary& lib)
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
  stream << QString("KLATEXFORMULA_LIBRARY_EXPORT") << (qint16)2 << (qint16)1
	 << reslist << lib;
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
  : KLFLibResourceSimpleEngine(url, FeatureReadOnly|FeatureSaveAs, parent)
{
  // load library
  pFileName = fileName;
  loadLibraryFile(fileName, &pResources, &pLibrary);

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
    pCurRes = resname;
    pCurResIndex = 0;
  } else {
    pCurRes = resname;
    pCurResIndex = findResourceName(resname);
    if (pCurResIndex < 0 || resname.isEmpty()) {
      // specified resource name doesn't exist. Open first in list.
      pCurResIndex = 0;
      pCurRes = pResources[pCurResIndex].name;
    }
  }

  KLFPropertizedObject::setProperty(PropAccessShared, QVariant::fromValue(false));
  KLFPropertizedObject::setProperty(PropTitle, resname);

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

bool KLFLibLegacyEngine::canModifyData(ModifyType modifytype) const
{
  if ( ! KLFLibResourceEngine::canModifyData(modifytype) )
    return false;
  /// \todo TODO: check if file is writable ..............

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

KLFLibEntry KLFLibLegacyEngine::entry(entryId id)
{
  KLFLegacyData::KLFLibraryList ll = pLibrary[pResources[pCurResIndex]];
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


QList<KLFLibResourceEngine::KLFLibEntryWithId> KLFLibLegacyEngine::allEntries()
{
  return allEntriesInResource(pCurRes);
}

QList<KLFLibResourceEngine::KLFLibEntryWithId> KLFLibLegacyEngine::allEntriesInResource(const QString& resource)
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


QStringList KLFLibLegacyEngine::getLegacyResourceNames(const QUrl& url)
{
  KLFLibLegacyEngine *resource = openUrl(url, NULL);
  QStringList resnamelist;
  int k;
  for (k = 0; k < resource->pResources.size(); ++k)
    resnamelist << resource->pResources[k].name;

  delete resource; // immediately close the resource

  return resnamelist;
}

bool KLFLibLegacyEngine::save()
{
  if (isReadOnly()) {
    qWarning("KLFLibLegacyEngine::save: resource is read-only!");
    return false;
  }
  return saveLibraryFile(pFileName, pResources, pLibrary);
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

bool KLFLibLegacyEngine::setCurrentLegacyResource(const QString& curLegacyResource)
{
  int index = findResourceName(curLegacyResource);
  if (index < 0) {
    qWarning()<<"Bad legacy resource name: "<<curLegacyResource;
    return false;
  }

  pCurRes = curLegacyResource;
  pCurResIndex = index;
  emit dataChanged(QList<KLFLib::entryId>()); // data "changed" completely
}

QList<KLFLibResourceEngine::entryId> KLFLibLegacyEngine::insertEntries(const KLFLibEntryList& entrylist)
{
  if ( entrylist.size() == 0 )
     return QList<entryId>();
  if (!canModifyData(InsertData))
    return QList<entryId>();

  QList<entryId> newIds;

  int k;
  for (k = 0; k < entrylist.size(); ++k) {
    KLFLegacyData::KLFLibraryItem item = toLegacyLibItem(entrylist[k]);
    pLibrary[pResources[pCurResIndex]] << item;
    newIds << item.id;
  }

  return newIds;
}
bool KLFLibLegacyEngine::changeEntries(const QList<entryId>& idlist, const QList<int>& properties,
				       const QList<QVariant>& values)
{
  if (idlist.size() == 0)
    return true;
  if (!canModifyData(ChangeData))
    return false;

  const KLFLegacyData::KLFLibraryList& ll = pLibrary[pResources[pCurResIndex]];

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
	pLibrary[pResources[pCurResIndex]][libindex].latex = values[j].toString();
	break;
      case KLFLibEntry::DateTime:
	pLibrary[pResources[pCurResIndex]][libindex].datetime = values[j].toDateTime();
	break;
      case KLFLibEntry::Preview:
	pLibrary[pResources[pCurResIndex]][libindex].preview = QPixmap::fromImage(values[j].value<QImage>());
	break;
      case KLFLibEntry::Category:
	pLibrary[pResources[pCurResIndex]][libindex].category = values[j].toString();
	break;
      case KLFLibEntry::Tags:
	pLibrary[pResources[pCurResIndex]][libindex].tags = values[j].toString();
	break;
      case KLFLibEntry::Style:
	pLibrary[pResources[pCurResIndex]][libindex].style = toLegacyStyle(values[j].value<KLFStyle>());
	break;
      default:
	qWarning("KLFLibLegacyEngine::changeEntries: Cannot set arbitrary property '%s'.",
		 qPrintable(propertyNameForId(properties[j])));
	success = false;
	break;
      }
    }
  }
  return success;
}

bool KLFLibLegacyEngine::deleteEntries(const QList<entryId>& idlist)
{
  if (idlist.isEmpty())
    return true;
  if (!canModifyData(DeleteData))
    return false;

  // now remove the requested entries

  KLFLegacyData::KLFLibraryList *ll = & pLibrary[pResources[pCurResIndex]];
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
  return success;
}

bool KLFLibLegacyEngine::saveAs(const QUrl& newPath)
{
  if (newPath.scheme() == "klf+legacy") {
    return saveLibraryFile(newPath.path(), pResources, pLibrary);
  }
  return false;
}

bool KLFLibLegacyEngine::saveResourceProperty(int propId, const QVariant& value)
{
  return false; // can't change anything.
}


// ------------------------------------


KLFLibLegacyEngineFactory::KLFLibLegacyEngineFactory(QObject *parent)
  : KLFLibEngineFactory(parent)
{
}

QStringList KLFLibLegacyEngineFactory::supportedSchemes() const
{
  return QStringList() << QLatin1String("klf+legacy");
}

QString KLFLibLegacyEngineFactory::schemeTitle(const QString& scheme) const
{
  if (scheme == QLatin1String("klf+legacy"))
    return tr("KLatexFormula 3.1 Library Export File");
  return QString();
}

KLFLibResourceEngine *KLFLibLegacyEngineFactory::openResource(const QUrl& location, QObject *parent)
{
  return KLFLibLegacyEngine::openUrl(location, parent);
}

QWidget * KLFLibLegacyEngineFactory::createPromptUrlWidget(QWidget *parent, const QString& scheme,
							   QUrl defaultlocation)
{
  if (scheme == QLatin1String("klf+legacy")) {
    KLFLibLegacyOpenWidget *w = new KLFLibLegacyOpenWidget(parent);
    w->setUrl(defaultlocation);
    return w;
  }
  return NULL;
}

QUrl KLFLibLegacyEngineFactory::retrieveUrlFromWidget(const QString& scheme, QWidget *widget)
{
  if (scheme == "klf+legacy") {
    if (widget == NULL || !widget->inherits("KLFLibLegacyOpenWidget")) {
      qWarning("KLFLibLegacyEngineFactory::retrieveUrlFromWidget(): Bad Widget provided!");
      return QUrl();
    }
    return qobject_cast<KLFLibLegacyOpenWidget*>(widget)->url();
  } else {
    qWarning()<<"Bad scheme: "<<scheme;
    return QUrl();
  }
}


QWidget *KLFLibLegacyEngineFactory::createPromptCreateParametersWidget(QWidget *parent,
								       const QString& scheme,
								       const Parameters& defaultparameters)
{
  if (scheme == QLatin1String("klf+legacy")) {
    KLFLibLegacyCreateWidget *w = new KLFLibLegacyCreateWidget(parent);
    w->setUrl(defaultparameters["Url"].toUrl());
    return w;
  }
  qWarning()<<"KLFLibLegacyEngineFactory::createP.C.P.Widget: Bad Scheme: "<<scheme;
  return NULL;
}

KLFLibEngineFactory::Parameters
/* */ KLFLibLegacyEngineFactory::retrieveCreateParametersFromWidget(const QString& scheme,
								QWidget *widget)
{
  if (scheme == QLatin1String("klf+legacy")) {
    if (widget == NULL || !widget->inherits("KLFLibLegacyCreateWidget")) {
      qWarning("KLFLibLegacyEngineFactory::retrieveUrlFromWidget(): Bad Widget provided!");
      return Parameters();
    }
    KLFLibLegacyCreateWidget *w = qobject_cast<KLFLibLegacyCreateWidget*>(widget);
    Parameters p;
    QString filename = w->fileName();
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
      // remove the previous file, because otherwise KLFLibLegacyEngine will fail.
      bool r = QFile::remove(filename);
      if ( !r ) {
	QMessageBox::critical(widget, tr("Error"), tr("Failed to overwrite the file %1.")
			      .arg(filename));
	return Parameters();
      }
    }

    p["Filename"] = w->fileName();
    p["Url"] = w->url();
    return p;
  }

  return Parameters();
}

KLFLibResourceEngine *KLFLibLegacyEngineFactory::createResource(const QString& scheme,
								const Parameters& parameters,
								QObject *parent)
{
  if (scheme == QLatin1String("klf+legacy")) {
    if ( !parameters.contains("Filename") || !parameters.contains("Url") ) {
      qWarning()
	<<"KLFLibLegacyEngineFactory::createResource: bad parameters. They do not contain `Filename' or\n"
	"`Url': "<<parameters;
      return NULL;
    }
    return KLFLibLegacyEngine::createDotKLF(parameters["Filename"].toString(),
					    parameters["legacyResourceName"].toString(), parent);
  }
  return NULL;
}



