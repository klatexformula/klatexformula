/***************************************************************************
 *   file klflibbrowser_p.h
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
 * This header contains (in principle _private_) definitions for klflibbrowser.cpp
 */


#ifndef KLFLIBBROWSER_P_H
#define KLFLIBBROWSER_P_H

#include <QWidget>
#include <QStackedWidget>
#include <QTabBar>
#include <QTabWidget>
#include <QEvent>
#include <QDragMoveEvent>
#include <QDialog>
#include <QStackedLayout>
#include <QDir>
#include <QFileDialog>
#include <QFileInfo>
#include <QAction>
#include <QSettings>
#include <QSignalMapper>
#include <QDesktopServices>
#include <QMessageBox>

#include <ui_klflibexportdialog.h>

#include <klfsearchbar.h>
#include "klflibview.h"
#include "klfconfig.h"
#include "klflibbrowser.h"

/** \internal */
class KLFLibBrowserViewContainer : public QStackedWidget, public KLFSearchableProxy
{
  Q_OBJECT
public:
  KLFLibBrowserViewContainer(KLFLibResourceEngine *resource, QTabWidget *parent)
    : QStackedWidget(parent), pResource(resource)
  {
    KLF_DEBUG_TIME_BLOCK(KLF_FUNC_NAME) ;

    KLF_DEBUG_WATCH_OBJECT(this) ;

    pResFlags = 0x0;

    pViewTypeActionGroup = new QActionGroup(this);
    QSignalMapper *actionSignalMapper = new QSignalMapper(this);

    if (pResource == NULL) {
      qWarning()<<"KLFLibBrowserViewContainer: NULL RESOURCE! Expect Imminent Crash!";
    }
    // find OK view type identifiers
    QStringList allViewTypeIdents = KLFLibViewFactory::allSupportedViewTypeIdentifiers();
    int k;
    for (k = 0; k < allViewTypeIdents.size(); ++k) {
      QString vtype = allViewTypeIdents[k];
      KLFLibViewFactory *factory =
	KLFLibViewFactory::findFactoryFor(vtype);
      bool thisViewTypeIdentOk = factory->canCreateLibView(vtype, pResource);
      if (thisViewTypeIdentOk)
	pOkViewTypeIdents << vtype;
      // create an action for this view type
      QAction *action = new QAction(pViewTypeActionGroup);
      action->setText(factory->viewTypeTitle(vtype));
      action->setCheckable(true);
      connect(action, SIGNAL(triggered(bool)), actionSignalMapper, SLOT(map()));
      actionSignalMapper->setMapping(action, vtype);
      action->setEnabled(thisViewTypeIdentOk);
      action->setData(vtype);
      pViewTypeActions << action;
    }
    connect(actionSignalMapper, SIGNAL(mapped(const QString&)), this, SLOT(openView(const QString&)));

    connect(this, SIGNAL(currentChanged(int)), this, SLOT(slotCurrentChanged(int)));
  }
  virtual ~KLFLibBrowserViewContainer()
  {
    KLF_DEBUG_BLOCK(KLF_FUNC_NAME) ;
  }

  QUrl url() const {
    KLF_DEBUG_BLOCK(KLF_FUNC_NAME) ;

    KLFAbstractLibView * view = qobject_cast<KLFAbstractLibView*>(currentWidget());
    if (view == NULL)
      return QUrl();
    return view->url();
  }

  KLFLibResourceEngine * resourceEngine() { return pResource; }

  KLFAbstractLibView * view() {
    KLFAbstractLibView * v = qobject_cast<KLFAbstractLibView*>(currentWidget());
    if (v == NULL) { klfDbg("view is NULL ..."); }
    return v;
  }

  QString currentViewTypeIdentifier() {
    return pOpenViewTypeIdents.key(currentIndex());
  }

  QString defaultSubResource() { return pResource->defaultSubResource(); }

  QStringList supportedViewTypeIdentifiers() const { return pOkViewTypeIdents; }

  QList<QAction*> viewTypeActions() const { return pViewTypeActions; }

  void setResourceRoleFlags(uint flags) {
    pResFlags = flags;
  }
  uint resourceRoleFlags() const { return pResFlags; }

