/***************************************************************************
 *   file main.cpp
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


#include <QApplication>

#include "klfconfig.h"
#include "klfmainwin.h"


// copy these lines into other files to access these variables
extern const char version[];
extern int version_maj, version_min, version_release;



// not static so we can get this value from other modules in the project
const char version[] = "3.0.0alpha0";
int version_maj = -1;
int version_min = -1;
int version_release = -1;



// our Main Function

int main(int argc, char **argv)
{
  // first thing : setup version_maj/min/release correctly
  sscanf(version, "%d.%d.%d", &version_maj, &version_min, &version_release);

  QApplication app(argc, argv);

  app.setFont(QFont("Nimbus Sans L", 10));

  fprintf(stderr, "KLatexFormula Version %s by Philippe Faist (c) 2005-2009\n"
	  "Licensed under the terms of the GNU Public License GPL\n\n",
	  version);

  // now load config
  klfconfig.loadDefaults(); // must be called before 'readFromConfig'
  klfconfig.readFromConfig();

  KLFColorChooser::setColorList(klfconfig.UI.userColorList);

  KLFMainWin mainWin;
  mainWin.show();

  return app.exec();
}

