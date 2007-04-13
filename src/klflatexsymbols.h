/***************************************************************************
 *   file klflatexsymbols.h
 *   This file is part of the KLatexFormula Project.
 *   Copyright (C) 2007 by Philippe Faist
 *   philippe.faist@bluewin.ch
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

#ifndef KLFLATEXSYMBOLS_H
#define KLFLATEXSYMBOLS_H

#include <qvaluelist.h>
#include <qpair.h>
#include <qstring.h>
#include <qstringlist.h>
#include <qevent.h>
#include <qiconview.h>
#include <qwidgetstack.h>

#include <klflatexsymbolsui.h>

class KLFMainWin;


class KLFLatexSymbolsCache; // see klflatexsymbols.cpp

class KLFLatexSymbols : public KLFLatexSymbolsUI
{
  Q_OBJECT
public:
  KLFLatexSymbols(KLFMainWin* parent);
  ~KLFLatexSymbols();

signals:

  void insertSymbol(QString symb);
  void refreshSymbolBrowserShownState(bool);

public slots:

  void slotClose();

protected slots:

  void slotNeedsInsert(QIconViewItem *item);
  void slotDisplayItem(QIconViewItem *item);
  void slotInsertCurrentDisplay();
  void slotShowCategory(int cat);

protected:

  void reject();

  KLFMainWin *_mainwin;

  QWidgetStack *stkViews;

  static KLFLatexSymbolsCache *mCache;
  static int mCacheRefCounter;

  QValueList<QPair<QString,QStringList> > _symbols;

  QValueList<QIconView *> mViews;

  void closeEvent(QCloseEvent *ev);

};

#endif

