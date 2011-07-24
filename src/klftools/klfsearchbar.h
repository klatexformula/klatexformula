/***************************************************************************
 *   file klfsearchbar.h
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


#ifndef KLFSEARCHBAR_H
#define KLFSEARCHBAR_H

#include <QObject>
#include <QWidget>
#include <QFrame>
#include <QMovie>
#include <QLabel>
#include <QTime>

#include <klfdefs.h>

#include <klfutil.h>

class QLineEdit;
class KLFWaitAnimationOverlay;

class KLFSearchBar;
class KLFSearchableProxy;
namespace Ui { class KLFSearchBar; }



//! An object that can be searched with a KLFSearchBar
/** This class is provides an interface for an object to be searched with a KLFSearchBar.
 *
 * An abstract positioning scheme is introduced with the class \ref Pos. This class handles
 * \c Pos objects without asking to what they refer to or point to, they could contain an index
 * in a list, or a QTextCursor in a document, or a pointer to some arbitrary data. A \c Pos is
 * valid if it has non-NULL \c posdata. You should subclass PosData to store some relevant data,
 * as well as provide an equality-test function.
 *
 * This interface, in conjunction with KLFSearchBar, handles the different search states,
 * searching forward/backwards, returning to previous results (e.g. with Emacs-style backspace key).
 *
 * How the search is performed, which logic is used, which order of elements is used, case sensitivity,
 * etc., is up to the subclass implementation. \c Pos objects do not have a sense of "order", the
 * interface just queries the "next match" from a given position, which can then be described by another
 * position object.
 *
 * Minimal example:
 * \todo MINIMAL EXAMPLE
 *
 * \note If you have data that is "linearly ordered", i.e. that you can describe the possible search
 *   results with a C++/STL-like iterator concept (for example a list of strings or values, etc.), then
 *   consider using KLFIteratorSearchable which takes this into account and requires you to write less
 *   code.
 */
class KLF_EXPORT KLFPosSearchable : public KLFTarget
{
public:

  /** \brief An abstract position in a searchable object
   *
   * Used by KLFPosSearchable to store search result positions.
   *
   * A position can be \a invalid, or \a valid. The actual data representing the position is stored
   * in a custom sub-class of PosData, to which a pointer is held in \c posdata.
   *
   * You can construct an invalid position with staticInvalidPos(). Then just assign a data pointer
   * to it and it becomes valid, e.g.
   * \code
   *   Pos p = Pos::staticInvalidPos();
   *   // p is invalid
   *   MyPosData *d = new MyPosData;
   *   d->somefield = some_data;
   *   d->someotherfield = some_other_data;
   *   p.posdata = d;
   *   // now p is a valid position object, storing the position represented
   *   // by data stored in 'somefield' and 'someotherfield'
   * \endcode
   */
  struct Pos {
    /** \brief A Base class for storing abstract position data
     *
     * See \ref KLFPosSearchable::Pos. Derive this class to store relevant data to represent
     * you position and an equality tester function.
     *
     * Minimal Example:
     * \code
     *  class MySearchable : public KLFPosSearchable
     *  {
     *  public:
     *    // ...
     *    class MyPosData : public Pos::PosData {
     *      QTextCursor cursor;
     *
     *      bool equals(PosData * other) const {
     *        return cursor == dynamic_cast<MyPosData*>(other)->cursor;
     *      }
     *    };
     *    // ...
     *  };
     * \endcode
     *
     * Additionally, this class handles ref()/deref() referencing mechanism that is used by
     * \ref KLFRefPtr. It handles automatically reference counts, and you don't have to worry
     * about deleting the object.
     */
    struct PosData {
      PosData() : r(0) { }

      /** tests for equality with other position. */
      virtual bool equals(PosData *other) const = 0;

      /** Subclasses may reimplement to return a string describing the current position
       * for debugging messages. */
      virtual QString toDebug() { return QLatin1String("<PosData>"); }

      int ref() { return ++r; }
      int deref() { return --r; }

