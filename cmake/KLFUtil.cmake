# ######################################### #
# CMake utility functions for klatexformula #
# ######################################### #
# $Id$
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


option(KLF_CMAKE_DEBUG "Enable Debug Messages in CMake Scripts" OFF)
mark_as_advanced(KLF_CMAKE_DEBUG)

macro(KLFCMakeDebug message)
  # To include debugging messages, define KLF_CMAKE_DEBUG to ON
  if(KLF_CMAKE_DEBUG)
    message("
    --- DEBUG MESSAGE ---
    ${message}
")
  endif(KLF_CMAKE_DEBUG)
endmacro(KLFCMakeDebug)



macro(KLFConditionalSet var condition valueTrue valueFalse)
  if(${condition})
    set(${var} "${valueTrue}")
  else(${condition})
    set(${var} "${valueFalse}")
  endif(${condition})
endmacro(KLFConditionalSet)


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

    if(klf_internal_${varname} STREQUAL "${${varname}}")
      KLFCMakeDebug("VARIABLE ${varname} NOT CHANGED (value='${${varname}}')")
      # variable NOT changed
      set(klf_changed_${varname} FALSE)
    else(klf_internal_${varname} STREQUAL "${${varname}}")
      KLFCMakeDebug("VARIABLE ${varname} CHANGED from '${klf_internal_${varname}}' to '${${varname}}'")
      set(klf_changed_${varname} TRUE)
      set(klf_old_${varname} "${klf_internal_${varname}}")
    endif(klf_internal_${varname} STREQUAL "${${varname}}")

  endif(NOT DEFINED klf_internal_${varname})

  set(klf_internal_${varname} "${${varname}}" CACHE INTERNAL "Stored previous value of ${varname}")
endmacro(KLFGetCMakeVarChanged)

# Remember the current value of varname to be used with KLFGetCMakeVarChanged(). Call this
# function after setting manually a cache variable in the code. Does not touch the current
# values of klf_{changed|first}_${varname}
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

# This function declares a cache variable that should change its value to that of genericoption
# if the latter is changed when the first is not touched.
# eg. KLF_INSTALL_BACKEND_HEADERS follows KLF_INSTALL_DEVEL
# Notes:
#  * This function calls KLFGetCMakeVarChanged() on the specific variable
#  * This function does NOT call KLFGetCMakeVarChanged() on the generic variable
macro(KLFDeclareCacheVarOptionFollow specificoption genericoption cachestring)
  KLFDeclareCacheVarOptionFollowComplexN("${specificoption}" "${cachetype}" "${cachestring}" "${updatenotice}" "${${genericoption}}" "${genericoption}" "klf_changed_${genericoption}")
endmacro(KLFDeclareCacheVarOptionFollow)

#  * if updatenotice is TRUE then a KLFNote() is emitted when this value is changed not for the
#    very first time
macro(KLFDeclareCacheVarOptionFollowComplexN specificoption cachetype cachestring updatenotice calcoptvalue depvarcommalist depvarchangedorlist)
  # declare option (cache var)
  if(NOT DEFINED ${specificoption})
    set(${specificoption} "${calcoptvalue}" CACHE ${cachetype} "${cachestring}")
  endif(NOT DEFINED ${specificoption})
  # and check if it was changed
  KLFGetCMakeVarChanged(${specificoption})
  KLFCMakeDebug("Specific option: ${specificoption} (chg: ${klf_changed_${specificoption}}) following ${depvarcommalist} (dep condition: ${depvarchangedorlist}).")
  if(${depvarchangedorlist})
    if(NOT klf_changed_${specificoption} AND NOT ${specificoption} STREQUAL "${calcoptvalue}")
      # dependency option(s) changed, then a non-changed specific option must follow
      set(${specificoption} "${calcoptvalue}" CACHE ${cachetype} "${cachestring}" FORCE)
      if(updatenotice)
	KLFNote("Updating ${specificoption} to \"${${specificoption}}\" following changes to ${depvarcommalist}.")
      else(updatenotice)
	KLFCMakeDebug("Updated ${specificoption} to \"${${specificoption}}\" following ${depvarcommalist}.")
      endif(updatenotice)
    endif(NOT klf_changed_${specificoption} AND NOT ${specificoption} STREQUAL "${calcoptvalue}")
  endif(${depvarchangedorlist})
  KLFCMakeSetVarChanged(${specificoption})
endmacro(KLFDeclareCacheVarOptionFollowComplexN)

#
# Calls KLFDeclareCacheVarOptionFollowComplexN()  for one dependency variable 'depvar1'.
#
macro(KLFDeclareCacheVarOptionFollowComplex1 specificoption cachetype cachestring updatenotice calcoptvalue depvar1)
  KLFDeclareCacheVarOptionFollowComplexN("${specificoption}" "${cachetype}" "${cachestring}" "${updatenotice}" "${calcoptvalue}" "${depvar1}" "klf_changed_${depvar1}")
endmacro(KLFDeclareCacheVarOptionFollowComplex1)

#
# Calls KLFDeclareCacheVarOptionFollowComplexN()  for two dependency variables 'depvar1' and 'depvar2'.
#
macro(KLFDeclareCacheVarOptionFollowComplex2 specificoption cachetype cachestring updatenotice calcoptvalue depvar1 depvar2)
  KLFDeclareCacheVarOptionFollowComplexN("${specificoption}" "${cachetype}" "${cachestring}" "${updatenotice}" "${calcoptvalue}" "${depvar1},${depvar2}" "klf_changed_${depvar1} OR klf_changed_${depvar2}")
endmacro(KLFDeclareCacheVarOptionFollowComplex2)

# Thanks to
#   http://www.cmake.org/pipermail/cmake/2009-February/027014.html
MACRO(KLFToday RESULT)
    IF (WIN32)
        EXECUTE_PROCESS(COMMAND "date" "/T" OUTPUT_VARIABLE ${RESULT})
        string(REGEX REPLACE "(..)/(..)/..(..).*" "\\3\\2\\1" ${RESULT} ${${RESULT}})
    ELSEIF(UNIX)
        EXECUTE_PROCESS(COMMAND "date" "+%d/%m/%Y" OUTPUT_VARIABLE ${RESULT})
        string(REGEX REPLACE "(..)/(..)/..(..).*" "\\3\\2\\1" ${RESULT} ${${RESULT}})
    ELSE (WIN32)
        KLFNote("Warning: date not implemented")
        SET(${RESULT} 000000)
    ENDIF (WIN32)
ENDMACRO(KLFToday)





