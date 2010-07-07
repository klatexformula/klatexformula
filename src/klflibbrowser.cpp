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
#include <QMovie>
#include <QProgressDialog>
#include <QPushButton>
#include <QApplication>
#include <QClipboard>

#include "klfconfig.h"
#include "klfguiutil.h"
#include "klflibbrowser_p.h"
#include "klflibbrowser.h"
#include <ui_klflibbrowser.h>



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

  pAnimMovie = new QMovie(":/pics/wait_anim.mng", "MNG", this);
  pAnimMovie->setCacheMode(QMovie::CacheAll);
  //  u->lblWaitAnimation->setMovie(movie);
  //  movie->start();
  u->lblWaitAnimation->hide();
  pAnimTimerId = -1;
  pIsWaiting = false;

  pResourceMenu = new QMenu(u->tabResources);
  // connect actions
  connect(u->aRename, SIGNAL(triggered()), this, SLOT(slotResourceRename()));
  connect(u->aRenameSubRes, SIGNAL(triggered()), this, SLOT(slotResourceRenameSubResource()));
  connect(u->aProperties, SIGNAL(triggered()), this, SLOT(slotResourceProperties()));
  connect(u->aNewSubRes, SIGNAL(triggered()), this, SLOT(slotResourceNewSubRes()));
  connect(u->aSaveTo, SIGNAL(triggered()), this, SLOT(slotResourceSaveTo()));
  connect(u->aNew, SIGNAL(triggered()), this, SLOT(slotResourceNew()));
  connect(u->aOpen, SIGNAL(triggered()), this, SLOT(slotResourceOpen()));
  connect(u->aClose, SIGNAL(triggered()), this, SLOT(slotResourceClose()));
  // and add them to menu
  pResourceMenu->addAction(u->aRename);
  pResourceMenu->addAction(u->aRenameSubRes);
  pResourceMenu->addAction(u->aProperties);
  pResourceMenu->addAction(u->aNewSubRes);
  pResourceMenu->addAction(u->aSaveTo);
  pResourceMenu->addAction(u->aViewType);
  pResourceMenu->addSeparator();
  pResourceMenu->addAction(u->aNew);
  pResourceMenu->addAction(u->aOpen);
  pResourceMenu->addAction(u->aClose);

  slotRefreshResourceActionsEnabled();

  pTabCornerButton = new QPushButton(u->tabResources);
  pTabCornerButton->setMenu(pResourceMenu);
  u->tabResources->setCornerWidget(pTabCornerButton);

  connect(u->tabResources, SIGNAL(currentChanged(int)), this, SLOT(slotTabResourceShown(int)));
  connect(u->tabResources, SIGNAL(customContextMenuRequested(const QPoint&)),
	  this, SLOT(slotShowTabContextMenu(const QPoint&)));

  // RESTORE

  QMenu * restoreMenu = new QMenu(this);
  connect(u->aRestoreWithStyle, SIGNAL(triggered()), this, SLOT(slotRestoreWithStyle()));
  connect(u->aRestoreLatexOnly, SIGNAL(triggered()), this, SLOT(slotRestoreLatexOnly()));
  restoreMenu->addAction(u->aRestoreWithStyle);
  restoreMenu->addAction(u->aRestoreLatexOnly);
  u->btnRestore->setMenu(restoreMenu);

  connect(u->btnRestore, SIGNAL(clicked()), this, SLOT(slotRestoreWithStyle()));
  connect(u->btnDelete, SIGNAL(clicked()), this, SLOT(slotDeleteSelected()));
  connect(u->aDelete, SIGNAL(triggered()), u->btnDelete, SLOT(animateClick()));

  // IMPORT/EXPORT

  pImportExportMenu = new QMenu(this);
  pImportExportMenu->addAction(u->aOpenAll);
  pImportExportMenu->addAction(u->aExport);
  u->btnImportExport->setMenu(pImportExportMenu);

  connect(u->aOpenAll, SIGNAL(triggered()), this, SLOT(slotOpenAll()));
  connect(u->aExport, SIGNAL(triggered()), this, SLOT(slotExport()));
  

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
		      
  // search-related
  (void)new QShortcut(QKeySequence(tr("Ctrl+F", "[[find]]")), this, SLOT(slotSearchClearOrNext()));
  (void)new QShortcut(QKeySequence(tr("Ctrl+S", "[[find]]")), this, SLOT(slotSearchClearOrNext()));
  (void)new QShortcut(QKeySequence(tr("/", "[[find]]")), this, SLOT(slotSearchClear()));
  (void)new QShortcut(QKeySequence(tr("F3", "[[find next]]")), this, SLOT(slotSearchFindNext()));
  (void)new QShortcut(QKeySequence(tr("Shift+F3", "[[find prev]]")), this, SLOT(slotSearchFindPrev()));
  (void)new QShortcut(QKeySequence(tr("Ctrl+R", "[[find]]")), this, SLOT(slotSearchFindPrev()));
  // Esc will be captured through event filter so that it isn't too obstrusive...
  //  (void)new QShortcut(QKeySequence(tr("Esc", "[[abort find]]")), this, SLOT(slotSearchAbort()));
  // cut/copy/paste
  (void)new QShortcut(QKeySequence::Delete, this, SLOT(slotDeleteSelected()));
  (void)new QShortcut(QKeySequence::Cut, this, SLOT(slotCut()));
  (void)new QShortcut(QKeySequence::Copy, this, SLOT(slotCopy()));
  (void)new QShortcut(QKeySequence::Paste, this, SLOT(slotPaste()));

  retranslateUi(false);

  // start with no entries selected
  slotEntriesSelected(QList<KLFLibEntry>());
  // and set search bar's focus-out palette
  slotSearchFocusOut();
}

