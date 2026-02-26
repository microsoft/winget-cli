# Copyright (c) Microsoft Corporation.
# Licensed under the MIT License.

# HTTP stack library
find_package(CURL REQUIRED)

# JSON library
find_package(nlohmann_json CONFIG REQUIRED)

# CorrelationVector Library from Microsoft
find_package(correlation_vector CONFIG REQUIRED)

add_library(${PROJECT_NAME} STATIC)
add_library(Microsoft::${PROJECT_NAME} ALIAS ${PROJECT_NAME})

target_sources(
    ${PROJECT_NAME}
    PRIVATE src/AppContent.cpp
            src/AppFile.cpp
            src/ApplicabilityDetails.cpp
            src/Content.cpp
            src/ContentId.cpp
            src/details/connection/Connection.cpp
            src/details/connection/ConnectionConfig.cpp
            src/details/connection/ConnectionManager.cpp
            src/details/connection/CurlConnection.cpp
            src/details/connection/CurlConnectionManager.cpp
            src/details/connection/HttpHeader.cpp
            src/details/connection/mock/MockConnection.cpp
            src/details/connection/mock/MockConnectionManager.cpp
            src/details/ContentUtil.cpp
            src/details/CorrelationVector.cpp
            src/details/entity/ContentType.cpp
            src/details/entity/FileEntity.cpp
            src/details/entity/VersionEntity.cpp
            src/details/Env.cpp
            src/details/ErrorHandling.cpp
            src/details/OSInfo.cpp
            src/details/ReportingHandler.cpp
            src/details/SFSClientImpl.cpp
            src/details/SFSException.cpp
            src/details/SFSUrlBuilder.cpp
            src/details/TestOverride.cpp
            src/details/UrlBuilder.cpp
            src/details/Util.cpp
            src/File.cpp
            src/Logging.cpp
            src/Result.cpp
            src/SFSClient.cpp)

# Include dir for the library depends on whether the library is being built or
# if it was installed
target_include_directories(
    ${PROJECT_NAME}
    PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include/sfsclient>
           $<INSTALL_INTERFACE:include> # <prefix>/include
)

target_link_libraries(${PROJECT_NAME} PRIVATE CURL::libcurl)
target_link_libraries(${PROJECT_NAME} PRIVATE nlohmann_json::nlohmann_json)
target_link_libraries(${PROJECT_NAME} PRIVATE microsoft::correlation_vector)

# Pick up git revision during configuration to add to logging
include(FindGit)
if(GIT_FOUND)
    execute_process(
        COMMAND ${GIT_EXECUTABLE} rev-parse --abbrev-ref HEAD
        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
        OUTPUT_VARIABLE SFS_GIT_HEAD_NAME
        OUTPUT_STRIP_TRAILING_WHITESPACE)
    execute_process(
        COMMAND ${GIT_EXECUTABLE} rev-parse --short HEAD
        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
        OUTPUT_VARIABLE SFS_GIT_HEAD_REVISION
        OUTPUT_STRIP_TRAILING_WHITESPACE)

    target_compile_definitions(
        ${PROJECT_NAME}
        PRIVATE SFS_GIT_INFO="${SFS_GIT_HEAD_NAME}:${SFS_GIT_HEAD_REVISION}")
endif()

set_compile_options_for_target(${PROJECT_NAME})

target_compile_definitions(${PROJECT_NAME}
                           PRIVATE SFS_VERSION="${SFS_LIBRARY_VERSION}")

if(SFS_ENABLE_TEST_OVERRIDES)
    target_compile_definitions(${PROJECT_NAME}
                               PRIVATE SFS_ENABLE_TEST_OVERRIDES=1)
endif()

if(SFS_BUILD_TESTS)
    # Enables one to run tests through "ctest --test-dir .\build\client"
    enable_testing()

    add_subdirectory(tests)
endif()

#
# Install section
#

set(_INSTALL_DESTINATION "${CMAKE_INSTALL_LIBDIR}/cmake/sfsclient")
set(_TARGET_CONFIG_CMAKE_FILEPATH
    "${CMAKE_CURRENT_BINARY_DIR}/sfsclient-config.cmake")

# Install headers
install(
    FILES include/sfsclient/AppContent.h
          include/sfsclient/AppFile.h
          include/sfsclient/ApplicabilityDetails.h
          include/sfsclient/ClientConfig.h
          include/sfsclient/Content.h
          include/sfsclient/ContentId.h
          include/sfsclient/File.h
          include/sfsclient/Logging.h
          include/sfsclient/RequestParams.h
          include/sfsclient/Result.h
          include/sfsclient/SFSClient.h
    DESTINATION include/sfsclient)

# Export targets for this library to a local file
install(
    TARGETS ${PROJECT_NAME}
    EXPORT sfsclient-targets
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR})

# And then install the exported targets file to the destination
install(
    EXPORT sfsclient-targets
    FILE sfsclient-targets.cmake
    NAMESPACE microsoft::
    DESTINATION ${_INSTALL_DESTINATION})

# Add configure_package_config_file for below
include(CMakePackageConfigHelpers)

# Create the sfsclient-config.cmake file, which will be used by find_package()
configure_package_config_file(
    ../cmake/sfsclient-config.cmake.in ${_TARGET_CONFIG_CMAKE_FILEPATH}
    INSTALL_DESTINATION ${_INSTALL_DESTINATION})

# Then install the generated file to the destination
install(FILES ${_TARGET_CONFIG_CMAKE_FILEPATH}
        DESTINATION ${_INSTALL_DESTINATION})
