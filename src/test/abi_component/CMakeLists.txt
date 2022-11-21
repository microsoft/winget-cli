add_library(abi_test_component MODULE widget.cpp module.def)
target_include_directories(abi_test_component PRIVATE 
    ${CMAKE_CURRENT_SOURCE_DIR} 
    "${CMAKE_SOURCE_DIR}/platform/helpers")
CONSUME_PAL(abi_test_component)
set_target_properties(abi_test_component PROPERTIES OUTPUT_NAME "AbiComponent")

install(TARGETS abi_test_component DESTINATION "test/platform")
if (WIN32)
    install(FILES $<TARGET_PDB_FILE:abi_test_component> DESTINATION "test/platform" OPTIONAL)
endif ()
