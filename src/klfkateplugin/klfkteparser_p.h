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

#include <ktexteditor/document.h>
#include <ktexteditor/highlightinterface.h>
#include <ktexteditor/attribute.h>


// handy shorthand

inline QString my_str_replaced(const QString& s, const QRegExp& rx, const QString& replacement)
{ QString copy = s; copy.replace(rx, replacement); return copy; }



// some little shorthands ;-)

typedef KTextEditor::Document Doc;
typedef KTextEditor::Cursor Cur;


static Cur step_cur(KTextEditor::Document * doc, const Cur& c, bool forward)
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


struct MathModeContext
{
  MathModeContext()
    : start(Cur::invalid()), end(Cur::invalid()), startcmd(), endcmd(), latexmath()
  {  }

  inline bool isValid() const { return start.isValid() && end.isValid(); }

  Cur start;
  Cur end;

  QString startcmd;
  QString endcmd;

  QString latexmath;
};

static QDebug operator<<(QDebug dbg, const MathModeContext& m)
{
  if (m.start.line() == m.end.line())
    dbg.nospace() << "MathModeContext{l."<<m.start.line()<<" "<<m.start.column()<<"--"<<m.end.column();
  else
    dbg.nospace() << "MathModeContext{l."<<m.start.line()<<":"<<m.start.column()<<"--l."<<m.end.line()
		  << ":"<<m.end.column();
  return dbg << " "<<m.startcmd<<"--"<<m.endcmd<<" math="<<m.latexmath;
}

class KLFKteParser
{
public:
  KLFKteParser(Doc *doc) : pDoc(doc)
  {
    KLF_ASSERT_NOT_NULL(doc, "doc is NULL!", ; ) ;
  }
  virtual ~KLFKteParser() { }

  virtual MathModeContext parseContext(const Cur& cur) = 0;

protected:
  Doc *pDoc;
};



// -----------




class KLFKteParser_Dummy : public KLFKteParser
{
public:
  KLFKteParser_Dummy(Doc *doc) : KLFKteParser(doc) { }

  MathModeContext parseContext(const Cur& /*cur*/)
  {
    return MathModeContext();
  }
};



// -----------



#define KatePartAttrNameProperty KTextEditor::Attribute::AttributeInternalProperty

typedef KTextEditor::Attribute HiAtt;
typedef KTextEditor::HighlightInterface::AttributeBlock HiAttBlock;
typedef QList<KTextEditor::HighlightInterface::AttributeBlock> HiAttList;


#define MATHENV  "(?:"  "equation|displaymath|eqnarray|subeqnarray|math|multline|gather|align|flalign"  ")"



static bool att_is_math(const HiAttBlock& att)
{
  const KTextEditor::Attribute * a = att.attribute.data();
  KLF_ASSERT_NOT_NULL(a, "a is NULL!", return false; ) ;
  if (!a->hasProperty(KatePartAttrNameProperty)) {
    qWarning()<<KLF_FUNC_NAME
	      <<": attribute does not have property KTextEditor::Attribute::AttributeInternalProperty="
	      <<KTextEditor::Attribute::AttributeInternalProperty;
    return false;
  }
  return a->property(KatePartAttrNameProperty).toString().contains("math", Qt::CaseInsensitive);
}


struct HiAttElem : public HiAttBlock
{
  HiAttElem() : HiAttBlock(-1, -1, KTextEditor::Attribute::Ptr(NULL)), line(-1), cache_ismath(-1) { }
  HiAttElem(const HiAttBlock& h, int l = -1) : HiAttBlock(h), line(l), cache_ismath(-1) { }
  
  int line;
  
  inline Cur startCur() const { return Cur(line, start); }
  inline Cur endCur() const { return Cur(line, start+length); }
  
  inline bool isMath() const
  { return (cache_ismath!=-1) ? cache_ismath : (cache_ismath=att_is_math(*this)); }
  
private:
  mutable int cache_ismath;
};

struct MathModeBoundary
{
  MathModeBoundary()
    : cur(Cur::invalid()), istrue(false), isstart(true), boundarycommand() { }
  MathModeBoundary(Cur c, bool t, bool st)
    : cur(c), istrue(t), isstart(st), boundarycommand() { }
  
  Cur cur;
  bool istrue;
  bool isstart;
  
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
  return dbg << "{l."<<a.line<<" "<<a.start<<"+"<<a.length
	     <<" ("<<a.attribute->property(KatePartAttrNameProperty)<<")}" ;
}



class KLFKteParser_KatePart : public KLFKteParser
{
  KTextEditor::HighlightInterface * pHiface;
public:

  KLFKteParser_KatePart(Doc *doc, KTextEditor::HighlightInterface * hiface)
    : KLFKteParser(doc)
  {
    pHiface = hiface;
    KLF_ASSERT_NOT_NULL(pHiface, "highlight-interface is NULL!", ; ) ;
  }

public:

