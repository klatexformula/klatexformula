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

set(KLF_ROOT_CMAKE_READ TRUE CACHE INTERNAL "Can be queried to see if the root CMakeLists.txt was read")

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


# Warn the user for defined cache variables that are not standard,
# i.e. may be a user's typo
include(klfValidOptions)



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


# experimental features
option(KLF_EXPERIMENTAL
  "DEVELOPERS ONLY. Enable experimental unstable features (NOT recommended for regular use!)"
  off)
mark_as_advanced(KLF_EXPERIMENTAL)
if(KLF_EXPERIMENTAL)
  KLFNote("WARNING: Experimental features are unstable and are meant only for developers!! Use at your risk!!")
  message(STATUS "Enabling experimental features (KLF_EXPERIMENTAL)")
endif(KLF_EXPERIMENTAL)


# see if user specified a QT_QMAKE_EXECUTABLE and set QT_QMAKE_EXECUTABLE_FINDQT too (FindQt4 bug?)
if(QT_QMAKE_EXECUTABLE AND NOT QT_QMAKE_EXECUTABLE_FINDQT)
  set(QT_QMAKE_EXECUTABLE_FINDQT "${QT_QMAKE_EXECUTABLE}" CACHE STRING "Qt QMake executable path" FORCE)
endif(QT_QMAKE_EXECUTABLE AND NOT QT_QMAKE_EXECUTABLE_FINDQT)

# Find Qt 4 and Include it
find_package(Qt4 4.5.0 COMPONENTS QtCore QtGui REQUIRED)

if(NOT QT4_FOUND)
  mark_as_advanced(CLEAR QT_QMAKE_EXECUTABLE)
  message(FATAL_ERROR
    "Didn't find Qt4. Try setting the QT_QMAKE_EXECUTABLE (and/or QT_QMAKE_EXECUTABLE_FINDQT) cache variable(s)." )
endif(NOT QT4_FOUND)

# Current Qt Version "MAJ.MIN"
set(KLF_CMAKE_QTVERSION "${QT_VERSION_MAJOR}.${QT_VERSION_MINOR}" CACHE INTERNAL "Qt Version, Maj.Min")


# variables that FindQt obviously forgets to set to advanced
mark_as_advanced(QT_QTMOTIF_INCLUDE_DIR QT_QTMOTIF_LIBRARY_DEBUG QT_QTMOTIF_LIBRARY_RELEASE)
if(WIN32)
  mark_as_advanced(CLEAR QT_QMAKE_EXECUTABLE_FINDQT)
endif(WIN32)


#KLFDeclareCacheVarOptionCondition(specificoption cachetype cachestring updatenoticestring condition forcedvalue defaultvalue)
KLFDeclareCacheVarOptionCondition(KLF_BUILD_TOOLS
	BOOL "Build klatexformula tools library (klftools)"          # cache info
	"Not building klftools library... should never happen."  # update notify message
	1              # the condition (1=no condition)
	OFF            # the forced value if condition is not met
	1              # the default value at beginning
)
if(KLF_BUILD_TOOLS)
  message(STATUS "Will build the klftools library (KLF_BUILD_TOOLS)")
else(KLF_BUILD_TOOLS)
  message(FATAL_ERROR "There is nothing to build. You must set at least KLF_BUILD_TOOLS.")
endif(KLF_BUILD_TOOLS)

set(KLF_BUILD_TOOLSDESPLUGIN TRUE CACHE BOOL "Build Qt Designer Plugin for klftools library widgets")
mark_as_advanced(KLF_BUILD_TOOLSDESPLUGIN)
# ...
KLFTestCondition(_klf_allow_toolsdesplugin
  "KLF_BUILD_TOOLS AND NOT KLF_LIBKLFBACKEND_STATIC AND NOT KLF_LIBKLFTOOLS_STATIC")
