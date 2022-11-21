project(test_library)

add_executable(test_library "")
target_sources(test_library
    PUBLIC pch.cpp text_writer.cpp)

target_include_directories(test_library
    PUBLIC ${XLANG_LIBRARY_PATH} ${XLANG_TEST_INC_PATH})

RPATH_ORIGIN(test_library)

if (MSVC)
    TARGET_CONFIG_MSVC_PCH(test_library pch.cpp pch.h)
    target_link_libraries(test_library windowsapp ole32)
else()
    target_link_libraries(test_library c++ c++abi c++experimental)
    target_link_libraries(test_library -lpthread)
endif()

target_sources(test_library PUBLIC main.cpp)

install(TARGETS test_library DESTINATION "test/library")
