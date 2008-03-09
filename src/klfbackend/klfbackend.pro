######################################################################
# Qt Project file for KLFBackend.
######################################################################
# Note: this project file is compatible both Qt3 and Qt4. See variable
# USE_QT4 in configurable section below.

# ------------------------------------------------------
# Easily accessible settings - change these if you wish

# Installation directory. Library will be installed in
# $$INSTALLDIR/lib, headers in $$INSTALLDIR/include
INSTALLDIR = /usr/local

# Are we building a static or a shared library ? (comment '#' the wrong one)
LIBRARY_TYPE = staticlib
#LIBRARY_TYPE = dll

# Are we using Qt4? Please set here to "true" if you're compiling with
# Qt >= 4.0 , leave to "false" if Qt is 3.x
USE_QT4 = false

# ------------------------------------------------------


######################################################################


# ------------------------------------------------------
message("")
message("")
contains(USE_QT4, false) {
  message("We are compiling KLFBackend with QT3, in directory")
}
contains(USE_QT4, true) {
  message("We are compiling KLFBackend with QT4, in directory")
}
message("$$INSTALLDIR    [libraries $$INSTALLDIR/lib, includes $$INSTALLDIR/include]")
contains(LIBRARY_TYPE, dll) {
  message("We are building a SHARED library")
}
contains(LIBRARY_TYPE, staticlib) {
  message("We are building a STATIC library")
}
message("")
message("")
message("If you wish to change these settings, edit the beginning of file klfbackend.pro.")
message("Otherwise, you may proceed with compilation. Type 'make'.")
message("")

# ------------------------------------------------------
# Change the following ONLY IF YOU KNOW WHAT YOU ARE DOING.

TEMPLATE = lib
INCLUDEPATH += .
CONFIG += qt warn_on release $$LIBRARY_TYPE
DESTDIR = .
VERSION = 2.1.0alpha2
contains(USE_QT4, false) {
  TARGET = klfbackend-qt3
}
contains(USE_QT4, true) {
  TARGET = klfbackend-qt4
  DEFINES += KLFBACKEND_QT4
}

HEADERS += klfbackend.h klfblockprocess.h
SOURCES += klfbackend.cpp klfblockprocess.cpp

target.path = $$INSTALLDIR/lib
includes.path = $$INSTALLDIR/include
includes.files = $$HEADERS
INSTALLS += target includes

