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

#include <klfdefs.h>


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

  KLFPluginGenericInterface * instance;
};


KLF_EXPORT extern QList<KLFPluginInfo> klf_plugins;



// VERSION INFORMATION

KLF_EXPORT extern char version[];
KLF_EXPORT extern int version_maj, version_min, version_release;




#endif 
