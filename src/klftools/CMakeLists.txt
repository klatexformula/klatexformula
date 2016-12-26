# ################################################# #
# CMake project file for klatexformula/src/klftools #
# ################################################# #
# $Id$
# ################################################# #


if(KLF_BUILD_TOOLS)

  find_package(Qt5Xml REQUIRED)

  # Sources
  set(klftools_SRCS
    klfdefs.cpp
    klfdebug.cpp
    klfcolorchooser.cpp
    klfpobj.cpp
    klfutil.cpp
    klfsysinfo.cpp
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
    klfprogerr.cpp
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
    klfprogerr.h
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
    klfprogerr.ui
    )
  # Resources
  set(klftools_QRCS
    ../klftoolsres.qrc
    )

  if(APPLE)
    set(klftools_SRCS ${klftools_SRCS}
      macosx/klfdrawerdefs.mm
      klfdefs_mac.mm
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

  if (KLF_LIBKLFTOOLS_STATIC)
    add_library(klftools STATIC ${klftools_SRCS})
  else()
    add_library(klftools SHARED ${klftools_SRCS})
  endif()
  if (KLF_MACOSX_BUNDLES)
    set_target_properties(klftools PROPERTIES
      FRAMEWORK	TRUE
      )
  endif()
    
  target_link_libraries(klftools PUBLIC Qt5::Widgets Qt5::Gui Qt5::Core Qt5::Xml)
  if (APPLE)
    target_link_libraries(klftools PUBLIC "-framework IOKit")
    target_include_directories(klftools PRIVATE /System/Library/Frameworks/IOKit.framework/Headers)
  endif()

  # determine which headers are public and should be installed, and set the target property
  KLFInstHeaders(klftools_INSTHEADERS "${klftools_HEADERS}")
  set_target_properties(klftools PROPERTIES
    VERSION		${KLF_LIB_VERSION}
    SOVERSION		${KLF_LIB_VERSION}
    PUBLIC_HEADER       "${klftools_INSTHEADERS}"
    )

  # our include directories
  target_include_directories(klftools PRIVATE "${CMAKE_CURRENT_BINARY_DIR}" "${CMAKE_CURRENT_SOURCE_DIR}")

  # Uis, Mocs, Qrcs
  qt5_wrap_ui(klftools_UIS_H ${klftools_UIS})
  qt5_wrap_cpp(klftools_MOC_CPPS ${klftools_MOCHEADERS})
  qt5_add_resources(klftools_QRC_CPPS ${klftools_QRCS})
  target_sources(klftools PRIVATE ${klftools_SRCS} ${klftools_QRC_CPPS} ${klftools_UIS_H} ${klftools_MOC_CPPS})

  if(APPLE)
    set(KLF_APPKIT "-framework AppKit" CACHE FILEPATH "Link to AppKit framework (path to library or link flags)")
    mark_as_advanced(KLF_APPKIT)
    target_link_libraries(klftools PRIVATE "${KLF_APPKIT}")
  endif(APPLE)


  # Qt Designer Plugin
  # ------------------
  if(KLF_BUILD_TOOLSDESPLUGIN)
    find_package(Qt5Designer REQUIRED)

    qt5_wrap_cpp(klftoolsdesplugin_MOC_CPPS  klftools_desplugin.h)
    add_library(klftoolsdesplugin MODULE  klftools_desplugin.cpp ${klftoolsdesplugin_MOC_CPPS})
    target_include_directories(klftoolsdesplugin PUBLIC "${QT_QTDESIGNER_INCLUDE_DIR}")
    set_target_properties(klftoolsdesplugin PROPERTIES
      COMPILE_DEFINITIONS "QT_PLUGIN;QT_NO_DEBUG;QDESIGNER_EXPORT_WIDGETS"
      SKIP_RPATH FALSE
      )
    target_link_libraries(klftoolsdesplugin  klftools Qt5::Core Qt5::Gui Qt5::Widgets
      Qt5::Designer
      #Qt5::DesignerComponents
      )
  endif()


  # Install Targets
  # ---------------

  KLFInstallLibrary(klftools KLF_INSTALL_KLFTOOLS_ "${KLF_INSTALL_LIB_DIR}" "include/klftools")
  # No support for fixing up frameworks installed by "make install".
  #
  #if(KLF_MACOSX_BUNDLES AND KLF_INSTALL_KLFTOOLS_FRAMEWORK)
  #  KLFMInstFrameworkUpdateId(
  #    "${KLF_INSTALL_LIB_DIR}/klftools.framework/Versions/${KLF_LIB_VERSION}/klftools"
  #    )
  #endif()

  if(KLF_BUILD_TOOLSDESPLUGIN AND KLF_INSTALL_KLFTOOLSDESPLUGIN)
    install(TARGETS klftoolsdesplugin  LIBRARY DESTINATION "${KLF_INSTALL_DESPLUGIN_DIR}")
  endif()


endif(KLF_BUILD_TOOLS)
