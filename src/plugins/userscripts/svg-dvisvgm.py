#!/usr/bin/env python

# svg-dvisvgm.py
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
    print "       "+os.path.basename(sys.argv[0])+" <file.dvi>";
    print "";
    exit(0);



#debug environment
#print repr(os.environ);

dvisvgm = ""
if "KLF_USCONFIG_dvisvgm" in os.environ:
    dvisvgm = os.environ["KLF_USCONFIG_dvisvgm"];

if not dvisvgm:
    dvisvgm = "/usr/bin/dvisvgm";

#print "dvisvgm: "+repr(dvisvgm);



if (sys.argv[1] == "--scriptinfo"):
    print "ScriptInfo";
    print "Category: klf-export-type";
    print "Name: SVG/dvisvgm format provider";
    print "Author: Philippe Faist <philippe.fai"+"st@b"+"luewin.ch>"
    print "Version: 0.2";
    print "License: GPL v2+";
    print "InputDataType: DVI";
    print "MimeType: image/svg+xml";
    print "OutputFilenameExtension: svg";
    print "OutputFormatDescription: SVG Vector Image (using dvisvgm)";
    print "WantStdinInput: false";
    print "HasStdoutOutput: false";
    print "SettingsFormUI: svg-dvisvgm_config.ui";
    if (not os.path.isfile(dvisvgm) or not os.access(dvisvgm, os.X_OK)):
        if (dvisvgm):
            print "Warning: Invalid dvisvgm path: %s" %(dvisvgm)
        print "Warning: Can't find dvisvgm executable.";
    print "";
    exit(0);


if (not os.path.isfile(dvisvgm) or not os.access(dvisvgm, os.X_OK)):
    print "Error: Can't find dvisvgm executable.";
    exit(255);
    
dvifile = sys.argv[1];

sys.stderr.write("Converting file "+dvifile+"\n");

dvifile = "'" + re.sub(r"\'", "'\"'\"'", dvifile) + "'";
# TODO !!!: Needs Error handling/messages/....
result = os.system("'"+dvisvgm+"' -a -e -n -b min '"+dvifile+"'");
if (result != 0):
    print "Error, result="+str(result)+"...";



exit(0);


