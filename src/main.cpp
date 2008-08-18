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

#include "klfconfig.h"
#include "klfmainwin.h"

// copy these lines into other files to access these variables
extern const char version[];
extern int version_maj, version_min, version_release;
extern KAboutData *klfaboutdata;



// not static so we can get this value from other modules in the project
const char version[] = "2.1.0";
int version_maj = -1;
int version_min = -1;
int version_release = -1;

KAboutData *klfaboutdata = 0;


static const char description[] =
    I18N_NOOP("KLatexFormula -- Easily get an image from a LaTeX equation");



static KCmdLineOptions options[] =
{
  { "e", 0, 0 },
  { "equation <eqn>", I18N_NOOP("Equation to start with"), "" },
  { "x", 0, 0 },
  { "evaluate", I18N_NOOP("Evaluate given equation on startup"), 0 },
  { "s", 0, 0 },
  { "style <stylename>", I18N_NOOP("Load Previously Saved Named Style"), "default" },
  { "f", 0, 0 },
  { "fg-color <color>", I18N_NOOP("Use Custom Foreground Color (#FFFFFF web format, escape # from shell)"), 0 },
  { "b", 0, 0 },
  { "bg-color <color|->", I18N_NOOP("Use Custom Background Color (#FFFFFF web format, '-' for transparent, escape # from shell)"), 0 },
  { "X", 0, 0 },
  { "dpi <number>", I18N_NOOP("Use Custom DPI value"), 0 },
  { "m", 0, 0 },
  { "mathmode <arg>", I18N_NOOP( "Use given math mode. <arg> must contain '...' to be replaced by the latex string." ), 0 },
  { "p", 0, 0 },
  { "preamble <arg>", I18N_NOOP( "Some arbitrary LaTeX code to be inserted after \\documentclass{} and before \\begin{document}. "
    "This may typically be \\usepackage{xyz} directives." ), 0 },

  KCmdLineLastOption
};



// our Main Routine

int main(int argc, char **argv)
{
  // first thing : setup version_maj/min/release correctly
  sscanf(version, "%d.%d.%d", &version_maj, &version_min, &version_release);

  // then KDE Stuff

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

  // now load config
  klfconfig.loadDefaults(); // must be called before 'readFromConfig'
  klfconfig.readFromConfig(app.config());

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
    // parse options
    mainWin->mMainWidget->txeLatex->setText(args->getOption("equation"));
    QString sty = args->getOption("style");
    if (!sty.isEmpty()) {
      mainWin->loadNamedStyle(sty);
    }
    QString s;
    if ( ! (s = args->getOption("fg-color")).isEmpty() )
      mainWin->mMainWidget->kccFg->setColor(QColor(s));
    if ( ! (s = args->getOption("bg-color")).isEmpty() ) {
      mainWin->mMainWidget->chkBgTransparent->setChecked(s == "-");
      if (s != "-")
	mainWin->mMainWidget->kccBg->setColor(QColor(s));
      else
	mainWin->mMainWidget->kccBg->setColor(QColor(255, 255, 255));
    }
    if ( ! (s = args->getOption("dpi")).isEmpty() )
      mainWin->mMainWidget->spnDPI->setValue(s.toInt());
    if ( ! (s = args->getOption("mathmode")).isEmpty() )
      mainWin->mMainWidget->cbxMathMode->setCurrentText(s);
    if ( ! (s = args->getOption("preamble")).isEmpty() )
      mainWin->mMainWidget->txePreamble->setText(s);

    if (args->isSet("evaluate"))
      mainWin->slotEvaluate();

    args->clear();

  }

  // mainWin has WDestructiveClose flag by default, so it will delete itself.
  return app.exec();
}

