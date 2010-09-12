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
set(KLF_LIB_VERSION "${KLF_VERSION_MAJ}.${KLF_VERSION_MIN}")

include(KLFUtil)

# Set some project settings
# -------------------------

message(STATUS "")
message(STATUS "[BUILD SETTINGS]")

KLFGetCMakeVarChanged(CMAKE_BUILD_TYPE)
set(CMAKE_BUILD_TYPE "Release" CACHE STRING "(FOR DEVELOPERS:) Build configuration (Debug|Release)")
mark_as_advanced(CLEAR CMAKE_BUILD_TYPE)
if(NOT CMAKE_BUILD_TYPE AND klf_first_CMAKE_BUILD_TYPE)
  # force to "Release" the first time if empty
  set(CMAKE_BUILD_TYPE "Release"
		      CACHE STRING "(FOR DEVELOPERS:) Build configuration (Debug|Release)" FORCE)
endif(NOT CMAKE_BUILD_TYPE AND klf_first_CMAKE_BUILD_TYPE)
message(STATUS "Build Type: ${CMAKE_BUILD_TYPE} (CMAKE_BUILD_TYPE)")
KLFCMakeSetVarChanged(CMAKE_BUILD_TYPE)

if(CMAKE_BUILD_TYPE STREQUAL "Debug")
  set(default_klf_debug TRUE)
else(CMAKE_BUILD_TYPE STREQUAL "Debug")
  set(default_klf_debug FALSE)
endif(CMAKE_BUILD_TYPE STREQUAL "Debug")
#KLFDeclareCacheVarOptionFollowComplex1(specificoption cachetype cachestring updatenotice calcoptvalue depvar1)
KLFDeclareCacheVarOptionFollowComplex1(KLF_DEBUG
	BOOL "Enable debugging messages"  # cache
	ON                                # updatenotice enabled
	"${default_klf_debug}"            # (possibly new) calculated value
	CMAKE_BUILD_TYPE                  # dependency variable
)
if(KLF_DEBUG)
  message(STATUS "Enabling debugging messages (KLF_DEBUG)")
endif(KLF_DEBUG)
if(WIN32)
  set(klf_default_debug_postfix "d")
elseif(APPLE)
  set(klf_default_debug_postfix "_debug")
else(WIN32)
  set(klf_default_debug_postfix "")  
endif(WIN32)
set(CMAKE_DEBUG_POSTFIX "${klf_default_debug_postfix}"
	    CACHE STRING "Suffix to add to target names when built in 'Debug' build configuration")
mark_as_advanced(CMAKE_DEBUG_POSTFIX)

# Choose a Qt Version to use
set(KLF_QT_VERSION 4 CACHE STRING "Qt major version to build with (4 or 3)")
KLFGetCMakeVarChanged(KLF_QT_VERSION)
if(NOT KLF_QT_VERSION STREQUAL "3" AND NOT KLF_QT_VERSION STREQUAL "4")
  KLFNote("KLF_QT_VERSION has to be \"3\" (for Qt 3) or \"4\" (for Qt 4) (major version number)")
  message(FATAL_ERROR "Invalid value for KLF_QT_VERSION")
endif(NOT KLF_QT_VERSION STREQUAL "3" AND NOT KLF_QT_VERSION STREQUAL "4")
if(klf_changed_KLF_QT_VERSION)
  # the KLF_QT_VERSION variable has changed value
  #  -> Reset some variables to let FindQt() find the right qmake and other executables (eg. moc)
  unset(QT_INCLUDE_DIR CACHE)
  unset(QT_INCLUDE_DIR)
  unset(QT_LIBRARIES CACHE)
  unset(QT_LIBRARIES)
  unset(QT_DEFINITIONS CACHE)
  unset(QT_DEFINITIONS)
  unset(QT_FOUND CACHE)
  unset(QT_FOUND)
  unset(QT_MOC_EXECUTABLE CACHE)
  unset(QT_MOC_EXECUTABLE)
  unset(QT_UIC_EXECUTABLE CACHE)
  unset(QT_UIC_EXECUTABLE)
  unset(QT_QT_LIBRARY CACHE)
  unset(QT_QT_LIBRARY)
  unset(QT_QMAKE_EXECUTABLE CACHE)
  unset(QT_QMAKE_EXECUTABLE)
  unset(QT3_QGLOBAL_H_FILE CACHE)
  unset(QT3_QGLOBAL_H_FILE)
