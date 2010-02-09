/***************************************************************************
 *   file klflibbrowser.h
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


#ifndef KLFLIBBROWSER_H
#define KLFLIBBROWSER_H

#include <QWidget>

#include <klflib.h>
#include <klflibview.h>


namespace Ui { class KLFLibBrowser; }

class KLFLibBrowser : public QWidget
{
  Q_OBJECT
public:
  KLFLibBrowser(QWidget *parent = NULL);
  virtual ~KLFLibBrowser();

  static QString urlToResourceTitle(const QUrl& url);

public slots:
  bool openResource(const QUrl& url);
  bool closeResource(const QUrl& url);

protected slots:

  void slotEntriesSelected(const QList<KLFLibEntry>& entries);

protected:
  KLFAbstractLibView * findOpenUrl(const QUrl& url);

private:
  Ui::KLFLibBrowser *pUi;
  QList<KLFAbstractLibView*> pLibViews;
};




#endif
