/***************************************************************************
 *   file klfdefs_linux.cpp
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

/** \file
 *
 * This file contains some Linux specific code for klfdefs.cpp
 */

#include <sys/utsname.h>

#include <klfdefs.h>
#include <klfsysinfo.h>

#include <QFileInfo>
#include <QFile>


KLF_EXPORT QString klf_defs_sysinfo_arch()
{
  static bool is_64 = (sizeof(void*) == 8);

  struct utsname d;
  uname(&d);

  if (d.machine[0] == 'i' && d.machine[2] == '8' && d.machine[3] == '6') {
    // i?86
    return QString::fromLatin1("x86");
  }
  if (!strcmp(d.machine, "x86_64")) {
    // we could still be a 32-bit application run on a 64-bit machine
    return is_64 ? QString::fromLatin1("x86_64") : QString::fromLatin1("x86");
  }
  if (!strncmp(d.machine, "ppc", strlen("ppc"))) {
    return is_64 ? QString::fromLatin1("ppc64") : QString::fromLatin1("ppc");
  }
  return QString::fromLatin1("unknown");
}




// access sys info in /sys/class/power_supply/ADP0/online

#define SYSINFO_FILE "/sys/class/power_supply/ADP0/online"

KLF_EXPORT KLFSysInfo::BatteryInfo _klf_linux_battery_info(bool want_info_onbatterypower)
{
  KLFSysInfo::BatteryInfo info;

  if (!QFile::exists(SYSINFO_FILE)) {
    info.islaptop = false;
    info.onbatterypower = false;
    return info;
  }

  if (want_info_onbatterypower) {
    QFile file(SYSINFO_FILE);
    if (!file.open(QIODevice::ReadOnly)) {
      klfWarning("Can't open file "<<SYSINFO_FILE);
      return info;
    }
    QTextStream tstr(&file);

    int val;
    tstr >> val;
    // val == 1 -> power online
    // val == 0 -> on battery power
    info.onbatterypower = ! (bool)val;
  }

  info.islaptop = true;

  return info;
}

KLF_EXPORT KLFSysInfo::BatteryInfo _klf_linux_battery_info()
{
  return _klf_linux_battery_info(true);
}

KLF_EXPORT bool _klf_linux_is_laptop()
{
  KLFSysInfo::BatteryInfo info;
  info = _klf_linux_battery_info(false);
  return info.islaptop;
}

KLF_EXPORT bool _klf_linux_is_on_battery_power()
{
  KLFSysInfo::BatteryInfo info;
  info = _klf_linux_battery_info(true);
  return info.onbatterypower;
}

