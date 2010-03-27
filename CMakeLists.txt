# #################################### #
# CMake project file for klatexformula #
# #################################### #
# $Id$
# #################################### #
cmake_minimum_required(VERSION 2.6)

PROJECT(klatexformula)

# Set up CMAKE properly
set(CMAKE_INCLUDE_CURRENT_DIR TRUE)


# Read project version
file(READ VERSION KLF_VERSION)
message(STATUS "KLatexFormula Version is ${KLF_VERSION}")

set(klfversion_regex "([0-9]+)\\.([0-9]+)\\.([0-9]+)(.*)")
string(REGEX REPLACE "${klfversion_regex}" "\\1" KLF_VERSION_MAJ "${KLF_VERSION}")
string(REGEX REPLACE "${klfversion_regex}" "\\2" KLF_VERSION_MIN "${KLF_VERSION}")
string(REGEX REPLACE "${klfversion_regex}" "\\3" KLF_VERSION_REL "${KLF_VERSION}")
string(REGEX REPLACE "${klfversion_regex}" "\\4" KLF_VERSION_SUFFIX "${KLF_VERSION}")

include(KLFUtil.cmake)

# Set some project settings
# -------------------------

# Installation destination
set(CMAKE_INSTALL_PREFIX "" CACHE PATH "Set KLF_INSTALL_PREFIX instead.")
if(NOT DEFINED old_CMAKE_INSTALL_PREFIX)
  set(old_CMAKE_INSTALL_PREFIX ${CMAKE_INSTALL_PREFIX})
endif(NOT DEFINED old_CMAKE_INSTALL_PREFIX)
set(KLF_INSTALL_PREFIX ${CMAKE_INSTALL_PREFIX} CACHE PATH "The Base Installation Prefix Path.")
if(CMAKE_INSTALL_PREFIX STREQUAL old_CMAKE_INSTALL_PREFIX)
  # empty case
else(CMAKE_INSTALL_PREFIX STREQUAL old_CMAKE_INSTALL_PREFIX)
  # detected that user specified CMAKE_INSTALL_PREFIX (maybe by habit)
  set(old_CMAKE_INSTALL_PREFIX ${CMAKE_INSTALL_PREFIX} CACHE INTERNAL "for user change detection.")
  message(FATAL_ERROR "You set CMAKE_INSTALL_PREFIX. Please set KLF_INSTALL_PREFIX instead.")
endif(CMAKE_INSTALL_PREFIX STREQUAL old_CMAKE_INSTALL_PREFIX)
set(CMAKE_INSTALL_PREFIX ${KLF_INSTALL_PREFIX} CACHE PATH "Installation Prefix" FORCE)
set(old_CMAKE_INSTALL_PREFIX ${CMAKE_INSTALL_PREFIX} CACHE INTERNAL "for user change detection.")
# we really set CMAKE_INSTALL_PREFIX. KLF_INSTALL_PREFIX is a user interface variable.
message(STATUS "Will install to ${CMAKE_INSTALL_PREFIX}")

set(CMAKE_INSTALL_PREFIX "/usr" CACHE PATH "The installation prefix for KLatexFormula.")

# Compile the Gui project ?
option(KLF_BUILD_GUI "Build the full GUI interface" ON)

# Static/Shared libklfbackend
option(KLF_BACKEND_STATIC "Compile static libklfbackend backend library instead of shared." OFF)

# If we changed wanted Qt version, there are some conflicting qt cache variables
# we need to reset -> we reset all qt variables...
if(NOT DEFINED old_KLF_USE_QT4)
  set(old_KLF_USE_QT4 ${KLF_USE_QT4})
endif(NOT DEFINED old_KLF_USE_QT4)
if(KLF_USE_QT4 STREQUAL old_KLF_USE_QT4)
else(KLF_USE_QT4 STREQUAL old_KLF_USE_QT4)
  # value of KLF_USE_QT4 changed!
#  message("
#    *** WARNING ***
#    You changed the Qt version to use, while already having configured for another Qt version.
#    Usually some Qt detection cache variables subsist and interfere between Qt version detections;
#    if compilation fails, consider deleting the cache file CMakeCache.txt and re-running cmake
#    with the correct KLF_USE_QT4 given on first run.\n")
endif(KLF_USE_QT4 STREQUAL old_KLF_USE_QT4)