endif(klf_changed_KLF_QT_VERSION)
KLFMarkVarAdvancedIf(QT_QMAKE_EXECUTABLE "NOT KLF_QT_VERSION EQUAL 4")
# and set KLF_USE_QT4 (internal to us, mirror of KLF_QT_VERSION==4)
if(KLF_QT_VERSION EQUAL 3)
  set(KLF_USE_QT4 FALSE CACHE INTERNAL "TRUE if klatexformula is compiled with Qt4 (reflects KLF_QT_VERSION)")
else(KLF_QT_VERSION EQUAL 3)
  set(KLF_USE_QT4 TRUE CACHE INTERNAL "TRUE if klatexformula is compiled with Qt4 (reflects KLF_QT_VERSION)")
endif(KLF_QT_VERSION EQUAL 3)

# Include that version of Qt
set(QT_REQUIRED TRUE CACHE INTERNAL "Qt required.")
set(DESIRED_QT_VERSION "${KLF_QT_VERSION}" CACHE INTERNAL "FindQt's Qt Version to use")
set(QT4_INSTALLED)
set(QT3_INSTALLED)
mark_as_advanced(QT_REQUIRED DESIRED_QT_VERSION QT3_INSTALLED QT4_INSTALLED)
KLFCMakeDebug("QT3_QGLOBAL_H_FILE is ${QT3_QGLOBAL_H_FILE}")
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
message(STATUS "Using Qt ${KLF_QT_VERSION} (KLF_QT_VERSION=\"${KLF_QT_VERSION}\")")

# variables that FindQt obviously forgets to set to advanced
mark_as_advanced(QT_QTMOTIF_INCLUDE_DIR QT_QTMOTIF_LIBRARY_DEBUG QT_QTMOTIF_LIBRARY_RELEASE)
if(WIN32)
  mark_as_advanced(CLEAR QT_QMAKE_EXECUTABLE_FINDQT)
endif(WIN32)



#KLFDeclareCacheVarOptionCondition(specificoption cachetype cachestring updatenoticestring condition forcedvalue defaultvalue)
KLFDeclareCacheVarOptionCondition(KLF_BUILD_TOOLS
	BOOL "Build klatexformula tools library (klftools), requires Qt4"          # cache info
	"Building klftools library requires Qt 4. Not building klftools library."  # update notify message
	KLF_USE_QT4           # the condition
	OFF                   # the forced value if condition is not met
	${KLF_USE_QT4}        # the default value at beginning
)
if(KLF_BUILD_TOOLS)
  message(STATUS "Will build the klftools library (KLF_BUILD_TOOLS)")
else(KLF_BUILD_TOOLS)
  message(STATUS "Will not build the klftools library (KLF_BUILD_TOOLS)")
endif(KLF_BUILD_TOOLS)


#KLFDeclareCacheVarOptionCondition(spec.option cachetype cachestr updatemsg cond. forceval def.val.)
KLFDeclareCacheVarOptionCondition(KLF_BUILD_GUI
	BOOL "Build klatexformula GUI (klfapp+klatexformula), requires Qt4 and klftools" # cache info
	"Building klatexformula GUI requires Qt 4 and klftools libraries. Not building GUI." # notify message
	"KLF_USE_QT4 AND KLF_BUILD_TOOLS"  # the condition
	OFF                                # the forced value if condition is not met
	${KLF_BUILD_TOOLS}                 # the default value at beginning (here this is a boolean)
)
if(KLF_BUILD_GUI)
  message(STATUS "Will build the GUI interface (klatexformula, klfapp) (KLF_BUILD_GUI)")
