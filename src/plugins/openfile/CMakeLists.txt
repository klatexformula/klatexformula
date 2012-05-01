# ######################################################### #
# CMake project file for klatexformula/src/plugins/openfile #
# ######################################################### #
# $Id: CMakeLists.txt 733 2011-09-16 15:41:13Z phfaist $
# ######################################################### #

set(openfile_SRCS
  openfile.cpp
)
set(openfile_HEADERS
  openfile.h
  openbufferslistwidget.h
  openbuffer.h
)
set(openfile_QRCS
  openfiledata.qrc
)
set(openfile_UIS
  openbufferswidget.ui
)

qt4_wrap_cpp(openfile_MOC_CPPS ${openfile_HEADERS})
qt4_wrap_ui(openfile_UIS_H ${openfile_UIS})
qt4_add_resources(openfile_QRC_CPPS ${openfile_QRCS})

add_library(openfile MODULE ${openfile_SRCS} ${openfile_MOC_CPPS} ${openfile_UIS_H} ${openfile_QRC_CPPS})

KLFPluginUndefOrLink(openfile  klftools klfbackend klfapp)
