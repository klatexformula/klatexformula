######################################################################
# Qt Project file for KLFBackend.
######################################################################
# Note: this project file is compatible both Qt3 and Qt4. See variable
# USE_QT4 in configurable section below.

# ------------------------------------------------------
# Easily accessible settings - change these if you wish

# Installation directory. Library will be installed in
# $$INSTALLPREFIX/lib, headers in $$INSTALLPREFIX/include
INSTALLPREFIX = /usr

# Are we building a static or a shared library ? (comment '#' the wrong one)
LIBRARY_TYPE = staticlib
#LIBRARY_TYPE = dll

# Are we using Qt4? Please leave this setting to "true" if you're compiling with
# Qt 4 , or set to "false" if you're using Qt 3.x
USE_QT4 = true

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
message("$$INSTALLPREFIX    [libraries $$INSTALLPREFIX/lib, includes $$INSTALLPREFIX/include]")
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
VERSION = 3.0.0
contains(USE_QT4, false) {
  TARGET = klfbackend-qt3
}
contains(USE_QT4, true) {
  TARGET = klfbackend
  DEFINES += KLFBACKEND_QT4
}

HEADERS += klfbackend.h klfblockprocess.h
SOURCES += klfbackend.cpp klfblockprocess.cpp

target.path = $$INSTALLPREFIX/lib
includes.path = $$INSTALLPREFIX/include
includes.files = $$HEADERS
INSTALLS += target includes

