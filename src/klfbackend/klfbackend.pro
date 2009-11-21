######################################################################
# QMake Project file for KLFBackend.
######################################################################
# $Id$
######################################################################

# NOTE: IN KLATEXFORMULA PRIOR TO 3.1.0, THE USER COULD EDIT CONFIG IN THIS
# FILE. BEGINNING VERSION 3.1.0, ALL SETTINGS MUST BE PASSED ON TO qmake
# WHILE BUILDING ROOT PROJECT. SEE COMMENT IN FILE klatexformula.pro FOR
# MORE INFORMATION.


INSTALLPREFIX = $$[KLF_BACKEND_INSTALLPREFIX]
INSTALLLIBDIRNAME = $$[KLF_BACKEND_INSTALLLIBDIR]
LIBRARY_TYPE = $$[KLF_BACKEND_SHAREDORSTATIC]
USE_QT4 = $$[KLF_BACKEND_USE_QT4]

######################################################################

TEMPLATE = lib
INCLUDEPATH += .
CONFIG += qt warn_on release $$LIBRARY_TYPE
DESTDIR = .
include(../../VERSION.pri)
contains(USE_QT4, false) {
  TARGET = klfbackend-qt3
}
contains(USE_QT4, true) {
  TARGET = klfbackend
  DEFINES += KLFBACKEND_QT4
}
DEFINES += KLF_SRC_BUILD

HEADERS += klfbackend.h klfblockprocess.h
SOURCES += klfbackend.cpp klfblockprocess.cpp

#TRANSLATIONS += ../i18n/klf_fr.ts

includes.path = $$INSTALLPREFIX/include
includes.files = $$HEADERS
target.path = $$INSTALLLIBDIRNAME
INSTALLS += target includes

