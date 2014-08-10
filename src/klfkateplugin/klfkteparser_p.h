/***************************************************************************
 *   file klfkteparser_p.h
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


#ifndef KLFKTEPARSER_P_H
#define KLFKTEPARSER_P_H

#include <algorithm>

#include "klfkteparser.h"


// --------------------------------------------------

// handy shorthands


/** Like \ref QString::replace(QRegExp, QString), except returns a copy without affecting
 * the original string.
 */
inline QString my_str_replaced(const QString& s, const QRegExp& rx, const QString& replacement)
{
  QString copy = s;
  copy.replace(rx, replacement);
  return copy;
}


/** An alternative implementation of QRegExp::lastIndexIn(), with better greedy matching.
 *
 * This first matches with indexIn() all matches in s in the forward direction, then
 * returns the position of the last match.
 *
 * For example, matching "abc $$ def" with "($|$$)" will return the position of the
 * double-$$ instead of the last $ only.
 */
static int my_last_index_in(QRegExp& rx, const QString& s)
{
  KLF_DEBUG_BLOCK(KLF_FUNC_NAME) ;

  int lasti = -1;
  int lastlen = 0;
  int i = 0;

  while ( (i = rx.indexIn(s, i+lastlen)) != -1 ) {
    klfDbg("   match at "<<i<<", len="<<rx.matchedLength()) ;
    lasti = i;
    lastlen = rx.matchedLength();
  }

  if (lasti != -1) {
    // re-match, so that the rx object has the right matchedLength(), cap() etc.
    rx.indexIn(s, lasti);
  }

  klfDbg("  last match was at "<<lasti<<": rx.cap()="<<rx.cap());

  return lasti;
}



/* * Step the cursor one step left or right. If the cursor goes beyond the beginning or the
 * end of the line, move at end of previous / at beginning of next line.
 *
 * If \c forward is \c true, steps the cursor one position forward (right), otherwise one
 * position backwards (left).
 *
 * This function does not affect the original cursor \c c, but returns a new cursor
 * corresponding to the new position.
 */
/*static Cur step_cur(KTextEditor::Document * doc, const Cur& c, bool forward)
{
  int step = forward ? +1 : -1;

  Cur newc(c.line(), c.column()+step);
  if (doc->cursorInText(newc))
    return newc;
  // step line instead
  if (step < 0)
    return Cur(c.line()+step, doc->lineLength(c.line()+step)-1);
  // step > 0
  return Cur(c.line()+step, 0);
}
*/


// =============================================================================
//
// DUMMY PARSER (do nothing)
//
// =============================================================================


class KLFKteParser_Dummy : public KLFKteParser
{
public:
  KLFKteParser_Dummy(Doc *doc) : KLFKteParser(doc) { }

  MathModeContext parseContext(const Cur& /*cur*/)
  {
    return MathModeContext();
  }
};






// =============================================================================
//
// FULL-BLOWN MATH MODE EXTRACTOR, USING KTEXTEDITOR'S LATEX HIGHLIGHTING ENGINE
//
// =============================================================================



// See API:
// http://api.kde.org/4.x-api/kdelibs-apidocs/interfaces/ktexteditor/html/classKTextEditor_1_1HighlightInterface.html


#define KatePartAttrNameProperty KTextEditor::Attribute::AttributeInternalProperty

//! Remember: "Attribute" \approx QTextCharFormat, i.e. character display format.
typedef KTextEditor::Attribute HiAtt;
//! "AttributeBlock": an "Attribute", with a given start position and length.
typedef KTextEditor::HighlightInterface::AttributeBlock HiAttBlock;
typedef QList<KTextEditor::HighlightInterface::AttributeBlock> HiAttList;


/** compare to HiAttBlock's for sorting. Sorting is based on *start* positions of the
 * blocks
 */
struct HiAttElem_cmp_start {
  inline bool operator()(const HiAttBlock& a, const HiAttBlock& b) const
  {
    return a.start < b.start;
  }
};
/** compare to HiAttBlock's for sorting. Sorting is based on *end* positions of the blocks
 */
struct HiAttElem_cmp_end {
  inline bool operator()(const HiAttBlock& a, const HiAttBlock& b) const
  {
    return (a.start + a.length) < (b.start + b.length);
  }
};


