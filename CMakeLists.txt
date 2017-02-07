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

   In most cases values are just 'ON' or 'OFF'. In some other cases they are
   paths. Non-absolute installation paths are understood as relative to
   CMAKE_INSTALL_PREFIX.

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

set(CMAKE_BUILD_TYPE "Release" CACHE STRING "Build configuration (Debug|Release|RelWithDebInfo)")

if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE "Release" CACHE STRING "Build configuration (Debug|Release|RelWithDebInfo)" FORCE)
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

# experimental features -- currently no experimental features to enable
#
#option(KLF_EXPERIMENTAL
#  "DEVELOPERS ONLY. Enable experimental unstable features (NOT recommended for regular use!)"
#  off)
#mark_as_advanced(KLF_EXPERIMENTAL)
#if(KLF_EXPERIMENTAL)
#  KLFNote("WARNING: Experimental features are unstable and are meant only for developers!! Use at your own risk!!")
#  message(STATUS "Enabling experimental features (KLF_EXPERIMENTAL)")
#endif(KLF_EXPERIMENTAL)
set(KLF_EXPERIMENTAL 0)


#
# Find Qt 5
#
find_package(Qt5Core REQUIRED)
find_package(Qt5Gui REQUIRED)
find_package(Qt5Widgets REQUIRED)
find_package(Qt5Xml REQUIRED)

