# ######################################################### #
# CMake project file for klatexformula/src/plugins/buffers #
# ######################################################### #
# $Id: CMakeLists.txt 733 2011-09-16 15:41:13Z phfaist $
# ######################################################### #

set(buffers_SRCS
  buffers.cpp
)
set(buffers_HEADERS
  buffers.h
  openbufferslistwidget.h
  openbuffer.h
)
set(buffers_QRCS
  buffersdata.qrc
)
set(buffers_UIS
  openbufferswidget.ui
)

qt4_wrap_cpp(buffers_MOC_CPPS ${buffers_HEADERS})
qt4_wrap_ui(buffers_UIS_H ${buffers_UIS})
qt4_add_resources(buffers_QRC_CPPS ${buffers_QRCS})

add_library(buffers MODULE ${buffers_SRCS} ${buffers_MOC_CPPS} ${buffers_UIS_H} ${buffers_QRC_CPPS})

KLFPluginUndefOrLink(buffers  klftools klfbackend klfapp)
KLFInstallPlugin(buffers)
