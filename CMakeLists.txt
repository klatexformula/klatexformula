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

# Set some project settings
# -------------------------

# Installation destination
set(CMAKE_INSTALL_PREFIX "/usr" CACHE PATH "The installation prefix for KLatexFormula.")

# Compile backend only or full project ?
option(KLF_BACKEND_ONLY "If set to TRUE, only build the libklfbackend library, no GUI." OFF)

# Static/Shared libklfbackend
option(KLF_BACKEND_STATIC "Compile static libklfbackend backend library instead of shared." ON)

# See what Qt we have
set(QT_REQUIRED TRUE)
set(DESIRED_QT_VERSION 4 CACHE STRING "Set KLF_USE_QT4 Instead.")
include(FindQt)

# Which version of Qt to use...
if(QT4_INSTALLED)
  set(KLF_USE_QT4 TRUE CACHE BOOL "Set to FALSE if KLatexFormula should use Qt3 instead of Qt4")
else(QT4_INSTALLED)
  set(KLF_USE_QT4 FALSE CACHE BOOL "Set to FALSE if KLatexFormula should use Qt3 instead of Qt4")
  set(KLF_BACKEND_ONLY ON)
endif(QT4_INSTALLED)

# Build a shared or static source klfsrc library (for plugins to link to)
if(WIN32 OR APPLE)
  set(KLF_SRCLIB_STATIC FALSE CACHE BOOL "Build a static klfsrc source library")
else(WIN32 OR APPLE)
  set(KLF_SRCLIB_STATIC TRUE CACHE BOOL "Build a static klfsrc source library")
endif(WIN32 OR APPLE)

# Build DBUS support ?
if(NOT WIN32)
  set(KLF_USE_DBUS ON CACHE BOOL "Compiles D-Bus support into KLatexFormula GUI")
else(NOT WIN32)
  set(KLF_USE_DBUS OFF CACHE BOOL "Compiles D-Bus support into KLatexFormula GUI")
endif(NOT WIN32)

# Install .desktop & pixmaps in DEST/share/{applications|pixmaps} ?
option(KLF_INSTALL_DESKTOP "Install a .desktop file and pixmap in DESTINTATION/share/{applications|pixmaps}" TRUE)

# DEVELOPER OPTION to install klfbaseplugins.rcc in ~/.klatexformula/rccresources
option(KLF_DEVEL_LOCAL_BASEPLUGINS_COPY
       "FOR DEVELOPERS. Installs klfbaseplugins.rcc in ~/.klatexformula/rccresources AT COMPILE-TIME."
       FALSE)


# Some more local definitions for descendant cmake files
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

CONFIGURE_FILE(
  "${CMAKE_CURRENT_SOURCE_DIR}/cmake_uninstall.cmake.in"
  "${CMAKE_CURRENT_BINARY_DIR}/cmake_uninstall.cmake"
  IMMEDIATE @ONLY)
ADD_CUSTOM_TARGET(uninstall
  "${CMAKE_COMMAND}" -P "${CMAKE_CURRENT_BINARY_DIR}/cmake_uninstall.cmake")


# Configure doxygen targets
# -------------------------

# ................. TODO .....................

