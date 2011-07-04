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
  : QObject(parent)
{
  parent->installEventFilter(this);
}

KLFRelativeFontBase::~KLFRelativeFontBase()
{
}

bool KLFRelativeFontBase::eventFilter(QObject *object, QEvent *event)
{
  QWidget *w = parentWidget();
  if (object == w) {
    if (event->type() == QEvent::FontChange || event->type() == QEvent::ApplicationFontChange) {
      w->setFont(calculateRelativeFont(w->font()));
    }
  }
  return false; // never eat an event
}


// -----------


KLFRelativeFont::KLFRelativeFont(QWidget *parent)
  : KLFRelativeFontBase(parent)
{
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

QFont KLFRelativeFont::calculateRelativeFont(const QFont& baseFont)
{
  QFont f = baseFont;
  if (!pForceFamily.isEmpty())
    f.setFamily(pForceFamily);
  if (pRelPointSize != 0)
    f.setPointSize(QFontInfo(f).pointSize() + pRelPointSize);

  return f;
}

