# ################################################ #
# CMake project file for klatexformula/src/plugins #
# ################################################ #
# $Id$
# ################################################ #


# We are no longer compiling KLF source !
remove_definitions(-DKLF_SRC_BUILD)

if(WIN32)
  # Windows
  set(KLF_CMAKE_PLUGINS_LIBNAME_SKIN skin.dll)
  set(KLF_CMAKE_PLUGINS_LIBNAME_SYSTRAYICON systrayicon.dll)
elseif(APPLE)
  # Apple Mac OS X
  set(KLF_CMAKE_PLUGINS_LIBNAME_SKIN libskin.so)
  set(KLF_CMAKE_PLUGINS_LIBNAME_SYSTRAYICON libsystrayicon.so)
else(WIN32)
  # Linux/Unix
  set(KLF_CMAKE_PLUGINS_LIBNAME_SKIN libskin.so)
  set(KLF_CMAKE_PLUGINS_LIBNAME_SYSTRAYICON libsystrayicon.so)
endif(WIN32)

configure_file(
  "${CMAKE_CURRENT_SOURCE_DIR}/klfbaseplugins.qrc.in"
  "${CMAKE_CURRENT_BINARY_DIR}/klfbaseplugins.qrc"
  IMMEDIATE @ONLY)
configure_file(
  "${CMAKE_CURRENT_SOURCE_DIR}/info_baseplugins.xml"
  "${CMAKE_CURRENT_BINARY_DIR}/info_baseplugins.xml"
  IMMEDIATE @ONLY)


add_subdirectory(skin)
add_subdirectory(systrayicon)

add_custom_command(OUTPUT klfbaseplugins.rcc
    COMMAND "${CMAKE_EXECUTABLE}" -E copy "skin/${KLF_CMAKE_PLUGINS_LIBNAME_SKIN}"
      "${KLF_CMAKE_PLUGINS_LIBNAME_SKIN}"
    COMMAND "${CMAKE_EXECUTABLE}" -E copy "systrayicon/${KLF_CMAKE_PLUGINS_LIBNAME_SYSTRAYICON}"
      "${KLF_CMAKE_PLUGINS_LIBNAME_SYSTRAYICON}"
    COMMAND ${QT_RCC_EXECUTABLE} -binary -o klfbaseplugins.rcc klfbaseplugins.qrc
    WORKING_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}"
    DEPENDS skin systrayicon
    COMMENT "Archiving Base Plugins into Add-On File"
    VERBATIM
)
add_custom_target(klfbaseplugins_rcc ALL
  DEPENDS klfbaseplugins.rcc
)
if(APPLE AND KLF_MACOSX_BUNDLE_PATH)
  add_custom_command(TARGET klfbaseplugins_rcc POST_BUILD
      COMMAND cp "${CMAKE_CURRENT_BINARY_DIR}/klfbaseplugins.rcc"
		 "${KLF_MACOSX_BUNDLE_PATH}/Contents/Resources/rccresources/klfbaseplugins.rcc"
      COMMENT "Installing base plugins into application bundle"   VERBATIM)
endif(APPLE AND KLF_MACOSX_BUNDLE_PATH)

if(KLF_DEVEL_LOCAL_BASEPLUGINS_COPY)
  add_custom_target(klfbaseplugins_rcc_localinstall ALL
    COMMAND "${CMAKE_EXECUTABLE}" -E copy klfbaseplugins.rcc "$ENV{HOME}/.klatexformula/rccresources/"
    DEPENDS klfbaseplugins.rcc
    COMMENT "Installing Base Plugins Add-On Locally in ~/.klatexformula/rccresources"
    VERBATIM
)
endif(KLF_DEVEL_LOCAL_BASEPLUGINS_COPY)

install(FILES klfbaseplugins.rcc DESTINATION share/klatexformula/rccresources)


