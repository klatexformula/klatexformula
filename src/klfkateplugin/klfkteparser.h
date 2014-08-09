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


/** Information about a LaTeX Math Mode section ("context").
 *
 * This information contains:
 *
 *  - start/end position in document of the math mode
 *
 *  - start/end LaTeX command that starts/end the LaTeX math mode (e.g. \c "$" and \c "$",
 *    or \c "\begin{equation}" and \c "\end{equation}" )
 *
 *  - the LaTeX contents of the math mode section
 */
struct MathModeContext
{
  MathModeContext();

  inline bool isValid() const { return start.isValid() && end.isValid(); }

  Cur start;
  Cur end;

  //! Math mode latex start command
  QString startcmd;
  //! Math mode latex end command
  QString endcmd;

  /** Returns all LaTeX code comprised between the start and end of this math mode.
   *
   * This does not include the start/end commands.
   */
  QString latexmath;

  /** Returns a "math mode" argument suitable for KLFBackend::getLatexFormula(), i.e. of
   *  the form \c "\[ ... \]". Some environments such as \c "\begin{equation}" are
   *  detected and replaced with their unnumbered equivalent, e.g. \c "\begin{equation*}".
   */
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
  KLFKteParser(Doc *doc);
  virtual ~KLFKteParser();

  virtual MathModeContext parseContext(const Cur& cur) = 0;

protected:
  Doc *pDoc;
};



KLFKteParser * createDummyParser(Doc * doc);
KLFKteParser * createKatePartParser(Doc * doc, KTextEditor::HighlightInterface * hiface);




#endif
