# ######################################## #
# CMake project file for klatexformula/src #
# ######################################## #
# $Id$
# ######################################## #


# Rely on CMake utilities for fixing up bundle dependencies
#
if(KLF_MACOSX_BUNDLES)
  include(BundleUtilities)
#include(KLFMacBundle)
endif(KLF_MACOSX_BUNDLES)


# Set up -DKLF_WS_<window-system-name> -DKLF_WS="..." to indicate window system in use
add_definitions("-DKLF_WS=\"${KLF_WS}\"")
string(TOUPPER "${KLF_WS}" klf_ws_uc)
add_definitions("-DKLF_WS_${klf_ws_uc}")


# include these subprojects...

add_subdirectory(klftools)

add_subdirectory(klfbackend)



# klatexformula main GUI program
# ------------------------------


if(KLF_BUILD_GUI)

  find_package(Qt5Sql REQUIRED)
  find_package(Qt5UiTools REQUIRED)
  find_package(Qt5LinguistTools REQUIRED)
  find_package(Qt5Svg REQUIRED)
  if(KLF_USE_DBUS)
    find_package(Qt5DBus REQUIRED)
  endif()
  if(KLF_WS STREQUAL "x11")
    find_package(Qt5X11Extras REQUIRED)
  endif()
  if(KLF_WS STREQUAL "win")
    find_package(Qt5WinExtras REQUIRED)
  endif()
  if(KLF_WS STREQUAL "mac")
    find_package(Qt5MacExtras REQUIRED)
  endif()

  # Sources
  set(klatexformula_SRCS
    klfconfig.cpp
    klflatexsymbols.cpp
    klfstyle.cpp
    klflibentryeditor.cpp
    klflibbrowser.cpp
    klflib.cpp
    klflibview.cpp
    klflibdbengine.cpp
    klfliblegacyengine.cpp
    klfexporter.cpp
    klfmime.cpp
    klfmainwin.cpp
    klfsettings.cpp
    klfstylemanager.cpp
    klfmain.cpp
    klfcmdiface.cpp
    klfuiloader.cpp
    main.cpp
    klfapp.cpp
    )
  # Headers with QObject classes (needs moc)
  set(klatexformula_MOCHEADERS
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
#    klfmime_p.h
#    klfexporter.h
    klfexporter_p.h
    klfmainwin.h
    klfmainwin_p.h
    klfsettings.h
    klfsettings_p.h
    klfstylemanager.h
    klfcmdiface.h
    klfuiloader_p.h
    klfautoupdater.h
    klfapp.h
    )
  # UI Forms
  set(klatexformula_UIS
    klflatexsymbols.ui
    klflibbrowser.ui
    klflibentryeditor.ui
    klflibopenresourcedlg.ui
    klflibrespropeditor.ui
    klfliblocalfilewidget.ui
    klflibnewsubresdlg.ui
    klfmainwin.ui
    klfsettings.ui
    klfstylemanager.ui
    klfaboutdialog.ui
    klfwhatsnewdialog.ui
    klflibexportdialog.ui
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

  add_executable(klatexformula ${klatexformula_SRCS})

  target_include_directories(klatexformula PUBLIC
    "${CMAKE_CURRENT_BINARY_DIR}"
    "${CMAKE_CURRENT_SOURCE_DIR}"
    "${CMAKE_CURRENT_SOURCE_DIR}/klftools"
    "${CMAKE_CURRENT_SOURCE_DIR}/klfbackend")

  # Uis, Mocs, Qrcs
  qt5_wrap_ui(klatexformula_UIS_H ${klatexformula_UIS})
  qt5_wrap_cpp(klatexformula_MOC_CPPS ${klatexformula_MOCHEADERS})
  qt5_add_resources(klatexformula_QRC_CPPS ${klatexformula_QRCS})
  target_sources(klatexformula PRIVATE ${klatexformula_UIS_H} ${klatexformula_MOC_CPPS} ${klatexformula_QRC_CPPS})

  target_link_libraries(klatexformula
    Qt5::Core Qt5::Gui Qt5::Widgets Qt5::Xml Qt5::Sql Qt5::Svg Qt5::UiTools klfbackend klftools)

  # Translations
  qt5_add_translation(klatexformula_QMS ${klatexformula_TSS})

  set(klatexformula_post_build_commands )
  set(klatexformula_bundle_include_libs )

  # Adjustments for certain platforms and options -- special features
  if(KLF_WS STREQUAL "win")
    target_sources(klatexformula PRIVATE
      mswin/klfwinclipboard.cpp
      )
    target_link_libraries(klatexformula Qt5::WinExtras)
  endif()
  if(KLF_USE_WINSPARKLE)
    target_compile_definitions(klatexformula PUBLIC
      -DKLF_USE_WINSPARKLE -DKLF_WINSPARKLE_FEED_URL=\"${KLF_WINSPARKLE_FEED_URL}\")
    target_include_directories(${KLF_WINSPARKLE_DIR})
    qt5_wrap_cpp(klatexformula_MOC_CPPS_winsparkle mswin/klfwinsparkleupdater.h)
    target_sources(klatexformula PRIVATE
      mswin/klfwinsparkleupdater.cpp
      ${klatexformula_MOC_CPPS_winsparkle}
      )
    target_link_libraries(klatexformula "${KLF_WINSPARKLE_DIR}/WinSparkle.dll")
  endif()
  if(KLF_WS STREQUAL "mac")
    target_sources(klatexformula PRIVATE
      macosx/klfmacclipboard.cpp
      macosx/klfmainwin_mac.mm
      macosx/klfcmdifacedispatch_mac.mm
      )
    target_link_libraries(klatexformula Qt5::MacExtras)
    target_link_libraries(klatexformula "-framework Cocoa")
  endif()
  if(KLF_USE_SPARKLE)
    target_compile_definitions(klatexformula PRIVATE
      -DKLF_USE_SPARKLE -DKLF_SPARKLE_FEED_URL=\"${KLF_SPARKLE_FEED_URL}\")
    target_include_directories(klatexformula PRIVATE
      ${KLF_SPARKLE_FRAMEWORK}/Headers)
    qt5_wrap_cpp(klatexformula_MOC_CPPS_sparkle macosx/klfsparkleupdater.h)
    target_sources(klatexformula PRIVATE
      macosx/klfsparkleupdater.mm
      ${klatexformula_MOC_CPPS_sparkle}
      # also include in the bundle:
      "${KLF_SPARKLE_DSA_PUBKEY}"
      )
    target_link_libraries(klatexformula ${KLF_SPARKLE_FRAMEWORK})
  endif()
  if(KLF_MACOSX_BUNDLES)
    target_sources(klatexformula PRIVATE
      # more stuff we may want to import/copy privately in the App bundle
      macosx/qt.conf
      macosx/klatexformula.icns
      ${KLF_INCLUDE_FONTS}
      klatexformula-16.png
      klatexformula-32.png
      klatexformula-64.png
      klatexformula-128.png
      klatexformula-256.png
      klatexformula-512.png
      klatexformula-1024.png
      )
    if (KLF_USE_SPARKLE)
      # this is needed for inside the info.plist
      get_filename_component(KLF_SPARKLE_DSA_PUBKEY_BASENAME "${KLF_SPARKLE_DSA_PUBKEY}" NAME)
    endif ()
    # Set up the application bundle under Mac OS X
    set_target_properties(klatexformula PROPERTIES
      MACOSX_BUNDLE                      true
      OUTPUT_NAME                        klatexformula
      MACOSX_BUNDLE_ICON_FILE            klatexformula.icns
      MACOSX_BUNDLE_INFO_PLIST           "${CMAKE_CURRENT_SOURCE_DIR}/macosx/Info.plist.in"
      MACOSX_BUNDLE_INFO_STRING          "KLatexFormula version ${KLF_VERSION}"
      MACOSX_BUNDLE_GUI_IDENTIFIER       "org.klatexformula.klatexformula"
      MACOSX_BUNDLE_LONG_VERSION_STRING  "${KLF_VERSION}"
      MACOSX_BUNDLE_SHORT_VERSION_STRING "${KLF_VERSION}"
      MACOSX_BUNDLE_BUNDLE_VERSION       "${KLF_VERSION}"
      MACOSX_BUNDLE_BUNDLE_COPYRIGHT     "Copyright (C) 2016 Philippe Faist"
      )
    set_source_files_properties(
      klatexformula-16.png
      klatexformula-32.png
      klatexformula-64.png
      klatexformula-128.png
      klatexformula-256.png
      klatexformula-512.png
      klatexformula-1024.png
      macosx/klatexformula.icns
      PROPERTIES MACOSX_PACKAGE_LOCATION "Resources"
      )
    if(KLF_INCLUDE_FONTS)
      set_source_files_properties(${KLF_INCLUDE_FONTS} PROPERTIES
        MACOSX_PACKAGE_LOCATION "Resources/fonts"
        )
    endif()
    if(KLF_USE_SPARKLE)
      set_source_files_properties("${KLF_SPARKLE_DSA_PUBKEY}" PROPERTIES
        MACOSX_PACKAGE_LOCATION Resources
        )
    endif()
    if(KLF_MACOSX_BUNDLE_EXTRAS)
      if(KLF_BUNDLE_QT_PLUGINS)
	foreach(qtplugintgt ${KLF_BUNDLE_QT_PLUGINS})
          get_target_property(qtplugintgt_loc ${qtplugintgt} LOCATION)
          get_filename_component(qtplugindirfull ${qtplugintgt_loc} DIRECTORY)
          get_filename_component(qtplugintgt_fname ${qtplugintgt_loc} NAME)
          get_filename_component(qtplugindirname ${qtplugindirfull} NAME)
          #message("${qtplugintgt}: ${qtplugintgt_loc} ${qtplugindirname}")
          set(klatexformula_post_build_commands ${klatexformula_post_build_commands}
            COMMAND "${CMAKE_COMMAND}" "-E" "make_directory"
            "${CMAKE_CURRENT_BINARY_DIR}/klatexformula.app/Contents/plugins/${qtplugindirname}"
            COMMAND "${CMAKE_COMMAND}" "-E" "copy" "${qtplugintgt_loc}"
            "${CMAKE_CURRENT_BINARY_DIR}/klatexformula.app/Contents/plugins/${qtplugindirname}"
            )
          set(klatexformula_bundle_include_libs ${klatexformula_bundle_include_libs}
            "${CMAKE_CURRENT_BINARY_DIR}/klatexformula.app/Contents/plugins/${qtplugindirname}/${qtplugintgt_fname}"
            )
          # insert @executable_path/../plugins/<qt-plugin-dir-name> if not already in dirs list
          if(NOT ";${klatexformula_bundle_include_dirs};" MATCHES ";@executable_path/../plugins/${qtplugindirname};")
            set(klatexformula_bundle_include_dirs ${klatexformula_bundle_include_dirs}
              "@executable_path/../plugins/${qtplugindirname}")
          endif()
	endforeach()
      endif(KLF_BUNDLE_QT_PLUGINS)
      set_source_files_properties(
        macosx/qt.conf
        PROPERTIES MACOSX_PACKAGE_LOCATION "Resources"
        )
      set(APP_TO_FIXUP "${CMAKE_CURRENT_BINARY_DIR}/klatexformula.app")
      set(LIBS "${klatexformula_bundle_include_libs}")
      set(DIRS "${klatexformula_bundle_include_dirs}")
      configure_file("${CMAKE_SOURCE_DIR}/cmake/klf_fix_up_app_bundle.cmake.in"
        "${CMAKE_CURRENT_BINARY_DIR}/fix_up_klatexformula_app_bundle.cmake"
        @ONLY)
      set(klatexformula_post_build_commands ${klatexformula_post_build_commands}
        COMMAND "${CMAKE_COMMAND}" -P "${CMAKE_CURRENT_BINARY_DIR}/fix_up_klatexformula_app_bundle.cmake"
        )
    endif(KLF_MACOSX_BUNDLE_EXTRAS)
  endif(KLF_MACOSX_BUNDLES)
  if(KLF_WS STREQUAL "x11")
    target_link_libraries(klatexformula Qt5::X11Extras)
    target_sources(klatexformula PRIVATE
      x11/klfx11.cpp
      )
  endif()
  if(KLF_USE_DBUS)
    target_compile_definitions(klatexformula PUBLIC -DKLF_USE_DBUS)
    qt5_wrap_cpp(klatexformula_MOC_CPPS_dbus klfdbus.h)
    target_sources(klatexformula PRIVATE
      klfdbus.cpp
      ${klatexformula_MOC_CPPS_dbus}
      )
    target_link_libraries(klatexformula Qt5::DBus)
  endif()
  if(KLF_DEBUG_USE_MODELTEST)
    target_compile_definitions(klatexformula PRIVATE -DKLF_DEBUG_USE_MODELTEST)
    qt5_wrap_cpp(klatexformula_MOC_CPPS_modeltest modeltest.h)
    target_sources(klatexformula PRIVATE
      modeltest.cpp
      ${klatexformula_MOC_CPPS_modeltest}
      )
  endif()

  if(KLF_NO_CMU_FONT)
    target_compile_definitions(klatexformula PRIVATE -DKLF_NO_CMU_FONT)
  endif()

  # Don't use CMAKE_INSTALL_PREFIX itself, as it is irrelevant for windows installers
  # we need relative path
  KLFMakeAbsInstallPath(abs_share_klf_data_dir KLF_INSTALL_SHARE_KLF_DATA_DIR)
  KLFRelativePath(klf_share_dir "${KLF_ABS_INSTALL_BIN_DIR}" "${abs_share_klf_data_dir}")
  if(KLF_MACOSX_BUNDLES OR IS_ABSOLUTE "${klf_share_dir}")
    set(klf_share_dir "") # let the program find out
  endif()
  if(klf_share_dir)
    KLFCMakeDebug("setting -DKLF_SHARE_DIR=\"${klf_share_dir}\"")
    target_compile_definitions(klatexformula PRIVATE -DKLF_SHARE_DIR="${klf_share_dir}")
  endif(klf_share_dir)

  #
  # extra data: create and include the klf_xtradata.qrc file
  #
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
  # with bundles, we include the file itself in the bundle, not in qrc (see QTBUG-30917):
  if(NOT KLF_MACOSX_BUNDLES)
    foreach(fnt ${KLF_INCLUDE_FONTS})
      get_filename_component(basefntname "${fnt}" NAME)
      if(NOT IS_ABSOLUTE "${fnt}")
        set(fnt "${CMAKE_CURRENT_SOURCE_DIR}/${fnt}")
      endif(NOT IS_ABSOLUTE "${fnt}")
      file(APPEND "${klf_xtradata_res}"   "    <file alias=\"${basefntname}\">${fnt}</file>\n")
    endforeach(fnt)
  endif(NOT KLF_MACOSX_BUNDLES)
  file(APPEND "${klf_xtradata_res}" "  </qresource>\n")
  file(APPEND "${klf_xtradata_res}" "</RCC>\n")
  qt5_add_resources(klatexformula_QRC_CPPS_xtradata "${klf_xtradata_res}")
  target_sources(klatexformula PRIVATE ${klatexformula_QRC_CPPS_xtradata})

  # Stuff for windows
  if(WIN32)
    set(KLF_WIN_ICON "mswin\\\\klficon64.ico" CACHE STRING
      "Icon for klatexformula.exe program, relative to src/, with (escaped double-backslashes!)")
    file(WRITE "${CMAKE_CURRENT_BINARY_DIR}/klatexformula.rc"
      "IDI_ICON1               ICON    DISCARDABLE     \"${KLF_WIN_ICON}\"\n")
    # windows .rc resource compilation for windows
    add_custom_command(OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/klatexformula.rc.o"
      COMMAND windres.exe -I"${CMAKE_CURRENT_SOURCE_DIR}"
      -i"${CMAKE_CURRENT_BINARY_DIR}/klatexformula.rc"
      -o "${CMAKE_CURRENT_BINARY_DIR}/klatexformula.rc.o"
      )
    target_sources(klatexformula PRIVATE "${CMAKE_CURRENT_BINARY_DIR}/klatexformula.rc.o")

    # On debug versions, the application will open a console in which the debug output is shown.
    # Add the -mwindows only if we are not compiling in debug mode.
    # Add also a separate target klatexformula_cmdl to build the cmdl version, keeping the shell
    # window open.
    add_executable(klatexformula_cmdl ${klatexformula_ALLSRC})
    target_link_libraries(klatexformula_cmdl
      Qt5::Core Qt5::Gui Qt5::Widgets Qt5::Xml Qt5::Sql Qt5::Svg Qt5::UiTools klfbackend klftools)
    if(NOT KLF_DEBUG)
      set_target_properties(klatexformula  PROPERTIES LINK_FLAGS_RELEASE  "-Wl,-subsystem,windows")
      set_target_properties(klatexformula_cmdl  PROPERTIES LINK_FLAGS_RELEASE  "-Wl,-subsystem,console")
    endif(NOT KLF_DEBUG)
  endif(WIN32)

  #
  # User scripts
  #
  file(GLOB_RECURSE klf_userscriptfiles RELATIVE "${CMAKE_CURRENT_SOURCE_DIR}" userscripts/*)
  foreach(usfile ${klf_userscriptfiles})
    if(usfile MATCHES "\\.(py|rb|sh|pl|bat|xml|ui)$")
      # backup file -- ignore
      target_sources(klatexformula PRIVATE
        ${usfile}
        )
      get_filename_component(usfiledir "${usfile}" DIRECTORY)
      set_source_files_properties(${usfile}
        PROPERTIES MACOSX_PACKAGE_LOCATION Resources/${usfiledir}
        )
      # add install target!!!!!
      # .....
    endif()
  endforeach()

  # Make man page
  if(HELP2MAN)
    # generate the man page
    set(LDPATHDEFS "export LD_LIBRARY_PATH='${CMAKE_CURRENT_BINARY_DIR}:${CMAKE_CURRENT_BINARY_DIR}/klfbackend:${CMAKE_CURRENT_BINARY_DIR}/klftools':\"$LD_LIBRARY_PATH\" ")
    add_custom_command(OUTPUT "klatexformula.1"
      COMMAND
      "sh" "-c" "${LDPATHDEFS}; ${HELP2MAN} -h --help='\"&1\"' -v --version='\"&1:klatexformula %k\"' --no-info $<TARGET_FILE:klatexformula> -o '${CMAKE_CURRENT_BINARY_DIR}/klatexformula.1'"
      COMMENT "Generating man page for klatexformula"
      DEPENDS "${klatexformula}"
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

    if(KLF_INSTALL_SHARE_MAN1_DIR)
      install(FILES
        "${CMAKE_CURRENT_BINARY_DIR}/${klatexformula_manpage_file}"
        "${CMAKE_CURRENT_BINARY_DIR}/${klatexformula_cmdl_manpage_file}"
        DESTINATION "${KLF_INSTALL_SHARE_MAN1_DIR}")
    endif(KLF_INSTALL_SHARE_MAN1_DIR)

  endif(HELP2MAN)

  if(KLF_INSTALL_QTLIBS)
    if(NOT WIN32)
      message(FATAL_ERROR "Not supported on non-windows system.")
    endif()
    # Install Qt libs
    install(DIRECTORY "${QT_BINARY_DIR}/" DESTINATION "${KLF_INSTALL_LIB_DIR}" FILES_MATCHING
      PATTERN "libgcc*.dll"
      PATTERN "mingw*.dll"
      REGEX "/Qt(Core|Gui|Sql|Xml)4?\\.dll$"
      )
  endif()

  if(WIN32 AND KLF_USE_WINSPARKLE)
    install(FILES "${KLF_WINSPARKLE_DIR}/WinSparkle.dll"
      DESTINATION "${KLF_INSTALL_LIB_DIR}")
  endif(WIN32 AND KLF_USE_WINSPARKLE)

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
  endif(KLF_INSTALL_QTPLUGINS)

  if(NOT KLF_MACOSX_BUNDLES)
    if(KLF_INSTALL_KLATEXFORMULA_BIN)
      install(TARGETS klatexformula RUNTIME DESTINATION "${KLF_INSTALL_BIN_DIR}")
    endif()
    if(UNIX AND KLF_INSTALL_KLATEXFORMULA_CMDL)
      install(CODE
	"message(STATUS \"Creating Symlink [\$ENV{DESTDIR}]${KLF_ABS_INSTALL_BIN_DIR}/klatexformula_cmdl -> klatexformula\")
	  execute_process(COMMAND \"${CMAKE_COMMAND}\" -E create_symlink \"klatexformula\" \"\$ENV{DESTDIR}${KLF_ABS_INSTALL_BIN_DIR}/klatexformula_cmdl\")"
        )
    elseif(WIN32 AND KLF_INSTALL_KLATEXFORMULA_CMDL)
      install(TARGETS klatexformula_cmdl RUNTIME DESTINATION "${KLF_INSTALL_BIN_DIR}")
    endif()
    # install targets for user scripts
    # .........
  endif()

  if(KLF_MACOSX_BUNDLES)
    install(TARGETS klatexformula BUNDLE DESTINATION "${KLF_INSTALL_BIN_DIR}")
  endif()

  # --- execute all post-build commands, in order ---
  if(klatexformula_post_build_commands)
    add_custom_command(TARGET klatexformula POST_BUILD
      ${klatexformula_post_build_commands}
      )
  endif()

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
    endif()
    if(KLF_INSTALL_SHARE_PIXMAPS_DIR)
      install(FILES
        klatexformula-128.png
        klatexformula-64.png
        klatexformula-32.png
        klatexformula-16.png
        DESTINATION "${KLF_INSTALL_SHARE_PIXMAPS_DIR}"
        )
    endif()
    if(KLF_INSTALL_ICON_THEME)
      install(FILES klatexformula-128.png
        DESTINATION "${KLF_INSTALL_ICON_THEME}/128x128/apps/" RENAME "klatexformula.png")
      install(FILES klatexformula-64.png
        DESTINATION "${KLF_INSTALL_ICON_THEME}/64x64/apps/" RENAME "klatexformula.png")
      install(FILES klatexformula-32.png
        DESTINATION "${KLF_INSTALL_ICON_THEME}/32x32/apps/" RENAME "klatexformula.png")
      install(FILES klatexformula-16.png
        DESTINATION "${KLF_INSTALL_ICON_THEME}/16x16/apps/" RENAME "klatexformula.png")
    endif()
    if(KLF_INSTALL_SHARE_MIME_PACKAGES_DIR)
      install(FILES "${CMAKE_SOURCE_DIR}/src/klatexformula-mime.xml"
        DESTINATION "${KLF_INSTALL_SHARE_MIME_PACKAGES_DIR}")
    endif()

    if(KLF_INSTALL_POST_UPDATEMIMEDATABASE)
      install(CODE
        "message(STATUS \"Updating Mime Types Database (update-mime-database)\")
	execute_process(COMMAND update-mime-database \"${CMAKE_INSTALL_PREFIX}/share/mime\")"
        )
    endif()
  endif()

  if(UNIX AND NOT KLF_MACOSX_BUNDLES AND KLF_USE_CPACK)
    configure_file(
      "${CMAKE_SOURCE_DIR}/src/klatexformula.sh.in"
      "${CMAKE_BINARY_DIR}/src/klatexformula.sh"
      @ONLY)
    install(PROGRAMS "${CMAKE_BINARY_DIR}/src/klatexformula.sh" DESTINATION "/")
  endif()


endif(KLF_BUILD_GUI)

