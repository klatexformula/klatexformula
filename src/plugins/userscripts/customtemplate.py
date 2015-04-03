#!/usr/bin/env python

# customtemplate.py
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
#   $Id$

import re;
import os;
import sys;

if (sys.argv[1] == "--help"):
    print "Usage: "+os.path.basename(sys.argv[0])+" --scriptinfo [KLF-VERSION]";
    print "       "+os.path.basename(sys.argv[0])+" <tex input file>";
    print "";
    exit(0);

if (sys.argv[1] == "--scriptinfo"):
    print "ScriptInfo";
    print "Category: klf-backend-engine";
    print "Name: Custom Template";
    print "Author: Philippe Faist <philippe.fai"+"st@b"+"luewin.ch>"
    print "Version: 0.2";
    print "License: GPL v2+";
    print "SpitsOut: latex";
    print "DisableInputs: FG_COLOR BG_COLOR MATHMODE FONT FONTSIZE PREAMBLE";
    print "InputFormUI: :/userscriptdata/customtemplate/customtemplate_input.ui";
    #print "Error: Can't find executable foo. DEBUG: REMOVE THIS MESSAGE IN RELEASE VERSION"
    #print "Warning: random warning. DEBUG: REMOVE THIS MESSAGE IN RELEASE VERSION"
    #print "Notice: random notice. DEBUG: REMOVE THIS MESSAGE IN RELEASE VERSION"
    print "";
    exit(0);

latexfname = sys.argv[1];


latexinput = os.environ["KLF_INPUT_LATEX"];
template = os.environ["KLF_ARG_Template"];


if (not "%%INPUT" in template):
    print "Error: expected '%%INPUT' in template.";
    print "";
    print "You might want to try out the following template for example:";
    print """\\documentclass[12pt]{article}

\\usepackage{amsmath}
\\usepackage{amssymb}
\\usepackage{amsfonts}

\\begin{document}\\thispagestyle{empty}
\\[
%%INPUT
\\]
\\end{document}
"""
    print "\n";
    exit(1);

fulllatex = re.sub(r'%%INPUT\b', lambda m: latexinput, template);

f = open(latexfname, 'w');
print "Full LaTeX is :\n", fulllatex;
f.write(fulllatex);
f.close();

# latex ready.

exit(0);


