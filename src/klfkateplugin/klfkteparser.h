/***************************************************************************
 *   file klfkteparser.h
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

#ifndef KLFKTEPARSER_H
#define KLFKTEPARSER_H


#include <ktexteditor/document.h>
#include <ktexteditor/highlightinterface.h>
#include <ktexteditor/attribute.h>

#include <klfdefs.h>


// some little shorthands ;-)

typedef KTextEditor::Document Doc;
typedef KTextEditor::Cursor Cur;



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

  QString fullMathModeWithoutNumbering() const;
};

inline bool operator==(const MathModeContext& a, const MathModeContext& b)
{
  return a.start == b.start && a.end == b.end && a.startcmd == b.startcmd &&
    a.endcmd == b.endcmd && a.latexmath == b.latexmath ;
}

QDebug operator<<(QDebug dbg, const MathModeContext& m);


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



extern KLFKteParser * createDummyParser(Doc * doc);
extern KLFKteParser * createKatePartParser(Doc * doc, KTextEditor::HighlightInterface * hiface);




#endif
