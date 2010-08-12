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
message(STATUS "'make install' will install to \"${CMAKE_INSTALL_PREFIX}\" (CMAKE_INSTALL_PREFIX)")


# Installation Paths
# ------------------

# Lib Dir
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
if(NOT DEFINED KLF_INSTALL_LIB_DIR)
  set(install_lib_dir_needs_set TRUE)
endif(NOT DEFINED KLF_INSTALL_LIB_DIR)
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

# Bin Dir
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
  set(install_bin_dir_needs_set TRUE)
endif(klf_changed_CMAKE_INSTALL_PREFIX AND NOT klf_changed_KLF_INSTALL_BIN_DIR)
if(NOT DEFINED KLF_INSTALL_BIN_DIR)
  set(install_bin_dir_needs_set TRUE)
endif(NOT DEFINED KLF_INSTALL_BIN_DIR)
if(install_bin_dir_needs_set)
  set(KLF_INSTALL_BIN_DIR "${install_bin_dir}" CACHE STRING
		  "Binaries installation directory (relative to install prefix, or absolute)" FORCE)
  mark_as_advanced(CLEAR KLF_INSTALL_BIN_DIR)
  if(NOT klf_first_KLF_INSTALL_BIN_DIR)
    KLFNote("Updating binary install dir to ${KLF_INSTALL_BIN_DIR} following changes to CMAKE_INSTALL_PREFIX")
  endif(NOT klf_first_KLF_INSTALL_BIN_DIR)
  KLFCMakeSetVarChanged(KLF_INSTALL_BIN_DIR)
endif(install_bin_dir_needs_set)
message(STATUS "Installing binary to ${KLF_INSTALL_BIN_DIR} (KLF_INSTALL_BIN_DIR)")
# Utility variable. Same as KLF_INSTALL_BIN_DIR, but garanteed to be absolute path.
# Not kept in cache, it is (trivially) computed here
set(KLF_ABS_INSTALL_BIN_DIR "${KLF_INSTALL_BIN_DIR}")
if(NOT IS_ABSOLUTE "${KLF_INSTALL_BIN_DIR}")
  set(KLF_ABS_INSTALL_BIN_DIR "${CMAKE_INSTALL_PREFIX}/${KLF_INSTALL_BIN_DIR}")
endif(NOT IS_ABSOLUTE "${KLF_INSTALL_BIN_DIR}")

# rccresources dir
if(WIN32)
  set(KLF_INSTALL_RCCRESOURCES_DIR  "${KLF_INSTALL_BIN_DIR}/rccresources" CACHE PATH
							      "Where to install rccresources files")
  # and check if it was changed
  KLFGetCMakeVarChanged(KLF_INSTALL_RCCRESOURCES_DIR)
  
  if(klf_changed_KLF_INSTALL_BIN_DIR OR klf_changed_CMAKE_INSTALL_PREFIX)
    set(rccres_dir_deps_changed TRUE)
  endif(klf_changed_KLF_INSTALL_BIN_DIR OR klf_changed_CMAKE_INSTALL_PREFIX)
  
  if(rccres_dir_deps_changed AND NOT klf_changed_KLF_INSTALL_RCCRESOURCES_DIR)
    # generic option changed, then a non-changed specific option must follow
    set(KLF_INSTALL_RCCRESOURCES_DIR  "${KLF_INSTALL_BIN_DIR}/rccresources" CACHE PATH
							      "Where to install rccresources files")
    if(NOT klf_first_KLF_INSTALL_RCCRESOURCES_DIR)
      KLFNote("Updating KLF_INSTALL_RCCRESOURCES_DIR to \"${KLF_INSTALL_RCCRESOURCES_DIR}\" following changes to other variables")
    endif(NOT klf_first_KLF_INSTALL_RCCRESOURCES_DIR)
  endif(rccres_dir_deps_changed AND NOT klf_changed_KLF_INSTALL_RCCRESOURCES_DIR)
  
  KLFCMakeSetVarChanged(KLF_INSTALL_RCCRESOURCES_DIR)
  
else(WIN32)
  set(KLF_INSTALL_RCCRESOURCES_DIR "share/klatexformula/rccresources" CACHE STRING
			      "Where to install rccresources files (see also KLF_INSTALL_PLUGINS)")
endif(WIN32)


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


# What to Install?
# ----------------

# general options
option(KLF_INSTALL_RUNTIME "Install run-time files (binaries, so libraries, plugins)" YES)
option(KLF_INSTALL_DEVEL "Install development files (headers, static libraries)" YES)

KLFGetCMakeVarChanged(KLF_INSTALL_RUNTIME)
KLFGetCMakeVarChanged(KLF_INSTALL_DEVEL)