  QVariantMap saveGuiState() const {
    KLF_DEBUG_BLOCK(KLF_FUNC_NAME) ;

    QVariantMap v;
    QVariantList vlist; // will hold QVariantMap's of each view's state
    QString curViewTypeIdent;
    QStringList saved_view_types;
    for (QMap<QString,int>::const_iterator cit = pOpenViewTypeIdents.begin();
	 cit != pOpenViewTypeIdents.end(); ++cit) {
      KLFAbstractLibView *view = qobject_cast<KLFAbstractLibView*>(widget(*cit));
      QString vti = cit.key(); // view type identifier
      if (view == currentWidget()) // this is current view -> save it in more global setting
	curViewTypeIdent = vti;
      QVariantMap vstate;
      vstate["ViewTypeIdentifier"] = vti;
      vstate["ViewState"] = QVariant::fromValue<QVariantMap>(view->saveGuiState());
      vlist << QVariant::fromValue<QVariantMap>(vstate);
      saved_view_types << vti;
    }
    // add also the states that are in pViewState from a previous loadGuiState() and for
    // which no view was opened.
    QVariantList lastViewStateList = pViewState.value("StateList").toList();
    for (QVariantList::const_iterator cit = lastViewStateList.begin();
	 cit != lastViewStateList.end(); ++cit) {
      const QVariantMap& vmap = (*cit).toMap();
      // see if we have this view type already
      if (saved_view_types.contains(vmap.value("ViewTypeIdentifier").toString()))
	continue;
      // otherwise, add this view state map too
      vlist << QVariant::fromValue<QVariantMap>(vmap);
    }
    

    v["StateList"] = QVariant::fromValue<QVariantList>(vlist);
    // Remember: this next key is queried also by KLFLibBrowser::openResourceByGuiState()
    v["CurrentViewTypeIdentifier"] = QVariant::fromValue<QString>(curViewTypeIdent);
    return v;
  }
  void loadGuiState(const QVariantMap& v)
  {
    KLF_DEBUG_BLOCK(KLF_FUNC_NAME) ;

    pViewState = v;
    klfDbg("loading view state map: "<<v) ;

    const QVariantList vlist = v["StateList"].toList(); // will hold QVariantMap's
    const QString curvti = v["CurrentViewTypeIdentifier"].toString();
    int k;
    for (k = 0; k < vlist.size(); ++k) {
      const QVariantMap vstate = vlist[k].toMap();
      QString vti = vstate["ViewTypeIdentifier"].toString();
      klfDbg("considering to restore view type "<<vti) ;
      if (vti != curvti)
	continue;
      klfDbg("restore view type "<<vti) ;

      // open the current view type identifier. The other view type identifiers will have
      // their state restored only if the user requests to display that view type.
      bool res = openView(vti);
      if (!res) {
	qWarning()<<"Can't open view of type "<<vti<<" for resource "<<url()<<"!";
	continue;
      }
      KLFAbstractLibView *view
	= qobject_cast<KLFAbstractLibView*>(widget(pOpenViewTypeIdents.value(vti)));
      if (view == NULL)
	continue;
      view->restoreGuiState(vstate["ViewState"].toMap());
    }
    
    // raise current view ...
    if (!curvti.isEmpty())
      openView(curvti);
  }

