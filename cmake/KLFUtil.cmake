# ######################################### #
# CMake utility functions for klatexformula #
# ######################################### #
# $Id$
# ######################################### #


#
# Filters a list of C/C++ header files ".h" and keeps only those header files
# that do not match "<headerfile>_p.h" (private headers).
#
# Notes:
#  - for this implementation we expect the <headerfile> to be "[A-Za-z0-9_-]+"
#  - the headers must be specified in one argument (note the quotes in below example)
#
# Example:
#   set(allheaders   header1.h   header1_p.h   otherheader.h)
#   KLFInstHeaders(instheaders "${allheaders}")
#   # instheaders is the list  (header1.h otherheader.h)
#
macro(KLFInstHeaders varInstHeaders varAllHeaders)

  string(REGEX REPLACE "[A-Za-z0-9_-]+_p\\.h" "" ${varInstHeaders} "${varAllHeaders}")

endmacro(KLFInstHeaders)


# internal cache variable
set(_klf_notices "" CACHE INTERNAL "no notices shown via KLFNote().")

if(NOT WIN32)
  set(_klf_path_sep ":")
else(NOT WIN32)
  set(_klf_path_sep ";")
endif(NOT WIN32)
set(KLF_CMAKE_PATH_SEP "${_klf_path_sep}"
  CACHE INTERNAL "PATH entry separator character (':' on unix and ';' on win32)")

