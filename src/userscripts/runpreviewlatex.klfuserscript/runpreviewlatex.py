#!/usr/bin/env python

# runpreviewlatex.py
#   This file is part of the KLatexFormula Project.
#   Copyright (C) 2020 by Philippe Faist
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

    tex_bin_paths = [
        "/usr/texbin/",
        "/usr/local/texbin/",
        "/Library/TeX/texbin/",
        # add more non-trivial paths here (but not necessary to include /usr/bin/
        # because we do a PATH search anyway
    ]

    pdflatex = pyklfuserscript.find_executable(['pdflatex'], tex_bin_paths)
    if pdflatex is not None:
        print("pdflatex={}".format(pdflatex))
    xelatex = pyklfuserscript.find_executable(['xelatex'], tex_bin_paths)
    if xelatex is not None:
        print("xelatex={}".format(xelatex))
    lualatex = pyklfuserscript.find_executable(['lualatex'], tex_bin_paths)
    if lualatex is not None:
        print("lualatex={}".format(lualatex))

    sys.exit(0)


latexfname = args.texfile
pdffname = os.environ["KLF_FN_PDF"]
pngfname = os.environ["KLF_FN_PNG"]

#

gsexe = os.environ.get("KLF_GS")

pdflatex = os.environ.get("KLF_USCONFIG_pdflatex")
xelatex = os.environ.get("KLF_USCONFIG_xelatex")
lualatex = os.environ.get("KLF_USCONFIG_lualatex")

dpi_value = int(os.environ.get("KLF_INPUT_DPI", 180))

vectorscale_s = os.environ.get("KLF_INPUT_VECTORSCALE", "1").strip()
vectorscale = float(vectorscale_s)
if re.match(r'^\s*0*1([.]0*)?(e[+-]0+)?$', vectorscale_s):
    has_vectorscale = False
else:
    has_vectorscale = True

ps_x = os.environ.get("KLF_ARG_pagesize_x")
ps_y = os.environ.get("KLF_ARG_pagesize_y")

arg_ltxengine = os.environ.get("KLF_ARG_ltxengine")

if arg_ltxengine == 'pdflatex': ltxexe = pdflatex
elif arg_ltxengine == 'xelatex': ltxexe = xelatex
elif arg_ltxengine == 'lualatex': ltxexe = lualatex
else:
    raise ValueError("Invalid latex engine: {}".format(arg_ltxengine))

print("Using engine {} at {}".format(arg_ltxengine, ltxexe), file=sys.stderr)

pyklfuserscript.ensure_configured_executable(ltxexe, exename=arg_ltxengine, userscript=__file__)


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
    if sys.version_info >= (3, 0):
        # need to convert output from bytes to string
        output = output.decode('utf-8')
    return output


# add pdf page size commands latex
# --------------------------------

#latexcode = ''
#with open(latexfname, 'r') as f:
#    latexcode = f.read()

# modify latex code

# m = re.search(r'\\begin\{document\}', latexcode)
# if m is None:
#     print("Latex code does not contain \\begin{document}!!")
#     raise ValueError("Latex code does not contain \\begin{document}!!")

outlinefonts_val = strtobool(os.environ.get("KLF_SETTINGS_OUTLINEFONTS", "1"))

fontsize_cmds = ""
fsz = int(os.environ.get("KLF_INPUT_FONTSIZE", -1))
if fsz != -1:
    fontsize_cmds = r"""
\fontsize{%(fsz)s}{%(fszbl)s}\selectfont%%
"""[1:] % {
        'fsz': fsz, 'fszbl': 1.2*fsz
    }

rx_col = re.compile(r"rgba?\(\s*(?P<r>\d+)\s*,\s*(?P<g>\d+)\s*,\s*(?P<b>\d+)\s*(?:,\s*(?P<a>\d+)\s*)\)", flags=re.IGNORECASE)

color_cmds = r"""
\definecolor{klffgcolor}{RGB}{%(r)s,%(g)s,%(b)s}%%
"""[1:] % rx_col.match(os.environ.get("KLF_INPUT_FG_COLOR_RGBA")).groupdict()

