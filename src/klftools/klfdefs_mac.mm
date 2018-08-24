/***************************************************************************
 *   file klfdefs_mac.cpp
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
 * This file contains some Mac OS X specific code for klfdefs.cpp
 */

#include <Cocoa/Cocoa.h>

#import <IOKit/ps/IOPowerSources.h>
#import <IOKit/ps/IOPSKeys.h>

#include <klfdefs.h>
#include <klfsysinfo.h>


KLF_EXPORT QString klf_defs_sysinfo_arch()
{
  static bool is_64 = (sizeof(void*) == 8);

  SInt32 x;
  Gestalt(gestaltSysArchitecture, &x);
  if (x == gestaltPowerPC) {
    return is_64 ? QString::fromLatin1("ppc64") : QString::fromLatin1("ppc");
  } else if (x == gestaltIntel) {
    return is_64 ? QString::fromLatin1("x86_64") : QString::fromLatin1("x86");
  }
  return QString::fromLatin1("unknown");
}


static bool init_power_sources_info(CFTypeRef *blobptr, CFArrayRef *sourcesptr)
{
  *blobptr = IOPSCopyPowerSourcesInfo();
  *sourcesptr = IOPSCopyPowerSourcesList(*blobptr);
  if (CFArrayGetCount(*sourcesptr) == 0) {
    klfDbg("Could not retrieve battery information. May be a system without battery.") ;
    return false;  // Could not retrieve battery information.  System may not have a battery.
  }
  // we have a battery.
  return true;
}

KLF_EXPORT KLFSysInfo::BatteryInfo _klf_mac_battery_info()
{
  CFTypeRef blob;
  CFArrayRef sources;

  klfDbg("_klf_mac_battery_info()") ;

  bool have_battery = init_power_sources_info(&blob, &sources);

  KLFSysInfo::BatteryInfo info;

  if(!have_battery) {
    klfDbg("_klf_mac_battery_info(): unable to get battery info. Probably don't have a battery.") ;
    info.islaptop = false;
    info.onbatterypower = false;
    return info;
  }

  CFDictionaryRef pSource = IOPSGetPowerSourceDescription(blob, CFArrayGetValueAtIndex(sources, 0));

  bool powerConnected = [(NSString*)[(NSDictionary*)pSource objectForKey:@kIOPSPowerSourceStateKey]
				     isEqualToString:@kIOPSACPowerValue];
  klfDbg("power is connected?: "<<(bool)powerConnected) ;

  //BOOL outputCharging = [[(NSDictionary*)pSource objectForKey:@kIOPSIsChargingKey] boolValue];

  //   bool outputInstalled = [[(NSDictionary*)pSource objectForKey:@kIOPSIsPresentKey] boolValue];
  //   bool outputConnected = [(NSString*)[(NSDictionary*)pSource objectForKey:@kIOPSPowerSourceStateKey] isEqualToString:@kIOPSACPowerValue];
  //   //BOOL outputCharging = [[(NSDictionary*)pSource objectForKey:@kIOPSIsChargingKey] boolValue];
  //   double outputCurrent = [[(NSDictionary*)pSource objectForKey:@kIOPSCurrentKey] doubleValue];
  //   double outputVoltage = [[(NSDictionary*)pSource objectForKey:@kIOPSVoltageKey] doubleValue];
  //   double outputCapacity = [[(NSDictionary*)pSource objectForKey:@kIOPSCurrentCapacityKey] doubleValue];
  //   double outputMaxCapacity = [[(NSDictionary*)pSource objectForKey:@kIOPSMaxCapacityKey] doubleValue];
  
  //  klfDbg("battery status: installed="<<outputInstalled<<"; connected="<<outputConnected<<"; charging="<<outputCharging<<"; current="<<outputCurrent<<"; voltage="<<outputVoltage<<"; capacity="<<outputCapacity<<"; outputMaxCapacity="<<outputMaxCapacity);

  CFRelease(blob);
  CFRelease(sources);

  info.islaptop = true;
  info.onbatterypower = !powerConnected;

  return info;
}

KLF_EXPORT bool _klf_mac_is_laptop()
{
  CFTypeRef blob;
  CFArrayRef sources;

  bool have_battery = init_power_sources_info(&blob, &sources);

  CFRelease(blob);
  CFRelease(sources);
  return have_battery;
}

KLF_EXPORT bool _klf_mac_is_on_battery_power()
{
  KLFSysInfo::BatteryInfo inf = _klf_mac_battery_info();
  return inf.onbatterypower;
}

