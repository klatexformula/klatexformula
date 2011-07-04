/***************************************************************************
 *   file klflibbrowser.cpp
 *   This file is part of the KLatexFormula Project.
 *   Copyright (C) 2011 by Philippe Faist
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
#include <QLineEdit>
#include <QProgressDialog>
#include <QPushButton>
#include <QApplication>
#include <QClipboard>
#include <QDesktopServices>

#include "klfconfig.h"
#include <klfguiutil.h>
#include "klflibbrowser_p.h"
#include "klflibbrowser.h"
#include <ui_klflibbrowser.h>
#include "klfliblegacyengine.h"


KLFLibBrowser::KLFLibBrowser(QWidget *parent)
  : QWidget(
#if defined(Q_WS_WIN) || defined(Q_WS_MAC)
	    0 /* parent */
#else
	    parent /* 0 */
#endif
	    , Qt::Window)
{
  KLF_DEBUG_BLOCK(KLF_FUNC_NAME) ;

  u = new Ui::KLFLibBrowser;
  u->setupUi(this);
  u->tabResources->setContextMenuPolicy(Qt::CustomContextMenu);

  KLF_DEBUG_ASSIGN_REF_INSTANCE(u->searchBar, "libbrowser-searchbar") ;
  u->searchBar->registerShortcuts(this);
  // set found/not-found colors
  u->searchBar->setColorFound(klfconfig.LibraryBrowser.colorFound);
  u->searchBar->setColorNotFound(klfconfig.LibraryBrowser.colorNotFound);

  pResourceMenu = new QMenu(u->tabResources);
  // connect actions
  connect(u->aRename, SIGNAL(triggered()), this, SLOT(slotResourceRename()));
  connect(u->aRenameSubRes, SIGNAL(triggered()), this, SLOT(slotResourceRenameSubResource()));
  connect(u->aProperties, SIGNAL(triggered()), this, SLOT(slotResourceProperties()));
  connect(u->aNewSubRes, SIGNAL(triggered()), this, SLOT(slotResourceNewSubRes()));
  connect(u->aDelSubRes, SIGNAL(triggered()), this, SLOT(slotResourceDelSubRes()));
  connect(u->aSaveTo, SIGNAL(triggered()), this, SLOT(slotResourceSaveTo()));
  connect(u->aNew, SIGNAL(triggered()), this, SLOT(slotResourceNew()));
  connect(u->aOpen, SIGNAL(triggered()), this, SLOT(slotResourceOpen()));
  connect(u->aClose, SIGNAL(triggered()), this, SLOT(slotResourceClose()));
  // and add them to menu
  pResourceMenu->addAction((new KLFLibBrowserTabMenu(u->tabResources))->menuAction());
  pResourceMenu->addSeparator();
  pResourceMenu->addAction(u->aRename);
  pResourceMenu->addAction(u->aRenameSubRes);
  pResourceMenu->addAction(u->aProperties);
  pResourceMenu->addSeparator();
  pResourceMenu->addAction(u->aViewType);
  pResourceMenu->addSeparator();
  pResourceMenu->addAction(u->aNewSubRes);
  pResourceMenu->addAction(u->aOpenSubRes);
  pResourceMenu->addAction(u->aDelSubRes);
  pResourceMenu->addSeparator();
  pResourceMenu->addAction(u->aNew);
  pResourceMenu->addAction(u->aOpen);
  /// \todo .......... save as copy action ............
  //  pResourceMenu->addAction(u->aSaveTo);
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
  pImportExportMenu->addAction(u->aExportSelection);
  u->btnImportExport->setMenu(pImportExportMenu);

  connect(u->aOpenAll, SIGNAL(triggered()), this, SLOT(slotOpenAll()));
  connect(u->aExport, SIGNAL(triggered()), this, SLOT(slotExport()));
  connect(u->aExportSelection, SIGNAL(triggered()), this, SLOT(slotExportSelection()));
  

  // CATEGORY/TAGS

  connect(u->wEntryEditor, SIGNAL(metaInfoChanged(const QMap<int,QVariant>&)),
	  this, SLOT(slotMetaInfoChanged(const QMap<int,QVariant>&)));

  connect(u->wEntryEditor, SIGNAL(restoreStyle(const KLFStyle&)),
	  this, SIGNAL(requestRestoreStyle(const KLFStyle&)));

  // OPEN / NEW BUTTONS IN WELCOME TAB

  connect(u->btnOpenRes, SIGNAL(clicked()), this, SLOT(slotResourceOpen()));
  connect(u->btnCreateRes, SIGNAL(clicked()), this, SLOT(slotResourceNew()));

  // SHORTCUTS
		      
  // cut/copy/paste
  (void)new QShortcut(QKeySequence::Delete, this, SLOT(slotDeleteSelected()));
  (void)new QShortcut(QKeySequence::Cut, this, SLOT(slotCut()));
  (void)new QShortcut(QKeySequence::Copy, this, SLOT(slotCopy()));
  (void)new QShortcut(QKeySequence::Paste, this, SLOT(slotPaste()));

  retranslateUi(false);

  // start with no entries selected
  slotEntriesSelected(QList<KLFLibEntry>());
}