#define MATHENV  "(?:"  "equation|displaymath|eqnarray|subeqnarray|math|multline|gather|align|flalign"  ")"

// note: weakness when looking for regexp's backwards: $$ will first match a single '$'...
#define RX_STARTMATH "(\\${1,2}|\\\\\\(|\\\\\\[|\\\\begin\\{(" MATHENV ")\\*?\\})"
#define RX_ENDMATH "(\\${1,2}|\\\\\\)|\\\\\\]|\\\\end\\{(" MATHENV ")\\*?\\})"

#define RX_STARTORENDMATH "(?:" RX_STARTMATH "|" RX_ENDMATH ")"



/** Returns the type name of the given attribute. Can be "LaTeX:Comment" for example.
 */
static inline QString att_type(const HiAttBlock& att)
{
  const KTextEditor::Attribute * a = att.attribute.data();
  KLF_ASSERT_NOT_NULL(a, "attribute data is NULL!", return QString(); ) ;
  if (!a->hasProperty(KatePartAttrNameProperty)) {
    klfWarning("attribute does not have property KTextEditor::Attribute::AttributeInternalProperty="
               <<KTextEditor::Attribute::AttributeInternalProperty);
    return QString();
  }
  return a->property(KatePartAttrNameProperty).toString();
}

/** Returns TRUE if the given attribute corresponds to the highlighing of a mathematical
 * LaTeX section.
 */
static inline bool att_is_math(const HiAttBlock& att)
{
  return att_type(att).contains("math", Qt::CaseInsensitive);
}

static inline bool att_is_comment(const HiAttBlock& att)
{
  return att_type(att).contains("comment", Qt::CaseInsensitive);
}

/** A KTextEditor::HighlightInterface::AttributeBlock (alias HiAttBlock), but with the
 * line information (the line number on which the attribution information corresponds to).
 */
struct HiAttElem : public HiAttBlock
{
  /** Construct a default, invalid, HiAttElem, with NULL attribute. */
  HiAttElem()
    : HiAttBlock(-1, -1, KTextEditor::Attribute::Ptr()),
      line(-1), cache_ismath(-1)
  {
  }
  /** Construct a HiAttElem by copying the given HiAttBlock. */
  HiAttElem(const HiAttBlock& h, int l = -1)
    : HiAttBlock(h),
      line(l), cache_ismath(-1)
  {
  }
  /** Construct a HiAttElem with an invalid attribute block, but a (most likely) valid
   * line. */
  HiAttElem(int l)
    : HiAttBlock(-1, -1, KTextEditor::Attribute::Ptr()),
      line(l), cache_ismath(-1)
  {
  }
  /** copy constructor */
  HiAttElem(const HiAttElem& copy)
    : HiAttBlock(copy),
      line(copy.line), cache_ismath(copy.cache_ismath)
  {
  }
  
  int line;
  
  /** Create a Cursor object corresponding to the start of this HiAtt element */
  inline Cur startCur() const { return Cur(line, start); }
  /** Create a Cursor object corresponding to the end of this HiAtt element */
  inline Cur endCur() const { return Cur(line, start+length); }
  
  /** Returns TRUE if the current attribute is a math highlighting attribute.
   *
   * Note that an invalid attribute is not math mode. A warning is issued if the line
   * number is also invalid.
   */
  inline bool isMath() const
  {
    // warning for invalid line number, but in fact nothing prevents us from continuing
    KLF_ASSERT_CONDITION(line >= 0, "Invalid line number line="<<line, ; );

    if (cache_ismath != -1) {
      return cache_ismath;
    }
    if (start < 0 || length < 0 || attribute.data() == NULL) {
      return false;
    }
    cache_ismath = (int)att_is_math(*this);
    return cache_ismath;
  }
  
private:
  /** Cache the answer to \ref isMath() (1/0, or -1 if not cached yet). */
  mutable int cache_ismath;
};

/** Store information about the boundary in a document between math mode and not math
 * mode.
 */
struct MathModeBoundary
{
  MathModeBoundary()
    : cur(Cur::invalid()), istrue(false), isstart(true), boundarycommand()
  {
  }

