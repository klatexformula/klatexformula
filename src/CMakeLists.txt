# ######################################## #
# CMake project file for klatexformula/src #
# ######################################## #
# $Id$
# ######################################## #


# Include what we need from Qt (only core+gui, or xml+sql(+dbus) too,
# depending on whether we're compiling klfsrc also or not)
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
set(klatexformula_HEADERS
	klfcolorchooser.h
#	klfconfig.h
#	klfdata.h
#	klfpobj.h
	klflatexsyntaxhighlighter.h
	klflatexsymbols.h
	klflibrary.h
	klflib.h
#	klflib_p.h
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
#	klfpluginiface.h
	qtcolortriangle.h
#	klfmain.h
	klfdisplaylabel.h
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
  list(APPEND klatexformula_HEADERS
	klfdbus.h
  )
endif(KLF_USE_DBUS)

# ....... TODO ......... Add TRANSLATIONS ...........
qt4_add_translation(klatexformula_QMS ${klatexformula_TSS})
qt4_wrap_ui(klatexformula_UIS_H ${klatexformula_UIS})
qt4_wrap_cpp(klatexformula_MOC_CPPS ${klatexformula_HEADERS})
include_directories(${CMAKE_CURRENT_BINARY_DIR} ${CMAKE_CURRENT_SOURCE_DIR}
  ${CMAKE_CURRENT_SOURCE_DIR}/klfbackend)
set(klatexformula_ALLSRC ${klatexformula_SRCS} ${klatexformula_UIS_H}
			${klatexformula_MOC_CPPS})
# klfsrc library
if(KLF_SRCLIB_STATIC)
  add_library(klfsrc STATIC ${klatexformula_ALLSRC})
else(KLF_SRCLIB_STATIC)
  add_library(klfsrc SHARED ${klatexformula_ALLSRC})
endif(KLF_SRCLIB_STATIC)
add_dependencies(klfsrc klfbackend)
target_link_libraries(klfsrc ${QT_LIBRARIES} klfbackend)
# KLatexFormula Executable
qt4_add_resources(klatexformula_QRC_CPPS ${klatexformula_QRCS})
add_executable(klatexformula main.cpp ${klatexformula_QRC_CPPS})
target_link_libraries(klatexformula ${QT_LIBRARIES} klfsrc klfbackend)


# Build Plugins
add_subdirectory(plugins)


configure_file(
  "${CMAKE_CURRENT_SOURCE_DIR}/klatexformula.desktop.in"
  "${CMAKE_CURRENT_BINARY_DIR}/klatexformula.desktop"
  IMMEDIATE @ONLY)

install(TARGETS klatexformula RUNTIME DESTINATION bin)
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



