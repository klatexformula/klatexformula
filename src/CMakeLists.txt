# ######################################## #
# CMake project file for klatexformula/src #
# ######################################## #
# $Id$
# ######################################## #

if(APPLE)
  set(KLF_MACOSXDIR "${CMAKE_CURRENT_SOURCE_DIR}/macosx")
  include(macosx/KLFMacBundle.cmake)
endif(APPLE)

# Include what we need from Qt (only core+gui, or xml+sql(+dbus) too,
# depending on whether we're compiling klflib also or not)
if(KLF_BUILD_GUI)
  # we need xml + sql too, and maybe dbus
  set(QT_USE_QTXML TRUE)
  set(QT_USE_QTSQL TRUE)
  if(KLF_USE_DBUS)
    set(QT_USE_QTDBUS TRUE)
  endif(KLF_USE_DBUS)
endif(KLF_BUILD_GUI)
# include our Qt4 stuff; Qt3 needs nothing.
if(KLF_USE_QT4)
include(${QT_USE_FILE})
endif(KLF_USE_QT4)


# Preprocessor instructions
add_definitions(-DKLF_VERSION_STRING="${KLF_VERSION}" -DKLF_SRC_BUILD)
if(KLF_USE_QT4)
  add_definitions(-DKLFBACKEND_QT4)
endif(KLF_USE_QT4)
if(CMAKE_DEBUG)
  list(APPEND CMAKE_CXX_FLAGS_DEBUG -DKLF_DEBUG_TIME_PRINT)
endif(CMAKE_DEBUG)

# include SUBDIR: our klfbackend
add_subdirectory(klfbackend)

if(NOT KLF_BUILD_GUI)
  RETURN()
endif(NOT KLF_BUILD_GUI)


# Sources
set(klatexformula_SRCS
	klfcolorchooser.cpp
	klfconfig.cpp
	klfdata.cpp
	klfpobj.cpp
	klflatexsyntaxhighlighter.cpp
	klflatexsymbols.cpp
	klflibrary.cpp
	klflib.cpp
	klflibdbengine.cpp
	klflibview.cpp
	klflibentryeditor.cpp
	klflibbrowser.cpp
	klfmime.cpp
	klfmainwin.cpp
	klfpathchooser.cpp
	klfsettings.cpp
	klfstylemanager.cpp
	qtcolortriangle.cpp
	klfmain.cpp
	klfdisplaylabel.cpp
)
# Headers
set(klatexformula_MOCHEADERS
	klfcolorchooser.h
	klflatexsyntaxhighlighter.h
	klflatexsymbols.h
	klflibrary.h
	klflib.h
	klflibdbengine.h
	klflibdbengine_p.h
	klflibview.h
	klflibview_p.h
	klflibentryeditor.h
	klflibbrowser.h
	klflibbrowser_p.h
	klfmime.h
	klfmainwin.h
	klfpathchooser.h
	klfsettings.h
	klfstylemanager.h
	qtcolortriangle.h
	klfdisplaylabel.h
)
set(klatexformula_HEADERS
	klfmain.h
	klfconfig.h
	klfdata.h
	klfpobj.h
	klflib_p.h
	klfpluginiface.h
	${klatexformula_MOCHEADERS}
)
# UI Forms
set(klatexformula_UIS
	klflatexsymbols.ui
	klflibrarybrowserui.ui
	klflibbrowser.ui
	klflibentryeditor.ui
	klflibrespropeditor.ui
	klfmainwinui.ui
	klfprogerrui.ui
	klfsettingsui.ui
	klfstylemanagerui.ui
	klfaboutdialogui.ui
	klfcolorchoosewidgetui.ui
	klfcolordialogui.ui
	klflibopenresourcedlg.ui
	klflibsqliteopenwidget.ui
)
# Translations
set(klatexformula_TSS
	i18n/klf_fr.ts
)
# Resources
set(klatexformula_QRCS
	klfres.qrc
)
# Special adjustments for certain platforms and options
# * Windows
if(WIN32)
  list(APPEND klatexformula_SRCS
	klfwinclipboard.cpp
  )
