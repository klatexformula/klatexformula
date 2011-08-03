#!/usr/bin/python

import re;
import os;
import sys;

if (sys.argv[1] == "--scriptinfo"):
    print "ScriptInfo";
    print "Name: FeynMF wrapper";
    print "Version: 0.1";
    print "ForceInputMathMode: \\begin{fmffile}{FMF_FEYNUSERSCRIPT_MFNAME} ... \\end{fmffile}";
    print "ForceInputPreambleLine: \\usepackage{feynmf}";
    print "";
    exit(0);


latexfname = sys.argv[1];

latexinput = os.environ["KLF_INPUT_LATEX"];

latexexe = os.environ["KLF_LATEX"];
mfexe = os.path.dirname(latexexe) + "/mf";

if (len(latexexe) == 0):
    sys.stderr.write("Error: latex executable not found.");
    exit(1);
if (len(mfexe) == 0):
    sys.stderr.write("Error: mf executable not found.");
    exit(1);

# prepare LaTeX template with \begin{fmfile}
# overwrite default-prepared template

mfbasefname = 'klffeynmf'+re.sub(r'[^a-zA-Z0-9_]', '', os.path.basename(latexfname));
print "mfbasename="+mfbasefname;

f = open(latexfname, 'w');
f.write("\\documentclass{article}\n\\usepackage{feynmf}\n\\begin{document}\n"+
        "\\thispagestyle{empty}\n"+
        "\\begin{fmffile}{" + mfbasefname + "}\n"
        + latexinput + "\n\\end{fmffile}\n\\end{document}\n");
f.close();

# now, run latex first time
print "Running "+latexexe+" "+latexfname+ "  ...";
res = os.system(latexexe + ' ' + latexfname);
#if (res != 0):
#    print "latex failed, res="+str(res);
#    exit(res>>8);
print "Running "+mfexe + " '\\mode=localfont; input "+mfbasefname+".mf;'" + "  ...";
res = os.system(mfexe + " '\\mode=localfont; input "+mfbasefname+".mf;'");
#if (res != 0):
#    print "mf failed, res="+str(res);
#    exit(res>>8);
print "Re-Running "+latexexe+" "+latexfname+ "  ...";
os.system(latexexe + ' ' + latexfname);
if (res != 0):
    print "re-latex failed, res="+str(res);
    exit(res>>8);

# remove temp file
print "rm "+mfbasefname+".mf";


exit(0);


