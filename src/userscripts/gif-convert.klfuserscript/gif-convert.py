#!/usr/bin/env python

# gif-convert.py
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

from __future__ import print_function

import re
import os
import sys
import os.path
import argparse
import subprocess

parser = argparse.ArgumentParser()
parser.add_argument("--format", help='output format to use (only valid value is "gif")')
parser.add_argument("pngfile", help='the PNG input file')

args = parser.parse_args()

format = args.format
pngfile = args.pngfile

if format != 'gif':
    print("Invalid format: ", format)
    sys.exit(1);

convert = ""
if "KLF_USCONFIG_convert" in os.environ:
    convert = os.environ["KLF_USCONFIG_convert"];
if not convert:
    if os.path.exists("/usr/bin/convert"):
        convert = "/usr/bin/convert"
    elif os.path.exists("/usr/local/bin/convert"):
        convert = "/usr/local/bin/convert"


giffile = re.sub(r'\.png$', r'.gif', pngfile);

# CalledProcessError is raised if an error occurs.
output = subprocess.check_output(args=[convert, pngfile, giffile], shell=False)

print("Output from {}: \n{}".format(convert, output.decode('utf-8')))

exit(0);


