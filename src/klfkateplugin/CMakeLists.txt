# KLatexFormula Kate Editor Part Plugin

find_package(KDE4 REQUIRED)
include(KDE4Defaults)
include_directories(${KDE4_INCLUDES})
include_directories(/usr/include/klfbackend)


########### next target ###############

set(ktexteditor_klf_PART_SRCS klfkateplugin.cpp klfkateplugin_config.cpp )

kde4_add_ui_files(ktexteditor_klf_PART_SRCS klfkatepluginconfigwidget.ui)
kde4_install_icons(${ICON_INSTALL_DIR})
kde4_add_plugin(ktexteditor_klf ${ktexteditor_klf_PART_SRCS} )

target_link_libraries(ktexteditor_klf ${KDE4_KDECORE_LIBS} ${KDE4_KDEUI_LIBS} ${KDE4_KIO_LIBS}
				       -lktexteditor -lklfbackend )

install(TARGETS ktexteditor_klf DESTINATION "${PLUGIN_INSTALL_DIR}" )

########### install files ###############

install(FILES klfkatepluginui.rc DESTINATION "${DATA_INSTALL_DIR}/ktexteditor_klf" )

install(FILES ktexteditor_klf.desktop
              ktexteditor_klf_config.desktop DESTINATION "${SERVICES_INSTALL_DIR}" )

