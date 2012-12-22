/***************************************************************************
 *   file $FILENAME$
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

#ifndef KLFMACDEFS_H
#define KLFMACDEFS_H

// this needs to be included _after_ inclusion of mac cocoa stuff

class KLFMacAutoReleaseBlock
{
public:
  KLFMacAutoReleaseBlock()
  {
    pool = [[NSAutoreleasePool alloc] init];
  }
  ~KLFMacAutoReleaseBlock()
  {
    [pool drain];
  }

private:
  NSAutoreleasePool *pool;
};

#define MAC_AUTORELEASE_BLOCK				\
  KLFMacAutoReleaseBlock __klf_mac_autorelease_block;






#endif
