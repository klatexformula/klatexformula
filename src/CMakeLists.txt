# ######################################## #
# CMake project file for klatexformula/src #
# ######################################## #
# $Id$
# ######################################## #

if(KLF_MACOSX_BUNDLES)
  set(KLF_MACOSXDIR "${CMAKE_CURRENT_SOURCE_DIR}/macosx")
  set(KLF_MACOSX_BUNDLE_PATH "${CMAKE_CURRENT_BINARY_DIR}/klatexformula-${KLF_VERSION}.app")
  include(KLFMacBundle)
endif(KLF_MACOSX_BUNDLES)

if(WIN32)
  # on debug versions, the application will open a console in which the debug output is shown.
  set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -mwindows")
  # Extra linker flags for win32 compilation
  set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -Wl,--enable-auto-import")
endif(WIN32)

# Preprocessor instructions
add_definitions(-DKLF_VERSION_STRING="${KLF_VERSION}"
		-DKLF_VERSION_MAJ=${KLF_VERSION_MAJ}
		-DKLF_VERSION_MIN=${KLF_VERSION_MIN}
		-DKLF_VERSION_REL=${KLF_VERSION_REL}
		-DKLF_SRC_BUILD
		-DKLF_CMAKE_ARCH="${KLF_CMAKE_ARCH}")
if(KLF_USE_QT4)
  add_definitions(-DKLFBACKEND_QT4)
endif(KLF_USE_QT4)
# Add flags for DEBUG build configuration
set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -DKLF_DEBUG -Wall")
# Add flags for RELEASE build configuration
set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -DQT_NO_DEBUG_OUTPUT -O3")

# include SUBDIR: our klfbackend
add_subdirectory(klfbackend)

if(NOT KLF_BUILD_GUI)
  RETURN()
endif(NOT KLF_BUILD_GUI)


# Re-Include what we need from Qt for GUI KLF core+gui+xml+sql(+dbus)
set(QT_USE_QTXML TRUE)
set(QT_USE_QTSQL TRUE)
if(KLF_USE_DBUS)
  set(QT_USE_QTDBUS TRUE)
endif(KLF_USE_DBUS)
# include our Qt4 stuff; Qt3 needs nothing.
if(KLF_USE_QT4)
include(${QT_USE_FILE})
endif(KLF_USE_QT4)


