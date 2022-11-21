
if (WIN32)
    add_custom_target(foundation_metadata
        COMMAND ilasm /DLL /mdv="WindowsRuntime 1.4" /output=${CMAKE_CURRENT_BINARY_DIR}/Foundation.xmeta ${CMAKE_CURRENT_SOURCE_DIR}/xlang_foundation.il
        DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/xlang_foundation.il
        BYPRODUCTS ${CMAKE_CURRENT_BINARY_DIR}/Foundation.xmeta
    )
    set_target_properties(foundation_metadata PROPERTIES
        Foundation_xmeta ${CMAKE_CURRENT_BINARY_DIR}/Foundation.xmeta
    )
endif()
