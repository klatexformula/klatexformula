# CMake definitions for CPack
# ===========================
# $Id$



option(KLF_USE_CPACK
  "Use CPack to create packages with 'make package'. NOTE: Affects possibly CMAKE_INSTALL_PREFIX" OFF)

if(KLF_USE_CPACK)
  message(STATUS "Will create binary klatexformula installation package using CPACK (KLF_USE_CPACK)")
else()
  message(STATUS "Will not create binary klatexformula installation package (KLF_USE_CPACK).")
endif()

if(KLF_USE_CPACK)

  message(STATUS "")
  message(STATUS "[SETTINGS FOR BINARY PACKAGE WITH CPACK]")

  # Current System OS -- used for package name
  if(APPLE)
    set(internal_KLF_CMAKE_OS "macosx")
  elseif(WIN32)
    set(internal_KLF_CMAKE_OS "win32")
  elseif(UNIX)
    set(internal_KLF_CMAKE_OS "linux")
  else(APPLE)
    set(internal_KLF_CMAKE_OS "unknown")
  endif(APPLE)
  KLFSetIfNotDefined(KLF_CMAKE_OS "${internal_KLF_CMAKE_OS}")
  message(STATUS "Building package for OS \"${KLF_CMAKE_OS}\"")

  # Current System Arch -- used for package name
  if(APPLE AND CMAKE_OSX_ARCHITECTURES)
    set(internal_KLF_CMAKE_ARCHES "${CMAKE_OSX_ARCHITECTURES}")
    set(default_KLF_CMAKE_ARCH )
    foreach(arch ${internal_KLF_CMAKE_ARCHES})
      set(thisarch "${arch}")
      if(thisarch MATCHES "^i.86$")
        set(thisarch "x86")
      endif(thisarch MATCHES "^i.86$")
      set(default_KLF_CMAKE_ARCH "${default_KLF_CMAKE_ARCH},${thisarch}")
    endforeach(arch)
    # and remove initial comma
    string(REGEX REPLACE "^," "" default_KLF_CMAKE_ARCH "${default_KLF_CMAKE_ARCH}")

    KLFSetIfNotDefined(KLF_CMAKE_ARCH "${default_KLF_CMAKE_ARCH}")
    message(STATUS "Building universal Mac OS X package for architecture(s) \"${KLF_CMAKE_ARCH}\"")

  elseif(APPLE) # but not CMAKE_OSX_ARCHITECTURES

    # best guess these days is that it's a x86_64. Change manually otherwise.
    if(NOT DEFINED KLF_CMAKE_ARCH)
      set(default_KLF_CMAKE_ARCH "x86_64")
      set(KLF_CMAKE_ARCH "${default_KLF_CMAKE_ARCH}")
      message(STATUS "Guessing ${KLF_CMAKE_ARCH} processor architecture on Mac OS X (KLF_CMAKE_ARCH)")
    else()
      message(STATUS "Building Mac OS X package for architecture ${KLF_CMAKE_ARCH} (KLF_CMAKE_ARCH)")
    endif()

  else()

    # CMAKE_SYSTEM_PROCESSOR is the processor target architecture we're building for
    set(default_KLF_CMAKE_ARCH "${CMAKE_SYSTEM_PROCESSOR}") 
    if(CMAKE_SYSTEM_PROCESSOR MATCHES "^i.86$")
      set(default_KLF_CMAKE_ARCH "x86")
    endif()

    KLFSetIfNotDefined(KLF_CMAKE_ARCH "${default_KLF_CMAKE_ARCH}")
    message(STATUS "Building package for architecture ${KLF_CMAKE_ARCH} (KLF_CMAKE_ARCH)")

  endif()

  message(STATUS "If OS and/or architecture have not been detected correctly, you may set
   KLF_CMAKE_{OS|ARCH} to their correct values manually.\"")


  configure_file("${CMAKE_SOURCE_DIR}/cmake/welcome_installer.txt.in"
    "${CMAKE_BINARY_DIR}/welcome_installer.txt"
    @ONLY)

  # KLF "with bundled latex" are discontinued
  #
  #configure_file("${CMAKE_SOURCE_DIR}/cmake/welcome_installer_withlatex.txt.in"
  #	       "${CMAKE_BINARY_DIR}/welcome_installer_withlatex.txt"
  #	       @ONLY)

  set(default_KLF_PACKAGE_NAME "klatexformula-${KLF_VERSION}-${KLF_CMAKE_OS}-${KLF_CMAKE_ARCH}")
  if(APPLE OR WIN32)
    # nicer package names, because we will only provide a single arch on Mac and Windows
    set(default_KLF_PACKAGE_NAME "klatexformula-${KLF_VERSION}-${KLF_CMAKE_OS}")
  endif(APPLE OR WIN32)

  KLFSetIfNotDefined(KLF_PACKAGE_NAME "${default_KLF_PACKAGE_NAME}")
  message(STATUS "Binary package name: ${KLF_PACKAGE_NAME} (KLF_PACKAGE_NAME)")

  set(klf_welcome_installer_fname "${CMAKE_BINARY_DIR}/welcome_installer.txt")
  file(TO_NATIVE_PATH "${klf_welcome_installer_fname}" klf_welcome_installer_fnpath)

  # == Binary packages ==

  set(CPACK_PACKAGE_NAME klatexformula)
  set(CPACK_VERSION_MAJOR ${KLF_VERSION_MAJ})
  set(CPACK_VERSION_MINOR ${KLF_VERSION_MIN})
  set(CPACK_VERSION_PATCH ${KLF_VERSION_REL})
  set(CPACK_PACKAGE_DESCRIPTION_FILE "${CMAKE_SOURCE_DIR}/descr_long.txt")
  set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "${CMAKE_SOURCE_DIR}/descr_short.txt")
  set(CPACK_PACKAGE_FILE_NAME "${KLF_PACKAGE_NAME}")
  set(CPACK_PACKAGE_INSTALL_DIRECTORY "KLatexFormula-${KLF_VERSION}")
  set(CPACK_RESOURCE_FILE_LICENSE "${CMAKE_SOURCE_DIR}/COPYING.txt")
  unset(CPACK_RESOURCE_FILE_README)
  #set(CPACK_RESOURCE_FILE_README "${CMAKE_SOURCE_DIR}/cmake/installer_readme.txt")
  set(CPACK_RESOURCE_FILE_WELCOME "${klf_welcome_installer_fnpath}")
  set(CPACK_MONOLITHIC_INSTALL TRUE)
  #set(CPACK_GENERATOR )
  #set(CPACK_OUTPUT_CONFIG_FILE )
  set(CPACK_PACKAGE_EXECUTABLES "klatexformula;KLatexFormula")
  set(CPACK_STRIP_FILES TRUE)
  set(CPACK_PACKAGE_VENDOR "KLatexFormula Project")
  set(CPACK_SET_DESTDIR TRUE)  # use ENV{DESTDIR}=... instead of setting CMAKE_INSTALL_PREFIX
  set(CPACK_CREATE_DESKTOP_LINKS "klatexformula")

  # == Source packages ==

  set(CPACK_SOURCE_PACKAGE_FILE_NAME  "klatexformula-${KLF_VERSION}")
  #set(CPACK_SOURCE_STRIP_FILES )
  #set(CPACK_SOURCE_GENERATOR )
  set(CPACK_SOURCE_OUTPUT_CONFIG_FILE )
  set(CPACK_SOURCE_IGNORE_FILES "/\\\\.svn/")


  # == Windows Installer ==

  if(WIN32)
    set(CPACK_PACKAGE_INSTALL_REGISTRY_KEY "klatexformula")
    set(cmake_CPACK_NSIS_MUI_ICON "${CMAKE_SOURCE_DIR}/src/mswin/klficon64.ico")
    STRING(REPLACE "/" "\\\\" CPACK_NSIS_MUI_ICON "${cmake_CPACK_NSIS_MUI_ICON}")
    set(cmake_CPACK_PACKAGE_ICON "${CMAKE_SOURCE_DIR}/src/mswin/klatexformula-nsis-header.bmp")
    STRING(REPLACE "/" "\\\\" CPACK_PACKAGE_ICON "${cmake_CPACK_PACKAGE_ICON}")
    #set(CPACK_NSIS_EXTRA_INSTALL_COMMANDS )
    #set(CPACK_NSIS_EXTRA_UNINSTALL_COMMANDS )
    #set(CPACK_NSIS_COMPRESSOR )
    set(CPACK_NSIS_MODIFY_PATH "ON")
    set(CPACK_NSIS_DISPLAY_NAME "KLatexFormula ${KLF_VERSION}")
    set(CPACK_NSIS_INSTALLED_ICON_NAME "${KLF_INSTALL_BIN_DIR}\\\\klatexformula.exe")
    set(CPACK_NSIS_HELP_LINK
      "http://klatexformula.sourceforge.net/")
    set(CPACK_NSIS_URL_INFO_ABOUT "http://klatexformula.sourceforge.net/")
    set(CPACK_NSIS_CONTACT "philippe.faist@bluewin.ch")
    set(CPACK_NSIS_ENABLE_UNINSTALL_BEFORE_INSTALL on) # ask user to uninstall previous version before installing
    set(CPACK_NSIS_MUI_FINISHPAGE_RUN "klatexformula.exe")
    #set(CPACK_NSIS_CREATE_ICONS_EXTRA )
    #set(CPACK_NSIS_DELETE_ICONS_EXTRA )
  endif(WIN32)


  # == Mac OS X Installer ==
  if(APPLE AND KLF_MACOSX_BUNDLES)
    set(CPACK_GENERATOR "DragNDrop")
    if(CMAKE_OSX_DEPLOYMENT_TARGET)
      set(CPACK_OSX_PACKAGE_VERSION "${CMAKE_OSX_DEPLOYMENT_TARGET}")
    else()
      # get current Mac OS X version
      exec_program(/usr/bin/sw_vers OUTPUT_VARIABLE sw_vers_output)
      string(REGEX REPLACE "^(.|\n|\r)*ProductVersion: *" "" sw_vers_output1 "${sw_vers_output}")
      string(REGEX REPLACE "^([0-9]+)\\.([0-9]+)(.|\n|\r)*$" "\\1.\\2" this_osx_ver "${sw_vers_output1}")
      set(CPACK_OSX_PACKAGE_VERSION "${this_osx_ver}")
    endif()
    # This is used in bundle identifier...
    set(CPACK_PACKAGE_VENDOR "klatexformula")
    set(CPACK_DMG_VOLUME_NAME "KLatexFormula-${KLF_VERSION}")
    set(CPACK_DMG_DS_STORE "${CMAKE_SOURCE_DIR}/cmake/DS_Store")
    set(CPACK_DMG_BACKGROUND_IMAGE "${CMAKE_SOURCE_DIR}/cmake/installer_dragndrop_bg.png")
    # FIXME: how do we properly strip files on mac?
    SET(CPACK_STRIP_FILES FALSE)
  endif(APPLE AND KLF_MACOSX_BUNDLES)

  # == Advanced ==
  #set(CPACK_CMAKE_GENERATOR )
  set(CPACK_INSTALL_CMAKE_PROJECTS  "${CMAKE_BINARY_DIR};klatexformula;ALL;/")

  #set(CPACK_SYSTEM_NAME )
  set(CPACK_PACKAGE_VERSION "${KLF_VERSION}")
  #set(CPACK_TOPLEVEL_TAG )
  #set(CPACK_INSTALL_COMMANDS )
  #set(CPACK_INSTALL_DIRECTORIES )


  if(UNIX AND NOT APPLE)
    set(CPACK_GENERATOR "TGZ")
  endif(UNIX AND NOT APPLE)

  #
  # Add klatexformula.sh script in root of TGZ package for convenience
  #
  if(UNIX AND NOT KLF_MACOSX_BUNDLES AND KLF_USE_CPACK)
    configure_file(
      "${CMAKE_SOURCE_DIR}/src/klatexformula.sh.in"
      "${CMAKE_BINARY_DIR}/src/klatexformula.sh"
      @ONLY)
    install(PROGRAMS "${CMAKE_BINARY_DIR}/src/klatexformula.sh" DESTINATION "/")
  endif()

  

  set(ForceValueForCPack_set 0)
  macro(ForceValueForCPack var value)
    if(NOT ${var} STREQUAL "${value}")
      KLFNote("For package creation on ${KLF_CMAKE_OS}, ${var} must be set to \"${value}\". Corrected. (Value was \"${value}\")") 
      set(${var} "${value}" CACHE STRING "Value forced by CPack on ${KLF_CMAKE_OS}" FORCE)
      set(ForceValueForCPack_set 1)
    endif()
  endmacro()
  macro(ForceValueForCPackTrue var)
    if(NOT ${var})
      KLFNote("For package creation on ${KLF_CMAKE_OS}, ${var} must be set to TRUE. Corrected. (Value was \"${value}\")")
      set(${var} TRUE CACHE STRING "Value forced by CPack on ${KLF_CMAKE_OS}" FORCE)
      set(ForceValueForCPack_set 1)
    endif()
  endmacro()
  macro(ForceValueForCPackFalse var)
    if(${var})
      KLFNote("For package creation on ${KLF_CMAKE_OS}, ${var} must be set to FALSE. Corrected. (Value was \"${value}\")") 
      set(${var} FALSE CACHE STRING "Value forced by CPack on ${KLF_CMAKE_OS}" FORCE)
      set(ForceValueForCPack_set 1)
    endif()
  endmacro()

  if(WIN32)
    ForceValueForCPack(CMAKE_INSTALL_PREFIX "/.")
    ForceValueForCPack(KLF_INSTALL_SHARE_KLF_DATA_DIR ".")
    if(NOT CMAKE_SKIP_RPATH)
      KLFNote("You have not set CMAKE_SKIP_RPATH. For windows installers, installed RPATHs seem to be incorrect.")
    endif(NOT CMAKE_SKIP_RPATH)
  endif(WIN32)

  if(KLF_MACOSX_BUNDLES)
    ForceValueForCPack(CMAKE_INSTALL_PREFIX "/.")
    ForceValueForCPack(KLF_INSTALL_BIN_DIR "/.")
    ForceValueForCPackFalse(KLF_INSTALL_KLFTOOLS_HEADERS)
    ForceValueForCPackFalse(KLF_INSTALL_KLFTOOLS_SO_LIBS)
    ForceValueForCPackFalse(KLF_INSTALL_KLFTOOLS_STATIC_LIBS)
    ForceValueForCPackFalse(KLF_INSTALL_KLFTOOLS_FRAMEWORK)
    ForceValueForCPackFalse(KLF_INSTALL_KLFBACKEND_HEADERS)
    ForceValueForCPackFalse(KLF_INSTALL_KLFBACKEND_SO_LIBS)
    ForceValueForCPackFalse(KLF_INSTALL_KLFBACKEND_STATIC_LIBS)
    ForceValueForCPackFalse(KLF_INSTALL_KLFBACKEND_FRAMEWORK)
    ForceValueForCPackFalse(KLF_INSTALL_KLFBACKEND_AUTO_HEADERS)
    ForceValueForCPackFalse(KLF_INSTALL_KLFBACKEND_AUTO_SO_LIBS)
    ForceValueForCPackFalse(KLF_INSTALL_KLFBACKEND_AUTO_STATIC_LIBS)
    ForceValueForCPackFalse(KLF_INSTALL_KLFBACKEND_AUTO_FRAMEWORK)
  endif()

  if(ForceValueForCPack_set)
    message(FATAL_ERROR "Some values have been corrected for CPack. Please re-run CMake to continue.")
  endif()


  KLFNote("You have configured to use CPack for generating packages.  Run 'make
    package' to generate package(s).  Note that (at least on windows and Mac OS
    X), you should not run 'make install' without a DESTDIR=... environment
    variable set, since the CMAKE_INSTALL_PREFIX might have been altered in
    order to generate the package(s).")

  # include CPack stuff
  include(CPack)

endif(KLF_USE_CPACK)

