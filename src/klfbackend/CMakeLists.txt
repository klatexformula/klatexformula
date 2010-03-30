# ################################################### #
# CMake project file for klatexformula/src/klfbackend #
# ################################################### #
# $Id$
# ################################################### #

set(klfbackend_SRCS
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
  else(KLF_BACKEND_STATIC)
    add_library(klfbackend SHARED ${klfbackend_allsrc})
    target_link_libraries(klfbackend ${QT_LIBRARIES})
    if(KLF_MACOSX_BUNDLES)
      set(klfbackend_FRAMEWORKS ${KLF_QT_FRAMEWORKS})
      KLFMakeFramework(klfbackend "${klfbackend_FRAMEWORKS}" "${klfbackend_INSTHEADERS}")
      set_target_properties(klfbackend PROPERTIES
		VERSION		${KLF_LIB_VERSION}
		SOVERSION	${KLF_LIB_VERSION}
      )
    endif(KLF_MACOSX_BUNDLES)
  endif(KLF_BACKEND_STATIC)
  set_target_properties(klfbackend PROPERTIES
	      PUBLIC_HEADER "${klfbackend_INSTHEADERS}"
  )
  install(TARGETS klfbackend LIBRARY DESTINATION "lib${KLF_LIB_SUFFIX}/"
			     ARCHIVE DESTINATION "lib${KLF_LIB_SUFFIX}/"
			     FRAMEWORK DESTINATION "lib${KLF_LIB_SUFFIX}/"
			     PUBLIC_HEADER DESTINATION "include/")
  if(KLF_MACOSX_BUNDLES)
    KLFInstFrameworkUpdateId(
      "/Library/Frameworks/klfbackend.framework/Versions/${KLF_LIB_VERSION}/klfbackend"
    )
  endif(KLF_MACOSX_BUNDLES)
  #install(FILES ${klfbackend_HEADERS} DESTINATION include/)
else(KLF_USE_QT4)
  add_definitions(${QT_DEFINITIONS})
  include_directories(${QT_INCLUDE_DIR})
  qt_wrap_cpp(klfbackend klfbackend_SRCS ${klfbackend_HEADERS})
  set(klfbackend_allsrc ${klfbackend_SRCS})
  if (KLF_BACKEND_STATIC)
    add_library(klfbackend-qt3 STATIC ${klfbackend_allsrc})
  else(KLF_BACKEND_STATIC)
    add_library(klfbackend-qt3 SHARED ${klfbackend_allsrc})
    target_link_libraries(klfbackend-qt3 ${QT_LIBRARIES})
# ....... TODO for APPLE ...... MAYBE: No frameworks, only normal libs (how is qt3 compiled?)
#    KLFMakeFramework(klfbackend-qt3 "${klfbackend_INSTHEADERS}")
  endif(KLF_BACKEND_STATIC)
  set_target_properties(klfbackend-qt3 PROPERTIES
	      PUBLIC_HEADER "${klfbackend_INSTHEADERS}"
  )
  install(TARGETS klfbackend-qt3 LIBRARY DESTINATION "lib${KLF_LIB_SUFFIX}/"
				 ARCHIVE DESTINATION "lib${KLF_LIB_SUFFIX}/"
				 FRAMEWORK DESTINATION "lib${KLF_LIB_SUFFIX}/"
				 PUBLIC_HEADER DESTINATION "include/")
endif(KLF_USE_QT4)


