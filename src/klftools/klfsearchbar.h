/***************************************************************************
 *   file klfsearchbar.h
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


#ifndef KLFSEARCHBAR_H
#define KLFSEARCHBAR_H

#include <QObject>
#include <QWidget>
#include <QFrame>
#include <QMovie>
#include <QLabel>
#include <QTime>

#include <klfdefs.h>

class KLFWaitAnimationOverlay;

class KLFSearchBar;
class KLFSearchableProxy;
namespace Ui { class KLFSearchBar; }

//! An interface for objects that can be I-searched with a KLFSearchBar
/** This class is the base skeleton interface for displays that will be targets for I-searches.
 * There are three functions to reimplement:
 * \code
 * virtual bool searchFind(const QString& queryString, bool forward);
 * virtual bool searchFindNext(bool forward);
 * virtual void searchAbort();
 * \endcode
 * That have to actually perform the search.
 *
 * It is not uncommon for the display widget itself to inherit also from a KLFSearchable. See
 * KLFAbstractLibView and KLFLibDefautlView for an example (in klfapp library).
 *
 * This class is pretty low-level search (you have to manually walk all items, remember the query
 * string for future find-next operations, etc.). For a higher-level search implementation, see
 * KLFIteratorSearchable (which itself is a KLFSearchable object and can also be used as target
 * for KLFSearchBar).
 */
class KLF_EXPORT KLFSearchable
{
public:
  KLFSearchable();
  virtual ~KLFSearchable();

  //! Find the first occurence of a query string
  /** This function has to be reimplemented to find the first occurence of the string
   * \c queryString, searching from top of the displayed information if \c forward is
   * TRUE, or reverse from end of display of FALSE.
   *
   * The reimplementation should call from time to time
   * \code qApp->processEvents() \endcode
   * to keep the GUI from freezing in long searches.
   *
   * \note If the reimplementation implements the above suggestion, note that the slot
   *   \ref searchAbort() may be called during that time! It is best to take that into
   *   account and provide a means to stop the search if that is the case.
   */
  virtual bool searchFind(const QString& queryString, bool forward) = 0;

  //! Find the first occurence of a query string
  /** This function is provided for calling convenience. Subclasses should reimplement
   * searchFind(const QString&, bool) instead.
   *
   * This function directly calls searchFind(const QString& bool) with the \c forward
   * argument set to TRUE.
   */
  inline bool searchFind(const QString& queryString) { return searchFind(queryString, true); }

  //! Find next or previous occurence of query string
  /** This function has to be reimplemented to find the next occurence of the query string
   * given by a previous call to \ref searchFind(). The search must be performed in the
   * direction given by \c forward (see \ref searchFind()).
   *
   * It is up to the sub-class to remember the query string and the current match location.
   *
   * This function should also call the applications's processEvents() to keep the GUI from
   * freezing. See documentation in \ref searchFind().
   */
  virtual bool searchFindNext(bool forward) = 0;

  //! Abort I-Search 
  /** The behavior depends on the object/data being searched. This could
   * be reimplemented for example to return to the beginning of the list, or to the position
   * where the user was at the beginning of the search.
   */
  virtual void searchAbort() = 0;

private:
  QList<KLFSearchBar*> pTargetOf;
  QList<KLFSearchableProxy*> pTargetOfProxy;

  friend class KLFSearchBar;
  friend class KLFSearchableProxy;
};

//! A proxy class that relays search queries to another searchable object
/** This class may be used for example when you have global search bar, but many sub-windows or sub-displays
 * displaying different data, and the search bar should search within the active one.
 */
class KLF_EXPORT KLFSearchableProxy : public KLFSearchable
{
public:
  KLFSearchableProxy() : pTarget(NULL) { }
  virtual ~KLFSearchableProxy();
  
  void setSearchTarget(KLFSearchable *target);

  virtual bool searchFind(const QString& queryString, bool forward);
  virtual bool searchFindNext(bool forward);
  virtual void searchAbort();

private:
  KLFSearchable *pTarget;

  friend class KLFSearchable;
};



