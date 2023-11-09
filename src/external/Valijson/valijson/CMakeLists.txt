cmake_minimum_required(VERSION 3.1)
project(valijson)

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

option(valijson_INSTALL_HEADERS "Install valijson headers." FALSE)
option(valijson_BUILD_EXAMPLES "Build valijson examples." FALSE)
option(valijson_BUILD_TESTS "Build valijson test suite." TRUE)
option(valijson_EXCLUDE_BOOST "Exclude Boost when building test suite." FALSE)
option(valijson_USE_EXCEPTIONS "Use exceptions in valijson and included libs." TRUE)

if(MSVC)
    SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /W4")
else()
    include(CheckCXXCompilerFlag)
    CHECK_CXX_COMPILER_FLAG("-std=c++11" COMPILER_SUPPORTS_CXX11)
    if(!COMPILER_SUPPORTS_CXX11)
        message(FATAL_ERROR "The compiler ${CMAKE_CXX_COMPILER} has no C++11 support. Please use a different C++ compiler.")
    endif()

    # Suppress boost warnings that aren't relevant to the test suite
    SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DBOOST_BIND_GLOBAL_PLACEHOLDERS")

    SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11 -Wall")
    SET(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -O0")
endif()

add_library(valijson INTERFACE)
# create alias, so that user could always write target_link_libraries(... ValiJSON::valijson)
# despite of valijson target is imported or not
add_library(ValiJSON::valijson ALIAS valijson)

include(GNUInstallDirs)
target_include_directories(valijson INTERFACE
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>)

if(valijson_INSTALL_HEADERS)
    install(DIRECTORY include/ DESTINATION include)
endif()

if(NOT valijson_BUILD_TESTS AND NOT valijson_BUILD_EXAMPLES)
    return()
endif()

if(valijson_USE_EXCEPTIONS)
    add_definitions(-DVALIJSON_USE_EXCEPTIONS=1)
else()
    add_definitions(-D_HAS_EXCEPTIONS=0)
    add_definitions(-DBOOST_NO_EXCEPTIONS)
    add_definitions(-DJSON_USE_EXCEPTION=0)
    add_definitions(-DVALIJSON_USE_EXCEPTIONS=0)
endif()

find_package(Poco OPTIONAL_COMPONENTS JSON)
find_package(Qt5Core)

# jsoncpp library
add_library(jsoncpp
    thirdparty/jsoncpp-1.9.4/src/lib_json/json_reader.cpp
    thirdparty/jsoncpp-1.9.4/src/lib_json/json_value.cpp
    thirdparty/jsoncpp-1.9.4/src/lib_json/json_writer.cpp
)

target_include_directories(jsoncpp SYSTEM PRIVATE thirdparty/jsoncpp-1.9.4/include)
set_target_properties(jsoncpp PROPERTIES ARCHIVE_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}/thirdparty/jsoncpp-1.9.4)

add_library(json11
    thirdparty/json11-ec4e452/json11.cpp
)

target_include_directories(json11 SYSTEM PRIVATE thirdparty/json11-ec4e452)
set_target_properties(json11 PROPERTIES ARCHIVE_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}/thirdparty/json11-ec4e452)

# Not all of these are required for examples build it doesn't hurt to include them
include_directories(include SYSTEM
    thirdparty/gtest-1.11.0/include
    thirdparty/json11-ec4e452
    thirdparty/jsoncpp-1.9.4/include
    thirdparty/rapidjson-48fbd8c/include
    thirdparty/picojson-1.3.0
    thirdparty/nlohmann-json-3.1.2
    )

