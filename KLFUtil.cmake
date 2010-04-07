# ######################################### #
# CMake utility functions for klatexformula #
# ######################################### #
# $Id: CMakeLists.txt 340 2010-03-26 19:24:15Z philippe $
# ######################################### #

macro(KLFInstHeaders varInstHeaders varAllHeaders)
#	message("headers are ${varAllHeaders}")
  string(REGEX REPLACE "[A-Za-z0-9_-]+_p\\.h" "" ${varInstHeaders} "${varAllHeaders}")
#	message("install headers are ${${varInstHeaders}}")
endmacro()
