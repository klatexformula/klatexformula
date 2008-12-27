/***************************************************************************
 *   file klflatexsymbols.cpp
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

#include <QFile>
#include <QTextStream>
#include <QScrollArea>
#include <QList>
#include <QStringList>
#include <QProgressDialog>
#include <QGridLayout>
#include <QPushButton>
#include <QMap>
#include <QStackedWidget>
#include <QLineEdit>
#include <QMessageBox>
#include <QApplication>
#include <QDebug>
#include <QCloseEvent>

#include <klfbackend.h>

#include "klfmainwin.h"
#include "klflatexsymbols.h"


extern int version_maj, version_min;




KLFLatexSymbol::KLFLatexSymbol(const QString& symentry)
{
  // parse symentry
  // it should be in the form:              *EXTRAPREAMBLE*\latexsymbol
  // or, if no extra preamble is needed:    \latexsymbol
  // if a literal '*' is needed at the beginning, start with a space.
  // EXTRAPREAMBLE should contain a list of Latex commands that should be inserted into preamble
  // if they are not there yet; separate commands with '%%'

  int ii;
  QString xtra = "";
  if (symentry[0] == '*' && (ii=symentry.indexOf('*', 1)) != -1) {
    symbol = symentry.mid(ii+1);
    xtra = symentry.mid(1, ii-1);
  } else {
    symbol = symentry;
  }

  preamble = xtra.split("%%");
}




class KLFLatexSymbolsCache
{
public:

  enum { Ok = 0, BadHeader, TooRecentVersion };

  KLFLatexSymbolsCache()
  {
  }

  int loadCache(QDataStream& stream)
  {
    QString magic;
    stream >> magic;
    if (magic != "KLATEXFORMULA_SYMBOLS_PIXMAP_CACHE") {
      return BadHeader;
    }
    qint16 vmaj, vmin;
    stream >> vmaj >> vmin;

    if (vmaj > version_maj || (vmaj == version_maj && vmin > version_min)) {
      return TooRecentVersion;
    }
    if (vmaj <= 2) { // KLF 2.x with Qt3
      // Qt3-compatible stream input
      stream.setVersion(QDataStream::Qt_3_3);
    } else {
      // KLF 3.x, we saved the stream version, read it now
      qint16 version;
      stream >> version;
      stream.setVersion(version);
    }

    stream >> cache;
    return 0;
  }

  int saveCache(QDataStream& stream)
  {
    // artificially set a lower version, so as to not handicap anyone who would like to read us with an old version of KLF
    stream.setVersion(QDataStream::Qt_3_3);
    stream << QString("KLATEXFORMULA_SYMBOLS_PIXMAP_CACHE") << (qint16)version_maj << (qint16)version_min
	   << stream.version() << cache;
    return 0;
  }

  QPixmap getPixmap(const KLFLatexSymbol& sym, bool fromcacheonly = true)
  {
    if (cache.contains(sym.symbol))
      return cache[sym.symbol];

    if (fromcacheonly) // if we weren't able to load it from cache, show failed icon
      return QPixmap(":/pics/badsym.png");

    KLFBackend::klfInput in;
    in.latex = sym.symbol;
    in.mathmode = "\\[ ... \\]";
    in.preamble = sym.preamble.join("\n")+"\n";
    in.fg_color = qRgb(0,0,0);
    in.bg_color = qRgba(255,255,255,0); // transparent Bg
    in.dpi = 150;

    backendsettings.epstopdfexec = ""; // don't waste time making PDF, we don't need it
    backendsettings.tborderoffset = backendsettings.tborderoffset >= 1 ? backendsettings.tborderoffset : 1;
    backendsettings.rborderoffset = backendsettings.rborderoffset >= 3 ? backendsettings.rborderoffset : 3;
    backendsettings.bborderoffset = backendsettings.bborderoffset >= 1 ? backendsettings.bborderoffset : 1;
    backendsettings.lborderoffset = backendsettings.lborderoffset >= 1 ? backendsettings.lborderoffset : 1;

    KLFBackend::klfOutput out = KLFBackend::getLatexFormula(in, backendsettings);

    if (out.status != 0) {
      qCritical() << QObject::tr("ERROR: Can't generate preview for symbol %1 : status %2 !\n\tError: %3\n")
	.arg(sym.symbol).arg(out.status).arg(out.errorstr);
      return QPixmap(":/pics/badsym.png");
    }

    QPixmap pix = QPixmap::fromImage(out.result);
    cache[sym.symbol] = pix;

    return pix;
  }

  int precacheList(const QList<KLFLatexSymbol>& list, bool userfeedback, QWidget *parent = 0)
  {
    QProgressDialog *pdlg;

    if (userfeedback) {
      pdlg = new QProgressDialog(QObject::tr("Please wait while generating symbol previews ... "), QObject::tr("Cancel"),
				 0, list.size()-1, parent);
      pdlg->setValue(0);
    }

    for (int i = 0; i < list.size(); ++i) {
      if (userfeedback) {
	if (pdlg->wasCanceled()) {
	  delete pdlg;
	  return 1;
	}
	pdlg->setValue(i);
      }
      getPixmap(list[i], false);
    }

    if (userfeedback) {
      delete pdlg;
    }

    return 0;
  };


  KLFBackend::klfSettings backendsettings;

  QMap<QString,QPixmap> cache;

};



KLFLatexSymbolsCache *KLFLatexSymbols::mCache = 0;
int KLFLatexSymbols::mCacheRefCounter = 0;







KLFLatexSymbolsView::KLFLatexSymbolsView(const QString& category, QWidget *parent)
  : QScrollArea(parent), _category(category)
{
  mFrame = new QFrame(this);
  mLayout = new QGridLayout(mFrame);

  setWidget(mFrame);
}

void KLFLatexSymbolsView::setSymbolList(const QList<KLFLatexSymbol>& symbols)
{
  _symbols = symbols;
}

void KLFLatexSymbolsView::buildDisplay()
{
  int i;
  for (i = 0; i < _symbols.size(); ++i) {
    QPushButton *btn = new QPushButton(mFrame);
    QPixmap p = KLFLatexSymbols::cache()->getPixmap(_symbols[i]);
    btn->setIconSize(p.size());
    btn->setIcon(p);
    btn->setProperty("symbol", (unsigned int) i);
    connect(btn, SIGNAL(clicked()), this, SLOT(slotSymbolActivated()));
    mSymbols.append(btn);
  }

  recalcLayout();
}

void KLFLatexSymbolsView::recalcLayout()
{
  int n = 1 + mFrame->width() / 40;
  int i;
  for (i = 0; i < mSymbols.size(); ++i) {
    mLayout->removeWidget(mSymbols[i]);
    mLayout->addWidget(mSymbols[i], i/n, i%n);
  }
}


void KLFLatexSymbolsView::slotSymbolActivated()
{
  QObject *s = sender();
  unsigned int i = s->property("symbol").toUInt();

  emit symbolActivated(_symbols[i]);
}




// class KLFLatexSymbolsViewItem : public QIconViewItem
// {
// public:

//   enum { RTTI = 1988 };

//   KLFLatexSymbolsViewItem(QIconView *view, QString ltx, QPixmap pix)
//     : QIconViewItem(view, "", pix), _latexsym(KLFLatexSymbols::latexsym(ltx)),
//       _xtrapreamble(KLFLatexSymbols::xtrapreamble(ltx))
//   {
//   }

//   virtual int rtti() const { return RTTI; }

//   QString latexsym() const { return _latexsym; }
//   QString xtrapreamble() const { return _xtrapreamble; }

// protected:
//   QString _latexsym;
//   QString _xtrapreamble;
// };













KLFLatexSymbols::KLFLatexSymbols(KLFMainWin *mw)
  : QDialog(mw), Ui::KLFLatexSymbolsUI()
{
  setupUi(this);

  _mainwin = mw;

  if (mCache == 0) {
    mCache = new KLFLatexSymbolsCache;
    mCache->backendsettings = mw->backendSettings();

    QString s = klfconfig.homeConfigDir + "/symbolspixmapcache";
    if ( ! QFile::exists(s) ) {
      s = ":/data/symbolspixmapcache_base";
    }
    
    {
      QFile f(s);
      f.open(QIODevice::ReadOnly);
      QDataStream ds(&f);
      int r = mCache->loadCache(ds);
      if (r != 0)
	qCritical() << tr("Warning: KLFLatexSymbols: error reading cache file ! code=%1\n").arg(r);
      f.close();
    }
    // and save the loaded/generated cache to file immediately...
    QFile fw(s);
    fw.open(QIODevice::WriteOnly);
    QDataStream dsw(&fw);
    mCache->saveCache(dsw);
  }
  mCacheRefCounter++;

  QList<KLFLatexSymbol> allsymbols;
  // create our UI
  cbxCategory->clear();
  QGridLayout *lytstk = new QGridLayout(frmStackContainer);
  stkViews = new QStackedWidget(frmStackContainer);
  lytstk->addWidget(stkViews, 0, 0);

  mViews.clear();

  // read our config
  {
    QString fn = klfconfig.homeConfigDir + "/latex_symbols";
    if ( ! QFile::exists(fn) ) {
      fn = ":/data/latex_symbols";
    }
    QFile f(fn);
    if ( ! f.open(QIODevice::ReadOnly) ) {
      qCritical() << tr("Can't open file `%1' !!").arg(fn);
    }

    // read all symbols
    QTextStream stream(&f);
    QString heading;
    QList<KLFLatexSymbol> l;
    QString s;

    do {
      l.clear();
      heading = stream.readLine();

      while ( ! heading.isEmpty() && ! (s = stream.readLine()).isEmpty() ) {
	KLFLatexSymbol sym(s);
	l.append(sym);
	allsymbols.append(sym);
      }
      if ( ! heading.isEmpty() ) {
	KLFLatexSymbolsView *view = new KLFLatexSymbolsView(heading, stkViews);
	view->setSymbolList(l);
	mViews.append(view);
	stkViews->addWidget(view);
	cbxCategory->addItem(heading);
      }
    } while ( ! heading.isNull() ) ;
  }

  // pre-cache all our symbols
  mCache->precacheList(allsymbols, true, this);

  int i;
  for (i = 0; i < mViews.size(); ++i) {
    mViews[i]->buildDisplay();
  }

  slotShowCategory(0);

  connect(cbxCategory, SIGNAL(highlighted(int)), this, SLOT(slotShowCategory(int)));
  connect(btnClose, SIGNAL(clicked()), this, SLOT(slotClose()));
}

KLFLatexSymbols::~KLFLatexSymbols()
{
  --mCacheRefCounter;
  if (mCacheRefCounter <= 0) {
    QString s = klfconfig.homeConfigDir + "/symbolspixmapcache";

    QFile f(s);
    f.open(QIODevice::WriteOnly);
    QDataStream ds(&f);
    mCache->saveCache(ds);

    delete mCache;
    mCache = 0;
  }
}



void KLFLatexSymbols::reject()
{
  // reimplemeneted to stop ESC from brutally closing the dialog.
  // see comment in same function in klfhistorybrowser.cpp for details.
}

// void KLFLatexSymbols::slotNeedsInsert(QIconViewItem *item)
// {
//   if (!item)
//     return;
//   if (item->rtti() != KLFLatexSymbolsViewItem::RTTI)
//     return;
//   KLFLatexSymbolsViewItem *klfitem = (KLFLatexSymbolsViewItem *) item;

//   emit insertSymbol(klfitem->latexsym(), klfitem->xtrapreamble());
// }


void KLFLatexSymbols::slotShowCategory(int c)
{
  // called by combobox
  stkViews->setCurrentIndex(c);
}

// void KLFLatexSymbols::slotDisplayItem(QIconViewItem *item)
// {
//   if (!item)
//     return;
//   if (item->rtti() != KLFLatexSymbolsViewItem::RTTI)
//     return;

//   KLFLatexSymbolsViewItem *klfitem = (KLFLatexSymbolsViewItem *) item;

//   lneDisplay->setText(klfitem->latexsym());
//   lneDisplayXtrapreamble->setText(klfitem->xtrapreamble());
// }


void KLFLatexSymbols::slotClose()
{
  hide();
  emit refreshSymbolBrowserShownState(false);
}

void KLFLatexSymbols::closeEvent(QCloseEvent *e)
{
  e->accept();
  emit refreshSymbolBrowserShownState(false);
}





