# CMake Install Instructions

# --- KLFBACKEND ----

set(KLF_INSTALL_LIB_DIR "lib${KLF_LIB_SUFFIX}" CACHE STRING "Library installation dir (rel. to prefix or abs.)")
mark_as_advanced(KLF_INSTALL_LIB_DIR)
if(WIN32)
   #install all in root dir
  set(KLF_INSTALL_LIB_DIR "" CACHE STRING "Library installation dir (rel. to prefix or abs.)")
endif(WIN32)
if(APPLE AND KLF_MACOSX_BUNDLES)
  set(KLF_INSTALL_LIB_DIR "/Library/Frameworks" CACHE STRING "Library installation dir (rel. to prefix or abs.)")
endif(APPLE AND KLF_MACOSX_BUNDLES)

if(KLF_USE_QT4)
  install(TARGETS klfbackend LIBRARY DESTINATION "${KLF_INSTALL_LIB_DIR}"
			     ARCHIVE DESTINATION "${KLF_INSTALL_LIB_DIR}"
			     FRAMEWORK DESTINATION "${KLF_INSTALL_LIB_DIR}"
			     PUBLIC_HEADER DESTINATION "include/klfbackend")
  if(KLF_MACOSX_BUNDLES)
    KLFInstFrameworkUpdateId(
      "${KLF_INSTALL_LIB_DIR}/klfbackend.framework/Versions/${KLF_LIB_VERSION}/klfbackend"
    )
  endif(KLF_MACOSX_BUNDLES)
else(KLF_USE_QT4)
  install(TARGETS klfbackend-qt3 LIBRARY DESTINATION "${KLF_INSTALL_LIB_DIR}"
				 ARCHIVE DESTINATION "${KLF_INSTALL_LIB_DIR}"
				 FRAMEWORK DESTINATION "${KLF_INSTALL_LIB_DIR}"
				 PUBLIC_HEADER DESTINATION "include/klfbackend")
endif(KLF_USE_QT4)


# --- ALL THE FOLLOWING ONLY IF GUI IS BUILT ---
if(NOT KLF_BUILD_GUI)
  RETURN()
endif(NOT KLF_BUILD_GUI)


# --- KLFLIB ---

install(TARGETS klflib RUNTIME DESTINATION "${KLF_INSTALL_LIB_DIR}"
		       ARCHIVE DESTINATION "${KLF_INSTALL_LIB_DIR}"
		       LIBRARY DESTINATION "${KLF_INSTALL_LIB_DIR}"
		       FRAMEWORK DESTINATION "${KLF_INSTALL_LIB_DIR}"
		       PUBLIC_HEADER DESTINATION include/klf)
if(KLF_MACOSX_BUNDLES)
  # Installed library's ID
  KLFInstFrameworkUpdateId("${KLF_INSTALL_LIB_DIR}/klflib.framework/Versions/${KLF_LIB_VERSION}/klflib")
  # Installed library's dependency on klfbackend should be updated
  KLFInstFrameworkUpdateLibChange(
  "${KLF_INSTALL_LIB_DIR}/klflib.framework/Versions/${KLF_LIB_VERSION}/klflib"
  "${CMAKE_CURRENT_BINARY_DIR}/klfbackend/klfbackend.framework/Versions/${KLF_LIB_VERSION}/klfbackend"
  "${KLF_INSTALL_LIB_DIR}/klfbackend.framework/Versions/${KLF_LIB_VERSION}/klfbackend"
  )
endif(KLF_MACOSX_BUNDLES)

if(WIN32)
  # Install Qt libs
  install(FILES "${QT_QTCORE_LIBRARY}" "${QT_QTGUI_LIBRARY}" "${QT_QTSQL_LIBRARY}"
		"${QT_QTXML_LIBRARY}"
	  DESTINATION "${KLF_INSTALL_LIB_DIR}")
end(WIN32)


# --- KLATEXFORMULA BIN ---

