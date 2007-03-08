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

#include <klfbackend.h>

static const char description[] =
    I18N_NOOP("KLatexFormula -- Easily get an image from a LaTeX equation");

static const char version[] = "2.0";

static KCmdLineOptions options[] =
{
//    { "+[URL]", I18N_NOOP( "Document to open" ), 0 },
    KCmdLineLastOption
};

int main(int argc, char **argv)
{
  KAboutData about("klatexformula", I18N_NOOP("KLatexFormula"), version, description,
		   KAboutData::License_GPL, "(C) 2005 Philippe Faist", 0, 0, "philippe.faist@bluewin.ch");
  about.addAuthor( "Philippe Faist", 0, "philippe.faist@bluewin.ch" );
  KCmdLineArgs::init(argc, argv, &about);
  KCmdLineArgs::addCmdLineOptions( options );
  KApplication app;
  //    KLatexFormula *mainWin = 0;
  
  if (app.isRestored()) {
    //        RESTORE(KLatexFormula);
  }
  else {
    // no session.. just start up normally
    KCmdLineArgs *args = KCmdLineArgs::parsedArgs();
    
    /** @todo do something with the command line args here */
    
    //        mainWin = new KLatexFormula();
    //        app.setMainWidget( mainWin );
    //        mainWin->show();

    KLFBackend::klfInput in;
    KLFBackend::klfSettings settings;
    settings.tempdir = "/tmp";
    settings.klfclspath = "/home/philippe/junk";
    settings.latexexec = "latex";
    settings.dvipsexec = "dvips";
    settings.gsexec = "gs";
    settings.epstopdfexec = "epstopdf";

    in.latex = "a + b = c \\Rightarrow a = c - b";
    in.mathmode = "<<~$ ... $~>>";
    in.preamble = "\\usepackage[T1]{fontenc}\n";
    in.fg_color = qRgb(255, 255, 255); // white
    in.bg_color = qRgba(0, 0, 80, 255); // dark blue
    in.dpi = 300;

    KLFBackend::klfOutput out = KLFBackend::getLatexFormula(in, settings);

    if (out.status) {
      printf("ERROR: %d: %s\n", out.status, out.errorstr.ascii());
      return 0;
    }
    out.result.save("/home/philippe/junk/klf_success.png", "PNG");

    {
      QFile f("/home/philippe/junk/klf_success.eps");
      f.open(IO_WriteOnly);
      f.writeBlock(out.epsdata);
    }
    {
      QFile f("/home/philippe/junk/klf_success.pdf");
      f.open(IO_WriteOnly);
      f.writeBlock(out.pdfdata);
    }
    args->clear();
  }
  
  return 0; // more radical
  
  // mainWin has WDestructiveClose flag by default, so it will delete itself.
  return app.exec();
}