  QList<QAction*> openSubResourceActions()
  {
    KLF_DEBUG_BLOCK(KLF_FUNC_NAME) ;

    if ( ! (pResource->supportedFeatureFlags() & KLFLibResourceEngine::FeatureSubResources) )
      return QList<QAction*>();

    QList<QAction*> actions;

    QStringList subreslist = pResource->subResourceList();
    QUrl baseurl = pResource->url();
    int k;
    for (k = 0; k < subreslist.size(); ++k) {
      QString t;
      if (pResource->supportedFeatureFlags() & KLFLibResourceEngine::FeatureSubResourceProps)
	t = pResource->subResourceProperty(subreslist[k],
					  KLFLibResourceEngine::SubResPropTitle).toString();
      else
	t = subreslist[k];

      QUrl url = baseurl;
      url.addQueryItem("klfDefaultSubResource", subreslist[k]);
      QString urlstr = url.toString();

      QAction *a = NULL;
      if (pOpenSubResActionCache.contains(urlstr)) {
	klfDbg("re-using action for url="<<urlstr<<" from our local action cache") ;
	a = pOpenSubResActionCache[urlstr];
      } else {
	klfDbg("creating action for url="<<urlstr<<", it is not yet in our local action cache...") ;
	a = new QAction(t, this);
	a->setData(url.toString());
	connect(a, SIGNAL(triggered()), this, SLOT(internalRequestOpenSubResourceSender()));
	pOpenSubResActionCache[urlstr] = a;
	KLF_DEBUG_WATCH_OBJECT(a) ;
      }
      actions << a;
      klfDbg(pResource->url().toString()<<": Added action for sub-resource "<<subreslist[k]<<": url is "
	     <<a->data().toString()) ;
    }

    return actions;
  }

public slots:
  bool openView(const QString& viewTypeIdent)
  {
    KLF_DEBUG_TIME_BLOCK(KLF_FUNC_NAME) ;
    klfDbg("view type="<<viewTypeIdent<<"; resource URL="<<pResource->url()) ; 

    // see if we already have this view type open and ready
    if (pOpenViewTypeIdents.contains(viewTypeIdent)) {
      klfDbg( ": view type "<<viewTypeIdent<<" is already open at index ="
	      <<pOpenViewTypeIdents[viewTypeIdent] ) ;
      setCurrentIndex(pOpenViewTypeIdents[viewTypeIdent]);
      return true;
    }
    // otherwise, instanciate view
    KLFLibViewFactory *factory =
	KLFLibViewFactory::findFactoryFor(viewTypeIdent);
    KLFAbstractLibView *v = factory->createLibView(viewTypeIdent, this, pResource);
    if (v == NULL) {
      qWarning() << "The factory can't open a view of type "<<viewTypeIdent<<"!";
      return false;
    }
    // some view settings as given by klfconfig
    if (viewTypeIdent == "default") { // category tree list
      KLFLibDefaultView *defview = qobject_cast<KLFLibDefaultView*>(v);
      if (defview == NULL) {
	qWarning()<<KLF_FUNC_NAME<<": Created view of type"<<viewTypeIdent<<", that is NOT a KLFLibDefaultView ?!?";
      } else {
	defview->setGroupSubCategories(klfconfig.LibraryBrowser.groupSubCategories);
      }
    }

    KLF_DEBUG_WATCH_OBJECT(v) ;

    v->setContextMenuPolicy(Qt::CustomContextMenu);
    // get informed about selection changes
    connect(v, SIGNAL(entriesSelected(const KLFLibEntryList& )),
	    this, SLOT(slotEntriesSelected(const KLFLibEntryList& )));
    // and of new category suggestions
    connect(v, SIGNAL(moreCategorySuggestions(const QStringList&)),
	    this, SLOT(slotMoreCategorySuggestions(const QStringList&)));
    // and of restore actions
    connect(v, SIGNAL(requestRestore(const KLFLibEntry&, uint)),
	    this, SLOT(slotRequestRestore(const KLFLibEntry&, uint)));
    connect(v, SIGNAL(requestRestoreStyle(const KLFStyle&)),
	    this, SLOT(slotRequestRestoreStyle(const KLFStyle&)));
    // and of after-change refresh
    connect(v, SIGNAL(resourceDataChanged(const QList<KLFLib::entryId>&)),
	    this, SLOT(slotResourceDataChanged(const QList<KLFLib::entryId>&)));
    // and of context menu request
    connect(v, SIGNAL(customContextMenuRequested(const QPoint&)),
	    this, SIGNAL(viewContextMenuRequested(const QPoint&)));
    // view progress reporting
    connect(v, SIGNAL(operationStartReportingProgress(KLFProgressReporter *, const QString&)),
	    this, SIGNAL(viewOperationStartReportingProgress(KLFProgressReporter *, const QString&)));

    klfDbgT(": connected signals.");
    if ((pResource->supportedFeatureFlags() & KLFLibResourceEngine::FeatureSubResources) &&
	(pResource->supportedFeatureFlags() & KLFLibResourceEngine::FeatureSubResourceProps))
      pResource->setSubResourceProperty(pResource->defaultSubResource(),
					KLFLibResourceEngine::SubResPropViewType, viewTypeIdent);
    else
      pResource->setViewType(viewTypeIdent);
    klfDbgT(": set view type.");

    int curindex = currentIndex();
    blockSignals(true);
    int index = addWidget(v);
    blockSignals(false);
    pOpenViewTypeIdents[viewTypeIdent] = index;
    klfDbgT(": added widget, about to raise");
    if (currentIndex() == curindex)
      setCurrentIndex(index); // show new open view (and call slot slotCurrentIndex())
    else {
      // call now the slot that would have been called with addWidget()
      // if the signals hadn't been blocked
      slotCurrentChanged(index);
    }
    klfDbgT(": Added view.");

    // see if we have a GUI state to restore
    if (pViewState.contains("StateList")) {
      const QVariantList& vlist = pViewState["StateList"].toList(); // will hold QVariantMap's
      klfDbg("possibly restoring a GUI state... list size="<<vlist.size()) ;
      int k;
      for (k = 0; k < vlist.size(); ++k) {
	const QVariantMap& vstate = vlist[k].toMap();
	QString vti = vstate["ViewTypeIdentifier"].toString();
	if (vti != viewTypeIdent)
	  continue;
	klfDbg("Restoring gui state, match found for vti[="<<vti<<"]==viewTypeIdent[="<<viewTypeIdent<<"]!") ;
	v->restoreGuiState(vstate["ViewState"].toMap());
	break;
      }
    }
    return true;
  }


signals:
  void viewContextMenuRequested(const QPoint& pos);

