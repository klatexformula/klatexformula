#!/usr/bin/python

# gif-convert.py
#   This file is part of the KLatexFormula Project.
#   Copyright (C) 2012 by Philippe Faist
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
    print "       "+os.path.basename(sys.argv[0])+" <file.png>";
    print "";
    exit(0);

if (sys.argv[1] == "--scriptinfo"):
    print "ScriptInfo";
    print "Category: klf-export-type";
    print "Name: GIF output format provider using convert utility";
    print "Author: Philippe Faist <philippe.fai"+"st@b"+"luewin.ch>"
    print "Version: 0.1";
    print "License: GPL v2+"
    print "InputDataType: PNG"
    print "MimeType: image/gif"
    print "OutputFilenameExtension: gif"
    print "OutputFormatDescription: GIF Image"
    print "WantStdinInput: false"
    print "HasStdoutOutput: false"
    print "";
    exit(0);


pngfile = sys.argv[1];
giffile = re.sub(r'\.png$', r'.gif', pngfile);

pngfile = "'" + re.sub(r"\'", "'\"'\"'", pngfile) + "'";
giffile = "'" + re.sub(r"\'", "'\"'\"'", giffile) + "'";

# TODO !!!: Needs Error handling/messages/....
os.system("convert "+pngfile+" "+giffile+"");


exit(0);


