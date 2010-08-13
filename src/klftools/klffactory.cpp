/***************************************************************************
 *   file klffactory.cpp
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
#include <QStringList>

#include "klffactory.h"


KLFFactoryBase::KLFFactoryBase(KLFFactoryManager *factoryManager)
  : pFactoryManager(factoryManager)
{
  pFactoryManager->registerFactory(this);
}
KLFFactoryBase::~KLFFactoryBase()
{
  pFactoryManager->unRegisterFactory(this);
}

KLFFactoryManager::KLFFactoryManager()
{
}
KLFFactoryManager::~KLFFactoryManager()
{
}

KLFFactoryBase * KLFFactoryManager::findFactoryFor(const QString& objType)
{
  int k;
  for (k = 0; k < pRegisteredFactories.size(); ++k) {
    KLFFactoryBase * factory = pRegisteredFactories[k];
    if (factory->supportedTypes().contains(objType)) {
      return factory;
    }
  }
  qWarning()
    <<"KLFFactoryManager::findFactoryFor(object type="<<objType<<"): No factory found!";

  return NULL;
}

QStringList KLFFactoryManager::allSupportedTypes()
{
  QStringList objtypes;
  int k;
  for (k = 0; k < pRegisteredFactories.size(); ++k) {
    objtypes << pRegisteredFactories[k]->supportedTypes();
  }
  return objtypes;
}

void KLFFactoryManager::registerFactory(KLFFactoryBase *factory)
{
  if (pRegisteredFactories.indexOf(factory) != -1) {
    qWarning()<<"KLFFactory<>::registerFactory(): Factory " << factory << " is already registered!";
    return;
  }
  pRegisteredFactories.prepend(factory);
}

void KLFFactoryManager::unRegisterFactory(KLFFactoryBase *factory)
{
  if (pRegisteredFactories.indexOf(factory) == -1) {
    qWarning()<<"KLFFactory<>::unRegisterFactory(): Factory "<<factory<<" is not registered!";
    return;
  }
  pRegisteredFactories.removeAll(factory);
}