  MathModeContext parseContext(const Cur& cur)
  {
    KLF_DEBUG_BLOCK(KLF_FUNC_NAME) ;

    // parse mathmode/non-mathmode transitions

    MathModeBoundary headm, tailm;

    headm = find_mathmode_transition(cur, false /*backwards*/);
    if (!headm.cur.isValid()) {
      klfDbg("failed to parse head mathmode boundary: invalid headm returned.") ;
      return MathModeContext();
    }

    if (!headm.isstart) {
      klfDbg("we are not in math mode.");
      return MathModeContext();
    }
    
    tailm = find_mathmode_transition(cur, true /*forward*/);
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

  MathModeBoundary boundary_info(const HiAttElem& attmin, const HiAttElem& attmax)
  {
    KLF_DEBUG_BLOCK(KLF_FUNC_NAME) ;
    klfDbg("attmin="<<attmin<<"; attmax="<<attmax) ;
    
    QRegExp startmath = QRegExp("(\\$|\\$\\$|\\\\\\(|\\\\\\[|\\\\begin\\{(" MATHENV ")\\*?\\})\\s*");
    QRegExp endmath   = QRegExp("\\s*(\\$|\\$\\$|\\\\\\)|\\\\\\]|\\\\end\\{(" MATHENV ")\\*?\\})");
    
    klfDbg("rx patterns:\n\tstartmath: "<<qPrintable(startmath.pattern())<<"\n\tendmath: "
	   <<qPrintable(endmath.pattern())) ;
    
    MathModeBoundary m;
    
    bool min_is_math = att_is_math(attmin);
    bool max_is_math = att_is_math(attmax);
    
    KLF_ASSERT_CONDITION( (int)min_is_math ^ (int)max_is_math,
			  "Assert Failed: min_is_math="<<min_is_math<<" XOR max_is_math="<<max_is_math<<" !",
			  return MathModeBoundary(); ) ;
    
    QString contextlines;
    contextlines = pDoc->line(attmin.endCur().line());
    int maxlineoffset = 0;
    if (attmax.startCur().line() != attmin.endCur().line()) {
      maxlineoffset = contextlines.length() + 1; // +1 for \n
      contextlines += "\n"+pDoc->line(attmax.startCur().line());
    }
    
    klfDbg("contextlines=\n\t> "<<qPrintable(my_str_replaced(contextlines, QRegExp("\n"), "\n\t> "))<<"\n") ;
    
    if (!min_is_math && max_is_math) {
      // need to match for  "start-math-mode"
      Cur c = attmax.startCur();
      int j = startmath.lastIndexIn(contextlines, maxlineoffset + c.column());
      klfDbg("matching startmath rx reverse from "<<maxlineoffset+c.column()<<". result="<<j
	     <<", matchedlen="<<startmath.matchedLength()) ;
      // startmath match intersects with [attmin.endCur() , attmax.startCur()]
      if (j != -1 && j <= maxlineoffset+attmax.startCur().column()
	  && j+startmath.matchedLength() >= attmin.endCur().column()) {
	// "True" boundary
	// position: just after start-math-marker: convert j+matchedlen to cursor pos
	m.cur = Cur(attmin.line, j+startmath.matchedLength());
	if (m.cur.column() >= maxlineoffset)
	  m.cur = Cur(attmax.line, m.cur.column() - maxlineoffset);
	m.istrue = true;
	m.isstart = true;
	m.boundarycommand = startmath.cap(1);
	return m;
      }
      m.cur = attmax.startCur();
      m.istrue = false;
      m.isstart = true;
      return m;
    }
    // else: (att_is_math(attlist[kmin]) && !att_is_math(attlist[kmax]))
    Cur c = attmax.startCur();
    int ji = endmath.lastIndexIn(contextlines, maxlineoffset + c.column());
    klfDbg("matching endmath rx reverse from "<<maxlineoffset + c.column()<<". "
	   "result="<<ji<<", matchedlen="<<endmath.matchedLength()) ;
    // endmath match intersects with [attmin.endCur() , attmax.startCur()]
    if (ji != -1 && ji <= maxlineoffset+attmax.startCur().column()
	&& ji+endmath.matchedLength() >= attmin.endCur().column()) {
      // "true" boundary
      // position: just before end-math-marker: convert ji to cursor pos
      m.cur = Cur(attmin.line, ji);
      if (m.cur.column() >= maxlineoffset)
	m.cur = Cur(attmax.line, m.cur.column() - maxlineoffset);
      m.istrue = true;
      m.isstart = false;
      m.boundarycommand = endmath.cap(1);
      return m;
    }
    m.cur = attmin.endCur();
    m.istrue = false;
    m.isstart = false;
    return m;
  }


  MathModeBoundary
  /* */ find_mathmode_transition(Cur pos, bool forward, bool wantTrueBoundary = true)
  {
    KLF_DEBUG_TIME_BLOCK(KLF_FUNC_NAME) ;
    klfDbg("position=["<<pos<<"], forward="<<forward<<"; wantTrueBoundary="<<wantTrueBoundary) ;

    int line = pos.line();
    int col = pos.column();

    int step = forward ? +1 : -1;

    int k;

    HiAttElem prevAtt;
    prevAtt.line = -1; // invalid

    do {
      klfDbg("line iteration: line="<<line<<":\n\t> "<<qPrintable(pDoc->line(line))) ;
      
      HiAttList attlist = pHiface->lineAttributes(line);
      
      klfDbg("attributes: "<<attlist) ;
      
      if (!attlist.size()) {
	line += step;
	continue;
      }
      
      ensure_attr_fill_order_start(line, &attlist);
      
      if (col >= 0) {
	// find the next/prev. attribute corresponding to this position
	if (forward) {
	  for (k = attlist.size()-1; k >= 0 && attlist[k].start >= col; --k)
	    ;
	} else {
	  for (k = 0; k < attlist.size() && attlist[k].start+attlist[k].length < col; ++k)
	    ;
	}
	// initialize prevAtt
	prevAtt = attlist[k];
	prevAtt.line = line;
      } else {
	// initialize k
	if (forward)
	  k = 0;
	else
	  k = attlist.size() - 1;
      }
      klfDbg("starting to step attribute index iterations. k="<<k<<"; prevAtt/is-math="<<prevAtt.isMath()) ;
      // now, step k to previous/next attributes until we reach a non-math mode element
      while (k >= 0 && k < attlist.size()) {
	HiAttElem att(attlist[k], line);
	klfDbg("stepping k="<<k<<"; att="<<att) ;
	if (att.isMath() != prevAtt.isMath()) {
	  klfDbg("mathmode boundary canditate at k="<<k) ;
	  // is this a true math mode boundary? (not eg. a \text{...} command)
	  HiAttElem attmin = forward ? prevAtt : att;
	  HiAttElem attmax = forward ? att : prevAtt;
	  MathModeBoundary m = boundary_info(attmin, attmax);
	  if (!wantTrueBoundary || m.istrue) {
	    klfDbg("got true boundary: "<<m) ;
	    return m;
	  }
	  // at this point, we have wantTrueBoundary && !istrue
	  klfDbg("not true boundary but wantTrueBoundary=true. boundary-info="<<m) ;
	  // continue stepping until we re-reach a boundary. Use recursion.
	  MathModeBoundary m2;
	  m2 = find_mathmode_transition(step_cur(pDoc, m.cur, forward), forward,
					false); // don't want necessarily true boundary (!)
	  if (!m2.cur.isValid()) {
	    klfDbg("failed to find prev. boundary. returning fail.");
	    return m2;
	  }
	  klfDbg("got boundary at m2="<<m2) ;
	  // if we found a true boundary, this means that we are in a "non-true" region, ie. there is also
	  // a non-true boundary AFTER our search start point. which means we found the final mathmode boundary
	  // we are looking for.
	  if (m2.istrue) {
	    klfDbg("returning the found true boundary m2.") ;
	    return m2;
	  }
	  // at which point we can continue our stepping search. Re-use recursion because
	  // we have a column position rather than an attlist-index.
	  Cur cc = step_cur(pDoc, m2.cur, forward);
	  MathModeBoundary mfinal = find_mathmode_transition(cc, forward, wantTrueBoundary);
	  klfDbg("got final boundary: mfinal="<<mfinal) ;
	  return mfinal;
	}
	k += step;
	prevAtt = att;
      }

      line += step;
      col = -1;
    } while (line >= 0 && line < pDoc->lines());

    klfDbg("failed to find mathmode boundary..") ;

    // return invalid mathmodeboundary object
    return MathModeBoundary();
  }

  HiAttBlock make_normal_text_att(int start, int length)
  {
    HiAtt *a = new HiAtt;
    a->setProperty(KatePartAttrNameProperty, QString("LaTeX:Normal Text"));
    return HiAttBlock(start, length, HiAtt::Ptr(a));
  }

  void ensure_attr_fill_order_start(int line, HiAttList * attlist)
  {
    int llen = pDoc->lineLength(line);
    if (attlist->isEmpty()) {
      if (llen == 0)
	return;
      attlist->insert(0, make_normal_text_att(0, llen));
      return;
    }

    if (attlist->at(0).start != 0)
      attlist->insert(0, make_normal_text_att(0, attlist->at(0).start));

    klfDbg("starting iteration; attlist is "<<*attlist) ;

    for (int k = 1; k < attlist->size(); ++k) {
      //    klfDbg("k="<<k) ;
      KLF_ASSERT_CONDITION(attlist->at(k).start >= attlist->at(k-1).start,
			   "att list is not sorted!!", return; ) ;
      int prevend = attlist->at(k-1).start + attlist->at(k-1).length;
      if (attlist->at(k).start > prevend) {
	// insert normal text attribute
	attlist->insert(k, make_normal_text_att(prevend, attlist->at(k).start - prevend));
      }
    }
    int n = attlist->size()-1;
    int lastpos = attlist->at(n).start + attlist->at(n).length;
    if (lastpos < llen)
      attlist->insert(n+1, make_normal_text_att(lastpos, llen-lastpos));
    
    klfDbg("final attlist is now "<<*attlist) ;
  }
  
};








#endif