KLFDeclareCacheVarOptionCondition(KLF_BUILD_TOOLSDESPLUGIN
	BOOL "Build klatexformula tools designer plug-in; requires building dynamic libraries."          # cache info
	"Build klatexformula tools designer plug-in requires building dynamic libraries.
    Not building the klftools designer plug-in."  # update notify message
	_klf_allow_toolsdesplugin  # the condition
	OFF                        # the forced value if condition is not met
	${KLF_BUILD_TOOLS}         # the default value at beginning
)
mark_as_advanced(KLF_BUILD_TOOLSDESPLUGIN) # advanced...
if(KLF_BUILD_TOOLSDESPLUGIN)
  message(STATUS "Will build the klftools designer plugin library (KLF_BUILD_TOOLSDESPLUGIN)")
else(KLF_BUILD_TOOLSDESPLUGIN)
  message(STATUS "Will not build the klftools designer plugin library (KLF_BUILD_TOOLSDESPLUGIN)")
endif(KLF_BUILD_TOOLSDESPLUGIN)


#KLFDeclareCacheVarOptionCondition(specificoption cachetype cachestring updatenoticestring condition forcedvalue defaultvalue)
KLFDeclareCacheVarOptionCondition(KLF_BUILD_BACKEND
	BOOL "Build klatexformula backend library (klfbackend)"                  # cache info
	"Building klfbackend library requires klftools. Not building klfbackend library."  # update notify message
	KLF_BUILD_TOOLS       # the condition
	OFF                   # the forced value if condition is not met
	TRUE                  # the default value at beginning
)
if(KLF_BUILD_BACKEND)
  message(STATUS "Will build the klfbackend library (KLF_BUILD_BACKEND)")
else(KLF_BUILD_BACKEND)
  message(STATUS "Will not build the klfbackend library (KLF_BUILD_BACKEND)")
endif(KLF_BUILD_BACKEND)


#KLFDeclareCacheVarOptionCondition(spec.option cachetype cachestr updatemsg cond. forceval def.val.)
KLFDeclareCacheVarOptionCondition(KLF_BUILD_GUI
	BOOL "Build klatexformula GUI (klfapp+klatexformula), requires klftools and klfbackend." # cache info
	"Building klatexformula GUI requires klftools and klfbackend libraries. Not building GUI." # notify message
	"KLF_BUILD_BACKEND AND KLF_BUILD_TOOLS"  # the condition
	OFF                                      # the forced value if condition is not met
	TRUE                                     # the default value at beginning (here this is a boolean)
)
if(KLF_BUILD_GUI)
  message(STATUS "Will build the GUI interface (klatexformula, klfapp) (KLF_BUILD_GUI)")
else(KLF_BUILD_GUI)
  message(STATUS "Will not build the GUI interface (klatexformula, klfapp) (KLF_BUILD_GUI)")
endif(KLF_BUILD_GUI)


