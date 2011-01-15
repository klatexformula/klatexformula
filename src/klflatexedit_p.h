/***************************************************************************
 *   file klflatexedit_p.h
 *   This file is part of the KLatexFormula Project.
 *   Copyright (C) 2010 by Philippe Faist
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

#include "klflatexedit.h"


struct ParenItem {
  ParenItem(int ps = -1, bool h = false, char c = 0, bool l = false)
    : pos(ps), highlight(h), ch(c), left(l) { }
  int pos;
  bool highlight;
  char ch;
  bool left; //!< if it's \left( instead of (


  // defined in klflatexedit.cpp
  static const char openparenlist[];
  static const char closeparenlist[];

  static bool match(char a_ch, bool a_left, char b_ch, bool b_right)
  {
    if (a_left != b_right)
      return false;
    if ( a_left && (a_ch == '.' || b_ch == '.') ) {
      // special case with \left( blablabla \right.  or  \left. blablabla \right)
      // so report a match
      return true;
    }
    int k;
    for (k = 0; openparenlist[k] != '\0' && closeparenlist[k] != '\0'; ++k) {
      if (a_ch == openparenlist[k])
	return  (b_ch == closeparenlist[k]);
      if (b_ch == closeparenlist[k])
	return false; // because a_ch != openparenlist[k]
    }
    if (a_ch == b_ch)
      return true;

    // otherwise, mismatch
    return false;
  }
};

