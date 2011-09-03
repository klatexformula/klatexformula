# ########################################## #
# CMake definitions for external KLF plugins #
# ########################################## #
# $Id$
# ########################################## #


## ---- THIS FILE IS NOT USED, IT IS IN A EARLY DRAFT FORM. -----


# This CMake file should be used to compile external plugins for
# KLatexFormula. This file is NOT used for main build.


# Find Qt 4 and Include it
find_package(Qt4 4.4.0 COMPONENTS QtCore QtGui REQUIRED)

include(KLFUtil)


# We are NOT compiling KLF source ! Make sure this definition is removed.
remove_definitions(-DKLF_SRC_BUILD)




# Creating the RCC Add-On
# --------------------------------------------------

# The subdir in which to put plugins
if(NOT KLFPLUGIN_OSARCH)
  set(KLFPLUGIN_OSARCH "plugin-osarch-dir_TODO_CONFIGURE_ME" CACHE STRING "KLF Plugin RCC archive sub-dir")
endif(NOT KLFPLUGIN_OSARCH)


macro(KLFPluginAddOnRule targetname plugin plugindir plugindirinfofile)
  # Add rule to include this plugin into the add-on
  string(TOUPPER "${plugin}" plugin_upper)
  KLFGetTargetLocation(plugin_location ${plugin})
  KLFGetTargetFileName(plugin_libname ${plugin})

  KLFCMakeDebug("plugin_libname: ${plugin_libname}, plugin_location: ${plugin_location}")

  set(klfaddon_qrc_defs_${targetname}
    "${klfaddon_qrc_defs_${targetname}}
    <qresource prefix=\"/plugins/${plugindir}\"><file alias=\"${plugin_libname}\">${plugin_location}</file></qresource>"
    )
  list(APPEND klfaddon_deps_${targetname} ${plugin})

endforeach(plugin)

add_custom_target(klfaddon)

macro(KLFMakeAddOn targetname filename info_file)

  set(klfaddon_name_${targetname} "${filename}")
  set(klfaddon_plugindirinfofile_${targetname} "klfaddon_plugindirinfo_${targetname}.xml")
  set(klfaddon_qrc_head_${targetname} "<RCC><qresource prefix=\"rccinfo\">
    <file alias=\"info.xml\">${info_file}</file>
    </qresource>
    <qresource prefix=\"/plugins/\">
      <file alias=\"plugindirinfo.xml\">${klfaddon_plugindirinfofile_${targetname}}</file>
    </qresource>")
  set(klfaddon_qrc_tail_${targetname} "</RCC>")
  set(klfaddon_qrcfile_${targetname} "klfaddon_${targetname}.qrc")

  file(WRITE "${klfaddon_qrcfile_${targetname}}"
    "${klfaddon_qrc_head_${targetname}} ${klfaddon_qrc_defs_${targetname}} ${klfaddon_qrc_tail_${targetname}}")

  add_custom_command(OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/${filename}"
    COMMAND "${QT_RCC_EXECUTABLE}" -binary -o "${filename}" "${klfaddon_qrcfile_${targetname}}"
    WORKING_DIRECTORY  "${CMAKE_CURRENT_BINARY_DIR}"
    COMMENT  "Creating Add-On File ${filename}"
    DEPENDS ${klfaddon_deps_${targetname}} ${info_file}
    VERBATIM
    )
  add_custom_target(klfaddon_${targetname} DEPENDS "${filename}")
  add_dependencies(klfaddon  klfaddon_${targetname})

endmacro(KLFMakeAddOn)



if(APPLE AND KLF_MACOSX_BUNDLES)
  KLFMMakeMacExtraBundledTarget(klfbaseplugins klatexformula "${KLF_LOCAL_PLUGIN_LIBNAMES}")

  foreach(plugin_libname ${KLF_LOCAL_PLUGIN_LIBNAMES})
    if(KLF_MACOSX_BUNDLE_EXTRAS_klatexformula)
      KLFMBundlePrivateLibUpdateQtDep(klfbaseplugins
	"${CMAKE_CURRENT_BINARY_DIR}/${plugin_libname}" "Core;Gui;Xml;Sql;DBus")
    endif(KLF_MACOSX_BUNDLE_EXTRAS_klatexformula)
    KLFMInstallNameToolChange(klfbaseplugins "${CMAKE_CURRENT_BINARY_DIR}/${plugin_libname}"
        "Frameworks/klfbackend.framework/Versions/${KLF_LIB_VERSION}/klfbackend"
        "${CMAKE_BINARY_DIR}/src/klfbackend/klfbackend.framework/Versions/${KLF_LIB_VERSION}/klfbackend")
    KLFMInstallNameToolChange(klfbaseplugins "${CMAKE_CURRENT_BINARY_DIR}/${plugin_libname}"
	"Frameworks/klftools.framework/Versions/${KLF_LIB_VERSION}/klftools"
	"${CMAKE_BINARY_DIR}/src/klftools/klftools.framework/Versions/${KLF_LIB_VERSION}/klftools")
    KLFMInstallNameToolChange(klfbaseplugins "${CMAKE_CURRENT_BINARY_DIR}/${plugin_libname}"
        "Frameworks/klfapp.framework/Versions/${KLF_LIB_VERSION}/klfapp"
        "${CMAKE_BINARY_DIR}/src/klfapp.framework/Versions/${KLF_LIB_VERSION}/klfapp")
  endforeach(plugin_libname)

endif(APPLE AND KLF_MACOSX_BUNDLES)


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


if(APPLE AND KLF_MACOSX_BUNDLES)
  add_dependencies(klfbaseplugins_rcc klfbaseplugins_maclibpacked)
  add_custom_command(TARGET klfbaseplugins_rcc POST_BUILD
      COMMAND cp "${CMAKE_CURRENT_BINARY_DIR}/${KLF_BASEPLUGINS_FN_RCC}"
		 "${klfbundlelocation_klatexformula}/Contents/Resources/rccresources/${KLF_BASEPLUGINS_FN_RCC}"
      COMMENT "Installing base plugins into application bundle"
      VERBATIM)
endif(APPLE AND KLF_MACOSX_BUNDLES)


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


