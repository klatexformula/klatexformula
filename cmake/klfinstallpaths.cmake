# CMake Install Instructions
# ==========================
# $Id$



# Installation destination
# ------------------------

set(CMAKE_INSTALL_PREFIX "/usr" CACHE PATH "The installation prefix for KLatexFormula.")
mark_as_advanced(CLEAR CMAKE_INSTALL_PREFIX)
message(STATUS "Install prefix for 'make install': \"${CMAKE_INSTALL_PREFIX}\" (CMAKE_INSTALL_PREFIX)")


# Installation Paths
# ------------------

include(GNUInstallDirs)


# Lib Dir
set(default_KLF_INSTALL_LIB_DIR "${CMAKE_INSTALL_LIBDIR}")
if(WIN32)
  # On windows, install DLLs alongside the executable inside "bin"
  set(default_KLF_INSTALL_LIB_DIR "bin")
endif()
if(APPLE AND KLF_MACOSX_BUNDLES)
  # install to /Library/Frameworks on mac OS X
  set(default_KLF_INSTALL_LIB_DIR "/Library/Frameworks")
endif()
KLFSetIfNotDefined(KLF_INSTALL_LIB_DIR "${default_KLF_INSTALL_LIB_DIR}")
message(STATUS "Installing libraries to \"${KLF_INSTALL_LIB_DIR}\" (KLF_INSTALL_LIB_DIR)")

# Bin Dir
set(default_KLF_INSTALL_BIN_DIR ${CMAKE_INSTALL_BINDIR})
if(WIN32)
  # install binary in "bin" on Ms Windows
  set(default_KLF_INSTALL_BIN_DIR "bin")
endif()
if(APPLE AND KLF_MACOSX_BUNDLES)
  # install to /Applications on mac OS X
  set(default_KLF_INSTALL_BIN_DIR "/Applications")
endif()
KLFSetIfNotDefined(KLF_INSTALL_BIN_DIR "${default_KLF_INSTALL_BIN_DIR}")
message(STATUS "Installing binaries to \"${KLF_INSTALL_BIN_DIR}\" (KLF_INSTALL_BIN_DIR)")

# Header Dir
set(default_KLF_INSTALL_HEADER_DIR ${CMAKE_INSTALL_INCLUDEDIR})
if(WIN32)
  # install headers in "include/" on Ms Windows
  set(default_KLF_INSTALL_HEADER_DIR "include")
endif()
if(APPLE AND KLF_MACOSX_BUNDLES)
  # install to /Applications on mac OS X
  set(default_KLF_INSTALL_HEADER_DIR "/Applications")
endif()
KLFSetIfNotDefined(KLF_INSTALL_HEADER_DIR "${default_KLF_INSTALL_HEADER_DIR}")
message(STATUS "Installing headers in \"${KLF_INSTALL_HEADER_DIR}\" (KLF_INSTALL_HEADER_DIR)")


# Utility variables. Garanteed to be absolute path. Not kept in cache, it is (trivially) computed here
KLFMakeAbsInstallPath(KLF_ABS_INSTALL_BIN_DIR  KLF_INSTALL_BIN_DIR)
KLFMakeAbsInstallPath(KLF_ABS_INSTALL_LIB_DIR  KLF_INSTALL_LIB_DIR)
KLFMakeAbsInstallPath(KLF_ABS_INSTALL_HEADER_DIR  KLF_INSTALL_HEADER_DIR)




