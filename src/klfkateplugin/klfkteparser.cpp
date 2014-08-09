/***************************************************************************
 *   file klfkteparser.cpp
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

#include <klfkteparser.h>

#include <klfkteparser_p.h>


// --------------------------------------------------------------------------------

MathModeContext::MathModeContext()
  : start(Cur::invalid()), end(Cur::invalid()), startcmd(), endcmd(), latexmath()
{
}


QString MathModeContext::fullMathModeWithoutNumbering() const
{
  QRegExp rx = QRegExp("(\\\\begin|end)\\{(equation|eqnarray|align|gather|multline)\\}");
  const QString replacement = "\\1{\\2*}";

  QString bcmd = startcmd;
  QString ecmd = endcmd;

  bcmd.replace(rx, replacement);
  ecmd.replace(rx, replacement);

  return bcmd + " ... " + ecmd;
}



QDebug operator<<(QDebug dbg, const MathModeContext& m)
{
  if (m.start.line() == m.end.line())
    dbg.nospace() << "MathModeContext{l."<<m.start.line()<<" "<<m.start.column()<<"--"<<m.end.column();
  else
    dbg.nospace() << "MathModeContext{l."<<m.start.line()<<":"<<m.start.column()<<"--l."<<m.end.line()
		  << ":"<<m.end.column();
  return dbg << " "<<m.startcmd<<"--"<<m.endcmd<<" math="<<m.latexmath;
}



// --------------------------------------------------------------------------------

KLFKteParser::KLFKteParser(Doc *doc) : pDoc(doc)
{
  KLF_ASSERT_NOT_NULL(doc, "doc is NULL!", ; ) ;
}

KLFKteParser::~KLFKteParser()
{
}




