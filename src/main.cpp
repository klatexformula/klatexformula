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

#include <qfile.h>
#include <qclipboard.h>
#include <qdragobject.h>

#include <kapplication.h>
#include <kaboutdata.h>
#include <kcmdlineargs.h>
#include <klocale.h>

#include "klfmainwin.h"

extern const char version[];
extern const int version_maj, version_min, version_release;
extern KAboutData *klfaboutdata;

static const char description[] =
    I18N_NOOP("KLatexFormula -- Easily get an image from a LaTeX equation");

// not static so we can get this value from other modules in the project
const char version[] = "2.0.2";
const int version_maj = 2;
const int version_min = 0;
const int version_release = 2;

KAboutData *klfaboutdata = 0;

static KCmdLineOptions options[] =
{
  KCmdLineLastOption
};

int main(int argc, char **argv)
{
  KAboutData about("klatexformula", I18N_NOOP("KLatexFormula"), version, description,
		   KAboutData::License_GPL, "(C) 2005-2007 Philippe Faist", 0, 0, "philippe.faist@bluewin.ch");
  about.addAuthor( "Philippe Faist", 0, "philippe.faist@bluewin.ch" );
  KCmdLineArgs::init(argc, argv, &about);
  KCmdLineArgs::addCmdLineOptions( options );

  klfaboutdata = &about;

  KApplication app;

  fprintf(stderr, "KLatexFormula Version %s by Philippe Faist (c) 2005-2007\n"
	  "Licensed under the terms of the GNU Public License GPL\n\n",
	  version);
  
  if (app.isRestored()) {
    RESTORE(KLFMainWin);
  }
  else {
    // no session.. just start up normally

    KLFMainWin *mainWin = 0;    
    mainWin = new KLFMainWin();
    app.setMainWidget(mainWin);
    mainWin->show();

    KCmdLineArgs *args = KCmdLineArgs::parsedArgs();
    /* nothing to do with options */
    args->clear();

  }
  
  // mainWin has WDestructiveClose flag by default, so it will delete itself.
  return app.exec();
}

