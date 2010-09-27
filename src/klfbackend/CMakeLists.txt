# ################################################### #
# CMake project file for klatexformula/src/klfbackend #
# ################################################### #
# $Id$
# ################################################### #

# Include what we need from Qt for backend (only core+gui)

# include our Qt4 stuff; Qt3 needs nothing.
if(KLF_USE_QT4)
include(${QT_USE_FILE})
endif(KLF_USE_QT4)

if(KLF_INSTALL_LATEXDIST)
  # Tell detectSettings() to go find latex at our local latex distribution
  KLFRelativePath(klf_latexdist_dir "${KLF_ABS_INSTALL_BIN_DIR}" "${CMAKE_INSTALL_PREFIX}")
  set(klf_latexdist_dir "@executable_path/${klf_latexdist_dir}")
  add_definitions(-DKLF_EXTRA_SEARCH_PATHS="${klf_latexdist_dir}")
endif(KLF_INSTALL_LATEXDIST)


# Source files
set(klfbackend_SRCS
	klfdefs.cpp
	klfdebug.cpp
	klfbackend.cpp
	klfblockprocess.cpp
)
set(klfbackend_MOCHEADERS
	klfblockprocess.h
	klfdebug.h
)
set(klfbackend_HEADERS
	klfdefs.h
	klfbackend.h
	${klfbackend_MOCHEADERS}
)

KLFInstHeaders(klfbackend_INSTHEADERS "${klfbackend_HEADERS}")

if(KLF_USE_QT4)
  qt4_wrap_cpp(klfbackend_MOC_CPPS ${klfbackend_MOCHEADERS})
  set(klfbackend_allsrc ${klfbackend_SRCS} ${klfbackend_MOC_CPPS})
  if (KLF_LIBKLFBACKEND_STATIC)
    add_library(klfbackend STATIC ${klfbackend_allsrc})
    set_target_properties(klfbackend PROPERTIES
	      VERSION		${KLF_LIB_VERSION}
    )
  else(KLF_LIBKLFBACKEND_STATIC)
    add_library(klfbackend SHARED ${klfbackend_allsrc})
    target_link_libraries(klfbackend ${QT_LIBRARIES})
    set_target_properties(klfbackend PROPERTIES
	      VERSION		${KLF_LIB_VERSION}
	      SOVERSION		${KLF_LIB_VERSION}
    )
    if(KLF_MACOSX_BUNDLES)
      set(klfbackend_FRAMEWORKS ${KLF_BUNDLE_QT_FRAMEWORKS})
      KLFMakeFramework(klfbackend "${klfbackend_FRAMEWORKS}" "${klfbackend_INSTHEADERS}")
    endif(KLF_MACOSX_BUNDLES)
  endif(KLF_LIBKLFBACKEND_STATIC)
  set_target_properties(klfbackend PROPERTIES
		#	      DEBUG_POSTFIX "${KLF_DEBUG_POSTFIX}"
	      PUBLIC_HEADER "${klfbackend_INSTHEADERS}"
  )
else(KLF_USE_QT4)
  add_definitions(${QT_DEFINITIONS})
  include_directories(${QT_INCLUDE_DIR})
  qt_wrap_cpp(klfbackend-qt3 klfbackend_SRCS ${klfbackend_MOCHEADERS})
  set(klfbackend_allsrc ${klfbackend_SRCS})
  if (KLF_LIBKLFBACKEND_STATIC)
    add_library(klfbackend-qt3 STATIC ${klfbackend_allsrc})
  else(KLF_LIBKLFBACKEND_STATIC)
    add_library(klfbackend-qt3 SHARED ${klfbackend_allsrc})
    set_target_properties(klfbackend-qt3 PROPERTIES
	      VERSION		${KLF_LIB_VERSION}
	      SOVERSION		${KLF_LIB_VERSION}
    )
  # ....... TODO for APPLE ...... MAYBE: No frameworks, only normal libs (how is qt3 compiled?)
  #    KLFMakeFramework(klfbackend-qt3 "${klfbackend_INSTHEADERS}")
  endif(KLF_LIBKLFBACKEND_STATIC)
  target_link_libraries(klfbackend-qt3 ${QT_LIBRARIES})
  set_target_properties(klfbackend-qt3 PROPERTIES
		#	      DEBUG_POSTFIX "${KLF_DEBUG_POSTFIX}"
	      PUBLIC_HEADER "${klfbackend_INSTHEADERS}"
  )
endif(KLF_USE_QT4)



# Install Targets
# ---------------

if(KLF_USE_QT4)
  KLFInstallLibrary(klfbackend KLF_INSTALL_KLFBACKEND_ "${KLF_INSTALL_LIB_DIR}" "include/klfbackend")
  if(KLF_MACOSX_BUNDLES AND KLF_INSTALL_KLFBACKEND_FRAMEWORK)
    KLFInstFrameworkUpdateId(
      "${KLF_INSTALL_LIB_DIR}/klfbackend.framework/Versions/${KLF_LIB_VERSION}/klfbackend"
    )
  endif(KLF_MACOSX_BUNDLES AND KLF_INSTALL_KLFBACKEND_FRAMEWORK)
else(KLF_USE_QT4)
  KLFInstallLibrary(klfbackend-qt3 KLF_INSTALL_KLFBACKEND_ "${KLF_INSTALL_LIB_DIR}" "include/klfbackend")
endif(KLF_USE_QT4)