# Sources
set(klatexformula_SRCS
	klfcolorchooser.cpp
	klfconfig.cpp
	klfpobj.cpp
	klfutil.cpp
	klfguiutil.cpp
	klffactory.cpp
	klflatexsyntaxhighlighter.cpp
	klflatexsymbols.cpp
	klfpixmapbutton.cpp
	klflib.cpp
	klflibdbengine.cpp
	klfliblegacyengine.cpp
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
#  * with QObject classes (needs moc)
set(klatexformula_MOCHEADERS
	klfcolorchooser.h
	klflatexsyntaxhighlighter.h
	klflatexsymbols.h
	klfpixmapbutton.h
	klfguiutil.h
	klflib.h
	klflibdbengine.h
	klflibdbengine_p.h
	klfliblegacyengine.h
	klflibview.h
	klflibview_p.h
	klflibentryeditor.h
	klflibbrowser.h
	klflibbrowser_p.h
	klfmime.h
	klfmainwin.h
	klfmainwin_p.h
	klfpathchooser.h
	klfsettings.h
	klfstylemanager.h
	qtcolortriangle.h
	klfdisplaylabel.h
)
#  * all headers
set(klatexformula_HEADERS
	klfmain.h
	klfutil.h
	klfconfig.h
	klfpobj.h
	klffactory.h
	klfpluginiface.h
	klflib_p.h
	klfliblegacyengine_p.h
	${klatexformula_MOCHEADERS}
)
# UI Forms
set(klatexformula_UIS
	klflatexsymbols.ui
	klflibbrowser.ui
	klflibentryeditor.ui
	klflibrespropeditor.ui
	klfmainwin.ui
	klfprogerr.ui
	klfsettings.ui
	klfstylemanager.ui
	klfaboutdialog.ui
	klfwhatsnewdialog.ui
	klfcolorchoosewidget.ui
	klfcolordialog.ui
	klflibopenresourcedlg.ui
	klfliblocalfilewidget.ui
	klflibnewsubresdlg.ui
	klflibexportdialog.ui
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
	mswin/klfwinclipboard.cpp
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
if(KLF_DEBUG_USE_MODELTEST)
  add_definitions(-DKLF_DEBUG_USE_MODELTEST)
  set(klatexformula_SRCS
	${klatexformula_SRCS}
	modeltest.cpp
  )
  set(klatexformula_MOCHEADERS
	${klatexformula_MOCHEADERS}
	modeltest.h
  )
endif(KLF_DEBUG_USE_MODELTEST)

KLFInstHeaders(klatexformula_INSTHEADERS "${klatexformula_HEADERS}")
# Translations
qt4_add_translation(klatexformula_QMS ${klatexformula_TSS})
if(klatexformula_QMS STREQUAL cache_klatexformula_QMS)
  # nothing to do, .qrc is up-to-date !
else(klatexformula_QMS STREQUAL cache_klatexformula_QMS)
  # re-build the klf_translations.qrc file
  file(WRITE "${CMAKE_CURRENT_BINARY_DIR}/klf_translations.qrc" "<RCC>\n  <qresource prefix=\"i18n\">")
  foreach(qm ${klatexformula_QMS})
    file(RELATIVE_PATH baseqmname "${CMAKE_CURRENT_BINARY_DIR}" "${qm}")  # very inelegant way to find file name without path
    file(APPEND "${CMAKE_CURRENT_BINARY_DIR}/klf_translations.qrc" "    <file alias=\"${baseqmname}\">${qm}</file>\n")
  endforeach()
  file(APPEND "${CMAKE_CURRENT_BINARY_DIR}/klf_translations.qrc" "  </qresource>\n</RCC>\n")
  set(cache_klatexformula_QMS "${klatexformula_QMS}" CACHE INTERNAL "List of translation files")
endif(klatexformula_QMS STREQUAL cache_klatexformula_QMS)
list(APPEND klatexformula_QRCS "${CMAKE_CURRENT_BINARY_DIR}/klf_translations.qrc")
# Uis, Mocs
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
  if(KLF_MACOSX_BUNDLES)
    KLFMakeFramework(klflib ${klatexformula_INSTHEADERS})
  endif(KLF_MACOSX_BUNDLES)
  set_target_properties(klflib PROPERTIES
	VERSION ${KLF_LIB_VERSION}
	SOVERSION ${KLF_LIB_VERSION}
	MACOSX_FRAMEWORK_IDENTIFIER "org.klatexformula.klflib"
	MACOSX_FRAMEWORK_SHORT_VERSION_STRING "${KLF_VERSION}"
	MACOSX_FRAMEWORK_BUNDLE_VERSION "${KLF_VERSION}"
  )
endif(KLF_KLFLIB_STATIC)
add_dependencies(klflib klfbackend)
target_link_libraries(klflib ${QT_LIBRARIES} klfbackend)
set_target_properties(klflib PROPERTIES
	PUBLIC_HEADER "${klatexformula_INSTHEADERS}"
)

# KLatexFormula Executable
qt4_add_resources(klatexformula_QRC_CPPS ${klatexformula_QRCS})
set(klatexformulamain_ALLSRC main.cpp ${klatexformula_QRC_CPPS})
set(klatexformulamain_winrc)
if(MINGW)
  set(KLF_WIN_ICON "mswin\\\\klficon64.ico" CACHE STRING
    "Icon for klatexformula.exe program, relative to src/, with (escaped double-backslashes!)")
  mark_as_advanced(KLF_WIN_ICON)
  file(WRITE "${CMAKE_CURRENT_BINARY_DIR}/klatexformula.rc"
       "IDI_ICON1               ICON    DISCARDABLE     \"${KLF_WIN_ICON}\"\n")
  include_directories("${CMAKE_CURRENT_SOURCE_DIR}")
  # windows .rc resource compilation for mingw
  ADD_CUSTOM_COMMAND(OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/klatexformula.rc.o"
    COMMAND windres.exe -I"${CMAKE_CURRENT_SOURCE_DIR}"
    -i"${CMAKE_CURRENT_BINARY_DIR}/klatexformula.rc"
    -o "${CMAKE_CURRENT_BINARY_DIR}/klatexformula.rc.o"
  )

  set(klatexformulamain_ALLSRC ${klatexformulamain_ALLSRC}
			      "${CMAKE_CURRENT_BINARY_DIR}/klatexformula.rc.o")
endif(MINGW)
add_executable(klatexformula ${klatexformulamain_ALLSRC})
target_link_libraries(klatexformula ${QT_LIBRARIES} klflib klfbackend)

if(KLF_MACOSX_BUNDLES)
  # Set up the application bundle under Mac OS X
  set_target_properties(klatexformula PROPERTIES
        OUTPUT_NAME               klatexformula-${KLF_VERSION}
        MACOSX_BUNDLE_ICON_FILE   klficon.icns
        MACOSX_BUNDLE_INFO_STRING "KLatexFormula version ${KLF_VERSION}"
        MACOSX_BUNDLE_GUI_IDENTIFIER "org.klatexformula.klatexformula"
        MACOSX_BUNDLE_LONG_VERSION_STRING "${KLF_VERSION}"
        MACOSX_BUNDLE_SHORT_VERSION_STRING "${KLF_VERSION}"
        MACOSX_BUNDLE_BUNDLE_VERSION "${KLF_VERSION}"
        MACOSX_BUNDLE_BUNDLE_COPYRIGHT "Copyright (C) 2010 Philippe Faist"
  )
  KLFMakeBundle(klatexformula klatexformula-${KLF_VERSION}.app)
  
  # Add the frameworks to be included as private frameworks.
  KLFBundlePrivateImportQtLib(klatexformula "${KLF_MACOSX_BUNDLE_PATH}"
	klatexformula_BUNDLEXTRA  Core  "")
  KLFBundlePrivateImportQtLib(klatexformula "${KLF_MACOSX_BUNDLE_PATH}"
	klatexformula_BUNDLEXTRA  Gui   Core)
  KLFBundlePrivateImportQtLib(klatexformula "${KLF_MACOSX_BUNDLE_PATH}"
	klatexformula_BUNDLEXTRA  Xml   Core)
  KLFBundlePrivateImportQtLib(klatexformula "${KLF_MACOSX_BUNDLE_PATH}"
	klatexformula_BUNDLEXTRA  Sql   Core)
  KLFBundlePrivateImportQtLib(klatexformula "${KLF_MACOSX_BUNDLE_PATH}"
	klatexformula_BUNDLEXTRA  DBus  "Core;Xml")
  message("Will Package Qt Plugins --> ${KLF_QT_PLUGINS}")
  foreach(plugin ${KLF_QT_PLUGINS})
    KLFBundlePrivateImport(klatexformula "${KLF_MACOSX_BUNDLE_PATH}" klatexformula_BUNDLEXTRA
			"${plugin}" "${QT_PLUGINS_DIR}/${plugin}" plugins)
    KLFInstallNameToolID(klatexformula "${KLF_MACOSX_BUNDLE_PATH}" "plugins/${plugin}")
    KLFBundlePrivateLibUpdateQtDep(klatexformula "${KLF_MACOSX_BUNDLE_PATH}"
			"plugins/${plugin}"  "Core;Gui;Xml;Sql")
  endforeach()
  # Import&Setup klfbackend framework
  KLFBundlePrivateImport(klatexformula "${KLF_MACOSX_BUNDLE_PATH}" klatexformula_BUNDLEXTRA
	"klfbackend.framework" "${CMAKE_CURRENT_BINARY_DIR}/klfbackend/klfbackend.framework"
	Frameworks)
  KLFInstallNameToolID(klatexformula "${KLF_MACOSX_BUNDLE_PATH}" "Frameworks/klfbackend.framework/Versions/${KLF_LIB_VERSION}/klfbackend")
  KLFBundlePrivateLibUpdateQtDep(klatexformula "${KLF_MACOSX_BUNDLE_PATH}"
				 "Frameworks/klfbackend.framework/Versions/${KLF_LIB_VERSION}/klfbackend"  "Core;Gui;Xml;Sql;DBus")
  # Import&Setup klflib framework
  KLFBundlePrivateImport(klatexformula "${KLF_MACOSX_BUNDLE_PATH}" klatexformula_BUNDLEXTRA
	"klflib.framework" "${CMAKE_CURRENT_BINARY_DIR}/klflib.framework"
	Frameworks)
  KLFInstallNameToolID(klatexformula "${KLF_MACOSX_BUNDLE_PATH}" "Frameworks/klflib.framework/Versions/${KLF_LIB_VERSION}/klflib")
  KLFBundlePrivateLibUpdateQtDep(klatexformula "${KLF_MACOSX_BUNDLE_PATH}"
				 "Frameworks/klflib.framework/Versions/${KLF_LIB_VERSION}/klflib"  "Core;Gui;Xml;Sql;DBus")
  KLFInstallNameToolChange(klatexformula
			   "Frameworks/klflib.framework/Versions/${KLF_LIB_VERSION}/klflib"
			   "${KLF_MACOSX_BUNDLE_PATH}"  "Frameworks/klfbackend.framework/Versions/${KLF_LIB_VERSION}/klfbackend"
			   "${CMAKE_CURRENT_BINARY_DIR}/klfbackend/klfbackend.framework/Versions/${KLF_LIB_VERSION}/klfbackend")
  # Setup klatexformula executable's lib dependencies
  KLFBundlePrivateLibUpdateQtDep(klatexformula "${KLF_MACOSX_BUNDLE_PATH}" "MacOS/klatexformula-${KLF_VERSION}" "Core;Gui;Xml;Sql;DBus")
  KLFInstallNameToolChange(klatexformula
			   "MacOS/klatexformula-${KLF_VERSION}" "${KLF_MACOSX_BUNDLE_PATH}"
			   "Frameworks/klfbackend.framework/Versions/${KLF_LIB_VERSION}/klfbackend"
			   "${CMAKE_CURRENT_BINARY_DIR}/klfbackend/klfbackend.framework/Versions/${KLF_LIB_VERSION}/klfbackend")
  KLFInstallNameToolChange(klatexformula
                           "MacOS/klatexformula-${KLF_VERSION}" "${KLF_MACOSX_BUNDLE_PATH}"
                           "Frameworks/klflib.framework/Versions/${KLF_LIB_VERSION}/klflib"
                           "${CMAKE_CURRENT_BINARY_DIR}/klflib.framework/Versions/${KLF_LIB_VERSION}/klflib")
  # Default Qt config stuff
  KLFBundlePrivateImport(klatexformula "${KLF_MACOSX_BUNDLE_PATH}" klatexformula_BUNDLEXTRA
			qt.conf "${CMAKE_CURRENT_SOURCE_DIR}/macosx/qt.conf" Resources)
  # Add a few icons for our app
  KLFBundlePrivateImport(klatexformula "${KLF_MACOSX_BUNDLE_PATH}" klatexformula_BUNDLEXTRA
			klficon.icns "${CMAKE_CURRENT_SOURCE_DIR}/macosx/klficon.icns" Resources)
  KLFBundlePrivateImport(klatexformula "${KLF_MACOSX_BUNDLE_PATH}" klatexformula_BUNDLEXTRA
	klatexformula-128.png "${CMAKE_CURRENT_SOURCE_DIR}/klatexformula-128.png" Resources)
  KLFBundlePrivateImport(klatexformula "${KLF_MACOSX_BUNDLE_PATH}" klatexformula_BUNDLEXTRA
	klatexformula-64.png "${CMAKE_CURRENT_SOURCE_DIR}/klatexformula-64.png" Resources)
  KLFBundlePrivateImport(klatexformula "${KLF_MACOSX_BUNDLE_PATH}" klatexformula_BUNDLEXTRA
	klatexformula-32.png "${CMAKE_CURRENT_SOURCE_DIR}/klatexformula-32.png" Resources)
  KLFBundlePrivateImport(klatexformula "${KLF_MACOSX_BUNDLE_PATH}" klatexformula_BUNDLEXTRA
	klatexformula-16.png "${CMAKE_CURRENT_SOURCE_DIR}/klatexformula-16.png" Resources)
  message("Extras to be bundled are ${klatexformula_BUNDLEXTRA}")
  KLFBundlePackage(klatexformula   "${klatexformula_BUNDLEXTRA}")
endif(KLF_MACOSX_BUNDLES)

# Build Plugins
add_subdirectory(plugins)


