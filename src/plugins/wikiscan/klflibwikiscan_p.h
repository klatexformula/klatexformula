/***************************************************************************
 *   file klflibwikiscan_p.h
 *   This file is part of the KLatexFormula Project.
 *   Copyright (C) 2012 by Philippe Faist
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
 * This header contains (in principle private) auxiliary classes for
 * library routines defined in klflibwikiscan.cpp */

#ifndef KLFLIBWIKISCAN_P_H
#define KLFLIBWIKISCAN_P_H

#include <ctype.h>

#include "klflibwikiscan.h"


#define KLFWIKISCAN_USER_AGENT						\
  "KLatexFormulaWikiScan/" KLF_VERSION_STRING " (KLatexFormula WikiScan plugin, klatexformula.sf.net)"




class KLFLibWikiFetcher : public QObject
{
  Q_OBJECT;
public:

  /**
   *
   * @param wikiurl the URL to the api.php entry point of the mediawiki API. For example
   * "http://en.wikipedia.org/w/api.php"
   *
   * @param parent the parent of this QObject.
   */
  KLFLibWikiFetcher(const QUrl& wikiurl, QObject *parent)
    : QObject(parent), pWikiUrl(wikiurl)
  {
    pNetAccess = new QNetworkAccessManager(this);
    connect(pNetAccess, SIGNAL(finished(QNetworkReply*)),
	    this, SLOT(netReply(QNetworkReply*)));
  }

  struct WikiFormula
  {
    WikiFormula(uint id_ = 0, const QString& latex_ = QString(),
                const QByteArray& imageref_ = QByteArray(),
                const QImage& image_ = QImage())
      : id(id_), latex(latex_), imageref(imageref_), image_(image)
    {
    }
    uint id;
    QString latex;
    QByteArray imageref;
    QImage image;
  };

  struct WikiPage
  {
    WikiPage(const QByteArray& html_ = QByteArray(), const QList<WikiFormula> formulas_ = QList<WikiFormulas>())
      : html(html_), formulas(formulas_) { }
    QByteArray html;
    QList<QByteArray> formulas
  };

  static QMap<QString,WikiPage> cache_pages;
  //! A map of formulas with keys being the image URLs (acting as IDs)
  static QMap<QByteArray,WikiFormula> cache_formulas;


  // Workflow
  // --------
  //
  // Idea: query for http://en.wikipedia.org/w/api.php?action=mobileview&page=Lagrange%20multipliers&sections=all&format=xml
  //
  // and then parse sections & images w/ class "... tex" for LaTeX equations.
  //


  void loadWikiPage(const QString& page)
  {
    QUrl url = pWikiUrl;
    url.addEncodedQueryItem("action", "mobileview");
    url.addEncodedQUeryItem("format", "xml");
    url.addQueryItem(QLatin1String("page"), page);

    QNetworkRequest request;
    request.setUrl(url);
    request.setRawHeader("User-Agent", KLFWIKISCAN_USER_AGENT);

    // request the page
    pNetAccess->get();
  }

  void loadResourceUrl(const QString& url)
  {
    
  }

  /*
  void ensureImageFor(int k)
  {
    if (k <= 0 || k > formulas.size()) {
      klfWarning("formula index "<<k<< " out of range.") ;
      return;
    }
    if (!formulas[k].image.isNull()) {
      klfDbg("Image for formula #"<<k<<" is already loaded.") ;
      return;
    }

    QNetworkRequest request;
    request.setUrl(formulas[k].imageref);
    request.setRawHeader("User-Agent", KLFWIKISCAN_USER_AGENT);

    QNetworkReply * reply = pNetAccess->get();
    reply->setProperty("klfwikiscan_download_f_image", QVariant(true));
    reply->setProperty("klfwikiscan_formula_id", QVariant::fromValue<uint>(formulas[k].id));
    }*/

