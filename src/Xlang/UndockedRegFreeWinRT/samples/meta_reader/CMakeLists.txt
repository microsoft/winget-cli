cmake_minimum_required(VERSION 3.9)

if (${CMAKE_VERSION} VERSION_GREATER_EQUAL "3.13")
    cmake_policy(SET CMP0076 NEW)
endif()

project(meta_reader)

set(XLANG_LIBRARY_PATH "${CMAKE_CURRENT_SOURCE_DIR}/../../src/library")

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_executable(meta_reader "")
target_sources(meta_reader PUBLIC main.cpp pch.cpp)
target_include_directories(meta_reader PUBLIC ${XLANG_LIBRARY_PATH})

function(ADD_OBJECT_DEPENDS file dependency)
    get_source_file_property(cur_obj_depends ${file} OBJECT_DEPENDS)
    if (${cur_obj_depends} STREQUAL "NOTFOUND")
        set(new_obj_depends "${pch_output_file_name}")
    else()
        set(new_obj_depends "${cur_obj_depends};${dependency}")
    endif()
    set_source_files_properties(${file}  PROPERTIES OBJECT_DEPENDS ${new_obj_depends})
endfunction(ADD_OBJECT_DEPENDS)

function(TARGET_CONFIG_MSVC_PCH target pch_cpp pch_header)
    get_target_property(target_name ${target} NAME)
    set(pch_output_file_name "${target_name}.pch")
    set(pch_output_file "\"${CMAKE_CURRENT_BINARY_DIR}/${pch_output_file_name}\"")

    get_target_property(sources ${target} SOURCES)
    foreach(file ${sources})
        if (${file} STREQUAL ${pch_cpp})
            set_source_files_properties(${file}
                PROPERTIES
                COMPILE_FLAGS " /Yc${pch_header} /Fp${pch_output_file}"
                OBJECT_OUTPUTS ${pch_output_file_name})
        else()
            set_source_files_properties(${file} PROPERTIES COMPILE_FLAGS " /Yu${pch_header} /Fp${pch_output_file}")
            ADD_OBJECT_DEPENDS(${file} ${pch_output_file_name})
        endif()
    endforeach()

    set_target_properties(${target} PROPERTIES PCH_OUTPUT_FILE ${pch_output_file})
    set_property(GLOBAL APPEND PROPERTY PCH_OUTPUT_FILES ${pch_output_file})
endfunction(TARGET_CONFIG_MSVC_PCH)

if (WIN32)
    add_definitions(-DNOMINMAX)
endif()

if (MSVC)
    TARGET_CONFIG_MSVC_PCH(meta_reader pch.cpp pch.h)
    target_link_libraries(meta_reader)
else()
    target_link_libraries(meta_reader c++ c++abi c++experimental)
    target_link_libraries(meta_reader -lpthread)
endif()
