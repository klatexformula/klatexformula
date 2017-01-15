/***************************************************************************
 *   file klflatexsymbols.h
 *   This file is part of the KLatexFormula Project.
 *   Copyright (C) 2011 by Philippe Faist
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
#include <QMetaObject>
#include <QMetaType>
#include <QHash>

#include <klfbackend.h>

#include <klfsearchbar.h>
#include <klfiteratorsearchable.h>


/** A Latex Symbol. */
struct KLF_EXPORT KLFLatexSymbol
{
  KLFLatexSymbol()
    : symbol(), symbol_numargs(0), symbol_option(false), previewlatex(), preamble(), textmode(false),
      hidden(true), keywords()  {  }
  KLFLatexSymbol(const QString& s, const QStringList& p, bool txtmod)
    : symbol(s), symbol_numargs(0), symbol_option(false), previewlatex(), preamble(p), textmode(txtmod),
      hidden(false), keywords()  {  }
  KLFLatexSymbol(const QDomElement& e);

  inline bool valid() const { return !symbol.isEmpty(); }

  QString symbol; //!< "<latex>" in XML def
  int symbol_numargs; //!< "numargs=###" in XML "<latex>" def
  bool symbol_option; //!< "option=[yes|no]" in XML "<latex>" def
  QString previewlatex; //!< "<preview-latex>" in XML def

  //! Returns the symbol with empty arguments, eg. "\sqrt[]{}". Ignores previewlatex.
  QString symbolWithArgs() const;
  //! Returns \ref previewlatex or \ref symbol with dummy args
  QString latexCodeForPreview() const;

  QStringList preamble; //!< all "<preambleline>"'s and "<usepackage>"
  bool textmode;
  struct BBOffset {
    BBOffset(int top = 0, int ri = 0, int bo = 0, int le = 0) : t(top), r(ri), b(bo), l(le) { }
    int t, r, b, l;
  } bbexpand;

  bool hidden;

  QStringList keywords; //!< "<keywords>"---add symbol search terms or alternative denomination
};

Q_DECLARE_METATYPE(KLFLatexSymbol) ;


KLF_EXPORT bool operator==(const KLFLatexSymbol& a, const KLFLatexSymbol& b);

KLF_EXPORT QDebug operator<<(QDebug str, const KLFLatexSymbol& symbol);


//! A Hash function for a KLFLatexSymbol
inline uint qHash(const KLFLatexSymbol& symb)
{
  return qHash(symb.symbol + symb.preamble.join("\n"));
}



struct KLFLatexSymbolsCachePrivate;

class KLF_EXPORT KLFLatexSymbolsCache : public QObject
{
  Q_OBJECT
public:
  KLFLatexSymbolsCache(QObject * parent);

  enum { Ok = 0, BadHeader, BadVersion };

  ~KLFLatexSymbolsCache();

  void setBackendSettings(const KLFBackend::klfSettings& settings);

  void setDevicePixelRatio(qreal ratio);

  bool contains(const KLFLatexSymbol & sym);

  KLFLatexSymbol findSymbol(const QString& symbolCode);
  QList<KLFLatexSymbol> findSymbols(const QString& symbolCode);
  QStringList symbolCodeList();

  QPixmap findSymbolPixmap(const QString& symbolCode);
  QPixmap getSymbolPixmap(const KLFLatexSymbol& sym);

  bool cacheNeedsSave() const;

public slots:

  void setSymbolPixmap(const KLFLatexSymbol& sym, const QPixmap& pixmap);
  void setSymbolPixmaps(const QHash<KLFLatexSymbol,QPixmap>& sympixmaps);

  void loadKlfCache();

  void saveCache(const QString & fname);
  void saveToKlfCache();

  void debugDump();

private:
  KLF_DECLARE_PRIVATE(KLFLatexSymbolsCache) ;

  int loadCacheStream(QDataStream& stream);
  int saveCacheStream(QDataStream& stream);

  /** returns -1 if file open read or the code returned by loadCache(),
   * that is KLFLatexSymbolsCache::{Ok,BadHeader,BadVersion} */
  int loadCacheFrom(const QString& file, int version);
};



class KLFLatexSymbolsSearchable;


class KLF_EXPORT KLFLatexSymbolsView : public QScrollArea
{
  Q_OBJECT
public:
  KLFLatexSymbolsView(const QString& category, QWidget *parent);

  void setSymbolList(const QList<KLFLatexSymbol>& symbols);
  void appendSymbolList(const QList<KLFLatexSymbol>& symbols);

  QString category() const { return _category; }

signals:
  void symbolActivated(const KLFLatexSymbol& symb);

public slots:
  void buildDisplay(KLFLatexSymbolsCache * cache);
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

  friend class KLFLatexSymbolsSearchable;

  bool symbolMatches(int symbol, const QString& qs);
  void highlightSearchMatches(int currentMatch, const QString& qs);
};


namespace Ui {
  class KLFLatexSymbols;
}



struct KLFLatexSymbolsPrivate;
class KLFLatexSymbolsView;

/** \brief Dialog that presents a selection of latex symbols to user
 */
class KLF_EXPORT KLFLatexSymbols : public QWidget
{
  Q_OBJECT
public:
  KLFLatexSymbols(QWidget* parent, const KLFBackend::klfSettings& baseSettings);
  ~KLFLatexSymbols();

  bool event(QEvent *event);

  void load();

  KLFLatexSymbolsCache * cache() ;

  QList<KLFLatexSymbolsView *> categoryViews();
  int currentCategoryIndex() const;

signals:

  void insertSymbol(const KLFLatexSymbol& symb);

public slots:

  void slotShowCategory(int cat);
  void retranslateUi(bool alsoBaseUi = true);
  void slotKlfConfigChanged();

  void emitInsertSymbol(const KLFLatexSymbol& symbol);

protected:

  void closeEvent(QCloseEvent *ev);
  void showEvent(QShowEvent *ev);

private:
  Ui::KLFLatexSymbols *u;

  KLF_DECLARE_PRIVATE(KLFLatexSymbols) ;
};




#endif

