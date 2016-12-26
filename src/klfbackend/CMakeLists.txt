# ################################################### #
# CMake project file for klatexformula/src/klfbackend #
# ################################################### #
# $Id$
# ################################################### #


set(klf_extra_search_paths_define_flags "")

if(KLF_EXTRA_SEARCH_PATHS)
  # Tell detectSettings() to go find latex at other locations, too
  set(klf_extra_search_paths "${KLF_EXTRA_SEARCH_PATHS}")
  # Transform ; list separators into C declaration string breaks
  string(REPLACE ";" "\",\"" klf_extra_search_paths "${klf_extra_search_paths}")
  set(klf_extra_search_paths_define_flags "-DKLF_EXTRA_SEARCH_PATHS=\"${klf_extra_search_paths}\"")
endif()


#
# Source files -- common to klfbackend and klfbackend_auto
#
set(klfbackend_SRCS
  klfbackend.cpp
  klfblockprocess.cpp
  klffilterprocess.cpp
  klflatexpreviewthread.cpp
  klfuserscript.cpp
  )
set(klfbackend_MOCHEADERS
  klfblockprocess.h
  klflatexpreviewthread.h
  klflatexpreviewthread_p.h
  )
set(klfbackend_HEADERS
  klfbackend.h
  klfbackend_p.h
  klfuserscript.h
  klffilterprocess.h
  ${klfbackend_MOCHEADERS}
  )


if(KLF_BUILD_BACKEND)

  if (KLF_LIBKLFBACKEND_STATIC)
    add_library(klfbackend STATIC ${klfbackend_SRCS})
  else()
    add_library(klfbackend SHARED ${klfbackend_SRCS})
  endif()
  if (KLF_MACOSX_BUNDLES)
    set_target_properties(klfbackend PROPERTIES
      FRAMEWORK	TRUE
      )
  endif()

  target_include_directories(klfbackend PRIVATE "." "../klftools")

  # determine which headers are public and should be installed, and set the target property
  KLFInstHeaders(klfbackend_INSTHEADERS "${klfbackend_HEADERS}")
  set_target_properties(klfbackend PROPERTIES
    VERSION		${KLF_LIB_VERSION}
    SOVERSION		${KLF_LIB_VERSION}
    PUBLIC_HEADER       "${klfbackend_INSTHEADERS}"
    )

  qt5_wrap_cpp(klfbackend_MOC_CPPS ${klfbackend_MOCHEADERS})
  target_sources(klfbackend PRIVATE ${klfbackend_MOC_CPPS})

  target_link_libraries(klfbackend PUBLIC Qt5::Core Qt5::Gui Qt5::Widgets klftools)

  add_dependencies(klfbackend klftools)

endif(KLF_BUILD_BACKEND)


# ---- KLFBACKEND_AUTO library:

if (KLF_BUILD_BACKEND_AUTO)

  # -- import sources from klftools to build *klfbackend_auto* library --
  set(klfbackend_auto_ADDSRCS
        ../klftools/klfdefs.cpp
        ../klftools/klfdebug.cpp
        ../klftools/klfutil.cpp
        ../klftools/klfdatautil.cpp
        ../klftools/klfpobj.cpp
        ../klftools/klffactory.cpp
    )
  set(klfbackend_auto_ADDMOCHEADERS
        ../klftools/klfdebug.h
    )
  set(klfbackend_auto_ADDHEADERS
        ../klftools/klfdefs.h
        ../klftools/klfutil.h
        ../klftools/klfdatautil.h
        ../klftools/klfpobj.h
        ../klftools/klffactory.h
        ../klftools/klfsysinfo.h
        ${klfbackend_auto_ADDMOCHEADERS}
    )
  # --
  
  if (KLF_LIBKLFBACKEND_AUTO_STATIC)
    add_library(klfbackend_auto STATIC ${klfbackend_SRCS} ${klfbackend_auto_ADDSRCS})
  else()
    add_library(klfbackend_auto SHARED ${klfbackend_SRCS} ${klfbackend_auto_ADDSRCS})
  endif()
  if (KLF_MACOSX_BUNDLES)
    set_target_properties(klfbackend_auto PROPERTIES
      FRAMEWORK	TRUE
      )
  endif()
  set_target_properties(klfbackend_auto PROPERTIES
    VERSION		${KLF_LIB_VERSION}
    SOVERSION		${KLF_LIB_VERSION}
    )

  target_include_directories(klfbackend_auto PRIVATE "../klftools")

  # No need for headers -- this library is meant to be distributed along another binary
  #
  #  KLFInstHeaders(klfbackend_auto_INSTHEADERS "${klfbackend_HEADERS};${klfbackend_auto_ADDHEADERS}")
  #  set_target_properties(klfbackend_auto PROPERTIES
  #    PUBLIC_HEADER ""
  #    )

  qt5_wrap_cpp(klfbackend_auto_MOC_CPPS ${klfbackend_MOCHEADERS} ${klfbackend_auto_ADDMOCHEADERS})
  target_sources(klfbackend_auto PRIVATE ${klfbackend_auto_MOC_CPPS})
  
  target_link_libraries(klfbackend_auto PUBLIC Qt5::Core Qt5::Gui)

endif(KLF_BUILD_BACKEND_AUTO)



# Install Targets
# ---------------

if(KLF_BUILD_BACKEND)
  KLFInstallLibrary(klfbackend KLF_INSTALL_KLFBACKEND_ "${KLF_INSTALL_LIB_DIR}" "include/klfbackend")
  # No support for fixing up frameworks installed by "make install".
  #
  # if(KLF_MACOSX_BUNDLES AND KLF_INSTALL_KLFBACKEND_FRAMEWORK)
  #   KLFMInstFrameworkUpdateId(
  #     "${KLF_INSTALL_LIB_DIR}/klfbackend.framework/Versions/${KLF_LIB_VERSION}/klfbackend"
  #     )
  #   # Installed library's dependency on klftools should be updated
  #   KLFMInstFrameworkUpdateLibChange(
  #     "${KLF_INSTALL_LIB_DIR}/klfbackend.framework/Versions/${KLF_LIB_VERSION}/klfbackend"
  #     "${CMAKE_CURRENT_BINARY_DIR}/klftools/klftools.framework/Versions/${KLF_LIB_VERSION}/klftools"
  #     "${KLF_INSTALL_LIB_DIR}/klftools.framework/Versions/${KLF_LIB_VERSION}/klftools"
  #     )
  # endif()
endif()


# --- and for klfbackend_auto :
if(KLF_BUILD_BACKEND_AUTO)
  KLFInstallLibrary(klfbackend_auto KLF_INSTALL_KLFBACKEND_AUTO_ "${KLF_INSTALL_LIB_DIR}" "")
  # No support for fixing up frameworks installed by "make install".
  #
  # if(KLF_MACOSX_BUNDLES AND KLF_INSTALL_KLFBACKEND_AUTO_FRAMEWORK)
  #   KLFMInstFrameworkUpdateId(
  #     "${KLF_INSTALL_LIB_DIR}/klfbackend_auto.framework/Versions/${KLF_LIB_VERSION}/klfbackend_auto"
  #     )
  # endif()
endif()
