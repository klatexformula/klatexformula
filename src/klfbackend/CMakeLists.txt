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

# Source files
set(klfbackend_SRCS
	klfdefs.cpp
	klfbackend.cpp
	klfblockprocess.cpp
)
set(klfbackend_MOCHEADERS
	klfblockprocess.h
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
  if (KLF_BACKEND_STATIC)
    add_library(klfbackend STATIC ${klfbackend_allsrc})
    set_target_properties(klfbackend PROPERTIES
	      VERSION		${KLF_LIB_VERSION}
    )
  else(KLF_BACKEND_STATIC)
    add_library(klfbackend SHARED ${klfbackend_allsrc})
    target_link_libraries(klfbackend ${QT_LIBRARIES})
    set_target_properties(klfbackend PROPERTIES
	      VERSION		${KLF_LIB_VERSION}
	      SOVERSION		${KLF_LIB_VERSION}
    )
    if(KLF_MACOSX_BUNDLES)
      set(klfbackend_FRAMEWORKS ${KLF_QT_FRAMEWORKS})
      KLFMakeFramework(klfbackend "${klfbackend_FRAMEWORKS}" "${klfbackend_INSTHEADERS}")
    endif(KLF_MACOSX_BUNDLES)
  endif(KLF_BACKEND_STATIC)
  set_target_properties(klfbackend PROPERTIES
	      PUBLIC_HEADER "${klfbackend_INSTHEADERS}"
  )
else(KLF_USE_QT4)
  add_definitions(${QT_DEFINITIONS})
  include_directories(${QT_INCLUDE_DIR})
  qt_wrap_cpp(klfbackend-qt3 klfbackend_SRCS ${klfbackend_MOCHEADERS})
  set(klfbackend_allsrc ${klfbackend_SRCS})
  if (KLF_BACKEND_STATIC)
    add_library(klfbackend-qt3 STATIC ${klfbackend_allsrc})
  else(KLF_BACKEND_STATIC)
    add_library(klfbackend-qt3 SHARED ${klfbackend_allsrc})
    set_target_properties(klfbackend PROPERTIES
	      VERSION		${KLF_LIB_VERSION}
	      SOVERSION		${KLF_LIB_VERSION}
    )
# ....... TODO for APPLE ...... MAYBE: No frameworks, only normal libs (how is qt3 compiled?)
#    KLFMakeFramework(klfbackend-qt3 "${klfbackend_INSTHEADERS}")
  endif(KLF_BACKEND_STATIC)
  target_link_libraries(klfbackend-qt3 ${QT_LIBRARIES})
  set_target_properties(klfbackend-qt3 PROPERTIES
	      PUBLIC_HEADER "${klfbackend_INSTHEADERS}"
  )
endif(KLF_USE_QT4)



# Install Targets
# ---------------

if(KLF_USE_QT4)
  install(TARGETS klfbackend LIBRARY DESTINATION "${KLF_INSTALL_LIB_DIR}"
			     ARCHIVE DESTINATION "${KLF_INSTALL_LIB_DIR}"
			     FRAMEWORK DESTINATION "${KLF_INSTALL_LIB_DIR}"
			     PUBLIC_HEADER DESTINATION "include/klfbackend")
  if(KLF_MACOSX_BUNDLES)
    KLFInstFrameworkUpdateId(
      "${KLF_INSTALL_LIB_DIR}/klfbackend.framework/Versions/${KLF_LIB_VERSION}/klfbackend"
    )
  endif(KLF_MACOSX_BUNDLES)
else(KLF_USE_QT4)
  install(TARGETS klfbackend-qt3 LIBRARY DESTINATION "${KLF_INSTALL_LIB_DIR}"
				 ARCHIVE DESTINATION "${KLF_INSTALL_LIB_DIR}"
				 FRAMEWORK DESTINATION "${KLF_INSTALL_LIB_DIR}"
				 PUBLIC_HEADER DESTINATION "include/klfbackend")
endif(KLF_USE_QT4)