endif(WIN32)
# * DBus
if(KLF_USE_DBUS)
  add_definitions(-DKLF_USE_DBUS)
  list(APPEND klatexformula_SRCS
	klfdbus.cpp
  )
  list(APPEND klatexformula_MOCHEADERS
	klfdbus.h
  )
  list(APPEND klatexformula_HEADERS
	klfdbus.h
  )
endif(KLF_USE_DBUS)


KLFInstHeaders(klatexformula_INSTHEADERS "${klatexformula_HEADERS}")
qt4_add_translation(klatexformula_QMS ${klatexformula_TSS})
qt4_wrap_ui(klatexformula_UIS_H ${klatexformula_UIS})
qt4_wrap_cpp(klatexformula_MOC_CPPS ${klatexformula_MOCHEADERS})
include_directories(${CMAKE_CURRENT_BINARY_DIR} ${CMAKE_CURRENT_SOURCE_DIR}
  ${CMAKE_CURRENT_SOURCE_DIR}/klfbackend)
set(klatexformula_ALLSRC ${klatexformula_SRCS} ${klatexformula_UIS_H}
			${klatexformula_MOC_CPPS})
# klf library
if(KLF_KLFLIB_STATIC)
  add_library(klflib STATIC ${klatexformula_ALLSRC})
else(KLF_KLFLIB_STATIC)
  add_library(klflib SHARED ${klatexformula_ALLSRC})
  if(APPLE)
    KLFMakeFramework(klflib ${klatexformula_INSTHEADERS})
  endif(APPLE)
  set_target_properties(klflib PROPERTIES
	VERSION ${KLF_VERSION}
	SOVERSION ${KLF_VERSION}
  )
endif(KLF_KLFLIB_STATIC)
add_dependencies(klflib klfbackend)
target_link_libraries(klflib ${QT_LIBRARIES} klfbackend)
# KLatexFormula Executable
qt4_add_resources(klatexformula_QRC_CPPS ${klatexformula_QRCS})
add_executable(klatexformula main.cpp ${klatexformula_QRC_CPPS})
target_link_libraries(klatexformula ${QT_LIBRARIES} klflib klfbackend)
# Set up the application bundle under Mac OS X
if(APPLE)
  set_target_properties(klatexformula PROPERTIES
	MACOSX_BUNDLE	true
	MACOSX_BUNDLE_ICON_FILE	klficon.icns
	OUTPUT_NAME	klatexformula-${KLF_VERSION}
  )
  set(KLF_MACOSX_BUNDLE_PATH "${CMAKE_CURRENT_BINARY_DIR}/klatexformula-${KLF_VERSION}.app")
  KLFMakeBundle(klatexformula klatexformula-${KLF_VERSION}.app)
endif(APPLE)



# Build Plugins
add_subdirectory(plugins)


configure_file(
  "${CMAKE_CURRENT_SOURCE_DIR}/klatexformula.desktop.in"
  "${CMAKE_CURRENT_BINARY_DIR}/klatexformula.desktop"
  IMMEDIATE @ONLY)

install(TARGETS klatexformula RUNTIME DESTINATION bin
			      BUNDLE DESTINATION bin)
if (KLF_INSTALL_DESKTOP)
install(FILES "${CMAKE_CURRENT_BINARY_DIR}/klatexformula.desktop" DESTINATION share/applications/)
install(FILES klatexformula-128.png DESTINATION share/pixmaps/)
install(FILES klatexformula-64.png DESTINATION share/pixmaps/)
install(FILES klatexformula-32.png DESTINATION share/pixmaps/)
install(FILES klatexformula-16.png DESTINATION share/pixmaps/)
install(FILES x-klatexformula.xml x-klatexformula-db.xml
	DESTINATION share/mime/packages/)
install(CODE
  "message(STATUS \"Updating Mime Type Database\")
   execute_process(COMMAND \"update-mime-database ${CMADE_INSTALL_PREFIX}/share/mime/\"
    )")
endif (KLF_INSTALL_DESKTOP)



# ............. TODO: SYMLINK klatexformula_cmdl .......................

