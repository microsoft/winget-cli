cmake_policy(SET CMP0022 NEW)

file(GLOB HEADERS_CPPREST "../include/cpprest/*.h" "../include/cpprest/*.hpp" "../include/cpprest/*.dat")
file(GLOB HEADERS_PPLX "../include/pplx/*.h" "../include/pplx/*.hpp")
file(GLOB HEADERS_DETAILS "../include/cpprest/details/*.h" "../include/cpprest/details/*.hpp" "../include/cpprest/details/*.dat" "../include/pplx/*.hpp" "../include/pplx/*.dat")
source_group("Header Files\\cpprest" FILES ${HEADERS_CPPREST})
source_group("Header Files\\pplx" FILES ${HEADERS_PPLX})
source_group("Header Files\\cpprest\\details" FILES ${HEADERS_DETAILS})

file(GLOB HEADER_PPLX_THREADPOOL "../include/pplx/threadpool.h")
list(REMOVE_ITEM HEADERS_PPLX ${HEADER_PPLX_THREADPOOL})

set(SOURCES
  ${HEADERS_CPPREST}
  ${HEADERS_PPLX}
  ${HEADERS_DETAILS}
  pch/stdafx.h
  http/client/http_client.cpp
  http/client/http_client_impl.h
  http/client/http_client_msg.cpp
  http/common/connection_pool_helpers.h
  http/common/http_compression.cpp
  http/common/http_helpers.cpp
  http/common/http_msg.cpp
  http/common/internal_http_helpers.h
  http/listener/http_listener.cpp
  http/listener/http_listener_msg.cpp
  http/listener/http_server_api.cpp
  http/listener/http_server_impl.h
  http/oauth/oauth1.cpp
  http/oauth/oauth2.cpp
  json/json.cpp
  json/json_parsing.cpp
  json/json_serialization.cpp
  uri/uri.cpp
  uri/uri_builder.cpp
  utilities/asyncrt_utils.cpp
  utilities/base64.cpp
  utilities/web_utilities.cpp
)

add_library(cpprest ${SOURCES})
target_include_directories(cpprest
  PUBLIC
    $<INSTALL_INTERFACE:include> $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include>
  PRIVATE
    pch
)

## Sub-components
# Websockets component
if(CPPREST_WEBSOCKETS_IMPL STREQUAL "none")
  target_compile_definitions(cpprest PUBLIC -DCPPREST_EXCLUDE_WEBSOCKETS=1)
elseif(CPPREST_WEBSOCKETS_IMPL STREQUAL "winrt")
  target_sources(cpprest PRIVATE
    websockets/client/ws_msg.cpp
    websockets/client/ws_client.cpp
    websockets/client/ws_client_impl.h
    websockets/client/ws_client_winrt.cpp
  )
elseif(CPPREST_WEBSOCKETS_IMPL STREQUAL "wspp")
  target_sources(cpprest PRIVATE
    websockets/client/ws_msg.cpp
    websockets/client/ws_client.cpp
    websockets/client/ws_client_impl.h
    websockets/client/ws_client_wspp.cpp
  )
  cpprest_find_websocketpp()
  target_link_libraries(cpprest PRIVATE cpprestsdk_websocketpp_internal)
  cpprest_find_boost()
  cpprest_find_openssl()
  target_link_libraries(cpprest PUBLIC cpprestsdk_boost_internal cpprestsdk_openssl_internal)
else()
  message(FATAL_ERROR "Invalid implementation")
endif()

# Compression component
if(CPPREST_EXCLUDE_COMPRESSION)
  if(NOT CPPREST_EXCLUDE_BROTLI)
    message(FATAL_ERROR "Use of Brotli requires compression to be enabled")
  endif()
  target_compile_definitions(cpprest PRIVATE -DCPPREST_EXCLUDE_COMPRESSION=1)
else()
  cpprest_find_zlib()
  target_link_libraries(cpprest PRIVATE cpprestsdk_zlib_internal)
  if(CPPREST_EXCLUDE_BROTLI)
    target_compile_definitions(cpprest PRIVATE -DCPPREST_EXCLUDE_BROTLI=1)
  else()
    cpprest_find_brotli()
  endif()
endif()

# PPLX component
if(CPPREST_PPLX_IMPL STREQUAL "apple")
  find_library(COREFOUNDATION CoreFoundation "/")
  find_library(SECURITY Security "/")
  target_link_libraries(cpprest PRIVATE ${COREFOUNDATION} ${SECURITY})
  target_sources(cpprest PRIVATE pplx/pplxapple.cpp pplx/pplx.cpp pplx/threadpool.cpp ../include/pplx/threadpool.h)
  if(CPPREST_INSTALL_HEADERS)
    install(FILES ../include/pplx/threadpool.h DESTINATION include/pplx)
  endif()
elseif(CPPREST_PPLX_IMPL STREQUAL "linux")
  target_sources(cpprest PRIVATE pplx/pplxlinux.cpp pplx/pplx.cpp pplx/threadpool.cpp ../include/pplx/threadpool.h)
  if(CPPREST_INSTALL_HEADERS)
    install(FILES ../include/pplx/threadpool.h DESTINATION include/pplx)
  endif()
