/***************************************************************************
 *   file klfsysinfo.cpp
 *   This file is part of the KLatexFormula Project.
 *   Copyright (C) 2014 by Philippe Faist
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

#include <stdlib.h>

#include <QDebug>
#include <QString>
#include <QStringList>

#include "klfdefs.h"
#include "klfsysinfo.h"


// declared in klfdefs_<OS>.{mm|cpp}
QString klf_defs_sysinfo_arch();

KLF_EXPORT QString KLFSysInfo::arch()
{
  return klf_defs_sysinfo_arch();
}

KLF_EXPORT QString KLFSysInfo::makeSysArch(const QString& os, const QString& arch)
{
  return os+":"+arch;
}
KLF_EXPORT bool KLFSysInfo::isCompatibleSysArch(const QString& systemarch)
{
  QString sysarch = systemarch;

  // on Windows, we use -- instead of ':' because ':' is an illegal char for a file name.
  sysarch.replace("--", ":");

  int ic = sysarch.indexOf(':');
  if (ic == -1) {
    qWarning()<<KLF_FUNC_NAME<<": Invalid sysarch string "<<sysarch;
    return false;
  }
  QString thisos = osString();
  if (thisos != sysarch.left(ic)) {
    klfDbg("incompatible architectures (this one)="<<thisos<<" and (tested)="<<sysarch) ;
    return false;
  }
  QStringList archlist = sysarch.mid(ic+1).split(',');
  QString thisarch = arch();
  klfDbg("testing if our arch="<<thisarch<<" is in the compatible arch list="<<archlist) ;
  return KLF_DEBUG_TEE( archlist.contains(thisarch) );
}

KLF_EXPORT KLFSysInfo::Os KLFSysInfo::os()
{
#if defined(Q_OS_LINUX)
  return Linux;
#elif defined(Q_OS_DARWIN)
  return MacOsX;
#elif defined(Q_OS_WIN32)
  return Win32;
#else
  return OtherOs;
#endif
}

KLF_EXPORT QString KLFSysInfo::osString(Os sysos)
{
  switch (sysos) {
  case Linux: return QLatin1String("linux");
  case MacOsX: return QLatin1String("macosx");
  case Win32: return QLatin1String("win32");
  case OtherOs: return QString();
  default: ;
  }
  qWarning("KLFSysInfo::osString: unknown OS: %d", sysos);
  return QString();
}


#ifdef Q_OS_DARWIN
 bool _klf_mac_is_laptop();
 bool _klf_mac_is_on_battery_power();
 KLFSysInfo::BatteryInfo _klf_mac_battery_info();
#elif defined(Q_OS_LINUX)
 bool _klf_linux_is_laptop();
 bool _klf_linux_is_on_battery_power();
 KLFSysInfo::BatteryInfo _klf_linux_battery_info();
#elif defined(Q_OS_WIN32)
 bool _klf_win_is_laptop();
 bool _klf_win_is_on_battery_power();
 KLFSysInfo::BatteryInfo _klf_win_battery_info();
#else
 bool _klf_otheros_is_laptop();
 bool _klf_otheros_is_on_battery_power();
 KLFSysInfo::BatteryInfo _klf_otheros_battery_info();
#endif

KLF_EXPORT KLFSysInfo::BatteryInfo KLFSysInfo::batteryInfo()
{
#if defined(Q_OS_DARWIN)
  return _klf_mac_battery_info();
#elif defined(Q_OS_LINUX)
  return _klf_linux_battery_info();
#elif defined(Q_OS_WIN32)
  return _klf_win_battery_info();
#else
  return _klf_otheros_battery_info();
#endif
}


static int _klf_cached_islaptop = -1;

KLF_EXPORT bool KLFSysInfo::isLaptop()
{
  if (_klf_cached_islaptop >= 0)
    return (bool) _klf_cached_islaptop;

#if defined(Q_OS_DARWIN)
  _klf_cached_islaptop = (int) _klf_mac_is_laptop();
#elif defined(Q_OS_LINUX)
  _klf_cached_islaptop = (int) _klf_linux_is_laptop();
#elif defined(Q_OS_WIN32)
  _klf_cached_islaptop = (int) _klf_win_is_laptop();
#else
  _klf_cached_islaptop = (int) _klf_otheros_is_laptop();
#endif

  return (bool) _klf_cached_islaptop;
}

KLF_EXPORT bool KLFSysInfo::isOnBatteryPower()
{
#if defined(Q_OS_DARWIN)
  return _klf_mac_is_on_battery_power();
#elif defined(Q_OS_LINUX)
  return _klf_linux_is_on_battery_power();
#elif defined(Q_OS_WIN32)
  return _klf_win_is_on_battery_power();
#else
  return _klf_otheros_is_on_battery_power();
#endif
}



// ----------------------------------------
