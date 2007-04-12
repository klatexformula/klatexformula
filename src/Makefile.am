# set the include path for X, qt and KDE
INCLUDES = -I$(top_srcdir)/src/klfbackend $(all_includes)

# these are the headers for your project


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
bin_PROGRAMS = klatexformula klatexformula_cmdl

# the application source, library search path, and link libraries
klatexformula_SOURCES = main.cpp klfmainwinui.ui klfhistorybrowserui.ui \
	klfmainwin.cpp klfsettingsui.ui klfsettings.cpp klfprogerrui.ui klfdata.cpp \
	klfhistorybrowser.cpp klfstylemanagerui.ui klfstylemanager.cpp klflatexsymbolsui.ui \
	klflatexsymbols.cpp
klatexformula_LDFLAGS = $(KDE_RPATH) $(all_libraries)
klatexformula_LDADD = $(top_builddir)/src/klfbackend/libklfbackend.a \
	$(LIB_KDEUI) $(LIB_KIO)

# this is where the desktop file will go 
shelldesktopdir = $(kde_appsdir)/Utilities
shelldesktop_DATA = klatexformula.desktop

# our data
ourdatadir = $(kde_datadir)/klatexformula
ourdata_DATA = latex_symbols symbolspixmapcache

SUBDIRS = klfbackend pics
noinst_HEADERS = klfmainwin.h klfsettings.h klfdata.h klfhistorybrowser.h \
		klfstylemanager.h klflatexsymbols.h
klatexformula_cmdl_LDFLAGS = $(KDE_RPATH) $(all_libraries)
klatexformula_cmdl_SOURCES = klfcmdl_main.cpp
klatexformula_cmdl_LDADD = $(top_builddir)/src/klfbackend/libklfbackend.a \
	$(LIB_KDECORE)
