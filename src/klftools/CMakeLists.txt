# ################################################# #
# CMake project file for klatexformula/src/klftools #
# ################################################# #
# $Id$
# ################################################# #


# Include our UseQt file here, so that we can require other configuration (not depend
# on so much qt libraries) as the main project.
set(QT_USE_QTXML TRUE)
include(${QT_USE_FILE})


# Sources
set(klftools_SRCS
	klfdefs.cpp
	klfdebug.cpp
	klfcolorchooser.cpp
	klfpobj.cpp
	klfutil.cpp
	klfdatautil.cpp
	klfconfigbase.cpp
	klfguiutil.cpp
	klffactory.cpp
	klfpixmapbutton.cpp
	klfpathchooser.cpp
	qtcolortriangle.cpp
	klfdisplaylabel.cpp
	klfsearchbar.cpp
	klfunitinput.cpp
	klflatexedit.cpp
	klfrelativefont.cpp
	klfenumlistwidget.cpp
	klfflowlayout.cpp
	klfflowlistwidget.cpp
	klfsidewidget.cpp
	klfpobjeditwidget.cpp
	klfadvancedconfigeditor.cpp
	klfitemviewsearchtarget.cpp
)
# Headers
#  * with QObject classes (needs moc)
set(klftools_MOCHEADERS
	klfdebug.h
	klfcolorchooser.h
	klfcolorchooser_p.h
	klfpixmapbutton.h
	klfguiutil.h
	klfpathchooser.h
	qtcolortriangle.h
	klfdisplaylabel.h
	klfunitinput.h
	klfsearchbar.h
	klflatexedit.h
	klflatexedit_p.h
	klfrelativefont.h
	klfenumlistwidget.h
	klfflowlayout.h
	klfflowlistwidget.h
	klfflowlistwidget_p.h
	klfsidewidget.h
	klfpobjeditwidget.h
	klfpobjeditwidget_p.h
	klfadvancedconfigeditor.h
	klfadvancedconfigeditor_p.h
	klfitemviewsearchtarget.h
	klfitemviewsearchtarget_p.h
)
#  * all headers
set(klftools_HEADERS
	klfdefs.h
	klfutil.h
	klfsysinfo.h
	klfdatautil.h
	klfdatautil_p.h
	klfconfigbase.h
	klfpobj.h
	klffactory.h
	klfiteratorsearchable.h
	klfsearchbar_p.h
	${klftools_MOCHEADERS}
)
# UI Forms
set(klftools_UIS
	klfcolorchoosewidget.ui
	klfcolordialog.ui
	klfsearchbar.ui
	klfadvancedconfigeditor.ui
)
# Resources
set(klftools_QRCS
	../klftoolsres.qrc
)


if(APPLE)
  set(klftools_SRCS ${klftools_SRCS}
    macosx/klfdrawerdefs.mm
    klfdefs_mac.cpp
    )
elseif(UNIX)
  set(klftools_SRCS ${klftools_SRCS}
    klfdefs_linux.cpp
    )
elseif(WIN32)
  set(klftools_SRCS ${klftools_SRCS}
    klfdefs_win.cpp
    )
endif(APPLE)


KLFInstHeaders(klftools_INSTHEADERS "${klftools_HEADERS}")

# our include directories
include_directories("${CMAKE_CURRENT_BINARY_DIR}" "${CMAKE_CURRENT_SOURCE_DIR}")

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
  target_link_libraries(klftools ${QT_LIBRARIES})
  set_target_properties(klftools PROPERTIES
	    VERSION		${KLF_LIB_VERSION}
	    SOVERSION		${KLF_LIB_VERSION}
  )
  if(KLF_MACOSX_BUNDLES)
    set(klftools_FRAMEWORKS ${KLF_BUNDLE_QT_FRAMEWORKS})
    KLFMMakeFramework(klftools "${klftools_FRAMEWORKS}" "${klftools_INSTHEADERS}")
  endif(KLF_MACOSX_BUNDLES)
endif(KLF_LIBKLFTOOLS_STATIC)

set_target_properties(klftools PROPERTIES
		#	    DEBUG_POSTFIX "${KLF_DEBUG_POSTFIX}"
	    PUBLIC_HEADER "${klftools_INSTHEADERS}"
)

if(APPLE)
  set(KLF_APPKIT "/System/Library/Frameworks/AppKit.framework" CACHE FILEPATH "Path to AppKit framework")
  target_link_libraries(klftools "${KLF_APPKIT}")
endif(APPLE)



# Qt Designer Plugin
# ------------------
if(KLF_BUILD_TOOLSDESPLUGIN)
  include_directories("${QT_QTDESIGNER_INCLUDE_DIR}")
  qt4_wrap_cpp(klftoolsdesplugin_MOC_CPPS  klftools_desplugin.h)
  add_library(klftoolsdesplugin MODULE  klftools_desplugin.cpp ${klftoolsdesplugin_MOC_CPPS})
  set_target_properties(klftoolsdesplugin PROPERTIES
    COMPILE_DEFINITIONS "QT_PLUGIN;QT_NO_DEBUG;QDESIGNER_EXPORT_WIDGETS"
    SKIP_RPATH FALSE  INSTALL_RPATH "${QT_PLUGINS_DIR}/designer"
    COMPILE_FLAGS "-O2 -g"
    LINK_FLAGS "-O2 -g"
    )
  target_link_libraries(klftoolsdesplugin  klftools ${QT_QTCORE_LIBRARY}
    ${QT_QTGUI_LIBRARY} ${QT_QTDESIGNER_LIBRARY} ${QT_QTDESIGNERCOMPONENTS_LIBRARY})
endif(KLF_BUILD_TOOLSDESPLUGIN)

# Install Targets
# ---------------

KLFInstallLibrary(klftools KLF_INSTALL_KLFTOOLS_ "${KLF_INSTALL_LIB_DIR}" "include/klftools")
if(KLF_MACOSX_BUNDLES AND KLF_INSTALL_KLFTOOLS_FRAMEWORK)
  KLFMInstFrameworkUpdateId(
    "${KLF_INSTALL_LIB_DIR}/klftools.framework/Versions/${KLF_LIB_VERSION}/klftools"
  )
endif(KLF_MACOSX_BUNDLES AND KLF_INSTALL_KLFTOOLS_FRAMEWORK)

if(KLF_BUILD_TOOLSDESPLUGIN AND KLF_INSTALL_KLFTOOLSDESPLUGIN)
  install(TARGETS klftoolsdesplugin  LIBRARY DESTINATION "${KLF_INSTALL_DESPLUGIN_DIR}")
endif(KLF_BUILD_TOOLSDESPLUGIN AND KLF_INSTALL_KLFTOOLSDESPLUGIN)
