/***************************************************************************
 *   file klfiteratorsearchable.h
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

#ifndef KLF_ITERATORSEARCHABLE_H
#define KLF_ITERATORSEARCHABLE_H

#include <QString>
#include <QTime>
#include <QApplication>

#include <klfdefs.h>
#include <klfsearchbar.h>


//! A Searchable object interface based on iterative searching
/** Most searchable displays work with iterators, or objects that behave as such. This can be, for example,
 * an integer position in a displayed list. When searchFind() is called, then usually the search iteratively
 * looks at all items in the list, until a match is found.
 *
 * This class implements an interface of such iterator-based searchable displays for use as a KLFSearchable,
 * to be searched with a KLFSearchBar for example.
 *
 * The \a iterator may be any object that:
 *  - can be (logically) incremented and decremeneted. (this does not have to be with the operators -- and ++)
 *  - can be compared for equality with ==
 *  - has a copy constructor
 * apart from these conditions, the iterator may be any data type (example: \c int, \c QModelIndex, ...)
 *
 * The functions searchIterBegin() and searchIterEnd() have to be implemented, to return respectively an
 * iterator position for the first valid displayed element, and an iterator for the one-after-last-valid
 * displayed item.
 *
 * The function searchIterAdvance(), by default uses operator-(Iter, int) and operator+(Iter, int) to
 * increment or decrement the iterator by one. If this is not the correct way to increment/decrement
 * an iterator, reimplement this function to perform the job correctly. (for example to walk with
 * QModelIndex'es in a tree view).
 *
 * The search functions defined in this class guarantee never to increment an iterator that is already
 * equal to searchIterEnd(), and never to decrement an operator that is equal to searchIterBegin().
 *
 * The searchIterMatches() has to be reimplemented to say whether the data displayed at the given position
 * matches with the query string.
 *
 * This class provides implementations for KLFSearchable's searchFind(), searchFindNext() and searchAbort().
 * Additionally, it provides searchIterFind(), searchIterFindNext() which are a bit more flexible than
 * KLFSearchable's base functions (for example by returning the match position!).
 */
template<class Iter>
class KLF_EXPORT KLFIteratorSearchable : public KLFSearchable
{
public:
  KLFIteratorSearchable() : KLFSearchable(), pSearchAborted(false) { }
  virtual ~KLFIteratorSearchable() { }

  typedef Iter SearchIterator;


  // FUNCTIONS TO ACCESS SEARCHABLE DATA (REIMPLEMENT TO FIT YOUR NEEDS)

  /** Returns the first valid \c SearchIterator object. This should point to the element at top of the display, or be
   * equal to searchIterEnd() if the display is empty. */
  virtual SearchIterator searchIterBegin() = 0;

  /** Returns the one-after-last-valid \c SearchIterator object. This should NOT point to a valid object, however
   * it should either be equal to searchIterBegin() if the display is empty, or if searchIterPrev() is called
   * on it it should validly point on the last object in display. */
  virtual SearchIterator searchIterEnd() = 0;

  /** Increment or decrement iterator. The default implementation does <tt>pos+1</tt> or <tt>pos-1</tt>; you
   * can re-implement this function if your iterator cannot be incremented/decremented this way. */
  virtual SearchIterator searchIterAdvance(const SearchIterator& pos, bool forward) { return forward ? (pos+1) : (pos-1); }

  /** Increment iterator. Shortcut for searchIterAdvance() with \c forward = TRUE. */
  inline SearchIterator searchIterNext(const SearchIterator& pos) { return searchIterAdvance(pos, true); }

  /** Decrement iterator. Shortcut for searchIterAdvance() with \c forward = FALSE. */
  inline SearchIterator searchIterPrev(const SearchIterator& pos) { return searchIterAdvance(pos, false); }

  /** Returns the position from where we should start the search, given the current view situation. This
   * can be reimplemented to start the search for example from the current scroll position in the display.
   *
   * If \c forward is TRUE, then the search is about to be performed forward, otherwise it is about to
   * be performed in reverse direction.
   *
   * The default implementation returns searchIterBegin() to search from beginning if \c forward is TRUE,
   * or searchIterEnd() if \c forward is FALSE. */
  virtual SearchIterator searchIterStartFrom(bool forward)
  { return forward ? searchIterBegin() : searchIterEnd(); }

  /** See if the data pointed at by \c pos matches the query string \c queryString. Return TRUE if it
   * does match, or FALSE if it does not match.
   *
   * \c pos is garanteed to point on a valid object (this function will never be called with \c pos
   * equal to searchIterEnd()). */
  virtual bool searchIterMatches(const SearchIterator& pos, const QString& queryString) = 0;


  /** Virtual handler for the subclass to act upon the result of a search. A subclass may for example
   * want to select the matched item in a list to make it conspicuous to the user.
   *
   * \c resultMatchPosition is the position of the item that matched the search. If \c resultMatchPosition
   * is equal to searchIterEnd(), then the search failed (no match was found). (Note in this case
   * calling searchFindNext() again will wrap the search).
   *
   * The base implementation does nothing. */
  virtual void searchPerformed(const SearchIterator& resultMatchPosition) { Q_UNUSED(resultMatchPosition); }


  // FUNCTIONS THAT PERFORM THE SEARCH (NO NEED TO REIMPLEMENT)

