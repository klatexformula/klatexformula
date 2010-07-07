

KLFPathChooser::KLFPathChooser()
{
  tr("Browse");
}


KLFAboutDialogUI::KLFAboutDialogUI()
{
  tr("<!DOCTYPE HTML PUBLIC \"-//W3C//DTD HTML 4.0//EN\" \"http://www.w3.org/TR/REC-html40/strict.dtd\">\n"
     "<html><head><meta name=\"qrichtext\" content=\"1\" /><style type=\"text/css\">\n"
     "p, li { white-space: pre-wrap; }\n"
     "</style></head><body style=\"\">\n"
     "<p style=\" margin-top:0px; margin-bottom:0px; margin-left:0px; margin-right:0px; -qt-block-indent:0; text-indent:0px;\"><span style=\" font-weight:600;\">About KLatexFormula</span></p>\n"
     "<p align=\"justify\" style=\" margin-top:12px; margin-bottom:12px; margin-left:0px; margin-right:0px; -qt-block-indent:0; text-indent:0px;\">KLatexFormula was written by Philippe Faist, currently (2010) studying physics in Switzerland. First released in 2006 written in KDE3 environment, the program has seen many improvements and restructuring since then, leading to the current version, written in Qt 4.</p>\n"
     "<p style=\" margin-top:12px; margin-bottom:12px; margin-left:0px; margin-right:0px; -qt-block-indent:0; text-indent:0px;\"><span style=\" font-weight:600;\">Author</span></p>\n"
     "<p style=\" margin-top:12px; margin-bottom:12px; margin-left:0px; margin-right:0px; -qt-block-indent:0; text-indent:0px;\">Philippe Faist, philippe.faist@bluewin.ch</p>\n"
     "<p style=\" margin-top:12px; margin-bottom:12px; margin-left:0px; margin-right:0px; -qt-block-indent:0; text-indent:0px;\"><span style=\" font-weight:600;\">KLatexFormula On the Web</span></p>\n"
     "<p style=\" margin-top:12px; margin-bottom:12px; margin-left:0px; margin-right:0px; -qt-block-indent:0; text-indent:0px;\"><a href=\"http://klatexformula.sourceforge.net/\"><span style=\" text-decoration: underline; color:#0000bf;\">Home Page at http://klatexformula.sourceforge.net/</span></a></p>\n"
     "<p style=\" margin-top:12px; margin-bottom:12px; margin-left:0px; margin-right:0px; -qt-block-indent:0; text-indent:0px;\"><a href=\"http://klatexformula.sourceforge.net/doc/\"><span style=\" text-decoration: underline; color:#0000bf;\">Documentation at http://klatexformula.sourceforge.net/doc/</span></a></p></body></html>");

  tr("<!DOCTYPE HTML PUBLIC \"-//W3C//DTD HTML 4.0//EN\" \"http://www.w3.org/TR/REC-html40/strict.dtd\">\n<html><head><meta name=\"qrichtext\" content=\"1\" /><style type=\"text/css\">\np, li { white-space: pre-wrap; }\n</style></head><body style=\"font-weight:400; font-style:normal;\">\n<p style=\" margin-top:12px; margin-bottom:12px; margin-left:0px; margin-right:0px; -qt-block-indent:0; text-indent:0px; font-size:large; font-weight:600;\"><span style=\" font-size:large;\">About KLatexFormula</span></p>\n<p align=\"justify\" style=\" margin-top:12px; margin-bottom:12px; margin-left:0px; margin-right:0px; -qt-block-indent:0; text-indent:0px;\">KLatexFormula was written by Philippe Faist, currently (2009) studying physics in Switzerland. First released in 2006 written in KDE3 environment, the program has seen many improvements and restructuring since then, leading to the current Qt 4 version.</p>\n<p style=\" margin-top:12px; margin-bottom:12px; margin-left:0px; margin-right:0px; -qt-block-indent:0; text-indent:0px; font-weight:600;\">Author</p>\n<p style=\" margin-top:12px; margin-bottom:12px; margin-left:0px; margin-right:0px; -qt-block-indent:0; text-indent:0px;\">Philippe Faist, philippe.faist@bluewin.ch</p>\n<p style=\" margin-top:12px; margin-bottom:12px; margin-left:0px; margin-right:0px; -qt-block-indent:0; text-indent:0px; font-weight:600;\">KLatexFormula On the Web</p>\n<p style=\" margin-top:12px; margin-bottom:12px; margin-left:0px; margin-right:0px; -qt-block-indent:0; text-indent:0px;\"><a href=\"http://klatexformula.sourceforge.net/\"><span style=\" text-decoration: underline; color:#0000bf;\">http://klatexformula.sourceforge.net/</span></a></p></body></html>");
}


