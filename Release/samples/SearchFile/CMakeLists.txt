if (NOT WINDOWS_STORE AND NOT WINDOWS_PHONE)
  add_executable(SearchFile searchfile.cpp)
  target_link_libraries(SearchFile cpprest)
endif()
