# ######################################## #
# CMake project file for klatexformula/src #
# ######################################## #
# $Id$
# ######################################## #

if(KLF_MACOSX_BUNDLES)
  set(KLF_MACOSXDIR "${CMAKE_CURRENT_SOURCE_DIR}/macosx")

  include(KLFMacBundle)
endif(KLF_MACOSX_BUNDLES)

if(WIN32)
  # Extra linker flags for win32 compilation
  set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -Wl,--enable-auto-import")
  set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} -Wl,--enable-auto-import")
endif(WIN32)


# include these subprojects...

if(KLF_BUILD_TOOLS)
  add_subdirectory(klftools)
endif(KLF_BUILD_TOOLS)

if(KLF_BUILD_BACKEND)
  add_subdirectory(klfbackend)
endif(KLF_BUILD_BACKEND)

# Re-Include what we need from Qt for GUI KLF core+gui+xml+sql(+dbus)
set(QT_USE_QTXML TRUE)
set(QT_USE_QTSQL TRUE)
if(KLF_USE_DBUS)
  set(QT_USE_QTDBUS TRUE)
endif(KLF_USE_DBUS)
set(QT_USE_QTUITOOLS TRUE)
include(${QT_USE_FILE})



if(NOT KLF_BUILD_GUI)
  return()
endif(NOT KLF_BUILD_GUI)


# Sources
set(klfapp_SRCS
	klfconfig.cpp
	klflatexsymbols.cpp
	klfstyle.cpp
	klflibentryeditor.cpp
	klflibbrowser.cpp
	klflib.cpp
	klflibview.cpp
	klflibdbengine.cpp
	klfliblegacyengine.cpp
	klfmime.cpp
	klfmainwin.cpp
	klfsettings.cpp
	klfstylemanager.cpp
	klfmain.cpp
	klfcmdiface.cpp
	klfuiloader.cpp
)
# Headers
#  * with QObject classes (needs moc)
set(klfapp_MOCHEADERS
	klflatexsymbols.h
	klflatexsymbols_p.h
	klflibentryeditor.h
	klflibbrowser.h
	klflibbrowser_p.h
	klflib.h
	klflibview.h
	klflibview_p.h
	klflibdbengine.h
	klflibdbengine_p.h
	klfliblegacyengine.h
	klfliblegacyengine_p.h
	klfmime.h
	klfmime_p.h
	klfmainwin.h
	klfmainwin_p.h
	klfsettings.h
	klfsettings_p.h
	klfstylemanager.h
	klfcmdiface.h
	klfuiloader_p.h
)
#  * all headers
set(klfapp_HEADERS
	klfmain.h
	klfconfig.h
	klfconfig_p.h
	klfstyle.h
	klfpluginiface.h
	klflib_p.h
	klfuiloader.h
	${klfapp_MOCHEADERS}
)
# UI Forms
set(klfapp_UIS
	klflatexsymbols.ui
	klflibbrowser.ui
	klflibentryeditor.ui
	klflibopenresourcedlg.ui
	klflibrespropeditor.ui
	klfliblocalfilewidget.ui
	klflibnewsubresdlg.ui
	klfmainwin.ui
	klfprogerr.ui
	klfsettings.ui
	klfstylemanager.ui
	klfaboutdialog.ui
	klfwhatsnewdialog.ui
	klflibexportdialog.ui
)

# Special adjustments for certain platforms and options
# * Windows
if(WIN32)
  list(APPEND klfapp_SRCS
	mswin/klfwinclipboard.cpp
  )
endif(WIN32)
if(APPLE)
  set(klfapp_HEADERS ${klfapp_HEADERS}
	macosx/klfmacclipboard.h
  )
  set(klfapp_SRCS ${klfapp_SRCS}
	macosx/klfmacclipboard.cpp
	macosx/klfmainwin_mac.mm
	macosx/klfcmdifacedispatch_mac.mm
  )
endif(APPLE)
# * X11
if(Q_WS_X11)
  set(klfapp_SRCS ${klfapp_SRCS}
	x11/klfx11.cpp
  )
endif(Q_WS_X11)
# * DBus
if(KLF_USE_DBUS)
  add_definitions(-DKLF_USE_DBUS)
  list(APPEND klfapp_SRCS
	klfdbus.cpp
  )
  list(APPEND klfapp_MOCHEADERS
	klfdbus.h
  )
  list(APPEND klfapp_HEADERS
	klfdbus.h
  )
