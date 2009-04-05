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
#include <QDomElement>

#include <ui_klflatexsymbolsui.h>

class KLFMainWin;


class KLFLatexSymbolsCache; // see klflatexsymbols.cpp

struct KLFLatexSymbol {
  KLFLatexSymbol() : symbol(), preamble(), textmode(false) { }
  KLFLatexSymbol(const QString& s, const QStringList& p, bool txtmod) : symbol(s), preamble(p), textmode(txtmod) { }
  KLFLatexSymbol(const QString& symentry);
  KLFLatexSymbol(const QDomElement& e);

  QString symbol;
  QStringList preamble;
  bool textmode;
  struct BBOffset {
    BBOffset(int top = 0, int ri = 0, int bo = 0, int le = 0) : t(top), r(ri), b(bo), l(le) { }
    int t, r, b, l;
  } bbexpand;

  //  QString symentry();
};




class KLFPixmapButton : public QPushButton
{
  Q_OBJECT
public:
  Q_PROPERTY(QPixmap pixmap READ pixmap WRITE setPixmap USER true)
  Q_PROPERTY(int pixmapMargin READ pixmapMargin WRITE setPixmapMargin)
  Q_PROPERTY(float pixXAlignFactor READ pixXAlignFactor WRITE setPixXAlignFactor)
  Q_PROPERTY(float pixYAlignFactor READ pixYAlignFactor WRITE setPixYAlignFactor)

  KLFPixmapButton(const QPixmap& pix, QWidget *parent = 0);
  virtual ~KLFPixmapButton() { }

  virtual QSize minimumSizeHint() const;
  virtual QSize sizeHint() const;

  virtual QPixmap pixmap() const { return _pix; }
  virtual int pixmapMargin() const { return _pixmargin; }
  virtual float pixXAlignFactor() const { return _xalignfactor; }
  virtual float pixYAlignFactor() const { return _yalignfactor; }

public slots:
  virtual void setPixmap(const QPixmap& pix) { _pix = pix; }
  virtual void setPixmapMargin(int pixels) { _pixmargin = pixels; }
  virtual void setPixXAlignFactor(float xalignfactor) { _xalignfactor = xalignfactor; }
  virtual void setPixYAlignFactor(float yalignfactor) { _yalignfactor = yalignfactor; }

protected:
  virtual void paintEvent(QPaintEvent *event);

private:
  QPixmap _pix;
  int _pixmargin;
  float _xalignfactor, _yalignfactor;
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
  QSpacerItem *mSpacerItem;
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
  void showEvent(QShowEvent *ev);
};

#endif