  void viewTypeChanged(const QString&);

  void requestRestore(const KLFLibEntry& entry, uint restoreflags = KLFLib::RestoreLatexAndStyle);
  void requestRestoreStyle(const KLFStyle& style);

  void resourceDataChanged(const QList<KLFLib::entryId>& entryIdList);

  void entriesSelected(const KLFLibEntryList& entries);
  void moreCategorySuggestions(const QStringList& categorylist);

  void requestOpenUrl(const QString& url);

  void viewOperationStartReportingProgress(KLFProgressReporter *progressReporter, const QString& descriptiveText);

protected slots:
  void slotRequestRestore(const KLFLibEntry& entry, uint restoreflags = KLFLib::RestoreLatexAndStyle) {
    if (sender() == view())
      emit requestRestore(entry, restoreflags);
  }
  void slotRequestRestoreStyle(const KLFStyle& style) {
    if (sender() == view())
      emit requestRestoreStyle(style);
  }
  void slotResourceDataChanged(const QList<KLFLib::entryId>& entryIdList) {
    if (sender() == view())
      emit resourceDataChanged(entryIdList);
  }
  void slotEntriesSelected(const KLFLibEntryList& entries) {
    if (sender() == view())
      emit entriesSelected(entries);
  }
  void slotMoreCategorySuggestions(const QStringList& categorylist) {
    if (sender() == view())
      emit moreCategorySuggestions(categorylist);
  }

  void slotCurrentChanged(int index) {
    KLF_DEBUG_BLOCK(KLF_FUNC_NAME) ;

    KLFAbstractLibView *v = view();
    if (v == NULL) {
      setSearchTarget(NULL);
      return;
    }
    setSearchTarget(v->searchable());
    QString vtype = pOpenViewTypeIdents.key(index);
    int ai = findViewTypeAction(vtype);
    if (ai >= 0)
      pViewTypeActions[ai]->setChecked(true);


    emit viewTypeChanged(vtype);
    emit entriesSelected(v->selectedEntries());
  }

  void internalRequestOpenSubResourceSender()
  {
    KLF_DEBUG_BLOCK(KLF_FUNC_NAME) ;

    QAction * a = qobject_cast<QAction*>(sender());
    KLF_ASSERT_NOT_NULL( a, "Sender action is NULL", return ) ;

    emit requestOpenUrl(a->data().toString());
  }

protected:
  QStringList pOkViewTypeIdents;
  KLFLibResourceEngine *pResource;

  QVariantMap pViewState;


  /** Stores the view type identifier with the index in widget stack for each open view */
  QMap<QString,int> pOpenViewTypeIdents;
  QActionGroup *pViewTypeActionGroup;
  QList<QAction*> pViewTypeActions;

  uint pResFlags;

  int findViewTypeAction(const QString& vtype) {
    KLF_DEBUG_BLOCK(KLF_FUNC_NAME) ;

    int k;
    for (k = 0; k < pViewTypeActions.size(); ++k)
      if (pViewTypeActions[k]->data().toString() == vtype)
	return k;
    return -1;
  }

  // re-use "open sub-resource ->" actions within multiple menu appearances
  QMap<QString,QAction*> pOpenSubResActionCache;
};


// ------------------------


/** \internal */
class KLFLibBrowserTabWidget : public QTabWidget
{
  Q_OBJECT
public:
  KLFLibBrowserTabWidget(QWidget *parent) : QTabWidget(parent)
  {
    setUsesScrollButtons(false);
    pTabBar = new QTabBar(this);
    pTabBar->setAcceptDrops(true);
    pTabBar->installEventFilter(this);
    setTabBar(pTabBar);
  }
  virtual ~KLFLibBrowserTabWidget() { }

  /** Returns the tab index at position \c pos relative to tab widget. */
  int getTabAtPoint(const QPoint& pos) {
    QPoint tabbarpos = pTabBar->mapFromGlobal(mapToGlobal(pos));
    return pTabBar->tabAt(tabbarpos);
  }

  /** Returns the rectangle, relative to tab widget, occupied by tab in the tab bar. */
  QRect getTabRect(int tab) {
    QRect tabbarrect = pTabBar->tabRect(tab);
    return QRect(mapFromGlobal(pTabBar->mapToGlobal(tabbarrect.topLeft())),
		 tabbarrect.size());
  }