void KLFLibBrowser::retranslateUi(bool alsoBaseUi)
{
  if (alsoBaseUi)
    u->retranslateUi(this);

  u->wEntryEditor->retranslateUi(alsoBaseUi);

  pResourceMenu->setTitle(tr("Resource Actions", "[[menu title]]"));
  u->searchBar->setFocusOutText("  "+tr("Hit Ctrl-F, Ctrl-S or / to search within the current resource"));
  pTabCornerButton->setText(tr("Resource"));
}


KLFLibBrowser::~KLFLibBrowser()
{
  KLF_DEBUG_BLOCK(KLF_FUNC_NAME) ;

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
  KLF_DEBUG_BLOCK(KLF_FUNC_NAME) ;

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
    klfDbg( ": progress dialog was hidden, deleting." ) ;
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
  if (i < 0 || i >= pLibViews.size())
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
    bool res = openResourceFromGuiState(url, viewState);
    // ignore flags, in case we eg. change library file, don't keep the old ones uncloseable
    // updateResourceRoleFlags(..., flags);
    if ( ! res ) {
      qWarning()<<"KLFLibBrowser::loadGuiState: Can't open resource "<<url<<"! (flags="
		<<flags<<")";
      continue;
    }
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
  KLF_DEBUG_BLOCK(KLF_FUNC_NAME) ;

  KLF_ASSERT_NOT_NULL( resource, "Resource is NULL!", return QString() ) ;

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
    klfDbg("\t\ttest lib view #"<<k) ;
    klfDbg("\t\turl="<<pLibViews[k]->url() ) ;
    // allow: * urls to be equal
    //        * this resource's URL to be less specific (shows more) than what we're searching for
    uint fl = pLibViews[k]->view()->compareUrlTo(url, KlfUrlCompareEqual|KlfUrlCompareLessSpecific);
    if (fl & KlfUrlCompareEqual ||
	fl & KlfUrlCompareLessSpecific) {
      klfDbg( ": Found!" ) ;
      return pLibViews[k];
    }
  }
  return NULL;
}

