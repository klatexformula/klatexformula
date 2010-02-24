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

#include <QDebug>
#include <QMenu>
#include <QAction>
#include <QEvent>
#include <QKeyEvent>
#include <QShortcut>
#include <QMessageBox>

#include "klfconfig.h"
#include "klflibbrowser_p.h"
#include <ui_klflibbrowser.h>
#include "klflibbrowser.h"




KLFLibBrowser::KLFLibBrowser(QWidget *parent)
  : QWidget(parent)
{
  pUi = new Ui::KLFLibBrowser;
  pUi->setupUi(this);
  pUi->tabResources->clear();
  pUi->tabResources->setContextMenuPolicy(Qt::CustomContextMenu);

  pResourceMenu = new QMenu(pUi->tabResources);
  pARename = pResourceMenu->addAction(tr("Rename"), this, SLOT(slotResourceRename()));
  pAClose = pResourceMenu->addAction(tr("Close"), this, SLOT(slotResourceClose()));
  pAProperties = pResourceMenu->addAction(tr("Properties..."),
					  this, SLOT(slotResourceProperties()));
  pASaveAs = pResourceMenu->addAction(tr("Save As..."), this, SLOT(slotResourceSaveAs()));
  pResourceMenu->addSeparator();
  pANew = pResourceMenu->addAction(tr("New..."), this, SLOT(slotResourceNew()));
  pAOpen = pResourceMenu->addAction(tr("Open..."), this, SLOT(slotResourceOpen()));

  slotRefreshResourceActionsEnabled();

  QPushButton *tabCornerButton = new QPushButton(tr("Resource"), pUi->tabResources);
  tabCornerButton->setMenu(pResourceMenu);
  pUi->tabResources->setCornerWidget(tabCornerButton);

  connect(pUi->tabResources, SIGNAL(currentChanged(int)), this, SLOT(slotTabResourceShown(int)));
  connect(pUi->tabResources, SIGNAL(customContextMenuRequested(const QPoint&)),
	  this, SLOT(slotShowTabContextMenu(const QPoint&)));


  // RESTORE

  QMenu * restoreMenu = new QMenu(this);
  restoreMenu->addAction(QIcon(":/pics/restoreall.png"), tr("Restore Formula with Style"),
			 this, SLOT(slotRestoreWithStyle()));
  restoreMenu->addAction(QIcon(":/pics/restore.png"), tr("Restore Formula Only"),
			 this, SLOT(slotRestoreLatexOnly()));
  pUi->btnRestore->setMenu(restoreMenu);
  connect(pUi->btnRestore, SIGNAL(clicked()), this, SLOT(slotRestoreWithStyle()));

  connect(pUi->btnDelete, SIGNAL(clicked()), this, SLOT(slotDeleteSelected()));

  // CATEGORY/TAGS

  connect(pUi->wEntryEditor, SIGNAL(categoryChanged(const QString&)),
	  this, SLOT(slotCategoryChanged(const QString&)));
  connect(pUi->wEntryEditor, SIGNAL(tagsChanged(const QString&)),
	  this, SLOT(slotTagsChanged(const QString&)));

  connect(pUi->wEntryEditor, SIGNAL(restoreStyle(const KLFStyle&)),
	  this, SIGNAL(requestRestoreStyle(const KLFStyle&)));

  // SEARCH

  pUi->txtSearch->installEventFilter(this);
  connect(pUi->txtSearch, SIGNAL(textChanged(const QString&)),
	  this, SLOT(slotSearchFind(const QString&)));
  connect(pUi->btnFindNext, SIGNAL(clicked()), this, SLOT(slotSearchFindNext()));
  connect(pUi->btnFindPrev, SIGNAL(clicked()), this, SLOT(slotSearchFindPrev()));

  QPalette defaultpal = pUi->txtSearch->palette();
  pUi->txtSearch->setProperty("defaultPalette", QVariant::fromValue<QPalette>(defaultpal));
  QPalette pal0 = pUi->txtSearch->palette();
  pal0.setColor(QPalette::Text, QColor(180,180,180));
  pal0.setColor(QPalette::WindowText, QColor(180,180,180));
  pal0.setColor(pUi->txtSearch->foregroundRole(), QColor(180,180,180));
  pUi->txtSearch->setProperty("paletteFocusOut", QVariant::fromValue<QPalette>(pal0));
  QPalette pal1 = pUi->txtSearch->palette();
  pal1.setColor(QPalette::Base, klfconfig.LibraryBrowser.colorFound);
  pal1.setColor(QPalette::Window, klfconfig.LibraryBrowser.colorFound);
  pal1.setColor(pUi->txtSearch->backgroundRole(), klfconfig.LibraryBrowser.colorFound);
  pUi->txtSearch->setProperty("paletteFound", QVariant::fromValue<QPalette>(pal1));
  QPalette pal2 = pUi->txtSearch->palette();
  pal2.setColor(QPalette::Base, klfconfig.LibraryBrowser.colorNotFound);
  pal2.setColor(QPalette::Window, klfconfig.LibraryBrowser.colorNotFound);
  pal2.setColor(pUi->txtSearch->backgroundRole(), klfconfig.LibraryBrowser.colorNotFound);
  pUi->txtSearch->setProperty("paletteNotFound", QVariant::fromValue<QPalette>(pal2));

  // SHORTCUTS
  // restore
  (void)new QShortcut(QKeySequence(tr("Ctrl+Enter", "[[restore]]")), this, SLOT(slotRestoreWithStyle()));
  (void)new QShortcut(QKeySequence(tr("Shift+Enter", "[[restore]]")), this, SLOT(slotRestoreLatexOnly()));
  // search-related
  (void)new QShortcut(QKeySequence(tr("Ctrl+F", "[[find]]")), this, SLOT(slotSearchClearOrNext()));
  (void)new QShortcut(QKeySequence(tr("Ctrl+S", "[[find]]")), this, SLOT(slotSearchClearOrNext()));
  (void)new QShortcut(QKeySequence(tr("/", "[[find]]")), this, SLOT(slotSearchClear()));
  (void)new QShortcut(QKeySequence(tr("F3", "[[find next]]")), this, SLOT(slotSearchFindNext()));
  (void)new QShortcut(QKeySequence(tr("Shift+F3", "[[find prev]]")), this, SLOT(slotSearchFindPrev()));
  (void)new QShortcut(QKeySequence(tr("Ctrl+R", "[[find]]")), this, SLOT(slotSearchFindPrev()));
  // Esc will be captured through event filter so that it isn't too ostrusive...
  //  (void)new QShortcut(QKeySequence(tr("Esc", "[[abort find]]")), this, SLOT(slotSearchAbort()));

  // start with no entries selected
  slotEntriesSelected(QList<KLFLibEntry>());
  // and set search bar's focus-out palette
  slotSearchFocusOut();
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


bool KLFLibBrowser::eventFilter(QObject *obj, QEvent *ev)
{
  if (obj == pUi->txtSearch) {
    if (ev->type() == QEvent::FocusIn) {
      slotSearchFocusIn();
      // don't eat event
    } else if (ev->type() == QEvent::FocusOut) {
      slotSearchAbort();
      slotSearchFocusOut();
      // don't eat event
    } else if (ev->type() == QEvent::KeyPress) {
      QKeyEvent *ke = (QKeyEvent*)ev;
      if (ke->key() == Qt::Key_Escape) {
	slotSearchAbort();
      }
    }
  }
  if (obj->property("resourceTitleEditor").toBool() == true) {
    if (ev->type() == QEvent::FocusOut) {
      obj->deleteLater(); // if lost focus, cancel...
      // don't eat event
    }
    if (ev->type() == QEvent::KeyPress) {
      QKeyEvent *ke = (QKeyEvent*)ev;
      if (ke->key() == Qt::Key_Escape) {
	obj->deleteLater();
	return true;
      }
    }
  }
  return QWidget::eventFilter(obj, ev);
}


QList<QUrl> KLFLibBrowser::openUrls() const
{
  QList<QUrl> urls;
  int k;
  for (k = 0; k < pLibViews.size(); ++k)
    urls << pLibViews[k]->resourceEngine()->url();

  return urls;
}

KLFLibResourceEngine * KLFLibBrowser::getOpenResource(const QUrl& url)
{
  KLFAbstractLibView * view = findOpenUrl(url);
  if (view == NULL)
    return NULL;
  return view->resourceEngine();
}


KLFAbstractLibView * KLFLibBrowser::findOpenUrl(const QUrl& url)
{
  int k;
  for (k = 0; k < pLibViews.size(); ++k)
    // don't compare the urls with removed query items because query items may be
    // important as to which part of the resource is displayed (for ex.)
    if (pLibViews[k]->resourceEngine()->url() == url)
      return pLibViews[k];
  return NULL;
}
KLFAbstractLibView * KLFLibBrowser::curView()
{
  return qobject_cast<KLFAbstractLibView*>(pUi->tabResources->currentWidget());
}
KLFAbstractLibView * KLFLibBrowser::viewForTabIndex(int tab)
{
  return qobject_cast<KLFAbstractLibView*>(pUi->tabResources->widget(tab));
}


bool KLFLibBrowser::openResource(const QUrl& url, uint resourceRoleFlags)
{
  KLFAbstractLibView * openview = findOpenUrl(url);
  if (openview != NULL) {
    qDebug("This resource is already open.");
    pUi->tabResources->setCurrentWidget(openview);
    updateResourceRoleFlags(openview, resourceRoleFlags);
    return true;
  }

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
  connect(view, SIGNAL(entriesSelected(const KLFLibEntryList& )),
	  this, SLOT(slotEntriesSelected(const KLFLibEntryList& )));
  // and of new category suggestions
  connect(view, SIGNAL(moreCategorySuggestions(const QStringList&)),
	  this, SLOT(slotAddCategorySuggestions(const QStringList&)));

  connect(view, SIGNAL(requestRestore(const KLFLibEntry&, uint)),
	  this, SIGNAL(requestRestore(const KLFLibEntry&, uint)));
  connect(view, SIGNAL(requestRestoreStyle(const KLFStyle&)),
	  this, SIGNAL(requestRestoreStyle(const KLFStyle&)));

  connect(resource, SIGNAL(resourcePropertyChanged(int)),
	  this, SLOT(slotResourcePropertyChanged(int)));

  // get more category completions
  view->wantMoreCategorySuggestions();

  // supply a context menu to view
  view->setContextMenuPolicy(Qt::CustomContextMenu);
  connect(view, SIGNAL(customContextMenuRequested(const QPoint&)),
	  this, SLOT(slotShowContextMenu(const QPoint&)));

  pUi->tabResources->addTab(view, resource->title());
  pUi->tabResources->setCurrentWidget(view);
  pLibViews.append(view);
  setStyleSheet(styleSheet());
  updateResourceRoleFlags(view, resourceRoleFlags);
  return true;
}
bool KLFLibBrowser::closeResource(const QUrl& url)
{
  KLFAbstractLibView * w = findOpenUrl(url);
  if (w == NULL)
    return false;

  return slotResourceClose(w);
}


void KLFLibBrowser::updateResourceRoleFlags(KLFAbstractLibView *view, uint resroleflags)
{
  view->setProperty("resourceRoleFlags", resroleflags);
}


void KLFLibBrowser::slotTabResourceShown(int /*tabIndex*/)
{
  KLFAbstractLibView * view = curView();
  if (view == NULL)
    return;

  // refresh selection-related displays
  slotEntriesSelected(view->selectedEntries());
  slotRefreshResourceActionsEnabled();
}

void KLFLibBrowser::slotShowTabContextMenu(const QPoint& pos)
{
  int tab = pUi->tabResources->getTabAtPoint(pos);
  if (tab != -1) {
    pUi->tabResources->setCurrentIndex(tab);
  }
  pResourceMenu->popup(pUi->tabResources->mapToGlobal(pos));
}

void KLFLibBrowser::slotResourceRename()
{
  int tab = pUi->tabResources->currentIndex();
  KLFAbstractLibView * view = curView();
  if (tab < 0 || view == NULL)
    return;

  qDebug()<<"Rename!";

  if ( ! view->resourceEngine()->canModify() )
    return;

  QLineEdit * editor = new QLineEdit(pUi->tabResources);
  editor->setGeometry(pUi->tabResources->getTabRect(tab));
  editor->show();
  editor->setText(view->resourceEngine()->title());
  editor->setFocus();
  editor->setProperty("tabURL", viewForTabIndex(tab)->resourceEngine()->url());
  editor->setProperty("resourceTitleEditor", true);
  editor->installEventFilter(this);

  // kill editor if tab changes
  connect(pUi->tabResources, SIGNAL(currentChanged(int)), editor, SLOT(deleteLater()));
  connect(editor, SIGNAL(returnPressed()), this, SLOT(slotResourceRenameFinished()));

}

void KLFLibBrowser::slotResourceRenameFinished()
{
  QObject * editor = sender();
  if (editor == NULL) {
    qWarning("KLFLibBrowser::slotResourceRenameFinished: no sender!");
  }
  QUrl url = editor->property("tabURL").toUrl();
  KLFAbstractLibView * view = findOpenUrl(url);
  view->resourceEngine()->setTitle(editor->property("text").toString());

  editor->deleteLater();
}

bool KLFLibBrowser::slotResourceClose(KLFAbstractLibView *view)
{
  if (view == NULL)
    view = curView();
  if (view == NULL)
    return false;

  qDebug()<<"Close!";

  int tabindex = pUi->tabResources->indexOf(view);
  if (tabindex < 0) {
    qWarning("KLFLibBrowser::closeResource(url): can't find view in tab widget?!?\n"
	     "\turl=%s, viewwidget=%p", qPrintable(view->resourceEngine()->url().toString()), view);
    return false;
  }
  pUi->tabResources->removeTab(tabindex);
  int index = pLibViews.indexOf(view);
  pLibViews.removeAt(index);
  // delete view and engine
  KLFLibResourceEngine *resource = view->resourceEngine();
  delete view;
  delete resource;

  return true;
}
void KLFLibBrowser::slotResourceProperties()
{
  KLFAbstractLibView *view = curView();
  if (view == NULL) {
    qWarning("KLFLibBrowser::slotResourceProperties: NULL View!");
    return;
  }
  KLFLibResPropEditorDlg * dialog = new KLFLibResPropEditorDlg(view->resourceEngine(), this);
  dialog->show();
}

bool KLFLibBrowser::slotResourceOpen()
{
  QUrl url = KLFLibOpenResourceDlg::queryOpenResource(QUrl(), this);
  if (url.isEmpty())
    return false;
  bool r = openResource(url);
  if ( ! r ) {
    QMessageBox::critical(this, tr("Error"), tr("Failed to open library resource `%1'!")
			  .arg(url.toString()));
  }
  return r;
}

bool KLFLibBrowser::slotResourceNew()
{
  
  return false;
}

bool KLFLibBrowser::slotResourceSaveAs()
{
  return false;
}


void KLFLibBrowser::slotResourcePropertyChanged(int propId)
{
  KLFLibResourceEngine *resource = qobject_cast<KLFLibResourceEngine*>(sender());
  if (resource == NULL) {
    qWarning("KLFLibBrowser::slotResourcePropertyChanged: NULL sender or not resource!");
    return;
  }

  slotRefreshResourceActionsEnabled();

  if (propId == KLFLibResourceEngine::PropTitle) {
    KLFAbstractLibView *view = findOpenUrl(resource->url());
    if (view == NULL) {
      qWarning()<<"KLFLibBrowser::slotResourcePropertyChanged: can't find view for url "
		<<resource->url()<<"!";
      return;
    }
    pUi->tabResources->setTabText(pUi->tabResources->indexOf(view), resource->title());
  }
}

void KLFLibBrowser::slotRestoreWithStyle()
{
  KLFAbstractLibView * view = curView();
  if ( view == NULL )
    return;
  view->restoreWithStyle();
}

void KLFLibBrowser::slotRestoreLatexOnly()
{
  KLFAbstractLibView * view = curView();
  if ( view == NULL )
    return;
  view->restoreLatexOnly();
}

void KLFLibBrowser::slotDeleteSelected()
{
  KLFAbstractLibView * view = curView();
  if ( view == NULL )
    return;
  view->deleteSelected();
}

void KLFLibBrowser::slotRefreshResourceActionsEnabled()
{
  bool master = false;
  bool canmodify = false;
  bool cansaveas = false;
  uint resrolefl = 0;

  KLFAbstractLibView * view = curView();
  if ( view != NULL ) {
    master = true;
    canmodify = view->resourceEngine()->canModify();
    cansaveas = (view->resourceEngine()->supportedFeatureFlags() & KLFLibResourceEngine::FeatureSaveAs);
    resrolefl = view->property("resourceRoleFlags").toUInt();
  }

  pARename->setEnabled(canmodify);
  pAClose->setEnabled(master && !(resrolefl & NoCloseRoleFlag));
  pAProperties->setEnabled(master);
  pASaveAs->setEnabled(master && cansaveas);
  pANew->setEnabled(true);
  pAOpen->setEnabled(true);
}



void KLFLibBrowser::slotEntriesSelected(const KLFLibEntryList& entries)
{
  if (sender() != pUi->tabResources->currentWidget())
    return;

  pUi->wEntryEditor->displayEntries(entries);

  pUi->btnDelete->setEnabled(entries.size() > 0);
  pUi->btnRestore->setEnabled(entries.size() == 1);
}
void KLFLibBrowser::slotAddCategorySuggestions(const QStringList& catlist)
{
  pUi->wEntryEditor->addCategorySuggestions(catlist);
}

void KLFLibBrowser::slotShowContextMenu(const QPoint& pos)
{
  KLFAbstractLibView *view = curView();
  if (view == NULL)
    return;

  QMenu *menu = new QMenu(view);

  QAction *a1 = menu->addAction(QIcon(":/pics/restoreall.png"), tr("Restore latex formula and style"),
				view, SLOT(restoreWithStyle()));
  QAction *a2 = menu->addAction(QIcon(":/pics/restore.png"), tr("Restore latex formula only"),
				view, SLOT(restoreLatexOnly()));
  menu->addSeparator();
  QAction *adel = menu->addAction(QIcon(":/pics/delete.png"), tr("Delete from library"),
				  view, SLOT(deleteSelected()));
  menu->addSeparator();

  QMenu *copytomenu = new QMenu;
  QMenu *movetomenu = new QMenu;
  int k;
  QAction *acopy, *amove;
  for (k = 0; k < pLibViews.size(); ++k) {
    if (pLibViews[k] == view) // skip this view
      continue;
    KLFLibResourceEngine *res = pLibViews[k]->resourceEngine();
    acopy = copytomenu->addAction(res->title(), this, SLOT(slotCopyToResource()));
    acopy->setProperty("resourceUrl", res->url());
    amove = movetomenu->addAction(res->title(), this, SLOT(slotMoveToResource()));
    amove->setProperty("resourceUrl", res->url());
    if (!res->canModify()) {
      acopy->setEnabled(false);
      amove->setEnabled(false);
    }
  }
  QAction *acopyto = menu->addMenu(copytomenu);
  acopyto->setText(tr("Copy to"));
  acopyto->setIcon(QIcon(":/pics/copy.png"));
  QAction *amoveto = menu->addMenu(movetomenu);
  amoveto->setText(tr("Move to"));
  amoveto->setIcon(QIcon(":/pics/move.png"));

  // Needed for when user pops up a menu without selection (ie. short list, free white space under)
  KLFLibEntryList selected = view->selectedEntries();
  bool cancopy = (selected.size() > 0);
  bool canre = (selected.size() == 1);
  bool canmod = view->resourceEngine()->canModify();
  a1->setEnabled(canre);
  a2->setEnabled(canre);
  adel->setEnabled(canmod && selected.size());
  acopyto->setEnabled(cancopy);
  amoveto->setEnabled(cancopy && canmod);

  menu->popup(view->mapToGlobal(pos));
}



void KLFLibBrowser::slotCategoryChanged(const QString& newcategory)
{
  QWidget *w = pUi->tabResources->currentWidget();
  KLFAbstractLibView *wview = qobject_cast<KLFAbstractLibView*>(w);
  if (wview == NULL) {
    qWarning("Current view is not a KLFAbstractLibView!");
    return;
  }
  bool r = wview->writeEntryProperty(KLFLibEntry::Category, newcategory);
  if ( ! r )
    QMessageBox::warning(this, tr("Error"),
			 tr("Failed to write category information!"));
}
void KLFLibBrowser::slotTagsChanged(const QString& newtags)
{
  QWidget *w = pUi->tabResources->currentWidget();
  KLFAbstractLibView *wview = qobject_cast<KLFAbstractLibView*>(w);
  if (wview == NULL) {
    qWarning("Current view is not a KLFAbstractLibView!");
    return;
  }
  bool r = wview->writeEntryProperty(KLFLibEntry::Tags, newtags);
  if ( ! r )
    QMessageBox::warning(this, tr("Error"),
			 tr("Failed to write tags information!"));
}


void KLFLibBrowser::slotSearchClear()
{
  if (QApplication::focusWidget() == pUi->txtSearch) {
    // already has focus: means that we want to recall history search
    pUi->txtSearch->setText(pLastSearchText);
  } else {
    pUi->txtSearch->setText("");
    pUi->txtSearch->setFocus();
  }
}
void KLFLibBrowser::slotSearchClearOrNext()
{
  if (QApplication::focusWidget() == pUi->txtSearch) {
    // already has focus
    // -> either recall history (if empty search text)
    // -> or find next
    if (pUi->txtSearch->text().isEmpty()) {
      pUi->txtSearch->setText(pLastSearchText);
    } else {
      if (pSearchText.isEmpty())
	slotSearchFind(pUi->txtSearch->text());
      else
	slotSearchFindNext();
    }
  } else {
    pUi->txtSearch->setText("");
    pUi->txtSearch->setFocus();
  }
}

void KLFLibBrowser::slotSearchFind(const QString& text, bool forward)
{
  KLFAbstractLibView * view = curView();
  if ( ! view ) {
    qWarning("KLFLibBrowser: No Current View!");
    return;
  }
  if (text.isEmpty()) {
    slotSearchAbort();
    return;
  }
  bool found = view->searchFind(text, forward);
  updateSearchFound(found);
  pSearchText = text;
}

void KLFLibBrowser::slotSearchFindNext(bool forward)
{
  KLFAbstractLibView * view = curView();
  if ( ! view ) {
    qWarning("KLFLibBrowser: No Current View!");
    return;
  }
  // focus search bar if not yet focused.
  if (QApplication::focusWidget() != pUi->txtSearch)
    pUi->txtSearch->setFocus();

  if (pSearchText.isEmpty()) {
    // we're not in search mode
    if (pUi->txtSearch->text().isEmpty()) {
      // recall history
      pUi->txtSearch->blockSignals(true); // stop txtSearch from calling our slotSearchFind()
      pUi->txtSearch->setText(pLastSearchText);
      pUi->txtSearch->blockSignals(false);
    }
    // and initiate search mode
    slotSearchFind(pUi->txtSearch->text(), forward);
    return;
  }
  bool found = view->searchFindNext(forward);
  updateSearchFound(found);
  pLastSearchText = pSearchText;
}

void KLFLibBrowser::slotSearchFindPrev()
{
  slotSearchFindNext(false);
}

void KLFLibBrowser::slotSearchAbort()
{
  qDebug("Search abort..");
  pSearchText = QString();
  if ( ! pUi->txtSearch->text().isEmpty() ) {
    pUi->txtSearch->setText("");
    // return because we will automatically be re-called because txtSearch will re-emit textChanged()
    // which in turn calls slotSearchFind() with empty text.
    return;
  }

  KLFAbstractLibView * view = curView();
  if ( ! view ) {
    qWarning("KLFLibBrowser: No Current View!");
    return;
  }
  view->searchAbort();

  pUi->txtSearch->setProperty("searchState", QString("aborted"));
  pUi->txtSearch->setStyleSheet(pUi->txtSearch->styleSheet());
  QPalette pal = pUi->txtSearch->property("defaultPalette").value<QPalette>();
  pUi->txtSearch->setPalette(pal);
}

void KLFLibBrowser::slotCopyToResource()
{
  slotCopyMoveToResource(sender(), false);
}

void KLFLibBrowser::slotMoveToResource()
{
  slotCopyMoveToResource(sender(), true);
}

void KLFLibBrowser::slotCopyMoveToResource(QObject *action, bool move)
{
  QUrl destUrl = action->property("resourceUrl").toUrl();
  if (destUrl.isEmpty()) {
    qWarning()<<"KLFLibBrowser::slotCopyMoveToResource(): bad sender property ! sender is a `"
	      <<action->metaObject()->className()<<"'; expected QAction with 'resourceUrl' property set.";
    return;
  }
  KLFAbstractLibView *sourceView = curView();
  if (sourceView == NULL) {
    qWarning()<<"KLFLibBrowser::slotCopyMoveToResource(): source view is NULL!";
    return;
  }
  KLFAbstractLibView *destView = findOpenUrl(destUrl);
  if (destView == NULL) {
    qWarning()<<"KLFLibBrowser::slotCopyMoveToResource(): can't find dest view url for URL="<<destUrl<<" !";
    return;
  }
  // now do the copy/move:
  slotCopyMoveToResource(destView, sourceView, move);
}

void KLFLibBrowser::slotCopyMoveToResource(KLFAbstractLibView *dest, KLFAbstractLibView *source,
					   bool move)
{
  KLFLibEntryList items = source->selectedEntries();

  bool result = dest->insertEntries(items);
  if ( ! result ) {
    QString msg = move ? tr("Failed to move the selected items.")
      : tr("Failed to copy the selected items.");
    QMessageBox::critical(this, tr("Error"), msg, QMessageBox::Ok, QMessageBox::Ok);
    return;
  }
  if (move)
    source->deleteSelected(false);
}



void KLFLibBrowser::updateSearchFound(bool found)
{
  qDebug("updateSearchFound(%d)", found);
  QPalette pal;
  if (found) {
    pal = pUi->txtSearch->property("paletteFound").value<QPalette>();
    pUi->txtSearch->setProperty("searchState", QString("found"));
  } else {
    pal = pUi->txtSearch->property("paletteNotFound").value<QPalette>();
    pUi->txtSearch->setProperty("searchState", QString("not-found"));
  }
  pUi->txtSearch->setStyleSheet(pUi->txtSearch->styleSheet());
  pUi->txtSearch->setPalette(pal);
}

void KLFLibBrowser::slotSearchFocusIn()
{
  qDebug("Focus in...");
  pUi->txtSearch->setProperty("searchState", QString("default"));
  pUi->txtSearch->setPalette(pUi->txtSearch->property("defaultPalette").value<QPalette>());
  pUi->txtSearch->blockSignals(true);
  pUi->txtSearch->setText("");
  pUi->txtSearch->blockSignals(false);
}
void KLFLibBrowser::slotSearchFocusOut()
{
  qDebug("Focus out...");
  pUi->txtSearch->setProperty("searchState", QString("focus-out"));
  pUi->txtSearch->setStyleSheet(pUi->txtSearch->styleSheet());
  pUi->txtSearch->setPalette(pUi->txtSearch->property("paletteFocusOut").value<QPalette>());
  pUi->txtSearch->blockSignals(true);
  pUi->txtSearch->setText("  "+tr("Hit Ctrl-F, Ctrl-S or / to search within the current resource"));
  pUi->txtSearch->blockSignals(false);
}