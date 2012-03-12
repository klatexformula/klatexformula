/***************************************************************************
 *   file klfuserscript.h
 *   This file is part of the KLatexFormula Project.
 *   Copyright (C) 2012 by Philippe Faist
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

#ifndef KLFUSERSCRIPT_H
#define KLFUSERSCRIPT_H

#include <klfdefs.h>
#include <klfbackend.h>

//! Summary of the info returned by a user script
/** See also \ref pageUserScript .
 */
class KLF_EXPORT KLFUserScriptInfo : public KLFAbstractPropertizedObject
{
public:
  /**
   * \note \c settings may only be NULL if the user script info was already queried by another instance
   *   of a KLFUserScriptInfo and the cache was not cleared.
   */
  KLFUserScriptInfo(const QString& scriptFileName, KLFBackend::klfSettings * settings = NULL);
  KLFUserScriptInfo(const KLFUserScriptInfo& copy);
  virtual ~KLFUserScriptInfo();

  static void clearCacheAll();

  int scriptInfoError() const;
  QString scriptInfoErrorString() const;

  QString fileName() const;
  QString scriptName() const;

  enum Properties {
    Category = 0,
    Name,
    Author,
    Version,
    License,
    KLFMinVersion,
    KLFMaxVersion,
    SpitsOut,
    SkipFormats
  };

  QString category() const;

  QString name() const;
  QString author() const;
  QStringList authorList() const;
  QString version() const;
  QString license() const;
  QString klfMinVersion() const;
  QString klfMaxVersion() const;

  QStringList spitsOut() const;

  //! List of formats that klfbackend should not attempt to generate
  /** The corresponding field(s) in KLFBackend::klfOutput will be set to empty QByteArray's.
   *
   * Same format list as 'spits-out'. */
  QStringList skipFormats() const;

  //   struct Param {
  //     enum ParamType { String, Bool, Int, Enum };
  //     Param() : type(String) { }

  //     ParamType type;
  //     QStringList type_enums;

  //     QString name;
  //     QString title;
  //     QString description;
  //   };

  //   QList<Param> paramList() const;

  bool hasNotices() const;
  QStringList notices() const;
  bool hasWarnings() const;
  QStringList warnings() const;
  bool hasErrors() const;
  QStringList errors() const;


  virtual QVariant info(int propId) const;
  /** Calls info(propId) for the correct id. */
  virtual QVariant info(const QString& key) const;

  /** Returns a list of Keys (eg. "Name", "Author", ... including custom infos) that have been
   * reported by the user script. */
  virtual QStringList infosList() const;

  // reimplemented from KLFAbstractPropertizedObject
  virtual QString objectKind() const;
  virtual QVariant property(const QString& propName) const { return info(propName); }
  virtual QStringList propertyNameList() const { return infosList(); }
  virtual bool setProperty(const QString&, const QVariant&) { return false; }
  
protected:
  void internalSetProperty(const QString& key, const QVariant &val);
  const KLFPropertizedObject * pobj();

private:
  class Private;

  KLFRefPtr<Private> d;
  inline Private * d_func() { return d(); }
  inline const Private * d_func() const { return d(); }
};


KLF_DECLARE_POBJ_TYPE(KLFUserScriptInfo) ;




#endif
