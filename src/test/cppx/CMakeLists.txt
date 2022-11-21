project(test_cppx)

add_executable(test_cppx "")
target_sources(test_cppx
    PRIVATE pch.cpp
    hstring.cpp
)

if (WIN32)
    target_sources(test_cppx
        PRIVATE
        collection_base.cpp
        error.cpp
    #    param_iterable.cpp
    #    param_map.cpp
    #    param_map_view.cpp
    #    param_vector.cpp
    #    param_vector_view.cpp
    #    produce_map.cpp
    #    produce_vector.cpp
    #    single_threaded_map.cpp
    #    single_threaded_vector.cpp
    )
endif ()

target_include_directories(test_cppx
    PRIVATE ${XLANG_LIBRARY_PATH} ${XLANG_TEST_INC_PATH} ${CMAKE_CURRENT_BINARY_DIR})

target_link_libraries(test_cppx pal)
RPATH_ORIGIN(test_cppx)

if (MSVC)
    TARGET_CONFIG_MSVC_PCH(test_cppx pch.cpp pch.h)
    target_link_libraries(test_cppx windowsapp ole32)
endif()

target_sources(test_cppx PUBLIC
    main.cpp
    IXlangObject.cpp
)

install(TARGETS test_cppx DESTINATION "test/platform")
if (WIN32)
    install(FILES $<TARGET_PDB_FILE:test_cppx> DESTINATION "test/platform" OPTIONAL)
endif ()

if (WIN32)
    get_target_property(xmeta_path foundation_metadata Foundation_xmeta)
    add_custom_target(test_cppx_base_projection
        COMMAND cppxlang -base -in ${xmeta_path} -out ${CMAKE_CURRENT_BINARY_DIR}
        DEPENDS ${foundation_metadata}
        BYPRODUCTS ${CMAKE_CURRENT_BINARY_DIR}/xlang/base.h
    )
    add_dependencies(test_cppx_base_projection foundation_metadata)
else ()
    add_custom_target(test_cppx_base_projection
        COMMAND cppxlang -base -out ${CMAKE_CURRENT_BINARY_DIR}
        BYPRODUCTS ${CMAKE_CURRENT_BINARY_DIR}/xlang/base.h
    )
endif ()

add_dependencies(test_cppx test_cppx_base_projection)
