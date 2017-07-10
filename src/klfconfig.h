/***************************************************************************
 *   file klfconfig.h
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

#ifndef KLFCONFIG_H
#define KLFCONFIG_H

#include <assert.h>

#include <qglobal.h>
#include <QDebug>
#include <QString>
#include <QFont>
#include <QSize>
#include <QColor>
#include <QSettings>
#include <QTextCharFormat>
#include <QMap>
#include <QHash>
#include <QSettings>

#include <klfbackend.h>
#include <klfutil.h>
#include <klfdatautil.h>
#include <klfconfigbase.h>


template<class T>
inline bool klf_config_read_value(QSettings &s, const QString& baseName, T * target,
				  const char * listOrMapType  = NULL)
{
  QVariant defVal = QVariant::fromValue<T>(*target);
  QVariant valstrv = s.value(baseName, QVariant());
  if (valstrv.isNull()) {
    klfDbg("No entry "<<baseName<<" in config.") ;
    return false;
  }
  QString valstr = valstrv.toString();
  QVariant val = klfLoadVariantFromText(valstr.toLatin1(), defVal.typeName(), listOrMapType);
  if (val.isValid()) {
    klfDbg("klf_config_read_value: read value for "<<baseName<<" : " << val) ;
    *target = val.value<T>();
    return true;
  }
  klfDbg("klf_config_read_value: read empty or invalid value for "<<baseName) ;
  return false;
}

template<class T>
inline void klf_config_read(QSettings &s, const QString& baseName, KLFConfigProp<T> *target,
			    const char * listOrMapType = NULL)
{
  T value = *target;
  if (klf_config_read_value(s, baseName, &value, listOrMapType))
    *target = value;
}
template<>
inline void klf_config_read<QTextCharFormat>(QSettings &s, const QString& baseName,
					     KLFConfigProp<QTextCharFormat> *target,
					     const char * /*listOrMapType*/)
{
  qDebug("klf_config_read<QTextCharFormat>(%s)", qPrintable(baseName));
  QTextFormat fmt = *target;
  klf_config_read_value(s, baseName, &fmt);
  *target = fmt.toCharFormat();
}

template<class T>
inline void klf_config_read_list(QSettings &s, const QString& baseName, KLFConfigProp<QList<T> > *target)
{
  QVariantList vlist = klfListToVariantList(target->value());
  klf_config_read_value(s, baseName, &vlist, QVariant::fromValue<T>(T()).typeName());
  *target = klfVariantListToList<T>(vlist);
}


template<class T>
inline void klf_config_write_value(QSettings &s, const QString& baseName, const T * value)
{
  QVariant val = QVariant::fromValue<T>(*value);
  QByteArray datastr = klfSaveVariantToText(val);
  s.setValue(baseName, QVariant::fromValue<QString>(QString::fromLocal8Bit(datastr)));
}
template<class T>
inline void klf_config_write(QSettings &s, const QString& baseName, const KLFConfigProp<T> * target)
{
  T temp = *target;
  klf_config_write_value(s, baseName, &temp);
}
template<>
inline void klf_config_write<QTextCharFormat>(QSettings &s, const QString& baseName,
					      const KLFConfigProp<QTextCharFormat> * target)
{
  klfDbg("<QTextCharFormat>, baseName="<<baseName) ;
  QTextFormat f = *target;
  klf_config_write_value(s, baseName, &f);
}
template<class T>
inline void klf_config_write_list(QSettings &s, const QString& baseName, const KLFConfigProp<QList<T> > * target)
{
  QVariantList vlist = klfListToVariantList(target->value());
  klf_config_write_value(s, baseName, &vlist);
}



class KLFConfig;