elseif(CPPREST_PPLX_IMPL STREQUAL "win")
  target_sources(cpprest PRIVATE pplx/pplxwin.cpp)
  if(CPPREST_WEBSOCKETS_IMPL STREQUAL "wspp")
    target_sources(cpprest PRIVATE pplx/threadpool.cpp ../include/pplx/threadpool.h)
    if(CPPREST_INSTALL_HEADERS)
      install(FILES ../include/pplx/threadpool.h DESTINATION include/pplx)
    endif()
  endif()
elseif(CPPREST_PPLX_IMPL STREQUAL "winpplx")
  target_compile_definitions(cpprest PUBLIC -DCPPREST_FORCE_PPLX=1)
  target_sources(cpprest PRIVATE pplx/pplxwin.cpp pplx/pplx.cpp pplx/threadpool.cpp ../include/pplx/threadpool.h)
  if(CPPREST_INSTALL_HEADERS)
    install(FILES ../include/pplx/threadpool.h DESTINATION include/pplx)
  endif()
elseif(CPPREST_PPLX_IMPL STREQUAL "winrt")
  target_sources(cpprest PRIVATE pplx/pplxwin.cpp)
else()
  message(FATAL_ERROR "Invalid implementation")
endif()

# Http client component
if(CPPREST_HTTP_CLIENT_IMPL STREQUAL "asio")
  cpprest_find_boost()
  cpprest_find_openssl()
  target_compile_definitions(cpprest PUBLIC -DCPPREST_FORCE_HTTP_CLIENT_ASIO)
  target_sources(cpprest PRIVATE http/client/http_client_asio.cpp http/client/x509_cert_utilities.cpp)
  target_link_libraries(cpprest PUBLIC cpprestsdk_boost_internal cpprestsdk_openssl_internal)
elseif(CPPREST_HTTP_CLIENT_IMPL STREQUAL "winhttppal")
  cpprest_find_boost()
  cpprest_find_openssl()
  cpprest_find_winhttppal()
  target_compile_definitions(cpprest PUBLIC -DCPPREST_FORCE_HTTP_CLIENT_WINHTTPPAL)
  target_sources(cpprest PRIVATE http/client/http_client_winhttp.cpp http/client/x509_cert_utilities.cpp)
  target_link_libraries(cpprest PUBLIC cpprestsdk_boost_internal cpprestsdk_openssl_internal cpprestsdk_winhttppal_internal)
elseif(CPPREST_HTTP_CLIENT_IMPL STREQUAL "winhttp")
  target_link_libraries(cpprest PRIVATE
    Winhttp.lib
  )
  target_sources(cpprest PRIVATE http/client/http_client_winhttp.cpp)
  if(CPPREST_WEBSOCKETS_IMPL STREQUAL "wspp")
    target_sources(cpprest PRIVATE http/client/x509_cert_utilities.cpp)
  endif()
elseif(CPPREST_HTTP_CLIENT_IMPL STREQUAL "winrt")
  target_sources(cpprest PRIVATE http/client/http_client_winrt.cpp)
else()
  message(FATAL_ERROR "Invalid implementation")
endif()

# fileio streams component
if(CPPREST_FILEIO_IMPL STREQUAL "win32")
  target_sources(cpprest PRIVATE streams/fileio_win32.cpp)
elseif(CPPREST_FILEIO_IMPL STREQUAL "winrt")
  target_sources(cpprest PRIVATE streams/fileio_winrt.cpp)
elseif(CPPREST_FILEIO_IMPL STREQUAL "posix")
  target_sources(cpprest PRIVATE streams/fileio_posix.cpp)
else()
  message(FATAL_ERROR "Invalid implementation")
endif()

# http listener component
if(CPPREST_HTTP_LISTENER_IMPL STREQUAL "asio")
  cpprest_find_boost()
  cpprest_find_openssl()
  target_compile_definitions(cpprest PUBLIC -DCPPREST_FORCE_HTTP_LISTENER_ASIO)
  target_sources(cpprest PRIVATE http/listener/http_server_asio.cpp)
  target_link_libraries(cpprest PUBLIC cpprestsdk_boost_internal cpprestsdk_openssl_internal)
elseif(CPPREST_HTTP_LISTENER_IMPL STREQUAL "httpsys")
  target_sources(cpprest PRIVATE
    http/listener/http_server_httpsys.cpp
    http/listener/http_server_httpsys.h
  )
  target_link_libraries(cpprest PRIVATE
    httpapi.lib
  )
elseif(CPPREST_HTTP_LISTENER_IMPL STREQUAL "none")
else()
  message(FATAL_ERROR "Invalid implementation")
endif()

configure_pch(cpprest stdafx.h pch/stdafx.cpp /Zm120)

if(CMAKE_CXX_COMPILER_ID MATCHES "Clang" OR CMAKE_CXX_COMPILER_ID MATCHES "GNU")
  if(WERROR)
    target_compile_options(cpprest PRIVATE -Werror)
  endif()
  target_compile_options(cpprest PRIVATE -pedantic ${WARNINGS})