KLFLatexSymbols::KLFLatexSymbols()
{
  tr("Warning: failed to open file `%1'!");
  tr("Warning: KLFLatexSymbols: error reading cache file ! code=%1");
}

KLFLibraryListManager::KLFLibraryListManager()
{
  tr("Are you sure you want to delete %1 selected item(s) from library?")
}

KLFMainWin::KLFMainWin()
{
  tr("Can't install klatexformula.cls to temporary directory !");
  tr("History");
  tr("Archive");
  tr("Unable to open library file!");
  tr("Error: Library file is incorrect or corrupt!\n");
  tr("Load Library");
  tr("The library file found was created by a more recent version of KLatexFormula.\nThe process of library loading may fail.");
  tr("Unable to load your formula history list!");
  tr("Error: History file is incorrect or corrupt!\n");
  tr("Load History");
  tr("The history file found was created by a more recent version of KLatexFormula.\nThe process of history loading may fail.");
  tr("Error: Unable to write to library file `%1'!");
  tr("Failed open for ToolTip Temp Image!\n%1");
  tr("Failed write to ToolTip Temp Image file %1!");
  tr("Sorry, format `%1' is not available.");
  tr("Error: Can't write to file %1!");
  tr("Created with KLatexFormula version %1");
}

class KLFMainWinUI {
  KLFMainWinUI()
  {
    tr("KLatexFormula", "window title");
    tr("Evaluate LaTeX Expression [Shift-Enter]", "tooltip");
    tr("Show Latex Symbols palette", "tooltip");
    tr("Toggle shrinked/expanded mode", "tooltip");
  }
};

KLFSettings::KLFSettings()
{
  tr("Cancel");
  tr("Apply");
  tr("OK");
  tr("English Default", "[[first item of language graphical choice box]]");
  tr("%1 (%2)", "[[%1=Language (%2=Country)]]");
  tr("%1", "[[%1=Language, no country is specified]]");
  tr("Could not find `%1' executable !");
  tr("Please check your installation and specify the path to `%1' executable manually if it is not installed in $PATH.");
  tr("You need to restart KLatexFormula for your new language settings to take effect.");
  tr("Main editor font sample - click to change");
  tr("Preamble editor font sample - click to change");
  tr("Preview Size");
  tr("Maximum Size of Preview Tooltip on LaTeX formula display");
  tr("System E&xecutables and paths");
  tr("A&dvanced settings");
  tr("I  ");
}

void dummyfunction()
{
  QObject::tr("Error");
  QObject::tr("Can't create local directory `%1' !");
  QObject::tr("Can't make local config directory `%1' !");
  QObject::tr("Cancel");
  QObject::tr("Warning: Ignoring --input when --latexinput is given.\n");
  QObject::tr("Error: Can't read standard input (!)\n");
  QObject::tr("Error: Can't read input file `%1'.\n");
  QObject::tr("Unable to open stderr for write! Error: %1\n");
  QObject::tr("Unable to write to file `%1'! Error: %2\n");
  QObject::tr("PDF format is not available!\n");
  QObject::tr("Unable to save image to file `%1' in format `%2'!\n");
  QObject::tr("Warning: Ignoring --input since --latexinput is given.\n");
  QObject::tr("Unable to start Latex program!", "KLFBackend");
  QObject::tr("Unable to save image to file `%1' in format `%2'!\n", "KLFBackend::saveOutputToFile");

}