# fine-tuning
# the OFF's at end of line is to turn off the KLFNotes() indicating the value changed
KLFDeclareCacheVarOptionFollow(KLF_INSTALL_KLFBACKEND_HEADERS      KLF_INSTALL_DEVEL  BOOL "Install klfbackend headers" OFF)
KLFDeclareCacheVarOptionFollow(KLF_INSTALL_KLFBACKEND_STATIC_LIBS  KLF_INSTALL_DEVEL  BOOL "Install klfbackend static libraries" OFF)
KLFDeclareCacheVarOptionFollow(KLF_INSTALL_KLFBACKEND_SO_LIBS      KLF_INSTALL_RUNTIME  BOOL "Install klfbackend so libraries" OFF)
KLFDeclareCacheVarOptionFollow(KLF_INSTALL_KLFBACKEND_FRAMEWORK    KLF_INSTALL_RUNTIME  BOOL "Install klfbackend framework (Mac OS X)" OFF)

if(KLF_BUILD_GUI)
  KLFDeclareCacheVarOptionFollow(KLF_INSTALL_KLFLIB_HEADERS      KLF_INSTALL_DEVEL  BOOL "Install klflib headers" OFF)
  KLFDeclareCacheVarOptionFollow(KLF_INSTALL_KLFLIB_STATIC_LIBS  KLF_INSTALL_DEVEL  BOOL "Install klflib static libraries" OFF)
  KLFDeclareCacheVarOptionFollow(KLF_INSTALL_KLFLIB_SO_LIBS      KLF_INSTALL_RUNTIME  BOOL "Install klflib so libraries" OFF)
  KLFDeclareCacheVarOptionFollow(KLF_INSTALL_KLFLIB_FRAMEWORK    KLF_INSTALL_RUNTIME  BOOL "Install klflib framework (Mac OS X)" OFF)
  KLFDeclareCacheVarOptionFollow(KLF_INSTALL_KLATEXFORMULA_BIN   KLF_INSTALL_RUNTIME  BOOL "Install klatexformula binary" OFF)
  KLFDeclareCacheVarOptionFollow(KLF_INSTALL_KLATEXFORMULA_CMDL  KLF_INSTALL_RUNTIME  BOOL "Install klatexformula_cmdl symlink" OFF)
  KLFDeclareCacheVarOptionFollow(KLF_INSTALL_KLATEXFORMULA_BUNDLE KLF_INSTALL_RUNTIME  BOOL "Install klatexformula bundle (Mac OS X)" OFF)
endif(KLF_BUILD_GUI)

if(NOT KLF_MACOSX_BUNDLES)
  KLFDeclareCacheVarOptionFollow(KLF_INSTALL_PLUGINS   KLF_INSTALL_RUNTIME  BOOL "Install klatexformula plugins" OFF)
else(NOT KLF_MACOSX_BUNDLES)
  if(KLF_INSTALL_PLUGINS)
    KLFNote("Not Installing plugins outside bundle.")
    set(KLF_INSTALL_PLUGINS  OFF  CACHE BOOL "Not needed. plugins are incorporated to bundle instead." FORCE)
  endif(KLF_INSTALL_PLUGINS)
endif(NOT KLF_MACOSX_BUNDLES)

