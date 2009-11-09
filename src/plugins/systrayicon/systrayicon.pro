######################################################################
# SkinPlugin Project File
######################################################################

TEMPLATE = lib
CONFIG += plugin qt release
QT += gui core xml
LIBS += -lQtUiTools
DEFINES += KLFBACKEND_QT4
DEPENDPATH += .
INCLUDEPATH += . ../.. ../../klfbackend
DESTDIR = ..

# Input
HEADERS += systrayicon.h
SOURCES += systrayicon.cpp
FORMS += systrayiconconfigwidget.ui

