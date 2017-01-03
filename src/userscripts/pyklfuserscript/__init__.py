
from __future__ import print_function

import re
import os
import os.path
import sys
import argparse
import platform
import subprocess



def export_type_args_parser(can_query_default_settings=True):
    parser = argparse.ArgumentParser()
    if can_query_default_settings:
        # usage: $0 --query-default-settings
        parser.add_argument("--query-default-settings", action='store_true',
                            dest='query_default_settings', default=False,
                            help='print out default settings')
    # usage: $0 --format=xyz <pdffile>
    parser.add_argument("--format", help='output format to use')
    parser.add_argument("inputfile", help='the input file', nargs='?')
    return parser


def exists_and_is_executable(x):
    return os.path.isfile(x) and os.access(x, os.X_OK)

def find_executable(exe_names, guess_paths=[]):
   
    all_paths = guess_paths + os.environ['PATH'].split(os.pathsep)

    for i in all_paths:
        for x in exe_names:
            f = os.path.join(i, x)
            if platform.system() == 'Windows':
                f += '.exe'
            if exists_and_is_executable(f):
                return f

    # not found.
    sys.stderr.write("Couldn't find {!r} executable. I looked everywhere I could in {!r}\n".format(exe_names, all_paths))
    return None


def ensure_configured_executable(exe, exename, userscript):
    if not exists_and_is_executable(exe):
        print("Error: Can't find {exename} executable. Please set the path to `{exename}` in\n"
              "Settings -> Scripts -> {userscriptname} -> Script Settings"
              .format({'exename':exename,'userscriptname':userscript}), file=sys.stderr)
        sys.exit(255)
    
