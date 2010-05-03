/***************************************************************************
 *   file klffactory.h
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

#ifndef KLFFACTORY_H
#define KLFFACTORY_H

#include <QString>
#include <QList>

#include <klfdefs.h>


class KLFFactoryManager;

//! Base class for factories
/** See \ref KLFactoryManager.
 *
 * This class automatically registers to the factory manager given to the constructor; it
 * also automatically unregisters itself in the destructor. */
class KLFFactoryBase
{
public:
  KLFFactoryBase(KLFFactoryManager *factoryManager);
  virtual ~KLFFactoryBase();

  //! A list of object types that this factory supports instantiating
  virtual QStringList supportedTypes() const = 0;

private:
  KLFFactoryManager *pFactoryManager;
};


//! A base abstract factory manager class
/** An abstract class that provides base common functions for factories of different kinds.
 *
 * A Factory is a class that can instantiate another class of given types. For example, you may have
 * a factory that can create objects of the correct sub-class of KLFLibResourceEngine depending
 * on the URL to open. Multiple factories can be installed, each capable of opening one or more subtypes
 * of the given object, a library resource in our example.
 *
 * Factories need to be explicitely registered and unregistered, they are done so in the
 * KLFFactoryBase constructor and destructor.
 *
 * The usage of KLFFactoryManager and KLFFactoryBase is for example:
 * \code
 *  // in .h:
 *  class MyFactory : public KLFFactoryBase {
 *  public:
 *    MyFactory() : KLFFactoryBase(&pFactoryManager) { }
 *
 *    virtual QStringList supportedTypes() const { ..... }
 *
 *    ... virtual MyObject * createMyObject(const QString& ofThisObjectType) = 0; ...
 *
 *    ...
 *
 *    MyFactory * findFactoryFor(...) {
 *      return dynamic_cast<MyFactory*>(pFactoryManager.findFactoryFor(...));
 *    }
 *
 *  private:
 *    static KLFFactoryManager pFactoryManager;
 *  };
 *
 *  // in .cpp:
 *  KLFFactoryManager MyFactory::pFactoryManager ;
 * \endcode
 */
class KLF_EXPORT KLFFactoryManager
{
public:
  /** Constructor. does nothing*/
  KLFFactoryManager();
  /** Destructor. This function unregisters the factory. */
  virtual ~KLFFactoryManager();

  /** Returns the first factory in registered factory list that is capable of creating
   * an object of type objType. */
  KLFFactoryBase * findFactoryFor(const QString& objType);

  /** Returns a combined list of all object types all registered factories combined support.
   * (ie. a list of all object types we're capable of instantiating) */
  QStringList allSupportedTypes();
  /** Returns a list of all registered factories. */
  inline QList<KLFFactoryBase*> registeredFactories() { return pRegisteredFactories; }

private:
  //! List of registered factories
  QList<KLFFactoryBase*> pRegisteredFactories;

  //! Registers a factory
  void registerFactory(KLFFactoryBase *factory);
  //! Unregisters a factory
  void unRegisterFactory(KLFFactoryBase *factory);

  friend class KLFFactoryBase;
};




#endif
