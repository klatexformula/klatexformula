/***************************************************************************
 *   file klfconfig.h
 *   This file is part of the KLatexFormula Project.
 *   Copyright (C) 2007 by Philippe Faist
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

#include <qglobal.h>
#include <QString>
#include <QFont>
#include <QSize>
#include <QColor>
#include <QSettings>
#include <QTextCharFormat>
#include <QMap>

#include <klfbackend.h>

class KLFConfig;

/** \brief Utility class for plugins to access their configuration space in KLFConfig
 *
 * KLatexFormula stores its configuration via KLFConfig and the global \c klfconfig object.
 * That structure relies on the config structure being known in advance and the named
 * fields to appear publicly in the KLFConfig class. This scheme is obviously NOT possible
 * for plugins, so a different approach is taken.
 *
 * Plugins are given a pointer to a \c KLFPluginConfigAccess object, which is an interface
 * to access a special part of KLFConfig that stores plugin-related configuration in the
 * form of QVariantMaps. (themselves written in an INI-based config file inside the plugin's
 * local directory, at <tt>~/.klatexformula/plugindata/&lt;plugin>/&lt;plugin>.conf</tt>)
 *
 * KLFConfig transparently takes care of reading the config for plugins at the beginning
 * when launching KLatexFormula and storing the plugin configurations in their respective
 * locations when quitting.
 *
 * Plugins can read values they have set in earlier sessions with readValue(). Default values
 * can be defined with \ref makeDefaultValue().
 *
 * Plugins can write changed settings with \ref writeValue().
 */
class KLF_EXPORT KLFPluginConfigAccess
{
  KLFConfig *_config;
  QString _pluginname;
public:
  KLFPluginConfigAccess();
  KLFPluginConfigAccess(KLFConfig *configObject, const QString& pluginName);
  KLFPluginConfigAccess(const KLFPluginConfigAccess& other);
  virtual ~KLFPluginConfigAccess();

  /** Returns the root directory in which KLatexFormula stores its stuff, usually
   * <tt>~/.klatexformula</tt>.
   */
  virtual QString homeConfigDir() const;

  /** Returns the directory (not necessarily existing) in which installed data that is
   * shared among different users is stored.
   * eg. system-wide installations of plugins/extensions can be placed in:
   * <tt>share-dir/rccresources/<i>*</i>.rcc</tt>.
   */
  virtual QString globalShareDir() const;

  /** Returns a directory in which we can read/write temporary files, eg. "/tmp".
   *
   * This is actually the value of <tt>klfconfig.BackendSettings.tempDir</tt> */
  virtual QString tempDir() const;

  /** Returns a path to a directory in which plugins can manage their data as they want.
   *
   * If the \c createIfNeeded argument is TRUE, then the directory is garanteed to exist,
   * and an empty string is returned if, for whatever reason, the directory can't be
   * created.
   *
   * If the \c createIfNeeded argument is FALSE, then the directory path is returned
   * regardless of whether the directory exists or not.
   *
   * Note that a file named <tt><i>pluginName</i>.conf</tt> is created to store the plugin's
   * settings in that directory (the settings are stored automatically).
   */
  virtual QString homeConfigPluginDataDir(bool createIfNeeded = true) const;

  /** \brief read a value in the config
   *
   * Returns the value of the entry with key \c key. If no such entry exists,
   * it is not created and an invalid QVariant() is returned.
   */
  virtual QVariant readValue(const QString& key) const;

  /** \brief write the value if inexistant in config
   *
   * equivalent to
   * \code
   *  if (readValue(key).isNull())
   *    writeValue(key, defaultValue);
   * \endcode
   *
   * \return the value this key has after this function call, ie. \c defaultValue if no
   *   existing value was found, or the existing value if one already exists. A null QVariant
   *   is returned upon error.
   */
  virtual QVariant makeDefaultValue(const QString& key, const QVariant& defaultValue);

  /** \brief write a value to settings
   *
   * Saves the value of a setting, referenced by \c key, to the given \c value.
   *
   * If \c key hasn't been previously set, creates an entry for \c key with the
   * given \c value.
   */
  virtual void writeValue(const QString& key, const QVariant& value);
};