  bool eventFilter(QObject *object, QEvent *event)
  {
    if (object == pTabBar && event->type() == QEvent::DragEnter) {
      QDragEnterEvent *de = (QDragEnterEvent*)event;
      de->accept();
      return true;
    }
    if (object == pTabBar && event->type() == QEvent::DragMove) {
      // ignore event, but raise the underlying tab.
      QDragMoveEvent *de = (QDragMoveEvent*)event;
      QPoint pos = de->pos();
      de->ignore();
      int index = getTabAtPoint(pos);
      if (index >= 0)
	setCurrentIndex(index);
      return true;
    }
    return QTabWidget::eventFilter(object, event);
  }


  void setTabEnabled(int index, bool enable)
  {
    QTabWidget::setTabEnabled(index, enable);
    emit pageEnabled(index, enable);
  }

  void setTabIcon(int index, const QIcon& icon)
  {
    QTabWidget::setTabIcon(index, icon);
    emit pageIconChanged(index, icon);
  }

  void setTabText(int index, const QString& label)
  {
    QTabWidget::setTabText(index, label);
    emit pageTextChanged(index, label);
  }

signals:

  void pageInserted(int index, const QIcon& icon, const QString& text);
  void pageTextChanged(int index, const QString& text);
  void pageIconChanged(int index, const QIcon& icon);
  void pageEnabled(int index, bool enable);
  void pageRemoved(int index);


public slots:

  void refreshTabReadOnly(int tabindex, bool readonly) {
    QPalette pal = palette();
    
    QColor normalColor = pal.color(QPalette::Active, QPalette::ButtonText);
    QColor readOnlyColor = pal.color(QPalette::Disabled, QPalette::ButtonText);

    if (readonly)
      pTabBar->setTabTextColor(tabindex, readOnlyColor);
    else
      pTabBar->setTabTextColor(tabindex, normalColor);
  }

protected:

  virtual void tabInserted(int index) {
    emit pageInserted(index, tabIcon(index), tabText(index));
  }
  virtual void tabRemoved(int index) {
    emit pageRemoved(index);
  }

private:
  QTabBar *pTabBar;
};

// -----------------------------------------------------------------



class KLFLibBrowserTabMenu : public QMenu
{
  Q_OBJECT
public:
  KLFLibBrowserTabMenu(KLFLibBrowserTabWidget *tabwidget)
    : QMenu(tabwidget), pTabWidget(tabwidget)
  {
    setTitle(tr("Switch to Tab"));
    pActionGroup = new QActionGroup(this);
    pActionGroup->setExclusive(true);

    int k;
    for (k = 0; k < pTabWidget->count(); ++k) {
      slotPageInserted(k, pTabWidget->tabIcon(k), pTabWidget->tabText(k));
    }

    connect(pTabWidget, SIGNAL(pageInserted(int, const QIcon&, const QString&)),
	    this, SLOT(slotPageInserted(int, const QIcon&, const QString&)));
    connect(pTabWidget, SIGNAL(pageRemoved(int)), this, SLOT(slotPageRemoved(int)));
    connect(pTabWidget, SIGNAL(pageTextChanged(int, const QString&)),
	    this, SLOT(slotPageTextChanged(int, const QString&)));
    connect(pTabWidget, SIGNAL(pageIconChanged(int, const QIcon&)),
	    this, SLOT(slotPageIconChanged(int, const QIcon&)));
    connect(pTabWidget, SIGNAL(pageEnabled(int, bool)), this, SLOT(slotPageEnabled(int, bool)));
    connect(pTabWidget, SIGNAL(currentChanged(int)), this, SLOT(slotCurrentIndexChanged(int)));
  }
  virtual ~KLFLibBrowserTabMenu()
  {
  }

protected slots:
  void slotPageInserted(int index, const QIcon& icon, const QString& text)
  {
    KLF_DEBUG_BLOCK(KLF_FUNC_NAME) ;
    QAction *a = new QAction(pActionGroup);
    a->setIcon(icon);
    a->setText(text);
    a->setCheckable(true);
    connect(a, SIGNAL(triggered()), this, SLOT(slotRaisePageFromAction()));
    if (index < actions().size())
      insertAction(actions()[index], a);
    else
      addAction(a);
  }
  void slotPageRemoved(int index)
  {
    KLF_DEBUG_BLOCK(KLF_FUNC_NAME) ;
    if (index >= 0 && index < actions().size())
      removeAction(actions()[index]);
  }
  void slotPageTextChanged(int index, const QString& text)
  {
    if (index >= 0 && index < actions().size())
      actions()[index]->setText(text);
  }
  void slotPageIconChanged(int index, const QIcon& icon)
  {
    if (index >= 0 && index < actions().size())
      actions()[index]->setIcon(icon);
  }
  void slotPageEnabled(int index, bool enable)
  {
    if (index >= 0 && index < actions().size())
      actions()[index]->setEnabled(enable);
  }
  void slotCurrentIndexChanged(int index)
  {
    if (index >= 0 && index < actions().size())
      actions()[index]->setChecked(true); // will auto-exclude the others because of exclusive action group
  }


