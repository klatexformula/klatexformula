# #################################### #
# CMake project file for klatexformula #
# #################################### #
# $Id$
# #################################### #

PROJECT(klatexformula)

cmake_minimum_required(VERSION 2.6)

# Qt stuff
include(FindQt4)
include(${QT_USE_FILE})

set(klatexformula_SRCS
	...................
)
set(klatexformula_UIS
	...................
)
set(klatexformula_QRCS
	...................
)
qt4_add_resources(klatexformula_QRC_CPPS ${klatexformula_QRCS})
qt4_wrap_ui(klatexformula_UIS_H ${klatexformula_UIS})
qt4_automoc(${klatexformula_SRCS})
include_directories(${CMAKE_CURRENT_BINARY_DIR} ${CMAKE_CURRENT_SOURCE_DIR})
add_executable(klatexformula ${klatexformula_SRCS} ${klatexformula_UIS_H} ${klatexformula_QRC_CPPS})
target_link_libraries(klatexformula ${QT_LIBRARIES})

configure_file(
  "${CMAKE_CURRENT_SOURCE_DIR}/klatexformula.desktop.in"
  "${CMAKE_CURRENT_BINARY_DIR}/klatexformula.desktop"
  IMMEDIATE @ONLY)

install(TARGETS klatexformula RUNTIME DESTINATION bin)
install(FILES "${CMAKE_CURRENT_BINARY_DIR}/klatexformula.desktop" DESTINATION share/applications/)
#install(FILES klatexformula-64.png DESTINATION share/pixmaps/)

CONFIGURE_FILE(
  "${CMAKE_CURRENT_SOURCE_DIR}/cmake_uninstall.cmake.in"
  "${CMAKE_CURRENT_BINARY_DIR}/cmake_uninstall.cmake"
  IMMEDIATE @ONLY)

ADD_CUSTOM_TARGET(uninstall
  "${CMAKE_COMMAND}" -P "${CMAKE_CURRENT_BINARY_DIR}/cmake_uninstall.cmake")










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
CONFIG += qt $$SHAREDORSTATIC debug
# release
QT = core gui xml sql
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
	   klfpobj.h \
	   klflatexsyntaxhighlighter.h \
           klflatexsymbols.h \
           klflibrary.h \
	   klflib.h \
	   klflib_p.h \
	   klflibview.h \
	   klflibview_p.h \
	   klflibentryeditor.h \
	   klflibbrowser.h \
	   klflibbrowser_p.h \
	   klfmime.h \
           klfmainwin.h \
           klfpathchooser.h \
           klfsettings.h \
           klfstylemanager.h \
	   klfpluginiface.h \
	   qtcolortriangle.h \
	   klfmain.h \
	   klfdisplaylabel.h
!win32 {
	HEADERS +=	klfdbus.h
}
FORMS += klflatexsymbolsui.ui \
         klflibrarybrowserui.ui \
	 klflibbrowser.ui \
	 klflibentryeditor.ui \
	 klflibrespropeditor.ui \
         klfmainwinui.ui \
         klfprogerrui.ui \
         klfsettingsui.ui \
         klfstylemanagerui.ui \
	 klfaboutdialogui.ui \
	 klfcolorchoosewidgetui.ui \
	 klfcolordialogui.ui \
	 klflibopenresourcedlg.ui \
	 klflibsqliteopenwidget.ui
SOURCES += klfcolorchooser.cpp \
           klfconfig.cpp \
           klfdata.cpp \
	   klfpobj.cpp \
	   klflatexsyntaxhighlighter.cpp \
           klflatexsymbols.cpp \
           klflibrary.cpp \
	   klflib.cpp \
	   klflibview.cpp \
	   klflibentryeditor.cpp \
	   klflibbrowser.cpp \
	   klfmime.cpp \
           klfmainwin.cpp \
           klfpathchooser.cpp \
           klfsettings.cpp \
           klfstylemanager.cpp \
	   qtcolortriangle.cpp \
	   klfmain.cpp \
	   klfdisplaylabel.cpp
win32 {
	SOURCES +=      klfwinclipboard.cpp
}
!win32 {
	SOURCES +=	klfdbus.cpp
}

#TRANSLATIONS += i18n/klf_fr.ts