endif(KLF_USE_DBUS)
if(KLF_DEBUG_USE_MODELTEST)
  add_definitions(-DKLF_DEBUG_USE_MODELTEST)
  set(klfapp_SRCS
	${klfapp_SRCS}
	modeltest.cpp
  )
  set(klfapp_MOCHEADERS
	${klfapp_MOCHEADERS}
	modeltest.h
  )
endif(KLF_DEBUG_USE_MODELTEST)

KLFInstHeaders(klfapp_INSTHEADERS "${klfapp_HEADERS}")
list(REMOVE_ITEM klfapp_INSTHEADERS
	# list more private headers here
	qtcolortriangle.h
	macosx/klfmacclipboard.h
)



# klfapp Library
# --------------

if(KLF_LIBKLFTOOLS_STATIC)
  # let main() know it has to initialize klftools' resource with  Q_INIT_RESOURCE(klftoolsres)
  add_definitions(-DKLF_LIBKLFTOOLS_STATIC)
endif(KLF_LIBKLFTOOLS_STATIC)

if(KLF_NO_CMU_FONT)
  add_definitions(-DKLF_NO_CMU_FONT)
endif(KLF_NO_CMU_FONT)

# Don't use CMAKE_INSTALL_PREFIX itself, as it is irrelevant for windows installers
# we need relative path
set(klf_share_dir_abs "${CMAKE_INSTALL_PREFIX}")
if(UNIX AND NOT KLF_MACOSX_BUNDLES)
  set(klf_share_dir_abs "${klf_share_dir_abs}/share/klatexformula")
endif(UNIX AND NOT KLF_MACOSX_BUNDLES)
KLFRelativePath(klf_share_dir "${KLF_ABS_INSTALL_BIN_DIR}" "${klf_share_dir_abs}")
KLFRelativePath(klf_share_rccresources_dir "${KLF_ABS_INSTALL_BIN_DIR}" "${KLF_ABS_INSTALL_RCCRESOURCES_DIR}")
if(KLF_MACOSX_BUNDLES OR IS_ABSOLUTE "${klf_share_dir}")
  set(klf_share_dir "") # let the program find out
endif(KLF_MACOSX_BUNDLES OR IS_ABSOLUTE "${klf_share_dir}")
if(KLF_MACOSX_BUNDLES OR IS_ABSOLUTE "${klf_share_rccresources_dir}")
  set(klf_share_rccresources_dir "") # let the program find out
endif(KLF_MACOSX_BUNDLES OR IS_ABSOLUTE "${klf_share_rccresources_dir}")
if(klf_share_dir)
  add_definitions(-DKLF_SHARE_DIR="${klf_share_dir}")
endif(klf_share_dir)
if(klf_share_rccresources_dir)
  add_definitions(-DKLF_SHARE_RCCRESOURCES_DIR="${klf_share_rccresources_dir}")
endif(klf_share_rccresources_dir)

# Uis, Mocs
qt4_wrap_ui(klfapp_UIS_H ${klfapp_UIS})
qt4_wrap_cpp(klfapp_MOC_CPPS ${klfapp_MOCHEADERS})
include_directories("${CMAKE_CURRENT_BINARY_DIR}" "${CMAKE_CURRENT_SOURCE_DIR}"
	"${CMAKE_CURRENT_SOURCE_DIR}/klftools" "${CMAKE_CURRENT_SOURCE_DIR}/klfbackend")
set(klfapp_ALLSRC ${klfapp_SRCS} ${klfapp_UIS_H} ${klfapp_MOC_CPPS})

if(KLF_LIBKLFAPP_STATIC)
  add_library(klfapp STATIC ${klfapp_ALLSRC})
else(KLF_LIBKLFAPP_STATIC)
  add_library(klfapp SHARED ${klfapp_ALLSRC})
  if(KLF_MACOSX_BUNDLES)
    KLFMMakeFramework(klfapp ${klfapp_INSTHEADERS})
    set_target_properties(klfapp PROPERTIES
	MACOSX_FRAMEWORK_IDENTIFIER "org.klatexformula.klfapp"
	MACOSX_FRAMEWORK_SHORT_VERSION_STRING "${KLF_VERSION}"
	MACOSX_FRAMEWORK_BUNDLE_VERSION "${KLF_VERSION}"
    )
  endif(KLF_MACOSX_BUNDLES)
  set_target_properties(klfapp PROPERTIES
	VERSION "${KLF_LIB_VERSION}"
	SOVERSION "${KLF_LIB_VERSION}"
  )
endif(KLF_LIBKLFAPP_STATIC)
add_dependencies(klfapp klftools klfbackend)
target_link_libraries(klfapp ${QT_LIBRARIES} klfbackend klftools)
set_target_properties(klfapp PROPERTIES
	#	DEBUG_POSTFIX "${KLF_DEBUG_POSTFIX}"
	PUBLIC_HEADER "${klfapp_INSTHEADERS}"
)


