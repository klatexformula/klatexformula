/***************************************************************************
 *   file klfmainwin_mac.mm
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


#include <Cocoa/Cocoa.h>

#include <klfmainwin.h>


#ifdef QT_MAC_USE_COCOA

/*
// WORKS!
void DEBUG_test()
{
  NSWorkspace *w = [NSWorkspace sharedWorkspace];
  [w openFile:@"/Users/philippe/temp/schrodinger-equation.png" withApplication:@"/Users/philippe/projects/klf/klatexformula/build/src/klatexformula-3.3.0alpha.app"];
}
*/

void klf_mac_hide_application(const KLFMainWin *mw, bool hide)
{
  NSApplication * a = [NSApplication sharedApplication];
  NSView *mw_mac = reinterpret_cast<NSView *>(mw->window()->winId());

  if (hide)
    [a hide:mw_mac];
  else
    [a unhide:mw_mac];
}

void klf_mac_hide_application(const KLFMainWin *mw)
{
  klf_mac_hide_application(mw, true);
}

bool klf_mac_unhide_application(const KLFMainWin *mw)
{
  klf_mac_hide_application(mw, false);
  return true;
}

bool klf_mac_application_hidden(const KLFMainWin */*mw*/)
{
  NSApplication * a = [NSApplication sharedApplication];
  //NSView *mw_mac = reinterpret_cast<NSView *>(mw->window()->winId());

  return ([a isHidden] == YES);
}

#else // QT_MAC_USE_COCOA

void klf_mac_hide_application(const KLFMainWin *mw)
{
  klfDbg("Not implemented without Cocoa.") ;
}

#endif

