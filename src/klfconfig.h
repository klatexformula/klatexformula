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
 */
class KLF_EXPORT KLFPluginConfigAccess
{
  KLFConfig *_config;
  QString _pluginname;
  uint _amode;
public:
  enum AccessMode { Read = 0x01, Write = 0x02, ReadWrite = Read|Write };
  KLFPluginConfigAccess();
  KLFPluginConfigAccess(KLFConfig *configObject, const QString& pluginName,
			uint accessmode = Read | Write);
  KLFPluginConfigAccess(const KLFPluginConfigAccess& other)
    : _config(other._config), _pluginname(other._pluginname), _amode(other._amode) { }
  virtual ~KLFPluginConfigAccess() { }

  virtual uint accessMode() const { return _amode; }

  /** \note this method can be used even if accessmode doesn't have \c Read flag */
  virtual QString homeConfigDir() const;
  /** Returns a directory in which plugins can manage their data as they want. If the
   * \c createIfNeeded argument is TRUE, then the directory is garanteed to exist (in
   * particular, an empty string is returned if for whatever reason the directory can't
   * be created).
   *
   * Note that a file named <tt><i>pluginName</i>.conf</tt> is created for the settings
   * set using this class, in this directory.
   *
   * \note this method can be used even if accessmode doesn't have \c Read flag */
  virtual QString homeConfigPluginDataDir(bool createIfNeeded = true) const;

  virtual QVariant readValue(const QString& key);

  /** \brief write the value if inexistant in config
   *
   * equivalent to
   * \code if (readValue(key).isNull()) writeValue(key, defaultValue); \endcode
   * except that only the \c Write flag is needed for makeDefaultValue().
   *
   * \return the value this key has after this function call, ie. \c defaultValue if no
   *   existing value was found, or the existing value if one already exists. A null QVariant
   *   is returned upon error (e.g. accessMode() doesn't have the Write flag set).
   */
  virtual QVariant makeDefaultValue(const QString& key, const QVariant& defaultValue);
  virtual void writeValue(const QString& key, const QVariant& value);
};


//! Structure that stores klatexformula's configuration
class KLF_EXPORT KLFConfig {
public:

  /** this doesn't do anything. It actually leaves every entry with undefined values.
      This is why it's important to call loadDefaults() quickly after building an instance
      of KLFConfig. readFromConfig() isn't enough, beacause it assumes the default values
      are already stored in the current fields. */
  KLFConfig();

  QString homeConfigDir;
  QString homeConfigSettingsFile; // current (now, "new") settings file
  QString homeConfigSettingsFileIni; // OLD config file
  QString homeConfigDirRCCResources;
  QString homeConfigDirPlugins;
  QString homeConfigDirPluginData;
  QString homeConfigDirI18n;

  struct {

    bool thisVersionFirstRun;

  } General;

  struct {

    QString locale;
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

  } UI;

  struct {

    unsigned int configFlags;
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
    int lborderoffset;
    int tborderoffset;
    int rborderoffset;
    int bborderoffset;

  } BackendSettings;

  struct {

    bool displayTaggedOnly;
    bool displayNoDuplicates;
    QColor colorFound;
    QColor colorNotFound;

    bool restoreURLs;

  } LibraryBrowser;

  struct {

    QMap<QString, QMap<QString, QVariant> > pluginConfig;

  } Plugins;

  KLFPluginConfigAccess getPluginConfigAccess(const QString& name, uint amode)
  { return KLFPluginConfigAccess(this, name, amode); }

  // call loadDefaults() before anything, at the beginning.
  void loadDefaults();

  int readFromConfig();

  int ensureHomeConfigDir();

  int writeToConfig();

private:
  int readFromConfig_v2();
  int readFromConfig_v1();
};




KLF_EXPORT extern KLFConfig klfconfig;




#endif
