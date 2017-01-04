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

  virtual bool setValue(const QVariant& newvalue) { Q_UNUSED(newvalue); return false; }

  virtual QVariant defaultValueVariant() const { return QVariant(); }

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
  /**
   * \warning The caller must garantee that \c object will not be destroyed! If this will be the case,
   *   disconnect it first with disconnectQObjectProperty() !
   *
   * \note the \c object's slot \c slotMethodName is called immediately, too, with the current configuration
   *   value. */
  virtual void connectQObjectSlot(const QString& configPropertyName, QObject *object,
				  const QByteArray& slotMethodName);
  virtual void disconnectQObjectSlot(const QString& configPropertyName, QObject *object,
				     const QByteArray& slotMethodName);

  /** disconnects all connections concerning this object */
  virtual void disconnectQObject(QObject * object);

  virtual QStringList propertyList() const;
  KLFConfigPropBase * property(const QString& name);

  /** \note ONLY TO BE CALLED BY KLFConfigProp !! */
  inline void registerConfigProp(KLFConfigPropBase *p) { pProperties.append(p); }

protected:
  enum ConnectionTarget { Property, Slot };
  struct ObjConnection {
    ConnectionTarget target;
    QObject *object;
    QByteArray targetName;
    inline bool operator==(const ObjConnection& b) const {
      return target == b.target && object == b.object && targetName == b.targetName;
    }
  };
  QHash<QString, QList<ObjConnection> > pObjConnections;

  void connectQObject(const QString& configPropertyName, QObject *object,
		      ConnectionTarget target, const QByteArray& targetName);
  void disconnectQObject(const QString& configPropertyName, QObject *object,
			 ConnectionTarget target, const QByteArray& targetName);

  QList<KLFConfigPropBase*> pProperties;
};




template<class T>
class KLFConfigProp : public KLFConfigPropBase
{
public:
  typedef T Type;

  KLFConfigProp() : config(NULL), val(T()), defval(T()), isdefaultvaluedefinite(false) { }


  operator Type () const
  {
    return value();
  }
  const Type operator()() const
  {
    return value();
  }
  const Type& operator=(const Type& newvalue)
  {
    setValue(newvalue);
    return newvalue;
  };
  bool operator==(const Type& compareValue) const
  {
    return value() == compareValue;
  }
  bool operator!=(const Type& compareValue) const
  {
    return value() != compareValue;
  }

  bool setValue(const QVariant& newvalue)
  {
    return setValue(KLFVariantConverter<T>::recover(newvalue));
  }

  bool setValue(const Type& newvalue)
  {
    Type oldvalue = value();
    KLF_ASSERT_NOT_NULL(config, "we ("<<pname<<") have not been initialized!", return false; ) ;
    KLFVariantConverter<Type> vc;
    if (!config->okChangeProperty(this, vc.convert(oldvalue), vc.convert(newvalue))) {
      return false;
    }
    val = newvalue;
    valisset = true;
    config->propertyChanged(this, vc.convert(oldvalue), vc.convert(newvalue));
    return true;
  }

  Type defaultValue() const
  {
    return defval;
  }
  bool defaultValueDefinite() const
  {
    return isdefaultvaluedefinite;
  }
  Type value() const
  {
    KLF_ASSERT_NOT_NULL(config, "we ("<<pname<<") have not been initialized!", return Type(); ) ;
    config->propertyValueRequested(this);

    if (valisset)
      return val;
    return defval;
  }
  bool hasValue() const
  {
    return valisset;
  }

  virtual QVariant toVariant() const
  {
    KLFVariantConverter<Type> v;
    return v.convert(value());
  }

  virtual QVariant defaultValueVariant() const
  {
    KLFVariantConverter<Type> v;
    return v.convert(defaultValue());
  }
  
  void initialize(KLFConfigBase *confptr, const QString& propName, const Type& defaultValue,
                  bool isDefaultValueDefinite = true)
  {
    KLF_ASSERT_NOT_NULL(confptr, "Cannot use a NULL config pointer!!", ; );
    config = confptr;
    config->registerConfigProp(this);
    pname = propName;
    defval = defaultValue;
    valisset = false;
    val = Type();
    isdefaultvaluedefinite = isDefaultValueDefinite;
  }

  void setDefaultValue(const Type& defaultValue)
  {
    defval = defaultValue;
    isdefaultvaluedefinite = true;
  }

  void connectQObjectProperty(QObject *object, const QByteArray& propName)
  {
    KLF_ASSERT_NOT_NULL(config, "we ("<<pname<<") are not initialized!", return; ) ;

    config->connectQObjectProperty(pname, object, propName) ;
  }
  void disconnectQObjectProperty(QObject *object, const QByteArray& propName)
  {
    KLF_ASSERT_NOT_NULL(config, "we ("<<pname<<") are not initialized!", return; ) ;

    config->disconnectQObjectProperty(pname, object, propName) ;
  }

  void connectQObjectSlot(QObject *object, const QByteArray& slotName)
  {
    KLF_ASSERT_NOT_NULL(config, "we ("<<pname<<") are not initialized!", return; ) ;

    config->connectQObjectSlot(pname, object, slotName) ;
  }
  void disconnectQObjectSlot(QObject *object, const QByteArray& slotName)
  {
    KLF_ASSERT_NOT_NULL(config, "we ("<<pname<<") are not initialized!", return; ) ;

    config->disconnectQObjectSlot(pname, object, slotName) ;
  }

private:
  KLFConfigBase *config;

  bool valisset;
  Type val;
  Type defval;
  /** This flag is used for settings which are initialized to some generic default value, but
   * then their default value needs to be reset to some detected value. For example,
   * BackendSettings.execLatex in KLFConfig.
   */
  bool isdefaultvaluedefinite;
};

#define KLFCONFIGPROP_INIT_CONFIG(configptr)  KLFConfigBase *__klfconfigprop_configbase = (configptr) ;
#define KLFCONFIGPROP_INIT(var, defval) (var).initialize(__klfconfigprop_configbase, #var, (defval))
#define KLFCONFIGPROP_INIT_DEFNOTDEF(var, defval) (var).initialize(__klfconfigprop_configbase, #var, (defval), false)




#endif
