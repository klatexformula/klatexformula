/***************************************************************************
 *   file klfsearchbar.cpp
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

#include <QDebug>
#include <QObject>
#include <QFrame>
#include <QLineEdit>
#include <QEvent>
#include <QKeyEvent>
#include <QShortcut>
#include <QKeySequence>
#include <QTime>

#include <klfguiutil.h>

#include <ui_klfsearchbar.h>
#include "klfsearchbar.h"
#include "klfsearchbar_p.h"



QDebug& operator<<(QDebug& str, const KLFPosSearchable::Pos& pos)
{
  QString s;
  s.sprintf("%p", (const KLFPosSearchable::Pos::PosData*)pos.posdata);
  return str << "Pos("<<pos.pos<<", "<<qPrintable(s)<<";valid="<<pos.valid()<<")";
}

// --

KLFPosSearchable::Pos KLFPosSearchable::searchStartFrom(bool forward)
{
  klfDbg("start from: base implementation, will return invalid. forward="<<forward) ;
  return invalidPos();
}

void KLFPosSearchable::setSearchInterruptRequested(bool on)
{
  klfDbg("on="<<on) ;
  pInterruptRequested = on;
}


// ------------------------------------------------


KLFPosSearchableProxy::~KLFPosSearchableProxy()
{
}

KLFPosSearchable::Pos KLFPosSearchableProxy::searchStartFrom(bool forward)
{
  KLF_ASSERT_NOT_NULL( target(), "Search target is NULL!", return invalidPos() );
  return target()->searchStartFrom(forward);
}

KLFPosSearchable::Pos KLFPosSearchableProxy::searchFind(const QString& queryString, const Pos& fromPos, bool forward)
{
  KLF_ASSERT_NOT_NULL( target(), "Search target is NULL!", return invalidPos() );
  return target()->searchFind(queryString, fromPos, forward);
}

void KLFPosSearchableProxy::searchMoveToPos(const Pos& pos)
{
  KLF_ASSERT_NOT_NULL( target(), "Search target is NULL!", return ; );
  return target()->searchMoveToPos(pos);
}

void KLFPosSearchableProxy::searchPerformed(const QString& queryString, bool found, const Pos& pos)
{
  KLF_ASSERT_NOT_NULL( target(), "Search target is NULL!", return ; );
  target()->searchPerformed(queryString, found, pos);
}

void KLFPosSearchableProxy::searchAborted()
{
  KLF_ASSERT_NOT_NULL( target(), "Search target is NULL!", return ; );
  target()->searchAborted();
}

KLFPosSearchable::Pos KLFPosSearchableProxy::invalidPos()
{
  KLF_ASSERT_NOT_NULL( target(), "Search target is NULL!", return invalidPos(); );
  return target()->invalidPos();
}

void KLFPosSearchableProxy::searchReinitialized()
{
  KLF_ASSERT_NOT_NULL( target(), "Search target is NULL!", return ; );
  target()->searchReinitialized();
}

QString KLFPosSearchableProxy::searchQueryString() const
{
  KLF_ASSERT_NOT_NULL( target(), "Search target is NULL!", return QString(); );
  return target()->searchQueryString();
}

void KLFPosSearchableProxy::setSearchQueryString(const QString& s)
{
  KLF_ASSERT_NOT_NULL( target(), "Search target is NULL!", return ; );
  target()->setSearchQueryString(s);
}

bool KLFPosSearchableProxy::searchHasInterruptRequested()
{
  KLF_ASSERT_NOT_NULL( target(), "Search target is NULL!", return false; );
  return target()->searchHasInterruptRequested();
}

void KLFPosSearchableProxy::setSearchInterruptRequested(bool on)
{
  KLF_ASSERT_NOT_NULL( target(), "Search target is NULL!", return ; );
  target()->setSearchInterruptRequested(on);
}



// ---


KLFSearchable::KLFSearchable()
{
}
KLFSearchable::~KLFSearchable()
{
}

KLFPosSearchable::Pos KLFSearchable::searchFind(const QString& queryString, const Pos& fromPos, bool forward)
{
  bool r;
  // simulate first search, then 'find next', by detecting if we're required to search from a given pos
  // (heuristic that will fail in special cases!!)
  if (!fromPos.valid())
    r = searchFind(queryString, forward);
  else
    r = searchFindNext(forward);
  Pos p = invalidPos();
  if (!r)
    return p;
  // valid pos, return pos '0'
  p.pos = 0;
  return p;
}


// -----

KLFSearchableProxy::~KLFSearchableProxy()
{
}

void KLFSearchableProxy::setTarget(KLFTarget *target)
{
  KLFSearchable *s = dynamic_cast<KLFSearchable*>(target);
  KLF_ASSERT_CONDITION( (s!=NULL) || (target==NULL),
			"target is not a valid KLFSearchable object !",
			return; ) ;
  KLFTargeter::setTarget(s);
}

bool KLFSearchableProxy::searchFind(const QString& queryString, bool forward)
{
  KLF_ASSERT_NOT_NULL( target(), "Search target is NULL!", return false );
  return target()->searchFind(queryString, forward);
}
bool KLFSearchableProxy::searchFindNext(bool forward)
{
  KLF_ASSERT_NOT_NULL( target(), "Search target is NULL!", return false );
  return target()->searchFindNext(forward);
}
void KLFSearchableProxy::searchAbort()
{
  KLF_ASSERT_NOT_NULL( target(), "Search target is NULL!", return );
  return target()->searchAbort();
}


// ------------------------

KLFSearchBar::KLFSearchBar(QWidget *parent)
  : QFrame(parent)
{
  KLF_DEBUG_TIME_BLOCK(KLF_FUNC_NAME) ;
  klfDbg("parent: "<<parent) ;

  d = new KLFSearchBarPrivate;

  u = new Ui::KLFSearchBar;
  u->setupUi(this);

  setSizePolicy(QSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed));

  u->txtSearch->installEventFilter(this);
  connect(u->btnSearchClear, SIGNAL(clicked()), this, SLOT(clear()));
  connect(u->txtSearch, SIGNAL(textChanged(const QString&)),
	  this, SLOT(find(const QString&)));
  connect(u->btnFindNext, SIGNAL(clicked()), this, SLOT(findNext()));
  connect(u->btnFindPrev, SIGNAL(clicked()), this, SLOT(findPrev()));

  QPalette defaultpal = u->txtSearch->palette();
  u->txtSearch->setProperty(palettePropName(Default).toAscii(), QVariant::fromValue<QPalette>(defaultpal));
  QPalette pal0 = defaultpal;
  pal0.setColor(QPalette::Text, QColor(180,180,180));
  pal0.setColor(QPalette::WindowText, QColor(180,180,180));
  pal0.setColor(u->txtSearch->foregroundRole(), QColor(180,180,180));
  u->txtSearch->setProperty(palettePropName(FocusOut).toAscii(), QVariant::fromValue<QPalette>(pal0));
  // default found/not-found colors
  setColorFound(QColor(128,255,128));
  setColorNotFound(QColor(255,128,128));

  connect(u->btnHide, SIGNAL(clicked()), this, SLOT(hide()));
  setShowHideButton(false); // not shown by default

  d->pWaitLabel = new KLFWaitAnimationOverlay(u->txtSearch);
  d->pWaitLabel->setWaitMovie(":/pics/wait_anim.mng");
  /* // amusing test
     d->pWaitLabel->setWaitMovie("/home/philippe/projects/klf/artwork/experimental/packman_anim.gif");
  */

  d->pAutoHide = false;

  d->pShowOverlayMode = false;
  // default relative geometry: position at (50%, 95%) (centered, quasi-bottom)
  //                            size     of (90%, 0%)   [remember: expanded to minimum size]
  d->pShowOverlayRelativeGeometry = QRect(QPoint(50, 95), QSize(90, 0));

  d->pFocusOutText = "  "+tr("Hit Ctrl-F, Ctrl-S or / to start searching");

  d->pSearchForward = true;
  d->pSearchText = QString();
  d->pIsSearching = false;
  d->pCurPos = KLFPosSearchable::Pos::staticInvalidPos();
  d->pLastPos = KLFPosSearchable::Pos::staticInvalidPos();
  d->pState = FocusOut;
  displayState(FocusOut);

  d->pIsFinding = false;
  d->pHasQueuedFind = false;
  d->pQueuedFindString = QString();
  d->pQueuedFindForward = true;

  klfDbg("pCurPos is "<<d->pCurPos<<"; pLastPos is "<<d->pLastPos) ;

  d->pUseEsbs = true;
}
KLFSearchBar::~KLFSearchBar()
{
  delete d;
}

