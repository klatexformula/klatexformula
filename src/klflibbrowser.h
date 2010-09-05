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
#include <QMovie>
#include <QMenu>
#include <QPushButton>
#include <QLabel>

#include <klflib.h>


namespace Ui { class KLFLibBrowser; }

class KLFLibBrowserViewContainer;
class KLFProgressReporter;

class KLFAbstractLibView;

class KLF_EXPORT KLFLibBrowser : public QWidget
{
  Q_OBJECT
public:
  KLFLibBrowser(QWidget *parent = NULL);
  virtual ~KLFLibBrowser();

  enum ResourceRoleFlag {
    NoRoleFlag              = 0x0, //!< This resource has nothing special

    NoCloseRoleFlag         = 0x00000001, //!< Resource 'Close' GUI button is disabled (grayed)

    HistoryRoleFlag         = 0x00010000, //!< This resource is the History resource
    ArchiveRoleFlag         = 0x00020000, //!< This resource is the Archive resource
    SpecialResourceRoleMask = 0x00ff0000, //!< Mask to extract the 'special resource' type (eg. history)

    NoChangeFlag            = 0x70000000 //!< Instructs to not set new flags for already-open resources
  };

  virtual bool eventFilter(QObject *object, QEvent *event);

  QList<QUrl> openUrls() const;
  //! Returns the URL of the current tab page
  QUrl currentUrl();
  //! Returns the index of \ref currentUrl() in \ref openUrls()
  int currentUrlIndex();

  /** Returns the KLFLibResourceEngine that is managing the display of the
   * currently open URL \c url. The url must currently be open.
   *
   * \note Mind that KLFLibBrowser deletes the views and their corresponding
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
  void loadGuiState(const QVariantMap& state, bool openURLs = true);

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
  bool openResource(const QUrl& url, uint resourceRoleFlags = NoChangeFlag,
		    const QString& viewTypeIdentifier = QString());

  /** Convenience function. Equivalent to
   * \code openResource(QUrl(url), ...) \endcode
   */
  bool openResource(const QString& url, uint resourceRoleFlags = NoChangeFlag,
		    const QString& viewTypeIdentifier = QString());

  /** Overloaded member, provided for convenience.
   *
   * Opens a previously (independently) open \c resource and displays it in the library.
   *
   * \warning this library browser takes ownership of the resource and will delete it
   *   when done using it.
   */
  bool openResource(KLFLibResourceEngine *resource, uint resourceRoleFlags = NoChangeFlag,
		    const QString& viewTypeIdentifier = QString());

  bool closeResource(const QUrl& url);

  void retranslateUi(bool alsoBaseUi = true);

protected slots:

  void slotRestoreWithStyle();
  void slotRestoreLatexOnly();
  void slotDeleteSelected();

  void slotRefreshResourceActionsEnabled();

  void slotTabResourceShown(int tabIndex);
  void slotShowTabContextMenu(const QPoint& pos);

  void slotResourceRename();
  void slotResourceRenameSubResource();
  /** implements both slotResourceRename() and slotResourceRenameSubResource(). If
   * \c renameSubResource is false, renames the resource. If true renames the sub-resource.
   */
  void slotResourceRename(bool renameSubResource);
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

  void slotMetaInfoChanged(const QMap<int,QVariant>& props);

  /** \note important data is defined in sender's custom properties (a QAction) */
  void slotCopyToResource();
  /** \note important data is defined in sender's custom properties (a QAction) */
  void slotMoveToResource();
  /** common code to slotCopyToResource() and slotMoveToResource() */
  void slotCopyMoveToResource(QObject *sender, bool move);
  void slotCopyMoveToResource(KLFAbstractLibView *dest, KLFAbstractLibView *source, bool move);

  void slotCut();
  void slotCopy();
  void slotPaste();

  void slotOpenAll();
  bool slotExport();

  void slotStartProgress(KLFProgressReporter *progressReporter, const QString& text);


protected:
  KLFLibBrowserViewContainer * findOpenUrl(const QUrl& url);
  KLFLibBrowserViewContainer * findOpenResource(KLFLibResourceEngine *resource);
  KLFLibBrowserViewContainer * curView();
  KLFAbstractLibView * curLibView();
  KLFLibBrowserViewContainer * viewForTabIndex(int tab);

  QList<KLFLibBrowserViewContainer*> findByRoleFlags(uint flags, uint mask);

  inline KLFLibBrowserViewContainer *findSpecialResource(uint specialResourceRoleFlag)
  { QList<KLFLibBrowserViewContainer*> l = findByRoleFlags(specialResourceRoleFlag, SpecialResourceRoleMask);
    if (l.isEmpty()) { return NULL; }  return l[0]; }

  bool event(QEvent *event);
  void showEvent(QShowEvent *event);
  void timerEvent(QTimerEvent *event);

private:
  Ui::KLFLibBrowser *u;
  QList<KLFLibBrowserViewContainer*> pLibViews;

  QMenu *pResourceMenu;
  QMenu *pImportExportMenu;

  QPushButton *pTabCornerButton;

private slots:
  void updateResourceRoleFlags(KLFLibBrowserViewContainer *view, uint flags);
};




#endif
