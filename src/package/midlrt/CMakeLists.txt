cmake_minimum_required(VERSION 3.9)

# The Microsoft.Windows.MidlRT NuGet package is only targeted at Visual Studio (on Windows)
if (WIN32 AND ("$ENV{VSCMD_ARG_TGT_ARCH}" STREQUAL "x86"))

file(TO_NATIVE_PATH "${CMAKE_BINARY_DIR}/build_tools/nuget.exe" nuget_exe)
file(TO_NATIVE_PATH "${CMAKE_CURRENT_BINARY_DIR}" midlrt_nupkg_dir)
file(TO_NATIVE_PATH "${midlrt_nupkg_dir}/Microsoft.Windows.midlrt.${XLANG_BUILD_VERSION}.nupkg" midlrt_nupkg)

file(DOWNLOAD https://dist.nuget.org/win-x86-commandline/latest/nuget.exe ${nuget_exe})

add_custom_command(OUTPUT ${midlrt_nupkg}
    COMMAND ${nuget_exe} pack ${CMAKE_CURRENT_SOURCE_DIR}/Microsoft.Windows.midlrt.nuspec -Properties -Version ${XLANG_BUILD_VERSION} -OutputDirectory ${CMAKE_CURRENT_BINARY_DIR} -NonInteractive -Verbosity Detailed
    DEPENDS ${XLANG_BUILD_VERSION_h} ${CMAKE_CURRENT_SOURCE_DIR}/Microsoft.Windows.midlrt.nuspec
)

add_custom_target(make_midlrt_nupkg ALL DEPENDS ${midlrt_nupkg} abi)

set_target_properties(make_midlrt_nupkg PROPERTIES "midlrt_nupkg_dir" ${midlrt_nupkg_dir})

endif()