  MathModeBoundary(Cur c, bool t, bool st, const QString& bc)
    : cur(c), istrue(t), isstart(st), boundarycommand(bc)
  {
  }
  
  /** \brief Position in the document.
   *
   * If this is a start math mode, the position corresponds to the cursor position right
   * *after* the command, otherwise, right *before* the command.
   */
  Cur cur;

  /** Whether this boundary is "true". A "true" boundary is one which we are interested
   * for extracting an equation, and is recognized by matching to a known boundary command
   * such as \c "\begin{equation}".
   *
   * A non-"true" boundary can arise, for example, if in the equation a command such as
   * \c "\text{...}" is used, in which case we will ultimately want to extract the full
   * equation.
   */
  bool istrue;

  //! TRUE if at this boundary math mode starts, FALSE if it ends.
  bool isstart;
  
  /** The LaTeX command that causes change in LaTeX math mode.
   *
   * e.g. \c "\begin{equation}", or \c "$"
   */
  QString boundarycommand;
};


inline QDebug operator<<(QDebug dbg, const MathModeBoundary& m)
{
  return dbg << "MathModeBoundary("<<m.cur<<", istrue="<<m.istrue<<", isstart="<<m.isstart<<", cmd="
	     <<m.boundarycommand<<")";
}
inline QDebug operator<<(QDebug dbg, const HiAttBlock& a)
{
  return dbg << "{"<<a.start<<"+"<<a.length<<" ("<<a.attribute->property(KatePartAttrNameProperty)<<")}" ;
}
inline QDebug operator<<(QDebug dbg, const HiAttElem& a)
{
  dbg << "{l."<<a.line<<" "<<a.start<<"+"<<a.length;
  if (a.attribute.data() != NULL) {
    return dbg << " ("<<a.attribute->property(KatePartAttrNameProperty)<<")}" ;
  } else {
    return dbg << " (NULL)}";
  }
}



// ---------------------


/** While scanning forward/backward from a given position for beginning/ending of math
 * mode block, stop at most after this number of lines.
 */
#define MAX_NUM_LINES_TO_SCAN 50



class KLFKteParser_KatePart : public KLFKteParser
{
  KTextEditor::HighlightInterface * pHiface;
public:

  KLFKteParser_KatePart(Doc *doc, KTextEditor::HighlightInterface * hiface)
    : KLFKteParser(doc), pHiface(hiface)
  {
    KLF_ASSERT_NOT_NULL(pHiface, "highlight-interface is NULL!", ; ) ;
  }

public:

  /** The main function that parses the "math mode information" at the current cursor
   * position
   */
  MathModeContext parseContext(const Cur& cur)
  {
    KLF_DEBUG_BLOCK(KLF_FUNC_NAME) ;

    // parse mathmode/non-mathmode transitions

    MathModeBoundary headm, tailm;

    headm = find_true_mathmode_transition(cur, false /*backwards*/);
    if (!headm.cur.isValid()) {
      klfDbg("failed to parse head mathmode boundary: invalid headm returned.") ;
      return MathModeContext();
    }

    if (!headm.isstart) {
      klfDbg("we are not in math mode. headm="<<headm);
      return MathModeContext();
    }
    
    tailm = find_true_mathmode_transition(cur, true /*forward*/);
    if (!tailm.cur.isValid()) {
      klfDbg("failed to parse tail mathmode boundary: invalid tailm returned.") ;
      return MathModeContext();
    }

    KLF_ASSERT_CONDITION(!tailm.isstart, "!!: tailcur is not math-mode end ?!", return MathModeContext(); ) ;

    // calculate mathmode region, get string
    QString latexmath = pDoc->text(KTextEditor::Range(headm.cur, tailm.cur));
    
    klfDbg("Boundaries: "<<headm<<" --- "<<tailm<<"\n\tGot latex math: "<<latexmath) ;

    MathModeContext m;
    m.start = headm.cur;
    m.end = tailm.cur;
    m.startcmd = headm.boundarycommand;
    m.endcmd = tailm.boundarycommand;
    m.latexmath = latexmath;

    klfDbg("math mode context: "<<m) ;

    return m;
  }

private:


