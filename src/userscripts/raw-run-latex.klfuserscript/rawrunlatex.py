#!/usr/bin/env python

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

    latex = pyklfuserscript.find_executable(['pdflatex'], [
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

with open(latexfname, 'w') as f:
    f.write(os.environ["KLF_INPUT_LATEX"])

#

gsexe = os.environ.get("KLF_GS")
dvipsexe = os.environ.get("KLF_DVIPS")

pdflatex = os.environ.get("KLF_ARG_pdflatex") # KLF_USCONFIG_pdflatex

dpi_value = 300 #os.environ.get("KLF_INPUT_DPI", 180)

print("Using pdflatex path: {}".format(pdflatex), file=sys.stderr)
pyklfuserscript.ensure_configured_executable(pdflatex, exename='pdflatex', userscript=__file__)


tempdir = os.path.dirname(os.environ["KLF_TEMPFNAME"])

def run_cmd(do_cmd, ignore_fail=False, callargs=None, add_env=None):
    print("Running " + " ".join(["'%s'"%(x) for x in do_cmd]) + "  [ in %s ]..." %(tempdir) + "\n")
    output = None
    thecallargs = {'stderr': subprocess.STDOUT}
    if callargs:
        thecallargs.update(dict(callargs))
    if add_env:
        env = thecallargs.get('env', {})
        if not env:
            env = dict(os.environ)
        env.update(add_env)
        thecallargs['env'] = env
    try:
        output = subprocess.check_output(do_cmd, cwd=tempdir, **thecallargs)
    except subprocess.CalledProcessError as e:
        if not ignore_fail:
            print("ERROR: Exit Status "+str(e.returncode)+ "\n" + e.output)
            raise
    return output



#
# Note: commands are run in correct temporary dir: See the definition of the
# function run_cmd()
#


# run pdflatex
# ------------

TEXINPUTS = os.path.dirname(__file__)+":"+os.environ.get('TEXINPUTS','')

output = run_cmd([pdflatex, os.path.basename(latexfname)],
                 add_env={'TEXINPUTS': TEXINPUTS})
print("{} output:\n{}".format(os.path.basename(pdflatex), output))


# # run twice to get transparencies right (if using {trasparent} latex package)
# output = run_cmd([pdflatex, os.path.basename(latexfname)],
#                  add_env={'TEXINPUTS': TEXINPUTS})
# print("{} output:\n{}".format(os.path.basename(pdflatex), output))




# if we're using latex instead of pdflatex, run dvips & gs
# --------------------------------------------------------

if os.path.basename(pdflatex) == 'latex':
    dvifname = re.sub(r'\.tex$', '.dvi', latexfname)
    psfname = re.sub(r'\.tex$', '.ps', latexfname)
    run_cmd([dvipsexe,
             dvifname])
    run_cmd([gsexe,
             '-q',
             '-dBATCH',
             '-dNOPAUSE',
             '-sDEVICE=pdfwrite',
             '-sOutputFile='+pdffname,
             '-r' + str(dpi_value),
             psfname])




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

