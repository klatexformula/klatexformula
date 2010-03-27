# ######################################### #
# CMake utility functions for klatexformula #
# ######################################### #
# $Id: CMakeLists.txt 340 2010-03-26 19:24:15Z philippe $
# ######################################### #

macro(KLFInstHeaders varInstHeaders varAllHeaders)

string(REGEX REPLACE ";[a-zA-Z0-9_-]+_p\\.h" "" ${varInstHeaders} ${varAllHeaders})


endmacro()