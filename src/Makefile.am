# set the include path for X, qt and KDE
INCLUDES = -I$(top_srcdir)/src/klfbackend $(all_includes)

# these are the headers for your project


# let automoc handle all of the meta source files (moc)
METASOURCES = AUTO

KDE_ICON = AUTO

messages: rc.cpp
	$(EXTRACTRC) `find . -name \*.ui -o -name \*.rc` > rc.cpp
	$(XGETTEXT) *.cpp -o $(podir)/klatexformula.pot

#########################################################################
# APPLICATION SECTION
#########################################################################
# this is the program that gets installed.  it's name is used for all
# of the other Makefile.am variables
if KLF_ENABLE_GUI
bin_PROGRAMS = klatexformula klatexformula_cmdl
else
bin_PROGRAMS = klatexformula_cmdl
endif
if FALSE
# this is to trick kdevelop's automake manager
bin_PROGRAMS = klatexformula klatexformula_cmdl
endif

# the application source, library search path, and link libraries
klatexformula_SOURCES = klfconfig.cpp klfdata.cpp \
	klflatexsymbols.cpp klflatexsymbolsui.ui klflibrary.cpp klflibrarybrowserui.ui \
	klfmainwin.cpp klfmainwinui.ui klfprogerrui.ui klfsettings.cpp klfsettingsui.ui \
	klfstylemanager.cpp klfstylemanagerui.ui main.cpp
klatexformula_LDFLAGS = $(KDE_RPATH) $(all_libraries)
klatexformula_LDADD = $(top_builddir)/src/klfbackend/libklfbackend.a \
	$(LIB_KDEUI) $(LIB_KIO)

noinst_HEADERS = klfconfig.h klfdata.h klflatexsymbols.h \
	klflibrary.h klfmainwin.h klfsettings.h klfstylemanager.h
klatexformula_cmdl_LDFLAGS = $(KDE_RPATH) $(all_libraries)
klatexformula_cmdl_SOURCES = klfcmdl_main.cpp
klatexformula_cmdl_LDADD = $(top_builddir)/src/klfbackend/libklfbackend.a \
	$(LIB_KDECORE)


# this is where the desktop file will go 
if KLF_ENABLE_GUI
shelldesktopdir = $(kde_appsdir)/Utilities
shelldesktop_DATA = klatexformula.desktop
endif

# our data
if KLF_ENABLE_GUI
ourdatadir = $(kde_datadir)/klatexformula
ourdata_DATA = latex_symbols symbolspixmapcache_base klatexformula.cls
else
ourdatadir = $(kde_datadir)/klatexformula
ourdata_DATA = klatexformula.cls
endif


SUBDIRS = klfbackend pics