# klatexformula Executable
# ------------------------

# Sources
set(klatexformula_SRCS
	main.cpp
	klfapp.cpp
)
set(klatexformula_MOCHEADERS
	klfapp.h
)
set(klatexformula_HEADERS
        ${klatexformula_MOCHEADERS}
)
# Translations
set(klatexformula_TSS
	i18n/klf_fr.ts
	i18n/klf_cs.ts
)
# Resources
set(klatexformula_QRCS
	klfres.qrc
)
# Translations
qt4_add_translation(klatexformula_QMS ${klatexformula_TSS})

# build the klf_xtradata.qrc file
set(klf_xtradata_res "${CMAKE_CURRENT_BINARY_DIR}/klf_xtradata.qrc")
file(WRITE "${klf_xtradata_res}" "<RCC>\n  <qresource prefix=\"i18n\">")
#  - add translations
foreach(qm ${klatexformula_QMS})
  get_filename_component(baseqmname  "${qm}"  NAME)
  file(APPEND "${klf_xtradata_res}"    "    <file alias=\"${baseqmname}\">${qm}</file>\n")
endforeach(qm)
file(APPEND "${klf_xtradata_res}" "  </qresource>\n")
#  - add fonts
file(APPEND "${klf_xtradata_res}" "  <qresource prefix=\"data/fonts\">\n")
foreach(fnt ${KLF_INCLUDE_FONTS})
  get_filename_component(basefntname "${fnt}" NAME)
  if(NOT IS_ABSOLUTE "${fnt}")
    set(fnt "${CMAKE_CURRENT_SOURCE_DIR}/${fnt}")
  endif(NOT IS_ABSOLUTE "${fnt}")
  file(APPEND "${klf_xtradata_res}"   "    <file alias=\"${basefntname}\">${fnt}</file>\n")
endforeach(fnt)
file(APPEND "${klf_xtradata_res}" "  </qresource>\n")
file(APPEND "${klf_xtradata_res}" "</RCC>\n")

list(APPEND klatexformula_QRCS "${klf_xtradata_res}")


qt4_add_resources(klatexformula_QRC_CPPS ${klatexformula_QRCS})
qt4_wrap_cpp(klatexformula_MOC_CPPS ${klatexformula_MOCHEADERS})
set(klatexformula_ALLSRC ${klatexformula_SRCS} ${klatexformula_QRC_CPPS} ${klatexformula_MOC_CPPS})
set(klatexformula_winrc)
if(MINGW)
  set(KLF_WIN_ICON "mswin\\\\klficon64.ico" CACHE STRING
	"Icon for klatexformula.exe program, relative to src/, with (escaped double-backslashes!)")
  file(WRITE "${CMAKE_CURRENT_BINARY_DIR}/klatexformula.rc"
       "IDI_ICON1               ICON    DISCARDABLE     \"${KLF_WIN_ICON}\"\n")
  include_directories("${CMAKE_CURRENT_SOURCE_DIR}")
  # windows .rc resource compilation for mingw
  ADD_CUSTOM_COMMAND(OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/klatexformula.rc.o"
    COMMAND windres.exe -I"${CMAKE_CURRENT_SOURCE_DIR}"
    -i"${CMAKE_CURRENT_BINARY_DIR}/klatexformula.rc"
    -o "${CMAKE_CURRENT_BINARY_DIR}/klatexformula.rc.o"
  )

  set(klatexformula_ALLSRC ${klatexformula_ALLSRC}
			      "${CMAKE_CURRENT_BINARY_DIR}/klatexformula.rc.o")
endif(MINGW)

add_executable(klatexformula ${klatexformula_ALLSRC})
target_link_libraries(klatexformula ${QT_LIBRARIES} klfbackend klftools klfapp)


if(WIN32)
  # On debug versions, the application will open a console in which the debug output is shown.
  # Add the -mwindows only if we are not compiling in debug mode.
  # Add also a separate target klatexformula_cmdl to build the cmdl version, keeping the shell
  # window open.
  add_executable(klatexformula_cmdl ${klatexformula_ALLSRC})
  target_link_libraries(klatexformula_cmdl ${QT_LIBRARIES} klfbackend klftools klfapp)
  if(NOT KLF_DEBUG)
    set_target_properties(klatexformula  PROPERTIES LINK_FLAGS_RELEASE  "-Wl,-subsystem,windows")
    set_target_properties(klatexformula_cmdl  PROPERTIES LINK_FLAGS_RELEASE  "-Wl,-subsystem,console")
  endif(NOT KLF_DEBUG)
