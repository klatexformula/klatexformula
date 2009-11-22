######################################################################
# QMake Project File for main klatexformula executable
######################################################################
# $Id$
######################################################################

include(../VERSION.pri)

# Warning: make install works only under Unix/Linux.
INSTALLPREFIX=$$[KLF_INSTALLPREFIX]
CP=cp
ICONTHEME=$$[KLF_ICONTHEME]

# ---

TEMPLATE = app
TARGET = klatexformula
DESTDIR = ..
DEPENDPATH += . klfbackend
win32:POST_TARGETDEPS += release/libklfsrc.a
!win32:POST_TARGETDEPS += libklfsrc.a
INCLUDEPATH += . klfbackend
CONFIG += qt release
QT = core gui xml
!win32 {
	QT += dbus
	DEFINES += KLF_USE_DBUS
}

DEFINES += KLFBACKEND_QT4 KLF_VERSION_STRING=\\\"$$VERSION\\\"

!macx:LIBS += -Wl,-export-dynamic
LIBS += -L. -Lrelease -lklfsrc  -Lklfbackend -Lklfbackend/release -lklfbackend

SOURCES += main.cpp

RESOURCES += klfres.qrc

win32: PLUGINSRESFILE = plugins/klfbaseplugins_win.qrc
unix:!macx: PLUGINSRESFILE = plugins/klfbaseplugins_unix.qrc
macx: PLUGINSRESFILE = plugins/klfbaseplugins_mac.qrc

# For Mac OS X
ICON = klficon.icns

# For Windows
RC_FILE = klatexformula.rc

#TRANSLATIONS += i18n/klf_fr.ts

# INSTALLS are UNIX-only.anyway
unix {
  klfcmdl.extra = ln -sf \"klatexformula\" \"$(INSTALL_ROOT)$$INSTALLPREFIX/bin/klatexformula_cmdl\"
  klfcmdl.path = $$INSTALLPREFIX/bin
  target.path = $$INSTALLPREFIX/bin
  desktopfile.files = klatexformula.desktop
  desktopfile.path = $$INSTALLPREFIX/share/applications
  icon16.extra = install -m 644 -p  hi16-app-klatexformula.png "$(INSTALL_ROOT)$$INSTALLPREFIX/share/icons/$$ICONTHEME/16x16/apps/klatexformula.png"
  icon16.path = $$INSTALLPREFIX/share/icons/$$ICONTHEME/16x16/apps
  icon32.extra = install -m 644 -p  hi32-app-klatexformula.png "$(INSTALL_ROOT)$$INSTALLPREFIX/share/icons/$$ICONTHEME/32x32/apps/klatexformula.png"
  icon32.path = $$INSTALLPREFIX/share/icons/$$ICONTHEME/32x32/apps
  icon64.extra = install -m 644 -p  hi64-app-klatexformula.png "$(INSTALL_ROOT)$$INSTALLPREFIX/share/icons/$$ICONTHEME/64x64/apps/klatexformula.png"
  icon64.path = $$INSTALLPREFIX/share/icons/$$ICONTHEME/64x64/apps
  icon128.extra = install -m 644 -p hi128-app-klatexformula.png "$(INSTALL_ROOT)$$INSTALLPREFIX/share/icons/$$ICONTHEME/128x128/apps/klatexformula.png"
  icon128.path = $$INSTALLPREFIX/share/icons/$$ICONTHEME/128x128/apps
  baseplugins.extra = $$[KLF_RCC] -binary -o plugins/klfbaseplugins.rcc $$PLUGINSRESFILE && install -m 644 -p plugins/klfbaseplugins.rcc "$(INSTALL_ROOT)$$INSTALLPREFIX/share/klatexformula/rccresources/klfbaseplugins.rcc"
  baseplugins.path = $$INSTALLPREFIX/share/klatexformula/rccresources
  INSTALLS += target klfcmdl desktopfile icon16 icon32 icon64 icon128 baseplugins
}
