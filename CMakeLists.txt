###
### CMake settings
###
# see http://www.cmake.org/Wiki/CMake_Policies
cmake_minimum_required(VERSION 3.1)

include(CheckCXXCompilerFlag)

###
### Project settings
###
project(YAML_CPP VERSION 0.6.3)

###
### Project options
###
## Project stuff
option(YAML_CPP_BUILD_TESTS "Enable testing" ON)
option(YAML_CPP_BUILD_TOOLS "Enable parse tools" ON)
option(YAML_CPP_BUILD_CONTRIB "Enable contrib stuff in library" ON)
option(YAML_CPP_INSTALL "Enable generation of install target" ON)

## Build options
# --> General
# see http://www.cmake.org/cmake/help/cmake2.6docs.html#variable:BUILD_SHARED_LIBS
#     http://www.cmake.org/cmake/help/cmake2.6docs.html#command:add_library
option(YAML_BUILD_SHARED_LIBS "Build Shared Libraries" OFF)

# --> Apple
if(APPLE)
    option(YAML_APPLE_UNIVERSAL_BIN "Apple: Build universal binary" OFF)
endif()

# --> Microsoft Visual C++
# see http://msdn.microsoft.com/en-us/library/aa278396(v=VS.60).aspx
#     http://msdn.microsoft.com/en-us/library/2kzt1wy3(v=VS.71).aspx
if(MSVC)
    option(YAML_MSVC_SHARED_RT "MSVC: Build with shared runtime libs (/MD)" ON)
    option(YAML_MSVC_STHREADED_RT "MSVC: Build with single-threaded static runtime libs (/ML until VS .NET 2003)" OFF)
endif()

###
### Sources, headers, directories and libs
###

# From http://www.cmake.org/pipermail/cmake/2010-March/035992.html:
# function to collect all the sources from sub-directories
# into a single list
function(add_sources)
  get_property(is_defined GLOBAL PROPERTY SRCS_LIST DEFINED)
  if(NOT is_defined)
    define_property(GLOBAL PROPERTY SRCS_LIST
      BRIEF_DOCS "List of source files"
      FULL_DOCS "List of all source files in the entire project")
  endif()
  # make absolute paths
  set(SRCS)
  foreach(s IN LISTS ARGN)
    if(NOT IS_ABSOLUTE "${s}")
      get_filename_component(s "${s}" ABSOLUTE)
    endif()
    list(APPEND SRCS "${s}")
  endforeach()
  # append to global list
  set_property(GLOBAL APPEND PROPERTY SRCS_LIST "${SRCS}")
endfunction(add_sources)

set(header_directory "include/yaml-cpp/")

file(GLOB sources "src/[a-zA-Z]*.cpp")
file(GLOB_RECURSE public_headers "include/yaml-cpp/[a-zA-Z]*.h")
file(GLOB private_headers "src/[a-zA-Z]*.h")

if(YAML_CPP_BUILD_CONTRIB)
	file(GLOB contrib_sources "src/contrib/[a-zA-Z]*.cpp")
	file(GLOB contrib_public_headers "include/yaml-cpp/contrib/[a-zA-Z]*.h")
	file(GLOB contrib_private_headers "src/contrib/[a-zA-Z]*.h")
else()
	add_definitions(-DYAML_CPP_NO_CONTRIB)
endif()

set(library_sources
  ${sources}
  ${public_headers}
  ${private_headers}
  ${contrib_sources}
  ${contrib_public_headers}
  ${contrib_private_headers}
)
add_sources(${library_sources})

if(VERBOSE)
	message(STATUS "sources: ${sources}")
	message(STATUS "public_headers: ${public_headers}")
	message(STATUS "private_headers: ${private_headers}")
	message(STATUS "contrib_sources: ${contrib_sources}")
	message(STATUS "contrib_public_headers: ${contrib_public_headers}")
	message(STATUS "contrib_private_headers: ${contrib_private_headers}")
endif()

if (CMAKE_VERSION VERSION_LESS 2.8.12)
    include_directories(${YAML_CPP_SOURCE_DIR}/src)
    include_directories(${YAML_CPP_SOURCE_DIR}/include)
endif()