#KLFDeclareCacheVarOptionCondition(spec.option cachetype cachestr updatemsg cond. forceval def.val.)
KLFDeclareCacheVarOptionCondition(KLF_BUILD_PLUGINS
	BOOL "Build klatexformula plugins; requires klftools, klfbackend and GUI" # cache info
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


# Static/Shared libraries
# In all cases we prefer shared libraries (also on linux, so we can build the
# klfdesplugin and klfkateplugin objects). Set static by default only in debug
# configurations.
if(WIN32)
  set(default_library_static FALSE)
else(WIN32)
  # unux systems.
  KLFTestCondition(default_library_static "CMAKE_BUILD_TYPE STREQUAL \"Debug\"")
endif(WIN32)

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

# Build a shared or static klfbackend library
option(KLF_LIBKLFBACKEND_STATIC "Compile static libklfbackend backend library instead of shared."
       ${default_library_static})
# status message
if(KLF_LIBKLFBACKEND_STATIC)
  message(STATUS "Building a static KLFBackend library (KLF_LIBKLFBACKEND_STATIC)")
else(KLF_LIBKLFBACKEND_STATIC)
  message(STATUS "Building a shared KLFBackend library (KLF_LIBKLFBACKEND_STATIC)")
endif(KLF_LIBKLFBACKEND_STATIC)

# Build or not a shared or static klfbackend_auto library
option(KLF_BUILD_BACKEND_AUTO "Compile autonomous libklfbackend_auto backend library without libklftools dependency."
       ON)
option(KLF_LIBKLFBACKEND_AUTO_STATIC "Compile static libklfbackend_auto backend library instead of shared."
       ${default_library_static})
# status message
if(KLF_BUILD_BACKEND_AUTO)
  if(KLF_LIBKLFBACKEND_AUTO_STATIC)
    message(STATUS "Building a static klfbackend_auto autonomous library (KLF_LIBKLFBACKEND_AUTO_STATIC)")
  else(KLF_LIBKLFBACKEND_AUTO_STATIC)
    message(STATUS "Building a shared klfbackend_auto autonomous library (KLF_LIBKLFBACKEND_AUTO_STATIC)")
  endif(KLF_LIBKLFBACKEND_AUTO_STATIC)
else(KLF_BUILD_BACKEND_AUTO)
  message(STATUS "Not building libklfbackend_auto autonomous library (KLF_BUILD_BACKEND_AUTO)")
endif(KLF_BUILD_BACKEND_AUTO)

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

if(APPLE)
  # These settings override on Mac OS X
  KLFGetCMakeVarChanged(CMAKE_OSX_ARCHITECTURES)
  if(CMAKE_OSX_ARCHITECTURES)
    set(display_CMAKE_OSX_ARCHITECTURES "${CMAKE_OSX_ARCHITECTURES}")
  else(CMAKE_OSX_ARCHITECTURES)
    set(display_CMAKE_OSX_ARCHITECTURES "(default)")
  endif(CMAKE_OSX_ARCHITECTURES)
  message(STATUS "Compiling for Mac OS X Architectures: ${display_CMAKE_OSX_ARCHITECTURES} (CMAKE_OSX_ARCHITECTURES)")
  if(KLF_TARGET_ARCH_64 OR klf_changed_KLF_TARGET_ARCH_64)
    KLFNote("Warning: KLF_TARGET_ARCH_64 ignored on Mac OS X. Use CMAKE_OSX_ARCHITECTURES instead.")
  endif(KLF_TARGET_ARCH_64 OR klf_changed_KLF_TARGET_ARCH_64)
else(APPLE)
  # Target architecture (allows compile for 32-bit on 64-bit architectures)
  KLFGetCMakeVarChanged(KLF_TARGET_ARCH_64)  # detect changes, for KLF_LIB_SUFFIX later on
  if(CMAKE_SIZEOF_VOID_P EQUAL 8)
    set(klf_host_arch_64 TRUE)
  else(CMAKE_SIZEOF_VOID_P EQUAL 8)
    set(klf_host_arch_64 FALSE)
  endif(CMAKE_SIZEOF_VOID_P EQUAL 8)
  set(KLF_TARGET_ARCH_64 ${klf_host_arch_64} CACHE BOOL "Configure build for 64-bit arch")
  KLFCMakeSetVarChanged(KLF_TARGET_ARCH_64)
  if(KLF_TARGET_ARCH_64)
    message(STATUS "Compiling for 64-bit architecture (KLF_TARGET_ARCH_64=ON/OFF)")
  else(KLF_TARGET_ARCH_64)
    message(STATUS "Compiling for 32-bit architecture (KLF_TARGET_ARCH_64=ON/OFF)")
  endif(KLF_TARGET_ARCH_64)

  KLFGetCMakeVarChanged(KLF_ARCH_FLAGS)
  set(the_arch_flags "")
  if(KLF_TARGET_ARCH_64)
    if(klf_host_arch_64)
      set(the_arch_flags "")
    else(klf_host_arch_64)
      set(the_arch_flags "-m64")
    endif(klf_host_arch_64)
  else(KLF_TARGET_ARCH_64)
    if(klf_host_arch_64)
      set(the_arch_flags "-m32")
    else(klf_host_arch_64)
      set(the_arch_flags "")
    endif(klf_host_arch_64)
  endif(KLF_TARGET_ARCH_64)
  #KLFDeclareCacheVarOptionFollowComplexN(specificoption cachetype cachestring updatenotice calcoptvalue depvarlist)
  KLFDeclareCacheVarOptionFollowComplexN(KLF_ARCH_FLAGS
    STRING "Additional architecture flags for gcc"
    TRUE                     # updatenotice
    "${the_arch_flags}"      # calcoptvalue
    "KLF_TARGET_ARCH_64"     # depvarlist
    )
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${KLF_ARCH_FLAGS}")
endif(APPLE)
if(KLF_ARCH_FLAGS)
  message(STATUS "Additional architecture flags for gcc: ${KLF_ARCH_FLAGS} (KLF_ARCH_FLAGS)")
endif(KLF_ARCH_FLAGS)


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
set(klf_def_incl_fonts "data/cmunsi.otf;data/cmunso.otf;data/cmunss.otf;data/cmunsx.otf") # CMU Sans Serif
if(APPLE)
  set(klf_def_incl_fonts
    "data/cmunbbx.otf;data/cmunbmo.otf;data/cmunbmr.otf;data/cmunbso.otf;data/cmunbsr.otf;data/cmunbxo.otf") # CMU Bright
endif(APPLE)
set(KLF_INCLUDE_FONTS "${klf_def_incl_fonts}"
  CACHE STRING "Include some extra fonts into KLF's internal resources (absolute or relative to src/)")
message(STATUS "Will include fonts into klatexformula's resources: \"${KLF_INCLUDE_FONTS}\" (KLF_INCLUDE_FONTS)")
mark_as_advanced(KLF_INCLUDE_FONTS)
KLFCMakeSetVarChanged(KLF_INCLUDE_FONTS)

# Use system font rather than CMU font if available
#KLFDeclareCacheVarOptionFollowComplexN(specificoption cachetype cachestring updatenotice calcoptvalue depvarlist)
KLFDeclareCacheVarOptionFollowComplexN(KLF_NO_CMU_FONT
  BOOL "Tells klatexformula to use system font instead of Computer Modern font"
  TRUE   # updatenotice
  OFF    # calcoptvalue
  ""     # depvarlist
  )
if(KLF_NO_CMU_FONT)
  message(STATUS "Will use system font instead of CMU font (KLF_NO_CMU_FONT)")
else(KLF_NO_CMU_FONT)
  message(STATUS "Will attempt to use Computer Modern unicode font (KLF_NO_CMU_FONT)")
endif(KLF_NO_CMU_FONT)

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
if(APPLE AND CMAKE_OSX_ARCHITECTURES)
  set(internal_KLF_CMAKE_ARCHES "${CMAKE_OSX_ARCHITECTURES}")
  set(internal_KLF_CMake_ARCH )
  foreach(arch ${internal_KLF_CMAKE_ARCHES})
    set(thisarch "${arch}")
    if(thisarch MATCHES "^i.86$")
      set(thisarch "x86")
    endif(thisarch MATCHES "^i.86$")
    set(internal_KLF_CMAKE_ARCH "${internal_KLF_CMAKE_ARCH},${thisarch}")
  endforeach(arch)
  # and remove initial comma
  string(REGEX REPLACE "^," "" internal_KLF_CMAKE_ARCH "${internal_KLF_CMAKE_ARCH}")
    
  #KLFDeclareCacheVarOptionFollowComplexN(specificoption cachetype cachestring updatenotice calcoptvalue depvarlist)
  KLFDeclareCacheVarOptionFollowComplexN(KLF_CMAKE_ARCH
    STRING "Target build processor architecture (Mac OS X universal: separated by commas)"
    FALSE                             # updatenotice
    "${internal_KLF_CMAKE_ARCH}"      # calcoptvalue
    "CMAKE_OSX_ARCHITECTURES"         # depvarlist
    )
else(APPLE AND CMAKE_OSX_ARCHITECTURES)
  if (APPLE)
    # best guess these days is that it's a x86_64. Change manually otherwise.
    KLFNote("Guessing x86_64 processor architecture on Mac OS X. set CMAKE_OSX_ARCHITECTURES to change.")
    set(internal_KLF_CMAKE_ARCH "x86_64")
  else(APPLE)
    # CMAKE_SYSTEM_PROCESSOR is the processor we're building _for_.
    set(internal_KLF_CMAKE_ARCH "${CMAKE_SYSTEM_PROCESSOR}") 
    if(CMAKE_SYSTEM_PROCESSOR MATCHES "^i.86$")
      set(internal_KLF_CMAKE_ARCH "x86")
    endif(CMAKE_SYSTEM_PROCESSOR MATCHES "^i.86$")
    if(NOT APPLE AND NOT KLF_TARGET_ARCH_64 AND CMAKE_SYSTEM_PROCESSOR EQUAL "x86_64")
      set(internal_KLF_CMAKE_ARCH "x86") # we are building for x86
    endif(NOT APPLE AND NOT KLF_TARGET_ARCH_64 AND CMAKE_SYSTEM_PROCESSOR EQUAL "x86_64")
  endif(APPLE)
  set(KLF_CMAKE_ARCH "${internal_KLF_CMAKE_ARCH}"
    CACHE STRING "Build Target Processor Arch (Mac OS X universal: separated by commas)")
endif(APPLE AND CMAKE_OSX_ARCHITECTURES)
mark_as_advanced(KLF_CMAKE_ARCH)

# Find KDE4 for KTextEditor Plugin
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


# some additional variables that get shown in some versions of CMake that could be hidden
mark_as_advanced(
	EXECUTABLE_OUTPUT_PATH
	LIBRARY_OUTPUT_PATH
	CMAKE_BACKWARDS_COMPATIBILITY
	LIB_SUFFIX
	)

# Manpage generation from --help and --version help text
if(NOT KLF_MACOSX_BUNDLES)
  if(NOT DEFINED HELP2MAN OR HELP2MAN STREQUAL "")
    find_program(HELP2MAN "help2man")
  endif(NOT DEFINED HELP2MAN OR HELP2MAN STREQUAL "")
endif(NOT KLF_MACOSX_BUNDLES)
if(HELP2MAN)
  message(STATUS "Manpage will be generated from --help text with help2man (HELP2MAN=OFF or /path/to/help2man)")
elseif(NOT KLF_MACOSX_BUNDLES AND NOT WIN32 AND HELP2MAN STREQUAL "")
  KLFNote("help2man was not found. Manpage will not be generated.
    set explicitely HELP2MAN to /path/.../to/help2man if needed.")
endif(HELP2MAN)
if(NOT HELP2MAN)
  message(STATUS "Manpage will not be generated with help2man (HELP2MAN=OFF or /path/to/help2man)")
endif(NOT HELP2MAN)
if(NOT DEFINED GZIP OR GZIP STREQUAL "")
  find_program(GZIP "gzip")
endif(NOT DEFINED GZIP OR GZIP STREQUAL "")
if(HELP2MAN)
  if(GZIP)
    message(STATUS "Manpage will be gzip'ed (GZIP=OFF or /path/to/gzip)")
  else(GZIP)
    message(STATUS "Manpage will not be gzip'ed (GZIP=OFF or /path/to/gzip)")
  endif(GZIP)
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
	sqldrivers/libqsqlite.dylib
	sqldrivers/libqsqlodbc.dylib)
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
		-DKLF_SRC_BUILD
		-DKLF_CMAKE_ARCH="${KLF_CMAKE_ARCH}")