//! Structure that stores klatexformula's configuration in memory
/**
 * This structure is more of a namespace than a class. Access it through the global
 * object \ref klfconfig.
 *
 * See also \ref KLFSettings for a graphical interface for editing these settings.
 */
class KLF_EXPORT KLFConfig {
public:

  /** this doesn't do anything. It actually leaves every entry with undefined values.
      This is why it's important to call loadDefaults() quickly after building an instance
      of KLFConfig. readFromConfig() isn't enough, beacause it assumes the default values
      are already stored in the current fields. */
  KLFConfig();


  QString homeConfigDir;
  QString globalShareDir;
  QString homeConfigSettingsFile; //!< current (now, "new" klatexformula.conf) settings file
  QString homeConfigSettingsFileIni; //!< OLD config file
  QString homeConfigDirRCCResources;
  QString homeConfigDirPlugins;
  QString homeConfigDirPluginData;
  QString homeConfigDirI18n;

  struct {

    bool thisVersionMajFirstRun;
    bool thisVersionMajMinFirstRun;
    bool thisVersionMajMinRelFirstRun;
    bool thisVersionExactFirstRun;

    /** The library file name, relative to homeConfigDir. */
    QString libraryFileName;
    /** The lib scheme to use to store the library. This scheme will be given the full path
     * to the library in the URL path part. */
    QString libraryLibScheme;

  } Core;

  struct {

    QString locale; //!< When setting this, don't forget to call QLocale::setDefault().
    bool useSystemAppFont;
    QFont applicationFont;
    QFont latexEditFont;
    QFont preambleEditFont;
    QSize previewTooltipMaxSize;
    QSize labelOutputFixedSize;
    QString lastSaveDir;
    int symbolsPerLine;
    QList<QColor> userColorList;
    QList<QColor> colorChooseWidgetRecent;
    QList<QColor> colorChooseWidgetCustom;
    int maxUserColors;
    bool enableToolTipPreview;
    bool enableRealTimePreview;
    int autosaveLibraryMin;
    bool showHintPopups;
    bool clearLatexOnly;
    QString copyExportProfile;
    QString dragExportProfile;
    bool glowEffect;
    QColor glowEffectColor;
    int glowEffectRadius;
    QStringList customMathModes;
    bool showExportProfilesLabel;
    bool menuExportProfileAffectsDrag;
    bool menuExportProfileAffectsCopy;
    bool emacsStyleBackspaceSearch;

  } UI;

  struct {

    bool enabled;
    bool highlightParensOnly;
    bool highlightLonelyParens;
    bool matchParenTypes;
    QTextCharFormat fmtKeyword;
    QTextCharFormat fmtComment;
    QTextCharFormat fmtParenMatch;
    QTextCharFormat fmtParenMismatch;
    QTextCharFormat fmtLonelyParen;

  } SyntaxHighlighter;

  struct {

    QString tempDir;
    QString execLatex;
    QString execDvips;
    QString execGs;
    QString execEpstopdf;
    QStringList execenv;
    double lborderoffset;
    double tborderoffset;
    double rborderoffset;
    double bborderoffset;
    bool outlineFonts;

  } BackendSettings;

  struct {

    QColor colorFound;
    QColor colorNotFound;

    bool restoreURLs;
    bool confirmClose;
    bool groupSubCategories;
    int iconViewFlow;
    bool historyTagCopyToArchive;
    QString lastFileDialogPath;

    int treePreviewSizePercent;
    int listPreviewSizePercent;
    int iconPreviewSizePercent;

  } LibraryBrowser;

  struct {

    QMap<QString, QMap<QString, QVariant> > pluginConfig;

  } Plugins;

  /** Not a saved setting. This is set in loadDefaults() */
  QFont defaultCMUFont;
  /** Not a saved setting. This is set in loadDefaults() */
  QFont defaultStdFont;
  /** Not a saved setting. This is set in loadDefaults() */
  QFont defaultTTFont;

  KLFPluginConfigAccess getPluginConfigAccess(const QString& name);

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

private:
  int readFromConfig_v2(const QString& fname);
  int readFromConfig_v1();

};




KLF_EXPORT extern KLFConfig klfconfig;





#endif
