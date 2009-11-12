######################################################################
# QMake Project File for SkinPlugin KLatexFormula Plugin
######################################################################
# $Id$
######################################################################

TEMPLATE = lib
CONFIG += plugin qt release dll
QT += gui core xml
DEFINES += KLFBACKEND_QT4
DEPENDPATH += .
INCLUDEPATH += . ../.. ../../klfbackend
DESTDIR = ..


#TRANSLATIONS += ../../i18n/klf_fr.ts

# Input
HEADERS += skin.h
SOURCES += skin.cpp
FORMS += skinconfigwidget.ui
