# CMake definitions for CPack
# ===========================
# $Id$


configure_file("${CMAKE_SOURCE_DIR}/cmake/welcome_installer.txt.in"
	       "${CMAKE_BINARY_DIR}/welcome_installer.txt"
	       @ONLY)
configure_file("${CMAKE_SOURCE_DIR}/cmake/welcome_installer_withlatex.txt.in"
	       "${CMAKE_BINARY_DIR}/welcome_installer_withlatex.txt"
	       @ONLY)

set(klf_package_name "klatexformula-${KLF_VERSION}-${KLF_CMAKE_OS}-${KLF_CMAKE_ARCH}")
if(APPLE OR WIN32)
  # ... win32-x86 is a bit redundant ...
  set(klf_package_name "klatexformula-${KLF_VERSION}-${KLF_CMAKE_OS}")
endif(APPLE OR WIN32)

if(KLF_INSTALL_LATEXDIST)
  set(klf_package_name "${klf_package_name}-with-latex")
endif(KLF_INSTALL_LATEXDIST)
if(KLF_INSTALL_LATEXDIST)
  set(klf_welcome_installer_fname "${CMAKE_BINARY_DIR}/welcome_installer.txt")
else(KLF_INSTALL_LATEXDIST)
  set(klf_welcome_installer_fname "${CMAKE_BINARY_DIR}/welcome_installer_withlatex.txt")
endif(KLF_INSTALL_LATEXDIST)
file(TO_NATIVE_PATH "${klf_welcome_installer_fname}" klf_welcome_installer_fnpath)

# == Binary packages ==

set(CPACK_PACKAGE_NAME klatexformula)
set(CPACK_VERSION_MAJOR ${KLF_VERSION_MAJ})
set(CPACK_VERSION_MINOR ${KLF_VERSION_MIN})
set(CPACK_VERSION_PATCH ${KLF_VERSION_REL})
set(CPACK_PACKAGE_DESCRIPTION_FILE "${CMAKE_SOURCE_DIR}/descr_long.txt")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "${CMAKE_SOURCE_DIR}/descr_short.txt")
set(CPACK_PACKAGE_FILE_NAME "${klf_package_name}")
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
set(CPACK_PACKAGE_VENDOR "Philippe Faist <philippe.faist@bluewin.ch>")
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
  set(CPACK_PACKAGE_INSTALL_REGISTRY_KEY "klatexformula-${KLF_VERSION}")
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
    "http://klatexformula.sourceforge.net/klfwiki/index.php/User_Manual:Home")
  set(CPACK_NSIS_URL_INFO_ABOUT "http://klatexformula.sourceforge.net/")
  set(CPACK_NSIS_CONTACT "philippe.faist@bluewin.ch")
  #set(CPACK_NSIS_CREATE_ICONS_EXTRA )
  #set(CPACK_NSIS_DELETE_ICONS_EXTRA )
endif(WIN32)


# == Mac OS X Installer ==
if(KLF_MACOSX_BUNDLES)
  set(CPACK_GENERATOR "DragNDrop")
  set(CPACK_OSX_PACKAGE_VERSION "10.5")
  # This is used in bundle identifier...
  set(CPACK_PACKAGE_VENDOR "klatexformula")
  set(CPACK_DMG_VOLUME_NAME "KLatexFormula-${KLF_VERSION}")
  set(CPACK_DMG_DS_STORE "${CMAKE_SOURCE_DIR}/cmake/DS_Store")
  set(CPACK_DMG_BACKGROUND_IMAGE "${CMAKE_SOURCE_DIR}/cmake/installer_dragndrop_bg.png")
endif(KLF_MACOSX_BUNDLES)

# == Advanced ==
#set(CPACK_CMAKE_GENERATOR )
set(CPACK_INSTALL_CMAKE_PROJECTS  "${CMAKE_BINARY_DIR};klatexformula;ALL;/")

#set(CPACK_SYSTEM_NAME )
set(CPACK_PACKAGE_VERSION "${KLF_VERSION}")
#set(CPACK_TOPLEVEL_TAG )
#set(CPACK_INSTALL_COMMANDS )
#set(CPACK_INSTALL_DIRECTORIES )






# Finally include the CPack module
# --------------------------------

option(KLF_USE_CPACK
       "Use CPack to create packages with 'make package'. Affects possibly CMAKE_INSTALL_PREFIX" OFF)

if(KLF_USE_CPACK)
  if(WIN32)
    if(NOT CMAKE_INSTALL_PREFIX STREQUAL "/.")
      set(CMAKE_INSTALL_PREFIX "/." CACHE PATH "CMake install prefix for CPack" FORCE)
      KLFNote("For windows/macosx CPack, CMAKE_INSTALL_PREFIX has to be '/.'. Corrected. Please re-run.")
      message(FATAL_ERROR "Bad CMAKE_INSTALL_PREFIX detected and corrected. Please re-run.")
    endif(NOT CMAKE_INSTALL_PREFIX STREQUAL "/.")
    if(NOT CMAKE_SKIP_RPATH)
      KLFNote("You have not set CMAKE_SKIP_RPATH. For windows installers, installed RPATHs are incorrect.")
    endif(NOT CMAKE_SKIP_RPATH)
  endif(WIN32)

  if(KLF_MACOSX_BUNDLES)
    #    KLFNote("CPack under Mac OS X is not (yet) (officially) supported.")
  endif(KLF_MACOSX_BUNDLES)
  
  KLFNote("You have configured to use CPack for generating packages.
    Run 'make package' to generate package(s).
    Note that (at least on windows), you should not run 'make install'
    without a DESTDIR=... environment variable set, since the
    CMAKE_INSTALL_PREFIX might have been altered in order to generate
    the package(s).")

  # include CPack stuff
  include(CPack)

else(KLF_USE_CPACK)
  
endif(KLF_USE_CPACK)

