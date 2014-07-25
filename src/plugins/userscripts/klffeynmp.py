#!/usr/bin/python

# klffeynmp.py
#   This file is part of the KLatexFormula Project.
#   Copyright (C) 2014 by Philippe Faist
#   philippe.faist at bluewin.ch
#
#   This program is free software; you can redistribute it and/or modify
#   it under the terms of the GNU General Public License as published by
#   the Free Software Foundation; either version 2 of the License, or
#   (at your option) any later version.
#
#   This program is distributed in the hope that it will be useful,
#   but WITHOUT ANY WARRANTY; without even the implied warranty of
#   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#   GNU General Public License for more details.
#
#   You should have received a copy of the GNU General Public License
#   along with this program; if not, write to the
#   Free Software Foundation, Inc.,
#   59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
#
#   $Id$

import re;
import os;
import sys;
import subprocess;

if (sys.argv[1] == "--help"):
    print "Usage: "+os.path.basename(sys.argv[0])+" --scriptinfo [KLF-VERSION]";
    print "       "+os.path.basename(sys.argv[0])+" <tex input file>";
    print "";
    exit(0);

if (sys.argv[1] == "--scriptinfo"):
    print "ScriptInfo";
    print "Category: klf-backend-engine";
    print "Name: FeynMP Wrapper";
    print "Author: Philippe Faist <philippe.fai"+"st@b"+"luewin.ch>"
    print "Version: 1.0";
    print "License: GPL v2+"
    print "SpitsOut: dvi"
    print "DisableInputs: ALL_INPUT EXCEPT PREAMBLE"
    print "";
    exit(0);


mpost_exe_name = 'mpost';
if (sys.platform.startswith('win')):
    mpost_exe_name = 'mpost.exe';


latexfname = sys.argv[1];

latexinput = os.environ["KLF_INPUT_LATEX"];

latexexe = os.environ["KLF_LATEX"];

if (len(latexexe) == 0):
    sys.stderr.write("Error: latex executable not found.\n");
    exit(1);

# guess 'mpost' exec location as same as latex exec location
mpostexe = os.path.join(os.path.dirname(latexexe),mpost_exe_name);

if (len(mpostexe) == 0):
    sys.stderr.write("Error: mpost executable not found.\n");
    exit(1);

# prepare LaTeX template with \begin{fmfile}
# overwrite default-prepared template

tempdir = os.path.dirname(os.environ["KLF_TEMPFNAME"]);

diagramname = 'klffeynmpdiagram';

f = open(latexfname, 'w');
latexcontents = ("\\documentclass{article}\n\\usepackage{amsmath}\n\\usepackage{feynmp}\n"+
                 os.environ["KLF_INPUT_PREAMBLE"]+
                 "\\begin{document}\n" +
                 "\\thispagestyle{empty}\n" +
                 "\\begin{fmffile}{" + diagramname + "}\n"
                 + latexinput + "\n\\end{fmffile}\n\\end{document}\n");
print "LaTeX template is: \n", latexcontents;
f.write(latexcontents);
f.close();



def run_cmd(do_cmd, ignore_fail=False):
    print "Running " + " ".join(["'%s'"%(x) for x in do_cmd]) + "  [ in %s ]..." %(tempdir) + "\n"
    res = subprocess.call(do_cmd, cwd=tempdir);
    if (not ignore_fail and res != 0):
        print "%s failed, res=%d" %(do_cmd[0], res);
        sys.exit(res>>8);


# now, run latex first time
# -------------------------

run_cmd([latexexe, os.path.basename(latexfname)], ignore_fail=True)


# now, run METAPOST
# -----------------

run_cmd([mpostexe, diagramname])


# now, run latex second time
# --------------------------

run_cmd([latexexe, os.path.basename(latexfname)])



sys.exit(0);