QString KLFSearchBar::currentSearchText() const
{
  return u->txtSearch->text();
}

QColor KLFSearchBar::colorFound() const
{
  QPalette p = u->txtSearch->property(palettePropName(Found).toAscii()).value<QPalette>();
  return p.color(QPalette::Base);
}
QColor KLFSearchBar::colorNotFound() const
{
  QPalette p = u->txtSearch->property(palettePropName(NotFound).toAscii()).value<QPalette>();
  return p.color(QPalette::Base);
}

bool KLFSearchBar::hideButtonShown() const
{
  return u->btnHide->isVisible();
}

bool KLFSearchBar::showSearchLabel() const
{
  return u->lblSearch->isVisible();
}

bool KLFSearchBar::emacsStyleBackspace() const
{
  return d->pUseEsbs;
}

KLFPosSearchable::Pos KLFSearchBar::currentSearchPos() const
{
  return d->pCurPos;
}

KLFSearchBar::SearchState KLFSearchBar::currentState() const
{
  return d->pState;
}


void KLFSearchBar::setColorFound(const QColor& color)
{
  QPalette pal1 = u->txtSearch->property(palettePropName(Default).toAscii()).value<QPalette>();
  pal1.setColor(QPalette::Base, color);
  pal1.setColor(QPalette::Window, color);
  pal1.setColor(u->txtSearch->backgroundRole(), color);
  u->txtSearch->setProperty(palettePropName(Found).toAscii(), QVariant::fromValue<QPalette>(pal1));
}

