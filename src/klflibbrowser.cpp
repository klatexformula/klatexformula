/***************************************************************************
 *   file klflibbrowser.cpp
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

#include <klflibbrowser.h>

#include <ui_klflibbrowser.h>



KLFLibBrowser::KLFLibBrowser(QWidget *parent)
  : QWidget(parent)
{
  pUi = new Ui::KLFLibBrowser;
  pUi->setupUi(this);
  pUi->tabResources->clear();
}


KLFLibBrowser::~KLFLibBrowser()
{
  int k;
  for (k = 0; k < pLibViews.size(); ++k) {
    KLFLibResourceEngine * engine = pLibViews[k]->resourceEngine();
    delete pLibViews[k];
    delete engine;
  }
  delete pUi;
}


// static
QString KLFLibBrowser::urlToResourceTitle(const QUrl& url)
{
  // ........ TODO ........
  return url.toString();
}

KLFAbstractLibView * KLFLibBrowser::findOpenUrl(const QUrl& url)
{
  int k;
  for (k = 0; k < pLibViews.size(); ++k)
    if (pLibViews[k]->resourceEngine()->url() == url)
      return pLibViews[k];
  return NULL;
}

bool KLFLibBrowser::openResource(const QUrl& url)
{
  if (findOpenUrl(url) != NULL)
    return false;

  KLFAbstractLibEngineFactory * factory = KLFAbstractLibEngineFactory::findFactoryFor(url.scheme());
  if ( factory == NULL )
    return false;
  KLFLibResourceEngine * resource = factory->openResource(url, this);
  if ( resource == NULL )
    return false;

  // now create appropriate view for this resource
  KLFAbstractLibViewFactory *viewfactory =
    KLFAbstractLibViewFactory::findFactoryFor(resource->suggestedViewTypeIdentifier());
  KLFAbstractLibView * view = viewfactory->createLibView(pUi->tabResources, resource);

  // get informed about selection changes
  connect(view, SIGNAL(entriesSelected(const QList<KLFLibEntry>& )),
	  this, SLOT(slotEntriesSelected(const QList<KLFLibEntry>& )));

  pUi->tabResources->addTab(view, urlToResourceTitle(url));
  pLibViews.append(view);
  setStyleSheet(styleSheet());
  return true;
}
bool KLFLibBrowser::closeResource(const QUrl& url)
{
  KLFAbstractLibView * w = findOpenUrl(url);
  if (w == NULL)
    return false;

  int tabindex = pUi->tabResources->indexOf(w);
  if (tabindex < 0) {
    qWarning("What's going on? KLFLibBrowser::closeResource(url): found view that's not in tab?!?\n"
	     "\turl=%s, viewwidget=%p", qPrintable(url.toString()), w);
    return false;
  }
  pUi->tabResources->removeTab(tabindex);
  pLibViews.removeAll(w);
  delete w;
  return true;
}


void KLFLibBrowser::slotEntriesSelected(const QList<KLFLibEntry>& entries)
{
  if (sender() != pUi->tabResources->currentWidget())
    return;

  pUi->wEntryEditor->displayEntries(entries);
}



