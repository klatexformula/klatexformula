# ##################################################### #
# CMake project file for klatexformula/src/plugins/skin #
# ##################################################### #
# $Id$
# ##################################################### #

set(skin_SRCS
  skin.cpp
)
set(skin_HEADERS
  skin.h
)
set(skin_QRCS
  skindata.qrc
)
set(skin_UIS
  skinconfigwidget.ui
)

qt4_wrap_cpp(skin_MOC_CPPS ${skin_HEADERS})
qt4_wrap_ui(skin_UIS_H ${skin_UIS})
qt4_add_resources(skin_QRC_CPPS ${skin_QRCS})

add_library(skin MODULE ${skin_SRCS} ${skin_MOC_CPPS} ${skin_UIS_H} ${skin_QRC_CPPS})

KLFPluginUndefOrLink(skin  klftools klfbackend klfapp)
KLFInstallPlugin(skin)