set(KLF_INSTALL_BIN_DIR "bin" CACHE STRING "binaries installation dir (rel. to prefix or abs.)")
mark_as_advanced(KLF_INSTALL_BIN_DIR)
if(WIN32)
  # install all in root dir
  set(KLF_INSTALL_BIN_DIR "" CACHE STRING "binaries installation dir (rel. to prefix or abs.)")
endif(WIN32)
if(APPLE AND KLF_MACOSX_BUNDLES)
  set(KLF_INSTALL_BIN_DIR "/Applications" CACHE STRING "binaries installation dir (rel. to prefix or abs.)")
endif(APPLE AND KLF_MACOSX_BUNDLES)

install(TARGETS klatexformula RUNTIME DESTINATION "${KLF_INSTALL_BIN_DIR}"
			      BUNDLE DESTINATION "${KLF_INSTALL_BIN_DIR}")
if(UNIX AND NOT KLF_MACOSX_BUNDLES)
  install(CODE
	  "message(STATUS \"Creating Symlink ${CMAKE_INSTALL_PREFIX}/bin/klatexformula_cmdl -> klatexformula\")
	  execute_process(COMMAND \"${CMAKE_COMMAND}\" -E create_symlink \"klatexformula\" \"${CMAKE_INSTALL_PREFIX}/bin/klatexformula_cmdl\")"
  )
endif(UNIX AND NOT KLF_MACOSX_BUNDLES)
if(KLF_MACOSX_BUNDLES)
  # Update klatexformula's dependency on the installed frameworks.
  KLFInstFrameworkUpdateLibChange(
  "${KLF_INSTALL_BIN_DIR}/klatexformula-${KLF_VERSION}/Contents/MacOS/klatexformula-${KLF_VERSION}"
  "${CMAKE_CURRENT_BINARY_DIR}/klfbackend/klfbackend.framework/Versions/${KLF_LIB_VERSION}/klfbackend"
  "${KLF_INSTALL_LIB_DIR}/klfbackend.framework/Versions/${KLF_LIB_VERSION}/klfbackend"
  )
  KLFInstFrameworkUpdateLibChange(
  "${KLF_INSTALL_BIN_DIR}/klatexformula-${KLF_VERSION}/Contents/MacOS/klatexformula-${KLF_VERSION}"
  "${CMAKE_CURRENT_BINARY_DIR}/klflib.framework/Versions/${KLF_LIB_VERSION}/klflib"
  "${KLF_INSTALL_LIB_DIR}/klflib.framework/Versions/${KLF_LIB_VERSION}/klflib"
  )
endif(KLF_MACOSX_BUNDLES)


# --- LINUX DESKTOP FILES ---

if(KLF_INSTALL_DESKTOP)
  configure_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/klatexformula.desktop.in"
    "${CMAKE_CURRENT_BINARY_DIR}/klatexformula.desktop"
    IMMEDIATE @ONLY)

  install(FILES "${CMAKE_CURRENT_BINARY_DIR}/klatexformula.desktop" DESTINATION share/applications/)
  install(FILES klatexformula-128.png DESTINATION share/pixmaps/)
  install(FILES klatexformula-64.png DESTINATION share/pixmaps/)
  install(FILES klatexformula-32.png DESTINATION share/pixmaps/)
  install(FILES klatexformula-16.png DESTINATION share/pixmaps/)
  install(FILES klatexformula-mime.xml
	  DESTINATION share/mime/packages/)
  install(CODE
    "message(STATUS \"Updating Mime Types Database (update-mime-database)\")
     execute_process(COMMAND update-mime-database \"${CMAKE_INSTALL_PREFIX}/share/mime\")"
  )
endif(KLF_INSTALL_DESKTOP)






# Uninstall option
# ----------------

configure_file(
  "${CMAKE_CURRENT_SOURCE_DIR}/cmake/cmake_uninstall.cmake.in"
  "${CMAKE_CURRENT_BINARY_DIR}/cmake_uninstall.cmake"
  IMMEDIATE @ONLY)
add_custom_target(uninstall
  "${CMAKE_COMMAND}" -P "${CMAKE_CURRENT_BINARY_DIR}/cmake_uninstall.cmake")