  void waitForPageLoaded()
  {
    while (!pPageLoaded) {
      // wait for the network answer
      qApp->processEvents(QEventLoop::ExcludeUserInputEvents);
    }
  }
  /** Warning: no check is made that you actually did call ensureImageFor() for this
   * formula. If you failed to do so, this will result in an infinite loop.
   */
  void waitForWikiFormulaLoaded(int k)
  {
    KLF_ASSERT_CONDITION(k >= 0 && k < formulas.size(), "k="<<k<<" out of range 0.."<<formulas.size(),
			 return ; ) ;

    while (formulas[k].image.isNull()) {
      qApp->processEvents(QEventLoop::ExcludeUserInputEvents);
    }
  }

signals:

  void loadedWikiPage();
  void loadedWikiFormula(int k);


private slots:

  void netReply(QNetworkReply *reply)
  {
    // no error received?
    if (reply->error() != QNetworkReply::NoError) {
      klfWarning("Network reply error!! err="<< reply->error());
      reply->deleteLater();
      return;
    }

    // see what kind of reply this is. A full wiki page or a formula image.
    if (reply->property("klfwikiscan_download_f_image").isValid()) {
      uint id = reply->property("klfwikiscan_formula_id").toUInt();
      // find the relevant formula
      int k;
      for (k = 0; k < formulas.size(); ++k) {
	if (formulas[k].id == id)
	  break;
      }
      if (k > formulas.size()) {
	klfWarning("Can't find requested formula: "<<id) ;
	reply->deleteLater();
	return;
      }
      QImageReader imagereader(reply);
      formulas[k].image = imagereader.read();
      emit loadedWikiFormula(k);
    }

    // parse a wiki page reply

    QByteArray alldata = reply.readAll();

    QList<WikiFormula> flist;

    // now go through all <img class="tex" ...> tags
    int i = 0;
    while ((i = alldata.indexOf("<img ", i)) >= 0) {
      // parse that img tag.
      // read the attributes
      int j = i+4;
      QMap<QByteArray,QByteArray> attribs = readTagAttribList(alldata, &j);
      i = j; // update i pointer
      if (attribs.value("class") != QLatin1String("tex")) {
	// not a latex formula, not interesting.
	continue;
      }
      QByteArray src = attribs.value("src");
      QByteArray alt = attribs.value("alt");
      if (alt.isEmpty() || src.isEmpty()) {
	klfWarning("alt="<<alt<<"; src="<<src) ;
	continue;
      }
      uint id = qHash(src);
      WikiFormula f;
      f.id = id;
      f.latex = alt;
      f.imageref = src;
      flist.append(f);
    }

    // update all the formulas
    formulas = flist;
    reply->deleteLater();

    emit loadedWikiPage();
  }

private:

  QUrl pWikiUrl;

  QNetworkAccessManager *pNetAccess;

  struct Attrib { QByteArray name; QByteArray value; } ;

#define ASSERT_NOT_AT_END(data, i, action)				\
  KLF_ASSERT_CONDITION((i) < (data).size() , "Reached end of input!" , action ) ;

  QMap<QByteArray,QByteArray> readTagAttribList(const QByteArray& data, int * pos)
  {
    QMap<QByteArray,QByteArray> attribs;

    while (*pos < data.size() && data[*pos] != '>') {
      if (isspace(data[*pos])) {
	skipSpaces(data, *pos);
	continue;
      }
      if (isalpha(data[*pos])) {
	// attribute
	Attrib a = readAttrib(data, pos);
	attribs[a.name] = a.value;
	continue;
      }
      klfWarning("Parse error: unexpected character "<<data[*pos]<<".") ;
      return alist;
    }
  }

  Attrib readAttrib(const QByteArray& data, int *pos)
  {
    // read a sequence of word, '=', value (word or quoted)
    Attrib a;

    // read the attribute name
    skipSpaces(data, pos);
    ASSERT_NOT_AT_END(data, *pos, return a ; ) ;

    a.name = readWord(data, pos);

    skipSpaces(data, pos);
    ASSERT_NOT_AT_END(data, *pos, return a ; ) ;

    // read the '=' sign
    if (data[*pos] != '=') {
      klfWarning("Parse Error: expected '=' at pos="<<*pos<<" where data[pos]="<<data[*pos]) ;
      return a;
    }
    *pos++;
    ASSERT_NOT_AT_END(data, *pos, return a ; ) ;

    // read the attrib value
    a.value = readWordOrQuoted(data, pos);
    ASSERT_NOT_AT_END(data, *pos, return a ; ) ;

    return a;
  }

