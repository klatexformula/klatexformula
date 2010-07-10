# ######################################### #
# CMake utility functions for klatexformula #
# ######################################### #
# $Id: CMakeLists.txt 340 2010-03-26 19:24:15Z philippe $
# ######################################### #

macro(KLFInstHeaders varInstHeaders varAllHeaders)
#	message("headers are ${varAllHeaders}")
  string(REGEX REPLACE "[A-Za-z0-9_-]+_p\\.h" "" ${varInstHeaders} "${varAllHeaders}")
#	message("install headers are ${${varInstHeaders}}")
endmacro(KLFInstHeaders)

macro(KLFNote message)
message("
    *** NOTE ***
    ${message}
")
endmacro(KLFNote)

# Detect changes to cache variable varname.
#
# This function sets the (local non-cache) variables
#   klf_changed_${varname} : TRUE if the variable was changed, FALSE if not.
#      If klf_first_${varname} is true, this variable is TRUE if varname is defined,
#      (ie. set by the user or by previous call of set(varname ...) ), FALSE otherwise.
#   klf_first_${varname} : TRUE for the very first call of this function for that
#      variable, FALSE otherwise
#   klf_old_${varname} : the old value of the variable varname in case it was changed,
#      not defined otherwise.
#
# Note: Call this function ONCE only per CMake run per variable !
#
macro(KLFGetCMakeVarChanged varname)
  if(NOT DEFINED klf_internal_${varname})

    set(klf_first_${varname} TRUE)
    if(DEFINED varname)
      set(klf_changed_${varname} TRUE)
    else(DEFINED varname)
      set(klf_changed_${varname} FALSE)
    endif(DEFINED varname)

  else(NOT DEFINED klf_internal_${varname})

    set(klf_first_${varname} FALSE)

    if(klf_internal_${varname} STREQUAL ${varname})
      # variable NOT changed
      set(klf_changed_${varname} FALSE)
    else(klf_internal_${varname} STREQUAL ${varname})
      set(klf_changed_${varname} TRUE)
      set(klf_old_${varname} "${klf_internal_${varname}}")
    endif(klf_internal_${varname} STREQUAL ${varname})

  endif(NOT DEFINED klf_internal_${varname})

  set(klf_internal_${varname} "${${varname}}" CACHE INTERNAL "Stored previous value of ${varname}")
endmacro(KLFGetCMakeVarChanged)

# Remember the current value of varname to be used with KLFGetCMakeVarChanged(). Call this
# function after setting manually a cache variable in the code.
#
macro(KLFCMakeSetVarChanged varname)
  set(klf_internal_${varname} "${${varname}}" CACHE INTERNAL "Stored previous value of ${varname}")
endmacro(KLFCMakeSetVarChanged)

macro(KLFMarkVarAdvancedIf varname condition)
  if(condition)
    mark_as_advanced(FORCE varname)
  else(condition)
    mark_as_advanced(CLEAR varname)
  endif(condition)
endmacro(KLFMarkVarAdvancedIf)
