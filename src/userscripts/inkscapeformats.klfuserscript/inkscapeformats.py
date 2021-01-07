#!/usr/bin/env python
# -*- coding: utf-8 -*-

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

#sys.path.insert(0, os.path.join(os.path.dirname(__file__), '..')) -- no longer needed
import pyklfuserscript


FORMATS = ('svg', 'emf', 'wmf', )



def get_inkscape_version(inkscape_path):

    rx_ver = re.compile(
        r'\s*inkscape\s*(?P<major>\d+)[.](?P<minor>\d+)(?P<rel_part>\S*)?(\s+|$)',
        flags=re.IGNORECASE | re.MULTILINE
    )
    try:
        output = subprocess.check_output(args=[inkscape_path, '--version'],
                                         shell=False, stderr=subprocess.STDOUT)
    except Exception:
        raise RuntimeError(u"Cannot run inkscape to determine its version: ‘{}’".format(inkscape))

    try:
        output = output.decode('utf-8')
    except Exception:
        pass

    m = rx_ver.match(output)
    if m is None:
        raise RuntimeError(u"Cannot parse inkscape version ’{}’: ‘{}’".format(inkscape,output))

    return {'major': int(m.group('major')), 'minor': int(m.group('minor')),
            'rel_part': m.group('rel_part')}
        

#
#
#


args = pyklfuserscript.export_type_args_parser().parse_args()

if args.query_default_settings:

    inkscape = pyklfuserscript.find_executable(['inkscape'], [
        "/Applications/Inkscape.app/Contents/Resources/bin/",
        "C:/Program Files/Inkscape/",
        "C:/Program Files (x86)/Inkscape/",
        # add more non-trivial paths here (but it's not necessary to include
        # /usr/bin/ because we do a PATH search anyway)
    ])

    use_new_option_syntax_v1x = True # by default

    if inkscape is not None:
        try:
            inkscape_version = get_inkscape_version(inkscape)
            if inkscape_version['major'] >= 1:
                use_new_option_syntax_v1x = True
            else:
                use_new_option_syntax_v1x = False
        except Exception as e:
            sys.stderr.write("{}\n".format(e))

    if inkscape is not None:
        # found
        print("""\
<?xml version='1.0' encoding='UTF-8'?>
<klfuserscript-default-settings>
   <pair>
      <key>inkscape</key>
      <value type="QString">{}</value>
   </pair>
   <pair>
      <key>use_new_option_syntax_v1x</key>
      <value type="bool">{}</value>
   </pair>
</klfuserscript-default-settings>
""".format(
    pyklfuserscript.escapexml(pyklfuserscript.saveStringForKlfVariantText(inkscape)),
    'true' if use_new_option_syntax_v1x else 'false'
)
        )
    sys.exit(0)



format = args.format
pdffile = args.inputfile

if format not in FORMATS:
    print("Invalid format: {}".format(format))
    sys.exit(255)

#debug environment
#print(repr(os.environ))

if "KLF_USCONFIG_inkscape" in os.environ:
    inkscape = os.environ["KLF_USCONFIG_inkscape"]
else:
    print("Warning: inkscape config not set.")
    inkscape = "inkscape"

if "KLF_USCONFIG_use_new_option_syntax_v1x" in os.environ:
    if re.match(r'^\s*(t|true|1|y|yes|on)\s*$',
                os.environ["KLF_USCONFIG_use_new_option_syntax_v1x"]) is not None:
        use_new_option_syntax_v1x = True
    else:
        use_new_option_syntax_v1x = False
else:
    use_new_option_syntax_v1x = True # guess ?


print("Using inkscape path: {}".format(inkscape), file=sys.stderr)

pyklfuserscript.ensure_configured_executable(inkscape, exename='inkscape', userscript=__file__)

print("Converting file {}\n".format(pdffile), file=sys.stderr)

outfile = re.sub(r'\.pdf$', '.'+format, pdffile)

if use_new_option_syntax_v1x:
    export_flags = [ '--export-type='+format ]
    if format == 'svg':
        export_flags.append('--export-plain-svg')

    cmdargs = [ inkscape ]
    cmdargs += export_flags
    cmdargs += [ pdffile,
                 '-o',
                 outfile ]
else:
    if format == 'svg':
        exportarg_outfile = "--export-plain-svg="+outfile
    else:
        exportarg_outfile = "--export-"+format+"="+outfile
    cmdargs = [ inkscape, pdffile, exportarg_outfile ]

# CalledProcessError is raised if an error occurs.
output = subprocess.check_output(args=cmdargs, shell=False, stderr=subprocess.STDOUT)

print("Output from {}: \n{}".format(inkscape, output.decode('utf-8')))

sys.exit(0)


