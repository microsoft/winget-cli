set(XLANG_TEST_INC_PATH "${CMAKE_CURRENT_SOURCE_DIR}/inc")

add_subdirectory(platform)
add_subdirectory(abi_component)
add_subdirectory(library)

if (WIN32)
    add_subdirectory(python)
endif()

