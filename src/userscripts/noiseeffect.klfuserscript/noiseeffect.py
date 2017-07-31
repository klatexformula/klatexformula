import sys
import os
import subprocess

import pyklfuserscript
args = pyklfuserscript.export_type_args_parser().parse_args()

#
# script is being run in "--query-default-settings" mode (since we set
# <can-provide-default-settings> in scriptinfo.xml)
#
if args.query_default_settings:
    convert = pyklfuserscript.find_executable(['convert'], [
        '/usr/local/bin',
        '/opt/local/bin',
        # add more paths to search for here (but $PATH is searched for anyway, so it's not
        # necessary to include /usr/bin/ etc.)
    ])
    if convert is not None: # found
        print("convert={}".format(convert))
    sys.exit()


#
# Run script
#

format = args.format
inputfile = args.inputfile

#
# Read configuration values
#
convertexe = os.environ.get("KLF_USCONFIG_convert")
noisepixels = int(os.environ.get("KLF_USCONFIG_noisepixels", 3)) # noise intensity, in pixels

pyklfuserscript.ensure_configured_executable(convertexe, exename='convert', userscript=__file__)

cmd = [convertexe, inputfile, '-interpolate', 'nearest',
       '-virtual-pixel', 'mirror',
       '-spread', "{}".format(noisepixels),
       inputfile] # overwrite input file

subprocess.check_output(cmd)
