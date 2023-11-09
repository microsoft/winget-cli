#
### Macro: subdirlist
#
# Returns a list of subdirectories.
#
macro(subdirlist result curdir)
  file(GLOB children RELATIVE ${curdir} ${curdir}/*)
  set(dirlist "")
  foreach(child ${children})
    if(IS_DIRECTORY ${curdir}/${child})
        set(dirlist ${dirlist} ${child})
    endif()
  endforeach()
  set(${result} ${dirlist})
endmacro()


#
### Macro: join
#
# Joins a string array.
# Example:
#   SET( letters "" "\;a" b c "d\;d" )
#   JOIN("${letters}" ":" output)
#   MESSAGE("${output}") # :;a:b:c:d;d
#
function(JOIN VALUES GLUE OUTPUT)
  string (REPLACE ";" "${GLUE}" _TMP_STR "${VALUES}")
  set (${OUTPUT} "${_TMP_STR}" PARENT_SCOPE)
endfunction()


#
### Macro: list_length
#
# Example:
# SET(MYLIST hello world foo bar)
# LIST_LENGTH(length ${MYLIST})
# MESSAGE("length: ${length}")
#
macro(LIST_LENGTH var)
  set(entries)
  foreach(e ${ARGN})
    set(entries "${entries}.")
  endforeach(e)
  string(LENGTH ${entries} ${var})
endmacro()


#
### Function: append_unique_list
#
# Appends items from the source list to the given target list
# if they are not already contained within the target list
# in flattened string form.
#
function(append_unique_list target source)
  if (NOT ${source})
    return()
  endif()
  if (NOT ${target})
    set(${target} ${${source}} PARENT_SCOPE)
  else()
    join("${${target}}" ":" target_str)
    join("${${source}}" ":" source_str)
    if (NOT ${target_str} MATCHES ${source_str})
      set(${target} ${${target}} ${${source}} PARENT_SCOPE)
    endif()
  endif()
endfunction()


#
### Function: filter_list
#
function(filter_list result source regex)
  set(items)
  foreach(ITR ${source})
    if(NOT ITR MATCHES ${regex})
      list(APPEND items ${ITR})
    endif()
  endforeach()
  set(${result} ${items} PARENT_SCOPE)
endfunction()


#
### Function: find_existing_directory
#
function(find_existing_directory result)
  foreach(dir ${ARGN})
    if(EXISTS ${dir})
      get_filename_component(dir ${dir} ABSOLUTE)
      set(${result} ${dir} PARENT_SCOPE)
      return()
    endif()
  endforeach()
endfunction()


#
### Macro: print_module_variables
#
macro(print_module_variables name)
    message(STATUS "${name} Variables:")
    message(STATUS "-- Found: ${${name}_FOUND}")
    if (${name}_INCLUDE_DIRS)
      message(STATUS "-- Include Dirs: ${${name}_INCLUDE_DIRS}")
    else()
      message(STATUS "-- Include Dir: ${${name}_INCLUDE_DIR}")
    endif()
    if (${name}_LIBRARIES)
      message(STATUS "-- Libraries: ${${name}_LIBRARIES}")
      message(STATUS "-- Debug Libraries: ${${name}_LIBRARIES_DEBUG}")
      message(STATUS "-- Release Libraries: ${${name}_LIBRARIES_RELEASE}")
    else()
      message(STATUS "-- Library: ${${name}_LIBRARY}")
      message(STATUS "-- Debug Library: ${${name}_LIBRARY_DEBUG}")
      message(STATUS "-- Release Library: ${${name}_LIBRARY_RELEASE}")
    endif()
    message(STATUS "-- Dependencies: ${${name}_DEPENDENCIES}")

    # message("Paths: ${${prefix}_PATHS}")
    # message("Debug Paths: ${${prefix}_PATHS_RELEASE}")
    # message("Release Paths: ${${prefix}_PATHS_DEBUG}")
    # message("Debug Names: ${${prefix}_NAMES_RELEASE}")
    # message("Release Names: ${${prefix}_NAMES_DEBUG}")
    # message("Names: ${${prefix}_NAMES}")
endmacro()


#
### Macro: set_option
#
# Provides an option that the user can optionally select.
# Can accept condition to control when option is available for user.
# Usage:
#   option(<option_variable> "help string describing the option" <initial value or boolean expression> [IF <condition>])
#
macro(set_option variable description value)
  set(__value ${value})
  set(__condition "")
  set(__varname "__value")
  foreach(arg ${ARGN})
    if(arg STREQUAL "IF" OR arg STREQUAL "if")
      set(__varname "__condition")
    else()
      list(APPEND ${__varname} ${arg})
    endif()
  endforeach()
  unset(__varname)
  if(__condition STREQUAL "")
    set(__condition 2 GREATER 1)
  endif()

  if(DEFINED ${variable})
    # set the default option value from existing value or command line arguments
    option(${variable} "${description}" ${${variable}})
  else()
    # if variable not defined set default from condition
    if(${__condition})
      if(${__value} MATCHES ";")
        if(${__value})
          option(${variable} "${description}" ON)
        else()
          option(${variable} "${description}" OFF)
        endif()
      elseif(DEFINED ${__value})
        if(${__value})
          option(${variable} "${description}" ON)
        else()
          option(${variable} "${description}" OFF)
        endif()
      else()
        option(${variable} "${description}" ${__value})
      endif()
    else()
      unset(${variable} CACHE)
    endif()
  endif()
  unset(__condition)
  unset(__value)
endmacro()


#
### Function: status
#
# Status report function.
# Automatically align right column and selects text based on condition.
# Usage:
#   status(<text>)
#   status(<heading> <value1> [<value2> ...])
#   status(<heading> <condition> THEN <text for TRUE> ELSE <text for FALSE> )
#
function(status text)
  set(status_cond)
  set(status_then)
  set(status_else)

  set(status_current_name "cond")
  foreach(arg ${ARGN})
    if(arg STREQUAL "THEN")
      set(status_current_name "then")
    elseif(arg STREQUAL "ELSE")
      set(status_current_name "else")
    else()
      list(APPEND status_${status_current_name} ${arg})
    endif()
  endforeach()

  if(DEFINED status_cond)
    set(status_placeholder_length 32)
    string(RANDOM LENGTH ${status_placeholder_length} ALPHABET " " status_placeholder)
    string(LENGTH "${text}" status_text_length)
    if(status_text_length LESS status_placeholder_length)
      string(SUBSTRING "${text}${status_placeholder}" 0 ${status_placeholder_length} status_text)
    elseif(DEFINED status_then OR DEFINED status_else)
      message(STATUS "${text}")
      set(status_text "${status_placeholder}")
    else()
      set(status_text "${text}")
    endif()

    if(DEFINED status_then OR DEFINED status_else)
      if(${status_cond})
        string(REPLACE ";" " " status_then "${status_then}")
        string(REGEX REPLACE "^[ \t]+" "" status_then "${status_then}")
        message(STATUS "${status_text} ${status_then}")
      else()
        string(REPLACE ";" " " status_else "${status_else}")
        string(REGEX REPLACE "^[ \t]+" "" status_else "${status_else}")
        message(STATUS "${status_text} ${status_else}")
      endif()
    else()
      string(REPLACE ";" " " status_cond "${status_cond}")
      string(REGEX REPLACE "^[ \t]+" "" status_cond "${status_cond}")
      message(STATUS "${status_text} ${status_cond}")
    endif()
  else()
    message(STATUS "${text}")
  endif()
endfunction()

#
### Macro: messageV
#
# Prints message only with MSG_VERBOSE=ON
# Usage:
#   messageV(<msg>)
#
function(messageV text)
  if(${MSG_VERBOSE})
    message(STATUS "${text}")
  endif()
endfunction()
