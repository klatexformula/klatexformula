# ###################################################### #
# CMake project file for klatexformula/src/klfkateplugin #
# ###################################################### #
# $Id$
# ###################################################### #

# KLatexFormula KTextEditor Plugin

# NOTE: This CMakeLists.txt can either be included from the root CMakeLists.txt
#       to build the plugin, or the plugin can also be built separately by
#       calling directly this CMakeLists.txt as root build script, eg.
#         > cd src/klfkateplugin/
#         > mkdir build && cd build
#         build> cmake .. && make
#

message(STATUS "")
message(STATUS "[KDE4 SETTINGS (for kate plugin)]")

if(NOT KLF_ROOT_CMAKE_READ)
  include(../../cmake/KLFUtil.cmake)
endif(NOT KLF_ROOT_CMAKE_READ)

macro(klf_nokde4)
  set(KLF_BUILD_KTEXTEDITORPLUGIN OFF CACHE BOOL "Build KTextEditor plugin" FORCE)
  KLFNote("KDE4 is required to build KTextEditor plugin.
    Since KDE4 could not be found, the KLatexFormula KTextEditor plugin will not be built (KLF_BUILD_KTEXTEDITORPLUGIN).
    KLatexFormula itself does not need KDE4 and will be compiled normally.
    Please re-run cmake to proceed.")
  message(FATAL_ERROR "KDE4 not found.")
endmacro(klf_nokde4)

# find KDE4
find_package(KDE4)
if(NOT KDE4_KDECONFIG_EXECUTABLE)
  klf_nokde4()
endif(NOT KDE4_KDECONFIG_EXECUTABLE)

include(KDE4Defaults OPTIONAL RESULT_VARIABLE klf_kde4defaults_found)
if(NOT klf_kde4defaults_found)
  klf_nokde4()
endif(NOT klf_kde4defaults_found)
include_directories(${KDE4_INCLUDES})
include_directories("${CMAKE_CURRENT_SOURCE_DIR}/../klfbackend")

mark_as_advanced(
	Automoc4_DIR
	KDE4_AUTH_POLICY_FILES_INSTALL_DIR
	KDE4_BUILD_TESTS
	KDE4_DATA_DIR
	KDE4_ENABLE_FINAL
	KDE4_ENABLE_FPIE
	KDE4_ENABLE_HTMLHANDBOOK
	KDE4_PLASMA_OPENGL_FOUND
	KDE4_SERIALIZE_TOOL
	KDE4_USE_COMMON_CMAKE_PACKAGE_CONFIG_DIR
)

# klfkateplugin plugin target

set(ktexteditor_klf_PART_SRCS klfkateplugin.cpp klfkteparser.cpp klfkateplugin_config.cpp )

if(KLF_LIBKLFBACKEND_STATIC)
  # Can't link the plugin to a static klfbackend library. We need to re-compile the klfbackend
  # sources for this target.
  get_target_property(klfbackend_sources klfbackend SOURCES)
  foreach(klfbackend_sourcefile ${klfbackend_sources})
    # TODO: this assumes that absolute paths are auto-generated sources (moc) and relative are
    #       real sources...
    if(NOT IS_ABSOLUTE ${klfbackend_sourcefile})
      set(ktexteditor_klf_PART_SRCS ${ktexteditor_klf_PART_SRCS}
				    "${CMAKE_SOURCE_DIR}/src/klfbackend/${klfbackend_sourcefile}")
    else(NOT IS_ABSOLUTE ${klfbackend_sourcefile})
    endif(NOT IS_ABSOLUTE ${klfbackend_sourcefile})
  endforeach(klfbackend_sourcefile)
  # add the moc file for klfdebug.h manually for automoc, since there isn't a klfdebug.cpp
  set(ktexteditor_klf_PART_SRCS ${ktexteditor_klf_PART_SRCS}
				    "${CMAKE_SOURCE_DIR}/src/klfbackend/klfdebug.h")
endif(KLF_LIBKLFBACKEND_STATIC)

kde4_add_ui_files(ktexteditor_klf_PART_SRCS klfkatepluginconfigwidget.ui)
kde4_install_icons(${ICON_INSTALL_DIR})
kde4_add_plugin(ktexteditor_klf ${ktexteditor_klf_PART_SRCS} )

target_link_libraries(ktexteditor_klf ${KDE4_KDECORE_LIBS} ${KDE4_KDEUI_LIBS} ${KDE4_KIO_LIBS}
				       -lktexteditor )

if(KLF_ROOT_CMAKE_READ AND NOT KLF_LIBKLFBACKEND_STATIC)
  # link to our klfbackend
  target_link_libraries(ktexteditor_klf  klfbackend)
elseif(NOT KLF_ROOT_CMAKE_READ)
  # link to external klfbackend, we are being compiled externally
  target_link_libraries(ktexteditor_klf  -lklfbackend)
endif(KLF_ROOT_CMAKE_READ AND NOT KLF_LIBKLFBACKEND_STATIC)

install(TARGETS ktexteditor_klf DESTINATION "${PLUGIN_INSTALL_DIR}" )

########### install files ###############

install(FILES klfkatepluginui.rc DESTINATION "${DATA_INSTALL_DIR}/ktexteditor_klf" )

install(FILES ktexteditor_klf.desktop
              ktexteditor_klf_config.desktop DESTINATION "${SERVICES_INSTALL_DIR}" )