KLFLibBrowserViewContainer * KLFLibBrowser::findOpenResource(KLFLibResourceEngine *resource)
{
  KLF_DEBUG_BLOCK(KLF_FUNC_NAME) ;

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
QList<KLFLibBrowserViewContainer*> KLFLibBrowser::findByRoleFlags(uint testflags, uint mask)
{
  klfDbg("Looking for flags "<<klfFmtCC("%#010x", testflags)<<" with mask "<<klfFmtCC("%#010x", mask)) ;
  QList<KLFLibBrowserViewContainer *> list;
  int k;
  for (k = 0; k < pLibViews.size(); ++k) {
    if ((pLibViews[k]->resourceRoleFlags() & mask) == testflags) {
      klfDbg("Adding #"<<k<<": "<<pLibViews[k]<<", url="<<pLibViews[k]->url()) ;
      list << pLibViews[k];
    }
  }
  return list;
}



bool KLFLibBrowser::openResource(const QString& url, uint resourceRoleFlags,
				 const QString& viewTypeIdentifier)
{
  KLF_DEBUG_TIME_BLOCK(KLF_FUNC_NAME+QString("(QString,uint,QString)")) ;
  return openResource(QUrl(url), resourceRoleFlags, viewTypeIdentifier);
}

bool KLFLibBrowser::openResource(const QUrl& url, uint resourceRoleFlags,
				 const QString& viewTypeIdentifier)
{
  KLF_DEBUG_TIME_BLOCK(KLF_FUNC_NAME+"(QUrl,uint,QString)") ;
  klfDbg( " url="<<url.toString()<<"; resroleflags="<<resourceRoleFlags
	  <<"; vtypeidentifier="<<viewTypeIdentifier ) ;

  KLFLibBrowserViewContainer * openview = findOpenUrl(url);
  if (openview != NULL) {
    qDebug("KLFLibBrowser::openResource(%s,%u): This resource is already open.",
	   qPrintable(url.toString()), resourceRoleFlags);
    if ((resourceRoleFlags & OpenNoRaise) == 0)
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
  KLF_DEBUG_TIME_BLOCK(KLF_FUNC_NAME+"(KLFLibResourceEngine*,uint,QString)") ;
  klfDbg( "\topening resource url="
	  <<resource->url(KLFLibResourceEngine::WantUrlDefaultSubResource).toString() ) ;

  //  KLFPleaseWaitPopup label(tr("Loading resource, please wait..."), this);

  KLF_ASSERT_NOT_NULL(resource,
		      "resource pointer is NULL! (flags="<<resourceRoleFlags<<",vti="<<viewTypeIdentifier<<")",
		      return false );

  KLFLibBrowserViewContainer * openview = findOpenResource(resource);
  if (openview != NULL) {
    qDebug("KLFLibBrowser::openResource(%p,%u): This resource is already open.",
	   resource, resourceRoleFlags);
    if ((resourceRoleFlags & OpenNoRaise) == 0)
      u->tabResources->setCurrentWidget(openview);
    updateResourceRoleFlags(openview, resourceRoleFlags);
    return true;
  }

  resource->setParent(this);

  klfDbgT(": created resource. about to create view container.") ;

  // now create appropriate view for this resource
  KLFLibBrowserViewContainer *viewc = new KLFLibBrowserViewContainer(resource, u->tabResources);

  klfDbgT(": adding tab page....") ;

  int i = u->tabResources->addTab(viewc, displayTitle(resource));
  if ((resourceRoleFlags & OpenNoRaise) == 0)
    u->tabResources->setCurrentWidget(viewc);
  u->tabResources
    ->refreshTabReadOnly(i, !resource->canModifyData(KLFLibResourceEngine::AllActionsData));
  pLibViews.append(viewc);
  setStyleSheet(styleSheet());
  updateResourceRoleFlags(viewc, resourceRoleFlags);


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

  connect(viewc, SIGNAL(requestOpenUrl(const QString&)),
	  this, SLOT(openResource(const QString&)));

  connect(viewc, SIGNAL(resourceDataChanged(const QList<KLFLib::entryId>&)),
	  this, SLOT(slotResourceDataChanged(const QList<KLFLib::entryId>&)));
  connect(resource, SIGNAL(resourcePropertyChanged(int)),
	  this, SLOT(slotResourcePropertyChanged(int)));
  connect(resource, SIGNAL(subResourcePropertyChanged(const QString&, int)),
	  this, SLOT(slotSubResourcePropertyChanged(const QString&, int)));
  connect(resource, SIGNAL(defaultSubResourceChanged(const QString&)),
	  this, SLOT(slotDefaultSubResourceChanged(const QString&)));

  // progress reporting originating from resource (eg. database operations)
  connect(resource, SIGNAL(operationStartReportingProgress(KLFProgressReporter *, const QString&)),
	  this, SLOT(slotStartProgress(KLFProgressReporter *, const QString&)));
  // progress reporting originating from view (eg. model updates)
  connect(viewc, SIGNAL(viewOperationStartReportingProgress(KLFProgressReporter *, const QString&)),
	  this, SLOT(slotStartProgress(KLFProgressReporter *, const QString&)));

  // supply a context menu to view
  connect(viewc, SIGNAL(viewContextMenuRequested(const QPoint&)),
	  this, SLOT(slotShowContextMenu(const QPoint&)));


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
  //   * the resource view type property
  viewtypeident_try << resource->viewType();
  //   * the suggested view type for this resource (as given by engine itself)
  viewtypeident_try << resource->suggestedViewTypeIdentifier();
  //   * the default view type suggested by the factory
  viewtypeident_try << KLFLibViewFactory::defaultViewTypeIdentifier();
  //   * a "default" view type (last resort, hoping it exists!)
  viewtypeident_try << QLatin1String("default");

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
      klfDbg( "can't find view factory for view type identifier "
	      <<viewtypeident_try[k]<<"!" ) ;
      continue;
    }
    if ( ! viewfactory->canCreateLibView(viewtypeident_try[k], resource) ) {
      klfDbg( "incompatible view type identifier "<<viewtypeident_try[k]
	      <<"for resource "<<resource->url()<<"." ) ;
      continue;
    }
    bool r = viewc->openView(viewtypeident_try[k]);
    if (!r) {
      klfDbg( "can't create view! viewtypeident="<<viewtypeident_try[k]<<"." ) ;
      continue;
    }

    klfDbgT(": found and instantiated good view type="<<viewtypeident_try[k]) ;

    // found good view type !
    resource->setViewType(viewtypeident_try[k]);
    // quit for() on first success.
    break;
  }

  klfDbgT(": requiring cat suggestions.") ;

  // get more category completions
  viewc->view()->wantMoreCategorySuggestions();

  // hide welcome page if it's shown
  if ((i = u->tabResources->indexOf(u->tabWelcome)) != -1)
    u->tabResources->removeTab(i);

  klfDbgT(": end of function") ;

  return true;
}

bool KLFLibBrowser::openResourceFromGuiState(const QUrl& url, const QVariantMap& guiState)
{
  KLF_DEBUG_TIME_BLOCK(KLF_FUNC_NAME) ;
  // peek into GUI state to open the correct view
  QString vti = guiState.value(QLatin1String("CurrentViewTypeIdentifier")).toString();
  klfDbg("view-type-identifier is "<<vti<<"; guiState is "<<guiState) ;
  bool result = openResource(url, NoChangeFlag, vti);
  if (!result)
    return false;
  klfDbg("restoring gui state..") ;
  KLFLibBrowserViewContainer *viewc = findOpenUrl(url);
  KLF_ASSERT_NOT_NULL( viewc, "can't find the view container we just opened!", return false ) ;
  viewc->loadGuiState(guiState);
  return true;
}


bool KLFLibBrowser::closeResource(const QUrl& url)
{
  KLF_DEBUG_BLOCK(KLF_FUNC_NAME) ;

  KLFLibBrowserViewContainer * w = findOpenUrl(url);
  if (w == NULL)
    return false;

  return slotResourceClose(w);
}


