/***************************************************************************
 *   file klflatexsymbols_p.h
 *   This file is part of the KLatexFormula Project.
 *   Copyright (C) 2010 by Philippe Faist
 *   philippe.faist at bluewin.ch
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


/** \file
 * This file contains internal definitions for file klflatexsymbols.cpp.
 * \internal
 */

#ifndef KLFLATEXSYMBOLS_P_H
#define KLFLATEXSYMBOLS_P_H

#include <QTimer>
#include <QThread>
#include <QToolTip>
#include <QScrollBar>

#include <klfsearchbar.h>

#include <ui_klflatexsymbols.h>
#include "klflatexsymbols.h"

#include "klfmain.h"


struct SymbolInfo
{
  SymbolInfo() : pix(), confirmed(false) { }
  
  QPixmap pix;
  //! Whether this symbol was specified in the config (true) or was only in cache previously (false)
  bool confirmed;
};

struct KLFLatexSymbolsCachePrivate
{
  KLF_PRIVATE_HEAD(KLFLatexSymbolsCache)
  {
    flag_modified = false;
    device_pixel_ratio = 1.0;
  }

  QHash<KLFLatexSymbol,SymbolInfo> cache;
  bool flag_modified;
  KLFBackend::klfSettings backendsettings;

  qreal device_pixel_ratio;

  QString klf_cache_file_name() const
  {
    QString variant;
    int device_pixel_ratio_i = (int)(device_pixel_ratio + 0.5);
    if (device_pixel_ratio_i == 1) {
      variant = "";
    } else if (device_pixel_ratio_i == 2) {
      variant = "@2x";
    } else {
      klfWarning("unknown device pixel ratio: " << device_pixel_ratio) ;
      variant = "";
    }
    return QString::fromLatin1("symbolspixmapcache%1-klf%2").arg(variant).arg(KLF_DATA_STREAM_APP_VERSION) ;
  }
};

// WARNING: Does NOT write s.confirmed (!)
inline QDataStream& operator<<(QDataStream& stream, const SymbolInfo& s)
{
  return stream << s.pix;
}

// WARNING: Does NOT read s.confirmed (!), sets to false.
inline QDataStream& operator>>(QDataStream& stream, SymbolInfo& s)
{
  s.confirmed = false;
  return stream >> s.pix;
}






struct KLFLatexSymbolsSearchIterator
{
  KLFLatexSymbolsSearchIterator(int iv = 0, int is = 0)
    : iview(iv), isymb(is) { }
  int iview;
  int isymb;

  inline bool operator==(const KLFLatexSymbolsSearchIterator& other) const
  {
    return iview == other.iview && isymb == other.isymb;
  }
};


inline QDebug& operator<<(QDebug& str, const KLFLatexSymbolsSearchIterator& it)
{
  return str<<"LtxSymbIter(v="<<it.iview<<",s="<<it.isymb<<")";
}



class KLFLatexSymbolsSearchable : public QObject, public KLFIteratorSearchable<KLFLatexSymbolsSearchIterator>
{
  Q_OBJECT
private:
  KLFLatexSymbols *s;
  QTimer tooltiptimer;
  QPoint tooltippos;
  QString tooltiptext;
  QWidget *tooltipwidget;

public:
  KLFLatexSymbolsSearchable(KLFLatexSymbols *symb) : QObject(symb), s(symb)
  {
    connect(&tooltiptimer, SIGNAL(timeout()), this, SLOT(slotToolTip()));
  }
  virtual ~KLFLatexSymbolsSearchable() { }

public slots:
  void slotToolTip()
  {
    QToolTip::showText(tooltippos, tooltiptext, tooltipwidget);
  }

  void slotInsertCurrentMatch()
  {
    KLF_DEBUG_BLOCK(KLF_FUNC_NAME) ;

    SearchIterator cur = searchCurrentIterPos();
    if (cur == searchIterEnd())
      return;
    KLFLatexSymbolsView *v = view(cur);
    if (v == NULL)
      return;
    if (cur.isymb >= 0 && cur.isymb < v->_symbols.size())
      s->emitInsertSymbol(v->_symbols[cur.isymb]);
  }

public:

  // reimplemented from KLFIteratorSearchable

