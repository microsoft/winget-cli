project(pywinrt)

add_executable(pywinrt "")
target_sources(pywinrt PUBLIC main.cpp pch.cpp "${PROJECT_BINARY_DIR}/strings.cpp")
target_include_directories(pywinrt PUBLIC ${XLANG_LIBRARY_PATH} ${PROJECT_BINARY_DIR} ${PROJECT_SOURCE_DIR})
target_compile_definitions(pywinrt PUBLIC "XLANG_VERSION_STRING=\"${XLANG_BUILD_VERSION}\"")

GENERATE_STRING_LITERAL_FILES("${PROJECT_SOURCE_DIR}/strings/*" "strings" "pywinrt::strings" pywinrt)

if (WIN32)
    TARGET_CONFIG_MSVC_PCH(pywinrt pch.cpp pch.h)
    target_compile_options(pywinrt PUBLIC /await)
    target_link_libraries(pywinrt windowsapp ole32 shlwapi)
else()
    target_link_libraries(pywinrt c++ c++abi c++experimental)
    target_link_libraries(pywinrt -lpthread)
endif()

file(TO_NATIVE_PATH "${CMAKE_CURRENT_BINARY_DIR}/pywinrt.exe" pywinrt_exe)
set_target_properties(pywinrt PROPERTIES "pywinrt_exe" ${pywinrt_exe})
