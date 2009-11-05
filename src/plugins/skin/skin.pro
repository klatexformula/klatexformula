######################################################################
# SkinPlugin Project File
######################################################################

TEMPLATE = lib
CONFIG += plugin qt release
QT += gui core xml
DEFINES += KLFBACKEND_QT4
DEPENDPATH += .
INCLUDEPATH += . ../.. ../../klfbackend
DESTDIR = ..

# Input
HEADERS += skin.h
SOURCES += skin.cpp
FORMS += skinconfigwidget.ui
