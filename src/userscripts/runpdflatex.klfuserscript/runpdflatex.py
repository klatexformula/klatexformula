#!/usr/bin/env python

# dvipng-backend.py
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
#   $Id: klffeynmf.py 941 2015-06-07 19:38:11Z phfaist $

from __future__ import print_function

import re
import os
import sys
import subprocess
from distutils.util import strtobool
import math

import pyklfuserscript


def intceil(x):
    return int(math.ceil(x) + 0.01)
def intfloor(x):
    return int(math.floor(x) + 0.01)


args = pyklfuserscript.backend_engine_args_parser(can_query_default_settings=True).parse_args()

if args.query_default_settings:

    pdflatex = pyklfuserscript.find_executable(['pdflatex'], [
        "/usr/texbin/",
        "/usr/local/texbin/",
        "/Library/TeX/texbin/",
        # add more non-trivial paths here (but not necessary to include /usr/bin/
        # because we do a PATH search anyway
    ])
    if pdflatex is not None:
        print("pdflatex={}".format(pdflatex))

    sys.exit(0)


latexfname = args.texfile
pdffname = os.environ["KLF_FN_PDF"]
pngfname = os.environ["KLF_FN_PNG"]

#

gsexe = os.environ.get("KLF_GS")

pdflatex = os.environ.get("KLF_USCONFIG_pdflatex")

dpi_value = os.environ.get("KLF_INPUT_DPI", 180)

ps_x = os.environ.get("KLF_ARG_pagesize_x")
ps_y = os.environ.get("KLF_ARG_pagesize_y")

print("Using pdflatex path: {}".format(pdflatex), file=sys.stderr)
pyklfuserscript.ensure_configured_executable(pdflatex, exename='pdflatex', userscript=__file__)


tempdir = os.path.dirname(os.environ["KLF_TEMPFNAME"])

def run_cmd(do_cmd, ignore_fail=False, callargs={}):
    print("Running " + " ".join(["'%s'"%(x) for x in do_cmd]) + "  [ in %s ]..." %(tempdir) + "\n")
    output = None
    thecallargs = {'stderr': subprocess.STDOUT}
    thecallargs.update(dict(callargs))
    try:
        output = subprocess.check_output(do_cmd, cwd=tempdir, **thecallargs)
    except subprocess.CalledProcessError as e:
        if not ignore_fail:
            print("ERROR: Exit Status "+str(e.returncode)+ "\n" + e.output)
            raise
    return output


# add pdf page size commands latex
# --------------------------------

latexcode = ''
with open(latexfname, 'r') as f:
    latexcode = f.read()

# modify latex code

m = re.search(r'\\begin\{document\}', latexcode)
if m is None:
    print("Latex code does not contain \\begin{document}!!")
    raise ValueError("Latex code does not contain \\begin{document}!!")


vertical_centering_arg = os.environ.get("KLF_ARG_vertical_centering")
print("vertical_centering_arg = %r"%(vertical_centering_arg))

crop_arg = os.environ.get("KLF_ARG_crop")
print("crop_arg = %r"%(crop_arg))
crop_arg_val = strtobool(crop_arg)

outlinefonts_val = strtobool(os.environ.get("KLF_SETTINGS_OUTLINEFONTS", "1"))


vspc = '0pt'
if strtobool(vertical_centering_arg):
    # plus 1fil: vertically center equations on page
    vspc = '0pt plus 1fil'
    

LENGTHSCMDS = r"""
\oddsidemargin=0pt
\evensidemargin=0pt
\topmargin=0pt
\textwidth=%(w)s
\textheight=%(h)s
\pdfpageheight=%(h)s
\pdfpagewidth=%(w)s
\paperwidth=%(w)s
\paperheight=%(h)s
\voffset=-1in
\hoffset=-1in
\headsep=0pt
\headheight=0pt
\marginparsep=0pt
\footskip=0pt

\parindent=0pt
\parskip=0pt

\def\klfXXXsetdisplayskiplengths{%%
  \abovedisplayskip=%(vspc)s%%
  \belowdisplayskip=%(vspc)s%%
  \abovedisplayshortskip=%(vspc)s%%
  \belowdisplayshortskip=%(vspc)s%%
}
\klfXXXsetdisplayskiplengths
""" % { 'w': ps_x, 'h': ps_y, 'vspc': vspc }


newlatexcode = latexcode[:m.start()]
newlatexcode += LENGTHSCMDS
newlatexcode += latexcode[m.start():m.end()]
newlatexcode += "\n\\klfXXXsetdisplayskiplengths\n"
newlatexcode += latexcode[m.end():]

print("NEW LATEX CODE:\n----------\n"+newlatexcode+"\n----------\n")

with open(latexfname, 'w') as f:
    f.write(newlatexcode)



#
# Note: commands are run in correct temporary dir: See the definition of the
# function run_cmd()
#


# run pdflatex
# ------------

run_cmd([pdflatex, os.path.basename(latexfname)])



# crop and outline fonts if applicable
# ------------------------------------

crop_gs_args = []
outlinefonts_gs_args = []

if outlinefonts_val:

    gs_ver = tuple([int(x) for x in os.environ.get("KLF_GS_VERSION").split(".")])

    outlinefonts_gs_args = [ '-dNOCACHE' ]

    if gs_ver >= (9, 15):
        outlinefonts_gs_args = [ '-dNoOutputFonts' ]

if crop_arg_val:

    # get bounding box using gs
    bboxoutput = run_cmd([
        gsexe,
        '-q',
        '-dBATCH',
        '-dNOPAUSE',
        '-sDEVICE=bbox',
        pdffname
    ])

    print("GS Bounding box info:\n" + bboxoutput + "\n\n")

    mbb = re.search(r'^%%HiResBoundingBox:\s+([0-9.e+-]+)\s+([0-9.e+-]+)\s+([0-9.e+-]+)\s+([0-9.e+-]+)\s*$',
                    bboxoutput,
                    flags=re.MULTILINE)
    if mbb is None:
        raise ValueError("Ghostscript didn't report any bounding box!!")

    bbox = (float(mbb.group(1)), float(mbb.group(2)), float(mbb.group(3)), float(mbb.group(4)))

    crop_gs_args = [
        '-dDEVICEWIDTHPOINTS='+str(intceil(bbox[2]-bbox[0])),
        '-dDEVICEHEIGHTPOINTS='+str(intceil(bbox[3]-bbox[1])),
        '-dFIXEDMEDIA',
        '-c',
        "<</BeginPage{%.3f %.3f translate 0 0 %.3f %.3f rectclip}>> setpagedevice "%(
            -bbox[0],-bbox[1],bbox[2]-bbox[0], bbox[3]-bbox[1]),
        '-f',
        ]


if outlinefonts_val or crop_arg_val:

    origpdffname = pdffname + '.orig.pdf'
    os.rename(pdffname, origpdffname)

    run_cmd([
        gsexe,
        '-q',
        '-dBATCH',
        '-dNOPAUSE',
        '-sDEVICE=pdfwrite',
        '-sOutputFile='+pdffname,
    ] + outlinefonts_gs_args + crop_gs_args + [
        origpdffname
    ])





# get png from the pdf via ghostscript, that we can display in the klf app
# ------------------------------------------------------------------------

run_cmd([gsexe,
         '-q',
         '-dBATCH',
         '-dNOPAUSE',
         '-sDEVICE=pngalpha',
         '-sOutputFile='+pngfname,
         '-r' + str(dpi_value),
         pdffname])

