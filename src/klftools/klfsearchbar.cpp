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
  return pTarget->searchFind(queryString, forward);
}
bool KLFSearchableProxy::searchFindNext(bool forward)
{
  KLF_ASSERT_NOT_NULL( pTarget, "Search target is NULL!", return false );
  return pTarget->searchFindNext(forward);
}
void KLFSearchableProxy::searchAbort()
{
  KLF_ASSERT_NOT_NULL( pTarget, "Search target is NULL!", return );
  return pTarget->searchAbort();
}


// ------------------------

KLFSearchBar::KLFSearchBar(QWidget *parent)
  : QFrame(parent), pTarget(NULL)
{
  KLF_DEBUG_TIME_BLOCK(KLF_FUNC_NAME) ;
  klfDbg("parent: "<<parent) ;

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

  pWaitLabel = new KLFWaitAnimationOverlay(u->txtSearch);
  pWaitLabel->setWaitMovie(":/pics/wait_anim.mng");
  /* // amusing test
     pWaitLabel->setWaitMovie("/home/philippe/projects/klf/artwork/experimental/packman_anim.gif");
  */

  pShowOverlayMode = false;
  // default relative geometry: position at (50%, 95%) (centered, quasi-bottom)
  //                            size     of (90%, 0%)   [remember: expanded to minimum size]
  pShowOverlayRelativeGeometry = QRect(QPoint(50, 95), QSize(90, 0));

  pFocusOutText = "  "+tr("Hit Ctrl-F, Ctrl-S or / to start searching");

  pSearchForward = true;

  setSearchTarget(NULL);
  slotSearchFocusOut();
}
KLFSearchBar::~KLFSearchBar()
{
  if (pTarget != NULL)
    pTarget->pTargetOf.removeAll(this);
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

void KLFSearchBar::setSearchText(const QString& text)
{
  u->lblSearch->setText(text);
}

void KLFSearchBar::setFocusOutText(const QString& focusOutText)
{
  pFocusOutText = focusOutText;
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
      abortSearch();
      // don't eat event
    } else if (ev->type() == QEvent::KeyPress) {
      QKeyEvent *ke = (QKeyEvent*)ev;
      if (ke->key() == Qt::Key_Escape) {
	abortSearch();
	emit escapePressed();
	return true;
      }
    }
  }
  return QFrame::eventFilter(obj, ev);
}

void KLFSearchBar::setShowOverlayMode(bool overlayMode)
{
  klfDbg("setting show overlay mode to "<<overlayMode) ;
  pShowOverlayMode = overlayMode;
  if (pShowOverlayMode && !searchBarHasFocus())
    hide();
  setProperty("klfShowOverlayMode", QVariant::fromValue<bool>(pShowOverlayMode));
  // cheat with klfTopLevelWidget property, set it always in show-overlay-mode
  setProperty("klfTopLevelWidget", QVariant::fromValue<bool>(pShowOverlayMode));

  /** \bug ..... the search bar should install an event filter on the parent to listen
   * for resize events, and to resize appropriately. */
}

void KLFSearchBar::setShowOverlayRelativeGeometry(const QRect& relativeGeometryPercent)
{
  pShowOverlayRelativeGeometry = relativeGeometryPercent;
}
void KLFSearchBar::setShowOverlayRelativeGeometry(int widthPercent, int heightPercent,
						  int positionXPercent, int positionYPercent)
{
  setShowOverlayRelativeGeometry(QRect(QPoint(positionXPercent, positionYPercent),
				       QSize(widthPercent, heightPercent)));
}



void KLFSearchBar::clear()
{
  klfDbgT("clear") ;
  u->txtSearch->setText("");
  focus();
}

void KLFSearchBar::focusOrNext(bool forward)
{
  pSearchForward = forward;

  if (QApplication::focusWidget() == u->txtSearch) {
    klfDbgT("already have focus") ;
    // already has focus
    // -> either recall history (if empty search text)
    // -> or find next
    if (u->txtSearch->text().isEmpty()) {
      u->txtSearch->setText(pLastSearchText);
    } else {
      if (pSearchText.isEmpty())
	find(u->txtSearch->text(), forward);
      else
	findNext(forward);
    }
  } else {
    klfDbgT("setting focus") ;
    u->txtSearch->setText("");
    focus();
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
    focus();

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
  bool found = pTarget->searchFindNext(forward);
  updateSearchFound(found);
  pWaitLabel->stopWait();
  pLastSearchText = pSearchText;
  emitFoundSignals(found, pSearchText, forward);
}

void KLFSearchBar::abortSearch()
{
  KLF_DEBUG_BLOCK(KLF_FUNC_NAME) ;

  pSearchText = QString();
  if ( ! u->txtSearch->text().isEmpty() ) {
    showSearchBarText("");
  }

  if (searchBarHasFocus())
    displayState(Aborted);
  else
    displayState(FocusOut);

  if (pTarget != NULL) {
    klfDbg("telling target to abort search...") ;
    pTarget->searchAbort();
    klfDbg("...done") ;
  }

  emit searchAborted();
}

void KLFSearchBar::focus()
{
  KLF_DEBUG_BLOCK(KLF_FUNC_NAME) ;

  if (pShowOverlayMode) {
    QWidget *pw = parentWidget();
    if (pw != NULL) {
      // if we have a parent widget, adjust using our relative geometry
      QSize pws = pw->size();
      
      QPoint relPos = pShowOverlayRelativeGeometry.topLeft();
      QSize relSz = pShowOverlayRelativeGeometry.size();
      
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
  displayState(Default);
  showSearchBarText("");
}

void KLFSearchBar::slotSearchFocusOut()
{
  klfDbgT("focus out") ;
  displayState(FocusOut);
  if (pShowOverlayMode)
    hide();
}

void KLFSearchBar::updateSearchFound(bool found)
{
  displayState(found ? Found : NotFound);
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
  return QString(NULL);
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

void KLFSearchBar::displayState(SearchState s)
{
  klfDbg("Setting state: "<<statePropValue(s));
  u->txtSearch->setProperty("searchState", statePropValue(s));
  QPalette pal = u->txtSearch->property(palettePropName(s).toAscii()).value<QPalette>();
  u->txtSearch->setStyleSheet(u->txtSearch->styleSheet());
  u->txtSearch->setPalette(pal);
  u->txtSearch->update();

  if (s == FocusOut) {
    showSearchBarText(pFocusOutText);
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