  /** Scan backwards or forwards starting from a given position, while looking for a math
   * mode boundary.
   *
   * \param pos: start looking from here.
   * \param forward: scans forward if TRUE, otherwise backwards.
   */
  MathModeBoundary
  /* */ find_true_mathmode_transition(Cur pos, bool forward)
  {
    KLF_DEBUG_TIME_BLOCK(KLF_FUNC_NAME) ;
    klfDbg("position=["<<pos<<"], forward="<<forward) ;

    KLF_ASSERT_NOT_NULL(pHiface, "pHiface is NULL !", return MathModeBoundary(); ) ;

    int line = pos.line();
    int col = pos.column();

    int step = forward ? +1 : -1;

    int kpos = 0;

    int numscannedlines = 0;

    QRegExp mboundary = QRegExp(RX_STARTORENDMATH);
      
    while (line >= 0 && line < pDoc->lines()) {

      ++numscannedlines;
      if (numscannedlines > MAX_NUM_LINES_TO_SCAN) {
        break;
      }

      klfDbg("line iteration: line="<<line<<":\n\t> "<<qPrintable(pDoc->line(line)));
      
      if (!forward && col == 0) {
        // at start of line, scanning back -> go to prev line
        col = -1;
        --line;
        continue;
      }
      if (forward && col == pDoc->lineLength(line)) {
        // at end of line, scanning forward -> go to next line
        col = -1;
        ++line;
        continue;
      }

      QString sline = pDoc->line(line);

      // see if we can find a math mode boundary.
      if (forward) {
        kpos = mboundary.indexIn(sline, (col >= 0) ? col : 0);
      } else {
        // make sure the pattern doesn't match past col position.
        if (col >= 0) {
          kpos = my_last_index_in(mboundary, sline.left(col));
        } else {
          kpos = my_last_index_in(mboundary, sline);
        }
      }

      if (kpos == -1) {
        // pattern not found. so skip to prev/next line.
        col = -1;
        line += step;
        continue;
      }

      int kposend = kpos + mboundary.matchedLength();

      // we found a pattern. Now, try to determine, using the syntax highlighter
      // interface, whether it is a starting or an ending of a math mode block.

      HiAttElem attleft  = att_left_or_right_of(pDoc, pHiface, line, kpos, LeftOf);
      HiAttElem attright = att_left_or_right_of(pDoc, pHiface, line, kposend, RightOf);

      klfDbg("Inspecting "<<mboundary.cap()<<" @line="<<line<<",col="<<kpos<<"-"<<kposend
             <<";  attleft="<<attleft<<"; attright="<<attright) ;

      /* this should no longer be needed:

      if (attleft.line == -1) {
        // didn't find attribute for "just left" --> assume normal text, not math mode
        // and we detected a boundary, so it should be a start math mode block.

        // we should have detected a math mode on the "right", just check
        KLF_ASSERT_CONDITION(attright.line != -1 && attright.isMath(), 
                             "?!?? Detected boundary, no attribute left, but no math right?? "
                             "attleft="<<attleft<<"; attright="<<attright ,
                             ; );
        return MathModeBoundary(Cur(line, kposend), true / * istrue * /, true / * isstart * /,
                                mboundary.cap() );
      }

      if (attright.line == -1) {
        // didn't find attribute for "just right" --> assume normal text
        // and we detected a boundary, so it should be an end math mode block.
        
        // we should have detected a math mode on the "left", just check
        KLF_ASSERT_CONDITION(attleft.line != -1 && attleft.isMath() ,
                             "?!?? Detected boundary, no attribute right, but no math left?? "
                             "attleft="<<attleft<<"; attright="<<attright ,
                             ; );
        return MathModeBoundary(Cur(line, kpos), true / * istrue * /, false / * isstart * /,
                                mboundary.cap() );
        }
      */

      // instead, the more simple check: indeed, this happens only if the scan went out of
      // bounds, see `att_left_or_right_of()` below.
      if (attleft.line == -1 || attright.line == -1) {
        klfDbg("Didn't manage to find matching left/right attributes (even \"fake\" ones): "
               << "attleft="<< attleft<< "; attright="<<attright) ;
        return MathModeBoundary();
      }


      // we have both before and after attributes.

      if (attleft.isMath()) {
        // we have math on the left --> should be ending boundary

        KLF_ASSERT_CONDITION(!attright.isMath(),
                             "?!?? Detected boundary, math left, but math also right?? "
                             "attleft="<<attleft<<"; attright="<<attright ,
                             ; );

        return MathModeBoundary(Cur(line, kpos), true /* istrue */, false /* isstart */,
                                mboundary.cap() );
      }

      // we have no math on the left --> should be start boundary

      KLF_ASSERT_CONDITION(attright.isMath(),
                           "?!?? Detected boundary, no math left, but also no math right?? "
                           "attleft="<<attleft<<"; attright="<<attright ,
                           ; );

      return MathModeBoundary(Cur(line, kposend), true /* istrue */, true /* isstart */,
                              mboundary.cap() );


    }

      
    klfDbg("failed to find mathmode boundary..") ;

    // return invalid mathmodeboundary object
    return MathModeBoundary();
  }



