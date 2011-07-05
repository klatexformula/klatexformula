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



QString MathModeContext::fullMathModeWithoutNumbering() const
{
  QRegExp rx = QRegExp("(\\\\begin|end)\\{(equation|eqnarray|align)\\}");
  const QString replacement = "\\1{\\2*}";

  QString bcmd = startcmd;
  QString ecmd = endcmd;

  bcmd.replace(rx, replacement);
  ecmd.replace(rx, replacement);

  return bcmd + " ... " + ecmd;
}