void KLFSearchBar::setColorNotFound(const QColor& color)
{
  QPalette pal2 = u->txtSearch->property(palettePropName(Default).toAscii()).value<QPalette>();
  pal2.setColor(QPalette::Base, color);
  pal2.setColor(QPalette::Window, color);
  pal2.setColor(u->txtSearch->backgroundRole(), color);
  u->txtSearch->setProperty(palettePropName(NotFound).toAscii(), QVariant::fromValue<QPalette>(pal2));
}

void KLFSearchBar::setShowHideButton(bool showHideButton)
{
  u->btnHide->setShown(showHideButton);
}

void KLFSearchBar::setShowSearchLabel(bool on)
{
  u->lblSearch->setShown(on);
}

void KLFSearchBar::setEmacsStyleBackspace(bool on)
{
  if (d->pIsSearching)
    abortSearch();
  d->pUseEsbs = on;
}


/** \internal */
#define DECLARE_SEARCH_SHORTCUT(shortcut, parent, slotmember)		\
  { QShortcut *s = new QShortcut(parent); s->setKey(QKeySequence(shortcut)); \
    connect(s, SIGNAL(activated()), this, slotmember); }

void KLFSearchBar::registerShortcuts(QWidget *parent)
{
  DECLARE_SEARCH_SHORTCUT(tr("Ctrl+F", "[[find]]"), parent, SLOT(focusOrNext()));
  DECLARE_SEARCH_SHORTCUT(tr("Ctrl+S", "[[find]]"), parent, SLOT(focusOrNext()));
  DECLARE_SEARCH_SHORTCUT(tr("/", "[[find]]"), parent, SLOT(clear()));
  DECLARE_SEARCH_SHORTCUT(tr("F3", "[[find next]]"), parent, SLOT(findNext()));
  DECLARE_SEARCH_SHORTCUT(tr("Shift+F3", "[[find prev]]"), parent, SLOT(findPrev()));
  DECLARE_SEARCH_SHORTCUT(tr("Ctrl+R", "[[find rev]]"), parent, SLOT(focusOrPrev()));
  // Esc will be captured through event filter so that it isn't too obstrusive...
}