// /** \brief Utility class for plugins to access their configuration space in KLFConfig
//  *
//  * KLatexFormula stores its configuration via KLFConfig and the global \c klfconfig object.
//  * That structure relies on the config structure being known in advance and the named
//  * fields to appear publicly in the KLFConfig class. This scheme is obviously NOT possible
//  * for plugins, so a different approach is taken.
//  *
//  * Plugins are given a pointer to a \c KLFPluginConfigAccess object, which is an interface
//  * to access a special part of KLFConfig that stores plugin-related configuration in the
//  * form of QVariantMaps. (themselves written in an INI-based config file inside the plugin's
//  * local directory, at <tt>~/.klatexformula/plugindata/&lt;plugin>/&lt;plugin>.conf</tt>)
//  *
//  * KLFConfig transparently takes care of reading the config for plugins at the beginning
//  * when launching KLatexFormula and storing the plugin configurations in their respective
//  * locations when quitting.
//  *
//  * Plugins can read values they have set in earlier sessions with readValue(). Default values
//  * can be defined with \ref makeDefaultValue().
//  *
//  * Plugins can write changed settings with \ref writeValue().
//  */
// class KLF_EXPORT KLFPluginConfigAccess
// {
//   KLFConfig *_config;
//   QString _pluginname;
// public:
//   KLFPluginConfigAccess();
//   KLFPluginConfigAccess(KLFConfig *configObject, const QString& pluginName);
//   KLFPluginConfigAccess(const KLFPluginConfigAccess& other);
//   virtual ~KLFPluginConfigAccess();

//   /** Returns the root directory in which KLatexFormula stores its stuff, usually
//    * <tt>~/.klatexformula</tt>.
//    */
//   virtual QString homeConfigDir() const;

//   /** Returns the directory (not necessarily existing) in which installed data that is
//    * shared among different users is stored.
//    * eg. system-wide installations of plugins/extensions can be placed in:
//    * <tt>share-dir/rccresources/<i>*</i>.rcc</tt>.
//    */
//   virtual QString globalShareDir() const;

//   /** Returns a directory in which we can read/write temporary files, eg. "/tmp".
//    *
//    * This is actually the value of <tt>klfconfig.BackendSettings.tempDir</tt> */
//   virtual QString tempDir() const;

//   /** Returns a path to a directory in which plugins can manage their data as they want.
//    *
//    * If the \c createIfNeeded argument is TRUE, then the directory is garanteed to exist,
//    * and an empty string is returned if, for whatever reason, the directory can't be
//    * created.
//    *
//    * If the \c createIfNeeded argument is FALSE, then the directory path is returned
//    * regardless of whether the directory exists or not.
//    *
//    * Note that a file named <tt><i>pluginName</i>.conf</tt> is created to store the plugin's
//    * settings in that directory (the settings are stored automatically).
//    */
//   virtual QString homeConfigPluginDataDir(bool createIfNeeded = true) const;

//   /** \brief read a value in the config
//    *
//    * Returns the value of the entry with key \c key. If no such entry exists,
//    * it is not created and an invalid QVariant() is returned.
//    */
//   virtual QVariant readValue(const QString& key) const;

//   /** \brief write the value if inexistant in config
//    *
//    * equivalent to
//    * \code
//    *  if (readValue(key).isNull())
//    *    writeValue(key, defaultValue);
//    * \endcode
//    *
//    * \return the value this key has after this function call, ie. \c defaultValue if no
//    *   existing value was found, or the existing value if one already exists. A null QVariant
//    *   is returned upon error.
//    */
//   virtual QVariant makeDefaultValue(const QString& key, const QVariant& defaultValue);

//   /** \brief write a value to settings
//    *
//    * Saves the value of a setting, referenced by \c key, to the given \c value.
//    *
//    * If \c key hasn't been previously set, creates an entry for \c key with the
//    * given \c value.
//    */
//   virtual void writeValue(const QString& key, const QVariant& value);
// };


//! Structure that stores klatexformula's configuration in memory
/**
 * This structure is more of a namespace than a class. Access it through the global
 * object \ref klfconfig.
 *
 * See also \ref KLFSettings for a graphical interface for editing these settings.
 */
class KLF_EXPORT KLFConfig : public KLFConfigBase
{
public:
  KLFConfig();
  virtual ~KLFConfig();

  QString homeConfigDir;
  QString globalShareDir;
  QString homeConfigSettingsFile; //!< current (now, "new" klatexformula.conf) settings file
  QString homeConfigSettingsFileIni; //!< OLD config file
  // QString homeConfigDirRCCResources;
  // QString homeConfigDirPlugins;
  // QString homeConfigDirPluginData;
  QString homeConfigDirI18n;
  QString homeConfigDirUserScripts;

  struct {

    KLFConfigProp<bool> thisVersionMajFirstRun;
    KLFConfigProp<bool> thisVersionMajMinFirstRun;
    KLFConfigProp<bool> thisVersionMajMinRelFirstRun;
    KLFConfigProp<bool> thisVersionExactFirstRun;

