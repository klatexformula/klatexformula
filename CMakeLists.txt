# #################################### #
# CMake project file for klatexformula #
# #################################### #
# $Id$
# #################################### #
cmake_minimum_required(VERSION 3.1)

PROJECT(klatexformula)

# Set up CMAKE properly

set(CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake")


if(APPLE)
  # find HomeBrew Qt5, if present
  set(CMAKE_PREFIX_PATH ${CMAKE_PREFIX_PATH} "/usr/local/opt/qt5/lib/cmake")
endif(APPLE)


# Read project version
file(READ "VERSION" klf_ver)
string(STRIP "${klf_ver}" KLF_VERSION)
message(STATUS "KLatexFormula Version ${KLF_VERSION}")
option(KLF_WELCOME_MSG_SILENT "Don't display welcome message" OFF)
if(NOT KLF_WELCOME_MSG_SILENT)
  message(STATUS "
   Welcome to the klatexformula build script. This CMake script will configure
   the klatexformula ${KLF_VERSION} build process for your system.

   Some settings will have to be detected. This script should be able to detect
   the correct settings for most systems. However, you may want to tune and even
   fine-tune this configuration in order to build and install the compenents you
   would like, and install them at your preferred locations.

   Each status message displays a default or detected value for a setting. In
   capital letters is given the corresponding CMake cache variable name, in case
   you want to change the setting. With command-line cmake, you can set a
   variable with:
     cmake <source-dir> -D<VARIABLE_NAME>=<VALUE>
   In most cases values are just 'ON' or 'OFF'. In some other cases they are paths.

   But don't worry, most default values should be fine. Unless an error was
   reported during this script, you can type
       make
   and
       sudo make install
   to install klatexformula on your system.
   
   For more options and help:
      - look at the stored cache variable documentation (displayed eg. by cmake-gui
        and ccmake, or directly in the CMakeCache.txt file)
      - take a look at
http://klatexformula.sourceforge.net/klfwiki/index.php/User_Manual:Downloading_%26_Installing

   And have a lot of fun!

")
  message(STATUS "Displayed welcome message (KLF_WELCOME_MSG_SILENT)")
else(NOT KLF_WELCOME_MSG_SILENT)
  message(STATUS "Skipped welcome message (KLF_WELCOME_MSG_SILENT)")
endif(NOT KLF_WELCOME_MSG_SILENT)


# Extract KLF Version from VERSION file
set(klfversion_regex "([0-9]+)\\.([0-9]+)\\.([0-9]+)(.*)")
string(REGEX REPLACE "${klfversion_regex}" "\\1" KLF_VERSION_MAJ "${KLF_VERSION}")
string(REGEX REPLACE "${klfversion_regex}" "\\2" KLF_VERSION_MIN "${KLF_VERSION}")
string(REGEX REPLACE "${klfversion_regex}" "\\3" KLF_VERSION_REL "${KLF_VERSION}")
string(REGEX REPLACE "${klfversion_regex}" "\\4" KLF_VERSION_SUFFIX "${KLF_VERSION}")

# The version that the libraries will be tagged with
set(KLF_LIB_VERSION "${KLF_VERSION_MAJ}")

include(KLFUtil)


# Warn the user for defined cache variables that are not standard,
# i.e. may be a user's typo -- recent CMake does that automatically
#include(klfValidOptions)


# Set some project settings
# -------------------------

message(STATUS "")
message(STATUS "[BUILD SETTINGS]")

set(CMAKE_BUILD_TYPE "Release" CACHE STRING "[For developers] Build configuration (Debug|Release|RelWithDebInfo)")

if(NOT CMAKE_BUILD_TYPE)
  message(FATAL_ERROR "Please set CMAKE_BUILD_TYPE, for example CMAKE_BUILD_TYPE=Release.")
endif()
message(STATUS "CMake build type: ${CMAKE_BUILD_TYPE}")

if(CMAKE_BUILD_TYPE STREQUAL "Debug" OR CMAKE_BUILD_TYPE STREQUAL "RelWithDebInfo")
  set(klf_debug_dflt 1)
else()
  set(klf_debug_dflt 0)
endif()
set(KLF_DEBUG "${klf_debug_dflt}" CACHE BOOL "Enable klf debugging messages (boolean)")

if(KLF_DEBUG)
  message(STATUS "Debugging messages in KLF libraries and applciation are enabled (KLF_DEBUG)")
else()
  message(STATUS "Debugging messages in KLF libraries and applciation are disabled (KLF_DEBUG)")
endif()

# experimental features
option(KLF_EXPERIMENTAL
  "DEVELOPERS ONLY. Enable experimental unstable features (NOT recommended for regular use!)"
  off)
mark_as_advanced(KLF_EXPERIMENTAL)
if(KLF_EXPERIMENTAL)
  KLFNote("WARNING: Experimental features are unstable and are meant only for developers!! Use at your own risk!!")
  message(STATUS "Enabling experimental features (KLF_EXPERIMENTAL)")
endif(KLF_EXPERIMENTAL)


# Find Qt 5
find_package(Qt5Core REQUIRED)
find_package(Qt5Gui REQUIRED)
find_package(Qt5Widgets REQUIRED)
find_package(Qt5Xml REQUIRED)


#
# by default, libraries are dynamic unless overridden by user options
#
set(klf_default_libraries_static 0)

#
# KLFTOOLS
#

# must be set to ON if you want to build anything at all.
option(KLF_BUILD_TOOLS "Build klatexformula tools library (klftools). Necessary for everything else." 1)
option(KLF_LIBKLFTOOLS_STATIC "Build a static klftools library" ${klf_default_libraries_static})
if(KLF_BUILD_TOOLS)
  message(STATUS "Will build the klftools library (KLF_BUILD_TOOLS)")
  if(KLF_LIBKLFTOOLS_STATIC)
    message(STATUS "Building a static KLatexFormula tools library (klftools) (KLF_LIBKLFTOOLS_STATIC)")
  else()
    message(STATUS "Building a shared KLatexFormula tools library (klftools) (KLF_LIBKLFTOOLS_STATIC)")
  endif()
else()
#  message(FATAL_ERROR "There is nothing to build. You must set at least KLF_BUILD_TOOLS.")
endif()

set(KLF_BUILD_TOOLSDESPLUGIN "$<NOT:$<KLF_LIBKLFTOOLS_STATIC>>" TRUE CACHE BOOL
  "Build Qt Designer Plugin for klftools library widgets")
if(KLF_BUILD_TOOLSDESPLUGIN)
  if(KLF_LIBKLFTOOLS_STATIC)
    message(FATAL_ERROR "Cannot build klftools designer plugin library with static klftools library (set KLF_BUILD_TOOLSDESPLUGIN=0 or KLF_LIBKLFTOOLS_STATIC=0)")
  endif()
  message(STATUS "Will build the klftools designer plugin library (KLF_BUILD_TOOLSDESPLUGIN)")
else()
  message(STATUS "Will not build the klftools designer plugin library (KLF_BUILD_TOOLSDESPLUGIN)")
endif()

#
# KLFBACKEND
#

option(KLF_BUILD_BACKEND "Build klatexformula backend library (klfbackend)" 1)
option(KLF_LIBKLFBACKEND_STATIC "Compile static libklfbackend backend library instead of shared."
  ${klf_default_libraries_static})
if(KLF_BUILD_BACKEND)
  if (NOT KLF_BUILD_TOOLS)
    message(FATAL_ERROR "Cannot build klfbackend without klftools (set KLF_BUILD_BACKEND=0 or KLF_BUILD_TOOLS=1)")
  endif()
  message(STATUS "Will build the klfbackend library (KLF_BUILD_BACKEND)")
  if(KLF_LIBKLFBACKEND_STATIC)
    message(STATUS "Building a static KLFBackend library (KLF_LIBKLFBACKEND_STATIC)")
  else()
    message(STATUS "Building a shared KLFBackend library (KLF_LIBKLFBACKEND_STATIC)")
  endif()
else()
  message(STATUS "Will not build the klfbackend library (KLF_BUILD_BACKEND)")
endif()


option(KLF_BUILD_BACKEND_AUTO "Build klatexformula autonomous backend library (klfbackend_auto)" 1)
option(KLF_LIBKLFBACKEND_AUTO_STATIC "Compile static libklfbackend_auto backend library instead of shared."
       TRUE)
if(KLF_BUILD_BACKEND_AUTO)
  message(STATUS "Will build autonomous klfbackend_auto without klftools dependency (KLF_BUILD_BACKEND_AUTO)")
  if(KLF_LIBKLFBACKEND_AUTO_STATIC)
    message(STATUS "Building a static klfbackend_auto autonomous library (KLF_LIBKLFBACKEND_AUTO_STATIC)")
  else()
    message(STATUS "Building a shared klfbackend_auto autonomous library (KLF_LIBKLFBACKEND_AUTO_STATIC)")
  endif()
else()
  message(STATUS "Will not build autonomous klfbackend_auto without klftools dependency (KLF_BUILD_BACKEND_AUTO)")
endif()

option(KLF_BUILD_GUI "Build klatexformula GUI application (klatexformula) [requires klftools and klfbackend]" 1)
if(KLF_BUILD_GUI)
  if (NOT KLF_BUILD_TOOLS OR NOT KLF_BUILD_BACKEND)
    message(FATAL_ERROR "Cannot build the klatexformula GUI main application without klftools and klfbackend (set KLF_BUILD_BACKEND=1 and KLF_BUILD_TOOLS=1, or set KLF_BUILD_GUI=0)")
  endif()
  message(STATUS "Will build the GUI interface (klatexformula) (KLF_BUILD_GUI)")
else()
  message(STATUS "Will not build the GUI interface (klatexformula) (KLF_BUILD_GUI)")
endif()

if(APPLE)
  # These settings override on Mac OS X
  if(CMAKE_OSX_ARCHITECTURES)
    set(display_CMAKE_OSX_ARCHITECTURES "${CMAKE_OSX_ARCHITECTURES}")
  else(CMAKE_OSX_ARCHITECTURES)
    set(display_CMAKE_OSX_ARCHITECTURES "(default)")
  endif(CMAKE_OSX_ARCHITECTURES)
  message(STATUS "Compiling for Mac OS X Architectures: ${display_CMAKE_OSX_ARCHITECTURES} (CMAKE_OSX_ARCHITECTURES)")
  if(KLF_TARGET_ARCH_64 OR klf_changed_KLF_TARGET_ARCH_64)
    KLFNote("Warning: KLF_TARGET_ARCH_64 ignored on Mac OS X. Use CMAKE_OSX_ARCHITECTURES instead.")
  endif(KLF_TARGET_ARCH_64 OR klf_changed_KLF_TARGET_ARCH_64)
endif()


# Mac OS X: building frameworks + bundles ?
if(APPLE)
  option(KLF_MACOSX_BUNDLES
	 "On Mac OS X: Build Bundles and Frameworks instead of UNIX-Style binaries" ON)
  option(KLF_MACOSX_BUNDLE_EXTRAS
	 "[Mac OS X Bundles] Copy dependent libraries & resources into bundle, ready to be copied anywhere else and on other systems" ON)
endif()

# Build DBUS support ?
if(UNIX AND NOT APPLE)
  set(KLF_USE_DBUS ON CACHE BOOL "Compiles D-Bus support into KLatexFormula GUI")
else()
  set(KLF_USE_DBUS OFF CACHE BOOL "Compiles D-Bus support into KLatexFormula GUI")
endif()
if(KLF_USE_DBUS)
  message(STATUS "Building with D-BUS support (KLF_USE_DBUS)")
else()
  message(STATUS "Building without D-BUS support (KLF_USE_DBUS)")
endif()

# DEVELOPER OPTION to test QAbstractItemModel-based models with ModelTest
option(KLF_DEBUG_USE_MODELTEST
 "DEVELOPERS ONLY. Uses ModelTest (Qt's Labs) to test QAbstractItemModel-based models. VERY slow."
 FALSE)
mark_as_advanced(KLF_DEBUG_USE_MODELTEST)
if(KLF_DEVEL_LOCAL_BASEPLUGINS_COPY)
  message(STATUS "Will install plugins in ~/.klatexformula/plugins/ at compile-time (KLF_DEVEL_LOCAL_BASEPLUGINS_COPY)")
endif(KLF_DEVEL_LOCAL_BASEPLUGINS_COPY)
if(KLF_DEBUG_USE_MODELTEST)
  message(STATUS "Will compile with QtLab's ModelTest for KLFLibModel testing (KLF_DEBUG_USE_MODELTEST)")
endif(KLF_DEBUG_USE_MODELTEST)

# Include some extra data files/fonts into KLatexFormula
set(klf_def_incl_fonts "data/cmunsi.otf;data/cmunso.otf;data/cmunss.otf;data/cmunsx.otf") # CMU Sans Serif
if(APPLE)
  set(klf_def_incl_fonts
    "data/cmunbbx.otf;data/cmunbmo.otf;data/cmunbmr.otf;data/cmunbso.otf;data/cmunbsr.otf;data/cmunbxo.otf") # CMU Bright
endif(APPLE)
set(KLF_INCLUDE_FONTS "${klf_def_incl_fonts}"
  CACHE STRING "Include some extra fonts into KLF's internal resources (absolute or relative to src/)")
string(REPLACE ";" "\n        " klf_include_fonts_msg_list "${KLF_INCLUDE_FONTS}")
message(STATUS "Will include fonts into klatexformula's resources (KLF_INCLUDE_FONTS):\n        ${klf_include_fonts_msg_list}")

# Use system font rather than CMU font if available
#KLFDeclareCacheVarOptionFollowComplexN(specificoption cachetype cachestring updatenotice calcoptvalue depvarlist)
option(KLF_NO_CMU_FONT
  "Tells klatexformula to use system font instead of Computer Modern font"
  FALSE
  )
if(KLF_NO_CMU_FONT)
  message(STATUS "Will use system font instead of CMU font (KLF_NO_CMU_FONT)")
else()
  message(STATUS "Will attempt to use Computer Modern unicode font (KLF_NO_CMU_FONT)")
endif()


# Use Sparkle framework under Mac OS X?
if(APPLE)
  option(KLF_USE_SPARKLE "Use Sparkle Framework for auto-updates on Mac OS X" OFF)
  # can only use if we are building a bundle
  if (NOT KLF_MACOSX_BUNDLES)
    message(FATAL_ERROR "Can only build with Sparkle framework when we are building a Mac OS X App bundle (use KLF_MACOSX_BUNDLES=1)")
  endif()

  if(KLF_USE_SPARKLE)
    set(KLF_SPARKLE_FEED_URL "http://klatexformula.sourceforge.net/sparklefeed.xml"
      CACHE STRING "URL for Sparkle to check for updates")
    set(KLF_SPARKLE_FRAMEWORK "Sparkle.framework"
      CACHE STRING "Path to the Sparkle Framework")
    set(KLF_SPARKLE_DSA_PUBKEY "" CACHE STRING "Path to the DSA public key for signing packages.")
    if(NOT EXISTS "${KLF_SPARKLE_FRAMEWORK}")
      message(FATAL_ERROR "Please specify the path to the Sparkle Framework in KLF_SPARKLE_FRAMEWORK, or disable the Sparkle framework with KLF_USE_SPARKLE=off.")
    endif()
    message(STATUS "Using sparkle framework with update URL ${KLF_SPARKLE_FEED_URL}")
  endif()
endif(APPLE)

# Use WinSparkle framework under Windows?
if(WIN32)
  option(KLF_USE_WINSPARKLE "Use WinSparkle for auto-updates on MS Windows" OFF)
  if(KLF_USE_WINSPARKLE)
    set(KLF_WINSPARKLE_FEED_URL "http://klatexformula.sourceforge.net/sparklefeed.xml"
      CACHE STRING "URL for WinSparkle to check for updates")
    set(KLF_WINSPARKLE_DIR ""
      CACHE STRING "Path to the WinSparkle Directory (with .h files & WinSparkle.dll file)")
    if(NOT EXISTS "${KLF_WINSPARKLE_DIR}")
      message(FATAL_ERROR "Please specify the path to the WinSparkle directory in KLF_WINSPARKLE_DIR, or disable the Sparkle framework with KLF_USE_WINSPARKLE=off.")
    endif()
    message(STATUS "Using WinSparkle with update URL ${KLF_WINSPARKLE_FEED_URL}")
  endif()
endif()


# Manpage generation from --help and --version help text
if(NOT KLF_MACOSX_BUNDLES)
  if(NOT DEFINED HELP2MAN OR HELP2MAN STREQUAL "")
    find_program(HELP2MAN "help2man")
  endif()
endif()
if(HELP2MAN)
  message(STATUS "Manpage will be generated from --help text with help2man (HELP2MAN=OFF or /path/to/help2man)")
elseif(NOT KLF_MACOSX_BUNDLES AND NOT WIN32 AND HELP2MAN STREQUAL "")
  KLFNote("help2man was not found. Manpage will not be generated.
    set explicitely HELP2MAN to /path/.../to/help2man if needed.")
endif()
if(NOT HELP2MAN)
  message(STATUS "Manpage will not be generated with help2man (HELP2MAN=OFF or /path/to/help2man)")
endif()
if(NOT DEFINED GZIP OR GZIP STREQUAL "")
  find_program(GZIP "gzip")
endif()
if(HELP2MAN)
  if(GZIP)
    message(STATUS "Manpage will be gzip'ed (GZIP=OFF or /path/to/gzip)")
  else()
    message(STATUS "Manpage will not be gzip'ed (GZIP=OFF or /path/to/gzip)")
  endif()
endif(HELP2MAN)


# Install Paths
# -------------

message(STATUS "")
message(STATUS "[INSTALLATION SETTINGS]")

include(klfinstallpaths)

if(KLF_MACOSX_BUNDLES)
  set(default_KLF_BUNDLE_QT_PLUGINS
	imageformats/libqgif.dylib
	imageformats/libqico.dylib
	imageformats/libqjpeg.dylib
	imageformats/libqmng.dylib
	imageformats/libqtiff.dylib
	codecs/libqcncodecs.dylib
	codecs/libqjpcodecs.dylib
	codecs/libqkrcodecs.dylib
	codecs/libqtwcodecs.dylib
	sqldrivers/libqsqlite.dylib)
  set(KLF_BUNDLE_QT_PLUGINS "${default_KLF_BUNDLE_QT_PLUGINS}"
					  CACHE STRING "Qt plugins to incorporate into Mac bundle")
  string(REGEX REPLACE ";" "
    * " klf_display_bundle_qt_plugins "${KLF_BUNDLE_QT_PLUGINS}")
  message(STATUS "Will Package Qt Plugins in bundle
    * ${klf_display_bundle_qt_plugins}
    (KLF_BUNDLE_QT_PLUGINS)")
endif(KLF_MACOSX_BUNDLES)


# CPack
# -----
include(klfcpack)


# Some Basic Compilation Definitions
# ----------------------------------

# Preprocessor instructions
add_definitions(-DKLF_VERSION_STRING="${KLF_VERSION}"
		-DKLF_VERSION_MAJ=${KLF_VERSION_MAJ}
		-DKLF_VERSION_MIN=${KLF_VERSION_MIN}
		-DKLF_VERSION_REL=${KLF_VERSION_REL}
		-DKLF_SRC_BUILD)
# Add warning flags for DEBUG build configuration
set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -Wall -Wextra")
if(KLF_DEBUG)
  add_definitions(-DKLF_DEBUG)
else()
  # Instruct Qt to ignore qDebug() for RELEASE build configuration (if not KLF_DEBUG)
  set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -DQT_NO_DEBUG_OUTPUT")
endif()
if(KLF_EXPERIMENTAL)
  add_definitions(-DKLF_EXPERIMENTAL)
endif()


# Main subdirectory with all sources
# ----------------------------------
add_subdirectory(src)


message(STATUS "")
message(STATUS "[ADDITIONAL SETTINGS]")


# Doxygen
# -------
include(klfdoxygen)



message(STATUS "")
message(STATUS "[DONE]")
KLFNotifyNotices()
message(STATUS "")