void KLFSearchBar::setTarget(KLFTarget * target)
{
  if (d->pIsSearching)
    abortSearch();

  KLFPosSearchable *s = dynamic_cast<KLFPosSearchable*>(target);
  KLF_ASSERT_CONDITION( (s!=NULL) || (target==NULL),
			"target is not a valid KLFPosSearchable object !",
			return; ) ;
  KLFTargeter::setTarget(s);
}

void KLFSearchBar::setSearchText(const QString& text)
{
  u->txtSearch->setText(text);
}

bool KLFSearchBar::autoHide() const
{
  return d->pAutoHide;
}

bool KLFSearchBar::showOverlayMode() const
{
  return d->pShowOverlayMode;
}
QRect KLFSearchBar::showOverlayRelativeGeometry() const
{
  return d->pShowOverlayRelativeGeometry;
}
QString KLFSearchBar::focusOutText() const
{
  return d->pFocusOutText;
}


void KLFSearchBar::setFocusOutText(const QString& focusOutText)
{
  d->pFocusOutText = focusOutText;
}


static KLFSearchBarPrivate::HistBuffer::CurLastPosPair
/* */ esbs_get_last_pos(const QList<KLFSearchBarPrivate::HistBuffer>& hb, int offset = 0)
{
  if (hb.size() <= offset) { // there is no preceeding histbuffer
    return KLFSearchBarPrivate::HistBuffer::CurLastPosPair(KLFPosSearchable::Pos::staticInvalidPos(),
							   KLFPosSearchable::Pos::staticInvalidPos());
  }
  const QList<KLFSearchBarPrivate::HistBuffer::CurLastPosPair>& poslist = hb[hb.size()-1 - offset].poslist;

  if (poslist.isEmpty()) {
    return esbs_get_last_pos(hb, offset+1);
  }
  return poslist.last();
}

