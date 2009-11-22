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
/* $Id$ */

#include <stdio.h>

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
#include <QScrollBar>
#include <QApplication>
#include <QCloseEvent>
#include <QDebug>
#include <QPainter>
#include <QPlastiqueStyle>

#include <QDomDocument>
#include <QDomElement>

#include <klfbackend.h>

#include "klfmainwin.h"
#include "klflatexsymbols.h"


extern int version_maj, version_min;

// ------------------


KLFPixmapButton::KLFPixmapButton(const QPixmap& pix, QWidget *parent)
  : QPushButton(parent), _pix(pix), _pixmargin(2), _xalignfactor(0.5f), _yalignfactor(0.5f)
{
  setText(QString());
  setIcon(QIcon());
}

QSize KLFPixmapButton::minimumSizeHint() const
{
  return sizeHint();
}

QSize KLFPixmapButton::sizeHint() const
{
  // inspired by QPushButton::sizeHint() in qpushbutton.cpp

  ensurePolished();

  int w = 0, h = 0;
  QStyleOptionButton opt;
  initStyleOption(&opt);

  // calculate contents size...
  w = _pix.width() + _pixmargin;
  h = _pix.height() + _pixmargin;

  if (menu())
    w += style()->pixelMetric(QStyle::PM_MenuButtonIndicator, &opt, this);

  return (style()->sizeFromContents(QStyle::CT_PushButton, &opt, QSize(w, h), this).
	  expandedTo(QApplication::globalStrut()).expandedTo(QSize(50, 30))); // (50,30) is minimum non-square buttons on Qt/Mac
}

void KLFPixmapButton::paintEvent(QPaintEvent *event)
{
  QPushButton::paintEvent(event);
  QPainter p(this);
  p.setClipRect(event->rect());
  p.drawPixmap(QPointF( _xalignfactor*(width()-(2*_pixmargin+_pix.width())) + _pixmargin,
			_yalignfactor*(height()-(2*_pixmargin+_pix.height())) + _pixmargin ),
	       _pix);
}




// ------------------


KLFLatexSymbol::KLFLatexSymbol(const QString& symentry)
{
  // parse symentry
  // it should be in the form:              *EXTRAPREAMBLE*\latexsymbol
  // or, if no extra preamble is needed:    \latexsymbol
  // if a literal '*' is needed at the beginning, start with a space.
  // EXTRAPREAMBLE should contain a list of Latex commands that should be inserted into preamble
  // if they are not there yet; separate commands with '%%'

  int ii = 0;
  QString xtra = "";
  if (symentry[0] == '*' && (ii=symentry.indexOf('*', 1)) != -1) {
    symbol = symentry.mid(ii+1);
    xtra = symentry.mid(1, ii-1);
  } else {
    symbol = symentry;
  }

  textmode = false;

  preamble = xtra.split("%%");

  bbexpand = BBOffset(1, 1, 1, 1);
}

KLFLatexSymbol::KLFLatexSymbol(const QDomElement& e)
  : symbol(), preamble(), textmode(false), bbexpand()
{
  // preamble
  QDomNodeList preamblelinelist = e.elementsByTagName("preambleline");
  int k;
  for (k = 0; k < preamblelinelist.size(); ++k) {
    preamble.append(preamblelinelist.at(k).toElement().text());
  }
  QDomNodeList usepackagelist = e.elementsByTagName("usepackage");
  for (k = 0; k < usepackagelist.size(); ++k) {
    QString package = usepackagelist.at(k).toElement().attribute("name");
    if (package[0] == '[' || package[0] == '{')
      preamble.append(QString::fromAscii("\\usepackage%1").arg(package));
    else
      preamble.append(QString::fromAscii("\\usepackage{%1}").arg(package));
  }
  // textmode
  if (e.attribute("textmode") == "true")
    textmode = true;
  else
    textmode = false;

  // bb offset
  QDomNodeList bblist = e.elementsByTagName("bb");
  if (bblist.size() > 1) {
    fprintf(stderr, "WARNING: Expected at most single <bb expand=\"..\"/> item!\n");
  }
  if (bblist.size()) {
    sscanf(bblist.at(0).toElement().attribute("expand").toLatin1().constData(), "%d,%d,%d,%d",
	   &bbexpand.t, &bbexpand.r, &bbexpand.b, &bbexpand.l);
  }

  // latex code
  QDomNodeList latexlist = e.elementsByTagName("latex");
  if (latexlist.size() != 1) {
    fprintf(stderr, "WARNING: Expected single <latex>...</latex> in symbol entry!\n");
  }
  if (latexlist.size() == 0)
    return;
  symbol = latexlist.at(0).toElement().text();

}

