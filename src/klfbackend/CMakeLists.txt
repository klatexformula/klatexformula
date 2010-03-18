# ################################################### #
# CMake project file for klatexformula/src/klfbackend #
# ################################################### #
# $Id$
# ################################################### #

set(klfbackend_SRCS
	klfbackend.cpp
	klfblockprocess.cpp
)
set(klfbackend_HEADERS
#	klfdefs.h
#	klfbackend.h
	klfblockprocess.h
)

if(KLF_USE_QT4)
  qt4_wrap_cpp(klfbackend_MOC_CPPS ${klfbackend_HEADERS})
  set(klfbackend_allsrc ${klfbackend_SRCS} ${klfbackend_MOC_CPPS})
  if (KLF_BACKEND_STATIC)
    add_library(klfbackend STATIC ${klfbackend_allsrc})
  else(KLF_BACKEND_STATIC)
    add_library(klfbackend SHARED ${klfbackend_allsrc})
  endif(KLF_BACKEND_STATIC)
  install(TARGETS klfbackend LIBRARY DESTINATION lib${LIB_POSTFIX}
			     ARCHIVE DESTINATION lib${LIB_POSTFIX})
else(KLF_USE_QT4)
  add_definitions(${QT_DEFINITIONS})
  include_directories(${QT_INCLUDE_DIR})
  qt_wrap_cpp(klfbackend klfbackend_SRCS ${klfbackend_HEADERS})
  set(klfbackend_allsrc ${klfbackend_SRCS})
  if (KLF_BACKEND_STATIC)
    add_library(klfbackend-qt3 STATIC ${klfbackend_allsrc})
  else(KLF_BACKEND_STATIC)
    add_library(klfbackend SHARED ${klfbackend_allsrc})
  endif(KLF_BACKEND_STATIC)
  install(TARGETS klfbackend-qt3 LIBRARY DESTINATION lib${LIB_POSTFIX}
				 ARCHIVE DESTINATION lib${LIB_POSTFIX})
endif(KLF_USE_QT4)