if(valijson_BUILD_TESTS)
    if(NOT valijson_EXCLUDE_BOOST)
        find_package(Boost)
    endif()

    # Build local gtest
    set(gtest_force_shared_crt ON)
    option(BUILD_GMOCK FALSE)
    option(INSTALL_GTEST FALSE)
    add_subdirectory(thirdparty/gtest-1.11.0)

    set(TEST_SOURCES
        tests/test_adapter_comparison.cpp
        tests/test_fetch_urn_document_callback.cpp
        tests/test_fetch_absolute_uri_document_callback.cpp
        tests/test_json_pointer.cpp
        tests/test_json11_adapter.cpp
        tests/test_jsoncpp_adapter.cpp
        tests/test_nlohmann_json_adapter.cpp
        tests/test_rapidjson_adapter.cpp
        tests/test_picojson_adapter.cpp
        tests/test_poly_constraint.cpp
        tests/test_validation_errors.cpp
        tests/test_validator.cpp
    )

    set(TEST_LIBS gtest gtest_main jsoncpp json11)

    if(Boost_FOUND)
        include_directories(${Boost_INCLUDE_DIRS})
        list(APPEND TEST_SOURCES tests/test_boost_json_adapter.cpp)
        list(APPEND TEST_SOURCES tests/test_property_tree_adapter.cpp)
    endif()

    if(Poco_FOUND)
        include_directories(${Poco_INCLUDE_DIRS})
        list(APPEND TEST_SOURCES tests/test_poco_json_adapter.cpp)
        list(APPEND TEST_LIBS ${Poco_Foundation_LIBRARIES} ${Poco_JSON_LIBRARIES})
    endif()

    if(Qt5Core_FOUND)
        include_directories(${Qt5Core_INCLUDE_DIRS})
        list(APPEND TEST_LIBS Qt5::Core)
        list(APPEND TEST_SOURCES tests/test_qtjson_adapter.cpp)
    endif()

    # Unit tests executable
    add_executable(test_suite ${TEST_SOURCES})
    if(NOT valijson_USE_EXCEPTIONS)
        if(MSVC)
            if(CMAKE_CXX_FLAGS MATCHES "/EHsc ")
                string(REPLACE "/EHsc" "/EHs-c-" CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}")
            else()
                target_compile_options(test_suite PUBLIC /EHs-c-)
            endif()
        else()
            target_compile_options(test_suite PUBLIC -fno-exceptions)
        endif()
    endif()

    if(NOT MSVC)
        set_target_properties(test_suite PROPERTIES COMPILE_FLAGS " -pedantic -Werror -Wshadow -Wunused")
    endif()

    # Definition for using picojson
    set_target_properties(test_suite PROPERTIES COMPILE_DEFINITIONS "PICOJSON_USE_INT64")

    if(Boost_FOUND)
        add_definitions(-DBOOST_ALL_DYN_LINK)
        set(Boost_USE_STATIC_LIBS OFF)
        set(Boost_USE_MULTITHREADED ON)
        set(Boost_USE_STATIC_RUNTIME OFF)
        target_compile_definitions(test_suite PRIVATE "VALIJSON_BUILD_BOOST_ADAPTERS")
    endif()

    if(Poco_FOUND)
        target_compile_definitions(test_suite PRIVATE "VALIJSON_BUILD_POCO_ADAPTER")
    endif()

    if(Qt5Core_FOUND)
        target_compile_definitions(test_suite PRIVATE "VALIJSON_BUILD_QT_ADAPTER")
    endif()

    target_link_libraries(test_suite ${TEST_LIBS} ${Boost_LIBRARIES})
endif()

if(valijson_BUILD_EXAMPLES)
    find_package(curlpp)
    include_directories(SYSTEM)

    add_executable(custom_schema
        examples/custom_schema.cpp
    )

    add_executable(external_schema
        examples/external_schema.cpp
    )

    add_executable(array_iteration_basics
        examples/array_iteration_basics.cpp
    )

    add_executable(array_iteration_template_fn
        examples/array_iteration_template_fn.cpp
    )

    add_executable(object_iteration
        examples/object_iteration.cpp
    )

    add_executable(json_pointers
        examples/json_pointers.cpp
    )

    add_executable(remote_resolution_local_file
        examples/remote_resolution_local_file.cpp
    )

    if(curlpp_FOUND)
        include_directories(${curlpp_INCLUDE_DIR})

        add_executable(remote_resolution_url
            examples/remote_resolution_url.cpp
        )

        target_link_libraries(remote_resolution_url curl ${curlpp_LIBRARIES})
    endif()

    target_link_libraries(custom_schema ${Boost_LIBRARIES})
    target_link_libraries(external_schema ${Boost_LIBRARIES})
    target_link_libraries(array_iteration_basics jsoncpp)
    target_link_libraries(array_iteration_template_fn jsoncpp)
    target_link_libraries(object_iteration jsoncpp)
    target_link_libraries(json_pointers)
endif()
