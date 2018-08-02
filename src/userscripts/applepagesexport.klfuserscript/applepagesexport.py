#!/usr/bin/env python

# applepagesexport.py
#   This file is part of the KLatexFormula Project.
#   Copyright (C) 2018 by Philippe Faist
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

from __future__ import print_function

import re
import os
import sys
import os.path
import argparse
import base64

try:
    from urllib.parse import quote_plus # Python 3
except ImportError:
    from urllib import quote_plus # Python 2
try:
    from html import escape as htmlescape  # python 3.x
except ImportError:
    from cgi import escape as htmlescape  # python 2.x


#sys.path.insert(0, os.path.join(os.path.dirname(__file__), '..')) -- no longer needed
import pyklfuserscript

args = pyklfuserscript.export_type_args_parser().parse_args()

format = args.format
pdffile = args.inputfile

if format != 'applepages-export':
    print("Invalid format: ", format)
    sys.exit(1)


HTML_TEMPLATE = """\
<img src=\"data:application/pdf;base64,{b64data}\" alt=\"{alttxt}\" title=\"{alttxt}\">
"""

pdfcontents = None
with open(pdffile) as f:
    pdfcontents = f.read()

latexcode = os.environ.get('KLF_INPUT_LATEX', '')
# some substitutions in the latex string to make it more readable [duplicates
# C++ code from klfmime.cpp arrgh!!]

#    \! \, \; \:  -> simple space
latexcode = re.sub(r"\\[,;:!]", " ", latexcode)
#    \text{Hello}, \mathrm{Hilbert-Einstein}  -->  {the text}
latexcode = re.sub(r"\\(?:text|mathrm)\{((?:\w|\s|[._-])*)\}", r"{\1}", latexcode)
#    \var(epsilon|phi|...)    ->   \epsilon,\phi,...
latexcode = re.sub(r"\\var([a-zA-Z]+)", r"\\\1", latexcode)


print(HTML_TEMPLATE.format(
    b64data=quote_plus(base64.b64encode(pdfcontents)),
    alttxt=htmlescape(latexcode)
    ))
