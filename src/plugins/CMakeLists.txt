# ################################################ #
# CMake project file for klatexformula/src/plugins #
# ################################################ #
# $Id$
# ################################################ #


# We are no longer compiling KLF source !
remove_definitions(-DKLF_SRC_BUILD)


# The subdir in which to put plugins in RCC
#set(KLF_CMAKE_BASEPLUGINSSUBDIR "${KLF_CMAKE_OS}-${KLF_CMAKE_ARCH}"
#					CACHE INTERNAL "(internal) Base Plugins RCC sub-dir")

# THE PLUGIN TARGET/BASE NAMES

set(KLF_LOCAL_PLUGINS skin systrayicon buffers)
if(KLF_EXPERIMENTAL)
  # set(KLF_LOCAL_PLUGINS ${KLF_LOCAL_PLUGINS} wikiscan)
endif(KLF_EXPERIMENTAL)


add_custom_target(klfbaseplugins ALL)

#
# e.g. Usage: KLFPluginUndefOrLink(skin  libklftools libklfbackend libklfapp)
#
macro(KLFPluginUndefOrLink  plugin)
  KLFCMakeDebug("KLFPluginUndefOrLink: plugin=${plugin}")
  if(UNIX)  #includes Apple
    KLFCMakeDebug("UNIX.")
    KLFNoShlibUndefined("${plugin}")
  else(UNIX)
    KLFCMakeDebug("NOT UNIX.")
    target_link_libraries("${plugin}" ${ARGN})
  endif(UNIX)
endmacro(KLFPluginUndefOrLink)

# local for-developers-only compile-time install
if(KLF_DEVEL_LOCAL_BASEPLUGINS_COPY)
  file(MAKE_DIRECTORY
    "$ENV{HOME}/.klatexformula/plugins/sysarch_${KLF_CMAKE_OS}:${KLF_CMAKE_ARCH}/klf${KLF_VERSION}/${plugin_libname}"
    )
endif(KLF_DEVEL_LOCAL_BASEPLUGINS_COPY)

macro(KLFInstallPlugin plugin)
  add_dependencies(klfbaseplugins ${plugin})
  KLFCMakeDebug("KLFInstallPlugin: plugin=${plugin}")
  if(KLF_INSTALL_PLUGINS)
    install(TARGETS ${plugin}
      RUNTIME DESTINATION "${KLF_INSTALL_PLUGINS_DIR}"
      LIBRARY DESTINATION "${KLF_INSTALL_PLUGINS_DIR}"
      ARCHIVE DESTINATION "${KLF_INSTALL_PLUGINS_DIR}"
      )
  endif(KLF_INSTALL_PLUGINS)

  # local for-developers-only compile-time install
  if(KLF_DEVEL_LOCAL_BASEPLUGINS_COPY)
    KLFGetTargetLocation(plugin_location ${plugin})
    KLFGetTargetFileName(plugin_libname ${plugin})
    add_custom_command(TARGET ${plugin}
      POST_BUILD
      COMMAND "${CMAKE_COMMAND}" -E copy "${plugin_location}" "$ENV{HOME}/.klatexformula/plugins/sysarch_${KLF_CMAKE_OS}:${KLF_CMAKE_ARCH}/klf${KLF_VERSION}/${plugin_libname}"
      COMMENT "Installing plugin ${plugin} to ~/.klatexformula/plugins/ ... (as requested by KLF_DEVEL_LOCAL_BASEPLUGINS_COPY)"
      VERBATIM
      )
  endif(KLF_DEVEL_LOCAL_BASEPLUGINS_COPY)
endmacro(KLFInstallPlugin)

# Setup per plugin
foreach(plugin ${KLF_LOCAL_PLUGINS})

  # Add the subdirectory
  add_subdirectory("${plugin}")

  # And set up a few more stuff
  string(TOUPPER "${plugin}" plugin_upper)
  KLFGetTargetLocation(plugin_location ${plugin})
  KLFGetTargetFileName(plugin_libname ${plugin})
  set(KLF_CMAKE_PLUGINS_LOCATION_${plugin_upper} "${plugin_location}")
  set(KLF_CMAKE_PLUGINS_LIBNAME_${plugin_upper} "${plugin_libname}")
  #set(KLF_CMAKE_PLUGINS_LOCAL_LIBNAME_${plugin_upper}
  #  "${CMAKE_CURRENT_BINARY_DIR}/${KLF_CMAKE_PLUGINS_LIBNAME_${plugin_upper}}")
  #add_custom_command(OUTPUT "${plugin_libname}"
  #	COMMAND "${CMAKE_COMMAND}" -E copy "${plugin_location}" "${plugin_libname}"
  #	WORKING_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}"
  #	DEPENDS "${plugin}"
  #	COMMENT "Copying ${plugin_libname} to build root plugins/"
  #	VERBATIM
  #  )
  set(KLF_LOCAL_PLUGIN_LIBNAMES ${KLF_LOCAL_PLUGIN_LIBNAMES} ${plugin_libname})

