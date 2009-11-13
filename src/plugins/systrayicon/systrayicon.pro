######################################################################
# QMake Project File for SysTrayIcon KLatexFormula Plugin
######################################################################
# $Id$
######################################################################

TEMPLATE = lib
CONFIG += plugin qt release
QT += gui core xml
DEFINES += KLFBACKEND_QT4
DEPENDPATH += .
INCLUDEPATH += . ../.. ../../klfbackend
DESTDIR = ..

win32:LIBS += -L../../release -L../.. -lklfsrc -L../../klfbackend/release -L../../klfbackend -lklfbackend


#TRANSLATIONS += ../../i18n/klf_fr.ts

# Input
HEADERS += systrayicon.h
SOURCES += systrayicon.cpp
FORMS += systrayiconconfigwidget.ui systraymainiconifybuttons.ui

