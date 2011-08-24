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

#include <klfdefs.h>

#include <CoreServices/CoreServices.h>


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