  void skipSpaces(const QByteArray& data, int * pos)
  {
    // advance to next non-space
    while (*pos < data.size() && isspace(data[*pos]))
      *pos++;
    ASSERT_NOT_AT_END(data, *pos, return; ) ;
  }

  QByteArray readQuoted(const QByteArray& data, int * pos)
  {
    if (data[*pos] != '"') {
      klfWarning("not at quote position!") ;
      return QByteArray();
    }
    int origpos = *pos;
    int j = origpos+1;
    while (j < data.size() && data[j] != '"') {
      ++j;
    }
    ++j;
    *pos = j;
    return data.mid(origpos, j-origpos);
  }

  QByteArray readWord(const QByteArray& data, int * pos)
  {
    int origpos = *pos;
    // advance to next space
    while (*pos < data.size() && !isspace(data[*pos]))
      *pos++;

    return data.mid(origpos, *pos-origpos);
  }

  QByteArray readWordOrQuoted(const QByteArray& data, int * pos)
  {
    skipSpaces(data, pos);
    ASSERT_NOT_AT_END(data, *pos, return QByteArray(); ) ;
    
    // see if it is a word or a quoted value
    if (data[*pos] == '"')
      return readQuoted(data, pos);
    // otherwise, it is a bare word
    return readWord(data, pos);
  }

};









class KLFWikiPageDocument : public QTextDocument
{
  Q_OBJECT
public:
  KLFWikiPageDocument(QObject * parent)
    : QTextDocument(parent)
  {
    pWikiUrl = QUrl();

    pNetAccess = new QNetworkAccessManager(this);
    connect(pNetAccess, SIGNAL(finished(QNetworkReply*)),
	    this, SLOT(netReply(QNetworkReply*)));
  }
  virtual ~KLFWikiPageDocument()
  {
  }

  void setWikiUrl(const QUrl& wikiurl)
  {
    pWikiUrl = wikiurl;
  }

protected:

  QVariant loadResource(int restype, const QUrl& url)
  {
    QNetworkReply * reply = NULL;

    enum WhatIsLoading {
      WikiPage = 1,
      UrlResource
    } what;

    if (restype == QTextDocument::HtmlResource) {
      if (url.scheme().toLower().startsWith("http") &&
          url.host().toLower().endsWith("wikipedia.org") &&
          url.host().toLower() == pWikiUrl.host().toLower() &&
          url.path().toLower().startsWith("/wiki/")) {
        // querying a wikipedia page
        what = WikiPage;
        reply = initiateLoadWikiPage(url.path().mid(strlen("/wiki/")));
      }
    }
    // by default, load the URL resource
    if (reply == NULL) {
      what = UrlResource;
      reply = initiateLoadUrlResource(url);
    }

    reply->setProperty("klfwikiscan_reply_done", QVariant(false));

    // now, wait for the resource to download.
    while (!reply->property("klfwikiscan_reply_done").toBool()) {
      qApp->processEvents(QEventLoop::ExcludeUserInputEvents);
    }
    reply->deleteLater();

    switch (what) {
    case WikiPage:
      QString normalizedName = reply->property("klfwikiscan_wiki_page_name").toString();
      KLF_ASSERT_CONDITION(pCacheWikiPages.contains(normalizedName),
                           "Wiki page "<<normalizedName<<" loaded but not in cache !?!?",
                           return QVariant(); );
      return QVariant(pCacheWikiPages.value(normalizedName));

    case UrlResource:
      QByteArray urlstr = url.toEncoded();
      KLF_ASSERT_CONDITION(pCacheUrlResources.contains(urlstr),
                           "URL "<<urlstr<<" loaded but not in cache !?!?",
                           return QVariant(); );
      return QVariant(pCacheWikiPages.value(normalizedName));

    default:
      KLF_ASSERT_CONDITION(false, "Shouldn't get here.", return QVariant(); );
    }

    KLF_ASSERT_CONDITION(false, "Shouldn't get here (2).", return QVariant(); );
  }