bool KLFSearchBar::eventFilter(QObject *obj, QEvent *ev)
{
  KLF_DEBUG_TIME_BLOCK(KLF_FUNC_NAME) ;

  if (obj == u->txtSearch) {
    if (ev->type() == QEvent::FocusIn) {
      klfDbg("focus-in event...") ;
      slotSearchFocusIn();
      // don't eat event
    } else if (ev->type() == QEvent::FocusOut) {
      klfDbg("focus-out event...") ;
      slotSearchFocusOut();
      // don't eat event
    } else if (ev->type() == QEvent::KeyPress) {
      klfDbg("key press event!") ;
      QKeyEvent *ke = (QKeyEvent*)ev;
      if (ke->key() == Qt::Key_Escape) {
	abortSearch();
	emit escapePressed();
	return true;
      }
      // Emacs-Style Backspace handling
      if (d->pUseEsbs) {
	// what kind of key press is this
	if (ke->key() == Qt::Key_Backspace) {
	  // if searching, backspace interrupts the search.
	  if (d->pIsFinding) {
	    KLF_ASSERT_CONDITION_ELSE(target()!=NULL, "Search Target is NULL!", ; )  {
	      target()->setSearchInterruptRequested(true);
	      return true;
	    }
	  }
	  if ( ! d->esbs_histbuffer.size() ) {
	    // back to beginning of text buffer...
	    promptEmptySearch();
	  } else {
	    // there is a current history buffer
	    KLFSearchBarPrivate::HistBuffer& histbuf = d->esbs_histbuffer.last();
	    if (histbuf.poslist.size() > 1) {
	      // jump to previous match
	      histbuf.poslist.pop_back();
	      const KLFSearchBarPrivate::HistBuffer::CurLastPosPair& pos = histbuf.poslist.last();
	      // move to previous match
	      KLF_ASSERT_CONDITION_ELSE(target()!=NULL, "Search Target is NULL!", ; )  {
		d->pCurPos = pos.cur;
		d->pLastPos = pos.last;
		// move to given pos, and present found result
		target()->searchMoveToPos(d->pCurPos);
		target()->searchPerformed(d->pSearchText, d->pCurPos.valid(), d->pCurPos);
		updateSearchFound(d->pCurPos.valid());
	      }
	    } else {
	      d->esbs_histbuffer.pop_back();
	      // if there is left 
	      if (!d->esbs_histbuffer.size()) {
		// back to beginning of text buffer...
		promptEmptySearch();
	      } else {
		// remove last item in buffer
		d->pSearchText = d->esbs_histbuffer.last().str;
		// check if there actually is text left
		u->txtSearch->blockSignals(true);
		u->txtSearch->setText(d->pSearchText);
		u->txtSearch->blockSignals(false);
		const QList<KLFSearchBarPrivate::HistBuffer::CurLastPosPair> poslist
		  = d->esbs_histbuffer.last().poslist;
		KLF_ASSERT_CONDITION_ELSE(target()!=NULL, "Search Target is NULL!", ; ) {
		  // find the last position where we were
		  KLFSearchBarPrivate::HistBuffer::CurLastPosPair clpos = esbs_get_last_pos(d->esbs_histbuffer);
		  d->pCurPos = clpos.cur;
		  d->pLastPos = clpos.last;
		  // make sure query string is up-to-date
		  target()->setSearchQueryString(d->pSearchText);
		  if (poslist.isEmpty()) {
		    // can be the result of an interrupted find, search from the preceeding poslist
		    d->pCurPos = d->pLastPos;
		    findNext(clpos.reachedForward);
		  } else {
		    target()->searchMoveToPos(d->pCurPos);
		    target()->searchPerformed(d->pSearchText, d->pCurPos.valid(), d->pCurPos);
		    updateSearchFound(d->pCurPos.valid());
		  }
		}
	      }
	    }
	  }
	  // in every case, eat the event
	  return true;
	} else if (ke->key() == Qt::Key_Left || ke->key() == Qt::Key_Right) {
	  // no left/right navigation
	  return true;
	} else if (ke->key() == Qt::Key_Home || ke->key() == Qt::Key_End) {
	  // don't allow text navigation
	  return true;
	} else if (ke->text().size() && ke->text()[0].isPrint()) {
	  // pass on the event further to QLineEdit
	  // Also, it is in find() that will we will create a new HistBuffer for this exact
	  // new partial search string.
	}
      } // if (use e-s-b-s)
      else {
	klfDbg("key press, but not using e-s-b-s.");
	return false;
      }
    } // if (is key-press)

  }
  return QFrame::eventFilter(obj, ev);
}

QLineEdit * KLFSearchBar::editor()
{
  return u->txtSearch;
}

void KLFSearchBar::setAutoHide(bool autohide)
{
  klfDbg("autohide="<<autohide) ;
  d->pAutoHide = autohide;
  if (d->pAutoHide && !searchBarHasFocus())
    hide();
}

void KLFSearchBar::setShowOverlayMode(bool overlayMode)
{
  klfDbg("setting show overlay mode to "<<overlayMode) ;
  d->pShowOverlayMode = overlayMode;
  setProperty("klfShowOverlayMode", QVariant::fromValue<bool>(d->pShowOverlayMode));
  // cheat with klfTopLevelWidget property, set it always in show-overlay-mode
  setProperty("klfTopLevelWidget", QVariant::fromValue<bool>(d->pShowOverlayMode));

  adjustOverlayGeometry();
  /** \todo ..... the search bar should install an event filter on the parent to listen
   * for resize events, and to resize appropriately. */
}

void KLFSearchBar::setShowOverlayRelativeGeometry(const QRect& relativeGeometryPercent)
{
  d->pShowOverlayRelativeGeometry = relativeGeometryPercent;
  adjustOverlayGeometry();
}
void KLFSearchBar::setShowOverlayRelativeGeometry(int widthPercent, int heightPercent,
						  int positionXPercent, int positionYPercent)
{
  setShowOverlayRelativeGeometry(QRect(QPoint(positionXPercent, positionYPercent),
				       QSize(widthPercent, heightPercent)));
  adjustOverlayGeometry();
}



void KLFSearchBar::clear()
{
  klfDbgT("clear") ;
  setSearchText("");
  focus();
}