  void slotRaisePageFromAction()
  {
    QAction * a = qobject_cast<QAction*>(sender());
    if (a == NULL) {
      qWarning()<<KLF_FUNC_NAME<<": action sender is NULL?!?";
      return;
    }
    int k;
    QList<QAction*> alist = actions();
    for (k = 0; k < alist.size(); ++k) {
      if (alist[k] == a) {
	pTabWidget->setCurrentIndex(k);
	return;
      }
    }
  }

private:
  QActionGroup *pActionGroup;
  KLFLibBrowserTabWidget *pTabWidget;
};






// -----------------------------------------------------------------


/** \internal */
class KLFLibExportDialog : public QDialog, public Ui::KLFLibExportDialog
{
  Q_OBJECT
public:
  enum { DataRole = Qt::UserRole, OldCheckStateRole } ;

  static KLFLibResourceEngine * showExportDialogCreateResource(KLFLibBrowser *libbrowser, QList<QUrl> *exportUrls)
  {
    KLFLibExportDialog dlg(libbrowser);
    bool ok;
    KLFLibWidgetFactory::Parameters param;
    *exportUrls = QList<QUrl>();
    do {
      int result = dlg.exec();
      if (result != QDialog::Accepted)
	return NULL; // cancelled by user
      param = dlg.wfactory->retrieveCreateParametersFromWidget("LocalFile", dlg.localFileWidget);
      ok = true; // by default we got parameters OK
      if (param.isEmpty())
	ok = false;
      if (param.contains("klfRetry") && param["klfRetry"].toBool())
	ok = false;
      if (param.contains("klfCancel") && param["klfCancel"].toBool())
	return NULL; // cancelled
    } while (!ok) ;

    QList<QUrl> urls = dlg.selectedExportUrls();
    if (urls.isEmpty()) {
      QMessageBox::critical(&dlg, tr("Error"), tr("You have not selected any resources to export!"));
      return NULL;
    }

    // now create the resource
    if (!param.contains("klfScheme")) {
      qWarning()<<KLF_FUNC_NAME<<": factory create parameters did not provide a scheme!";
      return NULL;
    }
    QString scheme = param["klfScheme"].toString();
    KLFLibEngineFactory *efactory = KLFLibEngineFactory::findFactoryFor(scheme);
    if (efactory == NULL) {
      qWarning()<<KLF_FUNC_NAME<<": can't find factory for scheme "<<scheme<<"!";
      return NULL;
    }
    // this is the sub-resource that will be created with the resource. It will be deleted after
    // we filled up the resource. Remember the name ("export_xtra"), it will be deleted at the
    // end of KLFLibBrowser::slotExport(), ie. don't change this without consideration.
    param["klfDefaultSubResource"] = QLatin1String("export_xtra");

    KLFLibResourceEngine *resource = efactory->createResource(scheme, param, libbrowser);
    if (resource == NULL) {
      QMessageBox::critical(&dlg, tr("Error"),
			    tr("Can't create resource %1!").arg(param["Filename"].toString()));
      qWarning()<<KLF_FUNC_NAME<<": Failed to create resource of type "<<scheme<<" with parameters: "<<param;
      return NULL;
    }
    *exportUrls = urls;
    return resource;
  }


  KLFLibExportDialog(KLFLibBrowser *libbrowser)
    : QDialog(libbrowser, 0), pLibBrowser(libbrowser), pModel(NULL),
    pInSlotItemChanged(false)
  {
    setupUi(this);
    setWindowModality(Qt::WindowModal);
    setModal(true);

    wfactory = KLFLibWidgetFactory::findFactoryFor("LocalFile");
    if (wfactory == NULL) {
      qWarning()<<KLF_FUNC_NAME<<": Can't find factory for type 'LocalFile'!";
      return;
    }
    QString mydir = klfconfig.LibraryBrowser.lastFileDialogPath;
    KLFLibWidgetFactory::Parameters pdefault;
    QString filedate = QDate::currentDate().toString("yyyy-MM-dd");
    pdefault["Url"] = QUrl("klf+legacy://"+tr("%1/klatexformula_export_%2.klf").arg(mydir, filedate));
    QStackedLayout *lyt_gbxExportLocalFileWidget = new QStackedLayout(gbxExportLocalFileWidget);
    lyt_gbxExportLocalFileWidget->setObjectName("lyt_gbxExportLocalFileWidget");
    lyt_gbxExportLocalFileWidget->setContentsMargins(0,0,0,0);
    lyt_gbxExportLocalFileWidget->setSpacing(0);
    localFileWidget = 
      wfactory->createPromptCreateParametersWidget(gbxExportLocalFileWidget, "LocalFile", pdefault);
    lyt_gbxExportLocalFileWidget->addWidget(localFileWidget);
    localFileWidget->setFixedHeight(localFileWidget->minimumSizeHint().height()+4);
    gbxExportLocalFileWidget->setSizePolicy(QSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed));

