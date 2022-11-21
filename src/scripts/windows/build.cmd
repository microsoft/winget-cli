@echo off
SETLOCAL

REM https://stackoverflow.com/a/45070967

goto :init

:usage
    echo xlang windows build
    echo.
    echo USAGE:
    echo   build.cmd [flags] "build target" 
    echo.
    echo.  -h, --help               shows this help
    echo.  -v, --verbose            shows detailed output
    echo.  -f, --force-cmake        forces re-run of CMake
    echo.  -b, --build-type value   specify build type (Debug, Release, RelWithDebInfo, MinSizeRel)
    echo.  --build-version value    specify build semantic version number
    goto :eof

:init
    set "OPT_HELP="
    set "OPT_VERBOSE_NINJA="
    set "OPT_VERBOSE_CMAKE="
    set "OPT_FORCE_CMAKE="
    set "BUILD_TYPE=Debug"
    set "BUILD_VERSION=0.0.0"
    set "BUILD_TARGET="

    pushd %~dp0..\..\..\
    set "REPO_ROOT_PATH=%CD%"
    popd

:parse
    if "%~1"=="" goto :main

    if /i "%~1"=="/h"         call :usage "%~2" & goto :end
    if /i "%~1"=="-h"         call :usage "%~2" & goto :end
    if /i "%~1"=="--help"     call :usage "%~2" & goto :end

    if /i "%~1"=="/v"         set "OPT_VERBOSE_NINJA=-v -d explain" & set "OPT_VERBOSE_CMAKE=-DCMAKE_VERBOSE_MAKEFILE:BOOL=ON" & shift & goto :parse
    if /i "%~1"=="-v"         set "OPT_VERBOSE_NINJA=-v -d explain" & set "OPT_VERBOSE_CMAKE=-DCMAKE_VERBOSE_MAKEFILE:BOOL=ON" & shift & goto :parse
    if /i "%~1"=="--verbose"  set "OPT_VERBOSE_NINJA=-v -d explain" & set "OPT_VERBOSE_CMAKE=-DCMAKE_VERBOSE_MAKEFILE:BOOL=ON" & shift & goto :parse

    if /i "%~1"=="/f"               set "OPT_FORCE_CMAKE=yes"  & shift & goto :parse
    if /i "%~1"=="-f"               set "OPT_FORCE_CMAKE=yes"  & shift & goto :parse
    if /i "%~1"=="--force-cmake"    set "OPT_FORCE_CMAKE=yes"  & shift & goto :parse

    if /i "%~1"=="-b"               set "BUILD_TYPE=%~2"   & shift & shift & goto :parse
    if /i "%~1"=="--build-type"     set "BUILD_TYPE=%~2"   & shift & shift & goto :parse

    if /i "%~1"=="--build-version"  set "BUILD_VERSION=%~2"   & shift & shift & goto :parse

    if not defined Target           set "BUILD_TARGET=%~1"     & shift & goto :parse

    shift
    goto :parse

:main

    set SRC_PATH=%REPO_ROOT_PATH%\src
    set BUILD_PATH=%REPO_ROOT_PATH%\_build\Windows\%VSCMD_ARG_TGT_ARCH%\%BUILD_TYPE%

    set "RUN_CMAKE="
    if defined OPT_FORCE_CMAKE                  set "RUN_CMAKE=yes"
    if not exist "%BUILD_PATH%/CMakeCache.txt"  set "RUN_CMAKE=yes"

    if defined RUN_CMAKE (
        cmake "%SRC_PATH%" "-B%BUILD_PATH%" %OPT_VERBOSE_CMAKE% -GNinja -DCMAKE_BUILD_TYPE=%BUILD_TYPE% -DCMAKE_C_COMPILER=cl -DCMAKE_CXX_COMPILER=cl -DXLANG_BUILD_VERSION=%BUILD_VERSION% "-DCMAKE_INSTALL_PREFIX=%BUILD_PATH%/Install"
    )

    ninja -C "%BUILD_PATH%" %OPT_VERBOSE_NINJA% %BUILD_TARGET% 2>&1


:end
    exit /B
