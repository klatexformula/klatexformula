import sys
import os
import shlex
import subprocess

try:
    from shlex import quote as shellquote
except ImportError:
    from pipes import quote as shellquote

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

pyklfuserscript.ensure_configured_executable(convertexe, exename='convert', userscript=__file__)


# image dimensions
width = int(os.environ.get('KLF_OUTPUT_WIDTH_PX'))
height = int(os.environ.get('KLF_OUTPUT_HEIGHT_PX'))
mindim = min(width, height)

print("Requested format is {}".format(format))


# round() in Python2 gives a float, not an int as in Python3
doround = lambda x : int(round(x)+0.01) # +0.01 to avoid numerical errors


if format == 'outer-glow':

    factor = mindim/200.0

    values = dict(
        blursize = 1+doround(factor*6),
        shiftx = 1+doround(factor*4),
        shifty = 1+doround(factor*3),
    )

    innercmd = shlex.split("""
        -background transparent -splice {spl1x}x{spl1y}+0+0 -splice {spl2x}x{spl2y}+{width}+{height}
        ( +clone -channel RGB +level-colors #008080,#008080
          -channel ALL -splice {shiftx}x{shifty}+0+0 -virtual-pixel transparent -blur 0x{blursize} )
        +swap -compose divide -composite
    """.format(**dict(values,
                      spl1x=values['blursize']-values['shiftx'],
                      spl2x=values['blursize']+values['shiftx'],
                      spl1y=values['blursize']-values['shifty'],
                      spl2y=values['blursize']+values['shifty'],
                      width=width,
                      height=height,
    )))

elif format == 'wave':

    factor = mindim/200.0

    innercmd = shlex.split("""
        -background transparent -wave {wave_a}x{wave_l}
    """.format(
        wave_a=1+doround(factor*10),
        wave_l=1+doround(factor*200),
    ))

elif format == 'swirl':

    factor = mindim/200.0
    swirllen = 1+doround(factor*40)

    innercmd = shlex.split("""
        -background transparent -swirl {swirllen}
    """.format(
        swirllen=swirllen
    ))

elif format == 'plasma':

    blursize = doround(mindim/100.0)
    paintsize = 4+doround((mindim/100.0)*2)

    innercmd = shlex.split("""
        ( -size {mindim}x{mindim} tile:plasma:red-blue -write mpr:tmpplasma +delete
          -size {width}x{height} tile:mpr:tmpplasma -blur 0x{blursize} -paint {paintsize} )
        -swap 0,1 -compose CopyOpacity -composite """.format(
            mindim=mindim,
            width=width,
            height=height,
            blursize=blursize,
            paintsize=paintsize)
    )

elif format == 'on-fire':

    factor = height/200.0

    values = dict(
        blurlen0 = 2+doround(factor*1.5),

        traillen1 = 1+doround(factor*4),
        blurlen1 = 1+doround(factor*3),
        wave1a = 1+doround(factor*1.3),
        wave1l = 1+doround(factor*80),

        traillen2 = 1+doround(factor*8),
        wave2a = 1+doround(factor*3),
        wave2l = 1+doround(factor*120),

        fac3pc = 25,
        fac3ipc = 400,
        traillen3 = 1+doround(factor*15/4.0),
        rolllen3 = 1+doround(factor*10/4.0),
        wave3a = 1+doround(factor*4),
        wave3l = 1+doround(factor*105),

        fac4pc = 20,
        fac4ipc = 500,
        traillen4 = 1+doround(factor*25/5.0),
        rolllen4 = 1+doround(factor*25/5.0),
        wave4a = 1+doround(factor*5),
        wave4l = 1+doround(factor*150),

        fac5pc = 10,
        fac5ipc = 1000,
#        shiftuplen5 = 0,#1+doround(factor*50/10.0),
        traillen5 = 1+doround(factor*60/10.0),
        rolllen5 = 1+doround(factor*45/10.0),
        wave5a = 1+doround(factor*6),
        wave5l = 1+doround(factor*250),
    )
    borderfactor = 1.5 # because blur dimensions are standard deviations, go further to
                       # make sure eg. smoke is not truncated by too much
    values.update(dict(
        border = int( ( values['wave5a'] +
                        values['wave4a'] +
                        # |cos(100 deg)| ~~ 0.18
                        values['wave3a'] + 0.18*values['traillen3']*values['fac3ipc']/100.0 +
                        # |cos(110 deg)| ~~ 0.35
                        values['wave2a'] + 0.35*values['traillen2'] +
                        # |cos(120 deg)| = 0.5
                        values['wave1a'] + 0.5*values['traillen1'] ) * borderfactor ) + 1 ,
    ))
    values.update(dict(
        topborderadd = int( ( #values['shiftuplen5']*values['fac5ipc']/100.0 +
                              values['traillen5']*values['fac5ipc']/100.0 +
                              values['traillen4']*values['fac4ipc']/100.0 +
                              values['traillen3']*values['fac3ipc']/100.0 +
                              values['traillen2'] +
                              values['traillen1'] +
                              values['blurlen1']) * borderfactor ) + 1 - values['border'],
    ))

    innercmd = shlex.split("""
    -bordercolor none -background none -splice 0x{topborderadd}+0+0 -border {border}
    -channel RGB +level-colors \#400000, -channel ALL
    \( +clone -channel RGB +level-colors \#a00000, -channel ALL -blur 0x{blurlen0} \)
    \( +clone -channel RGB +level-colors \#ff2000, -channel ALL -blur 0x{blurlen1} -motion-blur 0x{traillen1}+120
       -rotate 90 -wave -{wave1a},{wave1l} -rotate -90 \)
    \( -clone 0--1 -layers merge -channel RGB -channel ALL -motion-blur 0x{traillen2}+110
       -rotate 90 -wave {wave2a},{wave2l} -rotate -90 \)
    \( +clone -modulate 125,80,115.0 -channel A +level 0%,90% -channel ALL
       -resize {fac3pc}% -motion-blur 0x{traillen3}+100 -resize {fac3ipc}%
       -rotate 90 -roll +{rolllen3}+0 -wave {wave3a},{wave3l} -roll -{rolllen3}+0 -rotate -90 \)
    \( +clone -modulate 115,65,103.0 -channel A +level 0%,75% -channel ALL
       -resize {fac4pc}% -motion-blur 0x{traillen4}+90 -resize {fac4ipc}%
       -rotate 90 -roll +{rolllen4}+0 -wave {wave4a},{wave4l} -roll -{rolllen4}+0 -rotate -90 \)
    \( +clone -modulate 45,65,100.0 -channel A +level 0%,50% -channel ALL
       -resize {fac5pc}% -motion-blur 0x{traillen5}+90 -resize {fac5ipc}% """ # -geometry +0-{shiftuplen5}
    """
       -rotate 90 -roll +{rolllen5}+0 -wave {wave5a},{wave5l} -roll -{rolllen5}+0 -rotate -90 \)
    -layers merge """.format(**values))

else:

    raise ValueError("Unknown format: '{}'".format(format))


print("Running:  {convert} {inputfile} {innercmd} {inputfile}".format(
    convert=convertexe, inputfile=inputfile, innercmd=" ".join([shellquote(x) for x in innercmd])))

subprocess.check_output([ convertexe, inputfile ] + innercmd + [ inputfile ])
