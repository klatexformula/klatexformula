# ############################################################ #
# CMake project file for klatexformula/src/plugins/systrayicon #
# ############################################################ #
# $Id$
# ############################################################ #

set(systrayicon_SRCS
  systrayicon.cpp
)
set(systrayicon_HEADERS
  systrayicon.h
)
set(systrayicon_UIS
  systrayiconconfigwidget.ui
  systraymainiconifybuttons.ui
)

qt4_wrap_cpp(systrayicon_MOC_CPPS ${systrayicon_HEADERS})
qt4_wrap_ui(systrayicon_UIS_H ${systrayicon_UIS})
add_library(systrayicon MODULE ${systrayicon_SRCS} ${systrayicon_MOC_CPPS} ${systrayicon_UIS_H})
target_link_libraries(systrayicon ${QT_LIBRARIES})
target_link_libraries(systrayicon ${QT_LIBRARIES})
if(NOT KLF_LIBKLFTOOLS_STATIC)
  target_link_libraries(systrayicon klftools)
endif(NOT KLF_LIBKLFTOOLS_STATIC)
if(NOT KLF_LIBKLFAPP_STATIC)
  target_link_libraries(systrayicon klfapp)
endif(NOT KLF_LIBKLFAPP_STATIC)