bool operator<(const KLFLatexSymbol& a, const KLFLatexSymbol& b)
{
  if (a.symbol != b.symbol)
    return a.symbol < b.symbol;
  if (a.textmode != b.textmode)
    return a.textmode < b.textmode;
  if (a.preamble.size() != b.preamble.size())
    return a.preamble.size() < b.preamble.size();
  int k;
  for (k = 0; k < a.preamble.size(); ++k)
    if (a.preamble[k] != b.preamble[k])
      return a.preamble[k] < b.preamble[k];
  // a and b seem to be equal
  return false;
}

QDataStream& operator<<(QDataStream& stream, const KLFLatexSymbol& s)
{
  return stream << s.symbol << s.preamble << (quint8)s.textmode << (qint16)s.bbexpand.t
		<< (qint16)s.bbexpand.r << (qint16)s.bbexpand.b << (qint16)s.bbexpand.l;
}

QDataStream& operator>>(QDataStream& stream, KLFLatexSymbol& s)
{
  quint8 textmode;
  struct { qint16 t, r, b, l; } readbbexpand;
  stream >> s.symbol >> s.preamble >> textmode >> readbbexpand.t >> readbbexpand.r >> readbbexpand.b >> readbbexpand.l;
  s.bbexpand.t = readbbexpand.t;
  s.bbexpand.r = readbbexpand.r;
  s.bbexpand.b = readbbexpand.b;
  s.bbexpand.l = readbbexpand.l;
  s.textmode = textmode;
  return stream;
}



class KLFLatexSymbolsCache
{
public:

  enum { Ok = 0, BadHeader, BadVersion };

  KLFLatexSymbolsCache()
  {
    flag_modified = false;
  }

  bool cacheNeedsSave() const { return flag_modified; }

  int loadCache(QDataStream& stream)
  {
    QString magic;
    stream >> magic;
    if (magic != "KLATEXFORMULA_SYMBOLS_PIXMAP_CACHE") {
      return BadHeader;
    }
    qint16 vmaj, vmin;
    stream >> vmaj >> vmin;

    if (vmaj != version_maj || vmin != version_min) {
      // since this is just cache, we can require EXACT version
      return BadVersion;
    }
    // KLF 3.x, we saved the stream version, read it now
    qint16 version;
    stream >> version;
    stream.setVersion(version);

    stream >> cache;

    flag_modified = false;
    return 0;
  }

  int saveCache(QDataStream& stream)
  {
    stream.setVersion(QDataStream::Qt_3_3);
    stream << QString("KLATEXFORMULA_SYMBOLS_PIXMAP_CACHE") << (qint16)version_maj << (qint16)version_min
	   << (qint16)stream.version() << cache;
    flag_modified = false;
    return 0;
  }