endif(WIN32)



if(KLF_MACOSX_BUNDLES)
  # Set up the application bundle under Mac OS X
  set(KLF_BUNDLE_ICON_FILE "klficon.icns")
  configure_file(
    "${CMAKE_SOURCE_DIR}/src/macosx/Info.plist.in"
    "${CMAKE_CURRENT_BINARY_DIR}/Info.plist"
    @ONLY)
  set_target_properties(klatexformula PROPERTIES
        OUTPUT_NAME               klatexformula-${KLF_VERSION}
        MACOSX_BUNDLE_ICON_FILE   klficon.icns
	MACOSX_BUNDLE_INFO_PLIST  "${CMAKE_CURRENT_BINARY_DIR}/Info.plist"
        MACOSX_BUNDLE_INFO_STRING "KLatexFormula version ${KLF_VERSION}"
        MACOSX_BUNDLE_GUI_IDENTIFIER "org.klatexformula.klatexformula"
        MACOSX_BUNDLE_LONG_VERSION_STRING "${KLF_VERSION}"
        MACOSX_BUNDLE_SHORT_VERSION_STRING "${KLF_VERSION}"
        MACOSX_BUNDLE_BUNDLE_VERSION "${KLF_VERSION}"
        MACOSX_BUNDLE_BUNDLE_COPYRIGHT "Copyright (C) 2011 Philippe Faist"
  )
  KLFMMakeBundle(klatexformula)
  KLFMBundleMkPrivateDir(klatexformula "Contents/Resources/rccresources")
  
  # Add the frameworks to be included as private frameworks.
  KLFMBundlePrivateImportQtLib(klatexformula 	Core  "")
  KLFMBundlePrivateImportQtLib(klatexformula 	Gui   Core)
  KLFMBundlePrivateImportQtLib(klatexformula 	Xml   Core)
  KLFMBundlePrivateImportQtLib(klatexformula 	Sql   Core)
  if(KLF_USE_DBUS)
    KLFMBundlePrivateImportQtLib(klatexformula      DBus  "Core;Xml")
  endif(KLF_USE_DBUS)
  foreach(plugin ${KLF_BUNDLE_QT_PLUGINS})
    KLFMBundlePrivateImport(klatexformula "${plugin}" "${QT_PLUGINS_DIR}/${plugin}" plugins)
    KLFMInstallNameToolID(klatexformula "plugins/${plugin}")
    KLFMBundlePrivateLibUpdateQtDep(klatexformula "plugins/${plugin}"  "Core;Gui;Xml;Sql")
  endforeach()
  #- # Import&Setup klftools_desplugin designer plugin, needed for QUiLoader to load user scripts' input widget UIs
  #- KLFGetTargetLocation(klf_desplugin_target_path klftoolsdesplugin)
  #- KLFGetTargetFileName(klf_desplugin_target_name klftoolsdesplugin)
  #- KLFMBundlePrivateImport(klatexformula "designer/${klf_desplugin_target_name}"
  #-   "${klf_desplugin_target_path}" "plugins")
  #- KLFMInstallNameToolID(klatexformula "plugins/designer/${klf_desplugin_target_name}")
  #- KLFMBundlePrivateLibUpdateQtDep(klatexformula "plugins/designer/${klf_desplugin_target_name}"  "Core;Gui;Xml;")
  #- ...... but the plugin depends on libQtDesigner/libQtDesignerComponents (!???!)
  # Import&Setup klfbackend framework
  KLFMBundlePrivateImport(klatexformula "klfbackend.framework"
    "${CMAKE_CURRENT_BINARY_DIR}/klfbackend/klfbackend.framework"  Frameworks)
  KLFMInstallNameToolID(klatexformula "Frameworks/klfbackend.framework/Versions/${KLF_LIB_VERSION}/klfbackend")
  KLFMBundlePrivateLibUpdateQtDep(klatexformula "Frameworks/klfbackend.framework/Versions/${KLF_LIB_VERSION}/klfbackend"  "Core;Gui;Xml;Sql;DBus")
  # Import&Setup klftools framework
  KLFMBundlePrivateImport(klatexformula "klftools.framework"
    "${CMAKE_CURRENT_BINARY_DIR}/klftools/klftools.framework"  Frameworks)
  KLFMInstallNameToolID(klatexformula "Frameworks/klftools.framework/Versions/${KLF_LIB_VERSION}/klftools")
  KLFMBundlePrivateLibUpdateQtDep(klatexformula "Frameworks/klftools.framework/Versions/${KLF_LIB_VERSION}/klftools"  "Core;Gui;Xml;Sql;DBus")
  KLFMInstallNameToolChange(klatexformula
	"Frameworks/klfbackend.framework/Versions/${KLF_LIB_VERSION}/klfbackend"
	"Frameworks/klftools.framework/Versions/${KLF_LIB_VERSION}/klftools"
	"${CMAKE_CURRENT_BINARY_DIR}/klftools/klftools.framework/Versions/${KLF_LIB_VERSION}/klftools")
  # Import&Setup klfapp framework
  KLFMBundlePrivateImport(klatexformula "klfapp.framework" "${CMAKE_CURRENT_BINARY_DIR}/klfapp.framework"  Frameworks)
  KLFMInstallNameToolID(klatexformula "Frameworks/klfapp.framework/Versions/${KLF_LIB_VERSION}/klfapp")
  KLFMBundlePrivateLibUpdateQtDep(klatexformula
    "Frameworks/klfapp.framework/Versions/${KLF_LIB_VERSION}/klfapp"  "Core;Gui;Xml;Sql;DBus")
  KLFMInstallNameToolChange(klatexformula
	"Frameworks/klfapp.framework/Versions/${KLF_LIB_VERSION}/klfapp"
	"Frameworks/klfbackend.framework/Versions/${KLF_LIB_VERSION}/klfbackend"
	"${CMAKE_CURRENT_BINARY_DIR}/klfbackend/klfbackend.framework/Versions/${KLF_LIB_VERSION}/klfbackend")
  KLFMInstallNameToolChange(klatexformula
	"Frameworks/klfapp.framework/Versions/${KLF_LIB_VERSION}/klfapp"
	"Frameworks/klftools.framework/Versions/${KLF_LIB_VERSION}/klftools"
	"${CMAKE_CURRENT_BINARY_DIR}/klftools/klftools.framework/Versions/${KLF_LIB_VERSION}/klftools")
  # Setup klatexformula executable's lib dependencies
  KLFMBundlePrivateLibUpdateQtDep(klatexformula "MacOS/klatexformula-${KLF_VERSION}" "Core;Gui;Xml;Sql;DBus")
  KLFMInstallNameToolChange(klatexformula
	"MacOS/klatexformula-${KLF_VERSION}"
	"Frameworks/klfbackend.framework/Versions/${KLF_LIB_VERSION}/klfbackend"
	"${CMAKE_CURRENT_BINARY_DIR}/klfbackend/klfbackend.framework/Versions/${KLF_LIB_VERSION}/klfbackend")
  KLFMInstallNameToolChange(klatexformula
	"MacOS/klatexformula-${KLF_VERSION}"
	"Frameworks/klftools.framework/Versions/${KLF_LIB_VERSION}/klftools"
	"${CMAKE_CURRENT_BINARY_DIR}/klftools/klftools.framework/Versions/${KLF_LIB_VERSION}/klftools")
  KLFMInstallNameToolChange(klatexformula
	"MacOS/klatexformula-${KLF_VERSION}"
	"Frameworks/klfapp.framework/Versions/${KLF_LIB_VERSION}/klfapp"
	"${CMAKE_CURRENT_BINARY_DIR}/klfapp.framework/Versions/${KLF_LIB_VERSION}/klfapp")
  # Default Qt config stuff
  KLFMBundlePrivateImport(klatexformula
			qt.conf "${CMAKE_CURRENT_SOURCE_DIR}/macosx/qt.conf" Resources)
  # Add a few icons for our app
  KLFMBundlePrivateImport(klatexformula
	klficon.icns "${CMAKE_CURRENT_SOURCE_DIR}/macosx/klficon.icns" Resources)
  KLFMBundlePrivateImport(klatexformula
	klatexformula-128.png "${CMAKE_CURRENT_SOURCE_DIR}/klatexformula-128.png" Resources)
  KLFMBundlePrivateImport(klatexformula
	klatexformula-64.png "${CMAKE_CURRENT_SOURCE_DIR}/klatexformula-64.png" Resources)
  KLFMBundlePrivateImport(klatexformula
	klatexformula-32.png "${CMAKE_CURRENT_SOURCE_DIR}/klatexformula-32.png" Resources)
  KLFMBundlePrivateImport(klatexformula
	klatexformula-16.png "${CMAKE_CURRENT_SOURCE_DIR}/klatexformula-16.png" Resources)
  KLFCMakeDebug("Extras to be bundled are ${klatexformula_BUNDLEXTRA}")
  KLFMBundlePackage(klatexformula)
