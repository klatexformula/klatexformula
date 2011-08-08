/***************************************************************************
 *   file klfconfigbase.h
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


#ifndef KLFCONFIGBASE_H_
#define KLFCONFIGBASE_H_

#include <QString>
#include <QVariant>
#include <QObject>

#include <klfdefs.h>
#include <klfutil.h>

class KLF_EXPORT KLFConfigPropBase
{
public:
  KLFConfigPropBase();
  virtual ~KLFConfigPropBase();

  virtual QString propName() const { return pname; }
  virtual QVariant toVariant() const = 0;

  virtual bool setValue(const QVariant& newvalue) { return false; }

protected:
  QString pname;
};


class KLF_EXPORT KLFConfigBase
{
public:
  virtual bool okChangeProperty(KLFConfigPropBase *property, const QVariant& oldValue, const QVariant& newValue);

  virtual void propertyChanged(KLFConfigPropBase *property, const QVariant& oldValue, const QVariant& newValue);

  virtual void propertyValueRequested(const KLFConfigPropBase *property);

  /**
   * \warning The caller must garantee that \c object will not be destroyed! If this will be the case,
   *   disconnect it first with disconnectQObjectProperty() !
   *
   * \note the \c object's property \c objPropName is initialized to the current configuration value. */
  virtual void connectQObjectProperty(const QString& configPropertyName, QObject *object,
				      const QByteArray& objPropName);
  virtual void disconnectQObjectProperty(const QString& configPropertyName, QObject *object,
					 const QByteArray& objPropName);

  /** disconnects all connections concerning this object */
  virtual void disconnectQObject(QObject * object);

  virtual QStringList propertyList() const;
  KLFConfigPropBase * property(const QString& name);

  /** \note ONLY TO BE CALLED BY KLFConfigProp !! */
  inline void registerConfigProp(KLFConfigPropBase *p) { pProperties.append(p); }

protected:
  struct ObjPropConnection {
    QObject *object;
    QByteArray objPropName;
    inline bool operator==(const ObjPropConnection& b) const {
      return object == b.object && objPropName == b.objPropName;
    }
  };
  QHash<QString, QList<ObjPropConnection> > pObjPropConnections;

  QList<KLFConfigPropBase*> pProperties;
};




template<class T>
class KLFConfigProp : public KLFConfigPropBase
{
public:
  typedef T Type;

  KLFConfigProp() : config(NULL), val(T()), defval(T()) { }

  operator Type () const
  {
    return value();
  }
  Type operator()() const
  {
    return value();
  }
  const Type& operator=(const Type& newvalue)
  {
    setValue(newvalue);
    return newvalue;
  };
  bool operator==(const Type& compareValue)
  {
    return val == compareValue;
  }
  bool operator!=(const Type& compareValue)
  {
    return val != compareValue;
  }

  bool setValue(const QVariant& newvalue)
  {
    return setValue(KLFVariantConverter<T>::recover(newvalue));
  }

  bool setValue(const Type& newvalue)
  {
    Type oldvalue = val;
    KLF_ASSERT_NOT_NULL(config, "we ("<<pname<<") have not been initialized!", return false; ) ;
    KLFVariantConverter<Type> vc;
    if (!config->okChangeProperty(this, vc.convert(oldvalue), vc.convert(newvalue))) {
      return false;
    }
    val = newvalue;
    config->propertyChanged(this, vc.convert(oldvalue), vc.convert(newvalue));
    return true;
  }

  Type defaultValue() const
  {
    return defval;
  }
  Type value() const
  {
    KLF_ASSERT_NOT_NULL(config, "we ("<<pname<<") have not been initialized!", return Type(); ) ;
    config->propertyValueRequested(this);
    return val;
  }
  virtual QVariant toVariant() const
  {
    KLFVariantConverter<Type> v;
    return v.convert(value());
  }
  
  void initialize(KLFConfigBase *confptr, const QString& propName, const Type& defaultValue)
  {
    KLF_ASSERT_NOT_NULL(confptr, "Cannot use a NULL config pointer!!", ; );
    config = confptr;
    config->registerConfigProp(this);
    pname = propName;
    defval = defaultValue;
    val = defaultValue;
  }

  void connectQObjectProperty(QObject *object, const QByteArray& propName)
  {
    KLF_ASSERT_NOT_NULL(config, "we ("<<pname<<") are not initialized!", return; ) ;

    config->connectQObjectProperty(pname, object, propName) ;
  }

private:
  KLFConfigBase *config;

  Type val;
  Type defval;
};

#define KLFCONFIGPROP_INIT_CONFIG(configptr)  KLFConfigBase *__klfconfigprop_configbase = (configptr) ;
#define KLFCONFIGPROP_INIT(var, defval) (var).initialize(__klfconfigprop_configbase, #var, (defval))




#endif