  QPixmap getPixmap(const KLFLatexSymbol& sym, bool fromcacheonly = true)
  {
    if (cache.contains(sym))
      return cache[sym];

    if (fromcacheonly) // if we weren't able to load it from cache, show failed icon
      return QPixmap(":/pics/badsym.png");

    const float mag = 4.0;

    KLFBackend::klfInput in;
    in.latex = sym.symbol;
    in.mathmode = sym.textmode ? "..." : "\\[ ... \\]";
    in.preamble = sym.preamble.join("\n")+"\n";
    in.fg_color = qRgb(0,0,0);
    in.bg_color = qRgba(255,255,255,0); // transparent Bg
    in.dpi = (int)(mag * 150);

    backendsettings.epstopdfexec = ""; // don't waste time making PDF, we don't need it
    backendsettings.tborderoffset = sym.bbexpand.t;
    backendsettings.rborderoffset = sym.bbexpand.r;
    backendsettings.bborderoffset = sym.bbexpand.b;
    backendsettings.lborderoffset = sym.bbexpand.l;

    KLFBackend::klfOutput out = KLFBackend::getLatexFormula(in, backendsettings);

    if (out.status != 0) {
      qCritical() << QObject::tr("ERROR: Can't generate preview for symbol %1 : status %2 !\n\tError: %3\n")
	.arg(sym.symbol).arg(out.status).arg(out.errorstr);
      return QPixmap(":/pics/badsym.png");
    }

    flag_modified = true;

    QImage scaled = out.result.scaled((int)(out.result.width() / mag), (int)(out.result.height() / mag),
				      Qt::IgnoreAspectRatio, Qt::SmoothTransformation);
    QPixmap pix = QPixmap::fromImage(scaled);
    cache[sym] = pix;

    return pix;
  }