###
### General compilation settings
###
set(yaml_c_flags ${CMAKE_C_FLAGS})
set(yaml_cxx_flags ${CMAKE_CXX_FLAGS})

if(YAML_BUILD_SHARED_LIBS)
	set(LABEL_SUFFIX "shared")
else()
	set(LABEL_SUFFIX "static")
endif()

if(APPLE)
	if(YAML_APPLE_UNIVERSAL_BIN)
		set(CMAKE_OSX_ARCHITECTURES ppc;i386)
	endif()
endif()

if(IPHONE)
	set(CMAKE_OSX_SYSROOT "iphoneos4.2")
	set(CMAKE_OSX_ARCHITECTURES "armv6;armv7")
endif()

if(WIN32)
	if(YAML_BUILD_SHARED_LIBS)
		add_definitions(-D${PROJECT_NAME}_DLL)	# use or build Windows DLL
	endif()
	if(CMAKE_INSTALL_PREFIX_INITIALIZED_TO_DEFAULT)
		set(CMAKE_INSTALL_PREFIX "C:/")
	endif()
endif()

# GCC or Clang or Intel Compiler specialities
if(CMAKE_CXX_COMPILER_ID MATCHES "GNU" OR
   (CMAKE_CXX_COMPILER_ID MATCHES "Clang" AND NOT "x${CMAKE_CXX_SIMULATE_ID}" STREQUAL "xMSVC") OR
   CMAKE_CXX_COMPILER_ID MATCHES "Intel")

	### General stuff
	if(WIN32)
		set(CMAKE_SHARED_LIBRARY_PREFIX "")	# DLLs do not have a "lib" prefix
		set(CMAKE_IMPORT_LIBRARY_PREFIX "")	# same for DLL import libs
		set(CMAKE_LINK_DEF_FILE_FLAG "")	# CMake workaround (2.8.3)
	endif()

	### Project stuff
	if(NOT CMAKE_CONFIGURATION_TYPES AND NOT CMAKE_BUILD_TYPE)
		set(CMAKE_BUILD_TYPE Release)
	endif()
	#
	set(GCC_EXTRA_OPTIONS "")
	#
	if(YAML_BUILD_SHARED_LIBS)
		set(GCC_EXTRA_OPTIONS "${GCC_EXTRA_OPTIONS} -fPIC")
	endif()
	#
	set(FLAG_TESTED "-Wextra -Wshadow -Weffc++ -pedantic -pedantic-errors")
	check_cxx_compiler_flag(${FLAG_TESTED} FLAG_WEXTRA)
	if(FLAG_WEXTRA)
		set(GCC_EXTRA_OPTIONS "${GCC_EXTRA_OPTIONS} ${FLAG_TESTED}")
	endif()
	#
	set(yaml_cxx_flags "-Wall ${GCC_EXTRA_OPTIONS} -pedantic -Wno-long-long ${yaml_cxx_flags}")

	### Make specific
	if(${CMAKE_BUILD_TOOL} MATCHES make OR ${CMAKE_BUILD_TOOL} MATCHES gmake)
		add_custom_target(debuggable ${CMAKE_MAKE_PROGRAM} clean
			COMMAND ${CMAKE_COMMAND} -DCMAKE_BUILD_TYPE=Debug ${CMAKE_SOURCE_DIR}
			COMMENT "Adjusting settings for debug compilation"
			VERBATIM)
		add_custom_target(releasable ${CMAKE_MAKE_PROGRAM} clean
			COMMAND ${CMAKE_COMMAND} -DCMAKE_BUILD_TYPE=Release ${CMAKE_SOURCE_DIR}
			COMMENT "Adjusting settings for release compilation"
			VERBATIM)
	endif()
endif()

