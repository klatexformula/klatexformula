/***************************************************************************
 *   file klfx11.cpp
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

#include "../klfmainwin.h"

#include <QX11Info>

#include <X11/Xlib.h>
#include <X11/Xatom.h>


/*
bool KLFMainWin::x11Event(XEvent *event)
{
  KLF_DEBUG_BLOCK(KLF_FUNC_NAME) ;

  if (event->type == PropertyNotify) {
    XPropertyEvent xpropevent = event->xproperty;
    Atom wm_state = XInternAtom(xpropevent.display, "_NET_WM_STATE", False);
    if (xpropevent.atom == wm_state && xpropevent.state == PropertyNewValue) {
      Atom actual_type;
      int actual_format;
      unsigned long num_items = 0;
      unsigned long bytes_left = 0;
      Atom *data_ptr;
      if (XGetWindowProperty(xpropevent.display, xpropevent.window, wm_state, 0, 0xffffffff, False, XA_ATOM,
			     &actual_type, &actual_format, &num_items, &bytes_left,
			     (unsigned char **) &data_ptr) == Success) {

	Atom wm_state_shaded = XInternAtom(xpropevent.display, "_NET_WM_STATE_SHADED", True);
	uint i;
	bool is_shaded = false;
	for (i = 0; i < num_items; ++i) {
	  if (data_ptr[i] == wm_state_shaded) {
	    // window is shaded
	    is_shaded = true;
	  }
	}

	// set custom QObject property on this window
	bool shade_state_changed = property("x11WindowShaded").toBool() != is_shaded;
	setProperty("x11WindowShaded", QVariant::fromValue<bool>(is_shaded));

	XFree(data_ptr);
	if (shade_state_changed) {
	  // this events results from a shade/unshade event, do NOT let Qt translate it
	  // to a show/hide event
	  return true;
	}
      } else {
	fprintf(stderr, "KLFMainWin::x11Event: XGetWindowProperty fail (atom=%d).\n", (int)wm_state);
      }
    }
  }

  return false;
}
*/
