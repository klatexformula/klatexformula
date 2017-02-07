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
    klfsidewidget_p.h
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

  # determine which headers are public and should be installed, and set the target property
  KLFInstHeaders(klftools_INSTHEADERS "${klftools_HEADERS}" "${CMAKE_CURRENT_SOURCE_DIR}")
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

  # window system specific code
  if(KLF_WS STREQUAL "mac")
    target_sources(klftools PRIVATE macosx/klfdrawerdefs.mm)
  endif()
  # OS specific code (not window system specific):
  if(APPLE)
    target_sources(klftools PRIVATE
      klfdefs_mac.mm
      )
    target_link_libraries(klftools PUBLIC "-framework IOKit")
    target_include_directories(klftools PRIVATE /System/Library/Frameworks/IOKit.framework/Headers)
    set(KLF_APPKIT "-framework AppKit" CACHE FILEPATH "Link to AppKit framework (path to library or link flags)")
    mark_as_advanced(KLF_APPKIT)
    target_link_libraries(klftools PRIVATE "${KLF_APPKIT}")
  elseif(CMAKE_SYSTEM_NAME STREQUAL "Linux")
    target_sources(klftools PRIVATE
      klfdefs_linux.cpp
      )
  elseif(WIN32)
    target_sources(klftools PRIVATE
      klfdefs_win.cpp
      )
  else()
    # no platform-specific definitions
  endif()


  # Qt Designer Plugin
  # ------------------
  if(KLF_BUILD_TOOLSDESPLUGIN)
    find_package(Qt5Designer REQUIRED)
    find_package(Qt5UiPlugin REQUIRED)

    add_library(klftoolsdesplugin MODULE  klftools_desplugin.cpp klftools_desplugin_collection.cpp)
    target_include_directories(klftoolsdesplugin PUBLIC ${Qt5Designer_INCLUDE_DIRS} ".")
    set_target_properties(klftoolsdesplugin PROPERTIES
      SKIP_RPATH FALSE
      AUTOMOC 1
      )
    target_link_libraries(klftoolsdesplugin  klftools Qt5::Core Qt5::Gui Qt5::Widgets
      Qt5::Designer Qt5::UiPlugin
      )
  endif()


  # Install Targets
  # ---------------

  KLFInstallLibrary(klftools KLF_INSTALL_KLFTOOLS_ "klftools")
  # No support for fixing up frameworks installed by "make install".

  if(KLF_BUILD_TOOLSDESPLUGIN AND KLF_INSTALL_KLFTOOLSDESPLUGIN)
    install(TARGETS klftoolsdesplugin  LIBRARY DESTINATION "${KLF_INSTALL_DESPLUGIN_DIR}")
  endif()


endif(KLF_BUILD_TOOLS)