elseif(CMAKE_CXX_COMPILER_ID MATCHES "MSVC")
  if(WERROR)
    target_compile_options(cpprest PRIVATE /WX ${WARNINGS})
  endif()
else()
  message(FATAL_ERROR "Unknown compiler")
endif()

if(WIN32)
  if (BUILD_SHARED_LIBS)
    target_compile_definitions(cpprest PRIVATE -D_ASYNCRT_EXPORT -D_PPLX_EXPORT -D_USRDLL)
  else()
    target_compile_definitions(cpprest PUBLIC -D_NO_ASYNCRTIMP -D_NO_PPLXIMP)
  endif()
elseif(ANDROID)
  target_link_libraries(cpprest PRIVATE ${ANDROID_STL_FLAGS})
endif()

if (WIN32 AND NOT WINDOWS_STORE AND NOT WINDOWS_PHONE)
  target_link_libraries(cpprest PRIVATE
    bcrypt.lib
    crypt32.lib
  )
elseif(WINDOWS_STORE)
  if(NOT CMAKE_GENERATOR MATCHES "Visual Studio .*")
    target_compile_definitions(cpprest PRIVATE -DWINAPI_FAMILY=WINAPI_FAMILY_PC_APP)
    get_target_property(LINK_FLAGS cpprest LINK_FLAGS)
    if(NOT LINK_FLAGS)
        set(LINK_FLAGS "")
    endif()
    set(LINK_FLAGS "${LINK_FLAGS} /APPCONTAINER")
    set_target_properties(cpprest PROPERTIES LINK_FLAGS "${LINK_FLAGS}")
  endif()
endif()

set_target_properties(cpprest PROPERTIES OUTPUT_NAME "cpprest${CPPREST_ABI_TAG}")
if(WIN32)
elseif(ANDROID)
  # Do not use SOVERSION on android. It is completely unsupported (and causes problems).
  # Perhaps revisit in the future? (NDK r9d, 8/7/14)
else()
  set_target_properties(cpprest PROPERTIES
    SOVERSION ${CPPREST_VERSION_MAJOR}.${CPPREST_VERSION_MINOR})
endif()

if(CPPREST_INSTALL_HEADERS)
  install(FILES ${HEADERS_CPPREST} DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/cpprest)
  install(FILES ${HEADERS_PPLX} DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/pplx)
  install(FILES ${HEADERS_DETAILS} DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/cpprest/details)
endif()

if(CPPREST_INSTALL)
  set(CPPREST_USES_BOOST OFF)
  set(CPPREST_USES_ZLIB OFF)
  set(CPPREST_USES_BROTLI OFF)
  set(CPPREST_USES_OPENSSL OFF)
  set(CPPREST_USES_WINHTTPPAL OFF)

  set(CPPREST_TARGETS cpprest)
  if(TARGET cpprestsdk_boost_internal)
    list(APPEND CPPREST_TARGETS cpprestsdk_boost_internal)
    set(CPPREST_USES_BOOST ON)
  endif()
  if(TARGET cpprestsdk_zlib_internal)
    list(APPEND CPPREST_TARGETS cpprestsdk_zlib_internal)
    set(CPPREST_USES_ZLIB ON)
  endif()
  if(TARGET cpprestsdk_brotli_internal)
    list(APPEND CPPREST_TARGETS cpprestsdk_brotli_internal)
    set(CPPREST_USES_BROTLI ON)
  endif()
  if(TARGET cpprestsdk_openssl_internal)
    list(APPEND CPPREST_TARGETS cpprestsdk_openssl_internal)
    set(CPPREST_USES_OPENSSL ON)
  endif()
  if(TARGET cpprestsdk_winhttppal_internal)
    list(APPEND CPPREST_TARGETS cpprestsdk_winhttppal_internal)
    set(CPPREST_USES_WINHTTPPAL ON)
  endif()
  if(TARGET cpprestsdk_websocketpp_internal)
    list(APPEND CPPREST_TARGETS cpprestsdk_websocketpp_internal)
  endif()
  install(
    TARGETS ${CPPREST_TARGETS}
    EXPORT cpprestsdk-targets
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
  )

  configure_file(../cmake/cpprestsdk-config.in.cmake "${CMAKE_CURRENT_BINARY_DIR}/cpprestsdk-config.cmake" @ONLY)
  configure_file(../cmake/cpprestsdk-config-version.in.cmake "${CMAKE_CURRENT_BINARY_DIR}/cpprestsdk-config-version.cmake" @ONLY)

  install(
    FILES "${CMAKE_CURRENT_BINARY_DIR}/cpprestsdk-config.cmake" "${CMAKE_CURRENT_BINARY_DIR}/cpprestsdk-config-version.cmake"
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/${CPPREST_EXPORT_DIR}
  )
  install(
    EXPORT cpprestsdk-targets
    FILE cpprestsdk-targets.cmake
    NAMESPACE cpprestsdk::
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/${CPPREST_EXPORT_DIR}
  )
endif()
