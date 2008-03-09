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
#include <qmap.h>
#include <qvaluelist.h>
#include <qpair.h>
#include <qwidgetstack.h>
#include <qlineedit.h>
#include <qtooltip.h>

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

  QPixmap getPixmap(const QString& symentrylatex, bool fromcacheonly = true)
  {
    if (cache.contains(symentrylatex))
      return cache[symentrylatex];

    if (fromcacheonly) // if we weren't able to load it from cache, show failed icon
      return QPixmap(locate("appdata", "pics/badsym.png"));

    // parse symentrylatex
    QString xtrapreamble = KLFLatexSymbols::xtrapreamble(symentrylatex);
    QString latex = KLFLatexSymbols::latexsym(symentrylatex);

    KLFBackend::klfInput in;
    in.latex = latex;
    in.mathmode = "\\[ ... \\]";
    in.preamble = xtrapreamble+"\n";
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
      fprintf(stderr, "ERROR: Can't generate preview for symbol %s !\n\tError: %s\n", (const char*)symentrylatex.local8Bit(),
	      (const char*)out.errorstr.local8Bit());
      return QPixmap();
    }

    cache[symentrylatex] = out.result;

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
      getPixmap(list[i], false);
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
    : QIconViewItem(view, "", pix), _latexsym(KLFLatexSymbols::latexsym(ltx)),
      _xtrapreamble(KLFLatexSymbols::xtrapreamble(ltx))
  {
  }

  virtual int rtti() const { return RTTI; }

  QString latexsym() const { return _latexsym; }
  QString xtrapreamble() const { return _xtrapreamble; }

protected:
  QString _latexsym;
  QString _xtrapreamble;
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
    if (s.isEmpty()) {
      s = locate("appdata", "symbolspixmapcache_base");
    }
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

      while ( ! heading.isEmpty() && ! (s = stream.readLine()).isEmpty() ) {
	l.append(s);
      }
      if ( ! heading.isEmpty() )
	_symbols.append(QPair<QString,QStringList>(heading, l));
    } while ( ! heading.isNull() ) ;
    // all symbols are now in _symbols.
  }

  /*
   // debug: dump symbol list
   printf("DEBUG: symbols:\n");
   for (uint klj = 0; klj < _symbols.size(); ++klj) {
   printf("\t%s\n", _symbols[klj].first.ascii());
   QStringList sl = _symbols[klj].second;
   for (uint kl = 0; kl < sl.size(); ++kl) {
   printf("\t\t%s\n", sl[kl].ascii());
   }
   }
  */

  // pre-cache all our symbols
  QStringList alllist;
  for (QValueList<QPair<QString,QStringList> >::const_iterator it = _symbols.begin(); it != _symbols.end(); ++it) {
    alllist += (*it).second;
  }
  mCache->precacheList(alllist, true, this);

  // create our UI
  // clear our Tab Widget
  cbxCategory->clear();
  QVBoxLayout *lytstk = new QVBoxLayout(frmStackContainer);
  stkViews = new QWidgetStack(frmStackContainer);
  lytstk->addWidget(stkViews);

  mViews.clear();

  uint index = 0;
  for (index = 0; index < _symbols.size(); ++index) {
    // insert a tab page and an icon view
    QWidget *w = new QWidget(stkViews);
    QVBoxLayout *lyt = new QVBoxLayout(w);
    QIconView *icv = new QIconView(w);
    icv->setResizeMode(QIconView::Adjust);
    icv->setGridX(25);
    icv->setGridY(25);
    icv->setAutoArrange(true);
    icv->setSpacing(15);
    icv->setItemsMovable(false);
    QString tooltip = i18n("Click an item to view its LaTeX code. Double-click inserts the symbol.");
    QToolTip::add(icv, tooltip);
    QToolTip::add(icv->viewport(), tooltip);
    QFont font = icv->font();
    font.setPointSize(font.pointSize()-2);
    icv->setFont(font);
    const QStringList& sl = _symbols[index].second;
    for (uint k = 0; k < sl.size(); k++) {
      KLFLatexSymbolsViewItem *item = new KLFLatexSymbolsViewItem(icv, sl[k], mCache->getPixmap(sl[k]));
      item->setDragEnabled(false);
      item->setDropEnabled(false);
      item->setRenameEnabled(false);
      item->setSelectable(false);
    }

    lyt->addWidget(icv);
    stkViews->addWidget(w, index);
    cbxCategory->insertItem(_symbols[index].first, index);

    connect(icv, SIGNAL(clicked(QIconViewItem *)), this, SLOT(slotDisplayItem(QIconViewItem *)));
    connect(icv, SIGNAL(selectionChanged(QIconViewItem *)), this, SLOT(slotDisplayItem(QIconViewItem *)));
    connect(icv, SIGNAL(doubleClicked(QIconViewItem *)), this, SLOT(slotNeedsInsert(QIconViewItem *)));

    mViews.append(icv);
  }
  slotShowCategory(0);
  connect(cbxCategory, SIGNAL(highlighted(int)), this, SLOT(slotShowCategory(int)));
  connect(btnInsert, SIGNAL(clicked()), this, SLOT(slotInsertCurrentDisplay()));
  // all ok.


  // Finish setting up UI
  btnInsert->setPixmap(QPixmap(locate("appdata", "pics/insertsymb.png")));
  btnClose->setIconSet(QIconSet(locate("appdata", "pics/closehide.png")));
  lneDisplay->setPaletteBackgroundColor(lneDisplay->palette().active().background());
  lneDisplayXtrapreamble->setPaletteBackgroundColor(lneDisplayXtrapreamble->palette().active().background());

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

