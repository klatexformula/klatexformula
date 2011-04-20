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


// some little shorthands ;-)

typedef KTextEditor::Document Doc;
typedef KTextEditor::Cursor Cur;
typedef KTextEditor::Attribute HiAtt;
typedef KTextEditor::HighlightInterface::AttributeBlock HiAttBlock;
typedef QList<KTextEditor::HighlightInterface::AttributeBlock> HiAttList;



struct MathModeContext
{
  MathModeContext()
    : start(Cur::invalid()), end(Cur::invalid()), startcmd(), endcmd(), latexmath()
  {  }

  Cur start;
  Cur end;

  QString startcmd;
  QString endcmd;

  QString latexmath;
};

class KLFKteParser
{
  Doc *pDoc;
public:
  KLFKteParser(Doc *doc) : pDoc(doc) { }
  virtual ~KLFKteParser() { }

  virtual MathModeContext parseContext(const Cur& cur) = 0;
};

// ----

class KLFKteParser_Dummy : public KLFKteParser
{
public:
  KLFKteParser_Dummy(Doc *doc) : KLFKteParser(doc) { }

  MathModeContext parseContext(const Cur& cur)
  {
    return MathModeContext();
  }
};

// ----


#define KatePartAttrNameProperty KTextEditor::Attribute::AttributeInternalProperty


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

  MathModeContext parseContext(const Cur& cur)
  {
    // for now....
    return MathModeContext();
  }

};








#endif