//! An Search Bar for Incremental Search
/** This widget provides a set of controls an incremental search. This includes a line edit to input
 * the query string, a clear button, 'find next' and 'find previous' buttons.
 *
 * This widget acts upon an abstract \ref KLFSearchable object, which the object or display being searched
 * will have to implement. You only need to implement three straightforward functions providing the
 * actual search functionality. The search target can be set with \ref setSearchTarget().
 *
 * The user interface is inspired from (X)Emacs' I-search. More specifically:
 *  - results are searched for already while the user is typing
 *  - once arrived at the end of the list, the search will fail. However, re-attempting to find next
 *    (eg. F3 or Ctrl-S) (respectively previous) will wrap the search from the beginning (respectively
 *    from the end)
 *
 * Shortcuts can be enabled so that Ctrl-F, Ctrl-S, F3, and such other keys work. See registerShortcuts().
 *
 * The search bar will turn red or green depending on whether the query string is found or not, you can
 * customize these colors with setColorFound() and setColorNotFound(). To customize these colors using
 * stylesheets, you may use the rules
 * <pre>QLineEdit[searchState="found"] {
 *     background-color: rgb(128,255,128,128);
 * }
 * QLineEdit[searchState="not-found"] {
 *     background-color: rgb(255,128,128,128);
 * }
 * </pre>
 * since the property \c searchState is set to one of \c "default", \c "focus-out", \c "found", \c "not-found",
 * or \c "aborted" depending on the current state.
 */
class KLF_EXPORT KLFSearchBar : public QFrame
{
  Q_OBJECT

  Q_PROPERTY(QString currentSearchText READ currentSearchText WRITE setSearchText) ;
  Q_PROPERTY(bool showOverlayMode READ showOverlayMode WRITE setShowOverlayMode) ;
  Q_PROPERTY(QRect showOverlayRelativeGeometry READ showOverlayRelativeGeometry
	     WRITE setShowOverlayRelativeGeometry ) ;
  Q_PROPERTY(QString focusOutText READ focusOutText WRITE setFocusOutText) ;
  Q_PROPERTY(QColor colorFound READ colorFound WRITE setColorFound) ;
  Q_PROPERTY(QColor colorNotFound READ colorNotFound WRITE setColorNotFound) ;
public:
  KLFSearchBar(QWidget *parent = NULL);
  virtual ~KLFSearchBar();
  virtual void registerShortcuts(QWidget *parent);

  /** Set the object upon which we will perform searches. As long as no object is
   * set this bar is unusable. */
  virtual void setSearchTarget(KLFSearchable *object);

  QString currentSearchText() const;
  inline bool showOverlayMode() const { return pShowOverlayMode; }
  inline QRect showOverlayRelativeGeometry() const { return pShowOverlayRelativeGeometry; }
  inline QString focusOutText() const { return pFocusOutText; }
  /** This value is read from the palette. It does not take into account style sheets. */
  QColor colorFound() const;
  /** This value is read from the palette. It does not take into account style sheets. */
  QColor colorNotFound() const;

  void setShowOverlayMode(bool showOverlayMode);
  void setShowOverlayRelativeGeometry(const QRect& relativeGeometryPercent);
  void setShowOverlayRelativeGeometry(int widthPercent, int heightPercent,
				      int positionXPercent, int positionYPercent);
  virtual void setColorFound(const QColor& color);
  virtual void setColorNotFound(const QColor& color);


  virtual bool eventFilter(QObject *obj, QEvent *ev);

signals:
  void searchPerformed(bool found);
  void found();
  void found(const QString& queryString, bool forward);
  void didNotFind();
  void didNotFind(const QString& queryString, bool forward);
  void searchAborted();

public slots:
  /** Clears the search bar and takes focus. */
  void clear();
  /** If the search bar does not have focus, takes focus and clears the bar. If it has focus, finds the
   * next occurence of the current or last search string. */
  void focusOrNext();
  void find(const QString& string, bool forward = true);
  void findNext(bool forward = true);
  void findPrev() { findNext(false); }
  void abortSearch();

  void focus();

  virtual void setSearchText(const QString& text);
  void setFocusOutText(const QString& focusOutText);

protected:
  Ui::KLFSearchBar *u;

  virtual void slotSearchFocusIn();
  virtual void slotSearchFocusOut();
  virtual void updateSearchFound(bool found);

  enum SearchState { Default, FocusOut, Found, NotFound, Aborted };

  virtual void displayState(SearchState state);

  void emitFoundSignals(bool found, const QString& searchstring, bool forward);

  /** sets the given \c text in the search bar, ensuring that the search bar will NOT emit
   * any textChanged() signals. */
  void showSearchBarText(const QString& text);

  /** Little helper: returns TRUE if the search bar has focus, FALSE otherwise. */
  bool searchBarHasFocus();

  virtual bool event(QEvent *event);


private:

  KLFSearchable *pTarget;

  QString pSearchText;
  QString pLastSearchText;

  KLFWaitAnimationOverlay *pWaitLabel;

  bool pShowOverlayMode;
  QRect pShowOverlayRelativeGeometry;

  QString pFocusOutText;

  QString palettePropName(SearchState state) const;
  QString statePropValue(SearchState state) const;

  friend class KLFSearchable;

  KLF_DEBUG_DECLARE_ASSIGNABLE_REF_INSTANCE()
};




#endif