endif(KLF_MACOSX_BUNDLES)



if(KLF_BUILD_PLUGINS)
  # Build Plugins
  add_subdirectory(plugins)
endif(KLF_BUILD_PLUGINS)



# Make man page
if(HELP2MAN)
  KLFGetTargetLocation(klatexformula_binary klatexformula)
  # generate the man page
  set(LDPATHDEFS "export LD_LIBRARY_PATH='${CMAKE_CURRENT_BINARY_DIR}:${CMAKE_CURRENT_BINARY_DIR}/klfbackend:${CMAKE_CURRENT_BINARY_DIR}/klftools':\"$LD_LIBRARY_PATH\" ")
  add_custom_command(OUTPUT "klatexformula.1"
    COMMAND
      "sh" "-c" "${LDPATHDEFS}; ${HELP2MAN} -h --help='\"&1\"' -v --version='\"&1:klatexformula %k\"' --no-info ${klatexformula_binary} -o '${CMAKE_CURRENT_BINARY_DIR}/klatexformula.1'"
    COMMENT "Generating man page for klatexformula"
    DEPENDS "${klatexformula_binary}"
    VERBATIM
    )
  if(GZIP)
    add_custom_command(OUTPUT "klatexformula.1.gz"
      COMMAND "${CMAKE_COMMAND}" -E copy "klatexformula.1" "klatexformula.1.bkp"
      COMMAND "${GZIP}" -f "klatexformula.1.bkp"
      COMMAND "${CMAKE_COMMAND}" -E copy "klatexformula.1.bkp.gz" "klatexformula.1.gz"
      WORKING_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}"
      COMMENT "Compressing (gzip) manpage"
      DEPENDS "klatexformula.1"
      VERBATIM
      )
    set(klatexformula_manpage_file "klatexformula.1.gz")
    set(klatexformula_cmdl_manpage_file "klatexformula_cmdl.1.gz")
  else(GZIP)
    set(klatexformula_manpage_file "klatexformula.1")
    set(klatexformula_cmdl_manpage_file "klatexformula_cmdl.1")
  endif(GZIP)

  add_custom_command(OUTPUT "${klatexformula_cmdl_manpage_file}"
    COMMAND "${CMAKE_COMMAND}" -E copy "${klatexformula_manpage_file}" "${klatexformula_cmdl_manpage_file}"
    WORKING_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}"
    COMMENT "Copying ${klatexformula_manpage_file} to ${klatexformula_cmdl_manpage_file}"
    DEPENDS "${klatexformula_manpage_file}"
    VERBATIM
    )

  add_custom_target(klatexformula_manpage ALL
    DEPENDS "${klatexformula_manpage_file}" "${klatexformula_cmdl_manpage_file}")

  if(KLF_INSTALL_DESKTOP AND KLF_INSTALL_SHARE_MAN1_DIR)
    install(FILES
      "${CMAKE_CURRENT_BINARY_DIR}/${klatexformula_manpage_file}"
      "${CMAKE_CURRENT_BINARY_DIR}/${klatexformula_cmdl_manpage_file}"
      DESTINATION "${KLF_INSTALL_SHARE_MAN1_DIR}")
  endif(KLF_INSTALL_DESKTOP AND KLF_INSTALL_SHARE_MAN1_DIR)

