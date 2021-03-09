set(SOURCES
  http_asserts.cpp
  test_http_client.cpp
  test_http_server.cpp
  test_server_utilities.cpp
)

add_library(httptest_utilities ${SOURCES})
if(WIN32)
  target_compile_definitions(httptest_utilities PRIVATE -DHTTPTESTUTILITY_EXPORTS)
endif()
target_include_directories(httptest_utilities PUBLIC include)
target_link_libraries(httptest_utilities PUBLIC
  cpprest
  unittestpp
  common_utilities
)
if(WINDOWS_STORE)
  target_compile_options(httptest_utilities PRIVATE /DWINAPI_FAMILY=WINAPI_FAMILY_DESKTOP_APP)
endif()
