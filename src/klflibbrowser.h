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

  virtual bool eventFilter(QObject *object, QEvent *event);

  QList<QUrl> openedUrls() const;

  /** \note Mind that KLFLibBrowser deletes the views and their corresponding
   * engines upon destruction. */
  KLFLibResourceEngine * getOpenResource(const QUrl& url);

signals:
  void requestRestore(const KLFLibEntry& entry, uint restore_flags);
  void requestRestoreStyle(const KLFStyle& style);

public slots:
  bool openResource(const QUrl& url);
  bool closeResource(const QUrl& url);

protected slots:

  void slotRestoreWithStyle();
  void slotRestoreLatexOnly();
  void slotDeleteSelected();

  void slotTabResourceShown(int tabIndex);
  void slotShowTabContextMenu(const QPoint& pos);

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
  KLFAbstractLibView * findOpenUrl(const QUrl& url);
  KLFAbstractLibView * curView();

private:
  Ui::KLFLibBrowser *pUi;
  QList<KLFAbstractLibView*> pLibViews;

  QMenu *pResourceMenu;

  QString pSearchText;
  QString pLastSearchText;

private slots:
  void updateSearchFound(bool found);
  void slotSearchFocusIn();
  void slotSearchFocusOut();
};




#endif
