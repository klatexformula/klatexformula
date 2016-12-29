#!/usr/bin/env python

# klffeynmf.py
#   This file is part of the KLatexFormula Project.
#   Copyright (C) 2016 by Philippe Faist
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
#   $Id: klffeynmf.py 941 2015-06-07 19:38:11Z phfaist $

from __future__ import print_function

import re;
import os;
import sys;
import subprocess;

if (sys.argv[1] == "--help"):
    print("       "+os.path.basename(sys.argv[0])+" <tex input file>\n");
    exit(0);


mf_exe_name = 'mf';
mpost_exe_name = 'mpost';
if (sys.platform.startswith('win')):
    mf_exe_name = 'mf.exe';
    mpost_exe_name = 'mpost.exe';


# user input

latexfname = sys.argv[1];
latexinput = os.environ["KLF_INPUT_LATEX"];

rx_booltrue = re.compile(r'^\s*(t(rue)?|1|on|y(es)?)', flags=re.IGNORECASE)

usefeynmf = (rx_booltrue.match(os.environ['KLF_ARG_UseFeynMF']) is not None);
usefeynmp = (rx_booltrue.match(os.environ['KLF_ARG_UseFeynMP']) is not None);

print("usefeynmf=%r, usefeynmp=%r"%(usefeynmf,usefeynmp))

assert usefeynmf != usefeynmp, "Exactly one of usefeynmf or usefeynmp must be specified!"

# run latex

latexexe = os.environ["KLF_LATEX"];

if (len(latexexe) == 0):
    sys.stderr.write("Error: latex executable not found.\n");
    exit(1);


if usefeynmf:
    # guess 'mf' exec location as same as latex exec location
    mfexe = os.path.join(os.path.dirname(latexexe),mf_exe_name);
    if (len(mfexe) == 0):
        sys.stderr.write("Error: mf executable not found.\n");
        exit(1);

else:
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
latexcontents = ("\\documentclass{article}\n" +
                 "\\usepackage{amsmath}\n" +
                 ("\\usepackage{feynmf}\n" if usefeynmf else "\\usepackage{feynmp}\n") +
                 os.environ["KLF_INPUT_PREAMBLE"] +
                 "\\begin{document}\n" +
                 "\\thispagestyle{empty}\n" +
                 "\\begin{fmffile}{" + diagramname + "}\n"
                 + latexinput + "\n\\end{fmffile}\n\\end{document}\n");
print("LaTeX template is: \n" + latexcontents);
f.write(latexcontents);
f.close();



def run_cmd(do_cmd, ignore_fail=False):
    print("Running " + " ".join(["'%s'"%(x) for x in do_cmd]) + "  [ in %s ]..." %(tempdir) + "\n")
    res = subprocess.call(do_cmd, cwd=tempdir);
    if (not ignore_fail and res != 0):
        print("%s failed, res=%d" %(do_cmd[0], res));
        sys.exit(res>>8);


# now, run latex first time
# -------------------------

run_cmd([latexexe, os.path.basename(latexfname)], ignore_fail=True)


# now, run METAFONT or METAPOST
# -----------------------------

if usefeynmf:
    
    run_cmd([mfexe, r'\mode=localfont; input %s.mf;'%(diagramname)])
    
else:
    run_cmd([mpostexe, diagramname])


# now, run latex second time
# --------------------------

run_cmd([latexexe, os.path.basename(latexfname)])



sys.exit(0);


