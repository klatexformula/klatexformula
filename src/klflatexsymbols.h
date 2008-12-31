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

#include <QList>
#include <QString>
#include <QStringList>
#include <QEvent>
#include <QStackedWidget>
#include <QScrollArea>

#include <ui_klflatexsymbolsui.h>

class KLFMainWin;


class KLFLatexSymbolsCache; // see klflatexsymbols.cpp

struct KLFLatexSymbol {
  KLFLatexSymbol(const QString& symentry);
  KLFLatexSymbol(const QString& s, const QStringList& p) : symbol(s), preamble(p) { }

  QString symbol;
  QStringList preamble;

  //  QString symentry();
};

class KLFLatexSymbolsView : public QScrollArea
{
  Q_OBJECT
public:
  KLFLatexSymbolsView(const QString& category, QWidget *parent);

  void setSymbolList(const QList<KLFLatexSymbol>& symbols);

signals:
  void symbolActivated(const KLFLatexSymbol& symb);

public slots:
  void buildDisplay();
  void recalcLayout();

protected slots:
  void slotSymbolActivated();

protected:
  QString _category;
  QList<KLFLatexSymbol> _symbols;

private:
  QFrame *mFrame;
  QGridLayout *mLayout;
  QList<QWidget*> mSymbols;
};


class KLFLatexSymbols : public QWidget, private Ui::KLFLatexSymbolsUI
{
  Q_OBJECT
public:
  KLFLatexSymbols(KLFMainWin* parent);
  ~KLFLatexSymbols();

  static KLFLatexSymbolsCache *cache() { return mCache; }

signals:

  void insertSymbol(const KLFLatexSymbol& symb);
  void refreshSymbolBrowserShownState(bool);

public slots:

  void slotShowCategory(int cat);

protected:

  KLFMainWin *_mainwin;

  QStackedWidget *stkViews;

  static KLFLatexSymbolsCache *mCache;
  static int mCacheRefCounter;

  QList<KLFLatexSymbolsView *> mViews;

  void closeEvent(QCloseEvent *ev);
};

#endif

