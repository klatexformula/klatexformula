#!/usr/bin/env python

# dvipng-backend.py
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
#   $Id: klffeynmf.py 941 2015-06-07 19:38:11Z phfaist $

from __future__ import print_function

import re;
import os;
import sys;
import subprocess;

if (sys.argv[1] == "--help"):
    print("       "+os.path.basename(sys.argv[0])+" <tex input file>\n");
    exit(0);

latexfname = sys.argv[1];
dvifname = os.environ["KLF_FN_DVI"]
pngfname = os.environ["KLF_FN_PNG"]

# run latex

latexexe = os.environ["KLF_LATEX"]

dvipng = os.environ.get("KLF_USCONFIG_dvipng", "dvipng")

if (len(latexexe) == 0):
    sys.stderr.write("Error: latex executable not found.\n");
    sys.exit(1);

tempdir = os.path.dirname(os.environ["KLF_TEMPFNAME"]);

def run_cmd(do_cmd, ignore_fail=False):
    print("Running " + " ".join(["'%s'"%(x) for x in do_cmd]) + "  [ in %s ]..." %(tempdir) + "\n")
    res = subprocess.call(do_cmd, cwd=tempdir);
    if (not ignore_fail and res != 0):
        print("%s failed, res=%d" %(do_cmd[0], res));
        sys.exit(res>>8);

# run latex first to get DVI
# --------------------------

run_cmd([latexexe, os.path.basename(latexfname)])


# now, run DVIPNG
# ---------------

def rgba(r, g, b, a):
    return 'rgb {:3.1f} {:3.1f} {:3.1f}'.format(r/255.0, g/255.0, b/255.0)

fg_col_dvipng = eval(os.environ.get('KLF_INPUT_FG_COLOR_RGBA'))

if eval(os.environ.get('KLF_INPUT_BG_COLOR_TRANSPARENT')):
    bg_col_dvipng = 'Transparent'
else:
    bg_col_dvipng = eval(os.environ.get('KLF_INPUT_BG_COLOR_RGBA'))


run_cmd([
    dvipng,
    os.path.basename(dvifname),
    '-D', str(os.environ.get("KLF_INPUT_DPI")),
    '-T', 'tight',
    '-o', pngfname,
    '-bg', bg_col_dvipng,
    '-fg', fg_col_dvipng,
    '-z', '4',
])


print("To dump all KLF environment variables (user script interface debugging), comment "
      "out the early 'sys.exit(0)' instruction in this script.")

sys.exit(0); # comment this line to debug KLF_ env variables values



#
# Debug KLF_ environment variables.
#


def dump_env_var(varname):
    print(varname, ': ', os.environ.get(varname), sep='')

def dump_env_vars(varlists):
    print('========')
    for varlist in varlists:
        for varname in varlist:
            dump_env_var(varname)
        print('---')
    allvarlist = sum(varlists, [])
    for varname in [ x for x in os.environ.keys() if x.startswith('KLF_') and x not in allvarlist ]:
        dump_env_var(varname)
    print('========')

dump_env_vars([
    [
        "KLF_INPUT_LATEX",
        "KLF_INPUT_MATHMODE",
        "KLF_INPUT_PREAMBLE",
        "KLF_INPUT_FONTSIZE",
        "KLF_INPUT_FG_COLOR_WEB",
        "KLF_INPUT_FG_COLOR_RGBA",
        "KLF_INPUT_BG_COLOR_TRANSPARENT",
        "KLF_INPUT_BG_COLOR_WEB",
        "KLF_INPUT_BG_COLOR_RGBA",
        "KLF_INPUT_DPI",
        "KLF_INPUT_USERSCRIPT",
        "KLF_INPUT_BYPASS_TEMPLATE",
    ],
    [
        "KLF_TEMPDIR",
        "KLF_TEMPFNAME",
        "KLF_LATEX",
        "KLF_DVIPS",
        "KLF_GS",
        "KLF_GS_VERSION",
        "KLF_GS_DEVICES",
    ],
    [
        "KLF_SETTINGS_BORDEROFFSET",
        "KLF_SETTINGS_OUTLINEFONTS",
        "KLF_SETTINGS_CALCEPSBOUNDINGBOX",
        "KLF_SETTINGS_WANT_RAW",
        "KLF_SETTINGS_WANT_PDF",
        "KLF_SETTINGS_WANT_SVG",
    ],
    [
        "KLF_FN_TEX",
        "KLF_FN_LATEX",
        "KLF_FN_DVI",
        "KLF_FN_EPS_RAW",
        "KLF_FN_EPS_PROCESSED",
        "KLF_FN_PNG",
        "KLF_FN_PDFMARKS",
        "KLF_FN_PDF",
        "KLF_FN_SVG_GS",
        "KLF_FN_SVG",
    ],
])

sys.exit(0)