  int precacheList(const QList<KLFLatexSymbol>& list, bool userfeedback, QWidget *parent = 0)
  {
    QProgressDialog *pdlg = NULL;

    if (userfeedback) {
      pdlg = new QProgressDialog(QObject::tr("Please wait while generating symbol previews ... "),
				 QObject::tr("Skip"), 0, list.size()-1, parent);
      // TODO: we should do a first pass to see which symbols are missing, then
      // on a second pass generate those symbols with a progress dialog...
      //  pdlg->setMinimumDuration(15000);
      pdlg->setWindowModality(Qt::WindowModal);
      pdlg->setValue(0);
    }

    for (int i = 0; i < list.size(); ++i) {
      if (userfeedback) {
	// get events for cancel button (for example)
	qApp->processEvents(QEventLoop::AllEvents, 50);
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

  QMap<KLFLatexSymbol,QPixmap> cache;

  bool flag_modified;

};



KLFLatexSymbolsCache *KLFLatexSymbols::mCache = 0;
int KLFLatexSymbols::mCacheRefCounter = 0;







KLFLatexSymbolsView::KLFLatexSymbolsView(const QString& category, QWidget *parent)
  : QScrollArea(parent), _category(category)
{
  mFrame = new QFrame(this);

  setWidgetResizable(true);

  //  mFrame->setSizePolicy(QSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding));
  mFrame->setFrameShadow(QFrame::Sunken);
  mFrame->setFrameShape(QFrame::Box);

  mLayout = 0;
  mSpacerItem = 0;

  setWidget(mFrame);
}

void KLFLatexSymbolsView::setSymbolList(const QList<KLFLatexSymbol>& symbols)
{
  _symbols = symbols;
}

void KLFLatexSymbolsView::buildDisplay()
{
#ifdef Q_WS_MAC
  QStyle *myStyle = new QPlastiqueStyle;
  QPalette pal = palette();
  pal.setColor(QPalette::Window, QColor(206,207,233));
  pal.setColor(QPalette::Base, QColor(206,207,233));
  pal.setColor(QPalette::Button, QColor(206,207,233));
#endif
  mLayout = new QGridLayout(mFrame);
  int i;
  for (i = 0; i < _symbols.size(); ++i) {
    QPixmap p = KLFLatexSymbols::cache()->getPixmap(_symbols[i]);
    KLFPixmapButton *btn = new KLFPixmapButton(p, mFrame);
#ifdef Q_WS_MAC
    btn->setStyle(myStyle);
    btn->setPalette(pal);
#endif
    btn->setSizePolicy(QSizePolicy(QSizePolicy::Preferred, QSizePolicy::Preferred));
    btn->setProperty("symbol", (unsigned int) i);
    btn->setProperty("gridpos", QPoint(-1,-1));
    btn->setProperty("gridcolspan", -1);
    btn->setProperty("myWidth", p.width() + 4);
    btn->setToolTip(_symbols[i].symbol);
    connect(btn, SIGNAL(clicked()), this, SLOT(slotSymbolActivated()));
    mSymbols.append(btn);
  }
  mSpacerItem = new QSpacerItem(1, 1, QSizePolicy::Fixed, QSizePolicy::Expanding);
  recalcLayout();
}

void KLFLatexSymbolsView::recalcLayout()
{
  int row = 0, col = 0, colspan;
  int n = klfconfig.UI.symbolsPerLine;
  int quantawidth = 55; // hard-coded here
  //  printf("DEBUG: n=%d, quantawidth=%d\n", n, quantawidth);
  int i;
  // now add them all again as needed
  for (i = 0; i < mSymbols.size(); ++i) {
    colspan = 1 + mSymbols[i]->property("myWidth").toInt() / quantawidth;
    if (colspan < 1)
      colspan = 1;
    //    printf("DEBUG: %d: width is %d, colspan is %d\n", i, mSymbols[i]->property("myWidth").toInt(), colspan);

    if (colspan > n)
      colspan = n;
    if (col + colspan > n) {
      row++;
      col = 0;
    }
    if (mSymbols[i]->property("gridpos") != QPoint(row, col) ||
	mSymbols[i]->property("gridcolspan") != colspan) {
      //      printf("DEBUG: %d: setting to (%d,%d)+(1,%d)\n", i, row, col, colspan);
      mSymbols[i]->setProperty("gridpos", QPoint(row, col));
      mSymbols[i]->setProperty("gridcolspan", colspan);
      mLayout->removeWidget(mSymbols[i]);
      mLayout->addWidget(mSymbols[i], row, col, 1, colspan);
    }
    col += colspan;
    if (col >= n) {
      row++;
      col = 0;
    }
  }
  // remove spacer and add it again
  mLayout->removeItem(mSpacerItem);
  mLayout->addItem(mSpacerItem, row+1, 0);

  setMinimumWidth(mFrame->sizeHint().width() + verticalScrollBar()->width() + 2);
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









// returns -1 if file open read or the code returned by cache->loadCache(),
// that is KLFLatexSymbolsCache::{Ok,BadHeader,BadVersion}
int klf_load_cache(KLFLatexSymbolsCache *cache, const QString& fname)
{
  QFile f(fname);
  if ( ! f.open(QIODevice::ReadOnly) ) {
    return -1;
  }
  QDataStream ds(&f);
  int r = cache->loadCache(ds);
  return r;
}


KLFLatexSymbols::KLFLatexSymbols(KLFMainWin *mw)
  : QWidget(
#if defined(Q_OS_WIN32)
	    0 /* mw */
#else
	    mw /* 0 */
#endif
	    , /*Qt::Tool*/ Qt::Window /*0*/), KLFLatexSymbolsUI()
{
  setupUi(this);
  setObjectName("KLFLatexSymbols");


  _mainwin = mw;

  if (mCache == 0) {
    mCache = new KLFLatexSymbolsCache;
    mCache->backendsettings = mw->backendSettings();

    QStringList cachefiles
      = QStringList() << klfconfig.homeConfigDir + "/symbolspixmapcache"
		      << ":/data/symbolspixmapcache_base" ;
    int k;
    bool ok = false;
    for (k = 0; k < cachefiles.size(); ++k) {
      ok = (  klf_load_cache(mCache, cachefiles[k]) == KLFLatexSymbolsCache::Ok  ) ;
      if (ok)
	break;
    }
    if ( ! ok ) {
      qWarning() << tr("Warning: KLFLatexSymbols: error finding and reading cache file!");
    }
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
    QString fn = klfconfig.homeConfigDir + "/latexsymbols.xml";
    if ( ! QFile::exists(fn) ) {
      fn = ":/data/latexsymbols.xml";
    }
    QFile file(fn);
    if ( ! file.open(QIODevice::ReadOnly) ) {
      fprintf(stderr, "Error: Can't open latex symbols XML file `%s'!\n", fn.toLocal8Bit().constData());
    }

    QDomDocument doc("latexsymbols");
    doc.setContent(&file);
    file.close();

    QDomElement root = doc.documentElement();
    if (root.nodeName() != "latexsymbollist") {
      fprintf(stderr, "Error parsing XML for latex symbols from file `%s'!\n", fn.toLocal8Bit().constData());
    } else {
      QDomNode n;
      for (n = root.firstChild(); ! n.isNull(); n = n.nextSibling()) {
	QDomElement e = n.toElement(); // try to convert the node to an element.
	if ( e.isNull() || n.nodeType() != QDomNode::ElementNode )
	  continue;
	if ( e.nodeName() != "category" ) {
	  qWarning("WARNING in parsing XML : ignoring unexpected tag `%s'!\n", qPrintable(e.nodeName()));
	  continue;
	}
	// read category
	QString heading = e.attribute("name");
	QList<KLFLatexSymbol> l;
	QDomNode esym;
	for (esym = e.firstChild(); ! esym.isNull(); esym = esym.nextSibling() ) {
	  if ( esym.isNull() || esym.nodeType() != QDomNode::ElementNode )
	    continue;
	  if ( esym.nodeName() != "sym" ) {
	    fprintf(stderr, "WARNING in parsing XML : ignoring unexpected tag `%s' in category `%s'!\n",
		    esym.nodeName().toLocal8Bit().constData(), heading.toLocal8Bit().constData());
	    continue;
	  }
	  KLFLatexSymbol sym(esym.toElement());
	  l.append(sym);
	  allsymbols.append(sym);
	}
	// and add this category
	KLFLatexSymbolsView *view = new KLFLatexSymbolsView(heading, stkViews);
	view->setSymbolList(l);
	connect(view, SIGNAL(symbolActivated(const KLFLatexSymbol&)), this, SIGNAL(insertSymbol(const KLFLatexSymbol&)));
	mViews.append(view);
	stkViews->addWidget(view);
	cbxCategory->addItem(heading);
      }
    }
  }

  // pre-cache all our symbols
  mCache->precacheList(allsymbols, true, this);

  int i;
  for (i = 0; i < mViews.size(); ++i) {
    mViews[i]->buildDisplay();
  }

  slotShowCategory(0);

  connect(cbxCategory, SIGNAL(highlighted(int)), this, SLOT(slotShowCategory(int)));
  connect(cbxCategory, SIGNAL(activated(int)), this, SLOT(slotShowCategory(int)));
  connect(btnClose, SIGNAL(clicked()), this, SLOT(close()));
}

KLFLatexSymbols::~KLFLatexSymbols()
{
  --mCacheRefCounter;
  if (mCacheRefCounter <= 0) {
    QString s = klfconfig.homeConfigDir + "/symbolspixmapcache";

    if (mCache->cacheNeedsSave()) {
      QFile f(s);
      if ( ! f.open(QIODevice::WriteOnly) ) {
	qWarning() << tr("Can't save cache to file `%1'!").arg(s);
      } else {
	QDataStream ds(&f);
	mCache->saveCache(ds);
	qDebug("Saved cache to file %s.", qPrintable(s));
      }
    }
    delete mCache;
    mCache = 0;
  }
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


void KLFLatexSymbols::closeEvent(QCloseEvent *e)
{
  e->accept();
  emit refreshSymbolBrowserShownState(false);
}

void KLFLatexSymbols::showEvent(QShowEvent *e)
{
  //  int k;
  //  for (k = 0; k < mViews.size(); ++k) {
  //    mViews[k]->recalcLayout();
  //  }
  QWidget::showEvent(e);
}




