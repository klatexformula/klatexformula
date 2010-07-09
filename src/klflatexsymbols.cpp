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

#include <ui_klflatexsymbols.h>

#include "klfpixmapbutton.h"
#include "klfmain.h"
#include "klfconfig.h"
#include "klflatexsymbols.h"



// ------------------


KLFLatexSymbol::KLFLatexSymbol(const QDomElement& e)
  : symbol(), preamble(), textmode(false), bbexpand(), hidden(false)
{
  // preamble
  QDomNodeList usepackagelist = e.elementsByTagName("usepackage");
  int k;
  for (k = 0; k < usepackagelist.size(); ++k) {
    QString package = usepackagelist.at(k).toElement().attribute("name");
    if (package[0] == '[' || package[0] == '{')
      preamble.append(QString::fromAscii("\\usepackage%1").arg(package));
    else
      preamble.append(QString::fromAscii("\\usepackage{%1}").arg(package));
  }
  QDomNodeList preamblelinelist = e.elementsByTagName("preambleline");
  for (k = 0; k < preamblelinelist.size(); ++k) {
    preamble.append(preamblelinelist.at(k).toElement().text());
  }
  // textmode
  if (e.attribute("textmode") == "true")
    textmode = true;
  else
    textmode = false;

  if (e.elementsByTagName("hidden").size() > 0)
    hidden = true;

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

  klfDbg("read symbol "<<symbol<<" hidden="<<hidden);
}

KLF_EXPORT bool operator==(const KLFLatexSymbol& a, const KLFLatexSymbol& b)
{
  return a.symbol == b.symbol &&
    a.textmode == b.textmode &&
    a.preamble == b.preamble &&
    a.bbexpand.t == b.bbexpand.t &&
    a.bbexpand.r == b.bbexpand.r &&
    a.bbexpand.b == b.bbexpand.b &&
    a.bbexpand.l == b.bbexpand.l &&
    a.hidden == b.hidden;
}

KLF_EXPORT bool operator<(const KLFLatexSymbol& a, const KLFLatexSymbol& b)
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
  return stream << s.symbol << s.preamble << (quint8)s.textmode
		<< (qint16)s.bbexpand.t << (qint16)s.bbexpand.r
		<< (qint16)s.bbexpand.b << (qint16)s.bbexpand.l << (quint8)s.hidden;
}

QDataStream& operator>>(QDataStream& stream, KLFLatexSymbol& s)
{
  quint8 textmode, hidden;
  struct { qint16 t, r, b, l; } readbbexpand;
  stream >> s.symbol >> s.preamble >> textmode >> readbbexpand.t >> readbbexpand.r
	 >> readbbexpand.b >> readbbexpand.l >> hidden;
  s.bbexpand.t = readbbexpand.t;
  s.bbexpand.r = readbbexpand.r;
  s.bbexpand.b = readbbexpand.b;
  s.bbexpand.l = readbbexpand.l;
  s.textmode = textmode;
  s.hidden = hidden;
  return stream;
}



// -----------------------------------------------------------


KLFLatexSymbolsCache * KLFLatexSymbolsCache::staticCache = NULL;

KLFLatexSymbolsCache::KLFLatexSymbolsCache()
{
  // load the cache

  QStringList cachefiles;
  cachefiles
    << klfconfig.homeConfigDir+"/symbolspixmapcache-klf"+klfVersionMaj()+"."+klfVersionMin()
    << ":/data/symbolspixmapcache_base" ;
  int k;
  bool ok = false;
  for (k = 0; !ok && k < cachefiles.size(); ++k) {
    ok = (  loadCacheFrom(cachefiles[k], QDataStream::Qt_4_4)
	    == KLFLatexSymbolsCache::Ok  ) ;
    if (!ok)
      ok = (  loadCacheFrom(cachefiles[k], -1)
	      == KLFLatexSymbolsCache::Ok  ) ;
  }
  if ( ! ok ) {
    qWarning() << KLF_FUNC_NAME << ": error finding and reading cache file!";
  }

  flag_modified = false;
}

