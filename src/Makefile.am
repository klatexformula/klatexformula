# set the include path for X, qt and KDE
INCLUDES = $(all_includes)

# these are the headers for your project
noinst_HEADERS = klatexformula.h

# let automoc handle all of the meta source files (moc)
METASOURCES = AUTO

messages: rc.cpp
	$(EXTRACTRC) `find . -name \*.ui -o -name \*.rc` > rc.cpp
	$(XGETTEXT) *.cpp -o $(podir)/klatexformula.pot

KDE_ICON = AUTO

#########################################################################
# APPLICATION SECTION
#########################################################################
# this is the program that gets installed.  it's name is used for all
# of the other Makefile.am variables
bin_PROGRAMS = klatexformula

# the application source, library search path, and link libraries
klatexformula_SOURCES = main.cpp klatexformula.cpp
klatexformula_LDFLAGS = $(KDE_RPATH) $(all_libraries)
klatexformula_LDADD = $(LIB_KDEUI)

# this is where the desktop file will go 
shelldesktopdir   = $(kde_appsdir)/Utilities
shelldesktop_DATA = klatexformula.desktop

# this is where the shell's XML-GUI resource file goes
shellrcdir   = $(kde_datadir)/klatexformula
shellrc_DATA = klatexformulaui.rc

