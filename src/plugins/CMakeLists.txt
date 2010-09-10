# ################################################ #
# CMake project file for klatexformula/src/plugins #
# ################################################ #
# $Id$
# ################################################ #


# We are no longer compiling KLF source !
remove_definitions(-DKLF_SRC_BUILD)


# Current Qt Version "MAJ.MIN"
set(KLF_CMAKE_QTVERSION "${QT_VERSION_MAJOR}.${QT_VERSION_MINOR}" CACHE INTERNAL "Qt Version")

# The subdir in which to put plugins
set(KLF_CMAKE_BASEPLUGINSSUBDIR "${KLF_CMAKE_OS}-${KLF_CMAKE_ARCH}"
					CACHE INTERNAL "(internal) Base Plugins RCC sub-dir")

add_subdirectory(skin)
add_subdirectory(systrayicon)


KLFGetTargetFileName(KLF_CMAKE_PLUGINS_LIBNAME_SKIN skin)
KLFGetTargetFileName(KLF_CMAKE_PLUGINS_LIBNAME_SYSTRAYICON systrayicon)

set(KLF_CMAKE_PLUGINS_LOCAL_LIBNAME_SKIN
		"${CMAKE_CURRENT_BINARY_DIR}/${KLF_CMAKE_PLUGINS_LIBNAME_SKIN}")
set(KLF_CMAKE_PLUGINS_LOCAL_LIBNAME_SYSTRAYICON
		"${CMAKE_CURRENT_BINARY_DIR}/${KLF_CMAKE_PLUGINS_LIBNAME_SYSTRAYICON}")

configure_file(
  "${CMAKE_CURRENT_SOURCE_DIR}/plugindirinfo.xml.in"
  "${CMAKE_CURRENT_BINARY_DIR}/plugindirinfo.xml"
  @ONLY)

configure_file(
  "${CMAKE_CURRENT_SOURCE_DIR}/klfbaseplugins.qrc.in"
  "${CMAKE_CURRENT_BINARY_DIR}/klfbaseplugins.qrc"
  @ONLY)


add_custom_command(OUTPUT ${KLF_CMAKE_PLUGINS_LOCAL_LIBNAME_SKIN}
	COMMAND "${CMAKE_COMMAND}" -E copy "skin/${KLF_CMAKE_PLUGINS_LIBNAME_SKIN}"
		 "${KLF_CMAKE_PLUGINS_LIBNAME_SKIN}"
	WORKING_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}"
        DEPENDS skin
	COMMENT "Copying ${KLF_CMAKE_PLUGINS_LIBNAME_SKIN} to build root plugins/"
	VERBATIM
)
add_custom_command(OUTPUT ${KLF_CMAKE_PLUGINS_LOCAL_LIBNAME_SYSTRAYICON}
        COMMAND "${CMAKE_COMMAND}" -E copy "systrayicon/${KLF_CMAKE_PLUGINS_LIBNAME_SYSTRAYICON}" 
                 "${KLF_CMAKE_PLUGINS_LIBNAME_SYSTRAYICON}"
	WORKING_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}"
        DEPENDS systrayicon
	COMMENT "Copying ${KLF_CMAKE_PLUGINS_LIBNAME_SYSTRAYICON} to build root plugins/"
        VERBATIM
)
if(APPLE AND KLF_MACOSX_BUNDLE_PATH)
  add_custom_target(klfbaseplugins_maclibpacked
    DEPENDS ${KLF_CMAKE_PLUGINS_LOCAL_LIBNAME_SKIN} ${KLF_CMAKE_PLUGINS_LOCAL_LIBNAME_SYSTRAYICON}
  )

  KLFBundlePrivateLibUpdateQtDep(klfbaseplugins "${KLF_MACOSX_BUNDLE_PATH}"
	"../../plugins/${KLF_CMAKE_PLUGINS_LIBNAME_SKIN}" "Core;Gui;Xml;Sql;DBus")
  KLFBundlePrivateLibUpdateQtDep(klfbaseplugins "${KLF_MACOSX_BUNDLE_PATH}"
        "../../plugins/${KLF_CMAKE_PLUGINS_LIBNAME_SYSTRAYICON}" "Core;Gui;Xml;Sql;DBus")
  KLFInstallNameToolChange(klfbaseplugins "../../plugins/${KLF_CMAKE_PLUGINS_LIBNAME_SKIN}"
        "${KLF_MACOSX_BUNDLE_PATH}" "Frameworks/klfbackend.framework/Versions/${KLF_LIB_VERSION}/klfbackend"
        "${CMAKE_BINARY_DIR}/src/klfbackend/klfbackend.framework/Versions/${KLF_LIB_VERSION}/klfbackend")
  KLFInstallNameToolChange(klfbaseplugins "../../plugins/${KLF_CMAKE_PLUGINS_LIBNAME_SKIN}"
	"${KLF_MACOSX_BUNDLE_PATH}" "Frameworks/klflib.framework/Versions/${KLF_LIB_VERSION}/klflib"
	"${CMAKE_BINARY_DIR}/src/klflib.framework/Versions/${KLF_LIB_VERSION}/klflib")
  KLFInstallNameToolChange(klfbaseplugins "../../plugins/${KLF_CMAKE_PLUGINS_LIBNAME_SYSTRAYICON}"
        "${KLF_MACOSX_BUNDLE_PATH}" "Frameworks/klfbackend.framework/Versions/${KLF_LIB_VERSION}/klfbackend"
        "${CMAKE_BINARY_DIR}/src/klfbackend/klfbackend.framework/Versions/${KLF_LIB_VERSION}/klfbackend")
  KLFInstallNameToolChange(klfbaseplugins "../../plugins/${KLF_CMAKE_PLUGINS_LIBNAME_SYSTRAYICON}"
        "${KLF_MACOSX_BUNDLE_PATH}" "Frameworks/klflib.framework/Versions/${KLF_LIB_VERSION}/klflib"
        "${CMAKE_BINARY_DIR}/src/klflib.framework/Versions/${KLF_LIB_VERSION}/klflib")

