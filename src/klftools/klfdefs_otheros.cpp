#include <klfdefs.h>
#include <klfsysinfo.h>

KLF_EXPORT QString klf_defs_sysinfo_arch()
{
  return QString::fromLatin1("Unknown architecture and OS");
}


KLF_EXPORT KLFSysInfo::BatteryInfo _klf_otheros_battery_info()
{
  KLFSysInfo::BatteryInfo info;

  klfWarning("Could not get battery status.") ;
  info.islaptop = false;
  info.onbatterypower = false;
  return info;
}

KLF_EXPORT bool _klf_otheros_is_laptop()
{
  KLFSysInfo::BatteryInfo info;
  info = _klf_otheros_battery_info();
  return info.islaptop;
}

KLF_EXPORT bool _klf_otheros_is_on_battery_power()
{
  KLFSysInfo::BatteryInfo info;
  info = _klf_otheros_battery_info();
  return info.onbatterypower;
}

