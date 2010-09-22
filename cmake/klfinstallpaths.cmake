# CMake Install Instructions
# ==========================
# $Id$


KLFGetCMakeVarChanged(CMAKE_INSTALL_PREFIX)
KLFGetCMakeVarChanged(KLF_INSTALL_RUN_POST_INSTALL)


# Installation destination
# ------------------------

if(CMAKE_INSTALL_PREFIX_INITIALIZED_TO_DEFAULT)
  set(CMAKE_INSTALL_PREFIX "/usr" CACHE PATH "The installation prefix for KLatexFormula." FORCE)
endif(CMAKE_INSTALL_PREFIX_INITIALIZED_TO_DEFAULT)
KLFCMakeSetVarChanged(CMAKE_INSTALL_PREFIX)
mark_as_advanced(CLEAR CMAKE_INSTALL_PREFIX)
message(STATUS "'make install' will install to \"${CMAKE_INSTALL_PREFIX}\" (CMAKE_INSTALL_PREFIX)")


# Installation Paths
# ------------------

# Lib Dir
set(install_lib_dir "lib${KLF_LIB_SUFFIX}")
if(WIN32)
  # install all in binary directory on MS Windows
  set(install_lib_dir "bin")
endif(WIN32)
if(APPLE AND KLF_MACOSX_BUNDLES)
  # install to /Library/Frameworks on mac OS X
  set(install_lib_dir "/Library/Frameworks")
endif(APPLE AND KLF_MACOSX_BUNDLES)