      /** Subclasses should reimplement to return FALSE if this object should NOT be \c delete'd when no
       * longer referenced. There should be no reason to do so, however. */
      virtual bool wantAutoDelete() { return true; }
    private:
      int r;
    };

    Pos(const Pos& other) : posdata(other.posdata)
    {
    }

    ~Pos()
    {
      if (posdata != NULL)
	posdata.setAutoDelete(posdata->wantAutoDelete());
    }

    Pos& operator=(const Pos& other)
    {
      KLF_DEBUG_BLOCK(KLF_FUNC_NAME) ;
      posdata = other.posdata;
      return *this;
    }

    /** A position is valid if it has a non-NULL posdata pointer. It is invalid otherwise. */
    bool valid() const {
      KLF_DEBUG_BLOCK(KLF_FUNC_NAME) ;
      klfDbg("posdata="<<posdata) ;
      return  (posdata != NULL) ;
    };
    /** Is equal to \c other if:
     * - both this and the other position are valid, and \c posdata 's equals() test is true;
     * - or both are invalid.
     *
     * \note \c PosData's equals() test called if and only if both this and the other position
     *   are valid, i.e. have non-NULL \c posdata pointer.
     */
    bool equals(const Pos& other) const {
      if (valid() && other.valid())
	return posdata->equals(other.data<PosData>());
      // if both are invalid, they are equal. If one only is valid, they are not equal.
      return (valid() == other.valid());
    }

    //! Stores the actual position data, see \ref PosData
    /**
     * This pointer is set up by KLFPosSearchable subclasses to instanciate valid Pos objects.
     * They may use this object transparently, just as a regular <tt>PosData*</tt> pointer, see
     * \ref KLFRefPtr.
     *
     * \note \ref PosData::ref() and \ref PosData::deref() are handled automatically when
     *   this field is assigned. See also \ref KLFRefPtr.
     */
    KLFRefPtr<PosData> posdata;

    /** \brief A shorthand for retrieving the \c posdata cast into the custom type.
     *
     * Example:
     * \code
     *   // Instead of:
     *   MyPosData * myposdata = dynamic_cast<MyPosData*>(pos.posdata);
     *   // we can use
     *   MyPosData * myposdata = pos.data<MyPosData>();
     * \endcode
     *
     * Additionally, a warning is issued if \c posdata is \c NULL or if \c posdata cannot
     * by cast (with \c dynamic_cast<>) to the required type.
     */
    template<class TT>
    inline TT * data() const
    {
      TT *ptr = posdata.dyn_cast<TT*>();
      KLF_ASSERT_NOT_NULL(ptr, "accessing a posdata that is NULL or of incompatible type!", return NULL;) ;
      return ptr;
    }

    /** Returns an invalid Pos. */
    static Pos staticInvalidPos() { return Pos(); }

  private:
    /** Use staticInvalidPos() or KLFPosSearchable::invalidPos() to create a Pos object */
    Pos() : posdata() { KLF_DEBUG_BLOCK(KLF_FUNC_NAME) ; }
  };

  /** Returns the position from where we should start the search, given the current view situation. This
   * can be reimplemented to start the search for example from the current scroll position in the display.
   *
   * If \c forward is TRUE, then the search is about to be performed forward, otherwise it is about to
   * be performed in reverse direction.
   *
   * The search is performed AFTER the returned pos, and invalidPos() is regarded as being both before
   * the beginning and after the end, ie. to search forward from the beginning, use invalidPos() as well
   * as to search backward from the end.
   *
   * The default implementation returns invalidPos().
   */
  virtual Pos searchStartFrom(bool forward);

  /** Search the content for \c queryString, from position \c fromPos, in direction \c forward.
   * If found, return the position of the match, if not found, return an invalid position. Do not
   * act upon the find (eg. select element in list), because the function searchMoveTo() and
   * searchPerformed() will be called after a successful searchFind() automatically, while the
   * function searchMoveTo() will be called with an invalid position and the function searchPerformed()
   * will be called if the search failed.
   *
   * \note The search is performed \a AFTER \c fromPos, ie. do not return \c fromPos itself if it
   *   matches.
   *
   * The reimplementation should call from time to time
   * \code qApp->processEvents() \endcode
   * to keep the GUI from freezing in long searches.
   *
   * \note If the reimplementation implements the above suggestion, note that the slot
   *   \ref searchAborted() may be called during that time! It is best to take that into
   *   account and provide a means to stop the search if that is the case.
   */
  virtual Pos searchFind(const QString& queryString, const Pos& fromPos, bool forward) = 0;

