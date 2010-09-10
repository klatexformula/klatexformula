# ################################################# #
# CMake project file for klatexformula/src/klftools #
# ################################################# #
# $Id$
# ################################################# #


# Sources
set(klftools_SRCS
	klfcolorchooser.cpp
	klfpobj.cpp
	klfutil.cpp
	klfguiutil.cpp
	klffactory.cpp
	klfpixmapbutton.cpp
	klfpathchooser.cpp
	qtcolortriangle.cpp
	klfdisplaylabel.cpp
	klfstyle.cpp
	klfsearchbar.cpp
	klfunitinput.cpp
)
# Headers
#  * with QObject classes (needs moc)
set(klftools_MOCHEADERS
	klfcolorchooser.h
	klfpixmapbutton.h
	klfguiutil.h
	klfpathchooser.h
	qtcolortriangle.h
	klfdisplaylabel.h
	klfunitinput.h
	klfsearchbar.h
)
#  * all headers
set(klftools_HEADERS
	klfutil.h
	klfpobj.h
	klffactory.h
	klfstyle.h
	klfiteratorsearchable.h
	${klftools_MOCHEADERS}
)
# UI Forms
set(klftools_UIS
	klfcolorchoosewidget.ui
	klfcolordialog.ui
	klfsearchbar.ui
)
# Resources
set(klftools_QRCS
	../klftoolsres.qrc
)




KLFInstHeaders(klftools_INSTHEADERS "${klftools_HEADERS}")

# our include directories
include_directories("${CMAKE_CURRENT_BINARY_DIR}" "${CMAKE_CURRENT_SOURCE_DIR}"
		    "${CMAKE_CURRENT_SOURCE_DIR}/../klfbackend")

# Uis, Mocs, Qrcs
qt4_wrap_ui(klftools_UIS_H ${klftools_UIS})
qt4_wrap_cpp(klftools_MOC_CPPS ${klftools_MOCHEADERS})
qt4_add_resources(klftools_QRC_CPPS ${klftools_QRCS})

set(klftools_allsrc ${klftools_SRCS} ${klftools_QRC_CPPS} ${klftools_UIS_H} ${klftools_MOC_CPPS})
if (KLF_LIBKLFTOOLS_STATIC)
  add_library(klftools STATIC ${klftools_allsrc})
  set_target_properties(klftools PROPERTIES
	    VERSION		${KLF_LIB_VERSION}
  )
else(KLF_LIBKLFTOOLS_STATIC)
  add_library(klftools SHARED ${klftools_allsrc})
  target_link_libraries(klftools ${QT_LIBRARIES} klfbackend)
  set_target_properties(klftools PROPERTIES
	    VERSION		${KLF_LIB_VERSION}
	    SOVERSION		${KLF_LIB_VERSION}
  )
  if(KLF_MACOSX_BUNDLES)
    set(klftools_FRAMEWORKS ${KLF_BUNDLE_QT_FRAMEWORKS})
    KLFMakeFramework(klftools "${klftools_FRAMEWORKS}" "${klftools_INSTHEADERS}")
  endif(KLF_MACOSX_BUNDLES)
endif(KLF_LIBKLFTOOLS_STATIC)

set_target_properties(klftools PROPERTIES
		#	    DEBUG_POSTFIX "${KLF_DEBUG_POSTFIX}"
	    PUBLIC_HEADER "${klftools_INSTHEADERS}"
)

# add dependency on klfbackend
add_dependencies(klftools  klfbackend)


# Install Targets
# ---------------

KLFInstallLibrary(klftools KLF_INSTALL_KLFTOOLS_ "${KLF_INSTALL_LIB_DIR}" "include/klftools")
if(KLF_MACOSX_BUNDLES AND KLF_INSTALL_KLFTOOLS_FRAMEWORK)
  KLFInstFrameworkUpdateId(
    "${KLF_INSTALL_LIB_DIR}/klftools.framework/Versions/${KLF_LIB_VERSION}/klftools"
  )
  # Installed library's dependency on klfbackend should be updated
  KLFInstFrameworkUpdateLibChange(
    "${KLF_INSTALL_LIB_DIR}/klftools.framework/Versions/${KLF_LIB_VERSION}/klftools"
    "${CMAKE_CURRENT_BINARY_DIR}/klfbackend/klfbackend.framework/Versions/${KLF_LIB_VERSION}/klfbackend"
    "${KLF_INSTALL_LIB_DIR}/klftools.framework/Versions/${KLF_LIB_VERSION}/klftools"
  )
endif(KLF_MACOSX_BUNDLES AND KLF_INSTALL_KLFTOOLS_FRAMEWORK)

