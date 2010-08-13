# ###################################################### #
# CMake project file for klatexformula/src/klfkateplugin #
# ###################################################### #
# $Id$
# ###################################################### #

# KLatexFormula KTextEditor Plugin

message(STATUS "")
message(STATUS "[KDE4 SETTINGS]")

# find KDE4
find_package(KDE4)
if(NOT KDE4_KDECONFIG_EXECUTABLE)
  set(KLF_BUILD_KTEXTEDITORPLUGIN OFF CACHE BOOL "Build KTextEditor plugin" FORCE)
  KLFNote(
  "KDE4 is required to build KTextEditor plugin. The plugin has been disabled (KLF_BUILD_KTEXTEDITORPLUGIN). Please re-run cmake to continue.")
  message(FATAL_ERROR "KDE4 not found.")
endif(NOT KDE4_KDECONFIG_EXECUTABLE)

include(KDE4Defaults)
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

set(ktexteditor_klf_PART_SRCS klfkateplugin.cpp klfkateplugin_config.cpp )

kde4_add_ui_files(ktexteditor_klf_PART_SRCS klfkatepluginconfigwidget.ui)
kde4_install_icons(${ICON_INSTALL_DIR})
kde4_add_plugin(ktexteditor_klf ${ktexteditor_klf_PART_SRCS} )

target_link_libraries(ktexteditor_klf ${KDE4_KDECORE_LIBS} ${KDE4_KDEUI_LIBS} ${KDE4_KIO_LIBS}
				       -lktexteditor klfbackend )

install(TARGETS ktexteditor_klf DESTINATION "${PLUGIN_INSTALL_DIR}" )

########### install files ###############

install(FILES klfkatepluginui.rc DESTINATION "${DATA_INSTALL_DIR}/ktexteditor_klf" )

install(FILES ktexteditor_klf.desktop
              ktexteditor_klf_config.desktop DESTINATION "${SERVICES_INSTALL_DIR}" )

