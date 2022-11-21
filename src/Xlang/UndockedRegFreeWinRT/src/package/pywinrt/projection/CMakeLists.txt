cmake_minimum_required(VERSION 3.12)

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
        get_filename_component(filename ${file} NAME)
        if (${filename} STREQUAL ${pch_cpp})
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

file(GLOB sources "${CMAKE_CURRENT_SOURCE_DIR}/pywinrt/winrt/src/*.cpp")
foreach(file ${sources})
    message(${file})
endforeach()

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# TODO: re-enable /GL and /LTCG when we get a bigger build machine
add_compile_options(/await /bigobj /GR- /permissive-)

project(_winrt)

find_package (Python3 ${PYTHON_VERSION} EXACT COMPONENTS Interpreter Development)

Python3_add_library (_winrt MODULE ${sources})

target_include_directories(_winrt PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/cppwinrt" "${CMAKE_CURRENT_SOURCE_DIR}/pywinrt/winrt/src")
target_link_libraries(_winrt PRIVATE onecore)

TARGET_CONFIG_MSVC_PCH(_winrt pch.cpp pch.h)

string(REPLACE "/MDd" "/MTd" CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG}")
foreach(build_type RELEASE MINSIZEREL RELWITHDEBINFO)
    string(REPLACE "/MD" "/MT" CMAKE_CXX_FLAGS_${build_type} "${CMAKE_CXX_FLAGS_${build_type}}")
endforeach()

string(REGEX REPLACE "/GR" "" CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}")
# TODO: re-enable /GL and /LTCG when we get a bigger build machine
# string(APPEND CMAKE_MODULE_LINKER_FLAGS " /LTCG:STATUS")

if (CMAKE_CXX_COMPILER_VERSION VERSION_GREATER_EQUAL "19.20.27404.0")
    string(APPEND CMAKE_CXX_FLAGS " /d2FH4")
endif()

install(TARGETS _winrt DESTINATION ".")