  QNetworkReply * initiateLoadWikiPage(const QString& pageName)
  {
    QUrl url = pWikiUrl;
    url.addEncodedQueryItem("action", "mobileview");
    url.addEncodedQueryItem("format", "xml");
    url.addEncodedQueryItem("sections", "all");
    url.addQueryItem(QLatin1String("page"), page);

    QNetworkRequest request;
    request.setUrl(url);
    request.setRawHeader("User-Agent", KLFWIKISCAN_USER_AGENT);

    // request the page
    QNetworkReply * reply = pNetAccess->get();
    reply->setProperty("klfwikiscan_getting_wiki_page", QVariant(pageName));
    return reply;
  }

  QNetworkReply * initiateLoadUrlResource(const QUrl& url)
  {
    QNetworkRequest request;
    request.setUrl(url);
    request.setRawHeader("User-Agent", KLFWIKISCAN_USER_AGENT);

    // request the page
    QNetworkReply * reply = pNetAccess->get();
    reply->setProperty("klfwikiscan_getting_url_resource", QVariant(url));
    return reply;
  }


private slots:

  void netReply(QNetworkReply *reply)
  {
    reply->setProperty("klfwikiscan_reply_done", QVariant(true));

    // no error received?
    if (reply->error() != QNetworkReply::NoError) {
      klfWarning("Network reply error!! err="<< reply->error());
      return;
    }

    // see what kind of reply this is. A full wiki page or a formula image.
    QVariant var_wiki_page = reply->property("klfwikiscan_getting_wiki_page");
    if (var_wiki_page.isValid()) {
      QByteArray alldata = reply.readAll();

      // do some post-processing with alldata ....

      // store in cache by normalized name (TODO......)
      QString normalizedName = var_wiki_page.toString();
      reply->setProperty("klfwikiscan_wiki_page_name", QVariant(normalizedName));
      pCacheWikiPages[normalizedName] = alldata;

      /*
      // parse a wiki page reply

      QByteArray alldata = reply.readAll();

      QList<WikiFormula> flist;

      // now go through all <img class="tex" ...> tags
      int i = 0;
      while ((i = alldata.indexOf("<img ", i)) >= 0) {
      // parse that img tag.
      // read the attributes
      int j = i+4;
      QMap<QByteArray,QByteArray> attribs = readTagAttribList(alldata, &j);
      i = j; // update i pointer
      if (attribs.value("class") != QLatin1String("tex")) {
      // not a latex formula, not interesting.
      continue;
      }
      QByteArray src = attribs.value("src");
      QByteArray alt = attribs.value("alt");
      if (alt.isEmpty() || src.isEmpty()) {
      klfWarning("alt="<<alt<<"; src="<<src) ;
      continue;
      }
      uint id = qHash(src);
      WikiFormula f;
      f.id = id;
      f.latex = alt;
      f.imageref = src;
      flist.append(f);
      }
      */

      return;
    }

    // otherwise, simple URL resource ...
    QUrl url = reply.property("klfwikiscan_getting_url_resource").toUrl();
    QByteArray urlstr = url.toEncoded();
    QByteArray alldata = reply.readAll();

    pCacheUrlResoruces[urlstr] = alldata;
    return;
  }


private:

  QUrl pWikiUrl;
  QNetworkAccessManager *pNetAccess;

  QCache<QString,QByteArray> pCachePages;
  QCache<QByteArray,QByteArray> pCacheUrlResources;
};





class KLFLibWikiScanPrivate : public QObject
{
  Q_OBJECT;
public:
  KLF_PRIVATE_QOBJ_HEAD(KLFLibWikiScan, QObject)
  {
    pDoc = new KLFWikiPageDocument(this);
  }
  virtual ~KLFLibWikiScanPrivate()
  {
    delete pDoc;
  }

  KLFWikiPageDocument * pDoc;
};






#endif