  /** Called by the search bar to inform the searched object that the current search position
   * is \c pos. */
  virtual void searchMoveToPos(const Pos& pos)  { Q_UNUSED(pos);  }

  /** Called by the search bar to inform that the \c queryString was reported either to be found
   * (in which case \c found is TRUE), either not to be found (then \c found is FALSE), at
   * position \c pos. This function call always immediately preceded by a searchMoveToPos().
   * \c pos is invalid if the query string was not found (\c found==FALSE). */
  virtual void searchPerformed(const QString& queryString, bool found, const Pos& pos)
  {  Q_UNUSED(queryString); Q_UNUSED(found); Q_UNUSED(pos);  }

  /** Called by the search bar to inform the searched object that the search was aborted by the
   * user. */
  virtual void searchAborted() = 0;

  /** Subclasses should override to use this function as Pos object factory. See also
   * \ref Pos::valid().
   *
   * This function is used as constructor for \c Pos objects. You may derive it to initialize
   * your Pos objects appropriately with PosData object instances if wanted, eg.
   * \code
   * class MyPosData : public KLFSearchable::Pos::PosData {
   *   ... // data...
   *   virtual bool valid() const { ... };
   *   virtual bool equals(const Pos& other) const { ... };
   * }
   * KLFPosSearchable::Pos MyPosSearchable::invalidPos() {
   *   Pos p = KLFPosSearchable::invalidPos();
   *   p.posdata = new MyPosData;
   *   return p;
   * }
   * \endcode
   *
   * Note that however, your subclass should also understand the \ref Pos object instance returned
   * by Pos::staticInvalidPos() as being an invalid position.
   *  */
  virtual Pos invalidPos() { return Pos::staticInvalidPos(); };

  /** Called when search bar has focus, but not text is typed. Typically if user hits
   * backspace enough times to an empty string.
   */
  virtual void searchReinitialized() { }


  /** \brief The current query string.
   *
   * This function can be used by subclasses to retrieve the current search string.
   */
  virtual QString searchQueryString() const { klfDbg("pQString="<<pQString) ; return pQString; }

  /** \internal
   * Used internally to update the return value of searchQueryString().
   */
  virtual void setSearchQueryString(const QString& s) { klfDbg("pQString="<<pQString<<"; setting to "<<s) ; pQString = s; }

  virtual bool searchHasInterruptRequested() { return pInterruptRequested; }

  virtual void setSearchInterruptRequested(bool on);

private:
  QString pQString;
  bool pInterruptRequested;
};

KLF_EXPORT QDebug& operator<<(QDebug& str, const KLFPosSearchable::Pos& pos);


class KLFPosSearchableProxy : public KLFPosSearchable, public KLFTargeter
{
public:
  KLFPosSearchableProxy() { }
  virtual ~KLFPosSearchableProxy();

  virtual Pos searchStartFrom(bool forward);
  virtual Pos searchFind(const QString& queryString, const Pos& fromPos, bool forward);
  virtual void searchMoveToPos(const Pos& pos);
  virtual void searchPerformed(const QString& queryString, bool found, const Pos& pos);
  virtual void searchAborted();
  virtual Pos invalidPos();
  virtual void searchReinitialized();
  virtual QString searchQueryString() const;
  virtual void setSearchQueryString(const QString& s);
  virtual bool searchHasInterruptRequested();
  virtual void setSearchInterruptRequested(bool on);

  virtual void setSearchTarget(KLFPosSearchable *t) { setTarget(t); }

protected:
  virtual KLFPosSearchable *target() { return dynamic_cast<KLFPosSearchable*>(pTarget); }
  virtual const KLFPosSearchable *target() const { return dynamic_cast<KLFPosSearchable*>(pTarget); }
};