  virtual SearchIterator searchIterBegin() {
    int iview = 0;
    QList<KLFLatexSymbolsView*> views = s->categoryViews();
    while (iview < views.size() && views[iview]->mSymbols.isEmpty()) {
      ++iview;
    }
    return KLFLatexSymbolsSearchIterator(iview, 0);
  }
  virtual SearchIterator searchIterEnd()
  {
    QList<KLFLatexSymbolsView*> views = s->categoryViews();
    return KLFLatexSymbolsSearchIterator(views.size(), 0);
  }
  virtual SearchIterator searchIterAdvance(const SearchIterator& pos, bool forward)
  {
    klfDbg("pos="<<pos<<"; forward="<<forward);
    SearchIterator it = pos;
    // sanity
    KLF_ASSERT_CONDITION( !(forward && it == searchIterEnd()), "Attempt to advance 'end' iterator!",
			  return searchIterEnd() ) ;
    KLF_ASSERT_CONDITION( !(!forward && it == searchIterBegin()), "Attempt to decrement 'begin' iterator!",
			  return searchIterBegin() ) ;
    KLFLatexSymbolsView *v = view(it);
    if (forward) {
      // advance
      it.isymb++;
      while (v != NULL && it.isymb >= v->mSymbols.size()) {
	it.iview++; it.isymb = 0;
	v = view(it);
      }
      return KLF_DEBUG_TEE(it);
    } else {
      // decrement
      it.isymb--;
      while (it.iview > 0 && (it.isymb < 0 || (v != NULL && it.isymb >= v->mSymbols.size()))) {
	klfDbg("decrementing: it="<<it) ;
	it.iview--;
	v = view(it);
	it.isymb =  (v!=NULL) ? v->mSymbols.size()-1 : 0 ;
      }
      return KLF_DEBUG_TEE(it);
    }
  }

  virtual SearchIterator searchIterStartFrom(bool forward)
  {
    // current symbols page
    int i = s->currentCategoryIndex();
    KLFLatexSymbolsSearchIterator it(i, 0);

    QList<KLFLatexSymbolsView*> views = s->categoryViews();
    while (it.iview < views.size() && views[it.iview]->mSymbols.isEmpty()) {
      ++it.iview;
    }
    // if reverse, set on last symbol of page
    if (!forward && it.iview < views.size()) {
      it.isymb = views[it.iview]->mSymbols.size();
    }
    it = searchAdvanceIteratorCycle(it, forward ? -1 : 1);
    klfDbg("Starting search from "<<it) ;
    return it;
  }

  virtual bool searchIterMatches(const SearchIterator& pos, const QString& queryString)
  {
    KLFLatexSymbolsView *v = view(pos);
    KLF_ASSERT_NOT_NULL(v, "View for "<<pos<<" is NULL!", return false; ) ;

    return v->symbolMatches(pos.isymb, queryString);
  }

  virtual void searchMoveToIterPos(const SearchIterator& pos)
  {
    QToolTip::hideText();
    if (!(pos == searchIterEnd())) {
      KLFLatexSymbolsView *v = view(pos);
      KLF_ASSERT_NOT_NULL(v, "View for "<<pos<<" is NULL!", return ; ) ;
      s->slotShowCategory(pos.iview);
      //s->u->cbxCategory->setCurrentIndex(pos.iview);
      //s->stkViews->setCurrentWidget(v);
      QWidget *w = v->mSymbols[pos.isymb];
      KLF_ASSERT_CONDITION_ELSE(pos.isymb >= 0 && pos.isymb < v->mSymbols.size(),
				"Invalid pos object:"<<pos<<"; symb list size="<<v->mSymbols.size(),
				;) {
	v->ensureWidgetVisible(w);
      }
      klfDbg("pos is "<<pos) ;
      // show tooltip after some delay
      tooltiptext = w->toolTip();
      tooltippos = v->viewport()->mapToGlobal(w->geometry().center() -
					      QPoint(v->horizontalScrollBar()->value(),
						     v->verticalScrollBar()->value()));
      tooltipwidget = w;
      tooltiptimer.stop();
      tooltiptimer.setSingleShot(true);
      tooltiptimer.setInterval(1000);
      tooltiptimer.start();
    }
  }
  virtual void searchPerformed(const SearchIterator& pos)
  {
    if (!(pos == searchIterEnd())) {
      KLFLatexSymbolsView *v = view(pos);
      KLF_ASSERT_NOT_NULL(v, "View for "<<pos<<" is NULL!", return ; ) ;
      v->highlightSearchMatches(pos.isymb, searchQueryString());
    } else {
      int i = s->currentCategoryIndex();
      QList<KLFLatexSymbolsView*> views = s->categoryViews();
      KLF_ASSERT_CONDITION_ELSE(i>=0 && i<views.size(), "bad current view index "<<i<<"!", ;) {
	KLFLatexSymbolsView *v = views[i];
	// `current match' at 1 past end, so as to highlight all other matches, too
	v->highlightSearchMatches(v->mSymbols.size(), searchQueryString());
      }
    }
    klfDbg("pos is is "<<pos) ;
    KLFIteratorSearchable<KLFLatexSymbolsSearchIterator>::searchPerformed(pos);
  }
  virtual void searchAborted()
  {
    int i = s->currentCategoryIndex();
    QList<KLFLatexSymbolsView*> views = s->categoryViews();
    KLF_ASSERT_CONDITION_ELSE(i>=0 && i<views.size(), "bad current view index "<<i<<"!", ;) {
      KLFLatexSymbolsView *v = views[i];
      v->highlightSearchMatches(-1, searchQueryString());
    }
    KLFIteratorSearchable<KLFLatexSymbolsSearchIterator>::searchAborted();
  }
  virtual void searchReinitialized()
  {
    int i = s->currentCategoryIndex();
    QList<KLFLatexSymbolsView*> views = s->categoryViews();
    KLF_ASSERT_CONDITION_ELSE(i>=0 && i<views.size(), "bad current view index "<<i<<"!", ;) {
      KLFLatexSymbolsView *v = views[i];
      v->highlightSearchMatches(-1, searchQueryString());
    }
    KLFIteratorSearchable<KLFLatexSymbolsSearchIterator>::searchReinitialized();
  }


