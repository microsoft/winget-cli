cmake_minimum_required(VERSION 3.9)

# The Microsoft.Windows.CppWinRT NuGet package is only targeted at Visual Studio (on Windows)
if (WIN32 AND ("$ENV{VSCMD_ARG_TGT_ARCH}" STREQUAL "x86"))

file(TO_NATIVE_PATH "${CMAKE_BINARY_DIR}/build_tools/nuget.exe" nuget_exe)
file(TO_NATIVE_PATH "${CMAKE_CURRENT_BINARY_DIR}" pywinrt_nupkg_dir)
file(TO_NATIVE_PATH "${pywinrt_nupkg_dir}/Microsoft.Windows.PyWinRT.${XLANG_BUILD_VERSION}.nupkg" pywinrt_nupkg)

get_target_property(pywinrt_exe pywinrt "pywinrt_exe")

file(DOWNLOAD https://dist.nuget.org/win-x86-commandline/latest/nuget.exe ${nuget_exe})

add_custom_command(OUTPUT ${pywinrt_nupkg}
    COMMAND ${nuget_exe} pack ${CMAKE_CURRENT_SOURCE_DIR}/Microsoft.Windows.PyWinRT.nuspec -Properties pywinrt_exe=${pywinrt_exe}; -Version ${XLANG_BUILD_VERSION} -OutputDirectory ${CMAKE_CURRENT_BINARY_DIR} -NonInteractive -Verbosity Detailed
    DEPENDS ${XLANG_BUILD_VERSION_h} ${CMAKE_CURRENT_SOURCE_DIR}/Microsoft.Windows.PyWinRT.nuspec
)

add_custom_target(make_pywinrt_nupkg ALL DEPENDS ${pywinrt_nupkg} pywinrt)

set_target_properties(make_pywinrt_nupkg PROPERTIES "pywinrt_nupkg_dir" ${pywinrt_nupkg_dir})

endif()