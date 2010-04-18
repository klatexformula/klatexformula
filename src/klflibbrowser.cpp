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
#include <QFile>
#include <QMenu>
#include <QAction>
#include <QEvent>
#include <QKeyEvent>
#include <QShortcut>
#include <QMessageBox>
#include <QSignalMapper>

#include "klfconfig.h"
#include "klflibbrowser_p.h"
#include <ui_klflibbrowser.h>
#include "klflibbrowser.h"



KLFLibBrowser::KLFLibBrowser(QWidget *parent)
  : QWidget(
#if defined(Q_OS_WIN32)
	    0 /* parent */
#else
	    parent /* 0 */
#endif
	    , Qt::Window)
{
  u = new Ui::KLFLibBrowser;
  u->setupUi(this);
  u->tabResources->setContextMenuPolicy(Qt::CustomContextMenu);

  pResourceMenu = new QMenu(u->tabResources);
  pResourceMenu->setTitle(tr("Resource Actions"));
  pARename = pResourceMenu->addAction(tr("Rename"), this, SLOT(slotResourceRename()));
  pAProperties = pResourceMenu->addAction(tr("Properties..."),
					  this, SLOT(slotResourceProperties()));
  pASaveAs = pResourceMenu->addAction(tr("Save As Copy..."), this, SLOT(slotResourceSaveAs()));
  pAViewType = pResourceMenu->addAction(tr("View Type"));
  pResourceMenu->addSeparator();
  pANew = pResourceMenu->addAction(tr("New..."), this, SLOT(slotResourceNew()));
  pAOpen = pResourceMenu->addAction(tr("Open..."), this, SLOT(slotResourceOpen()));
  pAClose = pResourceMenu->addAction(tr("Close"), this, SLOT(slotResourceClose()));
  

  slotRefreshResourceActionsEnabled();

  QPushButton *tabCornerButton = new QPushButton(tr("Resource"), u->tabResources);
  tabCornerButton->setMenu(pResourceMenu);
  u->tabResources->setCornerWidget(tabCornerButton);

  connect(u->tabResources, SIGNAL(currentChanged(int)), this, SLOT(slotTabResourceShown(int)));
  connect(u->tabResources, SIGNAL(customContextMenuRequested(const QPoint&)),
	  this, SLOT(slotShowTabContextMenu(const QPoint&)));


  // RESTORE

  QMenu * restoreMenu = new QMenu(this);
  restoreMenu->addAction(QIcon(":/pics/restoreall.png"), tr("Restore Formula with Style"),
			 this, SLOT(slotRestoreWithStyle()));
  restoreMenu->addAction(QIcon(":/pics/restore.png"), tr("Restore Formula Only"),
			 this, SLOT(slotRestoreLatexOnly()));
  u->btnRestore->setMenu(restoreMenu);
  connect(u->btnRestore, SIGNAL(clicked()), this, SLOT(slotRestoreWithStyle()));

  connect(u->btnDelete, SIGNAL(clicked()), this, SLOT(slotDeleteSelected()));

  // CATEGORY/TAGS

  connect(u->wEntryEditor, SIGNAL(categoryChanged(const QString&)),
	  this, SLOT(slotCategoryChanged(const QString&)));
  connect(u->wEntryEditor, SIGNAL(tagsChanged(const QString&)),
	  this, SLOT(slotTagsChanged(const QString&)));

  connect(u->wEntryEditor, SIGNAL(restoreStyle(const KLFStyle&)),
	  this, SIGNAL(requestRestoreStyle(const KLFStyle&)));

  // OPEN / NEW BUTTONS IN WELCOME TAB

  connect(u->btnOpenRes, SIGNAL(clicked()), this, SLOT(slotResourceOpen()));
  connect(u->btnCreateRes, SIGNAL(clicked()), this, SLOT(slotResourceNew()));

  // SEARCH

  u->txtSearch->installEventFilter(this);
  connect(u->btnSearchClear, SIGNAL(clicked()), this, SLOT(slotSearchClear()));
  connect(u->txtSearch, SIGNAL(textChanged(const QString&)),
	  this, SLOT(slotSearchFind(const QString&)));
  connect(u->btnFindNext, SIGNAL(clicked()), this, SLOT(slotSearchFindNext()));
  connect(u->btnFindPrev, SIGNAL(clicked()), this, SLOT(slotSearchFindPrev()));

  QPalette defaultpal = u->txtSearch->palette();
  u->txtSearch->setProperty("defaultPalette", QVariant::fromValue<QPalette>(defaultpal));
  QPalette pal0 = u->txtSearch->palette();
  pal0.setColor(QPalette::Text, QColor(180,180,180));
  pal0.setColor(QPalette::WindowText, QColor(180,180,180));
  pal0.setColor(u->txtSearch->foregroundRole(), QColor(180,180,180));
  u->txtSearch->setProperty("paletteFocusOut", QVariant::fromValue<QPalette>(pal0));
  QPalette pal1 = u->txtSearch->palette();
  pal1.setColor(QPalette::Base, klfconfig.LibraryBrowser.colorFound);
  pal1.setColor(QPalette::Window, klfconfig.LibraryBrowser.colorFound);
  pal1.setColor(u->txtSearch->backgroundRole(), klfconfig.LibraryBrowser.colorFound);
  u->txtSearch->setProperty("paletteFound", QVariant::fromValue<QPalette>(pal1));
  QPalette pal2 = u->txtSearch->palette();
  pal2.setColor(QPalette::Base, klfconfig.LibraryBrowser.colorNotFound);
  pal2.setColor(QPalette::Window, klfconfig.LibraryBrowser.colorNotFound);
  pal2.setColor(u->txtSearch->backgroundRole(), klfconfig.LibraryBrowser.colorNotFound);
  u->txtSearch->setProperty("paletteNotFound", QVariant::fromValue<QPalette>(pal2));

  // SHORTCUTS
  // restore & delete
  (void)new QShortcut(QKeySequence(tr("Ctrl+Enter", "[[shortcut: restore]]")),
		      this, SLOT(slotRestoreWithStyle()));
  (void)new QShortcut(QKeySequence(tr("Shift+Enter", "[[shortcut: restore]]")),
		      this, SLOT(slotRestoreLatexOnly()));
  QShortcut *s = new QShortcut(QKeySequence(tr("Delete", "[[shortcut: delete item from library]]")),
			       u->tabResources);
  connect(s, SIGNAL(activated()), this, SLOT(slotDeleteSelected()));
		      
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
  /// \bug ........... DEBUG/TODO ........... REMOVE THIS BEFORE RELEASE VERSION ................

  // save state to local file
  qDebug()<<"LibBrowser: Saving GUI state !";
  QVariantMap vm = saveGuiState();
  QFile f("/home/philippe/temp/klf_saved_libbrowser_state");
  f.open(QIODevice::WriteOnly);
  QDataStream str(&f);
  str << vm;

  int k;
  for (k = 0; k < pLibViews.size(); ++k) {
    KLFLibResourceEngine * engine = pLibViews[k]->resourceEngine();
    delete pLibViews[k];
    delete engine;
  }
  delete u;
}