// static method
QString KLFLatexSymbols::xtrapreamble(const QString& symentry)
{
  // parse symentry
  // it should be in the form:              *EXTRAPREAMBLE*\latexsymbol
  // or, if no extra preamble is needed:    \latexsymbol
  // if a literal '*' is needed at the beginning, start with a space.
  // EXTRAPREAMBLE should contain a list of Latex commands that should be inserted into preamble
  // if they are not there yet; separate commands with '%%'
  if (symentry[0] == '*' && symentry.find('*', 1) != -1) {
    return symentry.section('*', 0, 0, QString::SectionSkipEmpty);
  }
  return "";
}
// static method
QString KLFLatexSymbols::latexsym(const QString& symentry)
{
  // see xtrapreamble() for comment on symentry format
  int ii;
  if (symentry[0] == '*' && (ii=symentry.find('*', 1)) != -1) {
    return symentry.mid(ii+1);
  }
  return symentry;
}



void KLFLatexSymbols::reject()
{
  // reimplemeneted to stop ESC from brutally closing the dialog.
  // see comment in same function in klfhistorybrowser.cpp for details.
}

void KLFLatexSymbols::slotNeedsInsert(QIconViewItem *item)
{
  if (!item)
    return;
  if (item->rtti() != KLFLatexSymbolsViewItem::RTTI)
    return;
  KLFLatexSymbolsViewItem *klfitem = (KLFLatexSymbolsViewItem *) item;

  emit insertSymbol(klfitem->latexsym(), klfitem->xtrapreamble());
}


#define KLF_SYM_ITEM_WIDTH 50
#define KLF_SYM_ITEM_HEIGHT 40
void KLFLatexSymbols::slotShowCategory(int c)
{
  // called by combobox
  stkViews->raiseWidget(c);

  /* ** This doesn't work as expected
   const uint KLF_SYM_ITEM_WIDTH = 50;
   const uint KLF_SYM_ITEM_HEIGHT = 40;
   for (uint i = 0; i < mViews.size(); ++i) {
   QIconViewItem *it = mViews[i]->firstItem();
   int k = 0;
   int y = 4 + KLF_SYM_ITEM_HEIGHT;
   int maxh = 0;
   while (it) {
   it->move(k*KLF_SYM_ITEM_WIDTH, y - it->height());
   k++;
   if (it->height() > maxh)
   maxh = it->height();
   if ((k+1)*KLF_SYM_ITEM_WIDTH >= mViews[i]->contentsWidth()) {
   // reset line
   y += maxh + 10;
   maxh = 0;
   k = 0;
   }
   it = it->nextItem();
   }
   mViews[i]->update();
   }
  */
}

void KLFLatexSymbols::slotDisplayItem(QIconViewItem *item)
{
  if (!item)
    return;
  if (item->rtti() != KLFLatexSymbolsViewItem::RTTI)
    return;

  KLFLatexSymbolsViewItem *klfitem = (KLFLatexSymbolsViewItem *) item;

  lneDisplay->setText(klfitem->latexsym());
  lneDisplayXtrapreamble->setText(klfitem->xtrapreamble());
}

void KLFLatexSymbols::slotInsertCurrentDisplay()
{
  emit insertSymbol(lneDisplay->text(), lneDisplayXtrapreamble->text());
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
