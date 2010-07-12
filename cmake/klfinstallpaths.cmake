# CMake Install Instructions



# Installation destination
# ------------------------

set(CMAKE_INSTALL_PREFIX "/usr" CACHE PATH "The installation prefix for KLatexFormula.")
mark_as_advanced(CLEAR CMAKE_INSTALL_PREFIX)
message(STATUS "'make install' will install to ${CMAKE_INSTALL_PREFIX} (CMAKE_INSTALL_PREFIX)")


# Installed RPATH
# ---------------

set(CMAKE_SKIP_RPATH FALSE CACHE BOOL "Don't set RPATH on executables and libraries")
KLFGetCMakeVarChanged(CMAKE_INSTALL_PREFIX)
KLFGetCMakeVarChanged(CMAKE_INSTALL_RPATH)
if(NOT CMAKE_SKIP_RPATH)
  set(wantSetNewRpath FALSE)
  if(klf_changed_CMAKE_INSTALL_PREFIX AND NOT klf_changed_CMAKE_INSTALL_RPATH)
    set(wantSetNewRpath TRUE)
  endif(klf_changed_CMAKE_INSTALL_PREFIX AND NOT klf_changed_CMAKE_INSTALL_RPATH)
  if(NOT CMAKE_INSTALL_RPATH)
    set(wantSetNewRpath TRUE)
  endif(NOT CMAKE_INSTALL_RPATH)
  if(wantSetNewRpath)
    set(CMAKE_INSTALL_RPATH "${CMAKE_INSTALL_PREFIX}/lib${KLF_LIB_SUFFIX}" CACHE PATH
					    "RPATH for installed libraries and executables" FORCE)
    if(NOT klf_first_CMAKE_INSTALL_RPATH)
      KLFNote("Updating CMAKE_INSTALL_RPATH to \"${CMAKE_INSTALL_RPATH}\" (following change of CMAKE_INSTALL_PREFIX)")
    endif(NOT klf_first_CMAKE_INSTALL_RPATH)
  endif(wantSetNewRpath)
  mark_as_advanced(CLEAR CMAKE_INSTALL_RPATH)
  message(STATUS "RPATH for installed libraries and executables: ${CMAKE_INSTALL_RPATH} (CMAKE_INSTALL_RPATH)")
else(NOT CMAKE_SKIP_RPATH)
  message(STATUS "Skipping RPATH on executables and libraries (CMAKE_SKIP_RPATH)")
endif(NOT CMAKE_SKIP_RPATH)



# Installation Paths
# ------------------

set(KLF_INSTALL_LIB_DIR "lib${KLF_LIB_SUFFIX}" CACHE STRING
						"Library installation dir (rel. to prefix or abs.)")
mark_as_advanced(KLF_INSTALL_LIB_DIR)
if(WIN32)
  # install all in root dir on windows
  set(KLF_INSTALL_LIB_DIR "" CACHE STRING "Library installation dir (rel. to prefix or abs.)")
endif(WIN32)
if(APPLE AND KLF_MACOSX_BUNDLES)
  # install to /Library/Frameworks on mac OS X
  set(KLF_INSTALL_LIB_DIR "/Library/Frameworks" CACHE STRING
						"Library installation dir (rel. to prefix or abs.)")
endif(APPLE AND KLF_MACOSX_BUNDLES)


set(KLF_INSTALL_BIN_DIR "bin" CACHE STRING "binaries installation dir (rel. to prefix or abs.)")
mark_as_advanced(KLF_INSTALL_BIN_DIR)
if(WIN32)
  # install all in root dir
  set(KLF_INSTALL_BIN_DIR "" CACHE STRING "binaries installation dir (rel. to prefix or abs.)")
endif(WIN32)
if(APPLE AND KLF_MACOSX_BUNDLES)
  # install to /Applications on mac OS X
  set(KLF_INSTALL_BIN_DIR "/Applications" CACHE STRING "binaries installation dir (rel. to prefix or abs.)")
endif(APPLE AND KLF_MACOSX_BUNDLES)



# Uninstall target
# ----------------

configure_file(
  "${CMAKE_CURRENT_SOURCE_DIR}/cmake/cmake_uninstall.cmake.in"
  "${CMAKE_CURRENT_BINARY_DIR}/cmake_uninstall.cmake"
  IMMEDIATE @ONLY)
add_custom_target(uninstall
  "${CMAKE_COMMAND}" -P "${CMAKE_CURRENT_BINARY_DIR}/cmake_uninstall.cmake")