    /** The library file name, relative to homeConfigDir. */
    KLFConfigProp<QString> libraryFileName;
    /** The lib scheme to use to store the library. This scheme will be given the full path
     * to the library in the URL path part. */
    KLFConfigProp<QString> libraryLibScheme;

  } Core;

  struct {

    KLFConfigProp<QString> locale; //!< When setting this, don't forget to call QLocale::setDefault().
    KLFConfigProp<bool> useSystemAppFont;
    KLFConfigProp<QFont> applicationFont;
    KLFConfigProp<QFont> latexEditFont;
    KLFConfigProp<QFont> preambleEditFont;
    KLFConfigProp<bool> editorTabInsertsTab;
    KLFConfigProp<bool> editorWrapLines;
    KLFConfigProp<QSize> previewTooltipMaxSize;
    KLFConfigProp<QSize> labelOutputFixedSize; //!< No Longer used (3.3.0alpha)
    KLFConfigProp<QSize> smallPreviewSize; //!< Size of preview to store e.g. in history/library items
    //    KLFConfigProp<QSize> savedWindowSize;
    KLFConfigProp<QString> detailsSideWidgetType; //!< "ShowHide","Drawer", or "Float" (or any custom type!)
    KLFConfigProp<QString> lastSaveDir;
    KLFConfigProp<int> symbolsPerLine;
    KLFConfigProp<bool> symbolIncludeWithPreambleDefs;
    KLFConfigProp<QList<QColor> > userColorList;
    KLFConfigProp<QList<QColor> > colorChooseWidgetRecent;
    KLFConfigProp<QList<QColor> > colorChooseWidgetCustom;
    KLFConfigProp<int> maxUserColors;
    KLFConfigProp<bool> enableToolTipPreview;
    KLFConfigProp<bool> enableRealTimePreview;
    KLFConfigProp<bool> realTimePreviewExceptBattery;
    KLFConfigProp<int> autosaveLibraryMin;
    KLFConfigProp<bool> showHintPopups;
    KLFConfigProp<bool> clearLatexOnly;
    KLFConfigProp<bool> glowEffect;
    KLFConfigProp<QColor> glowEffectColor;
    KLFConfigProp<int> glowEffectRadius;
    KLFConfigProp<QStringList> customMathModes;
    KLFConfigProp<bool> emacsStyleBackspaceSearch;
    KLFConfigProp<bool> macBrushedMetalLook;

  } UI;

  struct {

    KLFConfigProp<QString> copyExportProfile;
    KLFConfigProp<QString> dragExportProfile;
    KLFConfigProp<bool> showExportProfilesLabel;
    KLFConfigProp<bool> menuExportProfileAffectsDrag;
    KLFConfigProp<bool> menuExportProfileAffectsCopy;
    KLFConfigProp<double> oooExportScale;
    KLFConfigProp<int> htmlExportDpi;
    KLFConfigProp<int> htmlExportDisplayDpi;

  } ExportData;

  struct {

    KLFConfigProp<bool> enabled;
    KLFConfigProp<bool> highlightParensOnly;
    KLFConfigProp<bool> highlightLonelyParens;
    //KLFConfigProp<bool> matchParenTypes;
    KLFConfigProp<QTextCharFormat> fmtKeyword;
    KLFConfigProp<QTextCharFormat> fmtComment;
    KLFConfigProp<QTextCharFormat> fmtParenMatch;
    KLFConfigProp<QTextCharFormat> fmtParenMismatch;
    KLFConfigProp<QTextCharFormat> fmtLonelyParen;

  } SyntaxHighlighter;

  struct {

    KLFConfigProp<QString> tempDir;
    KLFConfigProp<QString> execLatex;
    KLFConfigProp<QString> execDvips;
    KLFConfigProp<QString> execGs;
    KLFConfigProp<QString> execEpstopdf;
    KLFConfigProp<QStringList> execenv;
    KLFConfigProp<QString> setTexInputs;
    KLFConfigProp<double> lborderoffset;
    KLFConfigProp<double> tborderoffset;
    KLFConfigProp<double> rborderoffset;
    KLFConfigProp<double> bborderoffset;
    KLFConfigProp<bool> calcEpsBoundingBox;
    KLFConfigProp<bool> outlineFonts;
    KLFConfigProp<bool> wantPDF;
    KLFConfigProp<bool> wantSVG;
    KLFConfigProp<QStringList> userScriptAddPath;
    KLFConfigProp<QVariantMap> userScriptInterpreters;

  } BackendSettings;

