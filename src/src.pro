######################################################################
# QMake Project File for klatexformula main source library (internal)
######################################################################
# $Id$
######################################################################

include(../VERSION.pri)

# ---


SHAREDORSTATIC = $$[KLF_SRCLIB_SHAREDORSTATIC]

TEMPLATE = lib
TARGET = klfsrc
DESTDIR = .
DEPENDPATH += . klfbackend
INCLUDEPATH += . klfbackend
CONFIG += qt release $$SHAREDORSTATIC
QT = core gui xml
!win32 {
	QT += dbus
	DEFINES += KLF_USE_DBUS
}

DEFINES += KLFBACKEND_QT4 KLF_VERSION_STRING=\\\"$$VERSION\\\" KLF_SRC_BUILD KLF_DEBUG_TIME_PRINT
LIBS += -Lklfbackend -Lklfbackend/release -lklfbackend

# Input
HEADERS += klfcolorchooser.h \
           klfconfig.h \
           klfdata.h \
           klflatexsymbols.h \
           klflibrary.h \
           klfmainwin.h \
           klfpathchooser.h \
           klfsettings.h \
           klfstylemanager.h \
	   klfpluginiface.h \
	   qtcolortriangle.h \
	   klfmain.h
!win32 {
	HEADERS +=	klfdbus.h
}
FORMS += klflatexsymbolsui.ui \
         klflibrarybrowserui.ui \
         klfmainwinui.ui \
         klfprogerrui.ui \
         klfsettingsui.ui \
         klfstylemanagerui.ui \
	 klfaboutdialogui.ui \
	 klfcolorchoosewidgetui.ui \
	 klfcolordialogui.ui
SOURCES += klfcolorchooser.cpp \
           klfconfig.cpp \
           klfdata.cpp \
           klflatexsymbols.cpp \
           klflibrary.cpp \
           klfmainwin.cpp \
           klfpathchooser.cpp \
           klfsettings.cpp \
           klfstylemanager.cpp \
	   qtcolortriangle.cpp \
	   klfmain.cpp
!win32 {
	SOURCES +=	klfdbus.cpp
}

#TRANSLATIONS += i18n/klf_fr.ts