# See what Qt we have
set(QT_REQUIRED TRUE)
set(DESIRED_QT_VERSION 4 CACHE STRING "Set KLF_USE_QT4 Instead.")
include(FindQt)

# Which version of Qt to use...
if(QT4_INSTALLED)
  set(KLF_USE_QT4 TRUE CACHE BOOL "Set to FALSE if KLatexFormula should use Qt3 instead of Qt4")
else(QT4_INSTALLED)
  set(KLF_USE_QT4 FALSE CACHE BOOL "Set to FALSE if KLatexFormula should use Qt3 instead of Qt4")
endif(QT4_INSTALLED)
set(old_KLF_USE_QT4 ${KLF_USE_QT4} CACHE INTERNAL "detects user value change")
if(KLF_USE_QT4)
  message(STATUS "Using Qt 4")
else(KLF_USE_QT4)
  message(STATUS "Using Qt 3")
  if(KLF_BUILD_GUI)
    set(KLF_BUILD_GUI OFF CACHE BOOL "" FORCE)
    message("
    *** NOTE ***
    Cannot build GUI with Qt3. Forcing build GUI option off.\n")
  endif(KLF_BUILD_GUI)
endif(KLF_USE_QT4)
if(KLF_BUILD_GUI)
  message(STATUS "Will build the GUI interface")
else(KLF_BUILD_GUI)
  message(STATUS "Will not build the GUI interface")
endif(KLF_BUILD_GUI)

# Build a shared or static source klflib library (for plugins to link to)
if(WIN32 OR APPLE)
  set(KLF_KLFLIB_STATIC FALSE CACHE BOOL "Build a static klflib library")
else(WIN32 OR APPLE)
  set(KLF_KLFLIB_STATIC TRUE CACHE BOOL "Build a static klflib library")
endif(WIN32 OR APPLE)
if(KLF_KLFLIB_STATIC)
  message(STATUS "Building a static KLatexFormula library (klflib)")
else(KLF_KLFLIB_STATIC)
  message(STATUS "Building a shared KLatexFormula library (klflib)")
endif(KLF_KLFLIB_STATIC)

# Target architecture (allows compile for 32-bit on 64-bit architectures)
if(CMAKE_SIZEOF_VOID_P EQUAL 8)
  set(KLF_TARGET_ARCH_64 TRUE CACHE BOOL "Build for 64-bit platform")
else(CMAKE_SIZEOF_VOID_P EQUAL 8)
  set(KLF_TARGET_ARCH_64 FALSE CACHE BOOL "Build for 64-bit platform")
endif(CMAKE_SIZEOF_VOID_P EQUAL 8)
if(KLF_TARGET_ARCH_64)
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -m64")
  message(STATUS "Compiling for 64-bit architecture")
else(KLF_TARGET_ARCH_64)
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -m32")
  message(STATUS "Compiling for 32-bit architecture")
endif(KLF_TARGET_ARCH_64)

# Guess lib dir suffix on linux/unix
if(UNIX)
  execute_process(COMMAND test -d /lib64 -a ! -L /lib64 RESULT_VARIABLE nothavelib64)
  if(NOT nothavelib64 AND KLF_TARGET_ARCH_64)
    set(wantlib64 TRUE)
  else(NOT nothavelib64 AND KLF_TARGET_ARCH_64)
    set(wantlib64 FALSE)
  endif(NOT nothavelib64 AND KLF_TARGET_ARCH_64)
  execute_process(COMMAND test -d /lib32 -a ! -L /lib32 RESULT_VARIABLE nothavelib32)
  if(NOT nothavelib32 AND NOT KLF_TARGET_ARCH_64)
    set(wantlib32 TRUE)
  else(NOT nothavelib32 AND NOT KLF_TARGET_ARCH_64)
    set(wantlib32 FALSE)
  endif(NOT nothavelib32 AND NOT KLF_TARGET_ARCH_64)
  # detect change in value for updating KLF_LIB_SUFFIX
  if(KLF_TARGET_ARCH_64 STREQUAL old_KLF_TARGET_ARCH_64)
    # no change
    message(STATUS "Using \"${KLF_LIB_SUFFIX}\" as library directory suffix")
  else(KLF_TARGET_ARCH_64 STREQUAL old_KLF_TARGET_ARCH_64)
    # KLF_TARGET_ARCH_64 changed, update KLF_LIB_SUFFIX
    if(wantlib64)
      set(NEW_KLF_LIB_SUFFIX "64")
    elseif(wantlib32)
      set(NEW_KLF_LIB_SUFFIX "32")
    else(wantlib64)
      set(NEW_KLF_LIB_SUFFIX "")
    endif(wantlib64)
    if(KLF_LIB_SUFFIX STREQUAL NEW_KLF_LIB_SUFFIX)
    else(KLF_LIB_SUFFIX STREQUAL NEW_KLF_LIB_SUFFIX)
      set(KLF_LIB_SUFFIX ${NEW_KLF_LIB_SUFFIX} CACHE STRING
			      "Suffix to library directory (eg. '64' for /usr/lib64')" FORCE)
      message("
    *** NOTE ***
    The library suffix was detected and adjusted to \"${KLF_LIB_SUFFIX}\".\n")
    endif(KLF_LIB_SUFFIX STREQUAL NEW_KLF_LIB_SUFFIX)
  endif(KLF_TARGET_ARCH_64 STREQUAL old_KLF_TARGET_ARCH_64)
  set(old_KLF_TARGET_ARCH_64 ${KLF_TARGET_ARCH_64} CACHE INTERNAL "Used to detect user change.")
endif(UNIX)


# Build DBUS support ?
if(NOT WIN32)
  set(KLF_USE_DBUS ON CACHE BOOL "Compiles D-Bus support into KLatexFormula GUI")
else(NOT WIN32)
  set(KLF_USE_DBUS OFF CACHE BOOL "Compiles D-Bus support into KLatexFormula GUI")
endif(NOT WIN32)
if(KLF_USE_DBUS)
  message(STATUS "Building with D-BUS support")
else(KLF_USE_DBUS)
  message(STATUS "Building without D-BUS support")
endif(KLF_USE_DBUS)

# Install .desktop & pixmaps in DEST/share/{applications|pixmaps} ?
option(KLF_INSTALL_DESKTOP
       "Install a .desktop file and pixmap in DESTINTATION/share/{applications|pixmaps}" TRUE)
if(KLF_INSTALL_DESKTOP)
  message(STATUS "Will installing desktop files")
else(KLF_INSTALL_DESKTOP)
  message(STATUS "Will not install desktop files")
endif(KLF_INSTALL_DESKTOP)
  
# DEVELOPER OPTION to install klfbaseplugins.rcc in ~/.klatexformula/rccresources
option(KLF_DEVEL_LOCAL_BASEPLUGINS_COPY
       "FOR DEVELOPERS ONLY. Installs klfbaseplugins.rcc in ~/.klatexformula/rccresources AT COMPILE-TIME."
       FALSE)


# Some more local definitions for descendant cmake files
# copy command used in src/plugins/CMakeLists.txt
if(WIN32)
  set(KLF_CMAKE_COPY "copy")
else(WIN32)
  set(KLF_CMAKE_COPY "cp")
endif(WIN32)

# Some extra tests to know what we have
include(CheckCXXSourceCompiles)
CHECK_CXX_SOURCE_COMPILES("int main() { const char *s = __PRETTY_FUNCTION__; }"
			  has_pretty_function)
if(has_pretty_function)
add_definitions(-DKLF_CMAKE_HAS_PRETTY_FUNCTION)
endif(has_pretty_function)
CHECK_CXX_SOURCE_COMPILES("int main() { const char *s = __FUNCTION__; }"
			  has_function)
if(has_function)
add_definitions(-DKLF_CMAKE_HAS_FUNCTION)
endif(has_function)
CHECK_CXX_SOURCE_COMPILES("int main() { const char *s = __func__; }"
			  has_func)
if(has_func)
add_definitions(-DKLF_CMAKE_HAS_FUNC)
endif(has_func)


# Main subdirectory with all sources
# ----------------------------------
add_subdirectory(src)

# Uninstall option
# ----------------

configure_file(
  "${CMAKE_CURRENT_SOURCE_DIR}/cmake_uninstall.cmake.in"
  "${CMAKE_CURRENT_BINARY_DIR}/cmake_uninstall.cmake"
  IMMEDIATE @ONLY)
add_custom_target(uninstall
  "${CMAKE_COMMAND}" -P "${CMAKE_CURRENT_BINARY_DIR}/cmake_uninstall.cmake")


# Configure doxygen targets
# -------------------------

# ................. TODO .....................