//! An interface for objects that can be I-searched with a KLFSearchBar (OBSOLETE)
/**
 * <b>THIS CLASS IS OBSOLETE</b>. Use KLFPosSearchable instead.
 *
 * This class is the base skeleton interface for displays that will be targets for I-searches.
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
class KLF_EXPORT KLFSearchable : public KLFPosSearchable
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


  virtual Pos searchFind(const QString& queryString, const Pos& fromPos, bool forward);
  virtual void searchMoveToPos(const Pos& pos)
  {  Q_UNUSED(pos);  }
  virtual void searchPerformed(const QString& queryString, bool found, const Pos& pos)
  {  Q_UNUSED(queryString); Q_UNUSED(found); Q_UNUSED(pos);  }
  virtual void searchAborted() { searchAbort(); }
};


//! A proxy class that relays search queries to another searchable object (OBSOLETE)
/**
 * THIS CLASS IS OBSOLETE. Use KLFPosSearchableProxy instead.
 *
 * This class may be used for example when you have global search bar, but many sub-windows or sub-displays
 * displaying different data, and the search bar should search within the active one.
 */
class KLF_EXPORT KLFSearchableProxy : public KLFSearchable, public KLFTargeter
{
public:
  KLFSearchableProxy() : KLFTargeter() { }
  virtual ~KLFSearchableProxy();
  
  void setSearchTarget(KLFPosSearchable *target) { setTarget(target); }
  virtual void setTarget(KLFTarget *target);

  virtual bool searchFind(const QString& queryString, bool forward);
  virtual bool searchFindNext(bool forward);
  virtual void searchAbort();

protected:
  virtual KLFSearchable *target() { return dynamic_cast<KLFSearchable*>(pTarget); }
};




class KLFSearchBarPrivate;

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
 * The shortcuts are NOT enabled by default, you need to call registerShortcuts().
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
class KLF_EXPORT KLFSearchBar : public QFrame, public KLFTargeter
{
  Q_OBJECT

  Q_PROPERTY(QString currentSearchText READ currentSearchText) ;
  Q_PROPERTY(bool autoHide READ autoHide WRITE setAutoHide) ;
  Q_PROPERTY(bool showOverlayMode READ showOverlayMode WRITE setShowOverlayMode) ;
  Q_PROPERTY(QRect showOverlayRelativeGeometry READ showOverlayRelativeGeometry
	     WRITE setShowOverlayRelativeGeometry ) ;
  Q_PROPERTY(QString focusOutText READ focusOutText WRITE setFocusOutText) ;
  Q_PROPERTY(QColor colorFound READ colorFound WRITE setColorFound) ;
  Q_PROPERTY(QColor colorNotFound READ colorNotFound WRITE setColorNotFound) ;
  Q_PROPERTY(bool showHideButton READ hideButtonShown WRITE setShowHideButton) ;
  Q_PROPERTY(bool showSearchLabel READ showSearchLabel WRITE setShowSearchLabel) ;
  Q_PROPERTY(bool emacsStyleBackspace READ emacsStyleBackspace WRITE setEmacsStyleBackspace) ;
public:

  enum SearchState { Default, FocusOut, Found, NotFound, Aborted };

  KLFSearchBar(QWidget *parent = NULL);
  virtual ~KLFSearchBar();
  /** Enables shortcuts such as C-F, C-S, /, F3 etc. The \c parent is given to the
   * \ref QShortcut constructor. */
  virtual void registerShortcuts(QWidget *parent);

  /** Set the object upon which we will perform searches. As long as no object is
   * set this bar is unusable. */
  virtual void setSearchTarget(KLFPosSearchable *target) { setTarget(target); }
  virtual void setTarget(KLFTarget *target);

  QString currentSearchText() const;
  bool autoHide() const;
  bool showOverlayMode() const;
  QRect showOverlayRelativeGeometry() const;
  QString focusOutText() const;
  /** This value is read from the palette. It does not take into account style sheets. */
  QColor colorFound() const;
  /** This value is read from the palette. It does not take into account style sheets. */
  QColor colorNotFound() const;
  bool hideButtonShown() const;
  bool showSearchLabel() const;
  bool emacsStyleBackspace() const;

