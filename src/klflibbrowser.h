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

class KLFLibBrowserViewContainer;


class KLFLibBrowser : public QWidget
{
  Q_OBJECT
public:
  KLFLibBrowser(QWidget *parent = NULL);
  virtual ~KLFLibBrowser();

  enum ResourceRoleFlag { NoCloseRoleFlag = 0x01 };

  virtual bool eventFilter(QObject *object, QEvent *event);

  QList<QUrl> openUrls() const;
  //! Returns the URL of the current tab page
  QUrl currentUrl();
  //! Returns the index of \ref currentUrl() in \ref openUrls()
  int currentUrlIndex();

  /** \note Mind that KLFLibBrowser deletes the views and their corresponding
   * engines upon destruction. */
  KLFLibResourceEngine * getOpenResource(const QUrl& url);

  /** Returns the view that is used to display the resource with URL \c url.
   *
   * \note the returned view belongs to this KLFLibBrowser object. */
  KLFAbstractLibView * getView(const QUrl& url);

  /** Returns the view that is used to display the resource \c resource.
   *
   * \note the returned view belongs to this KLFLibBrowser object. */
  KLFAbstractLibView * getView(KLFLibResourceEngine *resource);

  QVariantMap saveGuiState();
  void loadGuiState(const QVariantMap& state);

  static QString displayTitle(KLFLibResourceEngine *resource);

signals:
  void requestRestore(const KLFLibEntry& entry, uint restoreFlags);
  void requestRestoreStyle(const KLFStyle& style);

public slots:
  /** If the \c url is not already open, opens the given URL. An appropriate factory
   * needs to be installed supporting that scheme. Then an appropriate view is created
   * using the view factories.
   *
   * If the \c url is already open, then the appropriate tab is raised.
   *
   * Resource flags are updated in both cases.
   *
   * If an empty \c viewTypeIdentifier is given, the view type identifier suggested by
   * the resource itself is taken. If the latter is empty, then the default view type
   * identifier (\ref KLFLibViewFactory::defaultViewTypeIdentifier) is considered.
   */
  bool openResource(const QUrl& url, uint resourceRoleFlags = 0x0,
		    const QString& viewTypeIdentifier = QString());

  /** Overloaded member, provided for convenience.
   *
   * Opens a previously (independently) open \c resource and displays it in the library.
   *
   * \warning this library browser takes ownership of the resource and will delete it
   *   when done using it.
   */
  bool openResource(KLFLibResourceEngine *resource, uint resourceRoleFlags = 0x0,
		    const QString& viewTypeIdentifier = QString());

  bool closeResource(const QUrl& url);

protected slots:

  void slotRestoreWithStyle();
  void slotRestoreLatexOnly();
  void slotDeleteSelected();

  void slotRefreshResourceActionsEnabled();

  void slotTabResourceShown(int tabIndex);
  void slotShowTabContextMenu(const QPoint& pos);

  void slotResourceRename();
  void slotResourceRenameFinished();
  bool slotResourceClose(KLFLibBrowserViewContainer *view = NULL);
  void slotResourceProperties();
  bool slotResourceNewSubRes();
  bool slotResourceOpen();
  bool slotResourceNew();
  bool slotResourceSaveTo();

  /** sender is used to find resource engine emitter. */
  void slotResourceDataChanged(const QList<KLFLib::entryId>& entryIdList);
  /** sender is used to find resource engine emitter. */
  void slotResourcePropertyChanged(int propId);
  void slotUpdateForResourceProperty(KLFLibResourceEngine *resource, int propId);
  /** sender is used to find resource engine emitter. */
  void slotSubResourcePropertyChanged(const QString& subResource, int propId);
  /** sender is used to find resource engine emitter. */
  void slotDefaultSubResourceChanged(const QString& subResource);


  void slotEntriesSelected(const KLFLibEntryList& entries);
  void slotAddCategorySuggestions(const QStringList& catlist);
  void slotShowContextMenu(const QPoint& pos);

  void slotCategoryChanged(const QString& newcategory);
  void slotTagsChanged(const QString& newtags);

  void slotSearchClear();
  void slotSearchFind(const QString& searchText) { slotSearchFind(searchText, true); }
  void slotSearchFind(const QString& searchText, bool forward);
  void slotSearchClearOrNext();
  void slotSearchFindNext(bool forward = true);
  void slotSearchFindPrev();
  void slotSearchAbort();

  /** \note important data is defined in sender's custom properties (a QAction) */
  void slotCopyToResource();
  /** \note important data is defined in sender's custom properties (a QAction) */
  void slotMoveToResource();
  /** common code to slotCopyToResource() and slotMoveToResource() */
  void slotCopyMoveToResource(QObject *sender, bool move);
  void slotCopyMoveToResource(KLFAbstractLibView *dest, KLFAbstractLibView *source, bool move);

protected:
  KLFLibBrowserViewContainer * findOpenUrl(const QUrl& url);
  KLFLibBrowserViewContainer * findOpenResource(KLFLibResourceEngine *resource);
  KLFLibBrowserViewContainer * curView();
  KLFAbstractLibView * curLibView();
  KLFLibBrowserViewContainer * viewForTabIndex(int tab);

private:
  Ui::KLFLibBrowser *u;
  QList<KLFLibBrowserViewContainer*> pLibViews;

  QMenu *pResourceMenu;
  QAction *pARename;
  QAction *pAClose;
  QAction *pAProperties;
  QAction *pANewSubRes;
  QAction *pASaveTo;
  QAction *pANew;
  QAction *pAOpen;
  QAction *pAViewType;

  QString pSearchText;
  QString pLastSearchText;

  bool pIsWaiting;
  void startWait();
  void stopWait();

private slots:
  void updateSearchFound(bool found);
  void slotSearchFocusIn();
  void slotSearchFocusOut();

  void updateResourceRoleFlags(KLFLibBrowserViewContainer *view, uint flags);
};




#endif