void KLFSearchBar::focusOrNext(bool forward)
{
  d->pSearchForward = forward;

  if (QApplication::focusWidget() == u->txtSearch) {
    klfDbgT("already have focus") ;
    // already has focus
    // -> either recall history (if empty search text)
    // -> or find next
    if (u->txtSearch->text().isEmpty()) {
      setSearchText(d->pLastSearchText);
    } else {
      if (!d->pIsSearching) {
	find(u->txtSearch->text(), forward);
      } else {
	findNext(forward);
      }
    }
  } else {
    klfDbgT("setting focus") ;
    setSearchText("");
    focus();
  }
}

void KLFSearchBar::find(const QString& string)
{
  find(string, d->pSearchForward);
}

void KLFSearchBar::find(const QString& text, bool forward)
{
  KLF_DEBUG_BLOCK(KLF_FUNC_NAME) ;
  klfDbgT("text="<<text<<", forward="<<forward) ;

  if (target() == NULL && text.isEmpty())
    return;

  KLF_ASSERT_NOT_NULL( target() , "search target is NULL!", return ) ;

  if (d->pIsFinding) {
    klfDbg("find()ing already. queueing and requesting to stop previous find.");
    target()->setSearchInterruptRequested(true);
    d->pHasQueuedFind = true;
    d->pQueuedFindString = text;
    d->pQueuedFindForward = forward;
    return;
  }

  if ( text.isEmpty() ||
       (d->pUseEsbs && text.mid(0, d->pSearchText.size()) != d->pSearchText) ) {
    promptEmptySearch();
    return;
  }

  if (!d->pIsSearching) {
    klfDbg("new search. find start from position.") ;
    // first find() call, started new search, start from suggested position
    d->pCurPos = target()->searchStartFrom(forward);
    d->pLastPos = d->pCurPos;
    klfDbg("Starting from d->pCurPos="<<d->pCurPos) ;
  }

  d->pIsSearching = true;
  d->pSearchText = text;
  performFind(forward, false);
}


// private
void KLFSearchBar::performFind(bool forward, bool isfindnext)
{
  KLF_DEBUG_BLOCK(KLF_FUNC_NAME) ;

  KLF_ASSERT_NOT_NULL( target() , "search target is NULL!", return ) ;

  if (!isfindnext) {
    // prepare this esbs-hist-buffer
    if (d->pUseEsbs) {
      KLFSearchBarPrivate::HistBuffer buf;
      buf.str = d->pSearchText;
      d->esbs_histbuffer << buf;
    }
  }

  klfDbg("pSearchText="<<d->pSearchText<<"; pCurPos="<<d->pCurPos<<"; pLastPos="<<d->pLastPos) ;

  // reset the interrupt request flag
  target()->setSearchInterruptRequested(false);

  d->pIsFinding = true;
  d->pWaitLabel->startWait();
  target()->setSearchQueryString(d->pSearchText);
  klfDbg("pSearchText="<<d->pSearchText<<"; target()->searchQueryString()="<<target()->searchQueryString()) ;
  KLFPosSearchable::Pos pos = target()->searchFind(d->pSearchText, d->pLastPos, forward);
  d->pWaitLabel->stopWait();
  d->pIsFinding = false;

  if (!d->pIsSearching) {
    // search was aborted.
    return;
  }

  d->pCurPos = pos;

  if (target()->searchHasInterruptRequested()) {
    // perform queued find
    if (d->pHasQueuedFind) {
      QString s = d->pQueuedFindString;
      bool f = d->pQueuedFindForward;
      d->pHasQueuedFind = false;
      d->pQueuedFindString = QString();
      d->pQueuedFindForward = true;
      find(s, f);
    }
    return;
  }

  target()->searchMoveToPos(d->pCurPos);
  target()->searchPerformed(d->pSearchText, d->pCurPos.valid(), d->pCurPos);
  updateSearchFound(d->pCurPos.valid());

  emitFoundSignals(d->pCurPos, d->pSearchText, forward);

  klfDbg("Are now at position pCurPos="<<d->pCurPos) ;

  if (d->pUseEsbs) {
    KLF_ASSERT_CONDITION_ELSE(d->esbs_histbuffer.size(), "HistBuffer is empty!!", ;)  {
      if (d->pCurPos.valid() ||
	// append invalid item only if poslist is empty or if the last poslist item is valid
	  (d->esbs_histbuffer.last().poslist.isEmpty() ||
	   d->esbs_histbuffer.last().poslist.last().cur.valid())) {
	d->esbs_histbuffer.last().poslist
	  << KLFSearchBarPrivate::HistBuffer::CurLastPosPair(d->pCurPos, d->pLastPos, forward);
      }
    }
  }
}

