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

# Input
HEADERS += systrayicon.h
SOURCES += systrayicon.cpp
FORMS += systrayiconconfigwidget.ui

