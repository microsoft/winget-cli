project(cppx_base)

add_executable(cppx_base "")
target_sources(cppx_base PUBLIC main.cpp)
target_include_directories(cppx_base PRIVATE "${CMAKE_CURRENT_BINARY_DIR}")
target_link_libraries(cppx_base pal)

if (WIN32)
    target_compile_options(cppx_base PUBLIC /await /permissive-)
    target_link_libraries(cppx_base windowsapp ole32 shlwapi)
else()
    target_link_libraries(cppx_base c++ c++abi c++experimental)
    target_link_libraries(cppx_base -lpthread)
endif()

add_custom_target(cppx_base_h
    COMMAND cppxlang -base -out "${CMAKE_CURRENT_BINARY_DIR}")

add_dependencies(cppx_base cppx_base_h)
