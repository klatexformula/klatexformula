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
macro(KLFInstHeaders varInstHeaders varAllHeaders sourceDir)

  set(${varInstHeaders} )
  foreach(header ${varAllHeaders})
    if (header MATCHES "^[A-Za-z0-9_-]+_p\\.h$")
      # private -- skip header
    else()
      set(${varInstHeaders} ${${varInstHeaders}} "${sourceDir}/${header}")
    endif()
  endforeach()
      
  #  string(REGEX REPLACE "[A-Za-z0-9_-]+_p\\.h" "" ${varInstHeaders} "${varAllHeaders}")

endmacro(KLFInstHeaders)


# internal cache variable
set(_klf_notices "" CACHE INTERNAL "no notices shown via KLFNote().")

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
# An implemenation of the ?: operator in C. Example:
#   KLFSetConditional(menu "Steak with Café de Paris"  "String beans" DEPENDING_IF  NOT IS_VEGETARIAN)
# will set menu to either "Steak..." or "String beans" depending on the value of the variable
# IS_VEGETARIAN.
#
# Notes
#  - 'condition' can by any string suitable for usage in an if(...) clause.
#  - 'var' is not set in the cache
#
macro(KLFSetConditional varname val1 val2 DEPENDING_IF)
  if(NOT DEPENDING_IF STREQUAL "DEPENDING_IF")
    message(FATAL_ERROR "SetIfCondition: synatax: SetIfCondition(varname val1 val2 DEPENDING_IF <condition>)")
  endif()
  if(${ARGN})
    set(${varname} ${val1})
  else()
    set(${varname} ${val2})
  endif()
endmacro()



#
#
#
macro(KLFMakeAbsInstallPath abs_dir_var dir_var)
  if(IS_ABSOLUTE "${${dir_var}}")
    set(${abs_dir_var} "${${dir_var}}")
  else()
    set(${abs_dir_var} "${CMAKE_INSTALL_PREFIX}/${${dir_var}}")
  endif()
  KLFCMakeDebug("${abs_dir_var} is ${${abs_dir_var}}; ${dir_var} is ${${dir_var}}")
endmacro()

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



macro(KLFSetIfNotDefined var value)
  if(NOT DEFINED ${var})
    KLFCMakeDebug("Setting ${var} to default value ${value}")
    set(${var} ${value})
  else()
    KLFCMakeDebug("${var} is already defined as ${${var}}")
  endif()
endmacro()




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
  
  KLFCMakeDebug("Target is ${target}")
  KLFCMakeDebug("Property is ${property}")
  KLFCMakeDebug("addtext is ${addtext}")

  if(NOT "${addtext}" STREQUAL "")
    get_target_property(val "${target}" "${property}")
    KLFCMakeDebug("Value is ${val}")
    if(val)
      set(val "${val} ")
    else(val)
      set(val "")
    endif(val)
    set_target_properties("${target}" PROPERTIES
      ${property} "${val}${addtext}"
      )
  endif(NOT "${addtext}" STREQUAL "")
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

  KLFAppendToTargetProp("${target}" LINK_FLAGS "${add_link_flags}")

endmacro(KLFNoShlibUndefined)
