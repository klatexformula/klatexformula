#!/usr/bin/env python

# custom-latex-cmd.py
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
import subprocess
from pipes import quote as shellquote

if sys.argv[1] == "--help":
    print("Usage: "+os.path.basename(sys.argv[0])+" <tex input file>")
    print("")
    sys.exit(0)


# get config, input etc.

latexfname = sys.argv[1]
dvifname = os.environ["KLF_FN_DVI"]

latexcmd = os.environ.get("KLF_ARG_latexcmd", "latex {texfname}")

print("User-provided LaTeX command:", repr(latexcmd))

tempdir = os.path.dirname(os.environ["KLF_TEMPFNAME"])


# Run latex custom command
# ------------------------

# perform required replacements
latexcmd_full = latexcmd.format(latex=shellquote(os.environ["KLF_LATEX"]),
                                texfname=shellquote(latexfname))

# raises exception if error -- will be picked up by Python runtime
print("Running `{}' ...\n".format(latexcmd_full))
res = subprocess.call(latexcmd_full, cwd=tempdir, shell=True)
if res != 0:
    print("Failed, res=", res)
    sys.exit(res>>8)