bool KLFLibBrowser::eventFilter(QObject *obj, QEvent *ev)
{
  if (obj == u->txtSearch) {
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

int KLFLibBrowser::currentUrlIndex()
{
  KLFLibBrowserViewContainer *viewc = curView();
  if (viewc == NULL)
    return -1;
  return pLibViews.indexOf(viewc);
}

QUrl KLFLibBrowser::currentUrl()
{
  int i = currentUrlIndex();
  if (i < 0)
    return QUrl();
  return pLibViews[i]->url();
}


QList<QUrl> KLFLibBrowser::openUrls() const
{
  QList<QUrl> urls;
  int k;
  for (k = 0; k < pLibViews.size(); ++k)
    urls << pLibViews[k]->url();

  return urls;
}

KLFLibResourceEngine * KLFLibBrowser::getOpenResource(const QUrl& url)
{
  KLFLibBrowserViewContainer * viewc = findOpenUrl(url);
  if (viewc == NULL)
    return NULL;
  return viewc->resourceEngine();
}


QVariantMap KLFLibBrowser::saveGuiState()
{
  QVariantMap v;
  // first save the list of open URLs
  QList<QUrl> myurllist = openUrls();
  QUrl currenturl = currentUrl();
  QList<QVariant> urllist; // will hold URL's as QUrl's
  QList<QVariant> viewstatelist; // will hold variantMap's
  QList<QVariant> resroleflagslist; // will hold quint32's

  int k;
  for (k = 0; k < myurllist.size(); ++k) {
    KLFLibBrowserViewContainer *viewc = findOpenUrl(myurllist[k]);
    if (viewc == NULL) {
      qWarning()<<"Should NOT HAPPEN! viewc is NULL in KLFLibBrowser::saveGuiState()! URL-List=\n"
		<<myurllist;
      continue;
    }
    QVariantMap viewState = viewc->saveGuiState();
    urllist << QVariant::fromValue<QUrl>(myurllist[k]);
    viewstatelist << QVariant::fromValue<QVariantMap>(viewState);
    resroleflagslist << QVariant::fromValue<quint32>(viewc->resourceRoleFlags());
  }
  v["UrlList"] = QVariant::fromValue<QVariantList>(urllist);
  v["ViewStateList"] = QVariant::fromValue<QVariantList>(viewstatelist);
  v["ResourceRoleFlagsList"] = QVariant::fromValue<QVariantList>(resroleflagslist);
  v["CurrentUrl"] = QVariant::fromValue<QUrl>(currenturl);
  v["WidgetSize"] = QVariant::fromValue<QSize>(size());
  return v;
}
void KLFLibBrowser::loadGuiState(const QVariantMap& v)
{
  QUrl currenturl = v["CurrentUrl"].toUrl();
  QList<QVariant> urllist = v["UrlList"].toList();
  QList<QVariant> viewstatelist = v["ViewStateList"].toList();
  QList<QVariant> resroleflagslist = v["ResourceRoleFlagsList"].toList();
  QSize widgetsize = v["WidgetSize"].value<QSize>();
  int k;
  for (k = 0; k < urllist.size(); ++k) {
    QUrl url = urllist[k].toUrl();
    quint32 flags = resroleflagslist[k].value<quint32>();
    qDebug()<<"LibBrowser::loadGuiState: Opening url "<<url<<" with flags="<<flags;
    QVariantMap viewState = viewstatelist[k].toMap();
    // open this URL
    bool res = openResource(url, flags);
    if ( ! res ) {
      qWarning()<<"KLFLibBrowser::loadGuiState: Can't open resource "<<url<<"! (flags="
		<<flags<<")";
      continue;
    }
    KLFLibBrowserViewContainer *viewc = findOpenUrl(url); // get the freshly opened view
    if (viewc == NULL) {
      qWarning()<<"Should NOT HAPPEN! viewc is NULL in KLFLibBrowser::loadGuiState()! urllist=\n"
		<<urllist<< "\n"<<"at k="<<k<<" in list: "<<url;
      continue;
    }
    // and load the view's gui state
    viewc->loadGuiState(viewState);
  }
  qDebug()<<"Almost finished loading gui state.";
  KLFLibBrowserViewContainer *curviewc = findOpenUrl(currenturl);
  if (curviewc != NULL)
    u->tabResources->setCurrentWidget(curviewc);
  qDebug()<<"Loaded GUI state.";

  if (widgetsize.width() > 0 && widgetsize.height() > 0)
    resize(widgetsize);
}


KLFLibBrowserViewContainer * KLFLibBrowser::findOpenUrl(const QUrl& url)
{
  int k;
  for (k = 0; k < pLibViews.size(); ++k)
    // don't compare the urls with removed query items because query items may be
    // important as to which part of the resource is displayed (for ex.)
    if (pLibViews[k]->url() == url)
      return pLibViews[k];
  return NULL;
}
KLFLibBrowserViewContainer * KLFLibBrowser::findOpenResource(KLFLibResourceEngine *resource)
{
  int k;
  for (k = 0; k < pLibViews.size(); ++k)
    if (pLibViews[k]->resourceEngine() == resource)
      return pLibViews[k];
  return NULL;
}

KLFLibBrowserViewContainer * KLFLibBrowser::curView()
{
  return qobject_cast<KLFLibBrowserViewContainer*>(u->tabResources->currentWidget());
}
KLFAbstractLibView * KLFLibBrowser::curLibView()
{
  KLFLibBrowserViewContainer *v = curView();
  if (v == NULL)
    return NULL;
  return v->view();
}
KLFLibBrowserViewContainer * KLFLibBrowser::viewForTabIndex(int tab)
{
  return qobject_cast<KLFLibBrowserViewContainer*>(u->tabResources->widget(tab));
}


bool KLFLibBrowser::openResource(const QUrl& url, uint resourceRoleFlags,
				 const QString& viewTypeIdentifier)
{
  KLFLibBrowserViewContainer * openview = findOpenUrl(url);
  if (openview != NULL) {
    qDebug("KLFLibBrowser::openResource(%s,%u): This resource is already open.",
	   qPrintable(url.toString()), resourceRoleFlags);
    u->tabResources->setCurrentWidget(openview);
    updateResourceRoleFlags(openview, resourceRoleFlags);
    return true;
  }

  KLFLibEngineFactory * factory = KLFLibEngineFactory::findFactoryFor(url.scheme());
  if ( factory == NULL )
    return false;
  KLFLibResourceEngine * resource = factory->openResource(url, this);
  if ( resource == NULL )
    return false;

  // go on opening resource with our sister function
  return openResource(resource, resourceRoleFlags, viewTypeIdentifier);
}
bool KLFLibBrowser::openResource(KLFLibResourceEngine *resource, uint resourceRoleFlags,
				 const QString& viewTypeIdentifier)
{
  if (resource == NULL) {
    qWarning("KLFLibBrowser::openResource(***NULL***,%u,%s) !", resourceRoleFlags,
	     qPrintable(viewTypeIdentifier));
    return false;
  }
  KLFLibBrowserViewContainer * openview = findOpenResource(resource);
  if (openview != NULL) {
    qDebug("KLFLibBrowser::openResource(%p,%u): This resource is already open.",
	   resource, resourceRoleFlags);
    u->tabResources->setCurrentWidget(openview);
    updateResourceRoleFlags(openview, resourceRoleFlags);
    return true;
  }

  resource->setParent(this);

  // now create appropriate view for this resource
  KLFLibBrowserViewContainer *viewc = new KLFLibBrowserViewContainer(resource, u->tabResources);

  QStringList viewtypeident_try;
  viewtypeident_try << viewTypeIdentifier
		    << resource->viewType()
		    << resource->suggestedViewTypeIdentifier()
		    << KLFLibViewFactory::defaultViewTypeIdentifier() ;
  int k;
  // try each view type, first success is kept.
  for (k = 0; k < viewtypeident_try.size(); ++k) {
    if (viewtypeident_try[k].isEmpty())
      continue;

    KLFLibViewFactory *viewfactory =
      KLFLibViewFactory::findFactoryFor(viewtypeident_try[k]);
    if (viewfactory == NULL) {
      qDebug()<<"KLFLibBrowser::openResource: can't find view factory for view type identifier "
	      <<viewtypeident_try[k]<<"!";
      continue;
    }
    if ( ! viewfactory->canCreateLibView(viewtypeident_try[k], resource) ) {
      qDebug()<<"KLFLibBrowser::openResource: incompatible view type identifier "<<viewtypeident_try[k]
	      <<"for resource "<<resource->url()<<".";
      continue;
    }
    bool r = viewc->openView(viewtypeident_try[k]);
    if (!r) {
      qDebug()<<"KLFLibBrowser::openResource: can't create view! viewtypeident="<<viewtypeident_try[k]<<".";
      continue;
    }
    // found good view type !
    resource->setViewType(viewtypeident_try[k]);
    // quit for() on first success.
    break;
  }

  // get informed about selection changes
  connect(viewc, SIGNAL(entriesSelected(const KLFLibEntryList& )),
	  this, SLOT(slotEntriesSelected(const KLFLibEntryList& )));
  // and of new category suggestions
  connect(viewc, SIGNAL(moreCategorySuggestions(const QStringList&)),
	  this, SLOT(slotAddCategorySuggestions(const QStringList&)));

  connect(viewc, SIGNAL(requestRestore(const KLFLibEntry&, uint)),
	  this, SIGNAL(requestRestore(const KLFLibEntry&, uint)));
  connect(viewc, SIGNAL(requestRestoreStyle(const KLFStyle&)),
	  this, SIGNAL(requestRestoreStyle(const KLFStyle&)));

  connect(viewc, SIGNAL(resourceDataChanged(const QList<KLFLib::entryId>&)),
	  this, SLOT(slotResourceDataChanged(const QList<KLFLib::entryId>&)));
  connect(resource, SIGNAL(resourcePropertyChanged(int)),
	  this, SLOT(slotResourcePropertyChanged(int)));

  // get more category completions
  viewc->view()->wantMoreCategorySuggestions();

  // supply a context menu to view
  connect(viewc, SIGNAL(viewContextMenuRequested(const QPoint&)),
	  this, SLOT(slotShowContextMenu(const QPoint&)));

  int i = u->tabResources->addTab(viewc, resource->title());
  u->tabResources->setCurrentWidget(viewc);
  u->tabResources
    ->refreshTabReadOnly(i, !resource->canModifyData(KLFLibResourceEngine::AllActionsData));
  pLibViews.append(viewc);
  setStyleSheet(styleSheet());
  updateResourceRoleFlags(viewc, resourceRoleFlags);

  // hide welcome page if it's shown
  if ((i = u->tabResources->indexOf(u->tabWelcome)) != -1)
    u->tabResources->removeTab(i);

  return true;
}

bool KLFLibBrowser::closeResource(const QUrl& url)
{
  KLFLibBrowserViewContainer * w = findOpenUrl(url);
  if (w == NULL)
    return false;

  return slotResourceClose(w);
}


void KLFLibBrowser::updateResourceRoleFlags(KLFLibBrowserViewContainer *viewc, uint resroleflags)
{
  viewc->setResourceRoleFlags(resroleflags);
}


void KLFLibBrowser::slotTabResourceShown(int tabIndex)
{
  qDebug()<<"KLFLibBrowser::slotTabResourceShown("<<tabIndex<<")";
  KLFLibBrowserViewContainer * viewc =
    qobject_cast<KLFLibBrowserViewContainer*>(u->tabResources->widget(tabIndex));
  if (viewc == NULL || tabIndex < 0) {
    qDebug()<<"\tNULL View or invalid tab index="<<tabIndex<<".";
    return;
  }

  // set up view type menu appropriately
  QStringList vtypes = viewc->supportedViewTypeIdentifiers();
  int k;
  QMenu *menu = new QMenu(this);
  QSignalMapper *signalmapper = new QSignalMapper(menu);
  for (k = 0; k < vtypes.size(); ++k) {
    KLFLibViewFactory *factory =
      KLFLibViewFactory::findFactoryFor(vtypes[k]);
    QAction *a = menu->addAction(factory->viewTypeTitle(vtypes[k]), signalmapper, SLOT(map()));
    signalmapper->setMapping(a, vtypes[k]);
  }
  connect(signalmapper, SIGNAL(mapped(const QString&)), viewc, SLOT(openView(const QString&)));
  // and replace the old menu with the new one.
  QMenu *oldmenu = pAViewType->menu();
  if (oldmenu != NULL) {
    pAViewType->setMenu(0);
    delete oldmenu;
  }
  pAViewType->setMenu(menu);

  // refresh selection-related displays
  slotEntriesSelected(viewc->view()->selectedEntries());
  slotRefreshResourceActionsEnabled();
}

void KLFLibBrowser::slotShowTabContextMenu(const QPoint& pos)
{
  int tab = u->tabResources->getTabAtPoint(pos);
  if (tab != -1) {
    u->tabResources->setCurrentIndex(tab);
  }
  pResourceMenu->popup(u->tabResources->mapToGlobal(pos));
}

void KLFLibBrowser::slotResourceRename()
{
  int tab = u->tabResources->currentIndex();
  KLFLibBrowserViewContainer * view = curView();
  if (tab < 0 || view == NULL)
    return;

  qDebug()<<"Rename!";

  if ( ! view->resourceEngine()->canModifyProp(KLFLibResourceEngine::PropTitle) )
    return;

  QLineEdit * editor = new QLineEdit(u->tabResources);
  editor->setGeometry(u->tabResources->getTabRect(tab));
  editor->show();
  editor->setText(view->resourceEngine()->title());
  editor->setFocus();
  editor->setProperty("tabURL", viewForTabIndex(tab)->resourceEngine()->url());
  editor->setProperty("resourceTitleEditor", true);
  editor->setProperty("needsBackground", true);
  editor->setStyleSheet("");
  editor->installEventFilter(this);

  // kill editor if tab changes
  connect(u->tabResources, SIGNAL(currentChanged(int)), editor, SLOT(deleteLater()));
  connect(editor, SIGNAL(returnPressed()), this, SLOT(slotResourceRenameFinished()));

}

void KLFLibBrowser::slotResourceRenameFinished()
{
  QObject * editor = sender();
  if (editor == NULL) {
    qWarning("KLFLibBrowser::slotResourceRenameFinished: no sender!");
  }
  QUrl url = editor->property("tabURL").toUrl();
  KLFLibBrowserViewContainer * view = findOpenUrl(url);
  view->resourceEngine()->setTitle(editor->property("text").toString());

  editor->deleteLater();
}

bool KLFLibBrowser::slotResourceClose(KLFLibBrowserViewContainer *view)
{
  if (view == NULL)
    view = curView();
  if (view == NULL)
    return false;

  if (view->resourceRoleFlags() & NoCloseRoleFlag) // sorry, can't close.
    return false;

  qDebug()<<"Close! resflags="<<view->resourceRoleFlags();

  int tabindex = u->tabResources->indexOf(view);
  if (tabindex < 0) {
    qWarning("KLFLibBrowser::closeResource(url): can't find view in tab widget?!?\n"
	     "\turl=%s, viewwidget=%p", qPrintable(view->resourceEngine()->url().toString()), view);
    return false;
  }
  u->tabResources->removeTab(tabindex);
  int index = pLibViews.indexOf(view);
  pLibViews.removeAt(index);
  // delete view and engine
  KLFLibResourceEngine *resource = view->resourceEngine();
  delete view;
  delete resource;

  if (u->tabResources->count() == 0)
    u->tabResources->addTab(u->tabWelcome, QIcon(":/pics/library.png"), tr("Library Browser"));

  return true;
}
void KLFLibBrowser::slotResourceProperties()
{
  KLFLibBrowserViewContainer *view = curView();
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
  KLFLibResourceEngine *resource = KLFLibCreateResourceDlg::createResource(this, this);
  if (resource == NULL)
    return false;

  return openResource(resource);
}

bool KLFLibBrowser::slotResourceSaveAs()
{
  return false;
  // YET TO BE IMPLEMENTED... using the factories to get a save as... widget..
  //   KLFLibBrowserViewContainer *view = curView();
  //   if (view == NULL) {
  //     qWarning("KLFLibBrowser::slotResourceProperties: NULL View!");
  //     return false;
  //   }
}

void KLFLibBrowser::slotResourceDataChanged(const QList<KLFLib::entryId>& /*entryIdList*/)
{
  //   KLFLibResourceEngine *resource = qobject_cast<KLFLibResourceEngine*>(sender());
  //   if (resource == NULL) {
  //     qWarning("KLFLibBrowser::slotResourceDataChanged: NULL sender or not resource!");
  //     return;
  //   }
  KLFLibBrowserViewContainer *viewc = qobject_cast<KLFLibBrowserViewContainer*>(sender());
  if (viewc == NULL) {
    qWarning()<<"KLFLibBrowser::slotResourceDataChanged: NULL sender or not KLFLibBro.ViewCont.!";
    return;
  }
  slotRefreshResourceActionsEnabled();

  KLFAbstractLibView * view = viewc->view();
  if (view == NULL) {
    qWarning()<<"KLFLibBrowser::slotResourceDataChanged: NULL view !!";
    return;
  }
  slotEntriesSelected(view->selectedEntries());
}
void KLFLibBrowser::slotResourcePropertyChanged(int propId)
{
  KLFLibResourceEngine *resource = qobject_cast<KLFLibResourceEngine*>(sender());
  if (resource == NULL) {
    qWarning("KLFLibBrowser::slotResourcePropertyChanged: NULL sender or not resource!");
    return;
  }

  slotRefreshResourceActionsEnabled();

  KLFLibBrowserViewContainer *view = findOpenUrl(resource->url());
  if (view == NULL) {
    qWarning()<<"KLFLibBrowser::slotResourcePropertyChanged: can't find view for url "
	      <<resource->url()<<"!";
    return;
  }
  if (propId == KLFLibResourceEngine::PropTitle) {
    u->tabResources->setTabText(u->tabResources->indexOf(view), resource->title());
  }
  if (propId == KLFLibResourceEngine::PropLocked) {
    u->tabResources->refreshTabReadOnly(u->tabResources->indexOf(view),
					!resource->canModifyData(KLFLibResourceEngine::AllActionsData));
    u->wEntryEditor
      ->setInputEnabled(resource->canModifyData(KLFLibResourceEngine::ChangeData));
    u->wEntryEditor->displayEntries(view->view()->selectedEntries());
  }
}

void KLFLibBrowser::slotRestoreWithStyle()
{
  KLFAbstractLibView * view = curLibView();
  if ( view == NULL )
    return;
  view->restoreWithStyle();
}

void KLFLibBrowser::slotRestoreLatexOnly()
{
  KLFAbstractLibView * view = curLibView();
  if ( view == NULL )
    return;
  view->restoreLatexOnly();
}

void KLFLibBrowser::slotDeleteSelected()
{
  KLFAbstractLibView * view = curLibView();
  if ( view == NULL )
    return;
  view->deleteSelected();
}

void KLFLibBrowser::slotRefreshResourceActionsEnabled()
{
  bool master = false;
  bool canrename = false;
  bool cansaveas = false;
  uint resrolefl = 0;

  KLFLibBrowserViewContainer * view = curView();
  if ( view != NULL ) {
    master = true;
    canrename = view->resourceEngine()->canModifyProp(KLFLibResourceEngine::PropTitle);
    cansaveas = (view->resourceEngine()->supportedFeatureFlags() & KLFLibResourceEngine::FeatureSaveAs);
    resrolefl = view->resourceRoleFlags();
  }

  pARename->setEnabled(canrename);
  pAProperties->setEnabled(master);
  pASaveAs->setEnabled(master && cansaveas);
  pANew->setEnabled(true);
  pAOpen->setEnabled(true);
  pAClose->setEnabled(master && !(resrolefl & NoCloseRoleFlag));
}



void KLFLibBrowser::slotEntriesSelected(const KLFLibEntryList& entries)
{
  qDebug()<<"KLFLibBrowser::slotEntriesSelected(): "<<entries;
  if (entries.size()>=1)
    qDebug()<<"KLFLibBrowser::slotEntriesSelected():\tTag of first entry="<<entries[0].property(KLFLibEntry::Tags);

  KLFAbstractLibView *view = curLibView();
  if (view != NULL) {
    u->wEntryEditor
      ->setInputEnabled(view->resourceEngine()->canModifyData(KLFLibResourceEngine::ChangeData));
  }

  u->wEntryEditor->displayEntries(entries);

  u->btnDelete->setEnabled(entries.size() > 0);
  u->btnRestore->setEnabled(entries.size() == 1);
}
void KLFLibBrowser::slotAddCategorySuggestions(const QStringList& catlist)
{
  qDebug()<<"KLFLibBrowser: got category suggestions: "<<catlist;
  u->wEntryEditor->addCategorySuggestions(catlist);
}

void KLFLibBrowser::slotShowContextMenu(const QPoint& pos)
{
  KLFAbstractLibView * view = curLibView();
  if ( view == NULL )
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
  int n_destinations = 0;
  for (k = 0; k < pLibViews.size(); ++k) {
    if (pLibViews[k]->url() == view->resourceEngine()->url()) // skip this view
      continue;
    KLFLibResourceEngine *res = pLibViews[k]->resourceEngine();
    n_destinations++;
    acopy = copytomenu->addAction(res->title(), this, SLOT(slotCopyToResource()));
    acopy->setProperty("resourceUrl", res->url());
    amove = movetomenu->addAction(res->title(), this, SLOT(slotMoveToResource()));
    amove->setProperty("resourceUrl", res->url());
    if (!res->canModifyData(KLFLibResourceEngine::InsertData)) {
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

  menu->addSeparator();
  menu->addMenu(pResourceMenu);

  // Needed for when user pops up a menu without selection (ie. short list, free white space under)
  KLFLibEntryList selected = view->selectedEntries();
  bool cancopy = (selected.size() > 0) && n_destinations;
  bool canre = (selected.size() == 1);
  bool candel = view->resourceEngine()->canModifyData(KLFLibResourceEngine::DeleteData);
  a1->setEnabled(canre);
  a2->setEnabled(canre);
  adel->setEnabled(candel && selected.size());
  acopyto->setEnabled(cancopy);
  amoveto->setEnabled(cancopy && candel);

  // add view's own actions

  QList<QAction*> viewActions = view->addContextMenuActions(pos);
  if (viewActions.size())
    menu->addSeparator(); // separate view's menu items from ours
  for (k = 0; k < viewActions.size(); ++k) {
    qDebug()<<"Added action "<<k<<": "<<viewActions[k];
    menu->addAction(viewActions[k]);
  }

  menu->popup(view->mapToGlobal(pos));
}



void KLFLibBrowser::slotCategoryChanged(const QString& newcategory)
{
  QWidget *w = u->tabResources->currentWidget();
  KLFLibBrowserViewContainer *wviewc = qobject_cast<KLFLibBrowserViewContainer*>(w);
  if (wviewc == NULL) {
    qWarning("Current view is not a KLFLibBrowserViewContainer!");
    return;
  }
  KLFAbstractLibView * wview = wviewc->view();
  if ( wview == NULL )
    return;
  bool r = wview->writeEntryProperty(KLFLibEntry::Category, newcategory);
  if ( ! r ) {
    QMessageBox::warning(this, tr("Error"),
			 tr("Failed to write category information!"));
    // and refresh display
    slotEntriesSelected(wview->selectedEntries());
  }
  // display refreshed anyway in case of success.
}
void KLFLibBrowser::slotTagsChanged(const QString& newtags)
{
  QWidget *w = u->tabResources->currentWidget();
  KLFLibBrowserViewContainer *wviewc = qobject_cast<KLFLibBrowserViewContainer*>(w);
  if (wviewc == NULL) {
    qWarning("Current view is not a KLFLibBrowserViewContainer!");
    return;
  }
  KLFAbstractLibView * wview = wviewc->view();
  if ( wview == NULL )
    return;
  bool r = wview->writeEntryProperty(KLFLibEntry::Tags, newtags);
  if ( ! r ) {
    QMessageBox::warning(this, tr("Error"),
			 tr("Failed to write tags information!"));
    // and refresh display
    slotEntriesSelected(wview->selectedEntries());
  }
  // display refreshed anyway in case of success.
}


void KLFLibBrowser::slotSearchClear()
{
  if (QApplication::focusWidget() == u->txtSearch) {
    // already has focus: means that we want to recall history search
    u->txtSearch->setText(pLastSearchText);
  } else {
    u->txtSearch->setText("");
    u->txtSearch->setFocus();
  }
}
void KLFLibBrowser::slotSearchClearOrNext()
{
  if (QApplication::focusWidget() == u->txtSearch) {
    // already has focus
    // -> either recall history (if empty search text)
    // -> or find next
    if (u->txtSearch->text().isEmpty()) {
      u->txtSearch->setText(pLastSearchText);
    } else {
      if (pSearchText.isEmpty())
	slotSearchFind(u->txtSearch->text());
      else
	slotSearchFindNext();
    }
  } else {
    u->txtSearch->setText("");
    u->txtSearch->setFocus();
  }
}

void KLFLibBrowser::slotSearchFind(const QString& text, bool forward)
{
  KLFAbstractLibView * view = curLibView();
  if (view == NULL) {
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
  KLFAbstractLibView * view = curLibView();
  if (view == NULL) {
    qWarning("KLFLibBrowser: No Current View!");
    return;
  }
  // focus search bar if not yet focused.
  if (QApplication::focusWidget() != u->txtSearch)
    u->txtSearch->setFocus();

  if (pSearchText.isEmpty()) {
    // we're not in search mode
    if (u->txtSearch->text().isEmpty()) {
      // recall history
      u->txtSearch->blockSignals(true); // stop txtSearch from calling our slotSearchFind()
      u->txtSearch->setText(pLastSearchText);
      u->txtSearch->blockSignals(false);
    }
    // and initiate search mode
    slotSearchFind(u->txtSearch->text(), forward);
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
  if ( ! u->txtSearch->text().isEmpty() ) {
    u->txtSearch->setText("");
    // return because we will automatically be re-called because txtSearch will re-emit textChanged()
    // which in turn calls slotSearchFind() with empty text.
    return;
  }

  KLFAbstractLibView * view = curLibView();
  if (view == NULL) {
    qWarning("KLFLibBrowser: No Current View!");
    return;
  }
  view->searchAbort();

  u->txtSearch->setProperty("searchState", QString("aborted"));
  u->txtSearch->setStyleSheet(u->txtSearch->styleSheet());
  QPalette pal = u->txtSearch->property("defaultPalette").value<QPalette>();
  u->txtSearch->setPalette(pal);
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
  KLFAbstractLibView *sourceView = curLibView();
  if (sourceView == NULL) {
    qWarning()<<"KLFLibBrowser::slotCopyMoveToResource(): source view is NULL!";
    return;
  }
  KLFLibBrowserViewContainer *destViewC = findOpenUrl(destUrl);
  if (destViewC == NULL || destViewC->view() == NULL) {
    qWarning()<<"KLFLibBrowser::slotCopyMoveToResource(): can't find dest view url for URL="<<destUrl<<" !";
    return;
  }
  KLFAbstractLibView *destView = destViewC->view();
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
  //  qDebug("updateSearchFound(%d)", found);
  QPalette pal;
  if (found) {
    pal = u->txtSearch->property("paletteFound").value<QPalette>();
    u->txtSearch->setProperty("searchState", QString("found"));
  } else {
    pal = u->txtSearch->property("paletteNotFound").value<QPalette>();
    u->txtSearch->setProperty("searchState", QString("not-found"));
  }
  u->txtSearch->setStyleSheet(u->txtSearch->styleSheet());
  u->txtSearch->setPalette(pal);
}

void KLFLibBrowser::slotSearchFocusIn()
{
  u->txtSearch->setProperty("searchState", QString("default"));
  u->txtSearch->setPalette(u->txtSearch->property("defaultPalette").value<QPalette>());
  u->txtSearch->blockSignals(true);
  u->txtSearch->setText("");
  u->txtSearch->blockSignals(false);
}
void KLFLibBrowser::slotSearchFocusOut()
{
  u->txtSearch->setProperty("searchState", QString("focus-out"));
  u->txtSearch->setStyleSheet(u->txtSearch->styleSheet());
  u->txtSearch->setPalette(u->txtSearch->property("paletteFocusOut").value<QPalette>());
  u->txtSearch->blockSignals(true);
  u->txtSearch->setText("  "+tr("Hit Ctrl-F, Ctrl-S or / to search within the current resource"));
  u->txtSearch->blockSignals(false);
}


