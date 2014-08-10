/***************************************************************************
 *   file klfsparkleupdater.mm
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

#include <QWidget>

#if !defined(QT_MAC_USE_COCOA)
#error Sparkle framework can only be used with Qt/Cocoa.
#endif

#include <qmacdefines_mac.h>

#include <objc/runtime.h>
#include <Cocoa/Cocoa.h>
#include <Foundation/NSAutoreleasePool.h>

#include "klfsparkleupdater.h"

#include <SUUpdater.h>


// see: http://el-tramo.be/blog/mixing-cocoa-and-qt/

class KLFSparkleAutoUpdater::Private
{
public:

  SUUpdater* updater;

  NSAutoreleasePool *arpool;
};


KLFSparkleAutoUpdater::KLFSparkleAutoUpdater(QObject * parent, const QString& aUrl)
  : KLFAutoUpdater(parent)
{
  d = new Private;

  d->arpool = [[NSAutoreleasePool alloc] init];

  d->updater = [[SUUpdater sharedUpdater] retain];
  NSURL* url = [NSURL URLWithString:
      [NSString stringWithUTF8String: aUrl.toUtf8().data()]];
  [d->updater setFeedURL: url];
}


KLFSparkleAutoUpdater::~KLFSparkleAutoUpdater()
{
  [d->updater release];
  [d->arpool release];
  delete d;
}

void KLFSparkleAutoUpdater::checkForUpdates(bool inBackground)
{
  if (inBackground) {
    [d->updater checkForUpdatesInBackground];
  } else {
    [d->updater checkForUpdates:nil];
  }
}

