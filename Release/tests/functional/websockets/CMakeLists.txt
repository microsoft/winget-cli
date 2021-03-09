if (NOT CPPREST_EXCLUDE_WEBSOCKETS)
  add_library(websockettest_utilities utilities/test_websocket_server.cpp)
  target_include_directories(websockettest_utilities PUBLIC utilities)
  target_compile_definitions(websockettest_utilities PRIVATE -DWEBSOCKETTESTUTILITY_EXPORTS)
  if(NOT WIN32)
    target_compile_definitions(websockettest_utilities PRIVATE "-DWEBSOCKET_UTILITY_API=__attribute__ ((visibility (\"default\")))")
    target_compile_definitions(websockettest_utilities INTERFACE "-DWEBSOCKET_UTILITY_API=")
  endif()

  cpprest_find_websocketpp()
  target_link_libraries(websockettest_utilities
    PRIVATE
      cpprest
      unittestpp
      common_utilities
      cpprestsdk_websocketpp_internal
  )

  # websocketsclient_test
  set(SOURCES
    client/authentication_tests.cpp
    client/client_construction.cpp
    client/close_tests.cpp
    client/error_tests.cpp
    client/receive_msg_tests.cpp
    client/send_msg_tests.cpp
    client/stdafx.cpp
  )

  add_casablanca_test(websocketsclient_test SOURCES)
  if(NOT TEST_LIBRARY_TARGET_TYPE STREQUAL "OBJECT")
    target_link_libraries(websocketsclient_test PRIVATE websockettest_utilities)
  endif()
  target_include_directories(websocketsclient_test PRIVATE utilities)
endif()
