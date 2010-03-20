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

#include "klflib_p.h"
#include "klflib.h"


KLFLibEntry::KLFLibEntry(const QString& latex, const QDateTime& dt, const QImage& preview,
			 const QString& category, const QString& tags, const KLFStyle& style)
  : KLFPropertizedObject("KLFLibEntry")
{
  initRegisteredProperties();
  setLatex(latex);
  setDateTime(dt);
  setPreview(preview);
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
  registerBuiltInProperty(Category, "Category");
  registerBuiltInProperty(Tags, "Tags");
  registerBuiltInProperty(Style, "Style");
}


// ---------------------------------------------------

KLFLibResourceEngine::KLFLibResourceEngine(const QUrl& url, uint featureflags,
					   QObject *parent)
  : QObject(parent), KLFPropertizedObject("KLFLibResourceEngine"), pUrl(url),
    pFeatureFlags(featureflags), pReadOnly(false)
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
}

bool KLFLibResourceEngine::canModifyData(ModifyType /*modifytype*/) const
{
  return !isReadOnly() && !locked();
}
bool KLFLibResourceEngine::canModifyProp(int propId) const
{
  if (propId == PropLocked)
    return !isReadOnly() && (pFeatureFlags & FeatureLocked);

  return !isReadOnly() && !locked();
}
bool KLFLibResourceEngine::canRegisterProperty(const QString& /*propName*/) const
{
  return false;
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


KLFLibResourceEngine::entryId KLFLibResourceEngine::insertEntry(const KLFLibEntry& entry)
{
  QList<entryId> ids = insertEntries(KLFLibEntryList() << entry);
  if (ids.size() == 0)
    return -1;

  return ids[0];
}

bool KLFLibResourceEngine::saveAs(const QUrl&)
{
  // not permitted by default. Subclasses must reimplement
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


QList<KLFLibEngineFactory*> KLFLibEngineFactory::pRegisteredFactories =
	 QList<KLFLibEngineFactory*>();

KLFLibEngineFactory::KLFLibEngineFactory(QObject *parent)
  : QObject(parent)
{
  registerFactory(this);
}
KLFLibEngineFactory::~KLFLibEngineFactory()
{
  unRegisterFactory(this);
}

bool KLFLibEngineFactory::canCreateResource(const QString& /*scheme*/) const
{
  return false;
}

QWidget * KLFLibEngineFactory::createPromptCreateParametersWidget(QWidget */*parent*/,
									  const QString& /*scheme*/,
									  const Parameters& /*par*/)
{
  return NULL;
}
KLFLibEngineFactory::Parameters
/* */ KLFLibEngineFactory::retrieveCreateParametersFromWidget(const QString& /*scheme*/,
								      QWidget */*parent*/)
{
  return Parameters();
}
KLFLibResourceEngine *KLFLibEngineFactory::createResource(const QString& /*scheme*/,
								  const Parameters& /*param*/,
								  QObject */*parent*/)
{
  return NULL;
}

bool KLFLibEngineFactory::canResourceSaveAs(const QString& /*scheme*/) const
{
  return false;
}
QWidget *KLFLibEngineFactory::createPromptSaveAsWidget(QWidget */*parent*/,
						       const QString& /*scheme*/,
						       const QUrl& /*defaultUrl*/)
{
  return NULL;
}
QUrl KLFLibEngineFactory::retrieveSaveAsUrlFromWidget(const QString& /*scheme*/,
						      QWidget */*widget*/)
{
  return QUrl();
}





KLFLibEngineFactory *KLFLibEngineFactory::findFactoryFor(const QString& urlScheme)
{
  int k;
  // walk registered factories, and return the first that supports this scheme.
  for (k = 0; k < pRegisteredFactories.size(); ++k) {
    if (pRegisteredFactories[k]->supportedSchemes().contains(urlScheme))
      return pRegisteredFactories[k];
  }
  // no factory found
  return NULL;
}

QStringList KLFLibEngineFactory::allSupportedSchemes()
{
  QStringList schemes;
  int k;
  for (k = 0; k < pRegisteredFactories.size(); ++k) {
    schemes << pRegisteredFactories[k]->supportedSchemes();
  }
  return schemes;
}

void KLFLibEngineFactory::registerFactory(KLFLibEngineFactory *factory)
{
  if (pRegisteredFactories.indexOf(factory) != -1)
    return;
  pRegisteredFactories.append(factory);
}

void KLFLibEngineFactory::unRegisterFactory(KLFLibEngineFactory *factory)
{
  if (pRegisteredFactories.indexOf(factory) == -1)
    return;
  pRegisteredFactories.removeAll(factory);
}


