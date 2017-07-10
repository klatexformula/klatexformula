
from __future__ import print_function

import re
import os
import os.path
import sys
import argparse
import platform
import subprocess

# bump version number to corresponding KLF version whenever we add something to this
# module, so that it can be detected by user scripts
__version__ = '4.0.1'


rx_xml_escape = re.compile(r"([^ 0-9A-Za-z\!\@\#\$\%\^\*\(\)\_\+\=\{\}\[\]\|\:\'\,\.\/\?\`\~\-])")

def saveStringForKlfVariantText(x):
    if isinstance(x, bytes):
        x = x.decode('utf-8')
    return rx_xml_escape.sub(lambda m: u'\\x{:04x}'.format(ord(m.group(0))), x)

def escapexml(x):
    return rx_xml_escape.sub(lambda m: '&#x{:04x};'.format(ord(m.group(0))), x)



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


def backend_engine_args_parser(can_query_default_settings=True):
    parser = argparse.ArgumentParser()
    if can_query_default_settings:
        # usage: $0 --query-default-settings
        parser.add_argument("--query-default-settings", action='store_true',
                            dest='query_default_settings', default=False,
                            help='print out default settings')
    # usage: $0 --format=xyz <pdffile>
    parser.add_argument("texfile", help='the LaTeX input file', nargs='?')
    return parser


def exists_and_is_executable(x):
    return x and os.path.isfile(x) and os.access(x, os.X_OK)

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
    if userscript.endswith('.py'): # passed __file__
        # this should extract the "xyz.klfuserscript" part:
        userscript = os.path.basename(os.path.dirname(os.path.realpath(os.path.abspath(userscript))))

    if not exists_and_is_executable(exe):
        print("Error: Can't find {exename} executable. Please set the path to `{exename}` in\n"
              "Settings -> Scripts -> {userscriptname} -> Script Settings"
              .format(exename=exename, userscriptname=userscript), file=sys.stderr)
        sys.exit(255)
    
