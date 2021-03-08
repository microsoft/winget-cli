set(SOURCES
  pplx_op_test.cpp
  pplx_task_options.cpp
  pplxtask_tests.cpp
)

add_casablanca_test(pplx_test SOURCES)

configure_pch(pplx_test stdafx.h stdafx.cpp)
