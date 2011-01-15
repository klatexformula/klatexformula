/***************************************************************************
 *   file klflatexsymbols_p.h
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


/** \file
 * This file contains internal definitions for file klflatexsymbols.cpp.
 * \internal
 */

#ifndef KLFLATEXSYMBOLS_P_H
#define KLFLATEXSYMBOLS_P_H

#include <QTimer>
#include <QToolTip>
#include <QScrollBar>

#include <klfsearchbar.h>

#include <ui_klflatexsymbols.h>
#include "klflatexsymbols.h"



struct SymbolInfo
{
  SymbolInfo() : pix(), confirmed(false) { }
  
  QPixmap pix;
  bool confirmed;
};

struct KLFLatexSymbolsCachePrivate
{
  QHash<KLFLatexSymbol,SymbolInfo> cache;
  bool flag_modified;
  KLFBackend::klfSettings backendsettings;
};

// WARNING: Does NOT write s.confirmed (!)
inline QDataStream& operator<<(QDataStream& stream, const SymbolInfo& s)
{
  return stream << s.pix;
}

// WARNING: Does NOT read s.confirmed (!), sets to false.
inline QDataStream& operator>>(QDataStream& stream, SymbolInfo& s)
{
  s.confirmed = false;
  return stream >> s.pix;
}






struct KLFLatexSymbolsSearchIterator
{
  KLFLatexSymbolsSearchIterator(int iv = 0, int is = 0)
    : iview(iv), isymb(is) { }
  int iview;
  int isymb;

  inline bool operator==(const KLFLatexSymbolsSearchIterator& other) const
  {
    return iview == other.iview && isymb == other.isymb;
  }
};


inline QDebug& operator<<(QDebug& str, const KLFLatexSymbolsSearchIterator& it)
{
  return str<<"LtxSymbIter(v="<<it.iview<<",s="<<it.isymb<<")";
}



class KLFLatexSymbolsSearchable : public QObject, public KLFIteratorSearchable<KLFLatexSymbolsSearchIterator>
{
  Q_OBJECT
private:
  KLFLatexSymbols *s;
  QTimer tooltiptimer;
  QPoint tooltippos;
  QString tooltiptext;
  QWidget *tooltipwidget;

public:
  KLFLatexSymbolsSearchable(KLFLatexSymbols *symb) : QObject(symb), s(symb)
  {
    connect(&tooltiptimer, SIGNAL(timeout()), this, SLOT(slotToolTip()));
  }
  virtual ~KLFLatexSymbolsSearchable() { }

public slots:
  void slotToolTip()
  {
    QToolTip::showText(tooltippos, tooltiptext, tooltipwidget);
  }

  void slotInsertCurrentMatch()
  {
    SearchIterator cur = searchCurrentIterPos();
    if (cur == searchIterEnd())
      return;
    KLFLatexSymbolsView *v = view(cur);
    if (v == NULL)
      return;
    if (cur.isymb >= 0 && cur.isymb < v->_symbols.size())
      s->emitInsertSymbol(v->_symbols[cur.isymb]);
  }

public:

  // reimplemented from KLFIteratorSearchable

  virtual SearchIterator searchIterBegin() {
    int iview = 0;
    while (iview < s->mViews.size() && s->mViews[iview]->mSymbols.isEmpty())
      ++iview;
    return KLFLatexSymbolsSearchIterator(iview, 0);
  }
  virtual SearchIterator searchIterEnd() {
    return KLFLatexSymbolsSearchIterator(s->mViews.size(), 0);
  }
  virtual SearchIterator searchIterAdvance(const SearchIterator& pos, bool forward)
  {
    klfDbg("pos="<<pos<<"; forward="<<forward);
    SearchIterator it = pos;
    // sanity
    KLF_ASSERT_CONDITION( !(forward && it == searchIterEnd()), "Attempt to advance 'end' iterator!",
			  return searchIterEnd() ) ;
    KLF_ASSERT_CONDITION( !(!forward && it == searchIterBegin()), "Attempt to decrement 'begin' iterator!",
			  return searchIterBegin() ) ;
    KLFLatexSymbolsView *v = view(it);
    if (forward) {
      // advance
      it.isymb++;
      while (v != NULL && it.isymb >= v->mSymbols.size()) {
	it.iview++; it.isymb = 0;
	v = view(it);
      }
      return KLF_DEBUG_TEE(it);
    } else {
      // decrement
      it.isymb--;
      while (it.iview > 0 && (it.isymb < 0 || (v != NULL && it.isymb >= v->mSymbols.size()))) {
	klfDbg("decrementing: it="<<it) ;
	it.iview--;
	v = view(it);
	it.isymb =  (v!=NULL) ? v->mSymbols.size()-1 : 0 ;
      }
      return KLF_DEBUG_TEE(it);
    }
  }