endif(HELP2MAN)


# Install Targets
# ---------------

# klfapp

KLFInstallLibrary(klfapp KLF_INSTALL_KLFAPP_ "${KLF_INSTALL_LIB_DIR}" "include/klfapp")
if(KLF_MACOSX_BUNDLES AND KLF_INSTALL_KLFAPP_FRAMEWORK)
  # Installed library's ID
  KLFMInstFrameworkUpdateId("${KLF_INSTALL_LIB_DIR}/klfapp.framework/Versions/${KLF_LIB_VERSION}/klfapp")
  # Installed library's dependency on klfbackend should be updated
  KLFMInstFrameworkUpdateLibChange(
	"${KLF_INSTALL_LIB_DIR}/klfapp.framework/Versions/${KLF_LIB_VERSION}/klfapp"
	"${CMAKE_CURRENT_BINARY_DIR}/klfbackend/klfbackend.framework/Versions/${KLF_LIB_VERSION}/klfbackend"
	"${KLF_INSTALL_LIB_DIR}/klfbackend.framework/Versions/${KLF_LIB_VERSION}/klfbackend"
  )
  # Installed library's dependency on klftools should be updated
  KLFMInstFrameworkUpdateLibChange(
	"${KLF_INSTALL_LIB_DIR}/klfapp.framework/Versions/${KLF_LIB_VERSION}/klfapp"
	"${CMAKE_CURRENT_BINARY_DIR}/klftools/klftools.framework/Versions/${KLF_LIB_VERSION}/klftools"
	"${KLF_INSTALL_LIB_DIR}/klftools.framework/Versions/${KLF_LIB_VERSION}/klftools"
  )