#  set(KLF_CMAKE_PLUGINS_QRC_DEFS
#    "${KLF_CMAKE_PLUGINS_QRC_DEFS} <file>${plugin_libname}</file>"
#  )

endforeach(plugin)




if(APPLE AND KLF_MACOSX_BUNDLES)
  KLFMMakeMacExtraBundledTarget(klfbaseplugins klatexformula "${KLF_LOCAL_PLUGIN_LIBNAMES}")

  foreach(plugin ${KLF_LOCAL_PLUGINS})
    string(TOUPPER "${plugin}" plugin_upper)
    set(plugin_location "${KLF_CMAKE_PLUGINS_LOCATION_${plugin_upper}}")
    set(plugin_libname "${KLF_CMAKE_PLUGINS_LIBNAME_${plugin_upper}}")

    if(KLF_MACOSX_BUNDLE_EXTRAS_klatexformula)
      KLFMBundlePrivateLibUpdateQtDep(klfbaseplugins
	"${plugin_location}" "Core;Gui;Xml;Sql;DBus")
    endif(KLF_MACOSX_BUNDLE_EXTRAS_klatexformula)
    KLFMInstallNameToolChange(klfbaseplugins "${plugin_location}"
        "Frameworks/klfbackend.framework/Versions/${KLF_LIB_VERSION}/klfbackend"
        "${CMAKE_BINARY_DIR}/src/klfbackend/klfbackend.framework/Versions/${KLF_LIB_VERSION}/klfbackend")
    KLFMInstallNameToolChange(klfbaseplugins "${plugin_location}"
	"Frameworks/klftools.framework/Versions/${KLF_LIB_VERSION}/klftools"
	"${CMAKE_BINARY_DIR}/src/klftools/klftools.framework/Versions/${KLF_LIB_VERSION}/klftools")
    KLFMInstallNameToolChange(klfbaseplugins "${plugin_location}"
        "Frameworks/klfapp.framework/Versions/${KLF_LIB_VERSION}/klfapp"
        "${CMAKE_BINARY_DIR}/src/klfapp.framework/Versions/${KLF_LIB_VERSION}/klfapp")

    # install plugin into bundle
    add_custom_command(TARGET klfbaseplugins POST_BUILD
      COMMAND mkdir -p
      "${klfbundlelocation_klatexformula}/Contents/Resources/plugins/sysarch_${KLF_CMAKE_OS}:${KLF_CMAKE_ARCH}/klf${KLF_VERSION}/"
      COMMAND cp "${plugin_location}"
      "${klfbundlelocation_klatexformula}/Contents/Resources/plugins/sysarch_${KLF_CMAKE_OS}:${KLF_CMAKE_ARCH}/klf${KLF_VERSION}/${plugin_libname}"
      VERBATIM
      )

  endforeach(plugin)


  # install user scripts
  add_custom_command(TARGET klfbaseplugins POST_BUILD
    COMMAND  mkdir -p
    "${klfbundlelocation_klatexformula}/Contents/Resources/userscripts"
    COMMAND  mkdir -p
    "${klfbundlelocation_klatexformula}/Contents/Resources/userscriptdata"
    COMMAND rsync -a "${CMAKE_CURRENT_SOURCE_DIR}/userscripts/"
    "${klfbundlelocation_klatexformula}/Contents/Resources/userscripts/"
    --exclude *~ --exclude .svn/
    COMMAND rsync -a "${CMAKE_CURRENT_SOURCE_DIR}/userscriptdata/"
    "${klfbundlelocation_klatexformula}/Contents/Resources/userscriptdata/"
    --exclude *~ --exclude .svn/
    VERBATIM
    )
    
#  # add klfbaseplugins.rcc because we need the config UI files for the user scripts
#  add_dependencies(klfbaseplugins_rcc klatexformula klfbaseplugins_maclibpacked)
#  add_custom_command(TARGET klfbaseplugins_rcc POST_BUILD
#      COMMAND cp "${CMAKE_CURRENT_BINARY_DIR}/${KLF_BASEPLUGINS_FN_RCC}"
#		 "${klfbundlelocation_klatexformula}/Contents/Resources/rccresources/${KLF_BASEPLUGINS_FN_RCC}"
#      COMMENT "Installing base plugin UI files into application bundle"
#      VERBATIM)

endif(APPLE AND KLF_MACOSX_BUNDLES)


#if(WIN32)
#  set(homePath "$ENV{USERPROFILE}")
#else(WIN32)
#  set(homePath "$ENV{HOME}")
#endif(WIN32)

#if(KLF_INSTALL_PLUGINS)
#  # The install target
#  install(FILES "${CMAKE_CURRENT_BINARY_DIR}/${KLF_BASEPLUGINS_FN_RCC}"
#	  DESTINATION "${KLF_INSTALL_RCCRESOURCES_DIR}")
#endif(KLF_INSTALL_PLUGINS)
