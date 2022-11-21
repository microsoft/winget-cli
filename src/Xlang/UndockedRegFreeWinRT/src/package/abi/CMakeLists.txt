cmake_minimum_required(VERSION 3.9)

# The Microsoft.Windows.AbiWinRT NuGet package is only targeted at Visual Studio (on Windows)
if (WIN32 AND ("$ENV{VSCMD_ARG_TGT_ARCH}" STREQUAL "x86"))

file(TO_NATIVE_PATH "${CMAKE_BINARY_DIR}/build_tools/nuget.exe" nuget_exe)
file(TO_NATIVE_PATH "${CMAKE_CURRENT_BINARY_DIR}" abiwinrt_nupkg_dir)
file(TO_NATIVE_PATH "${abiwinrt_nupkg_dir}/Microsoft.Windows.AbiWinRT.${XLANG_BUILD_VERSION}.nupkg" abiwinrt_nupkg)

get_target_property(abiwinrt_exe abi "abiwinrt_exe")

file(DOWNLOAD https://dist.nuget.org/win-x86-commandline/latest/nuget.exe ${nuget_exe})

add_custom_command(OUTPUT ${abiwinrt_nupkg}
    COMMAND ${nuget_exe} pack ${CMAKE_CURRENT_SOURCE_DIR}/Microsoft.Windows.AbiWinRT.nuspec -Properties abiwinrt_exe=${abiwinrt_exe}; -Version ${XLANG_BUILD_VERSION} -OutputDirectory ${CMAKE_CURRENT_BINARY_DIR} -NonInteractive -Verbosity Detailed
    DEPENDS ${XLANG_BUILD_VERSION_h} ${CMAKE_CURRENT_SOURCE_DIR}/Microsoft.Windows.AbiWinRT.nuspec
)

add_custom_target(make_abiwinrt_nupkg ALL DEPENDS ${abiwinrt_nupkg} abi)

set_target_properties(make_abiwinrt_nupkg PROPERTIES "abiwinrt_nupkg_dir" ${abiwinrt_nupkg_dir})

endif()