message(STATUS "Will install (irrelevant items eg. bundles on linux, are ignored):\n
             \theaders\t\tstatic,\t    shared libraries\tframework
 klfbackend: \t  ${KLF_INSTALL_KLFBACKEND_HEADERS}\t\t  ${KLF_INSTALL_KLFBACKEND_STATIC_LIBS}\t\t  ${KLF_INSTALL_KLFBACKEND_SO_LIBS}\t\t  ${KLF_INSTALL_KLFBACKEND_FRAMEWORK}
 klflib:     \t  ${KLF_INSTALL_KLFLIB_HEADERS}\t\t  ${KLF_INSTALL_KLFLIB_STATIC_LIBS}\t\t  ${KLF_INSTALL_KLFLIB_SO_LIBS}\t\t  ${KLF_INSTALL_KLFLIB_FRAMEWORK}

 klatexformula: \t${KLF_INSTALL_KLATEXFORMULA_BIN}
 klatexformula_cmdl: \t${KLF_INSTALL_KLATEXFORMULA_CMDL}
 klatexformula bundle: \t${KLF_INSTALL_KLATEXFORMULA_BUNDLE}
 
 individual installs can be tuned with KLF_INSTALL_{KLFLIB|KLFBACKEND}_{HEADERS|SO_LIBS|STATIC_LIBS|FRAMEWORK}
 and KLF_INSTALL_KLATEXFORMULA_{BIN|CMDL|BUNDLE}
")

# Install .desktop & pixmaps in DEST/share/{applications|pixmaps} ?
if(KLF_MACOSX_BUNDLES OR WIN32)
  set(default_KLF_INSTALL_DESKTOP FALSE)
else(KLF_MACOSX_BUNDLES OR WIN32)
  set(default_KLF_INSTALL_DESKTOP TRUE)
endif(KLF_MACOSX_BUNDLES OR WIN32)

option(KLF_INSTALL_DESKTOP
  "Install a .desktop file and pixmap in DESTINTATION/share/{applications|pixmaps}; update mime database"
  ${default_KLF_INSTALL_DESKTOP} )

if(KLF_INSTALL_DESKTOP)
  set(KLF_INSTALL_DESKTOP_CATEGORIES "Qt;Office;" CACHE STRING "Categories section in .desktop file")
  message(STATUS "Will install linux desktop files (KLF_INSTALL_DESKTOP)")
  message(STATUS "  categories \"${KLF_INSTALL_DESKTOP_CATEGORIES}\" (KLF_INSTALL_DESKTOP_CATEGORIES)")
else(KLF_INSTALL_DESKTOP)
  message(STATUS "Will not install linux desktop files (KLF_INSTALL_DESKTOP)")
endif(KLF_INSTALL_DESKTOP)

if(WIN32)
  option(KLF_INSTALL_QTLIBS "Copy Qt Libs next to installed executable" ON)
  if(KLF_INSTALL_QTLIBS)
    message(STATUS "Will install Qt libs next to installed executable (KLF_INSTALL_QTLIBS)")
  else(KLF_INSTALL_QTLIBS)
    message(STATUS "Will NOT install Qt libs next to installed executable (KLF_INSTALL_QTLIBS)")
  endif(KLF_INSTALL_QTLIBS)
endif(WIN32)


# Run Post-Install Scripts?
# -------------------------
# eg. for packaging, post-install scripts should be given in the RPM/deb/... definition
# and not be done at RPM creation time
option(KLF_INSTALL_RUN_POST_INSTALL "Run post-install scripts after 'make install'" YES)
if(KLF_INSTALL_RUN_POST_INSTALL)
  message(STATUS "Will run post-install scripts (KLF_INSTALL_RUN_POST_INSTALL)")
else(KLF_INSTALL_RUN_POST_INSTALL)
  message(STATUS "Will NOT run post-install scripts (KLF_INSTALL_RUN_POST_INSTALL)")
endif(KLF_INSTALL_RUN_POST_INSTALL)


set(klf_dummyinstdir "${CMAKE_BINARY_DIR}/dummy_install_prefix")

macro(KLFInstallLibrary targetlib varOptBase inst_lib_dir inst_pubheader_dir)
  KLFConditionalSet(inst_${targetlib}_runtime_dir ${varOptBase}SO_LIBS "${inst_lib_dir}" "${klf_dummyinstdir}")
  KLFConditionalSet(inst_${targetlib}_library_dir ${varOptBase}SO_LIBS "${inst_lib_dir}" "${klf_dummyinstdir}")
  KLFConditionalSet(inst_${targetlib}_archive_dir ${varOptBase}STATIC_LIBS "${inst_lib_dir}" "${klf_dummyinstdir}")
  KLFConditionalSet(inst_${targetlib}_framework_dir ${varOptBase}FRAMEWORK "${inst_lib_dir}" "${klf_dummyinstdir}")
  KLFConditionalSet(inst_${targetlib}_pubheader_dir ${varOptBase}HEADERS "${inst_pubheader_dir}" "${klf_dummyinstdir}")
  install(TARGETS ${targetlib}
	  RUNTIME DESTINATION "${inst_${targetlib}_runtime_dir}"
	  LIBRARY DESTINATION "${inst_${targetlib}_library_dir}"
	  ARCHIVE DESTINATION "${inst_${targetlib}_archive_dir}"
	  FRAMEWORK DESTINATION "${inst_${targetlib}_framework_dir}"
	  PUBLIC_HEADER DESTINATION "${inst_${targetlib}_pubheader_dir}"
  )
endmacro(KLFInstallLibrary)

macro(KLFRemoveDummyInstallDirRule)
  install(CODE "message(\"Removing dummy installation directory '${klf_dummyinstdir}'\")
  execute_process(COMMAND \"${CMAKE_COMMAND}\" -E remove_directory \"\$ENV{DESTDIR}${klf_dummyinstdir}\")")
endmacro(KLFRemoveDummyInstallDirRule)



# Uninstall target
# ----------------

configure_file(
  "${CMAKE_CURRENT_SOURCE_DIR}/cmake/cmake_uninstall.cmake.in"
  "${CMAKE_CURRENT_BINARY_DIR}/cmake_uninstall.cmake"
  IMMEDIATE @ONLY)
add_custom_target(uninstall
  "${CMAKE_COMMAND}" -P "${CMAKE_CURRENT_BINARY_DIR}/cmake_uninstall.cmake")


