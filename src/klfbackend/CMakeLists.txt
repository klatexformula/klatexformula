# ################################################### #
# CMake project file for klatexformula/src/klfbackend #
# ################################################### #
# $Id$
# ################################################### #

# Include our UseQt file here, so that we can require other configuration (not depend
# on so much qt libraries) as the main project.
set(QT_USE_QTXML TRUE)
include(${QT_USE_FILE})


if(KLF_EXTRA_SEARCH_PATHS)
  # Tell detectSettings() to go find latex at other locations, too
  set(klf_extra_search_paths "${KLF_EXTRA_SEARCH_PATHS}")
  # Transform ; list separators into C declaration string breaks
  string(REPLACE ";" "\",\"" klf_extra_search_paths "${klf_extra_search_paths}")
  add_definitions(-DKLF_EXTRA_SEARCH_PATHS="${klf_extra_search_paths}")
endif(KLF_EXTRA_SEARCH_PATHS)

include_directories(../klftools)

# Source files
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

KLFInstHeaders(klfbackend_INSTHEADERS "${klfbackend_HEADERS}")

if(KLF_BUILD_BACKEND)
  qt4_wrap_cpp(klfbackend_MOC_CPPS ${klfbackend_MOCHEADERS})
  set(klfbackend_allsrc ${klfbackend_SRCS} ${klfbackend_MOC_CPPS})
  if (KLF_LIBKLFBACKEND_STATIC)
    add_library(klfbackend STATIC ${klfbackend_allsrc})
    set_target_properties(klfbackend PROPERTIES
      VERSION		${KLF_LIB_VERSION}
      )
  else(KLF_LIBKLFBACKEND_STATIC)
    add_library(klfbackend SHARED ${klfbackend_allsrc})
    set_target_properties(klfbackend PROPERTIES
      VERSION		${KLF_LIB_VERSION}
      SOVERSION		${KLF_LIB_VERSION}
      )
    if(KLF_MACOSX_BUNDLES)
      set(klfbackend_FRAMEWORKS ${KLF_BUNDLE_QT_FRAMEWORKS})
      KLFMMakeFramework(klfbackend "${klfbackend_FRAMEWORKS}" "${klfbackend_INSTHEADERS}")
    endif(KLF_MACOSX_BUNDLES)
  endif(KLF_LIBKLFBACKEND_STATIC)
  set_target_properties(klfbackend PROPERTIES
    #	      DEBUG_POSTFIX "${KLF_DEBUG_POSTFIX}"
    PUBLIC_HEADER "${klfbackend_INSTHEADERS}"
    )
  # add dependency on klftools
  target_link_libraries(klfbackend ${QT_LIBRARIES} klftools)
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
  
  KLFInstHeaders(klfbackend_auto_INSTHEADERS "${klfbackend_HEADERS};${klfbackend_auto_ADDHEADERS}")

  qt4_wrap_cpp(klfbackend_auto_MOC_CPPS ${klfbackend_MOCHEADERS} ${klfbackend_auto_ADDMOCHEADERS})
  set(klfbackend_auto_allsrc ${klfbackend_SRCS} ${klfbackend_auto_ADDSRCS} ${klfbackend_auto_MOC_CPPS})
  
  if (KLF_LIBKLFBACKEND_AUTO_STATIC)
    add_library(klfbackend_auto STATIC ${klfbackend_auto_allsrc})
    set_target_properties(klfbackend_auto PROPERTIES
      VERSION		${KLF_LIB_VERSION}
      )
  else(KLF_LIBKLFBACKEND_AUTO_STATIC)
    add_library(klfbackend_auto SHARED ${klfbackend_auto_allsrc})
    set_target_properties(klfbackend_auto PROPERTIES
      VERSION		${KLF_LIB_VERSION}
      SOVERSION		${KLF_LIB_VERSION}
      )
    if(KLF_MACOSX_BUNDLES)
      set(klfbackend_auto_FRAMEWORKS ${KLF_BUNDLE_QT_FRAMEWORKS})
      KLFMMakeFramework(klfbackend_auto "${klfbackend_auto_FRAMEWORKS}" "") #"${klfbackend_auto_INSTHEADERS}")
    endif(KLF_MACOSX_BUNDLES)
  endif(KLF_LIBKLFBACKEND_AUTO_STATIC)

  set_target_properties(klfbackend_auto PROPERTIES
    #	      DEBUG_POSTFIX "${KLF_DEBUG_POSTFIX}"
    PUBLIC_HEADER "" # no need for headers(?), they're in klftools&klfbackend; "${klfbackend_auto_INSTHEADERS}"
    )
  target_link_libraries(klfbackend_auto ${QT_LIBRARIES})

endif(KLF_BUILD_BACKEND_AUTO)



# Install Targets
# ---------------

if(KLF_BUILD_BACKEND)
  KLFInstallLibrary(klfbackend KLF_INSTALL_KLFBACKEND_ "${KLF_INSTALL_LIB_DIR}" "include/klfbackend")
  if(KLF_MACOSX_BUNDLES AND KLF_INSTALL_KLFBACKEND_FRAMEWORK)
    KLFMInstFrameworkUpdateId(
      "${KLF_INSTALL_LIB_DIR}/klfbackend.framework/Versions/${KLF_LIB_VERSION}/klfbackend"
      )
    # Installed library's dependency on klftools should be updated
    KLFMInstFrameworkUpdateLibChange(
      "${KLF_INSTALL_LIB_DIR}/klfbackend.framework/Versions/${KLF_LIB_VERSION}/klfbackend"
      "${CMAKE_CURRENT_BINARY_DIR}/klftools/klftools.framework/Versions/${KLF_LIB_VERSION}/klftools"
      "${KLF_INSTALL_LIB_DIR}/klftools.framework/Versions/${KLF_LIB_VERSION}/klftools"
      )
  endif(KLF_MACOSX_BUNDLES AND KLF_INSTALL_KLFBACKEND_FRAMEWORK)
endif(KLF_BUILD_BACKEND)


# --- and for klfbackend_auto :
if(KLF_BUILD_BACKEND_AUTO)
  KLFInstallLibrary(klfbackend_auto KLF_INSTALL_KLFBACKEND_AUTO_ "${KLF_INSTALL_LIB_DIR}" "")
  if(KLF_MACOSX_BUNDLES AND KLF_INSTALL_KLFBACKEND_AUTO_FRAMEWORK)
    KLFMInstFrameworkUpdateId(
      "${KLF_INSTALL_LIB_DIR}/klfbackend_auto.framework/Versions/${KLF_LIB_VERSION}/klfbackend_auto"
      )
  endif(KLF_MACOSX_BUNDLES AND KLF_INSTALL_KLFBACKEND_AUTO_FRAMEWORK)

endif(KLF_BUILD_BACKEND_AUTO)