  /** Returns the current position in the searched object. This is useful only if you
   * know how the searched object uses KLFPosSearchable::Pos structures. */
  KLFPosSearchable::Pos currentSearchPos() const;

  SearchState currentState() const;

  /** Hides the search bar when it does not have focus.
   */
  void setAutoHide(bool autohide);

  /** Sets the overlay mode. If overlay mode is on, then the search bar is displayed overlaying the
   * parent widget, with no specific layout, possibly hiding other widgets. It is hidden as soon as
   * the search is over.
   *
   * You may use, eg. the keyboard shortcuts to activate the search and show the search bar. */
  void setShowOverlayMode(bool showOverlayMode);
  void setShowOverlayRelativeGeometry(const QRect& relativeGeometryPercent);
  void setShowOverlayRelativeGeometry(int widthPercent, int heightPercent,
				      int positionXPercent, int positionYPercent);
  void setColorFound(const QColor& color);
  void setColorNotFound(const QColor& color);
  void setShowHideButton(bool showHideButton);
  void setShowSearchLabel(bool show);
  void setEmacsStyleBackspace(bool on);

  virtual bool eventFilter(QObject *obj, QEvent *ev);

  QLineEdit * editor();

signals:
  void stateChanged(SearchState state);
  void searchPerformed(bool found);
  void searchPerformed(const QString& queryString, bool found);
  void found();
  void found(const QString& queryString, bool forward);
  void found(const QString& queryString, bool forward, const KLFPosSearchable::Pos& pos);
  void didNotFind();
  void didNotFind(const QString& queryString, bool forward);
  void searchAborted();
  void escapePressed();
  void searchReinitialized();

  //! Reflects whether the search is currently pointing on a valid result
  /** Emitted with argument TRUE each time that a successful search is performed; emitted with
   * FALSE if the query string is not found, or if the search is aborted, or if the user
   * resets the search. */
  void hasMatch(bool hasmatch);

  void visibilityChanged(bool isShown);

public slots:
  /** Clears the search bar and takes focus. */
  void clear();
  /** If the search bar does not have focus, takes focus and clears the bar, preparing to search
   * in forward direction (unless \c forward is FALSE). If it has focus, finds the next occurence
   * (resp. previous if \c forward is FALSE) of the current or last search string. */
  void focusOrNext(bool forward = true);
  /** If the search bar does not have focus, takes focus and clears the bar, preparing for a backwards
   * search. If it has focus, finds the previous occurence of the current or last search string. */
  void focusOrPrev() { focusOrNext(false); }
  void find(const QString& string);
  void find(const QString& string, bool forward);
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

  void promptEmptySearch();

  /** Does not change d->pState. Only sets up UI for the given state. */
  virtual void displayState(SearchState state);

  /** Updates d->pState, emits the stateChanged() signal, and calls displayState(). */
  void setCurrentState(SearchState state);

  void emitFoundSignals(const KLFPosSearchable::Pos& pos, const QString& searchstring, bool forward);

  /** sets the given \c text in the search bar, ensuring that the search bar will NOT emit
   * any textChanged() signals. */
  void showSearchBarText(const QString& text);

  /** Little helper: returns TRUE if the search bar has focus, FALSE otherwise. */
  bool searchBarHasFocus();

  virtual bool event(QEvent *event);


  bool _isInQtDesigner;
  friend class KLFSearchBarDesPlugin;

private:

  inline KLFPosSearchable *target() { return dynamic_cast<KLFPosSearchable*>(pTarget); }

  KLFSearchBarPrivate *d;

  void adjustOverlayGeometry();

  QString palettePropName(SearchState state) const;
  QString statePropValue(SearchState state) const;

  // Needed so that KLFSearchable's can ensure \ref pTarget is valid, and set it to NULL
  // when appropriate
  friend class KLFSearchable;

  void performFind(bool forward, bool isFindNext = false);

  KLF_DEBUG_DECLARE_ASSIGNABLE_REF_INSTANCE()
};




#endif
