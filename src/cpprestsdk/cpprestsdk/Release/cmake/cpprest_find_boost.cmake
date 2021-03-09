macro(cpprestsdk_find_boost_android_package)
  set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE NEVER)
  set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY NEVER)
  set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
  if(CMAKE_HOST_WIN32)
    set(WIN32 1)
    set(UNIX)
  elseif(CMAKE_HOST_APPLE)
    set(APPLE 1)
    set(UNIX)
  endif()
  find_package(${ARGN})
  set(APPLE)
  set(WIN32)
  set(UNIX 1)
  set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)
  set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
  set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM ONLY)
endmacro()

function(cpprest_find_boost)
  if(TARGET cpprestsdk_boost_internal)
    return()
  endif()

  if(IOS)
    if (EXISTS "${PROJECT_SOURCE_DIR}/../Build_iOS/boost")
      set(IOS_SOURCE_DIR "${PROJECT_SOURCE_DIR}/../Build_iOS")
      set(Boost_LIBRARIES "${IOS_SOURCE_DIR}/boost/lib" CACHE INTERNAL "")
      set(Boost_INCLUDE_DIR "${IOS_SOURCE_DIR}/boost/include" CACHE INTERNAL "")
    else()
      set(IOS_SOURCE_DIR "${PROJECT_SOURCE_DIR}/../Build_iOS")
      set(Boost_LIBRARIES "${IOS_SOURCE_DIR}/boost.framework/boost" CACHE INTERNAL "")
      set(Boost_INCLUDE_DIR "${IOS_SOURCE_DIR}/boost.framework/Headers" CACHE INTERNAL "")
    endif()
  elseif(ANDROID)
    set(Boost_COMPILER "-clang")
    if(ANDROID_ABI STREQUAL "armeabi-v7a")
      set(BOOST_ROOT "${CMAKE_BINARY_DIR}/../Boost-for-Android/build/out/armeabi-v7a" CACHE INTERNAL "")
      set(BOOST_LIBRARYDIR "${CMAKE_BINARY_DIR}/../Boost-for-Android/build/out/armeabi-v7a/lib" CACHE INTERNAL "")
      set(Boost_ARCHITECTURE "-a32" CACHE INTERNAL "")
    else()
      set(BOOST_ROOT "${CMAKE_BINARY_DIR}/../Boost-for-Android/build/out/x86" CACHE INTERNAL "")
      set(BOOST_LIBRARYDIR "${CMAKE_BINARY_DIR}/../Boost-for-Android/build/out/x86/lib" CACHE INTERNAL "")
      set(Boost_ARCHITECTURE "-x32" CACHE INTERNAL "")
    endif()
    cpprestsdk_find_boost_android_package(Boost ${BOOST_VERSION} EXACT REQUIRED COMPONENTS random system thread filesystem chrono atomic)
  elseif(UNIX)
    find_package(Boost REQUIRED COMPONENTS random system thread filesystem chrono atomic date_time regex)
  else()
    find_package(Boost REQUIRED COMPONENTS system date_time regex)
  endif()

  add_library(cpprestsdk_boost_internal INTERFACE)
  # FindBoost continually breaks imported targets whenever boost updates.
  if(1)
    target_include_directories(cpprestsdk_boost_internal INTERFACE "$<BUILD_INTERFACE:${Boost_INCLUDE_DIR}>")
    set(_prev)
    set(_libs)
    foreach(_lib ${Boost_LIBRARIES})
      if(_lib STREQUAL "optimized" OR _lib STREQUAL "debug")
      else()
        if(_prev STREQUAL "optimized")
          list(APPEND _libs "$<$<NOT:$<CONFIG:Debug>>:${_lib}>")
        elseif(_prev STREQUAL "debug")
          list(APPEND _libs "$<$<CONFIG:Debug>:${_lib}>")
        else()
          list(APPEND _libs "${_lib}")
        endif()
      endif()
      set(_prev "${_lib}")
    endforeach()
    if (NOT IOS OR NOT EXISTS "${PROJECT_SOURCE_DIR}/../Build_iOS/boost")
      target_link_libraries(cpprestsdk_boost_internal INTERFACE "$<BUILD_INTERFACE:${_libs}>")
    endif()
  else()
    if(ANDROID)
      target_link_libraries(cpprestsdk_boost_internal INTERFACE
        Boost::boost
        Boost::random
        Boost::system
        Boost::thread
        Boost::filesystem
        Boost::chrono
        Boost::atomic
      )
    elseif(UNIX)
      target_link_libraries(cpprestsdk_boost_internal INTERFACE
        Boost::boost
        Boost::random
        Boost::system
        Boost::thread
        Boost::filesystem
        Boost::chrono
        Boost::atomic
        Boost::date_time
        Boost::regex
      )
    else()
      target_link_libraries(cpprestsdk_boost_internal INTERFACE
        Boost::boost
        Boost::system
        Boost::date_time
        Boost::regex
      )
    endif()
  endif()
endfunction()
