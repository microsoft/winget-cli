# Copyright (c) Microsoft Corporation.
# Licensed under the MIT License.

# JSON library
find_package(nlohmann_json CONFIG REQUIRED)

project(SFSClientTool LANGUAGES CXX)

add_executable(${PROJECT_NAME})

target_sources(${PROJECT_NAME} PRIVATE SFSClientTool.cpp)

set(SFS_CLIENT_LIB_NAME "Microsoft::${CMAKE_PROJECT_NAME}")
add_dependencies(${PROJECT_NAME} ${SFS_CLIENT_LIB_NAME})

target_link_libraries(${PROJECT_NAME} PRIVATE ${SFS_CLIENT_LIB_NAME})
target_link_libraries(${PROJECT_NAME} PRIVATE nlohmann_json::nlohmann_json)

target_include_directories(${PROJECT_NAME} PUBLIC ../../client/include)

set_compile_options_for_target(${PROJECT_NAME})
