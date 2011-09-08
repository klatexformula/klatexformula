#!/usr/bin/python

# klffeynmf.py
#   This file is part of the KLatexFormula Project.
#   Copyright (C) 2011 by Philippe Faist
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

import re;
import os;
import sys;

if (sys.argv[1] == "--help"):
    print "Usage: "+os.path.basename(sys.argv[0])+" --scriptinfo";
    print "       "+os.path.basename(sys.argv[0])+" <tex input file>";
    print "";
    exit(0);

if (sys.argv[1] == "--scriptinfo"):
    print "ScriptInfo";
    print "Name: FeynMF Wrapper";
    print "Author: Philippe Faist <philippe.fai"+"st@b"+"luewin.ch>"
    print "Version: 0.2";
    print "License: GPL v2+"
    print "SpitsOut: dvi"
    # ###TODO: implement in klfbackend  Force*: ...
    #print "ForceInputMathMode: \\begin{fmffile}{FMF_FEYNUSERSCRIPT_MFNAME} ... \\end{fmffile}";
    #print "ForceInputPreambleLine: \\usepackage{feynmf}";
    print "";
    exit(0);


latexfname = sys.argv[1];

latexinput = os.environ["KLF_INPUT_LATEX"];

latexexe = os.environ["KLF_LATEX"];
mfexe = os.path.dirname(latexexe) + "/mf"; # guess 'mf' exec location as same as latex exec location

if (len(latexexe) == 0):
    sys.stderr.write("Error: latex executable not found.");
    exit(1);
if (len(mfexe) == 0):
    sys.stderr.write("Error: mf executable not found.");
    exit(1);

# prepare LaTeX template with \begin{fmfile}
# overwrite default-prepared template

# use the klfbackend basename so that it cleans up temp files at the end for us
mfbasefname = os.path.basename(os.environ["KLF_TEMPFNAME"]) + "mf";
print "mfbasename="+mfbasefname;

f = open(latexfname, 'w');
latexcontents = ("\\documentclass{article}\n\\usepackage{amsmath}\n\\usepackage{feynmf}\n"+
                 os.environ["KLF_INPUT_PREAMBLE"]+
                 "\\begin{document}\n" +
                 "\\thispagestyle{empty}\n" +
                 "\\begin{fmffile}{" + mfbasefname + "}\n"
                 + latexinput + "\n\\end{fmffile}\n\\end{document}\n");
print latexcontents;
f.write(latexcontents);
f.close();

# now, run latex first time
print "Running "+latexexe+" "+latexfname+ "  ...";
res = os.system(latexexe + ' ' + latexfname);
#if (res != 0):
#    print "latex failed, res="+str(res);
#    exit(res>>8);
print "Running "+mfexe + " '\\mode=localfont; input "+mfbasefname+".mf;'" + "  ...";
res = os.system(mfexe + " '\\mode=localfont; input "+mfbasefname+".mf;'");
if (res != 0):
    print "mf failed, res="+str(res);
    exit(res>>8);
print "Re-Running "+latexexe+" "+latexfname+ "  ...";
os.system(latexexe + ' ' + latexfname);
if (res != 0):
    print "re-latex failed, res="+str(res);
    exit(res>>8);

# remove temp file -- NO it is needed in the further steps !!
# Temp files are removed as long as their basename corresponds to the KLFBackend-generated base name
#print "Running rm "+mfbasefname+".mf";
#os.system("rm "+mfbasefname+".mf");


exit(0);