void KLFLibBrowser::retranslateUi(bool alsoBaseUi)
{
  if (alsoBaseUi)
    u->retranslateUi(this);

  u->wEntryEditor->retranslateUi(alsoBaseUi);

  pResourceMenu->setTitle(tr("Resource Actions", "[[menu title]]"));

  pTabCornerButton->setText(tr("Resource"));
}


KLFLibBrowser::~KLFLibBrowser()
{
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

  if (ev->type() == QEvent::Hide &&
      obj->property("klf_libbrowser_pdlg_want_hideautodelete").toBool() == true) {
    // this object is a resource-operation progress dialog that was just hidden
    klfDbg( ": Hiding progress dialog." ) ;
    obj->deleteLater();
    return true;
    //    return false;    // continue with event handling
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
  for (k = 0; k < pLibViews.size(); ++k) {
    urls << pLibViews[k]->url();
  }
  return urls;
}

KLFLibResourceEngine * KLFLibBrowser::getOpenResource(const QUrl& url)
{
  KLFLibBrowserViewContainer * viewc = findOpenUrl(url);
  if (viewc == NULL)
    return NULL;
  return viewc->resourceEngine();
}

KLFAbstractLibView * KLFLibBrowser::getView(const QUrl& url)
{
  KLFLibBrowserViewContainer * viewc = findOpenUrl(url);
  if (viewc == NULL)
    return NULL;
  return viewc->view();
}

KLFAbstractLibView * KLFLibBrowser::getView(KLFLibResourceEngine *resource)
{
  KLFLibBrowserViewContainer * viewc = findOpenResource(resource);
  if (viewc == NULL)
    return NULL;
  return viewc->view();
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
void KLFLibBrowser::loadGuiState(const QVariantMap& v, bool openURLs)
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
    klfDbg( "LibBrowser::loadGuiState: Opening url "<<url<<" with flags="<<flags ) ;
    QVariantMap viewState = viewstatelist[k].toMap();
    // don't open new URLs if openURLs is false
    if ( !openURLs && findOpenUrl(url) == NULL )
      continue;
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
  klfDbg( "Almost finished loading gui state." ) ;
  KLFLibBrowserViewContainer *curviewc = findOpenUrl(currenturl);
  if (curviewc != NULL)
    u->tabResources->setCurrentWidget(curviewc);
  klfDbg( "Loaded GUI state." ) ;

  if (widgetsize.width() > 0 && widgetsize.height() > 0)
    resize(widgetsize);
}



// static
QString KLFLibBrowser::displayTitle(KLFLibResourceEngine *resource)
{
  QString basestr;
  if (resource->supportedFeatureFlags() & KLFLibResourceEngine::FeatureSubResources) {
    QString subresourcetitle = resource->defaultSubResource();
    if (resource->supportedFeatureFlags() & KLFLibResourceEngine::FeatureSubResourceProps)
      subresourcetitle
	= resource->subResourceProperty(resource->defaultSubResource(),
					KLFLibResourceEngine::SubResPropTitle).toString();
    basestr = QString("%1 - %2").arg(resource->title(), subresourcetitle);
  } else {
    basestr = resource->title();
  }
  if (!resource->canModifyData(KLFLibResourceEngine::AllActionsData)) {
    basestr = "# "+basestr;
  }
  return basestr;
}


KLFLibBrowserViewContainer * KLFLibBrowser::findOpenUrl(const QUrl& url)
{
  KLF_DEBUG_BLOCK(KLF_FUNC_NAME) ;
  klfDbg( "\turl is "<<url ) ;
  int k;
  for (k = 0; k < pLibViews.size(); ++k) {
    klfDbg( ": test lib view #"<<k<<"; url="<<pLibViews[k]->url() ) ;
    uint fl = klfUrlCompare(pLibViews[k]->url(), url,
			    KlfUrlCompareEqual|KlfUrlCompareLessSpecific,
			    QStringList()<<"klfDefaultSubResource");
    // allow: * urls to be equal
    //        * this resource's URL to be less specific (shows more) than what we're searching for
    if (fl & KlfUrlCompareEqual ||
	fl & KlfUrlCompareLessSpecific) {
      klfDbg( ": Found!" ) ;
      return pLibViews[k];
    }
  }
  return NULL;

  /*
  QUrl searchurl = url;
  QString defsr = url.hasQueryItem("klfDefaultSubResource")
    ?  url.queryItemValue("klfDefaultSubResource")
    :  QString();
  searchurl.removeAllQueryItems("klfDefaultSubResource");
  int k;
  for (k = 0; k < pLibViews.size(); ++k)
    if ( pLibViews[k]->url() == searchurl &&
	 (defsr.isNull() || defsr == pLibViews[k]->defaultSubResource()) )
      return pLibViews[k];
  return NULL;
  */
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
  KLF_DEBUG_BLOCK(KLF_FUNC_NAME) ;
  klfDbg( " url="<<url.toString()<<"; resroleflags="<<resourceRoleFlags
	  <<"; vtypeidentifier="<<viewTypeIdentifier ) ;

  KLFLibBrowserViewContainer * openview = findOpenUrl(url);
  if (openview != NULL) {
    qDebug("KLFLibBrowser::openResource(%s,%u): This resource is already open.",
	   qPrintable(url.toString()), resourceRoleFlags);
    u->tabResources->setCurrentWidget(openview);
    updateResourceRoleFlags(openview, resourceRoleFlags);
    return true;
  }

  KLFLibEngineFactory * factory = KLFLibEngineFactory::findFactoryFor(url.scheme());
  if ( factory == NULL ) {
    qWarning()<<KLF_FUNC_NAME<<": failed to find appropriate factory for url="<<url.toString()<<"!";
    return false;
  }
  KLFLibResourceEngine * resource = factory->openResource(url, this);
  if ( resource == NULL ) {
    qWarning()<<KLF_FUNC_NAME<<": factory failed to open resource "<<url.toString()<<"!";
    return false;
  }

  // go on opening resource with our sister function
  return openResource(resource, resourceRoleFlags, viewTypeIdentifier);
}
bool KLFLibBrowser::openResource(KLFLibResourceEngine *resource, uint resourceRoleFlags,
				 const QString& viewTypeIdentifier)
{
  KLF_DEBUG_TIME_BLOCK(KLF_FUNC_NAME) ;
  klfDbg( "\topening resource url="
	  <<resource->url(KLFLibResourceEngine::WantUrlDefaultSubResource).toString() ) ;

  //  KLFPleaseWaitPopup label(tr("Loading resource, please wait..."), this);

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

  klfDbgT(": created resource. about to create view.") ;

  // now create appropriate view for this resource
  KLFLibBrowserViewContainer *viewc = new KLFLibBrowserViewContainer(resource, u->tabResources);

  // create a list of view types to attempt to open, in a given priority order
  QStringList viewtypeident_try;
  //   * the argument to this function
  viewtypeident_try << viewTypeIdentifier;
  if ((resource->supportedFeatureFlags() & KLFLibResourceEngine::FeatureSubResources) &&
      (resource->supportedFeatureFlags() & KLFLibResourceEngine::FeatureSubResourceProps)) {
    // * the sub-resource view type property (if applicable)
    viewtypeident_try
      << resource->subResourceProperty(resource->defaultSubResource(),
				       KLFLibResourceEngine::SubResPropViewType).toString();
  }
  // * the resource view type property
  viewtypeident_try << resource->viewType()
    // * the suggested view type for this resource (as given by engine itself)
		    << resource->suggestedViewTypeIdentifier()
    // * the default view type suggested by the factory
		    << KLFLibViewFactory::defaultViewTypeIdentifier()
    // * a "default" view type (last resort, hoping it exists!)
		    << QLatin1String("default");

  klfDbgT(": created resource. about to test view types.") ;
  klfDbg( "\tView types: "<<viewtypeident_try ) ;

  int k;
  // try each view type, first success is kept.
  for (k = 0; k < viewtypeident_try.size(); ++k) {
    if (viewtypeident_try[k].isEmpty())
      continue;

    KLFLibViewFactory *viewfactory =
      KLFLibViewFactory::findFactoryFor(viewtypeident_try[k]);
    if (viewfactory == NULL) {
      klfDbg( "KLFLibBrowser::openResource: can't find view factory for view type identifier "
	      <<viewtypeident_try[k]<<"!" ) ;
      continue;
    }
    if ( ! viewfactory->canCreateLibView(viewtypeident_try[k], resource) ) {
      klfDbg( "KLFLibBrowser::openResource: incompatible view type identifier "<<viewtypeident_try[k]
	      <<"for resource "<<resource->url()<<"." ) ;
      continue;
    }
    bool r = viewc->openView(viewtypeident_try[k]);
    if (!r) {
      klfDbg( "KLFLibBrowser::openResource: can't create view! viewtypeident="<<viewtypeident_try[k]<<"." ) ;
      continue;
    }

    klfDbgT(": found good view type="<<viewtypeident_try[k]) ;

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
  connect(resource, SIGNAL(subResourcePropertyChanged(const QString&, int)),
	  this, SLOT(slotSubResourcePropertyChanged(const QString&, int)));
  connect(resource, SIGNAL(defaultSubResourceChanged(const QString&)),
	  this, SLOT(slotDefaultSubResourceChanged(const QString&)));

  connect(resource, SIGNAL(operationStartReportingProgress(KLFProgressReporter *, const QString&)),
	  this, SLOT(slotStartProgress(KLFProgressReporter *, const QString&)));

  klfDbgT(": requiring cat suggestions.") ;

  // get more category completions
  viewc->view()->wantMoreCategorySuggestions();

  // supply a context menu to view
  connect(viewc, SIGNAL(viewContextMenuRequested(const QPoint&)),
	  this, SLOT(slotShowContextMenu(const QPoint&)));

  klfDbgT(": adding tab page....") ;

  int i = u->tabResources->addTab(viewc, displayTitle(resource));
  u->tabResources->setCurrentWidget(viewc);
  u->tabResources
    ->refreshTabReadOnly(i, !resource->canModifyData(KLFLibResourceEngine::AllActionsData));
  pLibViews.append(viewc);
  setStyleSheet(styleSheet());
  updateResourceRoleFlags(viewc, resourceRoleFlags);

  // hide welcome page if it's shown
  if ((i = u->tabResources->indexOf(u->tabWelcome)) != -1)
    u->tabResources->removeTab(i);

  klfDbgT(": end of function") ;

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
  KLF_DEBUG_TIME_BLOCK(KLF_FUNC_NAME) ;
  klfDbg( "\t tabIndex="<<tabIndex ) ;
  KLFLibBrowserViewContainer * viewc =
    qobject_cast<KLFLibBrowserViewContainer*>(u->tabResources->widget(tabIndex));
  if (viewc == NULL || tabIndex < 0) {
    klfDbg( "\tNULL View or invalid tab index="<<tabIndex<<"." ) ;
    return;
  }

  // set up view type menu appropriately
  QList<QAction*> actions = viewc->viewTypeActions();
  QMenu *menu = new QMenu(this);
  int k;
  for (k = 0; k < actions.size(); ++k) {
    menu->addAction(actions[k]);
  }
  // and replace the old menu with the new one.
  QMenu *oldmenu = u->aViewType->menu();
  if (oldmenu != NULL) {
    u->aViewType->setMenu(0);
    delete oldmenu;
  }
  u->aViewType->setMenu(menu);

  // DEBUG
  //  viewc->openView("default+list");

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
  slotResourceRename(false);
}
void KLFLibBrowser::slotResourceRenameSubResource()
{
  slotResourceRename(true);
}
void KLFLibBrowser::slotResourceRename(bool renamingSubResource)
{
  int tab = u->tabResources->currentIndex();
  KLFLibBrowserViewContainer * viewc = curView();
  if (tab < 0 || viewc == NULL)
    return;

  klfDbg( ": Rename! renamingSubResource="<<renamingSubResource ) ;

  KLFLibResourceEngine *res = viewc->resourceEngine();

  if ( ! renamingSubResource && ! res->canModifyProp(KLFLibResourceEngine::PropTitle) )
    return;

  if ( renamingSubResource &&
       ( !(res->supportedFeatureFlags() & KLFLibResourceEngine::FeatureSubResources) ||
	 !(res->supportedFeatureFlags() & KLFLibResourceEngine::FeatureSubResourceProps) ||
	 !(res->canModifySubResourceProperty(res->defaultSubResource(),
					     KLFLibResourceEngine::SubResPropTitle))
	 ) )
    return;

  QLineEdit * editor = new QLineEdit(u->tabResources);
  editor->setGeometry(u->tabResources->getTabRect(tab));
  editor->show();
  if (!renamingSubResource)
    editor->setText(res->title());
  else
    editor->setText(res->subResourceProperty(res->defaultSubResource(),
					     KLFLibResourceEngine::SubResPropTitle).toString());
  editor->setFocus();
  editor->setProperty("tabURL", viewc->url());
  editor->setProperty("resourceTitleEditor", true);
  editor->setProperty("needsBackground", true);
  editor->setProperty("renamingSubResource", renamingSubResource);
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
    return;
  }
  bool isRenamingSubResource = editor->property("renamingSubResource").toBool();
  QUrl url = editor->property("tabURL").toUrl();
  KLFLibBrowserViewContainer * viewc = findOpenUrl(url);
  if (viewc == NULL) {
    qWarning()<<KLF_FUNC_NAME<<": can't find the resource with URL "<<url;
    return;
  }
  KLFLibResourceEngine *res = viewc->resourceEngine();
  QString text = editor->property("text").toString();
  if (!isRenamingSubResource)
    res->setTitle(text);
  else
    res->setSubResourceProperty(res->defaultSubResource(), KLFLibResourceEngine::SubResPropTitle,
				QVariant::fromValue<QString>(text));

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

  klfDbg( "Close! resflags="<<view->resourceRoleFlags() ) ;

  int tabindex = u->tabResources->indexOf(view);
  if (tabindex < 0) {
    qWarning("KLFLibBrowser::closeResource(url): can't find view in tab widget?!?\n"
	     "\turl=%s, viewwidget=%p", qPrintable(view->url().toString()), view);
    return false;
  }

  // ask user for confirmation
  QMessageBox::StandardButton btn =
    QMessageBox::question(this, tr("Close Resource"), tr("Do you want to close this resource?"),
			  QMessageBox::Yes|QMessageBox::Cancel, QMessageBox::Yes);
  if (btn != QMessageBox::Yes)
    return false;

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

bool KLFLibBrowser::slotResourceNewSubRes()
{
  KLFLibBrowserViewContainer *view = curView();
  if (view == NULL) {
    qWarning("KLFLibBrowser::slotResourceProperties: NULL View!");
    return false;
  }
  KLFLibResourceEngine *res = view->view()->resourceEngine();
  QString name = KLFLibNewSubResDlg::createSubResourceIn(res, this);
  if (name.isEmpty())
    return false;

  // see remark in comment below
  QUrl url = res->url();
  url.removeAllQueryItems("klfDefaultSubResource");
  url.addQueryItem("klfDefaultSubResource", name);

  klfDbg( "KLFLibBrowser::slotRes.New.S.Res(): Create sub-resource named "<<name<<", opening "<<url ) ;

  // in case of a view displaying all sub-resources, this also works because findOpenUrl() will find
  // that view and raise it.
  return openResource(url);
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
  KLFLibResourceEngine *resource = KLFLibCreateResourceDlg::createResource(QString(), this, this);
  if (resource == NULL)
    return false;

  return openResource(resource);
}

bool KLFLibBrowser::slotResourceSaveTo()
{
  return false;
  // YET TO BE IMPLEMENTED... using the factories to get a save to... widget..
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
  slotUpdateForResourceProperty(resource, propId);
}

void KLFLibBrowser::slotUpdateForResourceProperty(KLFLibResourceEngine *resource, int propId)
{
  qDebug("KLFLibBrowser::slotUpdateForResourceProperty(%p, %d)", (void*)resource, propId);
  slotRefreshResourceActionsEnabled();

  KLFLibBrowserViewContainer *view = findOpenResource(resource);
  if (view == NULL) {
    qWarning()<<"KLFLibBrowser::slotResourcePropertyChanged: can't find view for resource "
	      <<resource<<", url="<<resource->url()<<"!";
    return;
  }
  if (propId == KLFLibResourceEngine::PropTitle || propId == KLFLibResourceEngine::PropLocked) {
    // the title is also affected by the locked state (# in front 
    u->tabResources->setTabText(u->tabResources->indexOf(view), displayTitle(resource));
  }
  if (propId == KLFLibResourceEngine::PropLocked) {
    u->tabResources->refreshTabReadOnly(u->tabResources->indexOf(view),
					!resource->canModifyData(KLFLibResourceEngine::AllActionsData));
    u->wEntryEditor
      ->setInputEnabled(resource->canModifyData(KLFLibResourceEngine::ChangeData));
    u->wEntryEditor->displayEntries(view->view()->selectedEntries());
  }
}

void KLFLibBrowser::slotSubResourcePropertyChanged(const QString& /*subResource*/, int propId)
{
  qDebug("KLFLibBrowser::slotSubResourcePropertyChanged(..,%d)", propId);

  KLFLibResourceEngine *resource = qobject_cast<KLFLibResourceEngine*>(sender());
  if (resource == NULL) {
    qWarning("KLFLibBrowser::slotSubResourcePropertyChanged: NULL sender or not resource!");
    return;
  }

  /** \todo ......... CHECK that subResource is our current sub-resource */

  if (propId < 0 || propId == KLFLibResourceEngine::SubResPropTitle) {
    // fake a resource property title change (updated stuff is the same) 
    slotUpdateForResourceProperty(resource, KLFLibResourceEngine::PropTitle);
  }
  if (propId < 0 || propId == KLFLibResourceEngine::SubResPropLocked) {
    // fake a resource property locked state change (updated stuff is the same)
    slotUpdateForResourceProperty(resource, KLFLibResourceEngine::PropLocked);
  }
}

void KLFLibBrowser::slotDefaultSubResourceChanged(const QString& )
{
  KLFLibResourceEngine *resource = qobject_cast<KLFLibResourceEngine*>(sender());
  if (resource == NULL) {
    qWarning("KLFLibBrowser::slotDefaultSubResourceChanged: NULL sender or not resource!");
    return;
  }
  // view already does a full refresh. We will just refresh the tab title.
  // fake a resource property title/locked state change (updated stuff is the same)
  slotUpdateForResourceProperty(resource, KLFLibResourceEngine::PropTitle);
  slotUpdateForResourceProperty(resource, KLFLibResourceEngine::PropLocked);
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
  if ( !view->resourceEngine()->canModifyData(KLFLibResourceEngine::DeleteData) )
    return;

  view->deleteSelected();
}

void KLFLibBrowser::slotRefreshResourceActionsEnabled()
{
  bool master = false;
  bool canrename = false;
  bool canrenamesubres = false;
  bool cansaveto = false;
  bool cannewsubres = false;
  uint resrolefl = 0;
  uint resfeatureflags = 0;

  KLFLibBrowserViewContainer * view = curView();
  if ( view != NULL ) {
    master = true;
    KLFLibResourceEngine *res = view->resourceEngine();
    canrename = res->canModifyProp(KLFLibResourceEngine::PropTitle);
    resfeatureflags = res->supportedFeatureFlags();
    cansaveto = (resfeatureflags & KLFLibResourceEngine::FeatureSaveTo);
    resrolefl = view->resourceRoleFlags();
    cannewsubres = (resfeatureflags & KLFLibResourceEngine::FeatureSubResources) &&
      (res->canCreateSubResource());
    canrenamesubres = (resfeatureflags & KLFLibResourceEngine::FeatureSubResources) &&
      (resfeatureflags & KLFLibResourceEngine::FeatureSubResourceProps) &&
      res->canModifySubResourceProperty(res->defaultSubResource(),
					KLFLibResourceEngine::SubResPropTitle) ;
  }

  u->aRename->setEnabled(canrename);
  u->aRenameSubRes->setEnabled(canrenamesubres);
  u->aProperties->setEnabled(master);
  u->aNewSubRes->setEnabled(master && cannewsubres);
  u->aSaveTo->setEnabled(master && cansaveto);
  u->aNew->setEnabled(true);
  u->aOpen->setEnabled(true);
  u->aClose->setEnabled(master && !(resrolefl & NoCloseRoleFlag));
}



void KLFLibBrowser::slotEntriesSelected(const KLFLibEntryList& entries)
{
  klfDbg( "KLFLibBrowser::slotEntriesSelected(): "<<entries ) ;
  if (entries.size()>=1)
    klfDbg( "KLFLibBrowser::slotEntriesSelected():\tTag of first entry="
	    <<entries[0].property(KLFLibEntry::Tags) ) ;

  KLFAbstractLibView *view = curLibView();
  if (view != NULL) {
    u->wEntryEditor
      ->setInputEnabled(view->resourceEngine()->canModifyData(KLFLibResourceEngine::ChangeData));
  }

  u->wEntryEditor->displayEntries(entries);

  u->btnDelete->setEnabled(entries.size() > 0);
  u->aDelete->setEnabled(u->btnDelete->isEnabled());
  u->btnRestore->setEnabled(entries.size() == 1);
}
void KLFLibBrowser::slotAddCategorySuggestions(const QStringList& catlist)
{
  klfDbg( "KLFLibBrowser: got category suggestions: "<<catlist ) ;
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
  // NOTE: the QKeySequences given here are only for display in the context menu. Their functionality
  // is due to additional QShortcuts declared in the constructor. (!) (reason: these actions are
  // short-lived, and having global actions would require keeping their enabled status up-to-date
  // + we want to add view-given context menu actions dynamically)
  QAction *acut = menu->addAction(QIcon(":/pics/cut.png"), tr("Cut"), this, SLOT(slotCut()),
				  QKeySequence::Cut);
  QAction *acopy = menu->addAction(QIcon(":/pics/copy.png"), tr("Copy"), this, SLOT(slotCopy()),
				  QKeySequence::Copy);
  QAction *apaste = menu->addAction(QIcon(":/pics/paste.png"), tr("Paste"), this, SLOT(slotPaste()),
				  QKeySequence::Paste);
  menu->addSeparator();
  QAction *adel = menu->addAction(QIcon(":/pics/delete.png"), tr("Delete from library"),
				  view, SLOT(deleteSelected()), QKeySequence::Delete);
  menu->addSeparator();

  QMenu *copytomenu = new QMenu;
  QMenu *movetomenu = new QMenu;
  int k;
  QAction *acopythere, *amovethere;
  int n_destinations = 0;
  for (k = 0; k < pLibViews.size(); ++k) {
    if (pLibViews[k]->url() == view->url()) // skip this view
      continue;
    KLFLibResourceEngine *res = pLibViews[k]->resourceEngine();
    QUrl viewurl = pLibViews[k]->url();
    n_destinations++;
    acopythere = copytomenu->addAction(displayTitle(res), this, SLOT(slotCopyToResource()));
    acopythere->setProperty("resourceViewUrl", viewurl);
    amovethere = movetomenu->addAction(displayTitle(res), this, SLOT(slotMoveToResource()));
    amovethere->setProperty("resourceViewUrl", viewurl);
    if (!res->canModifyData(KLFLibResourceEngine::InsertData)) {
      acopythere->setEnabled(false);
      amovethere->setEnabled(false);
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
  bool canpaste = KLFAbstractLibEntryMimeEncoder::canDecodeMimeData(QApplication::clipboard()->mimeData());
  a1->setEnabled(canre);
  a2->setEnabled(canre);
  adel->setEnabled(candel && selected.size());
  acut->setEnabled(cancopy && candel);
  acopy->setEnabled(cancopy);
  apaste->setEnabled(canpaste);
  acopyto->setEnabled(cancopy);
  amoveto->setEnabled(cancopy && candel);

  // add view's own actions

  QList<QAction*> viewActions = view->addContextMenuActions(pos);
  if (viewActions.size())
    menu->addSeparator(); // separate view's menu items from ours
  for (k = 0; k < viewActions.size(); ++k) {
    klfDbg( "Added action "<<k<<": "<<viewActions[k] ) ;
    menu->addAction(viewActions[k]);
  }

  menu->popup(view->mapToGlobal(pos));
}



void KLFLibBrowser::slotCategoryChanged(const QString& newcategory)
{
  QWidget *w = u->tabResources->currentWidget();
  KLFLibBrowserViewContainer *wviewc = qobject_cast<KLFLibBrowserViewContainer*>(w);
  if (wviewc == NULL) {
    qWarning("Current view is not a KLFLibBrowserViewContainer or no current tab widget!");
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
  startWait();
  bool found = view->searchFind(text, forward);
  updateSearchFound(found);
  stopWait();
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
  startWait();
  bool found = view->searchFindNext(forward);
  updateSearchFound(found);
  stopWait();
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
  QUrl destUrl = action->property("resourceViewUrl").toUrl();
  if (destUrl.isEmpty()) {
    qWarning()<<"KLFLibBrowser::slotCopyMoveToResource(): bad sender property ! sender is a `"
	      <<action->metaObject()->className()<<"'; expected QAction with 'resourceViewUrl' property set.";
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

void KLFLibBrowser::slotCut()
{
  KLFAbstractLibView * view = curLibView();
  if ( view == NULL )
    return;
  if ( view->selectedEntries().size() == 0 ||
       !view->resourceEngine()->canModifyData(KLFLibResourceEngine::DeleteData) )
    return;

  slotCopy();
  view->deleteSelected(false);
}
void KLFLibBrowser::slotCopy()
{
  KLFAbstractLibView * view = curLibView();
  if ( view == NULL )
    return;
  if ( view->selectedEntries().size() == 0 )
    return;

  KLFLibEntryList elist = view->selectedEntries();
  QVariantMap vprops;
  vprops["Url"] = view->url(); // originating URL

  QMimeData *mimeData = KLFAbstractLibEntryMimeEncoder::createMimeData(elist, vprops);
  QApplication::clipboard()->setMimeData(mimeData);
}
void KLFLibBrowser::slotPaste()
{
  KLFAbstractLibView * view = curLibView();
  if ( view == NULL )
    return;

  KLFLibEntryList elist;
  QVariantMap vprops;

  const QMimeData* mimeData = QApplication::clipboard()->mimeData();
  bool result =
    KLFAbstractLibEntryMimeEncoder::decodeMimeData(mimeData, &elist, &vprops);
  if (!result) {
    QMessageBox::critical(this, tr("Error"), tr("The clipboard doesn't contain any appropriate data."));
    return;
  }

  klfDbg( ": Pasting data! props="<<vprops ) ;
  view->insertEntries(elist);
}


void KLFLibBrowser::slotOpenAll()
{
  QStringList exportFilterList;
  QStringList filterlist;
  QString exportFilter;
  QList<KLFLibBasicWidgetFactory::LocalFileType> locfiletypes = KLFLibBasicWidgetFactory::localFileTypes();
  int k;
  for (k = 0; k < locfiletypes.size(); ++k) {
    exportFilterList << locfiletypes[k].filter;
    filterlist << locfiletypes[k].filepattern;
  }
  exportFilterList.prepend(tr("All Known Library Files (%1)").arg(filterlist.join(" ")));
  exportFilterList << tr("All Files (*)");
  exportFilter = exportFilterList.join(";;");
  QString selectedFilter;
  QString fn = QFileDialog::getOpenFileName(this, tr("Open Library File"), QDir::homePath(),
					    exportFilter, &selectedFilter);
  if (fn.isEmpty())
    return;
  int ifilter = exportFilterList.indexOf(selectedFilter);
  ifilter--; // index in locfiletypes now
  QString selectedScheme;
  if (ifilter >= 0 && ifilter < locfiletypes.size()) {
    selectedScheme = locfiletypes[ifilter].scheme;
  } else {
    selectedScheme = KLFLibBasicWidgetFactory::guessLocalFileScheme(fn);
  }
  if (!QFileInfo(fn).isReadable()) {
    qWarning()<<KLF_FUNC_NAME<<": The given file name is not readable: "<<fn;
    QMessageBox::critical(this, tr("Error"), tr("The given file cannot be read: %1").arg(fn));
    return;
  }
  if (selectedScheme.isEmpty()) {
    QMessageBox::critical(this, tr("Error"), tr("Unknown open file scheme!"));
    return;
  }

  QUrl baseUrl = QUrl::fromLocalFile(fn);
  baseUrl.setScheme(selectedScheme);
  QStringList subreslist = KLFLibEngineFactory::listSubResources(baseUrl);
  for (k = 0; k < subreslist.size(); ++k) {
    QUrl url = baseUrl;
    url.addQueryItem("klfDefaultSubResource", subreslist[k]);
    bool r = openResource(url);
    if ( !r )
      QMessageBox::critical(this, tr("Error"), tr("Failed to open resource %1!").arg(url.toString()));
  }
}

bool KLFLibBrowser::slotExport()
{
  /** \bug use any known local-file-based export filter (supporting sub-resources), using a
   * create-local-file-widget */
  const QString exportFilter = tr("Library Database File (*.klf.db);;All Files (*)");
  KLFLibExportDialog dlg(this, exportFilter);
  dlg.exec();
  klfDbg( ": Exporting : "<<dlg.selectedExportUrls() ) ;

  if (dlg.result() != QDialog::Accepted)
    return false;

  QUrl exportToUrl = dlg.exportToUrl();
  QList<QUrl> exportUrls = dlg.selectedExportUrls();

  KLFLibEngineFactory *factory = KLFLibEngineFactory::findFactoryFor(exportToUrl.scheme());
  if (factory == NULL) {
    qWarning()<<KLF_FUNC_NAME<<": No factory found for "<<exportToUrl;
    return false;
  }
  KLFLibWidgetFactory::Parameters param;
  param["Filename"] = exportToUrl.path();
  param["klfScheme"] = exportToUrl.scheme();
  KLFLibResourceEngine *exportRes = factory->createResource(exportToUrl.scheme(), param, this);
  if (exportRes == NULL) {
    QMessageBox::critical(this, tr("Error"),
			  tr("Can't create resource %1!").arg(exportToUrl.path()));
    return false;
  }
  /** \bug  ................................. IMPLEMENT THIS .............................. */
  QMessageBox::information(this, "..............", "NOT YET IMPLEMENTED !!");
  return false;
}

void KLFLibBrowser::slotStartProgress(KLFProgressReporter *progressReporter, const QString& text)
{
  klfDbg( ": min,max="<<progressReporter->min()<<","<<progressReporter->max()
	  <<"; text="<<text ) ;

  KLFProgressDialog *pdlg = new KLFProgressDialog(false, this);

  pdlg->startReportingProgress(progressReporter, text);
  pdlg->setValue(0);

  pdlg->setProperty("klf_libbrowser_pdlg_want_hideautodelete", QVariant(true));
  pdlg->installEventFilter(this);
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


void KLFLibBrowser::startWait()
{
  if (pIsWaiting)
    return;

  u->lblWaitAnimation->show();
  //  u->lblWaitAnimation->movie()->start();
  update();
  qApp->processEvents(QEventLoop::ExcludeUserInputEvents);

  pAnimMovie->jumpToFrame(0);
  pAnimTimerId = startTimer(pAnimMovie->nextFrameDelay());
  pIsWaiting = true;
}
void KLFLibBrowser::stopWait()
{
  if (!pIsWaiting)
    return;

  //  u->lblWaitAnimation->movie()->stop();
  u->lblWaitAnimation->hide();

  killTimer(pAnimTimerId);
  pAnimTimerId = -1;
  pIsWaiting = false;
}

void KLFLibBrowser::timerEvent(QTimerEvent *event)
{
  if (event->timerId() == pAnimTimerId) {
    pAnimMovie->jumpToNextFrame();
    u->lblWaitAnimation->setPixmap(pAnimMovie->currentPixmap());
    u->lblWaitAnimation->repaint();
  } else {
    QWidget::timerEvent(event);
  }
}

void KLFLibBrowser::showEvent(QShowEvent *event)
{
  // update some last-minute stuff
  // like the tab colors (which could have changed upon setting a skin)
  int k;
  for (k = 0; k < pLibViews.size(); ++k) {
    KLFLibBrowserViewContainer *viewc = pLibViews[k];
    KLFLibResourceEngine *resource = viewc->resourceEngine();
    u->tabResources->refreshTabReadOnly(u->tabResources->indexOf(viewc),
					!resource->canModifyData(KLFLibResourceEngine::AllActionsData));
  }
  // and call superclass
  QWidget::showEvent(event);
}