void KLFLibBrowser::updateResourceRoleFlags(KLFLibBrowserViewContainer *viewc, uint resroleflags)
{
  KLF_DEBUG_BLOCK(KLF_FUNC_NAME) ;

  KLF_ASSERT_NOT_NULL(viewc, "the viewc parameter is null!", return; ) ;

  if (resroleflags & NoChangeFlag)
    return;

  // only store flags that don't act 'now'
  resroleflags = resroleflags & ~NowMask;

  klfDbg("updating flags for resource="<<viewc->url()<<"; flags after mask="
	 <<klfFmtCC("%#010x", resroleflags)) ;

  // If history resroleflags is set, then unset that flag to its previous owner (history role flag
  // is exclusive).
  // Same respectively for archive flag.
  uint exclusive_flag_list[] = { HistoryRoleFlag, ArchiveRoleFlag, 0 } ;
  int j;
  for (j = 0; exclusive_flag_list[j] != 0; ++j) {
    uint xflag = exclusive_flag_list[j];
    if (resroleflags & xflag) {
      // If we are about to set this flag to resource 'viewc', then make sure no one (else) has it
      int k;
      for (k = 0; k < pLibViews.size(); ++k) {
	if (pLibViews[k] == viewc)
	  continue; // skip our targeted view
	uint fl = pLibViews[k]->resourceRoleFlags();
	if (fl & xflag) {
	  // unset it
	  pLibViews[k]->setResourceRoleFlags(fl & ~xflag);
	}
      }
    }
  }

  // now we can set the appropriate flags on the requested resource view
  viewc->setResourceRoleFlags(resroleflags);
}


void KLFLibBrowser::slotTabResourceShown(int tabIndex)
{
  KLF_DEBUG_TIME_BLOCK(KLF_FUNC_NAME) ;
  klfDbg( "\t tabIndex="<<tabIndex ) ;

  KLFLibBrowserViewContainer * viewc =
    qobject_cast<KLFLibBrowserViewContainer*>(u->tabResources->widget(tabIndex));
  if (viewc == NULL || tabIndex < 0) {
    emit resourceTabChanged(QUrl());
    return;
  }

  // redirect searches to the correct view
  u->searchBar->setSearchTarget(viewc);

  klfDbg("setting up view type menu...") ;

  // set up view type menu appropriately
  QList<QAction*> actions = viewc->viewTypeActions();
  QMenu *viewTypeMenu = u->aViewType->menu();
  if (viewTypeMenu == NULL) {
    viewTypeMenu = new QMenu(this);
    u->aViewType->setMenu(viewTypeMenu);
    KLF_DEBUG_WATCH_OBJECT(viewTypeMenu) ;
  }
  viewTypeMenu->clear();
  int k;
  for (k = 0; k < actions.size(); ++k) {
    viewTypeMenu->addAction(actions[k]);
  }

  QMenu * openSubResMenu = u->aOpenSubRes->menu();
  if (openSubResMenu == NULL) {
    openSubResMenu = new QMenu(this);
    u->aOpenSubRes->setMenu(openSubResMenu);
    KLF_DEBUG_WATCH_OBJECT(openSubResMenu) ;
  }
  openSubResMenu->clear();
  QList<QAction*> openSubResActions = viewc->openSubResourceActions();
  for (k = 0; k < openSubResActions.size(); ++k) {
    openSubResMenu->addAction(openSubResActions[k]);
  }
  if (openSubResActions.size() > 0)
    u->aOpenSubRes->setEnabled(true);
  else
    u->aOpenSubRes->setEnabled(false);

  // refresh selection-related displays
  KLFAbstractLibView *view = viewc->view();
  if (view != NULL)
    slotEntriesSelected(view->selectedEntries());
  slotRefreshResourceActionsEnabled();

  emit resourceTabChanged(viewc->url());
}

