# Copyright (c) Microsoft Corporation.
# Licensed under the MIT License.

project(IntegrationDOClient LANGUAGES CXX)

# Settings for do-client library
set(DO_INCLUDE_SDK
    ON
    CACHE BOOL "" FORCE)
set(DO_BUILD_TESTS
    OFF
    CACHE BOOL "" FORCE)
if(NOT DEFINED CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE debug)
endif()

if(MSVC)
    set(DO_PATCH_COMMAND "")
else()
    # Populate LSB_RELEASE_ID_SHORT with the linux distro if available
    find_program(LSB_RELEASE_EXEC lsb_release)
    execute_process(
        COMMAND ${LSB_RELEASE_EXEC} -is
        OUTPUT_VARIABLE LSB_RELEASE_ID_SHORT
        OUTPUT_STRIP_TRAILING_WHITESPACE)

    if(LSB_RELEASE_ID_SHORT STREQUAL "Ubuntu")
        # 1. Run bootstrap script to install Linux dependencies
        # 2. Build & install DO Agent, which acts as a service for the REST API
        set(DO_PATCH_COMMAND
            cd <SOURCE_DIR> && sudo ./build/scripts/bootstrap.sh --install
            build && python3 ./build/build.py --project agent --package-for deb
            && cd /tmp/build-deliveryoptimization-agent/linux-debug && sudo
            dpkg -i deliveryoptimization-agent_1.1.0_amd64.deb)
    else()
        message(FATAL_ERROR "Unsupported platform ${LSB_RELEASE_ID_SHORT}")
    endif()
endif()

# Include the capability of fetching external content
include(FetchContent)

FetchContent_Declare(
    do-client
    GIT_REPOSITORY https://github.com/microsoft/do-client.git
    GIT_TAG d71ade6f692dd8bc319ec3228c956517e9b29292 # v1.1.0
    PATCH_COMMAND "${DO_PATCH_COMMAND}")
FetchContent_MakeAvailable(do-client)

add_executable(${PROJECT_NAME})

target_sources(${PROJECT_NAME} PRIVATE IntegrationDOClient.cpp)

set(SFS_CLIENT_LIB_NAME "Microsoft::${CMAKE_PROJECT_NAME}")
add_dependencies(${PROJECT_NAME} ${SFS_CLIENT_LIB_NAME})
target_link_libraries(${PROJECT_NAME} PRIVATE ${SFS_CLIENT_LIB_NAME})

set(DO_SDK_LIB_NAME "Microsoft::deliveryoptimization")
add_dependencies(${PROJECT_NAME} ${DO_SDK_LIB_NAME})
target_link_libraries(${PROJECT_NAME} PRIVATE ${DO_SDK_LIB_NAME})

target_include_directories(${PROJECT_NAME} PUBLIC ../../client/include)
