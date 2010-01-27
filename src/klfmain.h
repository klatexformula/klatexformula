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

#include <QString>
#include <QList>
#include <QStringList>

#include <klfdefs.h>




// SOME DEFINITIONS

#if defined(Q_WS_WIN)
#define KLF_DLL_EXT "*.dll"
#elif defined(Q_WS_MAC)
#define KLF_DLL_EXT "*.dylib"
#else
#define KLF_DLL_EXT "*.so"
#endif


// SOME DECLARATIONS FOR ADD-ONS

class KLF_EXPORT KLFAddOnInfo
{
public:
  /** Builds empty add-on */
  KLFAddOnInfo() : islocal(false), isfresh(false) { }
  /** Reads RCC file \c rccfpath and parses its rccinfo/info.xml, etc.
   * sets all fields to correct values and \ref isfresh to \c FALSE . */
  KLFAddOnInfo(QString rccfpath);

  QString dir;
  QString fname;
  QString fpath; // grosso modo: absdir(dir) + "/" + fname
  bool islocal; // local file: can be removed (e.g. not in a global path /usr/share/... )

  QString title;
  QString author;
  QString description;

  /** The list of plugins provided by this add-on (list of files \c ":/plugins/<b></b>*.so").
   * This list stores full file names without the path (e.g. \c "libskin.so") . (Replace
   * of course \c ".so" by \c ".dll" and \c ".dylib" for windows and mac respectively). */
  QStringList plugins;

  /** The list of translation files provided by this add-on (list of files \c ":/i18n/<b></b>*.qm")
   * This list stores full file names without the path (e.g. \c "klf_fr.qm") */
  QStringList translations;

  /** Fresh file: add-on imported during this execution; ie. KLatexFormula needs to be restarted
   * for this add-on to take effect. The constructor sets this value to \c FALSE, set it manually
   * to \c TRUE if needed (e.g. in KLFSettings). */
  bool isfresh;
};

KLF_EXPORT extern QList<KLFAddOnInfo> klf_addons;
KLF_EXPORT extern bool klf_addons_canimport;



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



// VERSION INFORMATION

KLF_EXPORT extern char version[];
KLF_EXPORT extern int version_maj, version_min, version_release;




/** \brief Small minimalist structure to store basic information about
 * available translations.
 *
 * Intended for settings dialog to read.
 *
 * To manage translation files, see \ref KLFI18nFile.
 */
struct KLFTranslationInfo {
  QString localename;
  QString translatedname;
  /** \brief TRUE if the translatedname was provided by translator itself
   *
   * and FALSE if the name was guessed. */
  bool hasnicetranslatedname;
};

KLF_EXPORT extern QList<KLFTranslationInfo> klf_avail_translations;


/** \brief Small structure to store information for a translation file (.qm)
 *
 * Intented as (temporary) helper to manage translation files. Used e.g.
 * in main.cpp: \ref main_load_translations().
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



#endif 
