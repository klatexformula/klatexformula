# ################################################ #
# CMake project file for klatexformula/src/plugins #
# ################################################ #
# $Id$
# ################################################ #


# We are no longer compiling KLF source !
remove_definitions(-DKLF_SRC_BUILD)

if(WIN32)
  # Windows
  set(KLF_CMAKE_PLUGINS_LIBNAME_SKIN libskin.dll)
  set(KLF_CMAKE_PLUGINS_LIBNAME_SYSTRAYICON libsystrayicon.dll)
elseif(APPLE)
  # Apple Mac OS X
  set(KLF_CMAKE_PLUGINS_LIBNAME_SKIN libskin.so)
  set(KLF_CMAKE_PLUGINS_LIBNAME_SYSTRAYICON libsystrayicon.so)
else(WIN32)
  # Linux/Unix
  set(KLF_CMAKE_PLUGINS_LIBNAME_SKIN libskin.so)
  set(KLF_CMAKE_PLUGINS_LIBNAME_SYSTRAYICON libsystrayicon.so)
endif(WIN32)
set(KLF_CMAKE_PLUGINS_LOCAL_LIBNAME_SKIN
		"${CMAKE_CURRENT_BINARY_DIR}/${KLF_CMAKE_PLUGINS_LIBNAME_SKIN}")
set(KLF_CMAKE_PLUGINS_LOCAL_LIBNAME_SYSTRAYICON
		"${CMAKE_CURRENT_BINARY_DIR}/${KLF_CMAKE_PLUGINS_LIBNAME_SYSTRAYICON}")


configure_file(
  "${CMAKE_CURRENT_SOURCE_DIR}/klfbaseplugins.qrc.in"
  "${CMAKE_CURRENT_BINARY_DIR}/klfbaseplugins.qrc"
  IMMEDIATE @ONLY)

add_subdirectory(skin)
add_subdirectory(systrayicon)

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

add_custom_target(klfbaseplugins_rcc ALL
	COMMAND "${QT_RCC_EXECUTABLE}" -binary -o klfbaseplugins.rcc klfbaseplugins.qrc
	WORKING_DIRECTORY  "${CMAKE_CURRENT_BINARY_DIR}"
	COMMENT  "Archiving Base Plugins into Add-On File"
	DEPENDS ${KLF_CMAKE_PLUGINS_LOCAL_LIBNAME_SKIN} ${KLF_CMAKE_PLUGINS_LOCAL_LIBNAME_SYSTRAYICON}
	VERBATIM
)

if(APPLE AND KLF_MACOSX_BUNDLE_PATH)
  add_dependencies(klfbaseplugins_rcc klfbaseplugins_maclibpacked)
  add_custom_command(TARGET klfbaseplugins_rcc POST_BUILD
      COMMAND cp "${CMAKE_CURRENT_BINARY_DIR}/klfbaseplugins.rcc"
		 "${KLF_MACOSX_BUNDLE_PATH}/Contents/Resources/rccresources/klfbaseplugins.rcc"
      COMMENT "Installing base plugins into application bundle"
      VERBATIM)
endif(APPLE AND KLF_MACOSX_BUNDLE_PATH)


# local for-developers-only compile-time install
if(KLF_DEVEL_LOCAL_BASEPLUGINS_COPY)
  add_custom_target(klfbaseplugins_rcc_localinstall ALL
    COMMAND "${CMAKE_COMMAND}" -E copy klfbaseplugins.rcc "$ENV{HOME}/.klatexformula/rccresources/"
    DEPENDS klfbaseplugins.rcc
    COMMENT "Installing Base Plugins Add-On Locally in ~/.klatexformula/rccresources"
    VERBATIM
)
endif(KLF_DEVEL_LOCAL_BASEPLUGINS_COPY)


# The install target
install(FILES klfbaseplugins.rcc DESTINATION share/klatexformula/rccresources)


