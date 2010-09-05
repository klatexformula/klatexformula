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

#include <QObject>
#include <QFrame>
#include <QLineEdit>
#include <QEvent>
#include <QKeyEvent>
#include <QShortcut>
#include <QKeySequence>

#include <klfguiutil.h>

#include <ui_klfsearchbar.h>
#include "klfsearchbar.h"

KLFSearchable::KLFSearchable()
{
}
KLFSearchable::~KLFSearchable()
{
  int k;
  QList<KLFSearchBar*> bars = pTargetOf;
  for (k = 0; k < bars.size(); ++k) {
    bars[k]->pTarget = NULL;
  }
  QList<KLFSearchableProxy*> pl = pTargetOfProxy;
  for (k = 0; k < pl.size(); ++k) {
    pl[k]->pTarget = NULL;
  }
}

// -----

KLFSearchableProxy::~KLFSearchableProxy()
{
  if (pTarget != NULL)
    pTarget->pTargetOfProxy.removeAll(this);
}

void KLFSearchableProxy::setSearchTarget(KLFSearchable *target)
{
  if (pTarget != NULL)
    pTarget->pTargetOfProxy.removeAll(this);

  pTarget = target;

  if (pTarget != NULL)
    pTarget->pTargetOfProxy.append(this);
}

bool KLFSearchableProxy::searchFind(const QString& queryString, bool forward)
{
  KLF_ASSERT_NOT_NULL( pTarget, "Search target is NULL!", return false );
  pTarget->pQueryString = pQueryString;
  return pTarget->searchFind(queryString, forward);
}
bool KLFSearchableProxy::searchFindNext(bool forward)
{
  KLF_ASSERT_NOT_NULL( pTarget, "Search target is NULL!", return false );
  pTarget->pQueryString = pQueryString;
  return pTarget->searchFindNext(forward);
}
void KLFSearchableProxy::searchAbort()
{
  KLF_ASSERT_NOT_NULL( pTarget, "Search target is NULL!", return );
  pTarget->pQueryString = pQueryString;
  return pTarget->searchAbort();
}



// ------------------


KLFSearchBar::KLFSearchBar(QWidget *parent)
  : QFrame(parent), pTarget(NULL)
{
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

  pWaitLabel = new KLFWaitAnimationOverlay(u->txtSearch);
  pWaitLabel->setWaitMovie(":/pics/wait_anim.mng");

  setSearchTarget(NULL);
  slotSearchFocusOut();
}
KLFSearchBar::~KLFSearchBar()
{
  if (pTarget != NULL)
    pTarget->pTargetOf.removeAll(this);
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
  DECLARE_SEARCH_SHORTCUT(tr("Ctrl+R", "[[find rev]]"), parent, SLOT(findPrev()));
  // Esc will be captured through event filter so that it isn't too obstrusive...
}

void KLFSearchBar::setSearchTarget(KLFSearchable * object)
{
  KLF_DEBUG_BLOCK(KLF_FUNC_NAME) ;
  klfDbg("target="<<object) ;

  abortSearch();

  if (pTarget != NULL)
    pTarget->pTargetOf.removeAll(this);

  pTarget = object;

  if (pTarget != NULL)
    pTarget->pTargetOf.append(this);

  setEnabled(pTarget != NULL);
}


bool KLFSearchBar::eventFilter(QObject *obj, QEvent *ev)
{
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
      QKeyEvent *ke = (QKeyEvent*)ev;
      if (ke->key() == Qt::Key_Escape) {
	abortSearch();
	return true;
      }
    }
  }
  return QFrame::eventFilter(obj, ev);
}

void KLFSearchBar::clear()
{
  klfDbgT("clear") ;
  u->txtSearch->setText("");
  u->txtSearch->setFocus();
}

void KLFSearchBar::focusOrNext()
{
  if (QApplication::focusWidget() == u->txtSearch) {
    klfDbgT("already have focus") ;
    // already has focus
    // -> either recall history (if empty search text)
    // -> or find next
    if (u->txtSearch->text().isEmpty()) {
      u->txtSearch->setText(pLastSearchText);
    } else {
      if (pSearchText.isEmpty())
	find(u->txtSearch->text());
      else
	findNext();
    }
  } else {
    klfDbgT("setting focus") ;
    u->txtSearch->setText("");
    u->txtSearch->setFocus();
  }
}