else(KLF_BUILD_GUI)
  message(STATUS "Will not build the GUI interface (klatexformula, klfapp) (KLF_BUILD_GUI)")
endif(KLF_BUILD_GUI)


#KLFDeclareCacheVarOptionCondition(spec.option cachetype cachestr updatemsg cond. forceval def.val.)
KLFDeclareCacheVarOptionCondition(KLF_BUILD_PLUGINS
	BOOL "Build klatexformula plugins; requires Qt4, klftools and GUI" # cache info
	"Building klatexformula plugins requires klatexformula GUI. Not building plugins." # notify message
	"KLF_BUILD_GUI"                    # the condition (GUI depends on klftools and Qt4)
	OFF                                # the forced value if condition is not met
	${KLF_BUILD_GUI}                   # the default value at beginning (here this is a boolean)
)
if(KLF_BUILD_PLUGINS)
  message(STATUS "Will build klatexformula plugins (KLF_BUILD_PLUGINS)")
else(KLF_BUILD_PLUGINS)
  message(STATUS "Will not build klatexformula plugins (KLF_BUILD_PLUGINS)")
endif(KLF_BUILD_PLUGINS)


# Static/Shared libklfbackend
set(default_library_static FALSE)
if(WIN32 OR APPLE)
  set(default_library_static FALSE)
else(WIN32 OR APPLE)
  # linux systems.
  # Note that plugins can locate symbols in the binary and will compile with undefined
  # references, so it's ok
  set(default_library_static TRUE)
endif(WIN32 OR APPLE)
option(KLF_LIBKLFBACKEND_STATIC "Compile static libklfbackend backend library instead of shared."
       ${default_library_static})
# status message
if(KLF_LIBKLFBACKEND_STATIC)
  message(STATUS "Building a static KLFBackend library (KLF_LIBKLFBACKEND_STATIC)")
else(KLF_LIBKLFBACKEND_STATIC)
  message(STATUS "Building a shared KLFBackend library (KLF_LIBKLFBACKEND_STATIC)")
endif(KLF_LIBKLFBACKEND_STATIC)

# Build a shared or static klftools library
set(KLF_LIBKLFTOOLS_STATIC ${default_library_static} CACHE BOOL "Build a static klftools library")
# and status message
if(KLF_BUILD_TOOLS)
  if(KLF_LIBKLFTOOLS_STATIC)
    message(STATUS "Building a static KLatexFormula tools library (klftools) (KLF_LIBKLFTOOLS_STATIC)")
  else(KLF_LIBKLFTOOLS_STATIC)
    message(STATUS "Building a shared KLatexFormula tools library (klftools) (KLF_LIBKLFTOOLS_STATIC)")
  endif(KLF_LIBKLFTOOLS_STATIC)
endif(KLF_BUILD_TOOLS)

# Build a shared or static source klfapp library (for plugins to link to)
set(KLF_LIBKLFAPP_STATIC ${default_library_static} CACHE BOOL "Build a static klfapp library")
# and status message
if(KLF_BUILD_GUI)
  if(KLF_LIBKLFAPP_STATIC)
    message(STATUS "Building a static KLatexFormula klfapp library (KLF_LIBKLFAPP_STATIC)")
  else(KLF_LIBKLFAPP_STATIC)
    message(STATUS "Building a shared KLatexFormula klfapp library (KLF_LIBKLFAPP_STATIC)")
  endif(KLF_LIBKLFAPP_STATIC)
endif(KLF_BUILD_GUI)