  struct {

    KLFConfigProp<QColor> colorFound;
    KLFConfigProp<QColor> colorNotFound;

    KLFConfigProp<bool> restoreURLs;
    KLFConfigProp<bool> confirmClose;
    KLFConfigProp<bool> groupSubCategories;
    KLFConfigProp<int> iconViewFlow;
    KLFConfigProp<bool> historyTagCopyToArchive;
    KLFConfigProp<QString> lastFileDialogPath;

    KLFConfigProp<int> treePreviewSizePercent;
    KLFConfigProp<int> listPreviewSizePercent;
    KLFConfigProp<int> iconPreviewSizePercent;

  } LibraryBrowser;

  // struct {

  //   QMap<QString, QMap<QString, QVariant> > pluginConfig;

  // } Plugins;

  struct {
    
    QMap<QString, QMap<QString, QVariant> > userScriptConfig;

  } UserScripts;

  /** Not a saved setting. This is set in loadDefaults() */
  QFont defaultCMUFont;
  /** Not a saved setting. This is set in loadDefaults() */
  QFont defaultStdFont;
  /** Not a saved setting. This is set in loadDefaults() */
  QFont defaultTTFont;

  //  KLFPluginConfigAccess getPluginConfigAccess(const QString& name);

  /** call loadDefaults() before anything, at the beginning, to ensure that the values
   * in this structure are not undefined. (the constructor doesn't set any values).
   *
   * loadDefaults() will set reasonable default values for most settings, but will not
   * start detecting system settings, specifically look for system executables, possibly other
   * long detection tasks. To perform that, call detectMissingSettings().
   *
   * In practice, main() calls, in order, loadDefaults(), readFromConfig(), and
   * detectMissingSettings().
   * */
  void loadDefaults();
  int readFromConfig();
  void detectMissingSettings();

  int ensureHomeConfigDir();

  int writeToConfig();


  /** returns TRUE if the executable paths are valid. */
  bool checkExePaths();


  /** will be called before QApplication etc. are destroyed */
  void prepareDestruction();

private:
  int readFromConfig_v2(const QString& fname);
  int readFromConfig_v1();

};




extern KLFConfig * klf_the_config;

template<typename T>
inline T * klf_get_ptr_assert_not_null(T * ptr) {
  assert(ptr != NULL);
  return ptr;
}

#define klfconfig					\
  (*klf_get_ptr_assert_not_null(klf_the_config))




#define KLF_CONNECT_CONFIG_SH_LATEXEDIT(latexedit)				\
  klfconfig.SyntaxHighlighter.enabled					\
  .connectQObjectProperty((latexedit)->syntaxHighlighter(), "highlightEnabled") ; \
  klfconfig.SyntaxHighlighter.highlightParensOnly			\
  .connectQObjectProperty((latexedit)->syntaxHighlighter(), "highlightParensOnly") ; \
  klfconfig.SyntaxHighlighter.highlightLonelyParens			\
  .connectQObjectProperty((latexedit)->syntaxHighlighter(), "highlightLonelyParens") ; \
  klfconfig.SyntaxHighlighter.fmtKeyword				\
  .connectQObjectProperty((latexedit)->syntaxHighlighter(), "fmtKeyword") ; \
  klfconfig.SyntaxHighlighter.fmtComment				\
  .connectQObjectProperty((latexedit)->syntaxHighlighter(), "fmtComment") ; \
  klfconfig.SyntaxHighlighter.fmtParenMatch				\
  .connectQObjectProperty((latexedit)->syntaxHighlighter(), "fmtParenMatch") ; \
  klfconfig.SyntaxHighlighter.fmtParenMismatch				\
  .connectQObjectProperty((latexedit)->syntaxHighlighter(), "fmtParenMismatch") ; \
  klfconfig.SyntaxHighlighter.fmtLonelyParen				\
  .connectQObjectProperty((latexedit)->syntaxHighlighter(), "fmtLonelyParen") ;




#endif
