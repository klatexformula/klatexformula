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
#include <klffilterprocess.h>




//! Summary of the info returned by a user script
/** See also \ref pageUserScript .
 */
class KLF_EXPORT KLFUserScriptInfo : public KLFAbstractPropertizedObject
{
public:
  /**
   * \param klfuserscriptpath is the path to the "xxx.klfuserscript" directory
   */
  KLFUserScriptInfo(const QString& userScriptPath);
  KLFUserScriptInfo(const KLFUserScriptInfo& copy);
  virtual ~KLFUserScriptInfo();

  static bool hasScriptInfoInCache(const QString& userScriptPath);
  static KLFUserScriptInfo forceReloadScriptInfo(const QString& scriptFileName);
  static void clearCacheAll();
  static QMap<QString,QString> usConfigToStrMap(const QVariantMap& usconfig);
  static QStringList usConfigToEnvList(const QVariantMap& usconfig);

  int scriptInfoError() const;
  QString scriptInfoErrorString() const;

  //! e.g. "/path/to/klffeynmf.klfuserscript"
  QString userScriptPath() const;
  //! e.g. "klffeynmf.klfuserscript"
  QString userScriptName() const;
  //! e.g. "klffeynmf"
  QString userScriptBaseName() const;

  enum Properties {
    ExeScript = 0,
    Category,
    Name,
    Author,
    Version,
    License,
    KLFMinVersion,
    KLFMaxVersion,
    SettingsFormUI,
    CanProvideDefaultSettings,
    //! XML representation of the category-specific configuration (QByteArray)
    CategorySpecificXmlConfig
  };

  QString relativeFile(const QString& fname) const;

  QString exeScript() const;
  QString exeScriptFullPath() const;

  QString category() const;

  QString name() const;
  QString author() const;
  QStringList authorList() const;
  QString version() const;
  QString license() const;
  QString klfMinVersion() const;
  QString klfMaxVersion() const;

  //! A UI widget form file (Qt designer file) to display for setting up the user script
  QString settingsFormUI() const;

  bool canProvideDefaultSettings() const;

  QMap<QString,QVariant> queryDefaultSettings(const KLFBackend::klfSettings * settings = NULL) const;

  bool hasNotices() const;
  QStringList notices() const;
  bool hasWarnings() const;
  QStringList warnings() const;
  bool hasErrors() const;
  QStringList errors() const;

  /** \brief Formats most (all?) properties in HTML, suitable for human-readable text display */
  QString htmlInfo(const QString& extra_css = QString()) const;


  QVariant scriptInfo(int propId) const;
  /** Calls scriptInfo(propId) for the correct id. */
  QVariant scriptInfo(const QString& key) const;

  /** \brief A list of Keys (eg. "Name", "Author", ... including custom infos) found in
   *         the scriptinfo
   */
  QStringList scriptInfosList() const;

  // reimplemented from KLFAbstractPropertizedObject
  virtual QString objectKind() const;
  virtual QVariant property(const QString& propName) const { return scriptInfo(propName); }
  virtual QStringList propertyNameList() const { return scriptInfosList(); }
  virtual bool setProperty(const QString&, const QVariant&) { return false; }

protected:

  void internalSetProperty(const QString& key, const QVariant &val);
  const KLFPropertizedObject * pobj();

  /** \brief The XML for the category-specific config.
   *
   * This class is meant to be accessed by subclasses who parse this XML and expose a
   *  higher level API.
   */
  QByteArray categorySpecificXmlConfig() const;
  
  void setScriptInfoError(int code, const QString & msg);

private:
  struct Private;

  KLFRefPtr<Private> d;
  inline Private * d_func() { return d(); }
  inline const Private * d_func() const { return d(); }
};


KLF_DECLARE_POBJ_TYPE(KLFUserScriptInfo) ;


struct KLFBackendEngineUserScriptInfoPrivate;

class KLF_EXPORT KLFBackendEngineUserScriptInfo : public KLFUserScriptInfo
{
public:
  KLFBackendEngineUserScriptInfo(const QString& userScriptPath);
  virtual ~KLFBackendEngineUserScriptInfo();
    
  enum BackendEngineProperties {
    SpitsOut = 0,
    SkipFormats,
    DisableInputs,
    InputFormUI
  };

  //! List of formats that this script will generate
  QStringList spitsOut() const;

  //! List of formats that klfbackend should not attempt to generate
  /** The corresponding field(s) in KLFBackend::klfOutput will be set to empty QByteArray's.
   *
   * Same format list as 'spits-out'.
   */
  QStringList skipFormats() const;

  //! List of user input fields that should be disabled
  QStringList disableInputs() const;
  
  //! A UI input form file (Qt designer file) for additional input
  QString inputFormUI() const;

  QVariant klfBackendEngineInfo(int propId) const;
  QVariant klfBackendEngineInfo(const QString& key) const;
  QStringList klfBackendEngineInfosList() const;

private:
  KLF_DECLARE_PRIVATE(KLFBackendEngineUserScriptInfo) ;
};







struct KLFUserScriptFilterProcessPrivate;

class KLF_EXPORT KLFUserScriptFilterProcess : public KLFFilterProcess
{
public:
  /** This will already prepare the KLFFilterProcess to run the user script.
   *
   * Use \ref addArgv() to add parameters to the command-line. the script itself is already
   * added as first parameter automatically.
   */
  KLFUserScriptFilterProcess(const QString& scriptFileName,
                             const KLFBackend::klfSettings * settings = NULL);
  ~KLFUserScriptFilterProcess();

  void addUserScriptConfig(const QVariantMap& usconfig);

  /** \brief Return the user script log, formatted in human-readable HTML
   *
   * Unless \a include_head=false, a full HTML document is included with a default style
   * set.
   */
  static QString getUserScriptLogHtml(bool include_head=true) ;

protected:
  /**
   * This method is overriden to do some book-keeping, e.g. update the global user script
   * log.  You may call any of the other \a run() methods of KLFFilterProcess, they will
   * all redirect to this call.
   *
   */
  virtual bool do_run(const QByteArray& indata, const QMap<QString, QByteArray*> outdatalist);

private:
  KLF_DECLARE_PRIVATE(KLFUserScriptFilterProcess);
};




#endif
