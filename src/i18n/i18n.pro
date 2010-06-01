
# This project file is meant for use with lupdate only. Don't attempt
# to parse it with qmake.

TEMPLATE = no_template

SOURCES = ../*.cpp ../klfbackend/*.cpp ../plugins/skin/*.cpp ../plugins/systrayicon/*.cpp obsolete/*.cpp
HEADERS = ../*.h   ../klfbackend/*.h   ../plugins/skin/*.h   ../plugins/systrayicon/*.h obsolete/*.h
FORMS   = ../*.ui  ../klfbackend/*.ui  ../plugins/skin/*.ui  ../plugins/systrayicon/*.ui obsolete/*.ui

TRANSLATIONS = klf_fr.ts

