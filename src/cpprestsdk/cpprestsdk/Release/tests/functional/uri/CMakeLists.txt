set(SOURCES
  accessor_tests.cpp
  combining_tests.cpp
  constructor_tests.cpp
  conversions_tests.cpp
  diagnostic_tests.cpp
  encoding_tests.cpp
  operator_tests.cpp
  splitting_tests.cpp
  uri_builder_tests.cpp
  resolve_uri_tests.cpp
)

add_casablanca_test(uri_test SOURCES)

configure_pch(uri_test stdafx.h stdafx.cpp)
