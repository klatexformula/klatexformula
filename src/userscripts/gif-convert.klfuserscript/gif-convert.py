#!/usr/bin/env python

# gif-convert.py
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
import sys
import os.path
import argparse
import subprocess

#sys.path.insert(0, os.path.join(os.path.dirname(__file__), '..')) -- no longer needed
import pyklfuserscript


args = pyklfuserscript.export_type_args_parser().parse_args()

if args.query_default_settings:
    convert = pyklfuserscript.find_executable(['convert'], [
        '/usr/local/bin',
        '/opt/local/bin',
        os.path.join(os.environ.get('MAGICK_HOME',''), 'bin')
        # add more non-trivial paths here (but not necessary to include /usr/bin/
        # because we do a PATH search anyway
    ])
    if convert is not None:
        # found
        print("convert={}".format(convert))
    sys.exit(0)


format = args.format
pngfile = args.inputfile

if format != 'gif':
    print("Invalid format: ", format)
    sys.exit(1)

convert = ""
if "KLF_USCONFIG_convert" in os.environ:
    convert = os.environ["KLF_USCONFIG_convert"]
if not convert:
    if os.path.exists("/usr/bin/convert"):
        convert = "/usr/bin/convert"
    elif os.path.exists("/usr/local/bin/convert"):
        convert = "/usr/local/bin/convert"


giffile = re.sub(r'\.png$', r'.gif', pngfile)

# CalledProcessError is raised if an error occurs.
output = subprocess.check_output(args=[convert, pngfile, giffile],
                                 shell=False, stderr=subprocess.STDOUT)

print("Output from {}: \n{}".format(convert, output.decode('utf-8')))

sys.exit(0)