# Target architecture (allows compile for 32-bit on 64-bit architectures)
KLFGetCMakeVarChanged(KLF_TARGET_ARCH_64)  # detect changes, for KLF_LIB_SUFFIX later on
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
KLFCMakeSetVarChanged(KLF_TARGET_ARCH_64)

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
  if(klf_first_KLF_TARGET_ARCH_64 OR klf_changed_KLF_TARGET_ARCH_64 OR klf_updated_KLF_TARGET_ARCH_64)
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
      set(klf_updated_KLF_LIB_SUFFIX TRUE) # we updated this variable's value
      if(klf_lib_suffix_redefined)
        KLFNote("The library suffix was detected and adjusted to \"${KLF_LIB_SUFFIX}\".")
      endif(klf_lib_suffix_redefined)
    endif(KLF_LIB_SUFFIX STREQUAL NEW_KLF_LIB_SUFFIX)
  else(klf_first_KLF_TARGET_ARCH_64 OR klf_changed_KLF_TARGET_ARCH_64 OR klf_updated_KLF_TARGET_ARCH_64)
    # no change
  endif(klf_first_KLF_TARGET_ARCH_64 OR klf_changed_KLF_TARGET_ARCH_64 OR klf_updated_KLF_TARGET_ARCH_64)
  message(STATUS "Using \"${KLF_LIB_SUFFIX}\" as library directory suffix (KLF_LIB_SUFFIX)")
endif(UNIX)
KLFCMakeSetVarChanged(KLF_LIB_SUFFIX)


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
 "DEVELOPERS ONLY. Uses ModelTest (Qt's Labs) to test QAbstractItemModel-based models. VERY slow."
 FALSE)
mark_as_advanced(KLF_DEVEL_LOCAL_BASEPLUGINS_COPY KLF_DEBUG_USE_MODELTEST)
if(KLF_DEVEL_LOCAL_BASEPLUGINS_COPY)
  message(STATUS "Will install klfbaseplugins.rcc in ~/.klatexformula/rccresources at compile-time (KLF_DEVEL_LOCAL_BASEPLUGINS_COPY)")
endif(KLF_DEVEL_LOCAL_BASEPLUGINS_COPY)
if(KLF_DEBUG_USE_MODELTEST)
  message(STATUS "Will compile with QtLab's ModelTest for KLFLibModel testing (KLF_DEBUG_USE_MODELTEST)")
endif(KLF_DEBUG_USE_MODELTEST)

# Include some extra data files/fonts into KLatexFormula
KLFGetCMakeVarChanged(KLF_INCLUDE_FONTS)
set(KLF_INCLUDE_FONTS "data/cmunsi.otf;data/cmunso.otf;data/cmunss.otf;data/cmunsx.otf"
  CACHE STRING "Include some extra fonts into KLF's internal resources (absolute or relative to src/)")
message(STATUS "Will include fonts into klatexformula's resources: \"${KLF_INCLUDE_FONTS}\" (KLF_INCLUDE_FONTS)")
mark_as_advanced(KLF_INCLUDE_FONTS)
KLFCMakeSetVarChanged(KLF_INCLUDE_FONTS)

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
  if(UNIX AND NOT APPLE)
    set(KLF_BUILD_KTEXTEDITORPLUGIN ON CACHE BOOL "Build the KTextEditor plugin")
  else(UNIX AND NOT APPLE)
    set(KLF_BUILD_KTEXTEDITORPLUGIN OFF CACHE BOOL "Build the KTextEditor Plugin")
  endif(UNIX AND NOT APPLE)
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


# some additional variables that get shown on some versions of CMake that could be hidden
mark_as_advanced(
	EXECUTABLE_OUTPUT_PATH
	LIBRARY_OUTPUT_PATH
	CMAKE_BACKWARDS_COMPATIBILITY
	LIB_SUFFIX
	)


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
	sqldrivers/libqsqlite.dylib
	sqldrivers/libqsqlodbc.dylib)
  set(KLF_BUNDLE_QT_PLUGINS "${default_KLF_BUNDLE_QT_PLUGINS}"
					  CACHE STRING "Qt plugins to incorporate into Mac bundle")
  message("Will Package Qt Plugins in bundle --> ${KLF_BUNDLE_QT_PLUGINS} (KLF_BUNDLE_QT_PLUGINS)")
endif(KLF_MACOSX_BUNDLES)


# CPack
# -----
include(klfcpack)



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