void KLFSearchBar::findNext(bool forward)
{
  klfDbgT("forward="<<forward) ;

  if (d->pIsFinding) {
    klfDbg("finding already. Must wait, ignoring request.");
    return;
  }

  // focus search bar if not yet focused.
  if (!searchBarHasFocus())
    focus();

  if (d->pSearchText.isEmpty()) {
    klfDbg("called but not in search mode. recalling history="<<d->pLastSearchText) ;
    // we're not in search mode
    // recall history
    showSearchBarText(d->pLastSearchText);

    // and initiate search mode
    find(u->txtSearch->text(), forward);
    return;
  }

  KLF_ASSERT_NOT_NULL( pTarget , "Search target is NULL!" , return ) ;

  d->pLastPos = d->pCurPos; // precisely, find _next_
  d->pLastSearchText = d->pSearchText;
  performFind(forward, true);
}

void KLFSearchBar::promptEmptySearch()
{
  setCurrentState(Default);
  u->txtSearch->blockSignals(true);
  u->txtSearch->setText("");
  u->txtSearch->blockSignals(false);
  d->pSearchText = QString();
  d->pCurPos = KLFPosSearchable::Pos::staticInvalidPos();
  d->pLastPos = KLFPosSearchable::Pos::staticInvalidPos();
  if (target() != NULL) {
    klfDbg("telling target to reinitialize search...") ;
    if (d->pIsFinding)
      target()->setSearchInterruptRequested(true);
    target()->setSearchQueryString(QString());
    target()->searchMoveToPos(d->pCurPos);
    target()->searchReinitialized();
  }
}

void KLFSearchBar::abortSearch()
{
  KLF_DEBUG_BLOCK(KLF_FUNC_NAME) ;

  d->pSearchText = QString();
  d->pIsSearching = false;
  d->pCurPos = KLFPosSearchable::Pos::staticInvalidPos();
  d->pLastPos = KLFPosSearchable::Pos::staticInvalidPos();
  klfDbg("pCurPos="<<d->pCurPos) ;

  if ( ! u->txtSearch->text().isEmpty() ) {
    showSearchBarText("");
  }
  if (d->pUseEsbs)
    d->esbs_histbuffer.clear();

  if (searchBarHasFocus()) {
    setCurrentState(Aborted);
  } else {
    setCurrentState(FocusOut);
  }

  if (target() != NULL) {
    klfDbg("telling target to abort search...") ;
    if (d->pIsFinding)
      target()->setSearchInterruptRequested(true);
    target()->searchAborted();
    target()->setSearchQueryString(QString());
    klfDbg("...done") ;
  }

  emit searchAborted();
}

void KLFSearchBar::adjustOverlayGeometry()
{
  if (d->pShowOverlayMode) {
    QWidget *pw = parentWidget();
    if (pw != NULL) {
      // if we have a parent widget, adjust using our relative geometry
      QSize pws = pw->size();
      
      QPoint relPos = d->pShowOverlayRelativeGeometry.topLeft();
      QSize relSz = d->pShowOverlayRelativeGeometry.size();
      
      QSize sz = QSize(pws.width()*relSz.width()/100, pws.height()*relSz.height()/100);
      sz = sz.expandedTo(minimumSizeHint()) ;
      QRect gm = QRect( QPoint( (pws.width()-sz.width())*relPos.x()/100, (pws.height()-sz.height())*relPos.y()/100 ),
			sz );
      klfDbg("Geometry is "<<gm) ;
      setGeometry(gm);
      //      setAutoFillBackground(true);
      setStyleSheet(styleSheet());
      raise();
    } else {
      // set some widget window flags if we're parent-less...
      setWindowFlags(Qt::Tool);
      // just for fun...
      setWindowOpacity(0.95);
    }
  }
}

