# ################################################ #
# CMake project file for klatexformula/src/plugins #
# ################################################ #
# $Id$
# ################################################ #


# We are no longer compiling KLF source !
remove_definitions(-DKLF_SRC_BUILD)


# The subdir in which to put plugins
set(KLF_CMAKE_BASEPLUGINSSUBDIR "${KLF_CMAKE_OS}-${KLF_CMAKE_ARCH}"
					CACHE INTERNAL "(internal) Base Plugins RCC sub-dir")

# THE PLUGIN TARGET/BASE NAMES

set(KLF_LOCAL_PLUGINS skin systrayicon)

add_custom_target(klfbaseplugins ALL)

# Setup per plugin
foreach(plugin ${KLF_LOCAL_PLUGINS})

  # Add the subdirectory
  add_subdirectory("${plugin}")

  # And set up a few more stuff
  string(TOUPPER "${plugin}" plugin_upper)
  KLFGetTargetFileName(plugin_libname ${plugin})
  set(KLF_CMAKE_PLUGINS_LIBNAME_${plugin_upper} "${plugin_libname}")
  set(KLF_CMAKE_PLUGINS_LOCAL_LIBNAME_${plugin_upper} "${CMAKE_CURRENT_BINARY_DIR}/${KLF_CMAKE_PLUGINS_LIBNAME_${plugin_upper}}")
  add_custom_command(OUTPUT "${plugin_libname}"
	COMMAND "${CMAKE_COMMAND}" -E copy "${plugin}/${plugin_libname}" "${plugin_libname}"
	WORKING_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}"
        DEPENDS "${plugin}"
	COMMENT "Copying ${plugin_libname} to build root plugins/"
	VERBATIM
  )
  set(KLF_LOCAL_PLUGIN_LIBNAMES ${KLF_LOCAL_PLUGIN_LIBNAMES} ${plugin_libname})

  set(KLF_CMAKE_PLUGINS_QRC_DEFS
    "${KLF_CMAKE_PLUGINS_QRC_DEFS} <file>${plugin_libname}</file>"
  )

endforeach(plugin)



if(APPLE AND KLF_MACOSX_BUNDLE_PATH)
  KLFMakeMacExtraBundledTarget(klfbaseplugins "${KLF_LOCAL_PLUGIN_LIBNAMES}")

  foreach(plugin_libname ${KLF_LOCAL_PLUGIN_LIBNAMES})
    KLFBundlePrivateLibUpdateQtDep(klfbaseplugins "${KLF_MACOSX_BUNDLE_PATH}"
	"../../plugins/${plugin_libname}" "Core;Gui;Xml;Sql;DBus")
    KLFInstallNameToolChange(klfbaseplugins "../../plugins/${plugin_libname}"
        "${KLF_MACOSX_BUNDLE_PATH}" "Frameworks/klfbackend.framework/Versions/${KLF_LIB_VERSION}/klfbackend"
        "${CMAKE_BINARY_DIR}/src/klfbackend/klfbackend.framework/Versions/${KLF_LIB_VERSION}/klfbackend")
    KLFInstallNameToolChange(klfbaseplugins "../../plugins/${plugin_libname}"
	"${KLF_MACOSX_BUNDLE_PATH}" "Frameworks/klftools.framework/Versions/${KLF_LIB_VERSION}/klftools"
	"${CMAKE_BINARY_DIR}/src/klftools/klftools.framework/Versions/${KLF_LIB_VERSION}/klftools")
    KLFInstallNameToolChange(klfbaseplugins "../../plugins/${plugin_libname}"
        "${KLF_MACOSX_BUNDLE_PATH}" "Frameworks/klfapp.framework/Versions/${KLF_LIB_VERSION}/klfapp"
        "${CMAKE_BINARY_DIR}/src/klfapp.framework/Versions/${KLF_LIB_VERSION}/klfapp")
  endforeach(plugin_libname)

endif(APPLE AND KLF_MACOSX_BUNDLE_PATH)


configure_file(
  "${CMAKE_CURRENT_SOURCE_DIR}/plugindirinfo.xml.in"
  "${CMAKE_CURRENT_BINARY_DIR}/plugindirinfo.xml"
  @ONLY)

configure_file(
  "${CMAKE_CURRENT_SOURCE_DIR}/info_baseplugins.xml.in"
  "${CMAKE_CURRENT_BINARY_DIR}/info_baseplugins.xml"
  @ONLY)

configure_file(
  "${CMAKE_CURRENT_SOURCE_DIR}/klfbaseplugins.qrc.in"
  "${CMAKE_CURRENT_BINARY_DIR}/klfbaseplugins.qrc"
  @ONLY)


set(KLF_BASEPLUGINS_FN_RCC "klfbaseplugins-${KLF_VERSION}.rcc")

add_custom_command(OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/${KLF_BASEPLUGINS_FN_RCC}"
	COMMAND "${QT_RCC_EXECUTABLE}" -binary -o ${KLF_BASEPLUGINS_FN_RCC} klfbaseplugins.qrc
	WORKING_DIRECTORY  "${CMAKE_CURRENT_BINARY_DIR}"
	COMMENT  "Archiving Base Plugins into Add-On File"
	DEPENDS ${KLF_LOCAL_PLUGIN_LIBNAMES}
	  "${CMAKE_CURRENT_BINARY_DIR}/info_baseplugins.xml"
	  "${CMAKE_CURRENT_BINARY_DIR}/plugindirinfo.xml"
	  "${CMAKE_CURRENT_BINARY_DIR}/klfbaseplugins.qrc"
	VERBATIM
)
add_custom_target(klfbaseplugins_rcc ALL DEPENDS "${CMAKE_CURRENT_BINARY_DIR}/${KLF_BASEPLUGINS_FN_RCC}")


if(APPLE AND KLF_MACOSX_BUNDLE_PATH)
  add_dependencies(klfbaseplugins_rcc klfbaseplugins_maclibpacked)
  add_custom_command(TARGET klfbaseplugins_rcc POST_BUILD
      COMMAND cp "${CMAKE_CURRENT_BINARY_DIR}/${KLF_BASEPLUGINS_FN_RCC}"
		 "${KLF_MACOSX_BUNDLE_PATH}/Contents/Resources/rccresources/${KLF_BASEPLUGINS_FN_RCC}"
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
  COMMAND "${CMAKE_COMMAND}" -E copy "${KLF_BASEPLUGINS_FN_RCC}" "${homePath}/.klatexformula/rccresources/"
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
  install(FILES "${CMAKE_CURRENT_BINARY_DIR}/${KLF_BASEPLUGINS_FN_RCC}"
	  DESTINATION "${KLF_INSTALL_RCCRESOURCES_DIR}")
endif(KLF_INSTALL_PLUGINS)


