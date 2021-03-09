include_directories(include)

if(WIN32)
  add_definitions(-DCOMMONUTILITIES_EXPORTS)
endif()

add_library(common_utilities
  os_utilities.cpp
  )

if(NOT BUILD_SHARED_LIBS)
  target_compile_definitions(common_utilities INTERFACE -DTEST_UTILITY_API=)
endif()

target_link_libraries(common_utilities
  cpprest
  unittestpp
)