void KLFLibBrowser::slotShowTabContextMenu(const QPoint& pos)
{
  KLF_DEBUG_BLOCK(KLF_FUNC_NAME) ;

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
  KLF_DEBUG_BLOCK(KLF_FUNC_NAME) ;
  klfDbg("renamingSubResource="<<renamingSubResource) ;

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
  KLF_DEBUG_BLOCK(KLF_FUNC_NAME) ;

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

bool KLFLibBrowser::slotResourceClose(KLFLibBrowserViewContainer *view, bool force)
{
  KLF_DEBUG_BLOCK(KLF_FUNC_NAME) ;

  if (view == NULL)
    view = curView();
  if (view == NULL)
    return false;

  if (!force && view->resourceRoleFlags() & NoCloseRoleFlag) // sorry, can't close.
    return false;

  klfDbg( "Close! resflags="<<view->resourceRoleFlags() ) ;

  int tabindex = u->tabResources->indexOf(view);
  if (tabindex < 0) {
    qWarning("KLFLibBrowser::closeResource(url): can't find view in tab widget?!?\n"
	     "\turl=%s, viewwidget=%p", qPrintable(view->url().toString()), view);
    return false;
  }

  if (!force && klfconfig.LibraryBrowser.confirmClose) {
    // ask user for confirmation
    QMessageBox::StandardButton btn =
      QMessageBox::question(this, tr("Close Resource"), tr("Do you want to close this resource?"),
			    QMessageBox::Yes|QMessageBox::Cancel, QMessageBox::Yes);
    if (btn != QMessageBox::Yes)
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
  KLF_DEBUG_BLOCK(KLF_FUNC_NAME) ;

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
  KLF_DEBUG_BLOCK(KLF_FUNC_NAME) ;

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

bool KLFLibBrowser::slotResourceDelSubRes()
{
  KLF_DEBUG_BLOCK(KLF_FUNC_NAME) ;

  KLFLibBrowserViewContainer *viewc = curView();
  KLF_ASSERT_NOT_NULL(viewc , "NULL View Container!" , return false; ) ;
  KLFAbstractLibView *view = viewc->view();
  KLF_ASSERT_NOT_NULL(view , "NULL View in container "<<viewc<<" !" , return false; ) ;
  KLFLibResourceEngine *res = view->resourceEngine();
  KLF_ASSERT_NOT_NULL(res , "NULL resource for view="<<view<<" !" , return false; ) ;

  KLF_ASSERT_CONDITION( res->supportedFeatureFlags() & KLFLibResourceEngine::FeatureSubResources ,
			"Sub-resources are not supported in resource "<<res->url()<<"!"
			" Cannot delete sub-resource!",
			return false; ) ;

  if (res->subResourceList().size() <= 1) {
    QMessageBox::warning(this, tr("Error"),
			 tr("You may not delete the last remaining sub-resource of this resource."));
    klfDbg("Attempted to delete last remaining sub-resource"<<res->defaultSubResource()<<" of resource "
	   <<res->url()<<". Not allowed.") ;
    return false;
  }

  QString curSubResource = res->defaultSubResource();
  QString curSubResTitle = curSubResource;
  if (res->supportedFeatureFlags() & KLFLibResourceEngine::FeatureSubResourceProps) {
    QString t = res->subResourceProperty(curSubResource, KLFLibResourceEngine::SubResPropTitle).toString();
    if (!t.isEmpty())
      curSubResTitle = t;
  }

  // remove the current sub-resource: first ask user confirmation

  QMessageBox::StandardButton btn =
    QMessageBox::question(this, tr("Delete Sub-Resource", "[[msgbox title]]"),
			  tr("Do you really want to delete the sub-resource <b>%1</b>, "
			     "with all its contents, from resource <b>%2</b>?")
			  .arg(curSubResTitle, displayTitle(res)),
			  QMessageBox::Yes|QMessageBox::Cancel, QMessageBox::Cancel);
  if (btn != QMessageBox::Yes)
    return false;

  // now remove that sub-resource
  bool result = res->deleteSubResource(curSubResource);

  if (!result) {
    // report failure
    QMessageBox::critical(this, tr("Delete Sub-Resource", "[[msgbox title]]"),
			  tr("Deleting sub-resource failed."));
    return false;
  }

  // close the corresponding tab.
  slotResourceClose(viewc, true);
  return true;
}

bool KLFLibBrowser::slotResourceOpen()
{
  KLF_DEBUG_BLOCK(KLF_FUNC_NAME) ;

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
  KLF_DEBUG_BLOCK(KLF_FUNC_NAME) ;

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
  KLF_DEBUG_BLOCK(KLF_FUNC_NAME) ;

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
  KLF_DEBUG_BLOCK(KLF_FUNC_NAME) ;

  KLFLibResourceEngine *resource = qobject_cast<KLFLibResourceEngine*>(sender());
  if (resource == NULL) {
    qWarning("KLFLibBrowser::slotResourcePropertyChanged: NULL sender or not resource!");
    return;
  }
  slotUpdateForResourceProperty(resource, propId);
}

void KLFLibBrowser::slotUpdateForResourceProperty(KLFLibResourceEngine *resource, int propId)
{
  KLF_DEBUG_BLOCK(KLF_FUNC_NAME) ;
  klfDbg("res="<<resource<<", propId="<<propId) ;

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

void KLFLibBrowser::slotSubResourcePropertyChanged(const QString& subResource, int propId)
{
  KLF_DEBUG_BLOCK(KLF_FUNC_NAME) ;
  Q_UNUSED(subResource) ;
  klfDbg("subResource="<<subResource<<", propId="<<propId) ;

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

void KLFLibBrowser::slotDefaultSubResourceChanged(const QString& subResource)
{
  KLF_DEBUG_BLOCK(KLF_FUNC_NAME) ;
  Q_UNUSED(subResource) ;
  klfDbg("subResource="<<subResource) ;

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
  KLF_DEBUG_BLOCK(KLF_FUNC_NAME) ;

  KLFAbstractLibView * view = curLibView();
  if ( view == NULL )
    return;
  view->restoreWithStyle();
}

void KLFLibBrowser::slotRestoreLatexOnly()
{
  KLF_DEBUG_BLOCK(KLF_FUNC_NAME) ;

  KLFAbstractLibView * view = curLibView();
  if ( view == NULL )
    return;
  view->restoreLatexOnly();
}

void KLFLibBrowser::slotDeleteSelected()
{
  KLF_DEBUG_BLOCK(KLF_FUNC_NAME) ;

  KLFAbstractLibView * view = curLibView();
  if ( view == NULL )
    return;
  KLFLibResourceEngine * resource = view->resourceEngine();
  if ( !resource->canModifyData(KLFLibResourceEngine::DeleteData) )
    return;

  QList<KLFLib::entryId> sel = view->selectedEntryIds();
  klfDbg("selected "<<sel.size()<<" items:" <<sel);

  if (sel.isEmpty())
    return;

  QMessageBox::StandardButton res
    = QMessageBox::question(this, tr("Delete?"),
			    tr("Delete %n selected item(s) from resource \"%1\"?", "", sel.size())
			    .arg(resource->title()),
			    QMessageBox::Yes|QMessageBox::Cancel, QMessageBox::Cancel);
  if (res != QMessageBox::Yes)
    return; // abort action
  
  resource->deleteEntries(sel);
}

void KLFLibBrowser::slotRefreshResourceActionsEnabled()
{
  KLF_DEBUG_BLOCK(KLF_FUNC_NAME) ;

  bool master = false;
  bool canrename = false;
  bool canrenamesubres = false;
  bool cansaveto = false;
  bool cannewsubres = false;
  bool candelsubres = false;
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
    candelsubres = (resfeatureflags & KLFLibResourceEngine::FeatureSubResources) &&
      res->canDeleteSubResource(res->defaultSubResource()) ;
    canrenamesubres = (resfeatureflags & KLFLibResourceEngine::FeatureSubResources) &&
      (resfeatureflags & KLFLibResourceEngine::FeatureSubResourceProps) &&
      res->canModifySubResourceProperty(res->defaultSubResource(),
					KLFLibResourceEngine::SubResPropTitle) ;
  }

  u->aRename->setEnabled(canrename);
  u->aRenameSubRes->setEnabled(canrenamesubres);
  u->aProperties->setEnabled(master);
  u->aNewSubRes->setEnabled(master && cannewsubres);
  u->aDelSubRes->setEnabled(master && candelsubres);
  u->aSaveTo->setEnabled(master && cansaveto);
  u->aNew->setEnabled(true);
  u->aOpen->setEnabled(true);
  u->aClose->setEnabled(master && !(resrolefl & NoCloseRoleFlag));
}



void KLFLibBrowser::slotEntriesSelected(const KLFLibEntryList& entries)
{
  KLF_DEBUG_BLOCK(KLF_FUNC_NAME) ;
  klfDbg( "(): "<<entries ) ;
  if (entries.size()>=1)
    klfDbg( "Tag of first selected entry="<<entries[0].property(KLFLibEntry::Tags) ) ;

  KLFAbstractLibView *view = curLibView();
  if (view != NULL) {
    u->wEntryEditor
      ->setInputEnabled(view->resourceEngine()->canModifyData(KLFLibResourceEngine::ChangeData));
  }

  u->wEntryEditor->displayEntries(entries);

  u->btnDelete->setEnabled(entries.size() > 0);
  u->aDelete->setEnabled(u->btnDelete->isEnabled());
  u->btnRestore->setEnabled(entries.size() == 1);

  emit libEntriesSelected(entries);
}

void KLFLibBrowser::slotAddCategorySuggestions(const QStringList& catlist)
{
  klfDbg( "KLFLibBrowser: got category suggestions: "<<catlist ) ;
  u->wEntryEditor->addCategorySuggestions(catlist);
}

void KLFLibBrowser::slotShowContextMenu(const QPoint& pos)
{
  KLF_DEBUG_BLOCK(KLF_FUNC_NAME) ;

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
				  this, SLOT(slotDeleteSelected()), QKeySequence::Delete);
  menu->addSeparator();

  QMenu *copytomenu = new QMenu(menu);
  QMenu *movetomenu = new QMenu(menu);
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



void KLFLibBrowser::slotMetaInfoChanged(const QMap<int,QVariant>& props)
{
  KLF_DEBUG_BLOCK(KLF_FUNC_NAME) ;

  QWidget *w = u->tabResources->currentWidget();
  KLFLibBrowserViewContainer *wviewc = qobject_cast<KLFLibBrowserViewContainer*>(w);
  if (wviewc == NULL) {
    qWarning("Current view is not a KLFLibBrowserViewContainer or no current tab widget!");
    return;
  }
  if (props.isEmpty()) {
    klfDbg("no changes!");
    return;
  }
  KLFAbstractLibView * wview = wviewc->view();
  if ( wview == NULL )
    return;
  QList<KLFLib::entryId> selected = wview->selectedEntryIds();
  KLFLibResourceEngine *resource = wview->resourceEngine();
  QList<int> keys = props.keys();
  QList<QVariant> values;
  int k;
  for (k = 0; k < keys.size(); ++k)
    values << props[keys[k]];
  bool r = resource->changeEntries(selected, keys, values);
  if ( ! r ) {
    QMessageBox::warning(this, tr("Error"),
			 tr("Failed to write meta-information!"));
    // and refresh display
    slotEntriesSelected(wview->selectedEntries());
    return;
  }

  // ensure that exactly the modified items are selected, as view's refresh mechanism does
  // not garantee to preserve selection
  wview->selectEntries(selected);

  // if we categorized/tagged a formula in the history resource, copy it to the archive if the relevant
  // setting is enabled
  if (klfconfig.LibraryBrowser.historyTagCopyToArchive && !props.isEmpty()) {
    KLFLibBrowserViewContainer *vHistory = findSpecialResource(HistoryRoleFlag);
    KLFLibBrowserViewContainer *vArchive = findSpecialResource(ArchiveRoleFlag);
    klfDbg("vHistory="<<vHistory<<", vArchive="<<vArchive<<", wviewc="<<wviewc) ;
    if (vHistory != NULL && vArchive != NULL && vHistory != vArchive  &&
	vHistory == wviewc) {
      klfDbg("categorized formula in history. copying it to archive");
      // copy the formula from history to archive
      KLFLibEntryList entryList = wview->selectedEntries();
      if (entryList.isEmpty()) {
	qWarning()<<KLF_FUNC_NAME<<": no selected entries ?!?";
      } else {
	KLFAbstractLibView *archiveView = vArchive->view();
	KLFLibResourceEngine *archiveRes = vArchive->resourceEngine();
	if (archiveRes == NULL || archiveView == NULL) {
	  qWarning()<<KLF_FUNC_NAME<<": archiveRes or archiveView is NULL ?!?";
	} else {
	  QList<KLFLib::entryId> insertedIds = archiveRes->insertEntries(entryList);
	  if (!insertedIds.size() || insertedIds.contains(-1)) {
	    QMessageBox::critical(this, tr("Error"), tr("Error copying the given items to the archive!"));
	  } else {
	    u->tabResources->setCurrentWidget(vArchive);
	    // and select those items in archive that were inserted.
	    archiveView->selectEntries(insertedIds);
	  }
	}
      }
    }
  }
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
  KLF_DEBUG_BLOCK(KLF_FUNC_NAME) ;

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
  KLF_DEBUG_BLOCK(KLF_FUNC_NAME) ;

  QList<KLFLib::entryId> selectedids = source->selectedEntryIds();
  KLFLibEntryList items = source->selectedEntries();

  QList<int> inserted = dest->resourceEngine()->insertEntries(items);
  if ( inserted.isEmpty() || inserted.contains(-1) ) {
    QString msg = move ? tr("Failed to move the selected items.")
      : tr("Failed to copy the selected items.");
    QMessageBox::critical(this, tr("Error"), msg, QMessageBox::Ok, QMessageBox::Ok);
    return;
  }
  if (move)
    source->resourceEngine()->deleteEntries(selectedids);
}

void KLFLibBrowser::slotCut()
{
  KLF_DEBUG_BLOCK(KLF_FUNC_NAME) ;

  KLFAbstractLibView * view = curLibView();
  if ( view == NULL )
    return;
  if ( view->selectedEntries().size() == 0 ||
       !view->resourceEngine()->canModifyData(KLFLibResourceEngine::DeleteData) )
    return;

  QList<KLFLib::entryId> selected = view->selectedEntryIds();
  slotCopy();
  view->resourceEngine()->deleteEntries(selected);
}
void KLFLibBrowser::slotCopy()
{
  KLF_DEBUG_BLOCK(KLF_FUNC_NAME) ;

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
  KLF_DEBUG_BLOCK(KLF_FUNC_NAME) ;

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
  QList<KLFLib::entryId> inserted = view->resourceEngine()->insertEntries(elist);
  if (inserted.isEmpty() || inserted.contains(-1)) {
    QMessageBox::critical(this, tr("Error"), tr("Error pasting items"));
  }
}


void KLFLibBrowser::slotOpenAll()
{
  KLF_DEBUG_BLOCK(KLF_FUNC_NAME) ;

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
  QString dir = klfconfig.LibraryBrowser.lastFileDialogPath;
  QString fn = QFileDialog::getOpenFileName(this, tr("Open Library File"), dir, exportFilter, &selectedFilter);
  if (fn.isEmpty())
    return;

  klfconfig.LibraryBrowser.lastFileDialogPath = QFileInfo(fn).absolutePath();

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
  KLF_DEBUG_TIME_BLOCK(KLF_FUNC_NAME) ;

  QList<QUrl> exportUrls;
  KLFLibResourceEngine *exportRes = KLFLibExportDialog::showExportDialogCreateResource(this, &exportUrls);
  if (exportRes == NULL) {
    return false;
  }
  exportRes->setTitle(tr("Export %1").arg(QDateTime::currentDateTime()
					  .toString(Qt::DefaultLocaleShortDate)));

  klfDbg("Export: to resource "<<exportRes->url().toString()<<". Export: "<<exportUrls);

  // visual feedback for export
  KLFProgressDialog pdlg(QString(), this);
  connect(exportRes, SIGNAL(operationStartReportingProgress(KLFProgressReporter *,
							    const QString&)),
	  &pdlg, SLOT(startReportingProgress(KLFProgressReporter *)));
  pdlg.setAutoClose(false);
  pdlg.setAutoReset(false);

  bool fail = false;
  QStringList subresources;
  int k;
  for (k = 0; k < exportUrls.size(); ++k) {
    klfDbg("Exporting "<<exportUrls[k]<<" ...");
    QUrl u = exportUrls[k];
    QString usr = u.hasQueryItem("klfDefaultSubResource")
      ? u.queryItemValue("klfDefaultSubResource")
      : QString();
    QString sr = usr;
    if (usr.isEmpty()) {
      // the resource doesn't support sub-resources
      usr = u.path().section('/', -1, -1, QString::SectionSkipEmpty); // last path element
    }
    usr.replace(QRegExp("[^a-zA-Z0-9_-]"), "_");
    QString subres = usr;
    int counter = 1;
    while (subresources.contains(subres))
      subres = usr+"_"+QString::number(counter++);
    // subres is the name of the subresource we will create
    bool r = exportRes->createSubResource(subres);
    if (!r) {
      fail = true;
      qWarning()<<KLF_FUNC_NAME<<" exporting "<<u<<" failed: can't create sub-resource "<<subres<<"!";
      continue;
    }
    subresources.append(subres);
    KLFLibResourceEngine *res = getOpenResource(u);
    if (res == NULL) {
      fail = true;
      qWarning()<<KLF_FUNC_NAME<<" can't find open resource="<<u<<" !";
      continue;
    }
    QString title;
    if (usr.isEmpty()) {
      title = res->title();
    } else {
      if (res->supportedFeatureFlags() & KLFLibResourceEngine::FeatureSubResourceProps) {
	title = res->title() + ": " +
	  res->subResourceProperty(usr, KLFLibResourceEngine::SubResPropTitle).toString();
      } else {
	title = res->title() + ": " + usr;
      }
    }
    exportRes->setSubResourceProperty(subres, KLFLibResourceEngine::SubResPropTitle, title);

    pdlg.setDescriptiveText(tr("Exporting ... %3 (%1/%2)")
			    .arg(k+1).arg(exportUrls.size()).arg(title));

    QList<KLFLibResourceEngine::KLFLibEntryWithId> elistwid = res->allEntries(usr);
    /** \bug ....... Add a 'KLFLibEntryList KLFLibResourceEngine::entryList(idList)'
     * function so that we don't have to manually get rid of ID's ! */
    KLFLibEntryList elist;
    int j;
    for (j = 0; j < elistwid.size(); ++j)
      elist << elistwid[j].entry;

    QList<KLFLib::entryId> insertedIds = exportRes->insertEntries(subres, elist);
    if (!insertedIds.size() || insertedIds.contains(-1)) {
      QMessageBox::critical(this, tr("Error"), tr("Error exporting items!"));
    }
  }

  // remove our dummy sub-resource that was created with the resource
  // name was set in klflibbrowser_p.h: KLFLibExportDialog::showExportDi...()
  exportRes->deleteSubResource(QLatin1String("export_xtra"));

  // important, as it will cause save() to be called on legacy engines, otherwise we will just
  // have a zombie resource waiting for something
  delete exportRes;

  return !fail;
}

bool KLFLibBrowser::slotExportSelection()
{
  KLF_DEBUG_TIME_BLOCK(KLF_FUNC_NAME) ;

  // need to fetch selection from current view
  KLFAbstractLibView *view = curLibView();
  if (view == NULL) {
    qWarning("KLFLibBrowser::slotResourceProperties: NULL View!");
    return false;
  }

  // select and open target file
  KLFLibEngineFactory *factory = KLFLibLegacyEngineFactory::findFactoryFor("klf+legacy");
  if (factory == NULL) {
    qWarning()<<KLF_FUNC_NAME<<": Can't create KLFLibLegacyEngineFactory object (\"klf+legacy\") ?!?";
    return false;
  }
  QString filter = factory->schemeTitle(QLatin1String("klf+legacy")) + " (*.klf)";
  QString path = klfconfig.LibraryBrowser.lastFileDialogPath;
  QString fileName = QFileDialog::getSaveFileName(this, tr("Export selection to file..."), path, filter);

  if (fileName.isEmpty()) {
    klfDbg("Canceled by user.");
    return false;
  }

  if (QFile::exists(fileName)) {
    // erase it. the file dialog already asked for overwrite confirmation.
    if ( ! QFile::remove(fileName) ) {
      QMessageBox::critical(this, tr("Error"), tr("Failed to overwrite file %1").arg(fileName));
      qWarning()<<KLF_FUNC_NAME<<": Can't overwrite file "<<fileName;
      return false;
    }
  }

  klfDbg("Exporting to file "<<fileName);

  // get selected entries
  KLFLibEntryList entryList = view->selectedEntries();

  // create file resource
  KLFLibWidgetFactory::Parameters param;
  param["klfScheme"] = QLatin1String("klf+legacy");
  param["Filename"] = fileName;
  param["klfDefaultSubResource"] = tr("Export", "[[export selection default sub-resource name]]");
  KLFLibResourceEngine *resource = factory->createResource(QLatin1String("klf+legacy"), param, this);

  if (resource == NULL) {
    QMessageBox::critical(this, tr("Error"), tr("Failed to create export file %1!").arg(fileName));
    return false;
  }

  QList<KLFLib::entryId> insertedIds = resource->insertEntries(entryList);
  if (!insertedIds.size() || insertedIds.contains(-1)) {
    QMessageBox::critical(this, tr("Error"), tr("Error exporting items!"));
  }

  // important, as it will cause save() to be called on legacy engines, otherwise we will just
  // have a zombie resource waiting for something
  delete resource;

  return true;
}

void KLFLibBrowser::slotStartProgress(KLFProgressReporter *progressReporter, const QString& text)
{
  KLF_DEBUG_BLOCK(KLF_FUNC_NAME) ;
  klfDbg( ": min,max="<<progressReporter->min()<<","<<progressReporter->max()
	  <<"; text="<<text ) ;

  KLFProgressDialog *pdlg = new KLFProgressDialog(false, QString(), this);

  pdlg->startReportingProgress(progressReporter, text);

  pdlg->setProperty("klf_libbrowser_pdlg_want_hideautodelete", QVariant(true));
  pdlg->installEventFilter(this);
}



bool KLFLibBrowser::event(QEvent *e)
{
  if (e->type() == QEvent::KeyPress) {
    QKeyEvent *ke = (QKeyEvent*)e;
    if (ke->key() == Qt::Key_F8 && ke->modifiers() == 0) {
      hide();
      e->accept();
      return true;
    }
  }
  return QWidget::event(e);
}

void KLFLibBrowser::timerEvent(QTimerEvent *event)
{
  QWidget::timerEvent(event);
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
  u->tabResources->setFocus();
  // and call superclass
  QWidget::showEvent(event);
}


