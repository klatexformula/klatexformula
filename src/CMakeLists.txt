# ######################################## #
# CMake project file for klatexformula/src #
# ######################################## #
# $Id$
# ######################################## #

if(KLF_MACOSX_BUNDLES)
  set(KLF_MACOSXDIR "${CMAKE_CURRENT_SOURCE_DIR}/macosx")
  set(KLF_MACOSX_BUNDLE_PATH "${CMAKE_CURRENT_BINARY_DIR}/klatexformula-${KLF_VERSION}.app")
  include("${KLF_MACOSXDIR}/KLFMacBundle.cmake")
endif(KLF_MACOSX_BUNDLES)

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
#  * with QObject classes (needs moc)
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
#  * all headers
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
# Translations
qt4_add_translation(klatexformula_QMS ${klatexformula_TSS})
#add_custom_command(OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/klf_translations.qrc"
#	COMMAND "${CMAKE_COMMAND}" -P "${CMAKE_CURRENT_SOURCE_DIR}/maketranslationsqrc.cmake"
#	COMMENT "Creating a translations Qt resource"
#	VERBATIM
#)
file(WRITE "${CMAKE_CURRENT_SOURCE_DIR}/klf_translations.qrc" "<RCC>\n  <qresource>")
foreach(qm ${klatexformula_QMS})
  file(APPEND "${CMAKE_CURRENT_SOURCE_DIR}/klf_translations.qrc" "    <file>${qm}</file>\n")
endforeach()
file(APPEND "${CMAKE_CURRENT_SOURCE_DIR}/klf_translations.qrc" "  </qresource>\n</RCC>\n")
list(APPEND klatexformula_QRCS "${CMAKE_CURRENT_SOURCE_DIR}/klf_translations.qrc")
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
	PUBLIC_HEADER "${klflatexformula_INSTHEADERS}"
)

# KLatexFormula Executable
qt4_add_resources(klatexformula_QRC_CPPS ${klatexformula_QRCS})
set(klatexformulamain_ALLSRC main.cpp ${klatexformula_QRC_CPPS})

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

# Additional tweaks
configure_file(
  "${CMAKE_CURRENT_SOURCE_DIR}/klatexformula.desktop.in"
  "${CMAKE_CURRENT_BINARY_DIR}/klatexformula.desktop"
  IMMEDIATE @ONLY)


# --- INSTALLATION TARGETS ---
install(TARGETS klflib RUNTIME DESTINATION "lib${KLF_LIB_SUFFIX}"
		       ARCHIVE DESTINATION "lib${KLF_LIB_SUFFIX}"
		       FRAMEWORK DESTINATION /Library/Frameworks
		       PUBLIC_HEADER DESTINATION "include")
if(KLF_MACOSX_BUNDLES)
  # Installed library's ID
  KLFInstFrameworkUpdateId("/Library/Frameworks/klflib.framework/Versions/${KLF_LIB_VERSION}/klflib")
  # Installed library's dependency on klfbackend should be updated
  KLFInstFrameworkUpdateLibChange(
  "/Library/Frameworks/klflib.framework/Versions/${KLF_LIB_VERSION}/klflib"
  "${CMAKE_CURRENT_BINARY_DIR}/klfbackend/klfbackend.framework/Versions/${KLF_LIB_VERSION}/klfbackend"
  "/Library/Frameworks/klfbackend.framework/Versions/${KLF_LIB_VERSION}/klfbackend"
  )
endif(KLF_MACOSX_BUNDLES)
install(TARGETS klatexformula RUNTIME DESTINATION bin
			      BUNDLE DESTINATION /Applications)
if(UNIX AND NOT KLF_MACOSX_BUNDLES)
  install(CODE "execute_process(COMMAND \"${CMAKE_COMMAND} -E create_symlink \\\"${CMAKE_INSTALL_PREFIX}/bin/klatexformula\\\" \\\"${CMAKE_INSTALL_PREFIX}/bin/klatexformula_cmdl\\\"\")"
  )
endif(UNIX AND NOT KLF_MACOSX_BUNDLES)
if(KLF_MACOSX_BUNDLES)
  # Update klatexformula's dependency on the installed frameworks.
  KLFInstFrameworkUpdateLibChange(
  "/Applications/klatexformula-${KLF_VERSION}/Contents/MacOS/klatexformula-${KLF_VERSION}"
  "${CMAKE_CURRENT_BINARY_DIR}/klfbackend/klfbackend.framework/Versions/${KLF_LIB_VERSION}/klfbackend"
  "/Library/Frameworks/klfbackend.framework/Versions/${KLF_LIB_VERSION}/klfbackend"
  )
  KLFInstFrameworkUpdateLibChange(
  "/Applications/klatexformula-${KLF_VERSION}/Contents/MacOS/klatexformula-${KLF_VERSION}"
  "${CMAKE_CURRENT_BINARY_DIR}/klflib.framework/Versions/${KLF_LIB_VERSION}/klflib"
  "/Library/Frameworks/klflib.framework/Versions/${KLF_LIB_VERSION}/klflib"
  )
endif(KLF_MACOSX_BUNDLES)

if(KLF_INSTALL_DESKTOP)
install(FILES "${CMAKE_CURRENT_BINARY_DIR}/klatexformula.desktop" DESTINATION share/applications/)
install(FILES klatexformula-128.png DESTINATION share/pixmaps/)
install(FILES klatexformula-64.png DESTINATION share/pixmaps/)
install(FILES klatexformula-32.png DESTINATION share/pixmaps/)
install(FILES klatexformula-16.png DESTINATION share/pixmaps/)
install(FILES x-klatexformula.xml x-klatexformula-db.xml
	DESTINATION share/mime/packages/)
install(CODE
  "message(STATUS \"Updating Mime Types Database\")
   execute_process(COMMAND \"update-mime-database ${CMAKE_INSTALL_PREFIX}/share/mime/\"
    )")
endif(KLF_INSTALL_DESKTOP)

