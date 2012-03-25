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



class KLFLibWikiFetcher : public QObject
{
  Q_OBJECT;
public:
  KLFLibWikiFetcher(QObject *parent)
    : QObject(parent)
  {
    pNetAccess = new QNetworkAccessManager(this);
    connect(pNetAccess, SIGNAL(finished(QNetworkReply*)),
	    this, SLOT(netReply(QNetworkReply*)));
  }

  QNetworkAccessManager *pNetAccess;


  struct WikiFormula
  {
    uint id;
    QString latex;
    QByteArray imageref;
    QImage image;
  };

  QList<WikiFormula> formulas;
  

signals:

  void signalNewData();
  void signalDataUpdated(int k);

public slots:

  void loadWikiPage(const QUrl& url)
  {
    QNetworkRequest request;
    request.setUrl(url);
    request.setRawHeader("User-Agent", "KLatexFormula WikiScan (" KLF_VERSION_STRING ")");

    // request the page
    pNetAccess->get();
  }

  void requestImageFor(int k)
  {
    if (k <= 0 || k > formulas.size()) {
      klfWarning("formula index "<<k<< " out of range.") ;
      return;
    }
    if (!formulas[k].image.isNull()) {
      klfWarning("Image for formula #"<<k<<" is already loaded.") ;
      return;
    }

    QNetworkRequest request;
    request.setUrl(formulas[k].imageref);
    request.setRawHeader("User-Agent", "KLatexFormula WikiScan (" KLF_VERSION_STRING ")");

    QNetworkReply * reply = pNetAccess->get();
    reply->setProperty("klfwikiscan_download_f_image", QVariant(true));
    reply->setProperty("klfwikiscan_formula_id", QVariant::fromValue<uint>(formulas[k].id));
  }

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
      emit signalDataUpdated(k);
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
  }

private:

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


class KLFLibWikiScanPrivate : public QObject
{
  Q_OBJECT;
public:
  KLF_PRIVATE_QOBJ_HEAD(KLFLibWikiScan, QObject)
  {
  }



};






#endif
