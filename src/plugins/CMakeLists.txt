# ################################################ #
# CMake project file for klatexformula/src/plugins #
# ################################################ #
# $Id$
# ################################################ #


# We are no longer compiling KLF source !
remove_definitions(-DKLF_SRC_BUILD)

if(WIN32)
  # Windows
  set(KLF_CMAKE_PLUGINS_LIBNAME_SKIN skin.dll)
  set(KLF_CMAKE_PLUGINS_LIBNAME_SYSTRAYICON systrayicon.dll)
elseif(APPLE)
  # Apple Mac OS X
  set(KLF_CMAKE_PLUGINS_LIBNAME_SKIN libskin.so)
  set(KLF_CMAKE_PLUGINS_LIBNAME_SYSTRAYICON libsystrayicon.so)
else(WIN32)
  # Linux/Unix
  set(KLF_CMAKE_PLUGINS_LIBNAME_SKIN libskin.so)
  set(KLF_CMAKE_PLUGINS_LIBNAME_SYSTRAYICON libsystrayicon.so)
endif(WIN32)

configure_file(
  "${CMAKE_CURRENT_SOURCE_DIR}/klfbaseplugins.qrc.in"
  "${CMAKE_CURRENT_BINARY_DIR}/klfbaseplugins.qrc"
  IMMEDIATE @ONLY)
# Fancy way to copy a file...
configure_file(
  "${CMAKE_CURRENT_SOURCE_DIR}/info_baseplugins.xml"
  "${CMAKE_CURRENT_BINARY_DIR}/info_baseplugins.xml"
  IMMEDIATE @ONLY)


add_subdirectory(skin)
add_subdirectory(systrayicon)

add_custom_command(OUTPUT ${KLF_CMAKE_PLUGINS_LIBNAME_SKIN}
	COMMAND "${CMAKE_COMMAND}" -E copy "skin/${KLF_CMAKE_PLUGINS_LIBNAME_SKIN}"
		 "${KLF_CMAKE_PLUGINS_LIBNAME_SKIN}"
        DEPENDS skin
	VERBATIM
)
add_custom_command(OUTPUT ${KLF_CMAKE_PLUGINS_LIBNAME_SYSTRAYICON}
        COMMAND "${CMAKE_COMMAND}" -E copy "systrayicon/${KLF_CMAKE_PLUGINS_LIBNAME_SYSTRAYICON}" 
                 "${KLF_CMAKE_PLUGINS_LIBNAME_SYSTRAYICON}"
        DEPENDS systrayicon
        VERBATIM
)
if(APPLE AND KLF_MACOSX_BUNDLE_PATH)
  add_custom_target(klfbaseplugins_maclibpacked
  )
  add_dependencies(klfbaseplugins_maclibpacked ${KLF_CMAKE_PLUGINS_LIBNAME_SKIN} ${KLF_CMAKE_PLUGINS_LIBNAME_SYSTRAYICON})

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

  add_custom_command(OUTPUT klfbaseplugins.rcc
    COMMAND ${QT_RCC_EXECUTABLE} -binary -o klfbaseplugins.rcc klfbaseplugins.qrc
    WORKING_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}"
    COMMENT "Archiving Base Plugins into Add-On File"
    VERBATIM
  )
else(APPLE AND KLF_MACOSX_BUNDLE_PATH)
  add_custom_command(OUTPUT klfbaseplugins.rcc
    COMMAND ${QT_RCC_EXECUTABLE} -binary -o klfbaseplugins.rcc klfbaseplugins.qrc
    WORKING_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}"
    DEPENDS ${KLF_CMAKE_PLUGINS_LIBNAME_SKIN} ${KLF_CMAKE_PLUGINS_LIBNAME_SYSTRAYICON}
    COMMENT "Archiving Base Plugins into Add-On File"
    VERBATIM
  )
endif(APPLE AND KLF_MACOSX_BUNDLE_PATH)
add_custom_target(klfbaseplugins_rcc ALL
	COMMAND ${QT_RCC_EXECUTABLE} -binary -o klfbaseplugins.rcc klfbaseplugins.qrc
	WORKING_DIRECTORY  "${CMAKE_CURRENT_BINARY_DIR}"
    DEPENDS ${KLF_CMAKE_PLUGINS_LIBNAME_SKIN} ${KLF_CMAKE_PLUGINS_LIBNAME_SYSTRAYICON}
	COMMENT  "Archiving Base Plugins into Add-On File"
	VERBATIM
)
if(APPLE AND KLF_MACOSX_BUNDLE_PATH)
  add_dependencies(klfbaseplugins_rcc klfbaseplugins_maclibpacked)
  add_custom_command(TARGET klfbaseplugins_rcc POST_BUILD
      COMMAND cp "${CMAKE_CURRENT_BINARY_DIR}/klfbaseplugins.rcc"
		 "${KLF_MACOSX_BUNDLE_PATH}/Contents/Resources/rccresources/klfbaseplugins.rcc"
      COMMENT "Installing base plugins into application bundle"   VERBATIM)
endif(APPLE AND KLF_MACOSX_BUNDLE_PATH)

if(KLF_DEVEL_LOCAL_BASEPLUGINS_COPY)
  add_custom_target(klfbaseplugins_rcc_localinstall ALL
    COMMAND "${CMAKE_COMMAND}" -E copy klfbaseplugins.rcc "$ENV{HOME}/.klatexformula/rccresources/"
    DEPENDS klfbaseplugins.rcc
    COMMENT "Installing Base Plugins Add-On Locally in ~/.klatexformula/rccresources"
    VERBATIM
)
endif(KLF_DEVEL_LOCAL_BASEPLUGINS_COPY)

install(FILES klfbaseplugins.rcc DESTINATION share/klatexformula/rccresources)