endif(APPLE AND KLF_MACOSX_BUNDLE_PATH)

add_custom_command(OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/klfbaseplugins.rcc"
	COMMAND "${QT_RCC_EXECUTABLE}" -binary -o klfbaseplugins.rcc klfbaseplugins.qrc
	WORKING_DIRECTORY  "${CMAKE_CURRENT_BINARY_DIR}"
	COMMENT  "Archiving Base Plugins into Add-On File"
	DEPENDS ${KLF_CMAKE_PLUGINS_LOCAL_LIBNAME_SKIN}
	  ${KLF_CMAKE_PLUGINS_LOCAL_LIBNAME_SYSTRAYICON}
	  "${CMAKE_CURRENT_SOURCE_DIR}/info_baseplugins.xml"
	  "${CMAKE_CURRENT_BINARY_DIR}/plugindirinfo.xml"
	  "${CMAKE_CURRENT_BINARY_DIR}/klfbaseplugins.qrc"
	VERBATIM
)
add_custom_target(klfbaseplugins_rcc ALL DEPENDS "${CMAKE_CURRENT_BINARY_DIR}/klfbaseplugins.rcc")


if(APPLE AND KLF_MACOSX_BUNDLE_PATH)
  add_dependencies(klfbaseplugins_rcc klfbaseplugins_maclibpacked)
  add_custom_command(TARGET klfbaseplugins_rcc POST_BUILD
      COMMAND cp "${CMAKE_CURRENT_BINARY_DIR}/klfbaseplugins.rcc"
		 "${KLF_MACOSX_BUNDLE_PATH}/Contents/Resources/rccresources/klfbaseplugins.rcc"
      COMMENT "Installing base plugins into application bundle"
      VERBATIM)
endif(APPLE AND KLF_MACOSX_BUNDLE_PATH)


if(WIN32)
  set(homePath "$ENV{USERPROFILE}")
else(WIN32)
  set(homePath "$ENV{HOME}")
endif(WIN32)
# local for-developers-only compile-time install
add_custom_target(klfbaseplugins_localinstall
  COMMAND "${CMAKE_COMMAND}" -E copy klfbaseplugins.rcc "${homePath}/.klatexformula/rccresources/"
  WORKING_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}"
  COMMENT "Installing Base Plugins Add-On Locally in ~/.klatexformula/rccresources"
  VERBATIM
)
add_dependencies(klfbaseplugins_localinstall klfbaseplugins_rcc)
if(KLF_DEVEL_LOCAL_BASEPLUGINS_COPY)
  add_custom_target(klfbaseplugins_localinstall_all ALL)
  add_dependencies(klfbaseplugins_localinstall_all klfbaseplugins_localinstall)
endif(KLF_DEVEL_LOCAL_BASEPLUGINS_COPY)


if(KLF_INSTALL_PLUGINS)
  # The install target
  install(FILES "${CMAKE_CURRENT_BINARY_DIR}/klfbaseplugins.rcc"
	  DESTINATION "${KLF_INSTALL_RCCRESOURCES_DIR}")
endif(KLF_INSTALL_PLUGINS)


