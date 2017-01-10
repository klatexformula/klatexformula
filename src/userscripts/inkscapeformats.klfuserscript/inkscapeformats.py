#!/usr/bin/env python

# inkscapeformats.py
#   This file is part of the KLatexFormula Project.
#   Copyright (C) 2017 by Philippe Faist
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
import os.path
import sys
import argparse
import subprocess

sys.path.insert(0, os.path.join(os.path.dirname(__file__), '..'))
import pyklfuserscript


FORMATS = ('emf', 'wmf',)

args = pyklfuserscript.export_type_args_parser().parse_args()

if args.query_default_settings:

    inkscape = pyklfuserscript.find_executable(['inkscape'], [
        "/Applications/Inkscape.app/Contents/Resources/bin/",
        "C:/Program Files/Inkscape/",
        "C:/Program Files (x86)/Inkscape/",
        # add more non-trivial paths here (but not necessary to include /usr/bin/
        # because we do a PATH search anyway
    ])
    if inkscape is not None:
        # found
        print("""\
<?xml version='1.0' encoding='UTF-8'?>
<klfuserscript-default-settings>
   <pair>
      <key>inkscape</key>
      <value type="QString">{}</value>
   </pair>
</klfuserscript-default-settings>
""".format(pyklfuserscript.escapexml(pyklfuserscript.saveStringForKlfVariantText(inkscape))))
    sys.exit(0)



format = args.format
pdffile = args.inputfile

if format not in FORMATS:
    print("Invalid format: "+format);
    sys.exit(255);

#debug environment
#print(repr(os.environ))

if "KLF_USCONFIG_inkscape" in os.environ:
    inkscape = os.environ["KLF_USCONFIG_inkscape"];
else:
    print("Warning: inkscape config not set.")
    inkscape = "inkscape"

print("Using inkscape path: {}".format(inkscape), file=sys.stderr)

pyklfuserscript.ensure_configured_executable(inkscape, exename='inkscape', userscript=__name__)

print("Converting file {}\n".format(pdffile), file=sys.stderr)

outfile = re.sub(r'\.pdf$', '.'+format, pdffile)

# CalledProcessError is raised if an error occurs.
output = subprocess.check_output(args=[
    inkscape, pdffile, '--export-'+format+'='+outfile
], shell=False, stderr=subprocess.STDOUT)

print("Output from {}: \n{}".format(inkscape, output.decode('utf-8')))

sys.exit(0)


