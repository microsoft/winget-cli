if (WIN32)
  if (WINDOWS_STORE OR WINDOWS_PHONE)
    add_definitions(-DWINRT_TEST_RUNNER -D_CONSOLE)
  else()
    add_definitions(-DDESKTOP_TEST_RUNNER)
  endif()
endif()

add_executable(test_runner test_runner.cpp test_module_loader.cpp)
target_link_libraries(test_runner PRIVATE unittestpp ${CMAKE_DL_LIBS})
if (WIN32)
  target_sources(test_runner PRIVATE test_runner.manifest)
endif()

if(BUILD_SHARED_LIBS AND NOT TEST_LIBRARY_TARGET_TYPE STREQUAL "OBJECT")
elseif(APPLE)
  target_link_libraries(test_runner PRIVATE
    -Wl,-force_load httpclient_test
    -Wl,-force_load json_test
    -Wl,-force_load uri_test
    -Wl,-force_load pplx_test
    -Wl,-force_load httplistener_test
    -Wl,-force_load streams_test
    -Wl,-force_load utils_test
    )
elseif(UNIX)
  target_link_libraries(test_runner PRIVATE
    -Wl,--whole-archive
    httpclient_test
    json_test
    uri_test
    pplx_test
    httplistener_test
    streams_test
    utils_test
    -Wl,--no-whole-archive
    )
else()
  # In order to achieve --whole-archive on windows, we link all the test files into the test_runner directly
  # This means that the tests themselves must be created as "OBJECT" libraries
  target_sources(test_runner PRIVATE
    $<TARGET_OBJECTS:httpclient_test>
    $<TARGET_OBJECTS:json_test>
    $<TARGET_OBJECTS:uri_test>
    $<TARGET_OBJECTS:pplx_test>
    $<TARGET_OBJECTS:streams_test>
    $<TARGET_OBJECTS:utils_test>
  )
  if(NOT WINDOWS_STORE AND NOT WINDOWS_PHONE)
    target_sources(test_runner PRIVATE $<TARGET_OBJECTS:httplistener_test>)
  endif()
  target_link_libraries(test_runner PRIVATE
    common_utilities
    httptest_utilities
    cpprest
  )
  if(TARGET websockettest_utilities)
      target_link_libraries(test_runner PRIVATE websockettest_utilities)
  endif()
  if(CPPREST_WEBSOCKETS_IMPL STREQUAL "wspp")
    cpprest_find_websocketpp()
    target_link_libraries(test_runner PRIVATE cpprestsdk_websocketpp_internal)
  endif()
  if (WINDOWS_STORE)
    target_link_libraries(test_runner PRIVATE ucrtd.lib vcruntimed.lib vccorlibd.lib msvcrtd.lib msvcprtd.lib concrtd.lib RuntimeObject.lib)
  endif()
endif()