# Microsoft VisualC++ specialities
if(MSVC)
	### General stuff
	# a) Change MSVC runtime library settings (/MD[d], /MT[d], /ML[d] (single-threaded until VS 2003))
	#    plus set lib suffix for later use and project label accordingly
	# see http://msdn.microsoft.com/en-us/library/aa278396(v=VS.60).aspx
	#     http://msdn.microsoft.com/en-us/library/2kzt1wy3(v=VS.71).aspx
	set(LIB_RT_SUFFIX "md")	# CMake defaults to /MD for MSVC
	set(LIB_RT_OPTION "/MD")
	#
	if(NOT YAML_MSVC_SHARED_RT)	# User wants to have static runtime libraries (/MT, /ML)
		if(YAML_MSVC_STHREADED_RT)	# User wants to have old single-threaded static runtime libraries
			set(LIB_RT_SUFFIX "ml")
			set(LIB_RT_OPTION "/ML")
			if(NOT ${MSVC_VERSION} LESS 1400)
				message(FATAL_ERROR "Single-threaded static runtime libraries (/ML) only available until VS .NET 2003 (7.1).")
			endif()
		else()
			set(LIB_RT_SUFFIX "mt")
			set(LIB_RT_OPTION "/MT")
		endif()

		# correct linker options
		foreach(flag_var  CMAKE_C_FLAGS  CMAKE_CXX_FLAGS)
			foreach(config_name  ""  DEBUG  RELEASE  MINSIZEREL  RELWITHDEBINFO)
				set(var_name "${flag_var}")
				if(NOT "${config_name}" STREQUAL "")
					set(var_name "${var_name}_${config_name}")
				endif()
				string(REPLACE "/MD" "${LIB_RT_OPTION}" ${var_name} "${${var_name}}")
				set(${var_name} "${${var_name}}" CACHE STRING "" FORCE)
			endforeach()
		endforeach()
	endif()
	#
	set(LABEL_SUFFIX "${LABEL_SUFFIX} ${LIB_RT_SUFFIX}")

	# b) Change prefix for static libraries
	set(CMAKE_STATIC_LIBRARY_PREFIX "lib")	# to distinguish static libraries from DLL import libs

	# c) Correct suffixes for static libraries
	if(NOT YAML_BUILD_SHARED_LIBS)
		### General stuff
		set(LIB_TARGET_SUFFIX "${LIB_SUFFIX}${LIB_RT_SUFFIX}")
	endif()

	### Project stuff
	# /W3 = set warning level; see http://msdn.microsoft.com/en-us/library/thxezb7y.aspx
	# /wd4127 = disable warning C4127 "conditional expression is constant"; see http://msdn.microsoft.com/en-us/library/6t66728h.aspx
	# /wd4355 = disable warning C4355 "'this' : used in base member initializer list"; http://msdn.microsoft.com/en-us/library/3c594ae3.aspx
	set(yaml_cxx_flags "/W3 /wd4127 /wd4355 ${yaml_cxx_flags}")
endif()


###
### General install settings
###
set(INCLUDE_INSTALL_ROOT_DIR include)

set(INCLUDE_INSTALL_DIR ${INCLUDE_INSTALL_ROOT_DIR}/yaml-cpp)
set(LIB_INSTALL_DIR "lib${LIB_SUFFIX}")

set(_INSTALL_DESTINATIONS
	RUNTIME DESTINATION bin
	LIBRARY DESTINATION ${LIB_INSTALL_DIR}
	ARCHIVE DESTINATION ${LIB_INSTALL_DIR}
)


###
### Library
###
if(YAML_BUILD_SHARED_LIBS)
    add_library(yaml-cpp SHARED ${library_sources})
else()
    add_library(yaml-cpp STATIC ${library_sources})
endif()

if (NOT CMAKE_VERSION VERSION_LESS 2.8.12)
    target_include_directories(yaml-cpp
        PUBLIC $<BUILD_INTERFACE:${YAML_CPP_SOURCE_DIR}/include>
               $<INSTALL_INTERFACE:${INCLUDE_INSTALL_ROOT_DIR}>
        PRIVATE $<BUILD_INTERFACE:${YAML_CPP_SOURCE_DIR}/src>)
endif()

set_target_properties(yaml-cpp PROPERTIES
    CXX_STANDARD 11
    CXX_STANDARD_REQUIRED ON
)

set_target_properties(yaml-cpp PROPERTIES
  COMPILE_FLAGS "${yaml_c_flags} ${yaml_cxx_flags}"
)

set_target_properties(yaml-cpp PROPERTIES
	VERSION "${YAML_CPP_VERSION}"
	SOVERSION "${YAML_CPP_VERSION_MAJOR}.${YAML_CPP_VERSION_MINOR}"
	PROJECT_LABEL "yaml-cpp ${LABEL_SUFFIX}"
)