int KLFLatexSymbolsCache::loadCacheStream(QDataStream& stream)
{
  QString magic;
  stream.setVersion(QDataStream::Qt_3_3);
  stream >> magic;
  if (magic != "KLATEXFORMULA_SYMBOLS_PIXMAP_CACHE") {
    return BadHeader;
  }
  qint16 vmaj, vmin;
  stream >> vmaj >> vmin;

  if (vmaj != klfVersionMaj() || vmin != klfVersionMin()) {
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

int KLFLatexSymbolsCache::saveCacheStream(QDataStream& stream)
{
  stream.setVersion(QDataStream::Qt_3_3);
  // Write version 3.2 stream (added hidden symbol attribute at KLF 3.2)
  stream << QString("KLATEXFORMULA_SYMBOLS_PIXMAP_CACHE")
	 << (qint16)3 << (qint16)2
	 << (qint16)stream.version() << cache;
  flag_modified = false;
  return 0;
}

QPixmap KLFLatexSymbolsCache::getPixmap(const KLFLatexSymbol& sym, bool fromcacheonly)
{
  klfDbg("sym.symbol="<<sym.symbol<<" fromCacheOnly="<<fromcacheonly) ;

  if (cache.contains(sym)) {
    klfDbg("Found symbol in cache! sym.preamble="<<sym.preamble.join(";"));
    return cache[sym];
  }

  if (fromcacheonly) {
    // if we weren't able to load it from cache, show failed icon
    return QPixmap(":/pics/badsym.png");
  }

  // clean cache: make sure there are no two duplicate symbols (this is for the popup hint parser,
  // so that it doesn't detect old symbols in the cache)
  // This is done only if fromcache is false, so as to perform the check only on first pass
  // when generating the symbol cache.
  QMap<KLFLatexSymbol,QPixmap>::iterator it = cache.begin();
  while (it != cache.end()) {
    klfDbg("Testing symbol "<<it.key().symbol<<",preamble="<<it.key().preamble.join(",")
	   << "for being a duplicate of "<<sym.symbol);
    if (it.key().symbol == sym.symbol) {
      klfDbg("erasing duplicate.");
      it = cache.erase(it); // erase old symbol entry
    } else {
      ++it;
    }
  }

  if (sym.hidden) {
    // special treatment for hidden symbols
    // insert a QPixmap() into cache and return it
    return ( cache[sym] = QPixmap() );
  }

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
    qWarning()
      <<KLF_FUNC_NAME
      <<QString(":ERROR: Can't generate preview for symbol %1 : status %2 !\n\tError: %3\n")
      .arg(sym.symbol).arg(out.status).arg(out.errorstr);
    return QPixmap(":/pics/badsym.png");
  }

  flag_modified = true;

  QImage scaled = out.result.scaled((int)(out.result.width() / mag),
				    (int)(out.result.height() / mag),
				    Qt::IgnoreAspectRatio, Qt::SmoothTransformation);
  QPixmap pix = QPixmap::fromImage(scaled);
  cache[sym] = pix;

  return pix;
}

int KLFLatexSymbolsCache::precacheList(const QList<KLFLatexSymbol>& list, bool userfeedback,
				       QWidget *parent)
{
  QProgressDialog *pdlg = NULL;

  if (userfeedback) {
    pdlg = new QProgressDialog(QObject::tr("Please wait while generating symbol previews ... "),
			       QObject::tr("Skip"), 0, list.size()-1, parent);
    /** TODO: we should do a first pass to see which symbols are missing, then
     * on a second pass generate those symbols with a progress dialog... */
    //  pdlg->setMinimumDuration(15000);
    pdlg->setWindowModality(Qt::WindowModal);
    pdlg->setModal(true);
    pdlg->setValue(0);
  }

  for (int i = 0; i < list.size(); ++i) {
    if (userfeedback) {
      // get events for cancel button (for example)
      qApp->processEvents();
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



void KLFLatexSymbolsCache::setBackendSettings(const KLFBackend::klfSettings& settings)
{
  backendsettings = settings;
}

KLFLatexSymbol KLFLatexSymbolsCache::findSymbol(const QString& symbolCode)
{
  for (QMap<KLFLatexSymbol,QPixmap>::const_iterator it = cache.begin();
       it != cache.end(); ++it) {
    if (it.key().symbol == symbolCode)
      return it.key();
  }
  return KLFLatexSymbol();
}

QStringList KLFLatexSymbolsCache::symbolCodeList()
{
  QStringList l;
  for (QMap<KLFLatexSymbol,QPixmap>::const_iterator it = cache.begin();
       it != cache.end(); ++it)
    l << it.key().symbol;
  return l;
}







// private
int KLFLatexSymbolsCache::loadCacheFrom(const QString& fname, int version)
{
  QFile f(fname);
  if ( ! f.open(QIODevice::ReadOnly) ) {
    return -1;
  }
  QDataStream ds(&f);
  if (version >= 0)
    ds.setVersion(version);
  int r = loadCacheStream(ds);
  return r;
}


// static
KLFLatexSymbolsCache * KLFLatexSymbolsCache::theCache()
{
  if (staticCache == NULL) {
    staticCache = new KLFLatexSymbolsCache;
  }
  return staticCache;
}
// static
void KLFLatexSymbolsCache::saveTheCache()
{
  if (staticCache->cacheNeedsSave()) {
    QString s = 
      klfconfig.homeConfigDir + "/symbolspixmapcache-klf"+klfVersionMaj()+"."+klfVersionMin() ;
    QFile f(s);
    if ( ! f.open(QIODevice::WriteOnly) ) {
      qWarning() << KLF_FUNC_NAME<< "Can't save cache to file "<< s << "!";
      return;
    }
    QDataStream ds(&f);
    ds.setVersion(QDataStream::Qt_4_4);
    staticCache->saveCacheStream(ds);
    klfDbg("Saved cache to file "<<s);
  }
}




// -----------------------------------------------------------





KLFLatexSymbolsView::KLFLatexSymbolsView(const QString& category, QWidget *parent)
  : QScrollArea(parent), _category(category)
{
  mFrame = new QWidget(this);

  setWidgetResizable(true);

  //  mFrame->setSizePolicy(QSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding));
  //  mFrame->setFrameShadow(QFrame::Sunken);
  //  mFrame->setFrameShape(QFrame::Box);
  mFrame->setObjectName("frmSymbolList");
  //   mFrame->setFrameShadow(QFrame::Plain);
  //   mFrame->setFrameShape(QFrame::NoFrame);
  //   mFrame->setMidLineWidth(0);
  //   mFrame->setLineWidth(0);

  mLayout = 0;
  mSpacerItem = 0;

  setWidget(mFrame);
}

void KLFLatexSymbolsView::setSymbolList(const QList<KLFLatexSymbol>& symbols)
{
  // filter out hidden symbols
  _symbols.clear();
  int k;
  for (k = 0; k < symbols.size(); ++k)
    if ( ! symbols[k].hidden )
      _symbols.append(symbols[k]);
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
    QPixmap p = KLFLatexSymbolsCache::theCache()->getPixmap(_symbols[i]);
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
    QString tooltiptext =
      "<!DOCTYPE HTML PUBLIC \"-//W3C//DTD HTML 4.0//EN\""
      " \"http://www.w3.org/TR/REC-html40/strict.dtd\">\n"
      "<html><head><meta name=\"qrichtext\" content=\"1\" />"
      "<style type=\"text/css\">\n"
      "p { white-space: pre-wrap; padding: 0px; margin: 0px 0px 2px 0px; }\n"
      "pre { padding: 0px; margin: 0px 0px 2px 0px; }\n"
      "</style>"
      "</head>"
      "<body>\n"
      "<p style=\"white-space: pre\">"+tr("LaTeX code:")+" <b><tt>"+_symbols[i].symbol+"</tt></b>"+
      (_symbols[i].textmode?tr(" [in text mode]"):QString(""))+
      +"</p>";
    if (_symbols[i].preamble.size())
      tooltiptext += "<p>"+tr("Requires:")+"<b><pre>" +
	_symbols[i].preamble.join("\n")+"</pre></b></p>";
    tooltiptext += "</body></html>";
    btn->setToolTip(tooltiptext);
    klfDbg("tooltip text is "<<tooltiptext);
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






KLFLatexSymbols::KLFLatexSymbols(QWidget *parent, const KLFBackend::klfSettings& baseSettings)
  : QWidget(
#if defined(Q_OS_WIN32)
	    0 /* parent */
#else
	    parent /* 0 */
#endif
	    , /*Qt::Tool*/ Qt::Window /*0*/)
{
  u = new Ui::KLFLatexSymbols;
  u->setupUi(this);
  setAttribute(Qt::WA_StyledBackground);

  KLFLatexSymbolsCache::theCache()->setBackendSettings(baseSettings);

  QList<KLFLatexSymbol> allsymbols;
  // create our UI
  u->cbxCategory->clear();
  QGridLayout *lytstk = new QGridLayout(u->frmStackContainer);
  stkViews = new QStackedWidget(u->frmStackContainer);
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
      fprintf(stderr, "Error: Can't open latex symbols XML file `%s'!\n",
	      fn.toLocal8Bit().constData());
    }

    QDomDocument doc("latexsymbols");
    doc.setContent(&file);
    file.close();

    QDomElement root = doc.documentElement();
    if (root.nodeName() != "latexsymbollist") {
      fprintf(stderr, "Error parsing XML for latex symbols from file `%s'!\n",
	      fn.toLocal8Bit().constData());
    } else {
      QDomNode n;
      for (n = root.firstChild(); ! n.isNull(); n = n.nextSibling()) {
	QDomElement e = n.toElement(); // try to convert the node to an element.
	if ( e.isNull() || n.nodeType() != QDomNode::ElementNode )
	  continue;
	if ( e.nodeName() != "category" ) {
	  qWarning("WARNING in parsing XML : ignoring unexpected tag `%s'!\n",
		   qPrintable(e.nodeName()));
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
	    fprintf(stderr,
		    "WARNING in parsing XML : ignoring unexpected tag `%s' in category `%s'!\n",
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
	connect(view, SIGNAL(symbolActivated(const KLFLatexSymbol&)),
		this, SIGNAL(insertSymbol(const KLFLatexSymbol&)));
	mViews.append(view);
	stkViews->addWidget(view);
	u->cbxCategory->addItem(heading);
      }
    }
  }

  // pre-cache all our symbols
  KLFLatexSymbolsCache::theCache()->precacheList(allsymbols, true, this);

  int i;
  for (i = 0; i < mViews.size(); ++i) {
    mViews[i]->buildDisplay();
  }

  slotShowCategory(0);

  connect(u->cbxCategory, SIGNAL(highlighted(int)), this, SLOT(slotShowCategory(int)));
  connect(u->cbxCategory, SIGNAL(activated(int)), this, SLOT(slotShowCategory(int)));
  connect(u->btnClose, SIGNAL(clicked()), this, SLOT(close()));
}

void KLFLatexSymbols::retranslateUi(bool alsoBaseUi)
{
  if (alsoBaseUi)
    u->retranslateUi(this);
}


KLFLatexSymbols::~KLFLatexSymbols()
{
  KLFLatexSymbolsCache::saveTheCache();
}

void KLFLatexSymbols::slotShowCategory(int c)
{
  // called by combobox
  stkViews->setCurrentIndex(c);
}

void KLFLatexSymbols::closeEvent(QCloseEvent *e)
{
  e->accept();
  emit refreshSymbolBrowserShownState(false);
}

void KLFLatexSymbols::showEvent(QShowEvent *e)
{
  QWidget::showEvent(e);
}




