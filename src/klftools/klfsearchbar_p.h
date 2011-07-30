/***************************************************************************
 *   file klfsearchbar_p.h
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

#ifndef KLFSEARCHBAR_P_H
#define KLFSEARCHBAR_P_H

#include <QObject>

#include "klfsearchbar.h"

/** \file
 * This header contains (in principle private) auxiliary classes for
 * library routines defined in klfsearchbar.cpp */


struct KLFSearchBarPrivate
{
  KLFSearchBarPrivate() : pCurPos(KLFPosSearchable::Pos()),
			  pLastPos(KLFPosSearchable::Pos())
  {
  }

  /** This is set by focusOrNext() or focusOrPrev() to remember in which direction to
   * search when text in search bar is changed and thus find() is called. */
  bool pSearchForward;
  bool pIsSearching;
  /** Current search bar state. */
  KLFSearchBar::SearchState pState;
  QString pSearchText;
  KLFPosSearchable::Pos pCurPos;
  KLFPosSearchable::Pos pLastPos;

  QString pLastSearchText;

  KLFWaitAnimationOverlay *pWaitLabel;

  bool pAutoHide;

  bool pShowOverlayMode;
  QRect pShowOverlayRelativeGeometry;

  QString pFocusOutText;

  bool pIsFinding;
  bool pHasQueuedFind;
  QString pQueuedFindString;
  bool pQueuedFindForward;

  /** <i>Emacs-Style Backspace</i>: typing backspace key comes back to previous match if any, otherwise
   * erases last char of search string. */
  bool pUseEsbs;
  
  /** \internal
   *
   * For a given search string, eg. \c "fouri", stores the list of visited matching positions. Each time
   * a new letter is typed, a new HistBuffer is allocated. The previous HistBuffer is recalled when backspace
   * is called enough times (to 'undo' the searches). */
  struct HistBuffer {
    struct CurLastPosPair {
      CurLastPosPair(const KLFPosSearchable::Pos& c = KLFPosSearchable::Pos(),
		     const KLFPosSearchable::Pos& l = KLFPosSearchable::Pos(),
		     bool forward = true)
	: cur(c), last(l), reachedForward(forward)  { }
      KLFPosSearchable::Pos cur;
      KLFPosSearchable::Pos last;
      bool reachedForward;
    };
    QString str; //!< Query String
    QList<CurLastPosPair> poslist; //!< list of visited matching positions
  };
  /**
   * This list stores for each typed letter a list corresponding to the history of the match positions. For
   * example, if user types
   * <pre>C-S   f   o    u    r   C-S   C-S   i    e    r   C-S   C-R   C-R</pre>
   * this list might then be
   * <pre>  ['f', 0]  ['fo',0]  ['fou',1]  ['four',1 3 4] ['fouri',4]  ['fourie',4]  ['fourier',4 7 4 3]</pre>
   * Assuming the searched list is for example
   * <pre> 0 foo
   * 1 fourier
   * 2 something else
   * 3 fourier analysis
   * 4 continuous fourier transform
   * 5 ABC
   * 6 ZZZ
   * 7 fourier series
   * </pre>
   */
  QList<HistBuffer> esbs_histbuffer;

};


inline QDebug& operator<<(QDebug& str, const KLFSearchBarPrivate::HistBuffer::CurLastPosPair& cl)
{
  return str << "curpos="<<cl.cur<<"/lastpos="<<cl.last;
}
inline QDebug& operator<<(QDebug& str, const KLFSearchBarPrivate::HistBuffer& hb)
{
  return str << "SearchHistBuffer("<<hb.str<<", "<<hb.poslist<<")";
}



#endif