QT_TRANSLATE_NOOP3("QObject",
		     "\n"
		     "KLatexFormula by Philippe Faist\n"
		     "\n"
		     "Usage: klatexformula\n"
		     "           Opens klatexformula Graphical User Interface (GUI)\n"
		     "       klatexformula [OPTIONS]\n"
		     "           Performs actions required by [OPTIONS], and exits\n"
		     "       klatexformula --interactive|-I [OPTIONS]\n"
		     "           Opens the GUI and performs actions required by [OPTIONS]\n"
		     "       klatexformula filename1.klf [ filename2.klf ... ]\n"
		     "           Opens the GUI and imports the given .klf file(s) into the library.\n"
		     "\n"
		     "       If additional filename arguments are passed to the command line, they are\n"
		     "       interpreted as .klf files to load into the library in separate resources\n"
		     "       (only in interactive mode).\n"
		     "\n"
		     "OPTIONS may be one or many of:\n"
		     "  -I|--interactive\n"
		     "      Runs KLatexFormula in interactive mode with a full-featured graphical user\n"
		     "      interface. This option is on by default, except if --input or --latexinput\n"
		     "      is given.\n"
		     "  -i|--input <file|->\n"
		     "      Specifies a file to read latex input from.\n"
		     "  -l|--latexinput <expr>\n"
		     "      Specifies the LaTeX code of an equation to render.\n"
		     "  -o|--output <file|->\n"
		     "      Specifies to write the output image (obtained from equation given by --input\n"
		     "      or --latexinput) to <file> or standard output.\n"
		     "  -F|--format <format>\n"
		     "      Specifies the format the output should be written in. By default, the format\n"
		     "      is guessed from file name extention and defaults to PNG.\n"
		     "  -f|--fgcolor <'#xxxxxx'>\n"
		     "      Specifies a color (in web #RRGGBB hex format) to use for foreground color.\n"
		     "      Don't forget to escape the '#' to prevent the shell from interpreting it as\n"
		     "      a comment.\n"
		     "  -b|--bgcolor <-|'#xxxxxx'>\n"
		     "      Specifies a color (in web #RRGGBB hex format, or '-' for transparent) to\n"
		     "      use as background color (defaults to transparent)\n"
		     "  -X|--dpi <N>\n"
		     "      Use N dots per inch (DPI) when converting latex output to image. Defaults to\n"
		     "      1200 (high-resolution image).\n"
		     "  -m|--mathmode <expression containing '...'>\n"
		     "      Specifies which LaTeX math mode to use, if any. The argument to this option\n"
		     "      is any string containing \"...\", which will be replaced by the equation\n"
		     "      itself. Defaults to \"\\[ ... \\]\"\n"
		     "  -p|--preamble <LaTeX code>\n"
		     "      Any LaTeX code that will be inserted before \\begin{document}. Useful for\n"
		     "      including custom packages with \\usepackage{...}.\n"
		     "  -q|--quiet\n"
		     "      Disable console output of warnings and errors.\n"
		     "\n"
		     "  --lborderoffset <N>\n"
		     "  --tborderoffset <N>\n"
		     "  --rborderoffset <N>\n"
		     "  --bborderoffset <N>\n"
		     "      Include a margin of N postscript points on left, top, right, or bottom margin\n"
		     "      respectively.\n"
		     "  --tempdir </path/to/temp/dir>\n"
		     "      Specify the directory in which KLatexFormula will write temporary files.\n"
		     "      Defaults to a system-specific temporary directory like \"/tmp/\".\n"
		     "  --latex <latex executable>\n"
		     "  --dvips <dvips executable>\n"
		     "  --gs <gs executable>\n"
		     "  --epstopdf <epstopdf executable>\n"
		     "      Specifiy the executable for latex, dvips, gs or epstopdf. By default, they\n"
		     "      are searched for in $PATH and/or in common system directories.\n"
		     "\n"
		     "  -h|--help\n"
		     "      Display this help text and exit.\n"
		     "  -V|--version\n"
		     "      Display KLatexFormula version information and exit.\n"
		     "\n"
		     "  -Q|--qtoption <qt-option>\n"
		     "      Specify a Qt-Specific option. For example, to launch KLatexFormula in\n"
		     "      Plastique GUI style, use:\n"
		     "        klatexformula  --qtoption='-style=Plastique'\n"
		     "      Note that if <qt-option> begins with a '-', then it must be appended to the\n"
		     "      long '--qtoption=' syntax with the equal sign.\n"
		     "\n"
		     "\n"
		     "NOTES\n"
		     "  * When run in interactive mode, the newly evaluated equation is appended to\n"
		     "    KLatexFormula's history.\n"
		     "  * When not run in interactive mode, no X11 server is needed.\n"
		     "  * Additional translation files and/or data can be provided to klatexformula by specifying\n"
		     "    a list of Qt rcc files or directories containing such files to import in the KLF_RESOURCES\n"
		     "    environment variable. Separate the file names with ':' on unix/mac, or ';' on windows. The\n"
		     "    default paths can be included with an empty section ('::') or leading or trailing ':'.\n"
		     "  * Please report any bugs and malfunctions to the author.\n"
		     "\n"
		     "Have a lot of fun!\n"
		     "\n",
		   "Command-line help instructions") ;
