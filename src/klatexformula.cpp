/***************************************************************************
 *   file $FILENAME$
 *   This file is part of the KLatexFormula Project.
 *   Copyright (C) 2007 by Philippe Faist
 *   philippe.faist@bluewin.ch
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


#include "klatexformula.h"

#include <qlabel.h>

#include <kmainwindow.h>
#include <klocale.h>

KLatexFormula::KLatexFormula()
    : KMainWindow( 0, "KLatexFormula" )
{
    // set the shell's ui resource file
    setXMLFile("klatexformulaui.rc");

    new QLabel( "Hello World", this, "hello label" );
}

KLatexFormula::~KLatexFormula()
{
}

#include "klatexformula.moc"