# Add flags for DEBUG build configuration
set(CMAKE_CXX_FLAGS_DEBUG
  "${CMAKE_CXX_FLAGS_DEBUG} -Wall -Wextra -Wformat=2 -Wno-format-extra-args -Winit-self")
if(KLF_DEBUG)
  add_definitions(-DKLF_DEBUG)
else(KLF_DEBUG)
  # Instruct Qt to ignore qDebug() for RELEASE build configuration (if not KLF_DEBUG)
  set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -DQT_NO_DEBUG_OUTPUT")
endif(KLF_DEBUG)
if(KLF_EXPERIMENTAL)
  add_definitions(-DKLF_EXPERIMENTAL)
endif(KLF_EXPERIMENTAL)


# Main subdirectory with all sources
# ----------------------------------
add_subdirectory(src)

# Build KDE KTextEditor Plugin
# This can't be called from src/CMakeLists.txt because the ktexteditor plugin can
# be build with just klfbackend and no gui.
if(KLF_BUILD_KTEXTEDITORPLUGIN)
  add_subdirectory(src/klfkateplugin)
endif(KLF_BUILD_KTEXTEDITORPLUGIN)



message(STATUS "")
message(STATUS "[ADDITIONAL SETTINGS]")


# Doxygen
# -------
include(klfdoxygen)



message(STATUS "")
message(STATUS "[DONE]")
KLFNotifyNotices()
message(STATUS "")


