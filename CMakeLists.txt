# #################################### #
# CMake project file for klatexformula #
# #################################### #
# $Id$
# #################################### #
cmake_minimum_required(VERSION 2.6.4)

PROJECT(klatexformula)

# Set up CMAKE properly
set(CMAKE_INCLUDE_CURRENT_DIR TRUE)

set(CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake")

# Read project version
file(READ "VERSION" klf_ver)
string(STRIP "${klf_ver}" KLF_VERSION)
message(STATUS "KLatexFormula Version is ${KLF_VERSION}")

set(klfversion_regex "([0-9]+)\\.([0-9]+)\\.([0-9]+)(.*)")
string(REGEX REPLACE "${klfversion_regex}" "\\1" KLF_VERSION_MAJ "${KLF_VERSION}")
string(REGEX REPLACE "${klfversion_regex}" "\\2" KLF_VERSION_MIN "${KLF_VERSION}")
string(REGEX REPLACE "${klfversion_regex}" "\\3" KLF_VERSION_REL "${KLF_VERSION}")
string(REGEX REPLACE "${klfversion_regex}" "\\4" KLF_VERSION_SUFFIX "${KLF_VERSION}")

# The version that the libraries will be tagged with
set(KLF_LIB_VERSION "${KLF_VERSION_MAJ}.${KLF_VERSION_MIN}")

include(KLFUtil)

# Set some project settings
# -------------------------

set(CMAKE_BUILD_TYPE "Release" CACHE STRING "(FOR DEVELOPERS:) Build configuration (Debug|Release)")
mark_as_advanced(CLEAR CMAKE_BUILD_TYPE)
message(STATUS "Build Type: ${CMAKE_BUILD_TYPE} (CMAKE_BUILD_TYPE)")

# Compile the Gui project ?
option(KLF_BUILD_GUI "Build the full GUI interface" ON)

# Static/Shared libklfbackend
option(KLF_BACKEND_STATIC "Compile static libklfbackend backend library instead of shared." OFF)

# Choose a Qt Version to use
set(KLF_QT_VERSION 4 CACHE STRING "Qt major version to build with (4 or 3)")
KLFGetCMakeVarChanged(KLF_QT_VERSION)
if(klf_changed_KLF_QT_VERSION)
  # the KLF_QT_VERSION variable has changed value
  #  -> Reset some variables to let FindQt() find the right qmake
  set(QT_QMAKE_EXECUTABLE "" CACHE FILEPATH "qmake executable for Qt4" FORCE)
endif(klf_changed_KLF_QT_VERSION)
KLFMarkVarAdvancedIf(QT_QMAKE_EXECUTABLE "KLF_QT_VERSION NOTEQUAL 4")
# and set KLF_USE_QT4 (internal)
if(KLF_QT_VERSION EQUAL 3)
  set(KLF_USE_QT4 FALSE CACHE INTERNAL "TRUE if klatexformula is compiled with Qt4")
else(KLF_QT_VERSION EQUAL 3)
  set(KLF_USE_QT4 TRUE CACHE INTERNAL "TRUE if klatexformula is compiled with Qt4")
endif(KLF_QT_VERSION EQUAL 3)

# Include that version of Qt
set(QT_REQUIRED TRUE)
set(DESIRED_QT_VERSION "${KLF_QT_VERSION}" CACHE INTERNAL "FindQt's Qt Version to use")
include(FindQt)

if(NOT QT4_INSTALLED AND NOT QT3_INSTALLED)
  mark_as_advanced(CLEAR QT_QMAKE_EXECUTABLE)
  message(FATAL_ERROR
    "Didn't find Qt${KLF_QT_VERSION}. Try setting the QT_QMAKE_EXECUTABLE and/or QT_QMAKE_EXECUTABLE_FINDQT cache variable(s)." )
endif()
if(KLF_QT_VERSION EQUAL 3 AND NOT QT3_INSTALLED)
  mark_as_advanced(CLEAR QT_QMAKE_EXECUTABLE)
  message(FATAL_ERROR
    "Didn't find required Qt3. You may try setting QT_QMAKE_EXECUTABLE or using Qt4 by chaning the KLF_QT_VERSION cache variable.")
endif(KLF_QT_VERSION EQUAL 3 AND NOT QT3_INSTALLED)
if(KLF_QT_VERSION EQUAL 4 AND NOT QT4_INSTALLED)
  mark_as_advanced(CLEAR QT_QMAKE_EXECUTABLE)
  message(FATAL_ERROR
    "Didn't find required Qt4. You may try setting QT_QMAKE_EXECUTABLE or using Qt3 by chaning the KLF_QT_VERSION cache variable.")
endif(KLF_QT_VERSION EQUAL 4 AND NOT QT4_INSTALLED)
message(STATUS "Using Qt ${KLF_QT_VERSION} (KLF_QT_VERSION)")

# Static or shared backend
if(KLF_BACKEND_STATIC)
  message(STATUS "Building a static KLFBackend library (KLF_BACKEND_STATIC)")
else(KLF_BACKEND_STATIC)
  message(STATUS "Building a shared KLFBackend library (KLF_BACKEND_STATIC)")
endif(KLF_BACKEND_STATIC)

# Adjust KLF_BUILD_GUI to FALSE if we're using Qt3
KLFGetCMakeVarChanged(KLF_BUILD_GUI)
if(NOT KLF_USE_QT4)
  if(NOT klf_first_KLF_BUILD_GUI)
    if(KLF_BUILD_GUI)
      KLFNote("Cannot build GUI with Qt3. Forcing build GUI option off.")
    endif(KLF_BUILD_GUI)
  endif(NOT klf_first_KLF_BUILD_GUI)
  set(KLF_BUILD_GUI OFF CACHE BOOL
			  "Build the klatexformula graphical user interface (requires Qt4)." FORCE)
endif(NOT KLF_USE_QT4)
KLFCMakeSetVarChanged(KLF_BUILD_GUI)
if(KLF_BUILD_GUI)
  message(STATUS "Will build the GUI interface (KLF_BUILD_GUI)")
else(KLF_BUILD_GUI)
  message(STATUS "Will not build the GUI interface (KLF_BUILD_GUI)")
endif(KLF_BUILD_GUI)

# Build a shared or static source klflib library (for plugins to link to)
if(WIN32 OR APPLE)
  set(KLF_KLFLIB_STATIC FALSE CACHE BOOL "Build a static klflib library")
else(WIN32 OR APPLE)
  # could set to true, but have problems (?)
  set(KLF_KLFLIB_STATIC FALSE CACHE BOOL "Build a static klflib library")
endif(WIN32 OR APPLE)
if(KLF_BUILD_GUI)
  if(KLF_KLFLIB_STATIC)
    message(STATUS "Building a static KLatexFormula library (klflib) (KLF_KLFLIB_STATIC)")
  else(KLF_KLFLIB_STATIC)
    message(STATUS "Building a shared KLatexFormula library (klflib) (KLF_KLFLIB_STATIC)")
  endif(KLF_KLFLIB_STATIC)
endif(KLF_BUILD_GUI)

# Target architecture (allows compile for 32-bit on 64-bit architectures)
if(CMAKE_SIZEOF_VOID_P EQUAL 8)
  set(KLF_TARGET_ARCH_64 TRUE CACHE BOOL "Build for 64-bit platform")
else(CMAKE_SIZEOF_VOID_P EQUAL 8)
  set(KLF_TARGET_ARCH_64 FALSE CACHE BOOL "Build for 64-bit platform")
endif(CMAKE_SIZEOF_VOID_P EQUAL 8)
if(KLF_TARGET_ARCH_64)
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -m64")
  message(STATUS "Compiling for 64-bit architecture (KLF_TARGET_ARCH_64)")
else(KLF_TARGET_ARCH_64)
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -m32")
  message(STATUS "Compiling for 32-bit architecture (KLF_TARGET_ARCH_64)")
endif(KLF_TARGET_ARCH_64)

KLFGetCMakeVarChanged(KLF_LIB_SUFFIX)  # Also used later in klfinstallpaths.cmake
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
  KLFGetCMakeVarChanged(KLF_TARGET_ARCH_64)
  if(klf_first_KLF_TARGET_ARCH_64 OR klf_changed_KLF_TARGET_ARCH_64)
    # KLF_TARGET_ARCH_64 changed, update KLF_LIB_SUFFIX
    if(wantlib64)
      set(NEW_KLF_LIB_SUFFIX "64")
    elseif(wantlib32)
      set(NEW_KLF_LIB_SUFFIX "32")
    else(wantlib64)
      set(NEW_KLF_LIB_SUFFIX "")
    endif(wantlib64)
    if(DEFINED KLF_LIB_SUFFIX)
      set(klf_lib_suffix_redefined TRUE)
    endif(DEFINED KLF_LIB_SUFFIX)
    if(KLF_LIB_SUFFIX STREQUAL NEW_KLF_LIB_SUFFIX)
    else(KLF_LIB_SUFFIX STREQUAL NEW_KLF_LIB_SUFFIX)
      set(KLF_LIB_SUFFIX ${NEW_KLF_LIB_SUFFIX} CACHE STRING
			      "Suffix to library directory (eg. '64' for '/usr/lib64')" FORCE)
      if(klf_lib_suffix_redefined)
        KLFNote("The library suffix was detected and adjusted to \"${KLF_LIB_SUFFIX}\" (KLF_LIB_SUFFIX).")
      endif(klf_lib_suffix_redefined)
    endif(KLF_LIB_SUFFIX STREQUAL NEW_KLF_LIB_SUFFIX)
  else(klf_first_KLF_TARGET_ARCH_64 OR klf_changed_KLF_TARGET_ARCH_64)
    # no change
    message(STATUS "Using \"${KLF_LIB_SUFFIX}\" as library directory suffix (KLF_LIB_SUFFIX)")
  endif(klf_first_KLF_TARGET_ARCH_64 OR klf_changed_KLF_TARGET_ARCH_64)
endif(UNIX)

# Mac OS X: building frameworks + bundles ?
if(APPLE)
  option(KLF_MACOSX_BUNDLES
	 "On Mac OS X: Build Bundles and Frameworks instead of UNIX-Style binaries" ON)
endif(APPLE)

# Build DBUS support ?
if(NOT WIN32)
  set(KLF_USE_DBUS ON CACHE BOOL "Compiles D-Bus support into KLatexFormula GUI")
else(NOT WIN32)
  set(KLF_USE_DBUS OFF CACHE BOOL "Compiles D-Bus support into KLatexFormula GUI")
endif(NOT WIN32)
if(KLF_USE_DBUS)
  message(STATUS "Building with D-BUS support (KLF_USE_DBUS)")
else(KLF_USE_DBUS)
  message(STATUS "Building without D-BUS support (KLF_USE_DBUS)")
endif(KLF_USE_DBUS)

# DEVELOPER OPTION to install klfbaseplugins.rcc in ~/.klatexformula/rccresources
option(KLF_DEVEL_LOCAL_BASEPLUGINS_COPY
 "FOR DEVELOPERS ONLY. Installs klfbaseplugins.rcc in ~/.klatexformula/rccresources AT COMPILE-TIME."
 FALSE)
# DEVELOPER OPTION to test QAbstractItemModel-based models with ModelTest
option(KLF_DEBUG_USE_MODELTEST
 "FOR DEVELOPERS ONLY. Uses ModelTest (from Qt's Labs) to test QAbstractItemModel-based models"
 FALSE)
mark_as_advanced(KLF_DEVEL_LOCAL_BASEPLUGINS_COPY KLF_DEBUG_USE_MODELTEST)
if(KLF_DEVEL_LOCAL_BASEPLUGINS_COPY)
  message(STATUS "Will install klfbaseplugins.rcc in ~/.klatexformula/rccresources at compile-time (KLF_DEVEL_LOCAL_BASEPLUGINS_COPY)")
endif(KLF_DEVEL_LOCAL_BASEPLUGINS_COPY)
if(KLF_DEBUG_USE_MODELTEST)
  message(STATUS "Will compile with QtLab's ModelTest for KLFLibModel testing (KLF_DEBUG_USE_MODELTEST)")
endif(KLF_DEBUG_USE_MODELTEST)

# Some extra tests to know what we have
include(CheckCXXSourceCompiles)
CHECK_CXX_SOURCE_COMPILES("int main() { const char *s = __PRETTY_FUNCTION__; }"
			  has_pretty_function)
if(has_pretty_function)
add_definitions(-DKLF_HAS_PRETTY_FUNCTION)
endif(has_pretty_function)
CHECK_CXX_SOURCE_COMPILES("int main() { const char *s = __FUNCTION__; }"
			  has_function)
if(has_function)
add_definitions(-DKLF_HAS_FUNCTION)
endif(has_function)
CHECK_CXX_SOURCE_COMPILES("int main() { const char *s = __func__; }"
			  has_func)
if(has_func)
add_definitions(-DKLF_HAS_FUNC)
endif(has_func)

# Current System OS
if(APPLE)
set(internal_KLF_CMAKE_OS "macosx")
elseif(WIN32)
set(internal_KLF_CMAKE_OS "win32")
elseif(UNIX)
set(internal_KLF_CMAKE_OS "linux")
else(APPLE)
set(internal_KLF_CMAKE_OS "unknown")
endif(APPLE)
set(KLF_CMAKE_OS "${internal_KLF_CMAKE_OS}" CACHE INTERNAL "Current Build OS")
# Current System Arch
set(internal_KLF_CMAKE_ARCH "${CMAKE_SYSTEM_PROCESSOR}")
if(CMAKE_SYSTEM_PROCESSOR MATCHES "^i.86$")
  set(internal_KLF_CMAKE_ARCH "x86")
endif(CMAKE_SYSTEM_PROCESSOR MATCHES "^i.86$")
set(KLF_CMAKE_ARCH "${internal_KLF_CMAKE_ARCH}" CACHE INTERNAL "Current Build Processor Arch")

# Find KDE4 for KTextEditor Plugin
if(KLF_USE_QT4)
  if(UNIX)
    set(KLF_BUILD_KTEXTEDITORPLUGIN ON CACHE BOOL "Build the KTextEditor plugin")
  else(UNIX)
    set(KLF_BUILD_KTEXTEDITORPLUGIN OFF CACHE BOOL "Build the KTextEditor Plugin")
  endif(UNIX)
  if(KLF_BUILD_KTEXTEDITORPLUGIN)
    message(STATUS "Will build KTextEditor plugin (KLF_BUILD_KTEXTEDITORPLUGIN)")
  else(KLF_BUILD_KTEXTEDITORPLUGIN)
    message(STATUS "Will NOT build KTextEditor plugin (KLF_BUILD_KTEXTEDITORPLUGIN)")
  endif(KLF_BUILD_KTEXTEDITORPLUGIN)
else(KLF_USE_QT4)
  if(KLF_BUILD_KTEXTEDITORPLUGIN)
    set(KLF_BUILD_KTEXTEDITORPLUGIN OFF CACHE BOOL "Build the KTextEditor Plugin" FORCE)
    KLFNote("Not building the KTextEditor plugin because we're using Qt3.")
  endif(KLF_BUILD_KTEXTEDITORPLUGIN)
endif(KLF_USE_QT4)


# Install Paths
# -------------
# include after CPack declarations, so that changes in CMAKE_INSTALL_PREFIX take effect.
include(klfinstallpaths)


# CPack
# -----
include(klfcpack)



# Main subdirectory with all sources
# ----------------------------------
add_subdirectory(src)

KLFRemoveDummyInstallDirRule()

# Doxygen
# -------
include(klfdoxygen)