  //! Find occurence of a search string
  /** Extension of searchFind(). Looks for \c queryString starting at position \c startPos.
   *
   * This function returns the position to the first match, or searchIterEnd() if no match was found.
   *
   * This function starts searching from \c startPos \a inclusive, in forward direction, if \c forward is
   * TRUE, and searches from \c startPos \a not inclusive [see reason below], in reverse direction, if
   * \c forward is FALSE.
   *
   * <i>Reason: iterators have an asymmetry, namely that 'begin' points on the first item while 'end' points
   * to one-past-end. We want to be able to search forward naturally from 'begin' and search reverse from
   * 'end', see also searchIterStartFrom().</i>
   *
   * This function need not be reimplemented, the default implementation should suffice for most cases. */
  virtual SearchIterator searchIterFind(const SearchIterator& startPos, const QString& queryString, bool forward)
  {
    klfDbg( " s="<<queryString<<" from "<<startPos<<" forward="<<forward ) ;
    // start searching from pCurPos _included_, but findNext() immediately increments to next position. so
    // simply go back one position before calling findNext()
    // however when finding reverse we are searching from pos _not inclusive_, see function dox doc
    if (forward)
      pCurPos = safe_cycl_advance_iterator(startPos, !forward);
    pSearchAborted = false;
    pQString = queryString;
    SearchIterator it = searchIterFindNext(forward);
    return it;
  }

  //! Find the next occurence of previous search string
  /** Extension of searchFindNext(), in that this function returns the position of the next match.
   *
   * Returns searchIterEnd() if no match was found.
   *
   * This function need not be reimplemented, the default implementation should suffice for most cases. */
  virtual SearchIterator searchIterFindNext(bool forward)
  {
    KLF_DEBUG_TIME_BLOCK(KLF_FUNC_NAME) ;
    pSearchAborted = false;
    if (pQString.isEmpty())
      return tee_notify_search_result(searchIterEnd());
    
    QTime t;
    
    bool found = false;
    while ( ! found ) {
      // advance iterator.
      
      pCurPos = safe_cycl_advance_iterator(pCurPos, forward);

      // stop if we reached the end
      if (pCurPos == searchIterEnd())
	break;
      
      // at this point pCurPos points on something valid
      
      if ( searchIterMatches(pCurPos, pQString) ) {
	found = true;
	break;
      }
      
      // call application's processEvents() from time to time to prevent GUI from freezing
      if (t.elapsed() > 150) {
	qApp->processEvents();
	if (pSearchAborted)
	  break;
	t.restart();
      }
    }
    if (found) {
      klfDbg( "found "<<pQString<<" at "<<pCurPos ) ;
      return tee_notify_search_result(pCurPos);
    }
    
    // not found
    return tee_notify_search_result(searchIterEnd());
  }


  // reimplemented from KLFSearchable (do not reimplement)

  /* Calls searchIterFind() with searchIterStartFrom() as start position.
   *
   * This function need not be reimplemented, the default implementation should suffice for most cases. */
  virtual bool searchFind(const QString& queryString, bool forward)
  {
    KLF_DEBUG_BLOCK(KLF_FUNC_NAME) ;
    return KLF_DEBUG_TEE( ! (searchIterFind(searchIterStartFrom(forward), queryString, forward)
			     == searchIterEnd()) );
  }

  /* Calls searchIterFindNext().
   *
   * This function need not be reimplemented, the default implementation should suffice for most cases. */
  virtual bool searchFindNext(bool forward)
  {
    KLF_DEBUG_BLOCK(KLF_FUNC_NAME) ;
    return KLF_DEBUG_TEE( ! (searchIterFindNext(forward) == searchIterEnd()) );
  }

  /* Aborts the search.
   *
   * If you reimplement this function to perform additional actions when aborting searches, make sure you
   * call the base class implementation, since searchAbort() may be called by the event loop while
   * refreshing the GUI during an active search, in which case the base implementation of this function
   * tells the search to stop.
   *
   * This function need not be reimplemented, the default implementation should suffice for most cases. */
  virtual void searchAbort()
  {
    pSearchAborted = true;
  }

  /** Advances iterator \c it safely, that means it incremenets or decrements the iterator while always
   * making sure not to perform illegal operations like incremenet an iterator that has arrived at
   * searchIterEnd() and making sure not to decrement an iterator that has arrived at searchIterBegin().
   *
   * Iterators that have arrived to searchIterEnd() or searchIterBegin(), when again incremented (resp.
   * decremented), wrap around and start again from the other end. Namely decrementing an iterator equal
   * to searchIterBegin() will give you searchIterEnd() and incrementing searchIterEnd() will yield
   * searchIterBegin().
   */
  SearchIterator searchAdvanceIteratorSafe(const SearchIterator& it, int n = 1)
  {
    if (n == 0)
      return it;
    bool forward = (n>0);
    if (n < 0)
      n = -n;
    // 'n' is number of steps (positive) to perform in direction 'forward'
    SearchIterator a = it;
    while (n--)
      a = safe_cycl_advance_iterator(a, forward);

    return a;
  }

protected:

  inline QString searchQueryString() const { return pQString; }
  inline SearchIterator searchCurrentIterPos() const { return pCurPos; }

private:
  QString pQString;
  /** The current I-Search position */
  SearchIterator pCurPos;
  /** Used as break condition on if when processing app events, one stops the search  */
  bool pSearchAborted;

  inline SearchIterator tee_notify_search_result(const SearchIterator& iter)
  {
    searchPerformed(iter);
    return iter;
  }

  inline SearchIterator safe_cycl_advance_iterator(const SearchIterator& it, bool forward)
  {
    if (forward) {
      if (it == searchIterEnd())
	return searchIterBegin();
      return searchIterNext(it);
    } else {
      if (it == searchIterBegin())
	return searchIterEnd();
      return searchIterPrev(it);
    }
  }
};



#endif
