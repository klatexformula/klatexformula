/***************************************************************************
 *   file klfdefs_win.cpp
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
 * This file contains some MS-Windows specific code for klfdefs.cpp
 */

#include <klfdefs.h>

#include <windows.h>

#if defined(_M_IX86)
static const char * arch = "x86";
#elif defined(_M_AMD64)
static const char * arch = "x86_64";
#elif defined(_WIN64)
static const char * arch = "win64";
#else
static const char * arch = "unknown";
#error "Unknown Processor Architecture."
#endif

KLF_EXPORT QString klf_defs_sysinfo_arch()
{
  return QString::fromLatin1(arch);
}