endif(KLF_MACOSX_BUNDLES AND KLF_INSTALL_KLFAPP_FRAMEWORK)

if(WIN32 AND KLF_INSTALL_QTLIBS)
  # Install Qt libs
  #  QT_BINARY_DIR and QT_PLUGINS_DIR are set by FindQt4 CMake module
  install(DIRECTORY "${QT_BINARY_DIR}/" DESTINATION "${KLF_INSTALL_LIB_DIR}"
	    FILES_MATCHING REGEX "/Qt(Core|Gui|Sql|Xml)4?\\.dll$" REGEX "/libgcc.*\\.dll" REGEX "/mingw.*\\.dll"
	 )
elseif(KLF_INSTALL_QTLIBS)
  # Install Prereq libs
  message(ERROR "Not supported on non-windows system. TODO: write me. Actually, write a more general
    framework to import needed libraries, which could also replace the win32-specific code.")
endif(WIN32 AND KLF_INSTALL_QTLIBS)
if(KLF_INSTALL_QTPLUGINS)
  set(qtplugin_list ${KLF_INSTALL_QTPLUGINS_LIST})
  set(qtplugin_list "dummyitem" ${qtplugin_list})
  string(REGEX REPLACE ";" ";${QT_PLUGINS_DIR}/" qtplugin_list_temp "${qtplugin_list}")
  string(REGEX REPLACE "^dummyitem;" "" abs_qtplugin_list "${qtplugin_list_temp}")

  #install(FILES ${abs_qtplugin_list} DESTINATION "${KLF_INSTALL_QTPLUGINS_DIR}")
  foreach(qtplugin ${KLF_INSTALL_QTPLUGINS_LIST})
    get_filename_component(qtplugin_dir "${qtplugin}" PATH)
    install(FILES "${QT_PLUGINS_DIR}/${qtplugin}"
	    DESTINATION "${KLF_INSTALL_QTPLUGINS_DIR}/${qtplugin_dir}")
  endforeach(qtplugin)

  #- # install klftools_desplugin plugin too, in order to load user scripts' input widget with QUiLoader
  #- # TODO: TEST!!!
  #- KLFGetTargetLocation(klf_desplugin_target_path klftoolsdesplugin)
  #- install(FILES "${klf_desplugin_target_path}"
  #-   DESTINATION "${KLF_INSTALL_QTPLUGINS_DIR}/designer/")

endif(KLF_INSTALL_QTPLUGINS)


# klatexformula bin

if(NOT KLF_MACOSX_BUNDLES)
  if(KLF_INSTALL_KLATEXFORMULA_BIN)
    install(TARGETS klatexformula RUNTIME DESTINATION "${KLF_INSTALL_BIN_DIR}")
  endif(KLF_INSTALL_KLATEXFORMULA_BIN)
  if(UNIX AND KLF_INSTALL_KLATEXFORMULA_CMDL)
    install(CODE
	  "message(STATUS \"Creating Symlink [\$ENV{DESTDIR}]${KLF_ABS_INSTALL_BIN_DIR}/klatexformula_cmdl -> klatexformula\")
	  execute_process(COMMAND \"${CMAKE_COMMAND}\" -E create_symlink \"klatexformula\" \"\$ENV{DESTDIR}${KLF_ABS_INSTALL_BIN_DIR}/klatexformula_cmdl\")"
    )
  elseif(WIN32 AND KLF_INSTALL_KLATEXFORMULA_CMDL)
    install(TARGETS klatexformula_cmdl RUNTIME DESTINATION "${KLF_INSTALL_BIN_DIR}")
  endif(UNIX AND KLF_INSTALL_KLATEXFORMULA_CMDL)
endif(NOT KLF_MACOSX_BUNDLES)
if(KLF_MACOSX_BUNDLES AND KLF_INSTALL_KLATEXFORMULA_BUNDLE)
  install(TARGETS klatexformula BUNDLE DESTINATION "${KLF_INSTALL_BIN_DIR}")
  # Update klatexformula's dependency on the installed frameworks.
  KLFMInstFrameworkUpdateLibChange(
  "${KLF_INSTALL_BIN_DIR}/klatexformula-${KLF_VERSION}/Contents/MacOS/klatexformula-${KLF_VERSION}"
  "${CMAKE_CURRENT_BINARY_DIR}/klfbackend/klfbackend.framework/Versions/${KLF_LIB_VERSION}/klfbackend"
  "${KLF_INSTALL_LIB_DIR}/klfbackend.framework/Versions/${KLF_LIB_VERSION}/klfbackend"
  )
  KLFMInstFrameworkUpdateLibChange(
  "${KLF_INSTALL_BIN_DIR}/klatexformula-${KLF_VERSION}/Contents/MacOS/klatexformula-${KLF_VERSION}"
  "${CMAKE_CURRENT_BINARY_DIR}/klftools.framework/Versions/${KLF_LIB_VERSION}/klftools"
  "${KLF_INSTALL_LIB_DIR}/klftools.framework/Versions/${KLF_LIB_VERSION}/klftools"
  )
  KLFMInstFrameworkUpdateLibChange(
  "${KLF_INSTALL_BIN_DIR}/klatexformula-${KLF_VERSION}/Contents/MacOS/klatexformula-${KLF_VERSION}"
  "${CMAKE_CURRENT_BINARY_DIR}/klfapp.framework/Versions/${KLF_LIB_VERSION}/klfapp"
  "${KLF_INSTALL_LIB_DIR}/klfapp.framework/Versions/${KLF_LIB_VERSION}/klfapp"
  )
endif(KLF_MACOSX_BUNDLES AND KLF_INSTALL_KLATEXFORMULA_BUNDLE)


# --- LINUX DESKTOP FILES ---

if(KLF_INSTALL_DESKTOP)
  configure_file(
    "${CMAKE_SOURCE_DIR}/src/klatexformula.desktop.in"
    "${CMAKE_BINARY_DIR}/src/klatexformula.desktop"
    @ONLY)

  if(KLF_INSTALL_SHARE_APPLICATIONS_DIR)
    install(FILES "${CMAKE_BINARY_DIR}/src/klatexformula.desktop"
      DESTINATION "${KLF_INSTALL_SHARE_APPLICATIONS_DIR}"
      )
  endif(KLF_INSTALL_SHARE_APPLICATIONS_DIR)
  if(KLF_INSTALL_SHARE_PIXMAPS_DIR)
    install(FILES
      klatexformula-128.png
      klatexformula-64.png
      klatexformula-32.png
      klatexformula-16.png
      DESTINATION "${KLF_INSTALL_SHARE_PIXMAPS_DIR}"
      )
  endif(KLF_INSTALL_SHARE_PIXMAPS_DIR)
  if(KLF_INSTALL_ICON_THEME)
    install(FILES klatexformula-128.png
      DESTINATION "${KLF_INSTALL_ICON_THEME}/128x128/apps/" RENAME "klatexformula.png")
    install(FILES klatexformula-64.png
      DESTINATION "${KLF_INSTALL_ICON_THEME}/64x64/apps/" RENAME "klatexformula.png")
    install(FILES klatexformula-32.png
      DESTINATION "${KLF_INSTALL_ICON_THEME}/32x32/apps/" RENAME "klatexformula.png")
    install(FILES klatexformula-16.png
      DESTINATION "${KLF_INSTALL_ICON_THEME}/16x16/apps/" RENAME "klatexformula.png")
  endif(KLF_INSTALL_ICON_THEME)
  if(KLF_INSTALL_SHARE_MIME_PACKAGES_DIR)
    install(FILES "${CMAKE_SOURCE_DIR}/src/klatexformula-mime.xml"
      DESTINATION "${KLF_INSTALL_SHARE_MIME_PACKAGES_DIR}")
  endif(KLF_INSTALL_SHARE_MIME_PACKAGES_DIR)

  if(KLF_INSTALL_POST_UPDATEMIMEDATABASE)
    install(CODE
      "message(STATUS \"Updating Mime Types Database (update-mime-database)\")
	execute_process(COMMAND update-mime-database \"${CMAKE_INSTALL_PREFIX}/share/mime\")"
    )
  endif(KLF_INSTALL_POST_UPDATEMIMEDATABASE)
endif(KLF_INSTALL_DESKTOP)


if(NOT KLF_MACOSX_BUNDLES AND KLF_USE_CPACK)
  configure_file(
    "${CMAKE_SOURCE_DIR}/src/klatexformula.sh.in"
    "${CMAKE_BINARY_DIR}/src/klatexformula.sh"
    @ONLY)
  install(PROGRAMS "${CMAKE_BINARY_DIR}/src/klatexformula.sh" DESTINATION /)
endif(NOT KLF_MACOSX_BUNDLES AND KLF_USE_CPACK)