    pModel = new QStandardItemModel(this);
    lstRes->setModel(pModel);

    connect(pModel, SIGNAL(itemChanged(QStandardItem*)), this, SLOT(slotItemChanged(QStandardItem*)));

    connect(btnSelectAll, SIGNAL(clicked()), this, SLOT(selectAll()));
    connect(btnUnselectAll, SIGNAL(clicked()), this, SLOT(unselectAll()));

    populateExportList();
  }


  QList<QUrl> selectedExportUrls()
  {
    QList<QUrl> urlList;
    int k, j;
    QStandardItem *root = pModel->invisibleRootItem();
    for (k = 0; k < root->rowCount(); ++k) {
      QStandardItem *resItem = root->child(k);
      if (resItem->rowCount() > 0) {
	// has specific sub-resources
	for (j = 0; j < resItem->rowCount(); ++j) {
	  if (resItem->child(j)->checkState() != Qt::Unchecked) {
	    QUrl url = resItem->data(DataRole).toString();
	    url.addQueryItem("klfDefaultSubResource", resItem->child(j)->data(DataRole).toString());
	    urlList << url;
	    continue;
	  }
	}
      } else {
	// no specific sub-resources, ie. the resource does not support sub-resources
	if (resItem->checkState() != Qt::Unchecked)
	  urlList << QUrl(resItem->data(DataRole).toString());
      }
    }
    return urlList;
  }


public slots:

  void selectAll(bool select = true)
  {
    // select all root items, their children will follow by cascade
    QStandardItem *root = pModel->invisibleRootItem();
    int k;
    for (k = 0; k < root->rowCount(); ++k) {
      root->child(k)->setCheckState(select ? Qt::Checked : Qt::Unchecked);
    }
  }
  void unselectAll()
  {
    selectAll(false);
  }

private slots:

  void slotItemChanged(QStandardItem *item)
  {
    if (pInSlotItemChanged)
      return;
    pInSlotItemChanged = true;
    Qt::CheckState chk = item->checkState();
    Qt::CheckState oldchk = (Qt::CheckState)item->data(OldCheckStateRole).toInt();
    //    printf("Item %p maybe check state changed from %d to %d?\n", (void*)item,
    //	   oldchk, chk);
    if ((chk == Qt::Checked || chk == Qt::Unchecked) && oldchk != chk) {
      //      printf("Item %p check state changed from %d to %d!\n", (void*)item, oldchk, chk);
      // item was checked or un-checked
      item->setData(chk, OldCheckStateRole);
      // iterate its children and (un-)check them all
      int k;
      for (k = 0; k < item->rowCount(); ++k) {
	//	printf("-->\n");
	item->child(k)->setData(chk, OldCheckStateRole); // no update please
	item->child(k)->setCheckState(chk);
	//	printf("<--\n");
      }
      // and now set the parent's item check state to partially checked.
      QStandardItem *p = item->parent();
      if (p != NULL) {
	//	printf("parent-->\n");
	p->setData(Qt::PartiallyChecked, OldCheckStateRole);
	p->setCheckState(Qt::PartiallyChecked);
	//	printf("<--parent\n");
      }
      //      printf("Done processing item %p check state change from %d to %d\n", (void*)item, oldchk, chk);
    }
    pInSlotItemChanged = false;
  }