### --- \pagecolor{} is ignored by {preview} package
# hack_page_bg_color = ""
# bgm = rx_col.match(os.environ.get("KLF_INPUT_BG_COLOR_RGBA")).groupdict()
# if bgm['a'] and int(bgm['a']) > 0:
#     hack_page_bg_color = r"""
# \pdfliteral page{%
#     q % gsave
#     \current@page@color\GPT@space
#     n % newpath
#     0 0 \strip@pt\pdfpagewidth\GPT@space
#     \strip@pt\pdfpageheight\GPT@space re % rectangle
#     % there is no need to convert to bp
#     f % fill
#     Q% grestore
#     }%
# """[1:] ...........???????????
#     color_cmds += r"""
# \definecolor{klfbgcolor}{RGB}{%(r)s,%(g)s,%(b)s}%%
# \pagecolor{klfbgcolor}%%
# """[1:] % bgm.groupdict()

m = re.match(r'(?P<top>[\d.+-]+)\s*,\s*(?P<right>[\d.+-]+)\s*,\s*(?P<bottom>[\d.+-]+)\s*,\s*(?P<left>[\d.+-]+)\s*(?:pt)?',
             os.environ.get("KLF_SETTINGS_BORDEROFFSET", ""),
             flags=re.IGNORECASE)
preview_border_margins = ""
if m is not None:
    preview_border_margins = r"""
\renewcommand{\PreviewBbAdjust}{-%(left)spt -%(bottom)spt %(right)spt %(top)spt}%%
"""[1:] % m.groupdict()


latex_input = os.environ.get("KLF_INPUT_LATEX")

mm = os.environ.get("KLF_INPUT_MATHMODE")

# replace display equations by inline \displaystyle equivalents:
delims = mm.split('...', 1)
def match_mathmode(a, b, mm, escape=True):
    do_escape = re.escape if escape else lambda x : x
    return re.match(r'\s*'+do_escape(a)+r'\s*\.\.\.\s*'+do_escape(b)+r'\s*', mm)

def match_environ_rx(env, mm):
    return match_mathmode( r'\\begin\s*\{(?P<envname>'+env+r')\}',
                           r'\\end\s*\{(?P=envname)\}', mm,
                           escape=False )

if match_mathmode(r'\[', r'\]', mm) is not None or \
   match_environ_rx(r'equation\*?', mm) is not None:
    delims = (r'$\displaystyle', r'$')
else:
    m = match_environ_rx(r'align\*?|gather\*?', mm)
    if m is not None:
        env = m.group('envname').rstrip('*')
        delims = (r'$\displaystyle\begin{' + env + r'ed}', # "align" -> "aligned" etc.
                  r'\end{' + env + r'ed}$')
    else:
        delims = mm.split('...', 1)

eqn_content = delims[0] + latex_input + delims[1]

#
# get user-defined preamble
#
preamble = os.environ.get("KLF_INPUT_PREAMBLE")


#
# add \scalebox{}{} if we want to scale the vector graphics
#
if has_vectorscale:
    eqn_content = r"\scalebox{%s}{%s}"%(vectorscale_s, eqn_content)
    preamble += "\n\n" + r"\RequirePackage{graphicx}" + "\n\n"

# --- form template ---

latexcode = r"""
\documentclass{article}
\usepackage{color}

%(preamble)s

\usepackage[active,tightpage]{preview}
%(preview_border_margins)s
\begin{document}%%
%(color_cmds)s%%
%(fontsize_cmds)s%%
\begin{preview}%%
{\color{klffgcolor}%%
%(eqn_content)s%%
}%%
\end{preview}
\end{document}
"""[1:] % {
    'preamble': preamble,
    'preview_border_margins': preview_border_margins,
    'color_cmds': color_cmds,
    'fontsize_cmds': fontsize_cmds,
    'eqn_content': eqn_content
}

print("LATEX CODE:\n----------\n"+latexcode+"\n----------\n")

with open(latexfname, 'w') as f:
    f.write(latexcode)


#
# Note: commands are run in correct temporary dir: See the definition of the
# function run_cmd()
#


# run pdflatex
# ------------

run_cmd([ltxexe, os.path.basename(latexfname)])



# crop and outline fonts if applicable
# ------------------------------------

outlinefonts_gs_args = []

if outlinefonts_val:

    gs_ver = tuple([int(x) for x in os.environ.get("KLF_GS_VERSION").split(".")])

    if gs_ver >= (9, 15):
        outlinefonts_gs_args = [ '-dNoOutputFonts' ]
    else:
        outlinefonts_gs_args = [ '-dNOCACHE' ]

    origpdffname = pdffname + '.orig.pdf'
    os.rename(pdffname, origpdffname)

    run_cmd([
        gsexe,
        '-q',
        '-dBATCH',
        '-dNOPAUSE',
        '-sDEVICE=pdfwrite',
        '-sOutputFile='+pdffname,
    ] + outlinefonts_gs_args + [
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

