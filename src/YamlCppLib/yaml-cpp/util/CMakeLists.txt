cmake_minimum_required(VERSION 3.5)

add_sources(parse.cpp)
add_executable(parse parse.cpp)
set_target_properties(parse PROPERTIES
    CXX_STANDARD 11
    CXX_STANDARD_REQUIRED ON
)
target_link_libraries(parse yaml-cpp)

add_sources(sandbox.cpp)
add_executable(sandbox sandbox.cpp)
set_target_properties(sandbox PROPERTIES
    CXX_STANDARD 11
    CXX_STANDARD_REQUIRED ON
)
target_link_libraries(sandbox yaml-cpp)

add_sources(read.cpp)
add_executable(read read.cpp)
set_target_properties(read PROPERTIES
    CXX_STANDARD 11
    CXX_STANDARD_REQUIRED ON
)
target_link_libraries(read yaml-cpp)

