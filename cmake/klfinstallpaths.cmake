# CMake Install Instructions
# ==========================
# $Id$


KLFGetCMakeVarChanged(CMAKE_INSTALL_PREFIX)
KLFGetCMakeVarChanged(KLF_INSTALL_LIB_DIR)
KLFGetCMakeVarChanged(KLF_INSTALL_BIN_DIR)
KLFGetCMakeVarChanged(CMAKE_INSTALL_RPATH)


# Installation destination
# ------------------------

set(CMAKE_INSTALL_PREFIX "/usr" CACHE PATH "The installation prefix for KLatexFormula.")
mark_as_advanced(CLEAR CMAKE_INSTALL_PREFIX)
message(STATUS "'make install' will install to ${CMAKE_INSTALL_PREFIX} (CMAKE_INSTALL_PREFIX)")


# Installation Paths
# ------------------

set(install_lib_dir "lib${KLF_LIB_SUFFIX}")
if(WIN32)
  # install all in root dir on windows
  set(install_lib_dir "${CMAKE_INSTALL_PREFIX}")
endif(WIN32)
if(APPLE AND KLF_MACOSX_BUNDLES)
  # install to /Library/Frameworks on mac OS X
  set(install_lib_dir "/Library/Frameworks")
endif(APPLE AND KLF_MACOSX_BUNDLES)
if(klf_changed_CMAKE_INSTALL_PREFIX AND NOT klf_changed_KLF_INSTALL_LIB_DIR)
  set(KLF_INSTALL_LIB_DIR "${install_lib_dir}" CACHE STRING
		  "Library installation directory (relative to install prefix, or absolute)" FORCE)
  mark_as_advanced(KLF_INSTALL_LIB_DIR)
  if(NOT klf_first_KLF_INSTALL_LIB_DIR)
    message(STATUS "Updating library install dir to ${KLF_INSTALL_LIB_DIR} following changed of CMAKE_INSTALL_PREFIX")
  endif(NOT klf_first_KLF_INSTALL_LIB_DIR)
endif(klf_changed_CMAKE_INSTALL_PREFIX AND NOT klf_changed_KLF_INSTALL_LIB_DIR)


set(install_bin_dir "bin")
if(WIN32)
  # install all in root dir
  set(install_bin_dir "${CMAKE_INSTALL_PREFIX}")
endif(WIN32)
if(APPLE AND KLF_MACOSX_BUNDLES)
  # install to /Applications on mac OS X
  set(install_bin_dir "/Applications")
endif(APPLE AND KLF_MACOSX_BUNDLES)
if(klf_changed_CMAKE_INSTALL_PREFIX AND NOT klf_changed_KLF_INSTALL_BIN_DIR)
  set(KLF_INSTALL_BIN_DIR "${install_bin_dir}" CACHE STRING
		  "Binaries installation directory (relative to install prefix, or absolute)" FORCE)
  mark_as_advanced(KLF_INSTALL_BIN_DIR)
  if(NOT klf_first_KLF_INSTALL_BIN_DIR)
    message(STATUS "Updating binary install dir to ${KLF_INSTALL_BIN_DIR} following changed of CMAKE_INSTALL_PREFIX")
  endif(NOT klf_first_KLF_INSTALL_BIN_DIR)
endif(klf_changed_CMAKE_INSTALL_PREFIX AND NOT klf_changed_KLF_INSTALL_BIN_DIR)




# Installed RPATH
# ---------------

set(CMAKE_SKIP_RPATH FALSE CACHE BOOL "Don't set RPATH on executables and libraries")
if(NOT CMAKE_SKIP_RPATH)
  set(wantSetNewRpath FALSE)
  if(klf_changed_CMAKE_INSTALL_PREFIX AND NOT klf_changed_CMAKE_INSTALL_RPATH)
    set(wantSetNewRpath TRUE)
  endif(klf_changed_CMAKE_INSTALL_PREFIX AND NOT klf_changed_CMAKE_INSTALL_RPATH)
  if(NOT CMAKE_INSTALL_RPATH)
    set(wantSetNewRpath TRUE)
  endif(NOT CMAKE_INSTALL_RPATH)
  if(wantSetNewRpath)
    set(CMAKE_INSTALL_RPATH "${CMAKE_INSTALL_PREFIX}/${KLF_INSTALL_LIB_DIR}" CACHE PATH
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





# Uninstall target
# ----------------

configure_file(
  "${CMAKE_CURRENT_SOURCE_DIR}/cmake/cmake_uninstall.cmake.in"
  "${CMAKE_CURRENT_BINARY_DIR}/cmake_uninstall.cmake"
  IMMEDIATE @ONLY)
add_custom_target(uninstall
  "${CMAKE_COMMAND}" -P "${CMAKE_CURRENT_BINARY_DIR}/cmake_uninstall.cmake")