  KLFLatexSymbolsView * view(SearchIterator it)
  {
    QList<KLFLatexSymbolsView*> views = s->categoryViews();
    if (it.iview < 0 || it.iview >= views.size()) {
      return NULL;
    }
    return views[it.iview];
  }

};






class KLFLatexSymbolPreviewsGenerator : public QObject
{
  Q_OBJECT
public:
  KLFLatexSymbolPreviewsGenerator(QObject * parent = NULL)
    : QObject(parent), pBackendSettings(), pDevicePixelRatio(1.0)
  {
  }

  void setBackendSettings(const KLFBackend::klfSettings & settings)
  {
    pBackendSettings = settings;
  }

  void setDevicePixelRatio(qreal ratio)
  {
    pDevicePixelRatio = ratio;
  }

private:
  KLFBackend::klfSettings pBackendSettings;
  qreal pDevicePixelRatio;

signals:
  void progress(int percent) ;

  void previewGenerated(const KLFLatexSymbol& symbol, const QPixmap & pixmap);

  void started();
  void finished();

public slots:
  void generatePreviewList(const QList<KLFLatexSymbol> & list)
  {
    KLF_DEBUG_TIME_BLOCK(KLF_FUNC_NAME) ;

    emit started();

    for (int i = 0; i < list.size(); ++i) {

      klfDbg("generating preview for symbol #"<<i<<": "<<list[i].symbol) ;

      if ( QThread::currentThread()->isInterruptionRequested() ) {
        klfDbg("thread was interrupted, returning") ; 
        QThread::currentThread()->quit();
        return;
      }

      emit progress(i * 100 / list.size()) ;

      const KLFLatexSymbol & sym = list[i];

      if (sym.hidden) {
        // special treatment for hidden symbols
        // insert a QPixmap() into cache and return it
        klfDbg("symbol is hidden. Assigning NULL pixmap.") ;
        emit previewGenerated(sym, QPixmap());
        continue;
      }

      const double mag = 8.0;
      
      KLFBackend::klfInput in;
      in.latex = sym.latexCodeForPreview();
      in.mathmode = sym.textmode ? "..." : "\\[ ... \\]";
      in.preamble = sym.preamble.join("\n")+"\n";
      in.fg_color = qRgb(0,0,0);
      in.bg_color = qRgba(255,255,255,0); // transparent Bg
      in.dpi = (int)(mag * 150 * pDevicePixelRatio);
      
      pBackendSettings.epstopdfexec = ""; // don't waste time making PDF, we don't need it
      pBackendSettings.tborderoffset = sym.bbexpand.t;
      pBackendSettings.rborderoffset = sym.bbexpand.r;
      pBackendSettings.bborderoffset = sym.bbexpand.b;
      pBackendSettings.lborderoffset = sym.bbexpand.l;
      
      KLFBackend::klfOutput out = KLFBackend::getLatexFormula(in, pBackendSettings);

      if (out.status != 0) {
        klfWarning("Can't generate preview for symbol " << sym.symbol << " : status " << out.status << "!"
                   << "\n\tError: " << out.errorstr) ;
        continue;
      }
      klfDbg("successfully got pixmap for symbol "<<sym.symbol<<".") ;

      QImage scaled = out.result.scaled((int)(out.result.width() / mag),
                                        (int)(out.result.height() / mag),
                                        Qt::IgnoreAspectRatio, Qt::SmoothTransformation);
      QPixmap pix = QPixmap::fromImage(scaled);
      klfDbg("Ran getLatexFormula() to get the pixmap, adding to list.") ;

      emit previewGenerated(sym, pix);

    }

    emit finished();
  }
};





struct KLFLatexSymbolsPrivate : public QObject
{
  Q_OBJECT
public:
  KLF_PRIVATE_QOBJ_HEAD(KLFLatexSymbols, QObject)
  {
    pCache = NULL;
    pSearchable = NULL;
    pPreviewsGenerator = NULL;
  }

  KLFLatexSymbolsCache * pCache;

  KLFLatexSymbolsSearchable * pSearchable;

  QList<KLFLatexSymbolsView *> mViews;

  QThread workerThread;
  KLFLatexSymbolPreviewsGenerator *pPreviewsGenerator;

  void read_symbols_create_ui();

public slots:
  void previewGenerated(const KLFLatexSymbol & symbol, const QPixmap & pixmap);
  void generatePreviewsFinished();
  void skipGenPreviews();
};





#endif
