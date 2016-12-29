#!/usr/bin/python

# aicb-test-data-export.py
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
#   $Id$


import re
import os
import sys
import os.path
import base64
import cgi
import textwrap

if (sys.argv[1] == "--help"):
    print "Usage: "+os.path.basename(sys.argv[0])+" --scriptinfo [KLF-VERSION]";
    print "       "+os.path.basename(sys.argv[0])+" <file.png>";
    print "";
    exit(0);

if (sys.argv[1] == "--scriptinfo"):
    print "ScriptInfo";
    print "Category: klf-export-type";
    print "Name: AICB export for Adobe InDesign";
    print "Author: Philippe Faist <philippe.fai"+"st@b"+"luewin.ch>"
    print "Version: 0.1";
    print "License: GPL v2+"
    print "InputDataType: EPS"
    print "MimeType: application/x-aicb"
    print "OutputFilenameExtension: aicb"
    print "OutputFormatDescription: AICB Data"
    print "WantStdinInput: false"
    print "HasStdoutOutput: true"
    print "";
    exit(0);


assert len(sys.argv) >= 2, "Usage: aicb-data-export.py <file.eps>"

epsdata = None
with open(sys.argv[1], 'rb') as f:
    epsdata = f.read()

sys.stdout.write(epsdata);

sys.exit(0);


