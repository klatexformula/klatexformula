/***************************************************************************
 *   file klfsysinfo.h
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

#ifndef KLFSYSINFO_H
#define KLFSYSINFO_H

#include <klfdefs.h>


namespace KLFSysInfo
{
  enum Os { Linux, Win32, MacOsX, OtherOs };

  inline int sizeofVoidStar() { return sizeof(void*); }

  KLF_EXPORT QString arch();

  KLF_EXPORT QString makeSysArch(const QString& os, const QString& arch);
  KLF_EXPORT bool isCompatibleSysArch(const QString& sysarch);

  KLF_EXPORT KLFSysInfo::Os os();

  KLF_EXPORT QString osString(KLFSysInfo::Os sysos = os());

  struct BatteryInfo {
    BatteryInfo() : islaptop(false), onbatterypower(false) { }

    bool islaptop;
    bool onbatterypower;
  };
  KLF_EXPORT BatteryInfo batteryInfo();

  KLF_EXPORT bool isLaptop();
  KLF_EXPORT bool isOnBatteryPower();

};



#if defined(Q_OS_WIN32) || defined(Q_OS_WIN64)
#  define KLF_PATH_SEP ';'
#  define KLF_DIR_SEP '\\'
#else
#  define KLF_PATH_SEP ':'
#  define KLF_DIR_SEP '/'
#endif



#endif // KLFSYSINFO_H
