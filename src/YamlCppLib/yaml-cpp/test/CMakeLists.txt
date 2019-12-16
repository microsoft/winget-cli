include(ExternalProject)

if(MSVC)
    # MS Visual Studio expects lib prefix on static libraries,
    # but CMake compiles them without prefix
    # See https://gitlab.kitware.com/cmake/cmake/issues/17338
    set(CMAKE_STATIC_LIBRARY_PREFIX "")
endif()

ExternalProject_Add(
	googletest_project
	SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/gtest-1.8.0"
	INSTALL_DIR "${CMAKE_CURRENT_BINARY_DIR}/prefix"
	CMAKE_ARGS
		-DCMAKE_INSTALL_PREFIX:PATH=<INSTALL_DIR>
		-DBUILD_GMOCK=ON
		-Dgtest_force_shared_crt=ON
)

add_library(gmock UNKNOWN IMPORTED)
set_target_properties(gmock PROPERTIES
    IMPORTED_LOCATION
    ${PROJECT_BINARY_DIR}/test/prefix/lib/${CMAKE_STATIC_LIBRARY_PREFIX}gmock${CMAKE_STATIC_LIBRARY_SUFFIX}
)

find_package(Threads)

include_directories(SYSTEM "${PROJECT_BINARY_DIR}/test/prefix/include")

if(CMAKE_CXX_COMPILER_ID MATCHES "GNU" OR
	CMAKE_CXX_COMPILER_ID MATCHES "Clang")
	set(yaml_test_flags "-Wno-variadic-macros -Wno-sign-compare")

	if(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
		set(yaml_test_flags "${yaml_test_flags} -Wno-c99-extensions")
	endif()
endif()

file(GLOB test_headers [a-z_]*.h)
file(GLOB test_sources [a-z_]*.cpp integration/[a-z_]*.cpp node/[a-z_]*.cpp)
file(GLOB test_new_api_sources new-api/[a-z]*.cpp)

list(APPEND test_sources ${test_new_api_sources})
add_sources(${test_sources} ${test_headers})

include_directories(${YAML_CPP_SOURCE_DIR}/src)
include_directories(${YAML_CPP_SOURCE_DIR}/test)

add_executable(run-tests
    ${test_sources}
    ${test_headers}
)

set_target_properties(run-tests PROPERTIES
    CXX_STANDARD 11
    CXX_STANDARD_REQUIRED ON
)

add_dependencies(run-tests googletest_project)

set_target_properties(run-tests PROPERTIES
    COMPILE_FLAGS "${yaml_c_flags} ${yaml_cxx_flags} ${yaml_test_flags}"
)
target_link_libraries(run-tests
    yaml-cpp
    gmock
    ${CMAKE_THREAD_LIBS_INIT})

add_test(yaml-test ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/run-tests)