  enum LeftOrRightOf { LeftOf = 0, RightOf };

  HiAttElem att_left_or_right_of(KTextEditor::Document * doc, KTextEditor::HighlightInterface * hiface,
                                 int line, int kpos, LeftOrRightOf direction)
  {
    HiAttList attlist;

    QRegExp mboundary = QRegExp(RX_STARTORENDMATH);

    int k;

    int numscannedlines = 0;

    while (line >= 0 && line < doc->lines()) {

      ++numscannedlines;
      if (numscannedlines > MAX_NUM_LINES_TO_SCAN) {
        break;
      }

      attlist = hiface->lineAttributes(line);

      if (kpos == -1) {
        kpos = doc->lineLength(line);
      }

      // find the attribute matching right after or and right before, this pattern match.
      int a = -1;

      int nextboundary = -1;
      if (direction == RightOf) {
        nextboundary = mboundary.indexIn(doc->line(line), (kpos >= 0) ? kpos+1 : 0);
      } else {
        if (kpos >= 0) {
          nextboundary = my_last_index_in(mboundary, doc->line(line).left(kpos));
        } else {
          nextboundary = my_last_index_in(mboundary, doc->line(line));
        }
        if (nextboundary != -1) {
          // point to end of match, to make sure the attribute doesn't overlap
          nextboundary += mboundary.matchedLength();
        }
      }

      for (k = 0; k < attlist.size(); ++k) {
        // ignore comments
        if (att_is_comment(attlist[k])) {
          continue;
        }
        if (direction == LeftOf &&  attlist[k].start < kpos) {
          // this attribute applies to a character *before* the match position.  Remember
          // the rightmost such attribute, which will be the closest attribute just before
          // the pattern match.
          if ( a == -1 ||
               ( (attlist[k].start + attlist[k].length) > (attlist[a].start + attlist[a].length) ) ) {
            // keep this attribute, only if there is no math boundary in
            // between. (Criterion: this attribute applies to at least one character
            // before the boundary
            if (nextboundary == -1 || attlist[k].start + attlist[k].length > nextboundary) {
              a = k;
            }
          }
        }
        if (direction == RightOf &&  attlist[k].start >= kpos) {
          // this attribute applies to a character *after* the match position.  Remember
          // the leftmost such attribute, which will be the closest attribute just after
          // the pattern match.
          if ( a == -1 ||
               ( attlist[k].start < attlist[a].start ) ) {
            // keep this attribute, only if there is no math boundary in between.
            if (nextboundary == -1 || attlist[k].start < nextboundary) {
              a = k;
            }
          }
        }
      }

      if (a != -1) {
        // found a match, return it
        return HiAttElem(attlist[a], line);
      }

      if (nextboundary != -1) {
        // if we didn't find an attribute, but we have a boundary in our way, it's like
        // normal text.
        return HiAttElem(line);
      }

      // otherwise, scan next/previous line...
      if (direction == LeftOf) {
        --line;
        kpos = -1;
      } else {
        ++line;
        kpos = 0;
      }
    }

    // didn't find next/previous attribute...
    return HiAttElem();
  }




};






// -----------------------------


KLFKteParser * createDummyParser(Doc * doc)
{
  return new KLFKteParser_Dummy(doc);
}

KLFKteParser * createKatePartParser(Doc * doc, KTextEditor::HighlightInterface * hiface)
{
  return new KLFKteParser_KatePart(doc, hiface);
}







#endif