  virtual SearchIterator searchIterStartFrom(bool forward)
  {
    // current symbols page
    int i = s->u->cbxCategory->currentIndex();
    KLFLatexSymbolsSearchIterator it(i, 0);
    while (it.iview < s->mViews.size() && s->mViews[it.iview]->mSymbols.isEmpty())
      ++it.iview;
    // if reverse, set on last symbol of page
    if (!forward && it.iview < s->mViews.size()) {
      it.isymb = s->mViews[it.iview]->mSymbols.size();
    }
    it = searchAdvanceIteratorCycle(it, forward ? -1 : 1);
    klfDbg("Starting search from "<<it) ;
    return it;
  }

  virtual bool searchIterMatches(const SearchIterator& pos, const QString& queryString)
  {
    KLFLatexSymbolsView *v = view(pos);
    KLF_ASSERT_NOT_NULL(v, "View for "<<pos<<" is NULL!", return false; ) ;

    return v->symbolMatches(pos.isymb, queryString);
  }

  virtual void searchMoveToIterPos(const SearchIterator& pos)
  {
    QToolTip::hideText();
    if (!(pos == searchIterEnd())) {
      KLFLatexSymbolsView *v = view(pos);
      KLF_ASSERT_NOT_NULL(v, "View for "<<pos<<" is NULL!", return ; ) ;
      s->u->cbxCategory->setCurrentIndex(pos.iview);
      s->stkViews->setCurrentWidget(v);
      QWidget *w = v->mSymbols[pos.isymb];
      KLF_ASSERT_CONDITION_ELSE(pos.isymb >= 0 && pos.isymb < v->mSymbols.size(),
				"Invalid pos object:"<<pos<<"; symb list size="<<v->mSymbols.size(),
				;) {
	v->ensureWidgetVisible(w);
      }
      klfDbg("pos is "<<pos) ;
      // show tooltip after some delay
      tooltiptext = w->toolTip();
      tooltippos = v->viewport()->mapToGlobal(w->geometry().center() -
					      QPoint(v->horizontalScrollBar()->value(),
						     v->verticalScrollBar()->value()));
      tooltipwidget = w;
      tooltiptimer.stop();
      tooltiptimer.setSingleShot(true);
      tooltiptimer.setInterval(1000);
      tooltiptimer.start();
    }
  }
  virtual void searchPerformed(const SearchIterator& pos)
  {
    if (!(pos == searchIterEnd())) {
      KLFLatexSymbolsView *v = view(pos);
      KLF_ASSERT_NOT_NULL(v, "View for "<<pos<<" is NULL!", return ; ) ;
      v->highlightSearchMatches(pos.isymb, searchQueryString());
    } else {
      int i = s->u->cbxCategory->currentIndex();
      KLF_ASSERT_CONDITION_ELSE(i>=0 && i<s->mViews.size(), "bad current view index "<<i<<"!", ;) {
	KLFLatexSymbolsView *v = s->mViews[i];
	// `current match' at 1 past end, so as to highlight all other matches, too
	v->highlightSearchMatches(v->mSymbols.size(), searchQueryString());
      }
    }
    klfDbg("pos is is "<<pos) ;
    KLFIteratorSearchable<KLFLatexSymbolsSearchIterator>::searchPerformed(pos);
  }
  virtual void searchAborted()
  {
    int i = s->u->cbxCategory->currentIndex();
    KLF_ASSERT_CONDITION_ELSE(i>=0 && i<s->mViews.size(), "bad current view index "<<i<<"!", ;) {
      KLFLatexSymbolsView *v = s->mViews[i];
      v->highlightSearchMatches(-1, searchQueryString());
    }
    KLFIteratorSearchable<KLFLatexSymbolsSearchIterator>::searchAborted();
  }
  virtual void searchReinitialized()
  {
    int i = s->u->cbxCategory->currentIndex();
    KLF_ASSERT_CONDITION_ELSE(i>=0 && i<s->mViews.size(), "bad current view index "<<i<<"!", ;) {
      KLFLatexSymbolsView *v = s->mViews[i];
      v->highlightSearchMatches(-1, searchQueryString());
    }
    KLFIteratorSearchable<KLFLatexSymbolsSearchIterator>::searchReinitialized();
  }


  KLFLatexSymbolsView *view(SearchIterator it) {
    if (it.iview < 0 || it.iview >= s->mViews.size())
      return NULL;
    return s->mViews[it.iview];
  }

};


#endif