private:
  KLFLibBrowser *pLibBrowser;
  QStandardItemModel *pModel;
  bool pInSlotItemChanged;

  KLFLibWidgetFactory * wfactory;
  QWidget *localFileWidget;


  void populateExportList()
  {
    pModel->clear();
    pModel->setColumnCount(1);
    pModel->setHorizontalHeaderLabels(QStringList()<<tr("Resource", "[[export list title]]"));
    
    QList<QUrl> openurls = pLibBrowser->openUrls();
    int k, j;
    for (k = 0; k < openurls.size(); ++k) {
      KLFLibResourceEngine *res = pLibBrowser->getOpenResource(openurls[k]);
      QStandardItem *parent = getResourceParentItem(res);
      QStringList subreslist;
      if (openurls[k].hasQueryItem("klfDefaultSubResource")) {
	subreslist = QStringList() << openurls[k].queryItemValue("klfDefaultSubResource");
      } else if (res->supportedFeatureFlags() & KLFLibResourceEngine::FeatureSubResources) {
	// no default sub-resource is specified, so add them all
	subreslist = res->subResourceList();
      }
      // add all subresources
      for (j = 0; j < subreslist.size(); ++j) {
	if (findSubResourceItem(res, subreslist[j]) != NULL)
	  continue;
	// create this item
	QStandardItem *item = new QStandardItem;
	item->setData(subreslist[j], DataRole);
	QString srtitle;
	if (res->supportedFeatureFlags() & KLFLibResourceEngine::FeatureSubResourceProps)
	  srtitle =
	    res->subResourceProperty(subreslist[j], KLFLibResourceEngine::SubResPropTitle).toString();
	if (srtitle.isEmpty())
	  srtitle = subreslist[j];
	item->setText(srtitle);
	item->setFlags(Qt::ItemIsSelectable|Qt::ItemIsUserCheckable|Qt::ItemIsEnabled);
	item->setCheckState(Qt::Checked);
	item->setData(item->checkState(), OldCheckStateRole);
	parent->appendRow(item);
	lstRes->setExpanded(pModel->indexFromItem(parent), true);
      }
    }
  }

  QStandardItem *getResourceParentItem(KLFLibResourceEngine *res)
  {
    QStandardItem *root = pModel->invisibleRootItem();
    int j;
    for (j = 0; j < root->rowCount(); ++j) {
      if (root->child(j)->data(DataRole).value<QString>() == res->url().toString())
	return root->child(j);
    }
    // not existent, need to create it
    QStandardItem *item = new QStandardItem;
    item->setData(QVariant::fromValue<QString>(res->url().toString()), DataRole);
    item->setText(res->title());
    item->setFlags(Qt::ItemIsSelectable|Qt::ItemIsUserCheckable|Qt::ItemIsEnabled);
    item->setCheckState(Qt::Checked);
    item->setData(item->checkState(), OldCheckStateRole);
    root->appendRow(item);
    return item;
  }

  QStandardItem *findSubResourceItem(KLFLibResourceEngine *res, const QString& subResource)
  {
    QStandardItem *parent = getResourceParentItem(res);
    int k;
    for (k = 0; k < parent->rowCount(); ++k) {
      if (parent->child(k)->data(DataRole) == subResource)
	return parent->child(k);
    }
    return NULL;
  }
};





/* THIS HAS NEVER BEEN USED NOR TESTED, HOWEVER IT MAY COME IN HANDY IF NEEDED.

/ ** \internal * /
class KLFSignalUnmapper : public QObject
{
  Q_OBJECT
public:
  KLFSignalUnmapper(QObject *parent) : QObject(parent)
  {
  }
  virtual ~KLFSignalUnmapper() { }

  void registerObject(const QString& key, QObject *object, const QByteArray& member,
		      QList<QGenericArgument> staticArgs)
  {
    UnmappedObject o;
    o.key = key;
    o.object = object;
    o.member = member;
    o.staticArgs = staticArgs;
    unmappedObjects << o;
    connect(o.object, SIGNAL(destroyed()), this, SLOT(unregisterObjectSender()));
  }

public slots:
  void unmap(const QString& key) {
    int k;
    for (k = 0; k < unmappedObjects.size(); ++k) {
      if (unmappedObjects[k].key == key) {
	const QList<QGenericArgument>& al = unmappedObjects[k].staticArgs;
	QMetaObject::invokeMethod(unmappedObjects[k].object, unmappedObjects[k].member,
				  al.value(0), al.value(1), al.value(2), al.value(3), al.value(4),
				  al.value(5), al.value(6), al.value(7), al.value(8), al.value(9));
      }
    }
  }

  void unregisterObject(QObject *object) {
    int k;
    for (k = 0; k < unmappedObjects.size(); ++k)
      if (unmappedObjects[k].object == object)
	unmappedObjects.removeAt(k);
  }

private slots:
  void unregisterObjectSender() {
    QObject *object = sender();
    if (object == NULL)
      return;
    unregisterObject(object);
  }

private:
  struct UnmappedObject {
    QString key;
    QObject *object;
    QByteArray member;
    QList<QGenericArgument> staticArgs;
  };
  QList<UnmappedObject> unmappedObjects;
};
*/



#endif
