set(SOURCES
  fstreambuf_tests.cpp
  istream_tests.cpp
  memstream_tests.cpp
  ostream_tests.cpp
  stdstream_tests.cpp
)
if(WINDOWS_STORE OR WINDOWS_PHONE)
  list(APPEND SOURCES winrt_interop_tests.cpp)
else()
  list(APPEND SOURCES fuzz_tests.cpp)
  if(WIN32)
    list(APPEND SOURCES CppSparseFile.cpp)
  endif()
endif()

add_casablanca_test(streams_test SOURCES)
if(NOT WIN32 OR CPPREST_WEBSOCKETS_IMPL STREQUAL "wspp")
  cpprest_find_boost()
  if(NOT TEST_LIBRARY_TARGET_TYPE STREQUAL "OBJECT")
    target_link_libraries(streams_test PRIVATE cpprestsdk_boost_internal)
  else()
    target_include_directories(streams_test PRIVATE $<TARGET_PROPERTY:cpprestsdk_boost_internal,INTERFACE_INCLUDE_DIRECTORIES>)
  endif()
endif()

configure_pch(streams_test stdafx.h stdafx.cpp)