void KLFSearchBar::find(const QString& text, bool forward)
{
  klfDbgT("text="<<text<<", forward="<<forward) ;
  if (text.isEmpty()) {
    abortSearch();
    return;
  }

  KLF_ASSERT_NOT_NULL( pTarget , "search target is NULL!", return ) ;

  pWaitLabel->startWait();
  pTarget->pQueryString = text;
  bool found = pTarget->searchFind(text, forward);
  updateSearchFound(found);
  pWaitLabel->stopWait();
  pSearchText = text;
  emitFoundSignals(found, pSearchText, forward);
}

void KLFSearchBar::findNext(bool forward)
{
  klfDbgT("forward="<<forward) ;

  // focus search bar if not yet focused.
  if (!searchBarHasFocus())
    u->txtSearch->setFocus();

  if (pSearchText.isEmpty()) {
    klfDbg("called but not in search mode. recalling history="<<pLastSearchText) ;
    // we're not in search mode
    // recall history
    showSearchBarText(pLastSearchText);

    // and initiate search mode
    find(u->txtSearch->text(), forward);
    return;
  }

  KLF_ASSERT_NOT_NULL( pTarget , "Search target is NULL!" , return ) ;

  pWaitLabel->startWait();
  pTarget->pQueryString = pSearchText;
  bool found = pTarget->searchFindNext(forward);
  updateSearchFound(found);
  pWaitLabel->stopWait();
  pLastSearchText = pSearchText;
  emitFoundSignals(found, pSearchText, forward);
}

void KLFSearchBar::abortSearch()
{
  KLF_DEBUG_BLOCK(KLF_FUNC_NAME) ;

  if (searchBarHasFocus())
    displayState(Aborted);
  else
    displayState(FocusOut);

  pSearchText = QString();
  if ( ! u->txtSearch->text().isEmpty() ) {
    showSearchBarText("");
  }

  if (pTarget != NULL) {
    klfDbg("telling target to abort search...") ;
    pTarget->searchAbort();
    pTarget->pQueryString = QString();
    klfDbg("...done") ;
  }

  emit searchAborted();
}

void KLFSearchBar::slotSearchFocusIn()
{
  klfDbgT("focus in") ;
  displayState(Default);
  showSearchBarText("");
}

void KLFSearchBar::slotSearchFocusOut()
{
  klfDbgT("focus out") ;
  displayState(FocusOut);
}

void KLFSearchBar::updateSearchFound(bool found)
{
  displayState(found ? Found : NotFound);
}

// private
QString KLFSearchBar::palettePropName(SearchState state)
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
  return QString(NULL);
}
// private
QString KLFSearchBar::statePropValue(SearchState state)
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

void KLFSearchBar::displayState(SearchState s)
{
  klfDbg("Setting state: "<<statePropValue(s));
  u->txtSearch->setProperty("searchState", statePropValue(s));
  QPalette pal = u->txtSearch->property(palettePropName(s).toAscii()).value<QPalette>();
  u->txtSearch->setStyleSheet(u->txtSearch->styleSheet());
  u->txtSearch->setPalette(pal);
  u->txtSearch->update();

  if (s == FocusOut) {
    showSearchBarText("  "+tr("Hit Ctrl-F, Ctrl-S or / to search within the current resource"));
  }
}

void KLFSearchBar::emitFoundSignals(bool resultfound, const QString& searchstring, bool forward)
{
  emit searchPerformed(resultfound);
  if (resultfound) {
    emit found();
    emit found(pSearchText, forward);
  } else {
    emit didNotFind();
    emit didNotFind(pSearchText, forward);
  }
}

void KLFSearchBar::showSearchBarText(const QString& text)
{
  u->txtSearch->blockSignals(true);
  u->txtSearch->setText(text);
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

  return QFrame::event(event);
}