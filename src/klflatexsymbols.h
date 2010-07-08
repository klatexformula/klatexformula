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
/* $Id$ */

#ifndef KLFLATEXSYMBOLS_H
#define KLFLATEXSYMBOLS_H

#include <QList>
#include <QString>
#include <QStringList>
#include <QEvent>
#include <QStackedWidget>
#include <QLayout>
#include <QGridLayout>
#include <QSpacerItem>
#include <QPushButton>
#include <QScrollArea>
#include <QDomElement>

#include <klfbackend.h>

/** A Latex Symbol. */
struct KLF_EXPORT KLFLatexSymbol
{
  KLFLatexSymbol() : symbol(), preamble(), textmode(false), hidden(true) { }
  KLFLatexSymbol(const QString& s, const QStringList& p, bool txtmod)
    : symbol(s), preamble(p), textmode(txtmod), hidden(false) { }
  KLFLatexSymbol(const QDomElement& e);

  inline bool valid() const { return !symbol.isEmpty(); }

  QString symbol;
  QStringList preamble;
  bool textmode;
  struct BBOffset {
    BBOffset(int top = 0, int ri = 0, int bo = 0, int le = 0) : t(top), r(ri), b(bo), l(le) { }
    int t, r, b, l;
  } bbexpand;

  bool hidden;
};



class KLFLatexSymbolsCache
{
public:
  enum { Ok = 0, BadHeader, BadVersion };

  inline bool cacheNeedsSave() const { return flag_modified; }

  QPixmap getPixmap(const KLFLatexSymbol& sym, bool fromcacheonly = true);

  int precacheList(const QList<KLFLatexSymbol>& list, bool userfeedback, QWidget *parent = NULL);

  void setBackendSettings(const KLFBackend::klfSettings& settings);

  KLFLatexSymbol findSymbol(const QString& symbolCode);
  QStringList symbolCodeList();

  static KLFLatexSymbolsCache * theCache();
  static void saveTheCache();

private:
  /** Private constructor */
  KLFLatexSymbolsCache();
  /** No copy constructor */
  KLFLatexSymbolsCache(const KLFLatexSymbolsCache& /*other*/) { }

  QMap<KLFLatexSymbol,QPixmap> cache;
  bool flag_modified;
  KLFBackend::klfSettings backendsettings;

  int loadCacheStream(QDataStream& stream);
  int saveCacheStream(QDataStream& stream);

  static KLFLatexSymbolsCache * staticCache;

  /** returns -1 if file open read or the code returned by loadCache(),
   * that is KLFLatexSymbolsCache::{Ok,BadHeader,BadVersion} */
  int loadCacheFrom(const QString& file, int version);
};



class KLF_EXPORT KLFLatexSymbolsView : public QScrollArea
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
  QWidget *mFrame;
  QGridLayout *mLayout;
  QSpacerItem *mSpacerItem;
  QList<QWidget*> mSymbols;
};


namespace Ui {
  class KLFLatexSymbols;
}


/** \brief Dialog that presents a selection of latex symbols to user
 */
class KLF_EXPORT KLFLatexSymbols : public QWidget
{
  Q_OBJECT
public:
  KLFLatexSymbols(QWidget* parent, const KLFBackend::klfSettings& baseSettings);
  ~KLFLatexSymbols();

signals:

  void insertSymbol(const KLFLatexSymbol& symb);
  void refreshSymbolBrowserShownState(bool);

public slots:

  void slotShowCategory(int cat);

  void retranslateUi(bool alsoBaseUi = true);

protected:
  QStackedWidget *stkViews;

  QList<KLFLatexSymbolsView *> mViews;

  void closeEvent(QCloseEvent *ev);
  void showEvent(QShowEvent *ev);

private:
  Ui::KLFLatexSymbols *u;
};




#endif