#KLFDeclareCacheVarOptionFollowComplex1(specificoption cachetype cachestring updatenotice
#                                       calcoptvalue depvar1)
KLFDeclareCacheVarOptionFollowComplex1(KLF_INSTALL_LIB_DIR
	STRING "Library installation directory (relative to install prefix, or absolute)"
	ON                               # updatenotice
	"${install_lib_dir}"             # calculated value
	KLF_LIB_SUFFIX                   # dependance variables
)
message(STATUS "Installing libraries to ${KLF_INSTALL_LIB_DIR} (KLF_INSTALL_LIB_DIR)")
if(IS_ABSOLUTE "${KLF_INSTALL_LIB_DIR}"	)
  KLFNote("You have chosen an absolute KLF_INSTALL_LIB_DIR path. There is
    nothing wrong with that, but keep in mind that this value will
    NOT be updated if you change CMAKE_INSTALL_PREFIX.")
endif(IS_ABSOLUTE "${KLF_INSTALL_LIB_DIR}")

# Utility variable. Same as KLF_INSTALL_LIB_DIR, but garanteed to be absolute path.
# Not kept in cache, it is (trivially) computed here
KLFMakeAbsInstallPath(KLF_ABS_INSTALL_LIB_DIR  KLF_INSTALL_LIB_DIR)
KLFCMakeDebug("KLF_ABS_INSTALL_LIB_DIR is ${KLF_ABS_INSTALL_LIB_DIR}; lib-dir is ${KLF_INSTALL_LIB_DIR}")

# Bin Dir
set(install_bin_dir "bin")
if(WIN32)
  # install binary in "bin/" also on Ms Windows
  set(install_bin_dir "bin")
endif(WIN32)
if(APPLE AND KLF_MACOSX_BUNDLES)
  # install to /Applications on mac OS X
  set(install_bin_dir "/Applications")
endif(APPLE AND KLF_MACOSX_BUNDLES)

#KLFDeclareCacheVarOptionFollowComplex1(specificoption cachetype cachestring updatenotice calcoptvalue depvar1)
KLFDeclareCacheVarOptionFollowComplex1(KLF_INSTALL_BIN_DIR
	STRING "Binaries installation directory (relative to install prefix, or absolute)" # cache info
	ON                      # updatenotice
	"${install_bin_dir}"    # calculated value
	DUMMY_DEPENDANCE_VARIABLE # dependance variable (as of now none!)
)
message(STATUS "Installing binary to ${KLF_INSTALL_BIN_DIR} (KLF_INSTALL_BIN_DIR)")
if(IS_ABSOLUTE "${KLF_INSTALL_BIN_DIR}")
  KLFNote("You have chosen an absolute KLF_INSTALL_BIN_DIR path. There is
    nothing wrong with that, but keep in mind that this value will
    NOT be updated if you change CMAKE_INSTALL_PREFIX.")
endif(IS_ABSOLUTE "${KLF_INSTALL_BIN_DIR}")

# Utility variable. Same as KLF_INSTALL_BIN_DIR, but garanteed to be absolute path.
# Not kept in cache, it is (trivially) computed here
KLFMakeAbsInstallPath(KLF_ABS_INSTALL_BIN_DIR KLF_INSTALL_BIN_DIR)


# rccresources dir
if(WIN32)

  set(KLF_INSTALL_RCCRESOURCES_DIR "rccresources/" CACHE STRING
			    "Where to install rccresources files (see also KLF_INSTALL_PLUGINS)")
  mark_as_advanced(KLF_INSTALL_RCCRESOURCES_DIR)
  if(IS_ABSOLUTE "${KLF_INSTALL_RCCRESOURCES_DIR}")
    KLFNote("You have chosen an absolute KLF_INSTALL_RCCRESOURCES_DIR. There is
    nothing wrong with that, but keep in mind that this value will
    NOT be updated if you change CMAKE_INSTALL_PREFIX.")
  endif(IS_ABSOLUTE "${KLF_INSTALL_RCCRESOURCES_DIR}")

else(WIN32)
  set(KLF_INSTALL_RCCRESOURCES_DIR "share/klatexformula/rccresources/" CACHE STRING
			      "Where to install rccresources files (see also KLF_INSTALL_PLUGINS)")
endif(WIN32)
KLFMakeAbsInstallPath(KLF_ABS_INSTALL_RCCRESOURCES_DIR  KLF_INSTALL_RCCRESOURCES_DIR)


# Installed RPATH
# ---------------

# option skip RPATH completely
set(CMAKE_SKIP_RPATH FALSE CACHE BOOL "Don't set RPATH on executables and libraries")
mark_as_advanced(CLEAR CMAKE_SKIP_RPATH)
# option to specify RPATH, only if not skipping
if(NOT CMAKE_SKIP_RPATH)
  
  #KLFDeclareCacheVarOptionFollowComplex2(specificoption cachetype cachestring updatenotice calcoptvalue depvar1 depvar2)
  KLFDeclareCacheVarOptionFollowComplex2(CMAKE_INSTALL_RPATH
	PATH "RPATH for installed libraries and executables"
	ON
	"${KLF_ABS_INSTALL_LIB_DIR}"
	CMAKE_INSTALL_PREFIX
	KLF_INSTALL_LIB_DIR
  )
  mark_as_advanced(CLEAR CMAKE_INSTALL_RPATH)
  message(STATUS "RPATH for installed libraries and executables: ${CMAKE_INSTALL_RPATH} (CMAKE_SKIP_RPATH,CMAKE_INSTALL_RPATH)")

else(NOT CMAKE_SKIP_RPATH)

  message(STATUS "Skipping RPATH on executables and libraries (CMAKE_SKIP_RPATH,CMAKE_INSTALL_RPATH)")

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
KLFDeclareCacheVarOptionFollow(KLF_INSTALL_KLFBACKEND_HEADERS     KLF_INSTALL_DEVEL   "Install klfbackend headers")
KLFDeclareCacheVarOptionFollow(KLF_INSTALL_KLFBACKEND_STATIC_LIBS KLF_INSTALL_DEVEL   "Install klfbackend static libraries")
KLFDeclareCacheVarOptionFollow(KLF_INSTALL_KLFBACKEND_SO_LIBS     KLF_INSTALL_RUNTIME "Install klfbackend so libraries")
KLFDeclareCacheVarOptionFollow(KLF_INSTALL_KLFBACKEND_FRAMEWORK   KLF_INSTALL_RUNTIME "Install klfbackend framework (Mac OS X)")
mark_as_advanced( KLF_INSTALL_KLFBACKEND_HEADERS
		  KLF_INSTALL_KLFBACKEND_STATIC_LIBS
		  KLF_INSTALL_KLFBACKEND_SO_LIBS
		  KLF_INSTALL_KLFBACKEND_FRAMEWORK
)

if(KLF_BUILD_TOOLS)
  KLFDeclareCacheVarOptionFollow(KLF_INSTALL_KLFTOOLS_HEADERS      KLF_INSTALL_DEVEL   "Install klftools headers")
  KLFDeclareCacheVarOptionFollow(KLF_INSTALL_KLFTOOLS_STATIC_LIBS  KLF_INSTALL_DEVEL   "Install klftools static libraries")
  KLFDeclareCacheVarOptionFollow(KLF_INSTALL_KLFTOOLS_SO_LIBS      KLF_INSTALL_RUNTIME "Install klftools so libraries")
  KLFDeclareCacheVarOptionFollow(KLF_INSTALL_KLFTOOLS_FRAMEWORK    KLF_INSTALL_RUNTIME "Install klftools framework (Mac OS X)")
  mark_as_advanced( KLF_INSTALL_KLFTOOLS_HEADERS
		    KLF_INSTALL_KLFTOOLS_STATIC_LIBS
		    KLF_INSTALL_KLFTOOLS_SO_LIBS
		    KLF_INSTALL_KLFTOOLS_FRAMEWORK
  )
endif(KLF_BUILD_TOOLS)

if(KLF_BUILD_GUI)
  KLFDeclareCacheVarOptionFollow(KLF_INSTALL_KLFAPP_HEADERS      KLF_INSTALL_DEVEL   "Install klfapp headers")
  KLFDeclareCacheVarOptionFollow(KLF_INSTALL_KLFAPP_STATIC_LIBS  KLF_INSTALL_DEVEL   "Install klfapp static libraries")
  KLFDeclareCacheVarOptionFollow(KLF_INSTALL_KLFAPP_SO_LIBS      KLF_INSTALL_RUNTIME "Install klfapp so libraries")
  KLFDeclareCacheVarOptionFollow(KLF_INSTALL_KLFAPP_FRAMEWORK    KLF_INSTALL_RUNTIME "Install klfapp framework (Mac OS X)")
  mark_as_advanced( KLF_INSTALL_KLFAPP_HEADERS
		    KLF_INSTALL_KLFAPP_STATIC_LIBS
		    KLF_INSTALL_KLFAPP_SO_LIBS
		    KLF_INSTALL_KLFAPP_FRAMEWORK
  )

  KLFDeclareCacheVarOptionFollow(KLF_INSTALL_KLATEXFORMULA_BIN   KLF_INSTALL_RUNTIME "Install klatexformula binary")
  KLFDeclareCacheVarOptionFollow(KLF_INSTALL_KLATEXFORMULA_CMDL  KLF_INSTALL_RUNTIME "Install klatexformula_cmdl symlink")
  KLFDeclareCacheVarOptionFollow(KLF_INSTALL_KLATEXFORMULA_BUNDLE KLF_INSTALL_RUNTIME "Install klatexformula bundle (Mac OS X)")
  mark_as_advanced( KLF_INSTALL_KLATEXFORMULA_BIN
		    KLF_INSTALL_KLATEXFORMULA_CMDL
		    KLF_INSTALL_KLATEXFORMULA_BUNDLE
  )
endif(KLF_BUILD_GUI)

if(NOT KLF_MACOSX_BUNDLES)
  KLFDeclareCacheVarOptionFollow(KLF_INSTALL_PLUGINS    KLF_INSTALL_RUNTIME   "Install klatexformula plugins")
else(NOT KLF_MACOSX_BUNDLES)
  if(KLF_INSTALL_PLUGINS)
    KLFNote("Not Installing plugins outside bundle. On mac, klatexformula looks for plugins by expecting to reside in a bundle.")
    set(KLF_INSTALL_PLUGINS  OFF  CACHE BOOL "Not needed. plugins are incorporated into bundle instead." FORCE)
  endif(KLF_INSTALL_PLUGINS)
endif(NOT KLF_MACOSX_BUNDLES)

message(STATUS "Will install targets:

               \theaders\t\tstatic,\t    shared libraries   \tframework
   klfbackend: \t  ${KLF_INSTALL_KLFBACKEND_HEADERS}\t\t  ${KLF_INSTALL_KLFBACKEND_STATIC_LIBS}\t\t  ${KLF_INSTALL_KLFBACKEND_SO_LIBS}\t\t  ${KLF_INSTALL_KLFBACKEND_FRAMEWORK}
   klftools:   \t  ${KLF_INSTALL_KLFTOOLS_HEADERS}\t\t  ${KLF_INSTALL_KLFTOOLS_STATIC_LIBS}\t\t  ${KLF_INSTALL_KLFTOOLS_SO_LIBS}\t\t  ${KLF_INSTALL_KLFTOOLS_FRAMEWORK}
   klfapp:     \t  ${KLF_INSTALL_KLFAPP_HEADERS}\t\t  ${KLF_INSTALL_KLFAPP_STATIC_LIBS}\t\t  ${KLF_INSTALL_KLFAPP_SO_LIBS}\t\t  ${KLF_INSTALL_KLFAPP_FRAMEWORK}

   klatexformula: \t${KLF_INSTALL_KLATEXFORMULA_BIN}
   klatexformula_cmdl: \t${KLF_INSTALL_KLATEXFORMULA_CMDL}
   klatexformula bundle: \t${KLF_INSTALL_KLATEXFORMULA_BUNDLE}

   individual installs can be fine-tuned with
          KLF_INSTALL_KLF{BACKEND|TOOLS|APP}_{HEADERS|SO_LIBS|STATIC_LIBS|FRAMEWORK}
   and    KLF_INSTALL_KLATEXFORMULA_{BIN|CMDL|BUNDLE}

   Irrelevant settings, eg. installing os X bundles on linux, or KLF{TOOLS|APP} library install
   settings without the corresponding KLF_BUILD_KLF{TOOLS|GUI}, are ignored.

   Set: - KLF_INSTALL_RUNTIME to install binaries, frameworks and shared libraries in general;
        - KLF_INSTALL_DEVEL to install static libraries and headers in general;
   KLF_INSTALL_{RUNTIME|DEVEL} are overridden by individual specific settings.
\n")


# Install .desktop & pixmaps in DEST/share/{applications|pixmaps} ?
if(KLF_MACOSX_BUNDLES OR WIN32 OR NOT KLF_BUILD_GUI)
  set(default_KLF_INSTALL_DESKTOP FALSE)
else(KLF_MACOSX_BUNDLES OR WIN32 OR NOT KLF_BUILD_GUI)
  set(default_KLF_INSTALL_DESKTOP TRUE)
endif(KLF_MACOSX_BUNDLES OR WIN32 OR NOT KLF_BUILD_GUI)

#KLFDeclareCacheVarOptionFollowComplexN(specificoption cachetype cachestring updatenotice calcoptvalue depvarlist)
KLFDeclareCacheVarOptionFollowComplexN(KLF_INSTALL_DESKTOP
  BOOL "Install a .desktop file and pixmap in DESTINTATION/share/{applications|pixmaps}"
  TRUE   # update notice
  ${default_KLF_INSTALL_DESKTOP}
  "KLF_BUILD_GUI"
  )

set(KLF_INSTALL_DESKTOP_CATEGORIES "Qt;Office;" CACHE STRING
  "Categories section in .desktop file")
set(KLF_INSTALL_SHARE_APPLICATIONS_DIR "share/applications/" CACHE STRING
  "(if KLF_INSTALL_DESKTOP) Where application .desktop links should be installed (relative to install prefix or absolute).")
set(KLF_INSTALL_SHARE_PIXMAPS_DIR "share/pixmaps/" CACHE STRING
  "(if KLF_INSTALL_DESKTOP) Where to place an application icon for klatexformula (default share/pixmaps/)")
set(KLF_INSTALL_ICON_THEME "" CACHE STRING
  "(if KLF_INSTALL_DESKTOP) Icons are to be installed in this desktop icon theme, eg. /usr/share/icons/hicolor")
set(KLF_INSTALL_SHARE_MIME_PACKAGES_DIR "share/mime/packages/" CACHE STRING
  "(if KLF_INSTALL_DESKTOP) Where to install mime database xml file(s) (default share/mime/packages)")
set(KLF_INSTALL_SHARE_MAN1_DIR "share/man/man1" CACHE STRING
  "(if KLF_INSTALL_DESKTOP) Where to install manual page (if generated) (default share/man/man1)")
KLFMakeAbsInstallPath(KLF_ABS_INSTALL_SHARE_APPLICATIONS_DIR KLF_INSTALL_SHARE_APPLICATIONS_DIR)
KLFMakeAbsInstallPath(KLF_ABS_INSTALL_SHARE_PIXMAPS_DIR KLF_INSTALL_SHARE_PIXMAPS_DIR)
KLFMakeAbsInstallPath(KLF_ABS_INSTALL_SHARE_MIME_PACKAGES_DIR KLF_INSTALL_SHARE_MIME_PACKAGES_DIR)
KLFMakeAbsInstallPath(KLF_ABS_INSTALL_SHARE_MAN1_DIR KLF_INSTALL_SHARE_MAN1_DIR)
# Reasonable Icon= entry given the installation settings
if(KLF_INSTALL_SHARE_PIXMAPS_DIR)
  set(klf_icon "${KLF_ABS_INSTALL_SHARE_PIXMAPS_DIR}/klatexformula-64.png")
else(KLF_INSTALL_SHARE_PIXMAPS_DIR)
  set(klf_icon "klatexformula")
endif(KLF_INSTALL_SHARE_PIXMAPS_DIR)
#KLFDeclareCacheVarOptionFollowComplexN(specificoption cachetype cachestring updatenotice calcoptvalue depvarlist)
KLFDeclareCacheVarOptionFollowComplexN(KLF_INSTALL_DESKTOP_ICON STRING "The Icon= entry of the .desktop file"
  ON # updatenotice
  "${klf_icon}" # calculated value
  "KLF_INSTALL_SHARE_PIXMAPS_DIR;CMAKE_INSTALL_PREFIX" # dependency variables
  )
mark_as_advanced(
  KLF_INSTALL_DESKTOP_CATEGORIES
  KLF_INSTALL_DESKTOP_ICON
  KLF_INSTALL_SHARE_APPLICATIONS_DIR
  KLF_INSTALL_SHARE_PIXMAPS_DIR
  KLF_INSTALL_SHARE_MIME_PACKAGES_DIR
  KLF_INSTALL_SHARE_MAN1_DIR
  KLF_INSTALL_ICON_THEME
  )

if(KLF_INSTALL_DESKTOP)
  message(STATUS "Will install linux desktop files (KLF_INSTALL_DESKTOP):
   .desktop categories:  \t${KLF_INSTALL_DESKTOP_CATEGORIES}  (KLF_INSTALL_DESKTOP_CATEGORIES)
   .desktop Icon= entry: \t${KLF_INSTALL_DESKTOP_ICON}  (KLF_INSTALL_DESKTOP_ICON)
   app .desktop files:   \t${KLF_INSTALL_SHARE_APPLICATIONS_DIR}  (KLF_INSTALL_SHARE_APPLICATIONS_DIR)
   mime database xml:    \t${KLF_INSTALL_SHARE_MIME_PACKAGES_DIR}  (KLF_INSTALL_SHARE_MIME_PACKAGES_DIR)
   pixmaps:              \t${KLF_INSTALL_SHARE_PIXMAPS_DIR}  (KLF_INSTALL_SHARE_PIXMAPS_DIR)
   icon theme:           \t${KLF_INSTALL_ICON_THEME}  (KLF_INSTALL_ICON_THEME)
   man page:             \t${KLF_INSTALL_SHARE_MAN1_DIR} (KLF_INSTALL_SHARE_MAN1_DIR)
     All paths are absolute, or relative to CMAKE_INSTALL_PREFIX. Setting the corresponding variable
     to an empty value disables that particular install.
")
else(KLF_INSTALL_DESKTOP)
  message(STATUS "Will not install linux desktop files (KLF_INSTALL_DESKTOP)")
endif(KLF_INSTALL_DESKTOP)

if(WIN32)
  option(KLF_INSTALL_QTLIBS "Copy Qt Libs next to installed executable" ON)

  if(KLF_INSTALL_QTLIBS)

    message(STATUS "Will install Qt libs next to installed executable (KLF_INSTALL_QTLIBS)")

    set(KLF_INSTALL_QTPLUGINS_DIR "qt-plugins/" CACHE STRING
	"Where to install Qt Plugins to deploy with application (relative to prefix, or absolute)")
    if(NOT KLF_INSTALL_QTPLUGINS_LIST)
      file(GLOB_RECURSE qtplugins_list RELATIVE "${QT_INSTALL_PREFIX}/plugins"
							      "${QT_INSTALL_PREFIX}/plugins/*.dll")
    endif(NOT KLF_INSTALL_QTPLUGINS_LIST)
    set(KLF_INSTALL_QTPLUGINS_LIST "${qtplugins_list}" CACHE STRING
		"List of Qt plugins, relative to ${QT_INSTALL_PREFIX}/plugins, to deploy with exe")
    message(STATUS "Will install given Qt plugins next to installed executable in ${KLF_INSTALL_PLUGINS_LIST} (KLF_INSTALL_QTPLUGINS_DIR,KLF_INSTALL_QTPLUGINS_LIST)")
  else(KLF_INSTALL_QTLIBS)

    message(STATUS "Will NOT install Qt libs next to installed executable (KLF_INSTALL_QTLIBS)")

  endif(KLF_INSTALL_QTLIBS)

endif(WIN32)


# Run Post-Install Scripts?
# -------------------------
# eg. for packaging, post-install scripts should be given in the RPM/deb/... definition
# and not be done at RPM creation time
option(KLF_INSTALL_RUN_POST_INSTALL
		    "Run post-install scripts after 'make install' (eg. update mime database)" YES)
#KLFGetCMakeVarChanged(KLF_INSTALL_RUN_POST_INSTALL) is called at top of this file
KLFCMakeSetVarChanged(KLF_INSTALL_RUN_POST_INSTALL)
set(klf_default_install_post_updatemimedatabase OFF)
if(KLF_INSTALL_RUN_POST_INSTALL AND KLF_INSTALL_DESKTOP AND KLF_INSTALL_SHARE_MIME_PACKAGES_DIR)
  set(klf_default_install_post_updatemimedatabase ON)
endif(KLF_INSTALL_RUN_POST_INSTALL AND KLF_INSTALL_DESKTOP AND KLF_INSTALL_SHARE_MIME_PACKAGES_DIR)
KLFCMakeDebug("klf_default_install_post_updatemimedatabase is ${klf_default_install_post_updatemimedatabase}")
#KLFDeclareCacheVarOptionFollowComplexN(specificoption cachetype cachestring updatenotice calcoptvalue depvarlist)
KLFDeclareCacheVarOptionFollowComplexN(KLF_INSTALL_POST_UPDATEMIMEDATABASE
  BOOL "Update the mime database after installing package mime-database xml files"
  TRUE
  "${klf_default_install_post_updatemimedatabase}"
  "KLF_INSTALL_RUN_POST_INSTALL;KLF_INSTALL_DESKTOP;KLF_INSTALL_SHARE_MIME_PACKAGES_DIR"
  )
mark_as_advanced(KLF_INSTALL_POST_UPDATEMIMEDATABASE)

message(STATUS "Will run the following post-install scripts (master switch KLF_INSTALL_RUN_POST_INSTALL):
 Update the mime database: \t${KLF_INSTALL_POST_UPDATEMIMEDATABASE} \t(KLF_INSTALL_POST_UPDATEMIMEDATABASE)
")


# see klfdoxygen.cmake for installation instructions of doxygen api doc

# ---------------------------------------------------


macro(KLFInstallLibrary targetlib varOptBase inst_lib_dir inst_pubheader_dir)

  # this dummy installation directory cannot be set to an absolute path (actually, it could..., but
  # it's a headache)  because on windows installing to absolute paths with drive letters is EVIL
  # when using DESTDIR=... (eg. for packaging). Instead, install to a dummy directory specified as
  # a relative path in final installation directory, then remove that dummy directory.
  set(klf_dummy_inst_dir "__klf_dummy_install_directory__")

  KLFConditionalSet(inst_${targetlib}_runtime_dir ${varOptBase}SO_LIBS
	"${inst_lib_dir}"  "${klf_dummy_inst_dir}")
  KLFConditionalSet(inst_${targetlib}_library_dir ${varOptBase}SO_LIBS
	"${inst_lib_dir}"  "${klf_dummy_inst_dir}")
  KLFConditionalSet(inst_${targetlib}_archive_dir ${varOptBase}STATIC_LIBS
	"${inst_lib_dir}"  "${klf_dummy_inst_dir}")
  KLFConditionalSet(inst_${targetlib}_framework_dir ${varOptBase}FRAMEWORK
	"${inst_lib_dir}"  "${klf_dummy_inst_dir}")
  KLFConditionalSet(inst_${targetlib}_pubheader_dir ${varOptBase}HEADERS
	"${inst_pubheader_dir}"  "${klf_dummy_inst_dir}")

  set(need_dummy_dir FALSE)
  if(NOT ${varOptBase}SO_LIBS OR NOT ${varOptBase}SO_LIBS OR NOT ${varOptBase}STATIC_LIBS
     OR NOT ${varOptBase}FRAMEWORK OR NOT ${varOptBase}HEADERS)
    set(need_dummy_dir TRUE)
  endif()

  install(TARGETS ${targetlib}
	RUNTIME DESTINATION "${inst_${targetlib}_runtime_dir}"
	LIBRARY DESTINATION "${inst_${targetlib}_library_dir}"
	ARCHIVE DESTINATION "${inst_${targetlib}_archive_dir}"
	FRAMEWORK DESTINATION "${inst_${targetlib}_framework_dir}"
	PUBLIC_HEADER DESTINATION "${inst_${targetlib}_pubheader_dir}"
  )

  if(need_dummy_dir)
    install(CODE "
    set(dummyinstdir 
	\"\$ENV{DESTDIR}\${CMAKE_INSTALL_PREFIX}/${klf_dummy_inst_dir}\")
    message(STATUS \"Removing dummy install directory '\${dummyinstdir}'\")
    execute_process(COMMAND \"${CMAKE_COMMAND}\" -E remove_directory \"\${dummyinstdir}\")
    "
    )
  endif(need_dummy_dir)

endmacro(KLFInstallLibrary)


# Fool-proof check to avoid installing into root directory
#  * Root directory is detected if prefix is empty, '/', '/.' or '/./' and ENV{DESTDIR} not set
#  * Note: if prefix contains a drive letter, then it was set manually, so it's probably on purpose.
#    another check will make sure that you don't install into DESTDIR/X:/... below.
install(CODE "
# --- Fool-proof: forbid to install to root / ---
if(\"\$ENV{DESTDIR}\" STREQUAL \"\" AND
   NOT \"\$ENV{KLF_CONFIRM_INSTALL_TO_ROOT_DIR}\" STREQUAL \"YES\")

  if(CMAKE_INSTALL_PREFIX STREQUAL \"\" OR CMAKE_INSTALL_PREFIX MATCHES \"^/(\\\\./?)?$\")
    message(\"
    *** ERROR ***
    
    Installation into root directory '\${CMAKE_INSTALL_PREFIX}' forbidden!
    
    Most likely you made a mistake, eg. did not set CMAKE_INSTALL_PREFIX
    correctly, or ran 'make install' instead of 'make package' during CPack
    package generation under windows, or some other human error.
    
    If, however, you are certain of what you are doing and wish to proceed,
    define KLF_CONFIRM_INSTALL_TO_ROOT_DIR environment variable to YES, eg.
    on your make install line:
      make install KLF_CONFIRM_INSTALL_TO_ROOT_DIR=YES

\")
    message(FATAL_ERROR \"Installation to root directory forbidden.\")
  endif(CMAKE_INSTALL_PREFIX STREQUAL \"\" OR CMAKE_INSTALL_PREFIX MATCHES \"^/(\\\\./?)?$\")

endif(\"\$ENV{DESTDIR}\" STREQUAL \"\" AND
      NOT \"\$ENV{KLF_CONFIRM_INSTALL_TO_ROOT_DIR}\" STREQUAL \"YES\")
")

# Fool-proof to warn a foolhardy user from using absolute paths with drive names in conjunction
# with DESTDIR during installs (windows)
install(CODE "
if(NOT \"\$ENV{DESTDIR}\" STREQUAL \"\" AND CMAKE_INSTALL_PREFIX MATCHES \"^[A-Za-z]:\")
  message(\"
    *** WARNING ***
    You are using CMAKE_INSTALL_PREFIX with a drive letter in conjunction with DESTDIR=... !
    This can lead to inconsistencies in paths, as cmake's install commands strip the drive
    letter from the prefix.
\")
endif(NOT \"\$ENV{DESTDIR}\" STREQUAL \"\" AND CMAKE_INSTALL_PREFIX MATCHES \"^[A-Za-z]:\")
")



# Uninstall target
# ----------------

configure_file(
  "${CMAKE_CURRENT_SOURCE_DIR}/cmake/cmake_uninstall.cmake.in"
  "${CMAKE_CURRENT_BINARY_DIR}/cmake_uninstall_script.cmake"
  IMMEDIATE @ONLY)
add_custom_target(uninstall
  "${CMAKE_COMMAND}" -P "${CMAKE_CURRENT_BINARY_DIR}/cmake_uninstall_script.cmake")


