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

from __future__ import print_function

import re
import os
import sys
import argparse
import subprocess

parser = argparse.ArgumentParser()
parser.add_argument("--format", help='output format to use ("svg" or "svgz")')
parser.add_argument("dvifile", help='the DVI input file')

args = parser.parse_args()

format = args.format
dvifile = args.dvifile

if format not in ('svg', 'svgz'):
    print("Invalid format: "+format);
    sys.exit(255);

#debug environment
#print(repr(os.environ))

dvisvgm = ""
if "KLF_USCONFIG_dvisvgm" in os.environ:
    dvisvgm = os.environ["KLF_USCONFIG_dvisvgm"];

if not dvisvgm:
    dvisvgm = "/usr/bin/dvisvgm";

#print("dvisvgm: "+repr(dvisvgm))

if (not os.path.isfile(dvisvgm) or not os.access(dvisvgm, os.X_OK)):
    print("Error: Can't find dvisvgm executable. Please set the path to `dvisvgm` in\n"
          "Settings -> Scripts -> svg-dvisvgm -> Script Settings");
    sys.exit(255);

sys.stderr.write("Converting file "+dvifile+"\n");

svgfile = re.sub(r'\.dvi$', '.svg', dvifile)

# CalledProcessError is raised if an error occurs.
output = subprocess.check_output(args=[dvisvgm, '-a', '-e', '-n', '-b', 'min', dvifile],
                                 shell=False, stderr=subprocess.STDOUT)
print("Output from {}: \n{}".format(dvisvgm, output.decode('utf-8')))

if format == 'svgz':
    # compress the svg file
    gzip = '/usr/bin/gzip'
    # CalledProcessError is raised if an error occurs.
    output2 = subprocess.check_output(args=[gzip, '-Sz', svgfile], shell=False)
    print("Output from {}: \n{}".format(gzip, output2.decode('utf-8')))

sys.exit(0);


