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

#include <qfile.h>
#include <qtextstream.h>
#include <qiconview.h>
#include <qtabwidget.h>
#include <qvaluelist.h>
#include <qstringlist.h>
#include <qprogressdialog.h>
#include <qlayout.h>
#include <qpushbutton.h>

#include <kmessagebox.h>
#include <kstandarddirs.h>
#include <klocale.h>
#include <kapplication.h>

#include <klfbackend.h>

#include "klfmainwin.h"
#include "klflatexsymbols.h"


extern int version_maj, version_min;




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
    Q_INT16 vmaj, vmin;
    stream >> vmaj >> vmin;

    if (vmaj > version_maj || (vmaj == version_maj && vmin > version_min)) {
      return TooRecentVersion;
    }

    stream >> cache;
    return 0;
  }

  int saveCache(QDataStream& stream)
  {
    stream << QString("KLATEXFORMULA_SYMBOLS_PIXMAP_CACHE") << (Q_INT16)version_maj << (Q_INT16)version_min << cache;
    return 0;
  }

  QPixmap getPixmap(const QString& latex)
  {
    if (cache.contains(latex))
      return cache[latex];

    KLFBackend::klfInput in;
    in.latex = latex;
    in.mathmode = "\\[ ... \\]";
    in.preamble = "\\usepackage{amssymb,amsmath}";
    in.fg_color = qRgb(0,0,0);
    in.bg_color = qRgba(255,255,255,0); // transparent Bg
    in.dpi = 150;

    backendsettings.epstopdfexec = ""; // don't waste time making PDF, we don't need it

    KLFBackend::klfOutput out = KLFBackend::getLatexFormula(in, backendsettings);

    if (out.status != 0) {
      fprintf(stderr, "ERROR: Can't generate preview for symbol %s !\n\tError: %s\n", latex.latin1(), out.errorstr.latin1());
      return QPixmap();
    }

    cache[latex] = out.result;

    return out.result;
  }

  int precacheList(const QStringList& list, bool userfeedback, QWidget *parent = 0)
  {
    QProgressDialog *pdlg;

    if (userfeedback) {
      pdlg = new QProgressDialog(i18n("Please wait while generating symbol previews ... "), i18n("Cancel"),
				 list.size(), parent, 0, true/*modal*/);
      pdlg->setProgress(0);
    }

    for (uint i = 0; i < list.size(); ++i) {
      getPixmap(list[i]);
      if (userfeedback) {
	if (pdlg->wasCanceled()) {
	  delete pdlg;
	  return 1;
	}

	pdlg->setProgress(i);
	kapp->processEvents();
      }
    }

    if (userfeedback) {
      pdlg->setProgress(list.size());
      delete pdlg;
    }

    return 0;
  };


  KLFBackend::klfSettings backendsettings;

  QMap<QString,QPixmap> cache;

};







class KLFLatexSymbolsViewItem : public QIconViewItem
{
public:

  enum { RTTI = 1988 };

  KLFLatexSymbolsViewItem(QIconView *view, QString ltx, QPixmap pix)
    : QIconViewItem(view, "", pix), _latex(ltx)
  {
  }

  virtual int rtti() const { return RTTI; }

  QString latex() const { return _latex; }

protected:
  QString _latex;
};








KLFLatexSymbolsCache *KLFLatexSymbols::mCache = 0;
int KLFLatexSymbols::mCacheRefCounter = 0;








KLFLatexSymbols::KLFLatexSymbols(KLFMainWin *mw)
  : KLFLatexSymbolsUI(0, 0, false, 0) // No parent here
{
  _mainwin = mw;

  if (mCache == 0) {
    mCache = new KLFLatexSymbolsCache;
    mCache->backendsettings = mw->backendSettings();

    QString s = locate("appdata", "symbolspixmapcache");
    if (!s.isEmpty()) {
      QFile f(s);
      f.open(IO_ReadOnly);
      QDataStream ds(&f);
      int r = mCache->loadCache(ds);
      if (r != 0)
	fprintf(stderr, "Warning: KLFLatexSymbols: error reading cache file ! code=%d\n", r);
    }
  }
  mCacheRefCounter++;

  // read our config
  {
    QFile f(locate("appdata", "latex_symbols"));
    if ( ! f.open(IO_ReadOnly) ) {
      KMessageBox::error(this, i18n("Error reading latex symbols file! Check your installation!"),
			 i18n("Error"));
    }

    // read all symbols
    QTextStream stream(&f);
    QString heading;
    QStringList l;
    QString s;
    _symbols.clear();
    do {
      l.clear();
      heading = stream.readLine();

      while ( ! (s = stream.readLine()).isEmpty() ) {
	l.append(s);
      }
      if (!heading.isEmpty())
	_symbols[heading] = l;
    } while ( ! heading.isNull() ) ;
    // all symbols are now in _symbols.
  }

  // pre-cache all our symbols
  QStringList alllist;
  for (QMap<QString,QStringList>::const_iterator it = _symbols.begin(); it != _symbols.end(); ++it) {
    alllist += it.data();
  }
  mCache->precacheList(alllist, true, this);

  // create our UI
  // clear our Tab Widget
  QWidget *w;
  while ((w = mTabs->currentPage()) != 0) {
    mTabs->removePage(w);
    delete w;
  }
  for (QMap<QString,QStringList>::const_iterator it = _symbols.begin(); it != _symbols.end(); ++it) {
    // insert a tab page and an icon view
    QWidget *w = new QWidget(mTabs);
    QVBoxLayout *lyt = new QVBoxLayout(w);
    QIconView *icv = new QIconView(w);
    icv->setResizeMode(QIconView::Adjust);
    icv->setSpacing(15);
    QFont font = icv->font();
    font.setPointSize(font.pointSize()-2);
    icv->setFont(font);
    const QStringList& sl = it.data();
    for (uint k = 0; k < sl.size(); k++) {
      (void) new KLFLatexSymbolsViewItem(icv, sl[k], mCache->getPixmap(sl[k]));
    }

    lyt->addWidget(icv);
    mTabs->addTab(w, it.key());

    connect(icv, SIGNAL(doubleClicked(QIconViewItem *)), this, SLOT(slotNeedsInsert(QIconViewItem *)));
  }
  // all ok.

  // Finish setting up UI

  btnClose->setIconSet(QIconSet(locate("appdata", "pics/closehide.png")));

  connect(btnClose, SIGNAL(clicked()), this, SLOT(slotClose()));
}

KLFLatexSymbols::~KLFLatexSymbols()
{
  --mCacheRefCounter;
  if (mCacheRefCounter <= 0) {
    QString s = locateLocal("appdata", "symbolspixmapcache");
    if (!s.isEmpty()) {
      QFile f(s);
      f.open(IO_WriteOnly);
      QDataStream ds(&f);
      mCache->saveCache(ds);
    }

    delete mCache;
    mCache = 0;
  }
}


void KLFLatexSymbols::slotNeedsInsert(QIconViewItem *item)
{
  if (item->rtti() != KLFLatexSymbolsViewItem::RTTI)
    return;
  KLFLatexSymbolsViewItem *klfitem = (KLFLatexSymbolsViewItem *) item;

  emit insertSymbol(klfitem->latex());
}


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





#include "klflatexsymbols.moc"
