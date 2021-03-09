set(SOURCES
  datetime.cpp
  base64.cpp
  strings.cpp
  macro_test.cpp
  nonce_generator_tests.cpp
  win32_encryption_tests.cpp
)

add_casablanca_test(utils_test SOURCES)

if(CMAKE_COMPILER_IS_GNUCXX)
  target_compile_options(utils_test PRIVATE "-Wno-deprecated-declarations")
endif()

configure_pch(utils_test stdafx.h stdafx.cpp)
