/***************************************************************************
 *   file klfrelativefont.cpp
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

#include <QEvent>

#include "klfrelativefont.h"


KLFRelativeFontBase::KLFRelativeFontBase(QWidget *parent)
  : QObject(parent), pReference(parent), pTarget(parent), pInhibitFontChangeRecursion(false), pHasAppliedFont(false)
{
  klfDbg("constructor. Parent="<<parent) ;
  parent->installEventFilter(this);
}

KLFRelativeFontBase::KLFRelativeFontBase(QWidget *ref, QWidget *target)
  : QObject(target), pReference(ref), pTarget(target), pInhibitFontChangeRecursion(false), pHasAppliedFont(false)
{
  klfDbg("constructor. Ref="<<ref<<", tgt="<<target) ;
  ref->installEventFilter(this);
  if (ref != target)
    target->installEventFilter(this);
}

KLFRelativeFontBase::~KLFRelativeFontBase()
{
}

bool KLFRelativeFontBase::eventFilter(QObject *object, QEvent *event)
{
  KLF_DEBUG_BLOCK(KLF_FUNC_NAME) ;

  QWidget *ref = referenceWidget();
  QWidget *target = targetWidget();

  klfDbg("eventFilter("<<object<<",ev.type="<<event->type()<<"), ref="<<ref<<", tgt="<<target) ;

  //  // Don't care about ApplicationFontChange, a FontChange event is generated anyway.
  //  if (event->type() == QEvent::ApplicationFontChange) {
  //    klfDbg("FYI: application font change!!") ;
  //    return false; // a FontChange event is generated anyway
  //  }

  if (object == ref) {
    if (event->type() == QEvent::FontChange) {
      klfDbg("event filter, font change event! object="<<object<<", event/type="<<event->type()
	     <<", refWidget="<<ref<<", targetWidget="<<target) ;
      if (pInhibitFontChangeRecursion) {
	klfDbg("inhibited `font change' event recursion.");
	pInhibitFontChangeRecursion = false;
	return false;
      }

      calculateAndApplyNewFont();
      return false;
    }
  }
  if (object == target) {
    if (event->type() == QEvent::Show) {
      if (!pHasAppliedFont)
	calculateAndApplyNewFont();
      return false;
    }
  }
  return false; // never eat an event
}

void KLFRelativeFontBase::calculateAndApplyNewFont()
{
  QWidget *ref = referenceWidget();
  QWidget *target = targetWidget();
  QFont f = calculateRelativeFont(ref->font());
  klfDbg("Applying font "<<f<<" calculated from base font "<<ref->font()) ;
  if (ref == target) {
    // Set this flag to `true' so that the generated font change event is not picked up.
    pInhibitFontChangeRecursion = true;
  }
  target->setFont(f);

  pHasAppliedFont = true;
}



// -----------


KLFRelativeFont::KLFRelativeFont(QWidget *parent)
  : KLFRelativeFontBase(parent)
{
  rfinit();
}
KLFRelativeFont::KLFRelativeFont(QWidget *ref, QWidget *t)
  : KLFRelativeFontBase(ref, t)
{
  rfinit();
}

void KLFRelativeFont::rfinit()
{
  KLF_DEBUG_BLOCK(KLF_FUNC_NAME) ;
  pRelPointSize = 0;
  klfDbg("...");
  pForceFamily = QString();
  klfDbg("...");
  pForceWeight = -1;
  klfDbg("...");
  pForceStyle = -1;
}

KLFRelativeFont::~KLFRelativeFont()
{
}

void KLFRelativeFont::setRelPointSize(int relps)
{
  pRelPointSize = relps;
}
void KLFRelativeFont::setForceFamily(const QString& family)
{
  pForceFamily = family;
}
void KLFRelativeFont::setForceWeight(int w)
{
  pForceWeight = w;
}
void KLFRelativeFont::setForceStyle(int style)
{
  pForceStyle = style;
}

QFont KLFRelativeFont::calculateRelativeFont(const QFont& baseFont)
{
  KLF_DEBUG_BLOCK(KLF_FUNC_NAME) ;

  QFont f = baseFont;
  if (!pForceFamily.isEmpty())
    f.setFamily(pForceFamily);
  if (pForceWeight >= 0)
    f.setWeight(pForceWeight);
  if (pForceStyle >= 0)
    f.setStyle((QFont::Style)pForceStyle);
  if (pRelPointSize != 0)
    f.setPointSize(QFontInfo(f).pointSize() + pRelPointSize);

  return f;
}