if(IPHONE)
	set_target_properties(yaml-cpp PROPERTIES
		XCODE_ATTRIBUTE_IPHONEOS_DEPLOYMENT_TARGET "3.0"
	)
endif()

if(MSVC)
	if(NOT YAML_BUILD_SHARED_LIBS)
		# correct library names
		set_target_properties(yaml-cpp PROPERTIES
			DEBUG_POSTFIX "${LIB_TARGET_SUFFIX}d"
			RELEASE_POSTFIX "${LIB_TARGET_SUFFIX}"
			MINSIZEREL_POSTFIX "${LIB_TARGET_SUFFIX}"
			RELWITHDEBINFO_POSTFIX "${LIB_TARGET_SUFFIX}"
		)
	endif()
endif()

if (YAML_CPP_INSTALL)
	install(TARGETS yaml-cpp EXPORT yaml-cpp-targets ${_INSTALL_DESTINATIONS})
	install(
		DIRECTORY ${header_directory}
		DESTINATION ${INCLUDE_INSTALL_DIR}
		FILES_MATCHING PATTERN "*.h"
	)
endif()

export(
    TARGETS yaml-cpp
    FILE "${PROJECT_BINARY_DIR}/yaml-cpp-targets.cmake")
export(PACKAGE yaml-cpp)
set(EXPORT_TARGETS yaml-cpp CACHE INTERNAL "export targets")

set(CONFIG_INCLUDE_DIRS "${YAML_CPP_SOURCE_DIR}/include")
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/yaml-cpp-config.cmake.in
	"${PROJECT_BINARY_DIR}/yaml-cpp-config.cmake" @ONLY)

if(WIN32 AND NOT CYGWIN)
	set(INSTALL_CMAKE_DIR CMake)
else()
	set(INSTALL_CMAKE_DIR ${LIB_INSTALL_DIR}/cmake/yaml-cpp)
endif()


file(RELATIVE_PATH REL_INCLUDE_DIR "${CMAKE_INSTALL_PREFIX}/${INSTALL_CMAKE_DIR}" "${CMAKE_INSTALL_PREFIX}/${INCLUDE_INSTALL_ROOT_DIR}")
set(CONFIG_INCLUDE_DIRS "\${YAML_CPP_CMAKE_DIR}/${REL_INCLUDE_DIR}")
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/yaml-cpp-config.cmake.in
	"${PROJECT_BINARY_DIR}${CMAKE_FILES_DIRECTORY}/yaml-cpp-config.cmake" @ONLY)

configure_file(${CMAKE_CURRENT_SOURCE_DIR}/yaml-cpp-config-version.cmake.in
	"${PROJECT_BINARY_DIR}/yaml-cpp-config-version.cmake" @ONLY)

if (YAML_CPP_INSTALL)
	install(FILES
		"${PROJECT_BINARY_DIR}${CMAKE_FILES_DIRECTORY}/yaml-cpp-config.cmake"
		"${PROJECT_BINARY_DIR}/yaml-cpp-config-version.cmake"
		DESTINATION "${INSTALL_CMAKE_DIR}" COMPONENT dev)
	install(EXPORT yaml-cpp-targets DESTINATION ${INSTALL_CMAKE_DIR})

	if(UNIX)
		set(PC_FILE ${CMAKE_BINARY_DIR}/yaml-cpp.pc)
		configure_file("yaml-cpp.pc.cmake" ${PC_FILE} @ONLY)
		install(FILES ${PC_FILE} DESTINATION ${LIB_INSTALL_DIR}/pkgconfig)
	endif()

endif()


###
### Extras
###
if(YAML_CPP_BUILD_TESTS)
	enable_testing()
	add_subdirectory(test)
endif()
if(YAML_CPP_BUILD_TOOLS)
	add_subdirectory(util)
endif()

### Formatting
get_property(all_sources GLOBAL PROPERTY SRCS_LIST)
add_custom_target(format
	COMMAND clang-format --style=file -i ${all_sources}
	COMMENT "Running clang-format"
	VERBATIM)