# Designer Plugin Library Dir
# ...
if(KLF_BUILD_TOOLSDESPLUGIN)
  if(KLF_INSTALL_KLFTOOLSDESPLUGIN AND NOT DEFINED KLF_INSTALL_DESPLUGIN_DIR)
    # I haven't found the "official" way to find out where to install the
    # plugin; I didn't find any CMake variable or any indication in the official
    # documentation on where to install the plugin. So leave it up to the user.
    # In any case, the users who will want this installed know what this is
    # about and will know the final location better than what a script can
    # guess.
    KLFNote("The klftools qt designer plugin will not be installed. If you want to install
    it, please specify the final installation location in
    KLF_INSTALL_DESPLUGIN_DIR (or set to an empty value to silence this
    message) [see also KLF_INSTALL_KLFTOOLSDESPLUGIN=YES|NO].
")
    set(KLF_INSTALL_DESPLUGIN_DIR "")
    # and change the default to "don't install"
    KLFSetIfNotDefined(KLF_INSTALL_KLFTOOLSDESPLUGIN NO)
  endif()
  if(KLF_INSTALL_DESPLUGIN_DIR)
    message(STATUS "Klftools designer plugin install directory set to \"${KLF_INSTALL_DESPLUGIN_DIR}\" (KLF_INSTALL_DESPLUGIN_DIR)")
  endif()
endif()


# Installed RPATH -- may be useful
# --------------------------------

# option for setting up RPATH on binary
if(NOT CMAKE_SKIP_RPATH)
  KLFSetIfNotDefined(CMAKE_INSTALL_RPATH "${KLF_ABS_INSTALL_LIB_DIR}")
  message(STATUS "RPATH for installed libraries and executables: \"${CMAKE_INSTALL_RPATH}\" (CMAKE_SKIP_RPATH,CMAKE_INSTALL_RPATH)")
else(NOT CMAKE_SKIP_RPATH)
  message(STATUS "Skipping RPATH on executables and libraries (CMAKE_SKIP_RPATH,CMAKE_INSTALL_RPATH)")
endif(NOT CMAKE_SKIP_RPATH)


# What to Install?
# ----------------

# general options
KLFSetIfNotDefined(KLF_INSTALL_RUNTIME YES)
KLFSetIfNotDefined(KLF_INSTALL_DEVEL YES)


# fine-tuning
# the OFF's at end of line is to turn off the KLFNotes() indicating the value changed
if(KLF_BUILD_TOOLS)
  KLFSetIfNotDefined(KLF_INSTALL_KLFTOOLS_HEADERS      ${KLF_INSTALL_DEVEL})
  KLFSetIfNotDefined(KLF_INSTALL_KLFTOOLS_STATIC_LIBS  ${KLF_INSTALL_DEVEL})
  KLFSetIfNotDefined(KLF_INSTALL_KLFTOOLS_SO_LIBS      ${KLF_INSTALL_RUNTIME})
  KLFSetIfNotDefined(KLF_INSTALL_KLFTOOLS_FRAMEWORK    ${KLF_INSTALL_RUNTIME})
endif()
if(KLF_BUILD_TOOLSDESPLUGIN)
  KLFSetIfNotDefined(KLF_INSTALL_KLFTOOLSDESPLUGIN     ${KLF_INSTALL_DEVEL})
endif()

if(KLF_BUILD_BACKEND)
  KLFSetIfNotDefined(KLF_INSTALL_KLFBACKEND_HEADERS     ${KLF_INSTALL_DEVEL})
  KLFSetIfNotDefined(KLF_INSTALL_KLFBACKEND_STATIC_LIBS ${KLF_INSTALL_DEVEL})
  KLFSetIfNotDefined(KLF_INSTALL_KLFBACKEND_SO_LIBS     ${KLF_INSTALL_RUNTIME})
  KLFSetIfNotDefined(KLF_INSTALL_KLFBACKEND_FRAMEWORK   ${KLF_INSTALL_RUNTIME})
endif()

if(KLF_BUILD_BACKEND_AUTO)
  KLFSetIfNotDefined(KLF_INSTALL_KLFBACKEND_AUTO_STATIC_LIBS ${KLF_INSTALL_DEVEL})
  KLFSetIfNotDefined(KLF_INSTALL_KLFBACKEND_AUTO_SO_LIBS     ${KLF_INSTALL_RUNTIME})
  KLFSetIfNotDefined(KLF_INSTALL_KLFBACKEND_AUTO_FRAMEWORK   ${KLF_INSTALL_RUNTIME})
endif()

if(KLF_BUILD_GUI)
  KLFSetIfNotDefined(KLF_INSTALL_KLATEXFORMULA_BIN   ${KLF_INSTALL_RUNTIME})
  KLFSetIfNotDefined(KLF_INSTALL_KLATEXFORMULA_CMDL  ${KLF_INSTALL_RUNTIME})
  if(KLF_MACOSX_BUNDLES)
    KLFSetIfNotDefined(KLF_INSTALL_KLATEXFORMULA_BUNDLE ${KLF_INSTALL_RUNTIME})
  endif()
endif(KLF_BUILD_GUI)

set(tmp_msg "Will install targets:

                       \theaders\t\tstatic,\t    shared libraries   \tframework")
if(KLF_BUILD_TOOLS)
  set(tmp_msg "${tmp_msg}
   klftools:           \t  ${KLF_INSTALL_KLFTOOLS_HEADERS}\t\t  ${KLF_INSTALL_KLFTOOLS_STATIC_LIBS}\t\t  ${KLF_INSTALL_KLFTOOLS_SO_LIBS}\t\t  ${KLF_INSTALL_KLFTOOLS_FRAMEWORK}")
endif()
if(KLF_BUILD_BACKEND)
  set(tmp_msg "${tmp_msg}
   klfbackend:         \t  ${KLF_INSTALL_KLFBACKEND_HEADERS}\t\t  ${KLF_INSTALL_KLFBACKEND_STATIC_LIBS}\t\t  ${KLF_INSTALL_KLFBACKEND_SO_LIBS}\t\t  ${KLF_INSTALL_KLFBACKEND_FRAMEWORK}")
endif()
if(KLF_BUILD_BACKEND_AUTO)
  set(tmp_msg "${tmp_msg}
   klfbackend_auto:       --\t\t  ${KLF_INSTALL_KLFBACKEND_AUTO_STATIC_LIBS}\t\t  ${KLF_INSTALL_KLFBACKEND_AUTO_SO_LIBS}\t\t  ${KLF_INSTALL_KLFBACKEND_AUTO_FRAMEWORK}")
endif()
if(KLF_BUILD_TOOLSDESPLUGIN)
  set(tmp_msg "${tmp_msg}
   klftoolsdesplugin:     --\t\t  --\t\t  ${KLF_INSTALL_KLFTOOLSDESPLUGIN}\t\t  --")
endif()
if(KLF_BUILD_GUI)
  if (KLF_MACOSX_BUNDLES)
    set(tmp_msg "${tmp_msg}

   klatexformula bundle: \t${KLF_INSTALL_KLATEXFORMULA_BUNDLE}")
  else()
    set(tmp_msg "${tmp_msg}

   klatexformula:        \t${KLF_INSTALL_KLATEXFORMULA_BIN}
   klatexformula_cmdl:   \t${KLF_INSTALL_KLATEXFORMULA_CMDL}")
  endif()
endif()
set(tmp_msg "${tmp_msg}

   Set master switches:
     - KLF_INSTALL_RUNTIME=<BOOL>  to install (or not) binaries, frameworks and
                                   shared libraries in general;
     - KLF_INSTALL_DEVEL=<BOOL>    to install (or not) static libraries and
                                   headers in general;
   Individual installs can be fine-tuned with the following boolean variables:
     - KLF_INSTALL_KLF{TOOLS|BACKEND|BACKEND_AUTO}_{HEADERS|SO_LIBS|STATIC_LIBS|FRAMEWORK};
     - KLF_INSTALL_KLFTOOLSDESPLUGIN;
     - KLF_INSTALL_KLATEXFORMULA_{BIN|CMDL|BUNDLE}.
   If set, these variables take precedence over KLF_INSTALL_{RUNTIME|DEVEL}.
   Irrelevant variables (e.g. *_SO_LIBS for static builds or *_FRAMEWORK for
   non-macs) are ignored.

")
message(STATUS "${tmp_msg}")


if(KLF_BUILD_GUI)

  # Man Dir
  if(HELP2MAN)
    if(KLF_MACOSX_BUNDLES)
      set(default_KLF_INSTALL_SHARE_MAN1_DIR "")
    elseif(WIN32)
      set(default_KLF_INSTALL_SHARE_MAN1_DIR "")
    else() # linux, or unix-like but not app bundle
      set(default_KLF_INSTALL_SHARE_MAN1_DIR "${CMAKE_INSTALL_MANDIR}/man1")
    endif()
    KLFSetIfNotDefined(KLF_INSTALL_SHARE_MAN1_DIR "${default_KLF_INSTALL_SHARE_MAN1_DIR}")
    
    if(KLF_INSTALL_SHARE_MAN1_DIR)
      message(STATUS "Installing man page to \"${KLF_INSTALL_SHARE_MAN1_DIR}\" (KLF_INSTALL_SHARE_MAN1_DIR)")
    else()
      message(STATUS "Not installing man page (KLF_INSTALL_SHARE_MAN1_DIR)")
    endif()
  endif() # Man Dir

  # KLF_INSTALL_SHARE_KLF_DATA_DIR
  if(KLF_MACOSX_BUNDLES)
    set(default_KLF_INSTALL_SHARE_KLF_DATA_DIR "")
  else() # e.g. linux, win, apple-but-not-bundle
    set(default_KLF_INSTALL_SHARE_KLF_DATA_DIR "${CMAKE_INSTALL_DATADIR}/klatexformula")
  endif()
  KLFSetIfNotDefined(KLF_INSTALL_SHARE_KLF_DATA_DIR "${default_KLF_INSTALL_SHARE_KLF_DATA_DIR}")
  if(KLF_INSTALL_SHARE_KLF_DATA_DIR)
    message(STATUS "Installing shared klatexformula data (e.g. user scripts) to \"${KLF_INSTALL_SHARE_KLF_DATA_DIR}\" (KLF_INSTALL_SHARE_KLF_DATA_DIR)")
  elseif(default_KLF_INSTALL_SHARE_KLF_DATA_DIR)
    message(STATUS "Not installing shared klatexformula data (e.g. user scripts) (KLF_INSTALL_SHARE_KLF_DATA_DIR)")
  else()
    # no message if this is not relevant, e.g., if we're building a bundle
  endif()
  
  if(KLF_MACOSX_BUNDLES OR WIN32)
    set(default_KLF_INSTALL_DESKTOP FALSE)
  else()
    set(default_KLF_INSTALL_DESKTOP TRUE)
  endif()

  # Install .desktop & pixmaps in PREFIX/share/{applications|pixmaps} ?
  option(KLF_INSTALL_DESKTOP "Install linux desktop related files" ${default_KLF_INSTALL_DESKTOP})

  if(KLF_INSTALL_DESKTOP)

    KLFSetIfNotDefined(KLF_INSTALL_DESKTOP_CATEGORIES "Qt;Office;")
    KLFSetIfNotDefined(KLF_INSTALL_SHARE_APPLICATIONS_DIR "share/applications/")
    KLFSetIfNotDefined(KLF_INSTALL_SHARE_PIXMAPS_DIR "share/pixmaps/")
    KLFSetIfNotDefined(KLF_INSTALL_ICON_THEME "")
    KLFSetIfNotDefined(KLF_INSTALL_SHARE_MIME_PACKAGES_DIR "share/mime/packages/")
  
    # Reasonable Icon= entry given the installation settings
    set(klf_icon "klatexformula")
    if(KLF_INSTALL_SHARE_PIXMAPS_DIR)
      KLFMakeAbsInstallPath(abs_share_pix_dir KLF_INSTALL_SHARE_PIXMAPS_DIR)
      set(klf_icon "${abs_share_pix_dir}/klatexformula-64.png")
    endif()
    KLFSetIfNotDefined(KLF_INSTALL_DESKTOP_ICON "${klf_icon}")

    message(STATUS "Will install linux desktop files (KLF_INSTALL_DESKTOP=<BOOL>):
   .desktop categories:      ${KLF_INSTALL_DESKTOP_CATEGORIES}  (KLF_INSTALL_DESKTOP_CATEGORIES)
   .desktop Icon= entry:     ${KLF_INSTALL_DESKTOP_ICON}  (KLF_INSTALL_DESKTOP_ICON)
   app .desktop files:       ${KLF_INSTALL_SHARE_APPLICATIONS_DIR}  (KLF_INSTALL_SHARE_APPLICATIONS_DIR)
   mime database xml:        ${KLF_INSTALL_SHARE_MIME_PACKAGES_DIR}  (KLF_INSTALL_SHARE_MIME_PACKAGES_DIR)
   pixmaps:                  ${KLF_INSTALL_SHARE_PIXMAPS_DIR}  (KLF_INSTALL_SHARE_PIXMAPS_DIR)
   install into icon theme:  ${KLF_INSTALL_ICON_THEME}  (KLF_INSTALL_ICON_THEME)

   All installation paths are relative to CMAKE_INSTALL_PREFIX if not
   absolute. Setting the corresponding variable to an empty value disables that
   particular install.
")
  else(KLF_INSTALL_DESKTOP)
    message(STATUS "Will not install linux desktop files (KLF_INSTALL_DESKTOP=<BOOL>)")
  endif(KLF_INSTALL_DESKTOP)

endif()



if(WIN32)
  set(default_install_qtlibs OFF) # let the user specify this explicitly -- not sure which
                                  # exact library deps to install
  set(default_install_qtplugins ON)
else(WIN32)
  set(default_install_qtlibs OFF)
  set(default_install_qtplugins OFF)
endif(WIN32)

if(default_install_qtlibs)
  # add these options to cache only on the platforms for which it makes sense
  option(KLF_INSTALL_QTLIBS "Copy Qt Libs next to installed executable" TRUE)
  option(KLF_INSTALL_QTPLUGINS "Copy Qt Plugins next to installed executable" TRUE)
else()
  # otherwise, honor a cached value by don't add it if it's not already there
  KLFSetIfNotDefined(KLF_INSTALL_QTLIBS FALSE)
  KLFSetIfNotDefined(KLF_INSTALL_QTPLUGINS FALSE)
endif()

if(KLF_MACOSX_BUNDLES AND KLF_INSTALL_QTLIBS)
  KLFNote("You should not set KLF_INSTALL_QTLIBS when building an app bundle. Qt
   libraries can be imported into the application bundle (KLF_MACOSX_BUNDLE_EXTRAS)")
endif()
if(KLF_MACOSX_BUNDLES AND KLF_INSTALL_QTLIBS)
  KLFNote("You should not set KLF_INSTALL_QTPLUGINS when building an app bundle.
    Qt plugins should be bundled into the application bundle
    (KLF_BUNDLE_QT_PLUGINS)")
endif()

if(KLF_INSTALL_QTLIBS)
  set(KLF_INSTALL_QTLIBS_EXTRADEPS "" CACHE STRING
    "List of additional dlls (full paths) to include alonside installed executable")
  message(STATUS "Will install Qt libs next to installed executable (KLF_INSTALL_QTLIBS,KLF_INSTALL_QTLIBS_EXTRADEPS)")
elseif(default_install_qtlibs)
  message(STATUS "Will NOT install Qt libs next to installed executable (KLF_INSTALL_QTLIBS,KLF_INSTALL_QTLIBS_EXTRADEPS)")
else()
  # no message required on unix-like systems -- don't confuse user
endif()
#
if(KLF_INSTALL_QTPLUGINS)
  # put qt plugins in same directory as executable -- I couldn't otherwise get
  # addLibraryPath() to work.. :( why??
  KLFSetIfNotDefined(KLF_INSTALL_QTPLUGINS_DIR "${KLF_INSTALL_BIN_DIR}")
  set(default_KLF_INSTALL_QTPLUGINS_LIST
    ${Qt5Core_PLUGINS} ${Qt5Gui_PLUGINS} ${Qt5Widgets_PLUGINS} ${Qt5WinExtras_PLUGINS}
    Qt5::QSQLiteDriverPlugin Qt5::QSvgIconPlugin)
  KLFSetIfNotDefined(KLF_INSTALL_QTPLUGINS_LIST "${default_KLF_INSTALL_QTPLUGINS_LIST}")
  message(STATUS "Will install given Qt plugins next to installed executable, in
    ${KLF_INSTALL_QTPLUGINS_DIR}
    (KLF_INSTALL_QTPLUGINS_DIR=<BOOL>,KLF_INSTALL_QTPLUGINS_LIST=<list of qt5
    plugin targets e.g. Qt5::SvgIconEnginePlugin>)")
elseif(WIN32)
  message(STATUS "Will NOT install Qt plugins next to installed executable (KLF_INSTALL_QTPLUGINS)")
endif(KLF_INSTALL_QTPLUGINS)



# Run Post-Install Scripts
# ------------------------

if(KLF_BUILD_GUI)
  
  set(default_KLF_INSTALL_POST_UPDATEMIMEDATABASE OFF)
  if(KLF_INSTALL_DESKTOP AND KLF_INSTALL_SHARE_MIME_PACKAGES_DIR)
    set(default_KLF_INSTALL_POST_UPDATEMIMEDATABASE ON)
  endif(KLF_INSTALL_DESKTOP AND KLF_INSTALL_SHARE_MIME_PACKAGES_DIR)
  KLFSetIfNotDefined(KLF_INSTALL_POST_UPDATEMIMEDATABASE "${default_KLF_INSTALL_POST_UPDATEMIMEDATABASE}")

  if(KLF_INSTALL_POST_UPDATEMIMEDATABASE)
    message(STATUS "Will update the mime database after installing (KLF_INSTALL_POST_UPDATEMIMEDATABASE)")
  elseif(default_KLF_INSTALL_POST_UPDATEMIMEDATABASE)
    message(STATUS "Will NOT update the mime database after installing (KLF_INSTALL_POST_UPDATEMIMEDATABASE)")
  else()
    # no status message where it doesn't make sense
  endif()

endif()



# Doxygen API Documentation Installation
# --------------------------------------

# install targets are in klfdoxygen.cmake; this is because klfdoxygen.cmake is
# loaded after all the other targets and after all other subprojects




# # Install a distribution of LaTeX
# # -------------------------------

# # basically copies a directory containing a LaTeX distribution, to an installation location. This
# # is intentended to create "-with-latex" binary packages.

# set(KLF_INSTALL_LATEXDIST "" CACHE PATH "Path to a local latex installation to install")
# set(KLF_INSTALL_LATEXDIST_DIR "latex" CACHE STRING
#   "path to install the latex distribution KLF_INSTALL_LATEXDIST to. (rel. to prefix, or abs.)")

# KLFGetCMakeVarChanged(KLF_INSTALL_LATEXDIST)
# KLFGetCMakeVarChanged(KLF_INSTALL_LATEXDIST_DIR)

# if(KLF_INSTALL_LATEXDIST)
#   set(klfinstall_latexdist "${KLF_INSTALL_LATEXDIST}")
#   # Force terminating '/'
#   if(klfinstall_latexdist MATCHES "[^/]$")
#     set(klfinstall_latexdist "${klfinstall_latexdist}/")
#   endif(klfinstall_latexdist MATCHES "[^/]$")
#   install(DIRECTORY "${klfinstall_latexdist}" DESTINATION "${KLF_INSTALL_LATEXDIST_DIR}")

#   message(STATUS "Will use the local latex installation ${klfinstall_latexdist}
#     and install it to ${KLF_INSTALL_LATEXDIST_DIR}")

#   KLFMakeAbsInstallPath(KLF_ABS_INSTALL_LATEXDIST_DIR  KLF_INSTALL_LATEXDIST_DIR)

#   KLFRelativePath(klf_latexdist_reldir "${KLF_ABS_INSTALL_BIN_DIR}" "${KLF_ABS_INSTALL_LATEXDIST_DIR}")
#   set(klf_latexdist_reldir "@executable_path/${klf_latexdist_reldir}")
#   set(klf_extrasearchpaths
#     "${klf_latexdist_reldir}"
#     "${klf_latexdist_reldir}/*"
#     "${klf_latexdist_reldir}/*/*")
# else(KLF_INSTALL_LATEXDIST)
#   set(klf_extrasearchpaths "")
# endif(KLF_INSTALL_LATEXDIST)




# ---------------------------------------------------


macro(KLFInstallLibrary targetlib varOptBase inst_pubheader_subdir)

  # this dummy installation directory cannot be set to an absolute path (actually, it could..., but
  # it's a headache)  because on windows installing to absolute paths with drive letters is EVIL
  # when using DESTDIR=... (eg. for packaging). Instead, install to a dummy directory specified as
  # a relative path in final installation directory, then remove that dummy directory.
  set(klf_dummy_inst_dir "__klf_dummy_install_directory__")

  KLFSetConditional(inst_${targetlib}_runtime_dir
    "${KLF_INSTALL_LIB_DIR}"  "${klf_dummy_inst_dir}" DEPENDING_IF ${varOptBase}SO_LIBS)
  KLFSetConditional(inst_${targetlib}_library_dir
    "${KLF_INSTALL_LIB_DIR}"  "${klf_dummy_inst_dir}" DEPENDING_IF ${varOptBase}SO_LIBS)
  KLFSetConditional(inst_${targetlib}_archive_dir
    "${KLF_INSTALL_LIB_DIR}"  "${klf_dummy_inst_dir}" DEPENDING_IF ${varOptBase}STATIC_LIBS)
  KLFSetConditional(inst_${targetlib}_framework_dir
    "${KLF_INSTALL_LIB_DIR}"  "${klf_dummy_inst_dir}" DEPENDING_IF ${varOptBase}FRAMEWORK)
  KLFSetConditional(inst_${targetlib}_pubheader_dir
    "${KLF_INSTALL_HEADER_DIR}/${inst_pubheader_subdir}" "${klf_dummy_inst_dir}"
    DEPENDING_IF ${varOptBase}HEADERS)

  set(need_dummy_dir FALSE)
  if(NOT ${varOptBase}SO_LIBS OR NOT ${varOptBase}SO_LIBS OR NOT ${varOptBase}STATIC_LIBS
     OR NOT ${varOptBase}FRAMEWORK OR NOT ${varOptBase}HEADERS)
    set(need_dummy_dir TRUE)
  endif()

  KLFCMakeDebug("Install paths for library ${targetlib}:
	RUNTIME DESTINATION ${inst_${targetlib}_runtime_dir}
	LIBRARY DESTINATION ${inst_${targetlib}_library_dir}
	ARCHIVE DESTINATION ${inst_${targetlib}_archive_dir}
	FRAMEWORK DESTINATION ${inst_${targetlib}_framework_dir}
	PUBLIC_HEADER DESTINATION ${inst_${targetlib}_pubheader_dir}
")

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

# Fool-proof to warn a candid user from using absolute paths with drive names in conjunction
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


