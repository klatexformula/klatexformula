######################################################################
# SkinPlugin Project File
######################################################################

TEMPLATE = lib
CONFIG += plugin qt
QT += gui core xml
LIBS += -lQtUiTools
DEFINES += KLFBACKEND_QT4
DEPENDPATH += .
INCLUDEPATH += . ../.. ../../klfbackend
TARGET = $$qtLibraryTarget(skin)
DESTDIR = ..

# Input
HEADERS += skin.h
SOURCES += skin.cpp
