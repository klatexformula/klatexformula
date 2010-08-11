/***************************************************************************
 *   file klfmain.h
 *   This file is part of the KLatexFormula Project.
 *   Copyright (C) 2009 by Philippe Faist
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

#ifndef KLFMAIN_H
#define KLFMAIN_H

#include <QCoreApplication>
#include <QString>
#include <QList>
#include <QMap>
#include <QStringList>
#include <QTranslator>

#include <klfdefs.h>




// SOME DEFINITIONS

#if defined(Q_WS_WIN)
#define KLF_DLL_EXT "*.dll"
#elif defined(Q_WS_MAC)
#define KLF_DLL_EXT "*.so"
#else
#define KLF_DLL_EXT "*.so"
#endif



// SOME DECLARATIONS FOR ADD-ONS

class KLF_EXPORT KLFAddOnInfo
{
public:
  /** Reads RCC file \c rccfpath and parses its rccinfo/info.xml, etc.
   * sets all fields to correct values and \ref isfresh to \c isFresh . */
  KLFAddOnInfo(QString rccfpath, bool isFresh = false);

  /** Create a copy of the add-on info structure \c other */
  KLFAddOnInfo(const KLFAddOnInfo& o);

  struct PluginSysInfo {
    /** The directory in question */
    QString dir;
    /** minimum Qt version required to load the plugin, in version string format (eg. "4.4"). An
     * empty qtminversion disables version check. */
    QString qtminversion;
    /** minimum KLatexFormula version required to load the plugin, in version string format (eg.
     * "4.4"). An empty qtminversion disables version check. */
    QString klfminversion;
    /** The required value of \ref KLFSysInfo::osString() */
    QString os;
    /** The required value of \ref KLFSysInfo::arch() */
    QString arch;
  };

  ~KLFAddOnInfo();

  QString dir() { return d->dir; }
  QString fname() { return d->fname; };
  /** in principle: absdir(dir) + "/" + fname */
  QString fpath() { return d->fpath; }
  /** local file: can be removed (e.g. not in a global path /usr/share/... ) */
  bool islocal() { return d->islocal; }

  /** the info in the add-on's info.xml file */
  QString title() { return d->title; }
  /** the info in the add-on's info.xml file */
  QString author() { return d->author; }
  /** the info in the add-on's info.xml file */
  QString description() { return d->description; }
  /** the info in the add-on's info.xml file */
  QString klfminversion() { return d->klfminversion; }

  //! where in the resource tree this rcc resource data is mounted
  QString rccmountroot() { return d->rccmountroot; }
    
  /** The list of plugins provided by this add-on (list of files
   * \c ":/plugins/<b>[&lt;dir>/]&lt;plugin-name></b>*.so|dll").
   *
   * This list stores full file names relative to plugin dir in add-on (e.g. \c "libskin.so" or
   * \c "linux-x86-klf3.1.1/libskin.so") .
   */
  QStringList pluginList() { return d->pluginList; }

  PluginSysInfo pluginSysInfo(const QString& plugin) const { return d->plugins[plugin]; }

  /** The list of translation files provided by this add-on (list of files \c ":/i18n/<b></b>*.qm")
   * This list stores full file names without the path (e.g. \c "klf_fr.qm") */
  QStringList translations() { return d->translations; }

  /** Fresh file: add-on imported during this execution; ie. KLatexFormula needs to be restarted
   * for this add-on to take effect. The constructor sets this value to \c FALSE, set it manually
   * to \c TRUE if needed (e.g. in KLFSettings). */
  bool isfresh() { return d->isfresh; }

private:

  /** \internal */
  struct Private {
    int ref; // number of times this data structure is referenced

    QString dir;
    QString fname;
    QString fpath;
    bool islocal;

    QString title;
    QString author;
    QString description;
    QString klfminversion;

    QString rccmountroot;
    
    QStringList pluginList;
    QMap<QString,PluginSysInfo> plugins;
    
    QStringList translations;
    
    bool isfresh;
  };

  Private *d;

  void initPlugins();
};

KLF_EXPORT extern QList<KLFAddOnInfo> klf_addons;
KLF_EXPORT extern bool klf_addons_canimport;

KLF_EXPORT QDebug& operator<<(QDebug& str, const KLFAddOnInfo::PluginSysInfo& i);



// SOME DEFINITIONS FOR PLUGINS

class KLFPluginGenericInterface;

struct KLFPluginInfo
{
  QString name;
  QString author;
  QString title;
  QString description;

  QString fpath;
  QString fname;

  KLFPluginGenericInterface * instance;
};


KLF_EXPORT extern QList<KLFPluginInfo> klf_plugins;






/** \brief Small minimalist structure to store basic information about
 * available translations.
 *
 * Intended for settings dialog to read.
 *
 * To manage translation files, see \ref KLFI18nFile.
 */
struct KLF_EXPORT KLFTranslationInfo
{
  KLFTranslationInfo() : hasnicetranslatedname(false) { }

  QString localename;
  QString translatedname;
  /** \brief TRUE if the translatedname was provided by translator itself
   *
   * and FALSE if the name was guessed. */
  bool hasnicetranslatedname;
};

/** a list of locale names available for KLatexFormula */
KLF_EXPORT extern QList<KLFTranslationInfo> klf_avail_translations;

/** A list of instances of currently installed translators. */
KLF_EXPORT extern QList<QTranslator*> klf_translators;


/** \brief Small structure to store information for a translation file (.qm)
 *
 * Intented as (temporary) helper to manage translation files. Used e.g.
 * in \ref klf_reload_translations().
 *
 * To see a list of available translations accessible within the whole
 * program, see \ref KLFTranslationInfo and \ref klf_avail_translations.
 */
class KLF_EXPORT KLFI18nFile
{
public:
  QString fpath;
  /** \brief Translation file base name (e.g. 'klf' for klf_fr.qm) */
  QString name;
  /** \brief Locale Name (e.g. "fr" or "fr_CH") */
  QString locale;
  /** \brief  how specific the locale is (e.g. ""->0 , "fr"->1, "fr_CH"->2 ) */
  int locale_specificity;

  /** Initialize this structure to the translation file \c filepath. */
  KLFI18nFile(QString filepath);
};



KLF_EXPORT void klf_add_avail_translation(KLFI18nFile i18nfile);


/** Call this at startup or upon language change */
KLF_EXPORT void klf_reload_translations(QCoreApplication *app, const QString& currentlocale);



// eg. baseFileName="cmdl-help"  extension=".txt" will look for
// "cmdl-help_fr_CH.txt", "cmdl-help_fr.txt", "cmdl-help.txt" assuming current locale is "fr_CH"
KLF_EXPORT QString klfFindTranslatedDataFile(const QString& baseFileName, const QString& extension);


#endif 
