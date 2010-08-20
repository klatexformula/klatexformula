
# This project file is meant for use with lupdate only, for translations. Don't attempt
# to parse it with qmake.

TEMPLATE = no_template

SOURCES = ../*.cpp ../klftools/*.cpp ../klfbackend/*.cpp ../plugins/skin/*.cpp ../plugins/systrayicon/*.cpp obsolete/*.cpp
HEADERS = ../*.h   ../klftools/*.h   ../klfbackend/*.h   ../plugins/skin/*.h   ../plugins/systrayicon/*.h obsolete/*.h
FORMS   = ../*.ui  ../klftools/*.ui  ../klfbackend/*.ui  ../plugins/skin/*.ui  ../plugins/systrayicon/*.ui obsolete/*.ui

TRANSLATIONS = klf_fr.ts

