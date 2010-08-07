# CMake Install Instructions
# ==========================
# $Id$


KLFGetCMakeVarChanged(CMAKE_INSTALL_PREFIX)
KLFGetCMakeVarChanged(KLF_INSTALL_LIB_DIR)
KLFGetCMakeVarChanged(KLF_INSTALL_BIN_DIR)
KLFGetCMakeVarChanged(CMAKE_INSTALL_RPATH)
KLFGetCMakeVarChanged(KLF_INSTALL_RUN_POST_INSTALL)


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
if(klf_changed_KLF_LIB_SUFFIX OR klf_changed_CMAKE_INSTALL_PREFIX)
  if(NOT klf_changed_KLF_INSTALL_LIB_DIR AND NOT install_lib_dir STREQUAL "${KLF_INSTALL_LIB_DIR}")
    set(install_lib_dir_needs_set TRUE)
  else(NOT klf_changed_KLF_INSTALL_LIB_DIR AND NOT install_lib_dir STREQUAL "${KLF_INSTALL_LIB_DIR}")
    set(install_lib_dir_needs_set FALSE)
  endif(NOT klf_changed_KLF_INSTALL_LIB_DIR AND NOT install_lib_dir STREQUAL "${KLF_INSTALL_LIB_DIR}")
else(klf_changed_KLF_LIB_SUFFIX OR klf_changed_CMAKE_INSTALL_PREFIX)
  set(install_lib_dir_needs_set FALSE)
endif(klf_changed_KLF_LIB_SUFFIX OR klf_changed_CMAKE_INSTALL_PREFIX)
KLFCMakeDebug("install_lib_dir=${install_lib_dir}. klf_changed_CMAKE_INSTALL_PREFIX=${klf_changed_CMAKE_INSTALL_PREFIX}. install_lib_dir_needs_set:? ${install_lib_dir_needs_set}. klf_changed_KLF_INSTALL_LIB_DIR=${klf_changed_KLF_INSTALL_LIB_DIR} klf_internal_KLF_INSTALL_LIB_DIR=${klf_internal_KLF_INSTALL_LIB_DIR} klf_first_KLF_INSTALL_LIB_DIR=${klf_first_KLF_INSTALL_LIB_DIR} KLF_INSTALL_LIB_DIR=${KLF_INSTALL_LIB_DIR}")
if(install_lib_dir_needs_set)
  set(KLF_INSTALL_LIB_DIR "${install_lib_dir}" CACHE STRING
		  "Library installation directory (relative to install prefix, or absolute)" FORCE)
  mark_as_advanced(CLEAR KLF_INSTALL_LIB_DIR)
  if(NOT klf_first_KLF_INSTALL_LIB_DIR)
    KLFNote("Updating library install dir to ${KLF_INSTALL_LIB_DIR} following changes to CMAKE_INSTALL_PREFIX and/or KLF_LIB_SUFFIX")
  endif(NOT klf_first_KLF_INSTALL_LIB_DIR)
  KLFCMakeSetVarChanged(KLF_INSTALL_LIB_DIR)
endif(install_lib_dir_needs_set)
message(STATUS "Installing libraries to ${KLF_INSTALL_LIB_DIR} (KLF_INSTALL_LIB_DIR)")
# Utility variable. Same as KLF_INSTALL_LIB_DIR, but garanteed to be absolute path.
# Not kept in cache, it is (trivially) computed here
set(KLF_ABS_INSTALL_LIB_DIR "${KLF_INSTALL_LIB_DIR}")
if(NOT IS_ABSOLUTE "${KLF_INSTALL_LIB_DIR}")
  set(KLF_ABS_INSTALL_LIB_DIR "${CMAKE_INSTALL_PREFIX}/${KLF_INSTALL_LIB_DIR}")
endif(NOT IS_ABSOLUTE "${KLF_INSTALL_LIB_DIR}")


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
  mark_as_advanced(CLEAR KLF_INSTALL_BIN_DIR)
  if(NOT klf_first_KLF_INSTALL_BIN_DIR)
    message(STATUS "Updating binary install dir to ${KLF_INSTALL_BIN_DIR} following changed of CMAKE_INSTALL_PREFIX")
  endif(NOT klf_first_KLF_INSTALL_BIN_DIR)
  KLFCMakeSetVarChanged(KLF_INSTALL_BIN_DIR)
endif(klf_changed_CMAKE_INSTALL_PREFIX AND NOT klf_changed_KLF_INSTALL_BIN_DIR)
message(STATUS "Installing binary to ${KLF_INSTALL_BIN_DIR} (KLF_INSTALL_BIN_DIR)")
# Utility variable. Same as KLF_INSTALL_BIN_DIR, but garanteed to be absolute path.
# Not kept in cache, it is (trivially) computed here
set(KLF_ABS_INSTALL_BIN_DIR "${KLF_INSTALL_BIN_DIR}")
if(NOT IS_ABSOLUTE "${KLF_INSTALL_BIN_DIR}")
  set(KLF_ABS_INSTALL_BIN_DIR "${CMAKE_INSTALL_PREFIX}/${KLF_INSTALL_BIN_DIR}")
endif(NOT IS_ABSOLUTE "${KLF_INSTALL_BIN_DIR}")



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
    if(IS_ABSOLUTE KLF_INSTALL_LIB_DIR)
      # Absolute file path
      set(CMAKE_INSTALL_RPATH "${KLF_INSTALL_LIB_DIR}" CACHE PATH
					    "RPATH for installed libraries and executables" FORCE)
    else(IS_ABSOLUTE KLF_INSTALL_LIB_DIR)
      # Relative file path to install prefix
      set(CMAKE_INSTALL_RPATH "${CMAKE_INSTALL_PREFIX}/${KLF_INSTALL_LIB_DIR}" CACHE PATH
					    "RPATH for installed libraries and executables" FORCE)
    endif(IS_ABSOLUTE KLF_INSTALL_LIB_DIR)
    if(NOT klf_first_CMAKE_INSTALL_RPATH)
      KLFNote("Updating CMAKE_INSTALL_RPATH to \"${CMAKE_INSTALL_RPATH}\" (following change of CMAKE_INSTALL_PREFIX and/or KLF_INSTALL_LIB_DIR)")
    endif(NOT klf_first_CMAKE_INSTALL_RPATH)
  endif(wantSetNewRpath)
  mark_as_advanced(CLEAR CMAKE_INSTALL_RPATH)
  message(STATUS "RPATH for installed libraries and executables: ${CMAKE_INSTALL_RPATH} (CMAKE_INSTALL_RPATH)")
else(NOT CMAKE_SKIP_RPATH)
  message(STATUS "Skipping RPATH on executables and libraries (CMAKE_SKIP_RPATH)")
endif(NOT CMAKE_SKIP_RPATH)



# Run Post-Install Scripts?
# -------------------------

option(KLF_INSTALL_RUN_POST_INSTALL "Run post-install scripts after 'make install'" ON)
mark_as_advanced(KLF_INSTALL_RUN_POST_INSTALL)



# Uninstall target
# ----------------

configure_file(
  "${CMAKE_CURRENT_SOURCE_DIR}/cmake/cmake_uninstall.cmake.in"
  "${CMAKE_CURRENT_BINARY_DIR}/cmake_uninstall.cmake"
  IMMEDIATE @ONLY)
add_custom_target(uninstall
  "${CMAKE_COMMAND}" -P "${CMAKE_CURRENT_BINARY_DIR}/cmake_uninstall.cmake")