#
# Prints a note on the screen in such a way that the user will probably not miss it.
#
# However it does not necessarily indicate an error; no error is raised. Execution can
# proceed to succeed or fail.
#
macro(KLFNote message)
  message("
    *** NOTE ***
    ${message}
")
  if(NOT _klf_notices)
    set(_klf_notices "1" CACHE INTERNAL "have shown a notice with KLFNote().")
  else(NOT _klf_notices)
    math(EXPR klf_num_notices ${_klf_notices}+1)
    set(_klf_notices "${klf_num_notices}" CACHE INTERNAL "number of notices shown with KLFNote().")
  endif(NOT _klf_notices)
endmacro(KLFNote)

macro(KLFNotifyNotices)
  if(_klf_notices)
    if(_klf_notices STREQUAL "1")
      message(STATUS "There was **1 notice message** during the configuration process.")
    else(_klf_notices STREQUAL "1")
      message(STATUS "There were **${_klf_notices} notice messages** during the configuration process.")
    endif(_klf_notices STREQUAL "1")
  endif(_klf_notices)
endmacro(KLFNotifyNotices)


option(KLF_CMAKE_DEBUG "Enable Debug Messages in CMake Scripts" OFF)
mark_as_advanced(KLF_CMAKE_DEBUG)

#
# if KLF_CMAKE_DEBUG is set and TRUE, prints a debugging message.
# otherwise, does nothing.
#
macro(KLFCMakeDebug message)
  if(KLF_CMAKE_DEBUG)
    message("
    --- DEBUG MESSAGE ---
    ${message}
")
  endif(KLF_CMAKE_DEBUG)
endmacro(KLFCMakeDebug)



#
# definitions used in KLFTestCondition().
# This implementation is ****UGLY**** ... but IMHO leads to nicer code in scripts...
#
set(_klf_test_condition_templatefile "${CMAKE_BINARY_DIR}/klf_test_condition.cmake.in")
set(_klf_test_condition_file "${CMAKE_BINARY_DIR}/klf_test_condition.cmake")
# write the template file
file(WRITE "${_klf_test_condition_templatefile}"
"set(_klf_test_condition_result FALSE)
if(@_KLF_TEST_CONDITION_CONDITION@)
  set(_klf_test_condition_result TRUE)
endif(@_KLF_TEST_CONDITION_CONDITION@)
KLFCMakeDebug(\"[in configured template:] @_KLF_TEST_CONDITION_CONDITION@ is \${_klf_test_condition_result}\")
")

#
# I think CMake suffers from not being able to evaluate in-line expressions (though I agree
# not knowing how it would be possible to implement properly), and having to use the
# ugly and heavy if(..) ... else(..) ... endif(..) constructs (using 3 or 5 lines for each
# simple condition test!), so here is a workaround (very uglyly implemented, agreed. suggestions
# for better implementation welcome).
#
# 'condition' is anything that can be put in an if(...) clause.
# sets 'var' to TRUE or FALSE depending on if condition evaluates to TRUE or FALSE.
#
macro(KLFTestCondition var condition)
  file(REMOVE "${_klf_test_condition_file}")
  set(_KLF_TEST_CONDITION_CONDITION "${condition}")
  configure_file("${_klf_test_condition_templatefile}"
		 "${_klf_test_condition_file}"
		 @ONLY)
  set(_klf_test_condition_result )
  include("${_klf_test_condition_file}")
  if(_klf_test_condition_result)
    set(${var} TRUE)
  else(_klf_test_condition_result)
    set(${var} FALSE)
  endif(_klf_test_condition_result)
endmacro(KLFTestCondition var condition)


#
# An implemenation of the ?: operator in C. Example:
#   KLFConditionalSet(menu  "NOT IS_VEGETARIAN"  "Steak with Café de Paris"  "String beans")
# will set menu to either "Steak..." or "String beans" depending on the value of the variable
# IS_VEGETARIAN.
#
# Notes
#  - 'condition' can by any string suitable for usage in an if(...) clause.
#  - 'var' is not set in the cache
#
macro(KLFConditionalSet var condition valueTrue valueFalse)
  KLFTestCondition(_klf_conditiontest "${condition}")
  if(_klf_conditiontest)
    set(${var} "${valueTrue}")
  else(_klf_conditiontest)
    set(${var} "${valueFalse}")
  endif(_klf_conditiontest)
endmacro(KLFConditionalSet)


#
# Detect changes to cache variable 'varname' between two executions of cmake
#
# This function sets the following (local non-cache) variables:
#
#   klf_changed_${varname} :
#      is set to TRUE if the variable was changed, FALSE if not.
#      If klf_first_${varname} is true, this variable is TRUE if varname is defined,
#      (ie. set by the user or by previous call of set(varname ...) ), FALSE otherwise.
#
#   klf_first_${varname} :
#      is set to TRUE for the very first call of this function for that variable, FALSE
#      otherwise ("very first call" referring to first running of CMake calling this function
#      for this given variable for this build directory)
#
#   klf_old_${varname} :
#      the old value of the variable varname in case it was changed, not defined otherwise.
#
# Notes:
#  - Call this function ONCE only per CMake run and per variable, or klf_change|first|old_*
#    will be reset.
#  - this function sets internal cache variables klf_internal_${varname}.
#
macro(KLFGetCMakeVarChanged varname)

  if(NOT DEFINED klf_internal_${varname})

    set(klf_first_${varname} TRUE)
    if(DEFINED ${varname})
      set(klf_changed_${varname} TRUE)
    else(DEFINED ${varname})
      set(klf_changed_${varname} FALSE)
    endif(DEFINED ${varname})

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


#
# Remember current value of 'varname' for next time's call to KLFGetCMakeVarChanged().
#
# If you make changes to 'varname' in your script, after having called KLFGetCMakeVarChanged(),
# then next time you run CMake, then those variables will be detected as changed. To avoid that,
# call this function for those variables that you (might) have changed.
#
# Calling this function does not affect the current values of klf_{changed|first|old}_${varname}.
#
macro(KLFCMakeSetVarChanged varname)

  set(klf_internal_${varname} "${${varname}}" CACHE INTERNAL "Stored previous value of ${varname}")

endmacro(KLFCMakeSetVarChanged)


#
# A shortcut for marking a variable as advanced, on condition. Useful for making specific fine-tuning
# options visible to user when necessary.
#
# Example:
#   include(FindQt4)
#   KLFMarkVarAdvancedIf(QT_QMAKE_EXECUTABLE "NOT QT_QMAKE_EXECUTABLE")
# invites the user to enter a reasonable value for QT_QMAKE_EXECUTABLE by making that variable
# visible in cmake-gui's simple view, while maintaining it as "advanced" if the cmake script
# found Qt4 correctly.
#
# 'condition' is used in a if(...) clause.
#
macro(KLFMarkVarAdvancedIf varname condition)
  KLFTestCondition(_klf_conditiontest ${condition})
  if(_klf_conditiontest)
    mark_as_advanced(FORCE ${varname})
  else(_klf_conditiontest)
    mark_as_advanced(CLEAR ${varname})
  endif(_klf_conditiontest)
endmacro(KLFMarkVarAdvancedIf)


#
# Use this function to declare a boolean option that is a more specific fine-tuning
# setting, depending on a more general-indication (boolean) setting.
#
# For example the option KLF_INSTALL_KLFTOOLS_HEADERS is a fine-tuning of the more
# general KLF_INSTALL_DEVEL option.
#
# This function is a shortcut for calling KLFDeclareCacheVarOptionFollowComplexN(),
# while automatically setting the following options:
#    - cachetype BOOL
#    - updatenotice FALSE
#    - dependency variable "klf_changed_${genericoption}"
#    - calculated value "${${genericoption}}"
#
macro(KLFDeclareCacheVarOptionFollow specificoption genericoption cachestring)

  KLFDeclareCacheVarOptionFollowComplexN(${specificoption}
	BOOL  "${cachestring}"  # cache information
	OFF                     # disable update notice
	"${${genericoption}}"   # the option's recommended ('calculated') value
	${genericoption}        # the dependency variable
  )

endmacro(KLFDeclareCacheVarOptionFollow)


#
# This function declares a cache variable that depends on changes to other variable(s).
#
# Idea: you have a generic setting (eg. LIBRARY_SUFFIX) and a more specific setting
# (eg. FULL_LIBRARY_INSTALL_DIR). Logically, the latter depends on the former, for example
# by setting  FULL_LIBRARY_INSTALL_DIR  to  "lib${LIBRARY_SUFFIX}"
# However, you want to let the user set FULL_LIBRARY_INSTALL_DIR to something else.
#
# Possible Solution: by default, set FULL_LIBRARY_INSTALL_DIR ('specific option') to the
# default value of "lib${LIBRARY_SUFFIX}". Then consider the cases:
# - If the user changes the cache variable LIBRARY_SUFFIX ('dependency variable') without
#   touching FULL_LIBRARY_INSTALL_DIR, then update the latter;
# - If the user changes both cache variables, he wants specific settings, so don't update
#   anything;
# - If the user changes the specific option FULL_LIBRARY_INSTALL_DIR, then again he set
#   a manual value, so don't update anything
#
# This function implements this solution, by making use of the KLFGetCMakeVarChanged()
# macro to detect changes in the cache options.
#
# This function takes care of DECLARING the specific option and getting its changed status
# with KLFGetCMakeVarChanged(). However nothing is done for the dependency variables, and
# this function also assumes that KLFGetCMakeVarChanged() has already been called on them.
#
# If the value of 'specificoption' was updated, then klf_updated_${specificoption} is set
# to TRUE.
#
# When, above, we mean that a variable dependency has "changed", we mean that either
# klf_changed_${var} or klf_updated_${var} is TRUE. The fact that klf_updated_* is also checked
# can be used to make variables depend on variables that themselves depend on other variables.
#
# Arguments
#
#   specificoption   is a cache option (to be declared into cache, and checked its changed status
#                    by this function) that may have to be updated, depending on changes to other
#                    variables
#
#   cachetype        is the type of 'specificoption' (eg. BOOL,STRING,PATH), see cmake's
#                    documentation for set(.. CACHE <type> <docstring>..)
#
#   cachestring      is a short explanation of the option, the <docstring> on the above line here
#
#   updatenotice     a boolean flag indicating this function to notify or not the user of
#                    changes made to 'specificoption' because of a dependency change (with KLFNote())
#
#   calcoptvalue     the value this option should have considering the current values of the
#                    dependency variables (eg. in above example, "lib${LIBRARY_SUFFIX}")
#
#   depvarlist       a (semicolon-separated) list of the names of all the dependency variables.
#
#
macro(KLFDeclareCacheVarOptionFollowComplexN specificoption cachetype cachestring updatenotice
									  calcoptvalue depvarlist)
  # declare option (cache var)
  if(NOT DEFINED ${specificoption})
    KLFCMakeDebug("${specificoption} not defined adding it to cache with value \"${calcoptvalue}\". cache settings type='${cachetype}', cachestring='${cachestring}'")
    set(${specificoption} "${calcoptvalue}" CACHE ${cachetype} "${cachestring}")
  endif(NOT DEFINED ${specificoption})
  # and check if it was changed
  KLFGetCMakeVarChanged(${specificoption})
  
  KLFCMakeDebug("${specificoption} chg:${klf_changed_${specificoption}} val:${${specificoption}}, calcoptvalue=${calcoptvalue}")

  # see if this specific option was changed. If it was, don't bother looking for dependencies,
  # we won't update it anyway.
  if(NOT klf_changed_${specificoption})
    # Similarly, if its value is already the value we would update it to, skip the rest
    #       [note: don't quote "${specificoption}" in condition, it's a variable name]
    if(NOT ${specificoption} STREQUAL "${calcoptvalue}")
      
      # [build debug message]
      set(DCVOFCN_debugstr "Specific option: ${specificoption} (chg: ${klf_changed_${specificoption}}, updt: ${klf_updated_${specificoption}})")
      # the master variable that will be TRUE if (at least) one dependency has been changed
      set(DCVOFCN_deps_changed)
      set(DCVOFCN_what_changed_list)
      # loop on dependency variables
      foreach(DCVOFCNdepvar ${depvarlist})
	set(DCVOFCN_thisdepvarchanged "${klf_changed_${DCVOFCNdepvar}}") # check if this var changed
	set(DCVOFCN_thisdepvarupdated "${klf_updated_${DCVOFCNdepvar}}") # check if this var updated
	# [continue building debug message]
	set(DCVOFCN_debugstr "${DCVOFCN_debugstr}\n\t\tfollowing ${DCVOFCNdepvar} (chg.: ${DCVOFCN_thisdepvarchanged}, updt.: ${DCVOFCN_thisdepvarupdated}).")
	# if we detected a change, then set the master DCVOFCN_deps_changed to TRUE
	if(DCVOFCN_thisdepvarchanged OR DCVOFCN_thisdepvarupdated)
	  # set the master dependency changed variable
	  set(DCVOFCN_deps_changed TRUE)
	  # add to list of changed dependencies, to show to user
	  set(DCVOFCN_what_changed_list ${DCVOFCN_what_changed_list} ${DCVOFCNdepvar})
	endif(DCVOFCN_thisdepvarchanged OR DCVOFCN_thisdepvarupdated)
      endforeach(DCVOFCNdepvar)

      # debug
      KLFCMakeDebug("${DCVOFCN_debugstr}")

      # possibly update the variable
      if(DCVOFCN_deps_changed)
	# dependency option(s) changed, then the specific option must follow
	set(${specificoption} "${calcoptvalue}" CACHE ${cachetype} "${cachestring}" FORCE)
	set(klf_updated_${specificoption} TRUE)
	set(_klf_updatenotice ${updatenotice})
	if(_klf_updatenotice)
	  KLFNote("Updating ${specificoption} to \"${${specificoption}}\" following changes to ${DCVOFCN_what_changed_list}.\n    This is the default behavior. Pass -D${specificoption}=<value> to override.")
	else(_klf_updatenotice)
	  KLFCMakeDebug("Updated ${specificoption} to \"${${specificoption}}\" following chg to ${DCVOFCN_what_changed_list}. updatenotice=${updatenotice}")
	endif(_klf_updatenotice)
      endif(DCVOFCN_deps_changed)
    endif(NOT ${specificoption} STREQUAL "${calcoptvalue}")
  endif(NOT klf_changed_${specificoption})

  # make sure we remember the (possibly) new value of 'specificoption'
  KLFCMakeSetVarChanged(${specificoption})

endmacro(KLFDeclareCacheVarOptionFollowComplexN)


#
# Shortcut for calling KLFDeclareCacheVarOptionFollowComplexN()  for one dependency variable
# 'depvar1'.
#
# [Note that this function is superfluous, it is here for historical reasons when
#  KLFDeclareCacheVarOptionFollowComplexN accepted some different arguments]
#
macro(KLFDeclareCacheVarOptionFollowComplex1 specificoption cachetype cachestring updatenotice
									      calcoptvalue depvar1)

  KLFDeclareCacheVarOptionFollowComplexN("${specificoption}"
	${cachetype} "${cachestring}"  # cache info
	${updatenotice}                # whether to show update notice or not
	"${calcoptvalue}"              # calculated default value
	"${depvar1}"                   # dependency list (one element)
  )

endmacro(KLFDeclareCacheVarOptionFollowComplex1)

#
# Calls KLFDeclareCacheVarOptionFollowComplexN()  for two dependency variables 'depvar1' and 'depvar2'.
#
macro(KLFDeclareCacheVarOptionFollowComplex2 specificoption cachetype cachestring updatenotice
								      calcoptvalue depvar1 depvar2)

  KLFDeclareCacheVarOptionFollowComplexN("${specificoption}"
	${cachetype} "${cachestring}"  # cache info
	${updatenotice}                # whether to show update notice or not
	"${calcoptvalue}"              # calculated default value
	"${depvar1};${depvar2}"        # dependency list
  )

endmacro(KLFDeclareCacheVarOptionFollowComplex2)


#
# Ensures that an option 'specificoption' is compatible with a condition 'condition'.
#
# For example, you might have a variable  BUILD_API_DOCUMENTATION  that needs doxygen
# which has been found, or not found, with find_program(DOXYGEN_PATH ...) . Obviously
# there is a condition on BUILD_API_DOCUMENTATION ('specificoption') that depends on
# whether DOXYGEN_PATH is valid ('condition').
#
# However in this example you cannot use KLFDeclareCacheVarOptionCondition, since you
# do NOT want to let the user set BUILD_API_DOCUMENTATION unless a valid DOXYGEN_PATH
# has been set (manually specified or automatically)
#
# This function sets the cache variable 'specificoption' into cache with value
# "${defaultvalue}", with type 'cachetype' and docstring 'cachestring' with a
# call to instruction set(.. CACHE ...).
#
# If condition is met, then nothing is done, and 'specificoption' is not modified.
#
# If condition is NOT met and the 'specificoption' already has value "${forcedvalue}",
# then nothing is done.
#
# Otherwise if condition is NOT met, then the 'specificoption' is modified to the value
# "${forcedvalue}" by a call to set(... CACHE ... FORCE) and a notice is emitted with
# KLFNote(). In this case, klf_changed_*, klf_first_* reflect the status of the variable
# _before_ the change, (you should consider klf_changed_* as meaning "changed by the
# user") and additionally, klf_updated_${specificoption} is set to TRUE.
#
# In any case, the klf_{changed|first|old}_* variables are initialized by a call to
# KLFGetCMakeVarChanged().
#
# This function makes use of KLFGetCMakeVarChanged() tool. KLFGetCMakeVarChanged() is
# called in this function on the 'specificoption', even though the result is not used:
# it calls KLFCMakeSetVarChanged() if its value was updated, and for klf_changed_VAR
# etc to be set properly, KLFGetCMakeVarChanged() has to be called before. Additionally,
# this follows the spirit of KLFDeclareCacheVarOptionFollow*() which take care of declaring
# their specific option and taking care of its changes.
#
#
macro(KLFDeclareCacheVarOptionCondition specificoption cachetype cachestring updatenoticestring
								condition forcedvalue defaultvalue)

  # declare option (cache var)
  set(${specificoption} "${defaultvalue}" CACHE ${cachetype} "${cachestring}")

  # and check if it was changed
  KLFGetCMakeVarChanged(${specificoption})
  
  KLFTestCondition(_klf_conditiontest "${condition}")
  if(_klf_conditiontest)
    
    # all is right
    KLFCMakeDebug("Specific option: ${specificoption} (chg: ${klf_changed_${specificoption}}, value: '${${specificoption}}') is compatible with condition ${condition} (=${_klf_conditiontest}).")
    
  else(_klf_conditiontest)

    # condition is not met
    # force 'specificoption' to 'forcedvalue' (unless of course it already has that value!)
    if(NOT ${specificoption} STREQUAL "${forcedvalue}")

      set(${specificoption} "${forcedvalue}" CACHE ${cachetype} "${cachestring}" FORCE)
      KLFNote("${updatenoticestring}")
      set(klf_updated_${specificoption} TRUE)

      KLFCMakeDebug("Specific option: ${specificoption} (chg: ${klf_changed_${specificoption}}, value: '${${specificoption}}') is not compatible with condition ${condition} (=${_klf_conditiontest}) and is being forced to '${forcedvalue}'.")

    else(NOT ${specificoption} STREQUAL "${forcedvalue}")

      KLFCMakeDebug("Specific option: ${specificoption} (chg: ${klf_changed_${specificoption}}') is constrained by condition ${condition} (=${_klf_conditiontest}) to its current value '${forcedvalue}'.")

    endif(NOT ${specificoption} STREQUAL "${forcedvalue}")

  endif(_klf_conditiontest)
  
  KLFCMakeSetVarChanged(${specificoption})

endmacro(KLFDeclareCacheVarOptionCondition)



macro(KLFMakeAbsInstallPath abs_dir_var dir_var)
  if(IS_ABSOLUTE "${${dir_var}}")
    set(${abs_dir_var} "${${dir_var}}")
  else(IS_ABSOLUTE "${${dir_var}}")
    set(${abs_dir_var} "${CMAKE_INSTALL_PREFIX}/${${dir_var}}")
  endif(IS_ABSOLUTE "${${dir_var}}")
endmacro(KLFMakeAbsInstallPath)

#
# Returns first element in list 'list' (variable name), but does not complain if list is empty.
# If that is the case then 'var' is set to an empty string.
#
macro(KLFListHeadSafe list var)
  if(${list})
    list(GET ${list} 0 ${var})
  else(${list})
    set(${var} "")
  endif(${list})
endmacro(KLFListHeadSafe)

#
# like  file(RELATIVE_PATH )  but not quite as stupid as not to know that '.' entries in path have
# no directory changing effect.
#
macro(KLFRelativePath var from_path to_path)

  KLFCMakeDebug("var=${var} from_path=${from_path} to_path=${to_path}")

  file(TO_CMAKE_PATH "${from_path}" klfRP_from_path)
  file(TO_CMAKE_PATH "${to_path}" klfRP_to_path)

  # Convert paths to lists of entries
  string(REPLACE "/" ";" klfRP_from_path_split "${klfRP_from_path}")
  string(REPLACE "/" ";" klfRP_to_path_split "${klfRP_to_path}")
  # remove any '.' and '' entries from both lists
  list(REMOVE_ITEM klfRP_from_path_split "." "")
  list(REMOVE_ITEM klfRP_to_path_split "." "")
  KLFCMakeDebug("Lists: ${klfRP_from_path_split} -> ${klfRP_to_path_split}")
  # now remove common beginning items from both lists
  # this is equivalent in thought to start at root dir, and chdir'ing to the next directory
  # as long as both paths are in that same directory. We stop once both paths separate and are no
  # longer in the same directory
  KLFListHeadSafe(klfRP_from_path_split klfRP_from_first)
  KLFListHeadSafe(klfRP_to_path_split klfRP_to_first)
  while(klfRP_from_path_split AND klfRP_to_path_split AND klfRP_from_first STREQUAL "${klfRP_to_first}")
    KLFCMakeDebug("Removing common item ${klfRP_from_first}==${klfRP_to_first}")
    # remove first item from both lists
    list(REMOVE_AT klfRP_from_path_split 0)
    list(REMOVE_AT klfRP_to_path_split 0)
    # and get the new first elements
  KLFListHeadSafe(klfRP_from_path_split klfRP_from_first)
  KLFListHeadSafe(klfRP_to_path_split klfRP_to_first)
    KLFCMakeDebug("New first items are ${klfRP_from_first} and ${klfRP_to_first}")
  endwhile(klfRP_from_path_split AND klfRP_to_path_split AND klfRP_from_first STREQUAL "${klfRP_to_first}")
  # Prepare the relative string.
  set(klfRP_relpath "")
  # Start by adding '..' the needed number of times to get to the first common directory (beginning of list)
  # - This is effectively done by replacing the directory names in from_path by '..'s  ...
  string(REGEX REPLACE "[^;]+" ".." klfRP_ddots "${klfRP_from_path_split}")
  # - ... and replacing the ';' list element separator by directory separators '/'
  string(REPLACE ";" "/" klfRP_relpath "${klfRP_ddots}")
  # Now prepare the path to the other location
  string(REPLACE ";" "/" klfRP_otherpath "${klfRP_to_path_split}")

  # And glue together the pieces
  # - add a '/' if needed
  if(klfRP_relpath AND klfRP_otherpath)
    set(klfRP_relpath "${klfRP_relpath}/")
  endif(klfRP_relpath AND klfRP_otherpath)
  # - and concatenate both path segments
  set(klfRP_relpath "${klfRP_relpath}${klfRP_otherpath}")

  # make sure we are not returning an empty string (in case the user concatenates the result with a '/',
  # which would make it an absolute path... not wanted)
  if(NOT klfRP_relpath)
    set(klfRP_relpath ".")
  endif(NOT klfRP_relpath)
  
  # and save the result into the target variable
  set(${var} "${klfRP_relpath}")

  KLFCMakeDebug("Result is : ${klfRP_relpath}, saved in ${var}")

endmacro(KLFRelativePath)


macro(KLFGetTargetLocation var target)
  if(CMAKE_BUILD_TYPE)
    get_target_property(${var} ${target} ${CMAKE_BUILD_TYPE}_LOCATION)
  else(CMAKE_BUILD_TYPE)
    get_target_property(${var} ${target} LOCATION)
  endif(CMAKE_BUILD_TYPE)
endmacro(KLFGetTargetLocation)

macro(KLFGetTargetFileName var target)
  KLFGetTargetLocation(${var} ${target})
  string(REGEX REPLACE "^.*/([^/]+)$" "\\1" ${var} "${${var}}")
endmacro(KLFGetTargetFileName)


macro(KLFAppendToTargetProp target property addtext)
  if(addtext)
    get_target_property(val ${target} ${property})
    if(val)
      set(val "${lflags} ")
    else(val)
      set(val "")
    endif(val)
    set_target_properties(${target} PROPERTIES
      ${property} "${val}${addtext}"
      )
  endif(addtext)
endmacro(KLFAppendToTargetProp)

macro(KLFNoShlibUndefined target)

  set(add_link_flags "")
  if(APPLE)
    set(add_link_flags "-undefined dynamic_lookup")
  elseif(WIN32)
    # DLL's don't allow undefined references (!!)
    set(add_link_flags "")
  else()
    set(add_link_flags "-Wl,--allow-shlib-undefined")
  endif(APPLE)

  KLFAppendToTargetProp(target LINK_FLAGS "${add_link_flags}")

endmacro(KLFNoShlibUndefined)
