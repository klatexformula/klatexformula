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

#sys.path.insert(0, os.path.join(os.path.dirname(__file__), '..')) -- no longer needed
import pyklfuserscript


args = pyklfuserscript.export_type_args_parser().parse_args()

format = args.format
dvifile = args.inputfile

if args.query_default_settings:

    dvisvgm = pyklfuserscript.find_executable(['dvisvgm'], [
        "/usr/texbin/",
        "/usr/local/texbin/",
        "/Library/TeX/texbin/",
        # add more non-trivial paths here (but not necessary to include /usr/bin/
        # because we do a PATH search anyway
    ])
    if dvisvgm is not None:
        # found
        print("dvisvgm={}".format(dvisvgm))
    gzip = pyklfuserscript.find_executable(['gzip'], [
        "/usr/bin", "/bin", "/usr/local/bin"
    ])
    if gzip is not None:
        print("gzip={}".format(gzip))

    sys.exit(0)



if format not in ('svg', 'svgz'):
    print("Invalid format: "+format, file=sys.stderr)
    sys.exit(255)


#debug environment
#print(repr(os.environ))

dvisvgm = os.environ.get("KLF_USCONFIG_dvisvgm")
if not dvisvgm:
    print("Warning: dvisvgm config not set", file=sys.stderr)
    dvisvgm = "/usr/bin/dvisvgm"

print("Using dvisvgm path: {}".format(dvisvgm), file=sys.stderr)

pyklfuserscript.ensure_configured_executable(dvisvgm, exename='dvisvgm', userscript=__file__)

print("Converting file {}\n".format(dvifile), file=sys.stderr)

svgfile = re.sub(r'\.dvi$', '.svg', dvifile)

# CalledProcessError is raised if an error occurs.
output = subprocess.check_output(args=[dvisvgm, '-a', '-e', '-n', '-b', 'min', dvifile],
                                 shell=False, stderr=subprocess.STDOUT)
print("Output from {}: \n{}".format(dvisvgm, output.decode('utf-8')))

if format == 'svgz':

    gzip = ""
    if "KLF_USCONFIG_gzip" in os.environ:
        gzip = os.environ["KLF_USCONFIG_gzip"]
    if not gzip:
        print("Warning: gzip config not set", file=sys.stderr)
        gzip = "/usr/bin/gzip"

    pyklfuserscript.ensure_configured_executable(gzip, exename='gzip', userscript=__file__)

    # CalledProcessError is raised if an error occurs.
    output2 = subprocess.check_output(args=[gzip, '-Sz', svgfile], shell=False)
    print("Output from {}: \n{}".format(gzip, output2.decode('utf-8')))

sys.exit(0)