void KLFSearchBar::focus()
{
  KLF_DEBUG_BLOCK(KLF_FUNC_NAME) ;

  if (d->pShowOverlayMode)
    adjustOverlayGeometry();

  if (!isVisible()) {
    // show the search bar. This works with in overlay mode as well as when the widget is hidden
    // with the hide button.
    show();
  }
  u->txtSearch->setFocus();
}

void KLFSearchBar::slotSearchFocusIn()
{
  klfDbgT("focus in") ;
  setCurrentState(Default);
  showSearchBarText("");
}

void KLFSearchBar::slotSearchFocusOut()
{
  klfDbgT("focus out") ;
  if (d->pAutoHide)
    hide();

  abortSearch();
}

void KLFSearchBar::updateSearchFound(bool found)
{
  setCurrentState(found ? Found : NotFound);
}

// private
QString KLFSearchBar::palettePropName(SearchState state) const
{
  switch (state) {
  case Default:  return QString("paletteDefault");
  case FocusOut: return QString("paletteFocusOut");
  case Found:    return QString("paletteFound");
  case NotFound: return QString("paletteNotFound");
  case Aborted:  return QString("paletteDefault");
  default:
    qWarning()<<KLF_FUNC_NAME<<": invalid state: "<<state;
  }
  return QString();
}
// private
QString KLFSearchBar::statePropValue(SearchState state) const
{
  switch (state) {
  case Default:  return QLatin1String("default");
  case FocusOut: return QLatin1String("focus-out");
  case Found:    return QLatin1String("found");
  case NotFound: return QLatin1String("not-found");
  case Aborted:  return QLatin1String("aborted");
  default:       return QLatin1String("invalid");
  }
}

void KLFSearchBar::setCurrentState(SearchState state)
{
  klfDbg("state: "<<state) ;

  if (d->pState == state)
    return;

  d->pState = state;
  displayState(state);
  emit stateChanged(state);
}

void KLFSearchBar::displayState(SearchState s)
{
  klfDbg("Setting state: "<<statePropValue(s));
  u->txtSearch->setProperty("searchState", statePropValue(s));
  QPalette pal = u->txtSearch->property(palettePropName(s).toAscii()).value<QPalette>();
  /// \todo should these next two lines be inverted? (ie. first set palette, then style sheet)
  u->txtSearch->setStyleSheet(u->txtSearch->styleSheet());
  u->txtSearch->setPalette(pal);
  u->txtSearch->update();

  if (s == FocusOut) {
    showSearchBarText(d->pFocusOutText);
  }
}

void KLFSearchBar::emitFoundSignals(const KLFPosSearchable::Pos& pos, const QString& searchstring, bool forward)
{
  bool resultfound = pos.valid();
  emit searchPerformed(resultfound);
  emit searchPerformed(searchstring, resultfound);
  if (resultfound) {
    emit found();
    emit found(d->pSearchText, forward);
    emit found(d->pSearchText, forward, pos);
  } else {
    emit didNotFind();
    emit didNotFind(d->pSearchText, forward);
  }
}

void KLFSearchBar::showSearchBarText(const QString& text)
{
  u->txtSearch->blockSignals(true);
  u->txtSearch->setText(text);
  if (d->pUseEsbs)
    d->esbs_histbuffer.clear();
  u->txtSearch->blockSignals(false);
}
bool KLFSearchBar::searchBarHasFocus()
{
  return  QApplication::focusWidget() == u->txtSearch;
}


bool KLFSearchBar::event(QEvent *event)
{
  if (event->type() == QEvent::Polish)
    setMinimumSize(minimumSizeHint());

  if (event->type() == QEvent::Show) {
    emit visibilityChanged(true);
  }
  if (event->type() == QEvent::Hide) {
    emit visibilityChanged(false);
  }

  return QFrame::event(event);
}