get_target_property(Qt5Core_location Qt5::Core LOCATION)
get_target_property(Qt5Gui_location Qt5::Gui LOCATION)
get_target_property(Qt5Widgets_location Qt5::Widgets LOCATION)
get_target_property(Qt5Xml_location Qt5::Xml LOCATION)
message(STATUS "Found minimal Qt5 components (use CMAKE_PREFIX_PATH=/path/to/qt/lib/cmake):
      - ${Qt5Core_location}
      - ${Qt5Gui_location}
      - ${Qt5Widgets_location}
      - ${Qt5Xml_location}")


#
# Decide which window system to use. This is independent of OS, say, theoretically we
# could compile an X11 version on a Mac (though not sure why we'd do it).
#
# This setting KLF_WS actually defines which additional features are included.  You can
# use make a fully ws-portable build with KLF_WS=other (disable all extra features).
#
if (APPLE)
  set(klf_ws_dflt "mac")
elseif(UNIX)
  set(klf_ws_dflt "x11")
elseif(WIN32)
  set(klf_ws_dflt "win")
else()
  set(klf_ws_dflt "other")
endif()
set(KLF_WS "${klf_ws_dflt}" CACHE STRING
  "Which window system to use (mac,x11,win,other) -- used to enable some additional platform-specific features")
if(KLF_WS STREQUAL "mac")
  message(STATUS "Enabling mac-specific features (KLF_WS)")
elseif(KLF_WS STREQUAL "x11")
  message(STATUS "Enabling X11-specific features (KLF_WS)")
elseif(KLF_WS STREQUAL "win")
  message(STATUS "Enabling windows-specific features (KLF_WS)")
elseif(KLF_WS STREQUAL "other")
  message(STATUS "Not enabling any window system specific feature (KLF_WS)")
else()
  message(FATAL_ERROR "Invalid setting for KLF_WS=${KLF_WS}.  Please Select one of \"mac\", \"win\", \"x11\" or \"other\".")
endif()

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

if(KLF_LIBKLFTOOLS_STATIC)
  set(default_KLF_BUILD_TOOLSDESPLUGIN FALSE)
else()
  set(default_KLF_BUILD_TOOLSDESPLUGIN TRUE)
endif()
set(KLF_BUILD_TOOLSDESPLUGIN "${default_KLF_BUILD_TOOLSDESPLUGIN}" CACHE BOOL
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

# Extra search paths 
if(KLF_BUILD_BACKEND OR KLF_BUILD_BACKEND_AUTO)
  set(KLF_EXTRA_SEARCH_PATHS "" CACHE STRING
    "Extra paths in which klfbackend (and klatexformula) will search for latex/dvips/etc. executables. Separate paths with ';' on all platforms")

  message(STATUS "Will prepend \"${KLF_EXTRA_SEARCH_PATHS}\" to klfbackend/klatexformula's
    latex/dvips/gs search paths (KLF_EXTRA_SEARCH_PATHS)")
endif()


option(KLF_BUILD_GUI "Build klatexformula GUI application (klatexformula) [requires klftools and klfbackend]" 1)
if(KLF_BUILD_GUI)
  if (NOT KLF_BUILD_TOOLS OR NOT KLF_BUILD_BACKEND)
    message(FATAL_ERROR "Cannot build the klatexformula GUI main application without klftools and klfbackend (set KLF_BUILD_BACKEND=1 and KLF_BUILD_TOOLS=1, or set KLF_BUILD_GUI=0)")
  endif()
  message(STATUS "Will build the main klatexformula program (GUI) (KLF_BUILD_GUI)")
else()
  message(STATUS "Will not build the main klatexformula program (GUI) (KLF_BUILD_GUI)")
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
else()
  # no need to put these in cache, it's irrelevant
  #
  # However let's keep the KLF_MACOSX_BUNDLES setting independent from
  # APPLE.  There may be systems on which we can create bundles, that
  # are not APPLE ??? not sure, but keep the possibility open.
  KLFSetIfNotDefined(KLF_MACOSX_BUNDLES OFF)
  KLFSetIfNotDefined(KLF_MACOSX_BUNDLE_EXTRAS OFF)
endif()

if(KLF_MACOSX_BUNDLES)

  if (KLF_MACOSX_BUNDLE_EXTRAS)
    message(STATUS
      "Will package library and framework dependencies into klatexformula app bundle (KLF_MACOSX_BUNDLE_EXTRAS)")
  else()
    message(STATUS
      "Will NOT package library and framework dependencies into klatexformula app bundle (KLF_MACOSX_BUNDLE_EXTRAS)")
  endif()

  set(default_KLF_BUNDLE_QT_PLUGINS
    Qt5::QCocoaIntegrationPlugin
    Qt5::QGifPlugin
    Qt5::QDDSPlugin
    Qt5::QGifPlugin
    Qt5::QICNSPlugin
    Qt5::QICOPlugin
    Qt5::QJpegPlugin
    Qt5::QMacJp2Plugin
    #Qt5::QMinimalIntegrationPlugin
    #Qt5::QOffscreenIntegrationPlugin
    Qt5::QTgaPlugin
    Qt5::QTiffPlugin
    #Qt5::QTuioTouchPlugin
    #Qt5::QVirtualKeyboardPlugin
    Qt5::QWbmpPlugin
    Qt5::QWebpPlugin 
    Qt5::QSQLiteDriverPlugin
    Qt5::QSvgPlugin
    Qt5::QSvgIconPlugin
    )
  set(KLF_BUNDLE_QT_PLUGINS "${default_KLF_BUNDLE_QT_PLUGINS}"
    CACHE STRING "Qt plugin list (imported targets e.g. Qt5::QJpegPlugin) to package into app bundle")
  string(REGEX REPLACE ";" "
      " klf_display_bundle_qt_plugins "${KLF_BUNDLE_QT_PLUGINS}")
  message(STATUS "Will Package Qt Plugins in bundle (KLF_BUNDLE_QT_PLUGINS):
      ${klf_display_bundle_qt_plugins}")
  if(NOT "${KLF_BUNDLE_QT_PLUGINS}" STREQUAL "${default_KLF_BUNDLE_QT_PLUGINS}")
    KLFNote("You are bundling a list of qt plugins which differs from the default list. In case the default list
    has changed and want to restore the defaults, run \"cmake .. -UKLF_BUNDLE_QT_PLUGINS\".")
  endif()

endif(KLF_MACOSX_BUNDLES)
     

# Build DBUS support ?
if(UNIX AND NOT APPLE)
  set(klf_use_dbus_dflt 1)
else()
  set(klf_use_dbus_dflt 0)
endif()
option(KLF_USE_DBUS "Compiles D-Bus support into KLatexFormula GUI" ${klf_use_dbus_dflt})
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
if(KLF_DEBUG_USE_MODELTEST)
  message(STATUS "Will compile with QtLab's ModelTest for KLFLibModel testing (KLF_DEBUG_USE_MODELTEST)")
endif(KLF_DEBUG_USE_MODELTEST)

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

# Include some extra data files/fonts into KLatexFormula
if(KLF_NO_CMU_FONT)
  set(klf_def_incl_fonts "")
elseif(KLF_WS STREQUAL "mac")
  set(klf_def_incl_fonts
    "data/cmunbbx.otf;data/cmunbmo.otf;data/cmunbmr.otf;data/cmunbso.otf;data/cmunbsr.otf;data/cmunbxo.otf") # CMU Bright
else()
  set(klf_def_incl_fonts "data/cmunsi.otf;data/cmunso.otf;data/cmunss.otf;data/cmunsx.otf") # CMU Sans Serif
endif()
set(KLF_INCLUDE_FONTS "${klf_def_incl_fonts}"
  CACHE STRING "Include some extra fonts into KLF's internal resources (absolute or relative to src/)")
if(KLF_INCLUDE_FONTS)
  string(REPLACE ";" "\n        " klf_include_fonts_msg_list "${KLF_INCLUDE_FONTS}")
  message(STATUS "Will include fonts into klatexformula's resources (KLF_INCLUDE_FONTS):\n        ${klf_include_fonts_msg_list}")
else()
  message(STATUS "Will not include fonts into klatexformula's resources (KLF_INCLUDE_FONTS)")
endif()


# Use Sparkle framework under Mac OS X?
if(KLF_WS STREQUAL "mac")
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
endif()

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
# Add warning flags for all build configurations
set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -Wall -Wextra")
set(CMAKE_CXX_FLAGS_RELWITHDEBINFO "${CMAKE_CXX_FLAGS_RELWITHDEBINFO} -Wall -Wextra")
set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -Wall -Wextra")
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


