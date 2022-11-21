#!/bin/bash

# get current script directory in bash : https://stackoverflow.com/a/246128 
CURRENT_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" >/dev/null && pwd )"

# parse command line arguments with getopt : https://stackoverflow.com/a/29754866
set -o errexit -o pipefail -o noclobber -o nounset # saner programming env: these switches turn some bugs into errors

! getopt --test > /dev/null 
if [[ ${PIPESTATUS[0]} -ne 4 ]]; then
    echo "I’m sorry, getopt --test failed in this environment."
    exit 1
fi

OPTIONS=hvfb:t:
LONGOPTS=help,verbose,force-cmake,build-type:,target:

# -use ! and PIPESTATUS to get exit code with errexit set
# -temporarily store output to be able to check for errors
# -activate quoting/enhanced mode (e.g. by writing out “--options”)
# -pass arguments only via   -- "$@"   to separate them correctly
! PARSED=$(getopt --options=$OPTIONS --longoptions=$LONGOPTS --name "$0" -- "$@")
if [[ ${PIPESTATUS[0]} -ne 0 ]]; then
    # e.g. return value is 1
    #  then getopt has complained about wrong arguments to stdout
    exit 2
fi
# read getopt’s output this way to handle the quoting right:
eval set -- "$PARSED"

help=false force=false verbose="" buildType=Debug target=""
# now enjoy the options in order and nicely split until we see --
while true; do
    case "$1" in
        -h|--help)
            help=true
            shift
            ;;
        -f|--force-cmake)
            force=true
            shift
            ;;
        -v|--verbose)
            verbose="-v"
            shift
            ;;
        -b|--build-type)
            buildType="$2"
            shift 2
            ;;
        -t|--target)
            target="$2"
            shift 2
            ;;
        --)
            shift
            break
            ;;
        *)
            echo "Programming error"
            exit 3
            ;;
    esac
done

if $help
then
    echo xlang ubuntu build
    echo
    echo USAGE:
    echo   bash build.sh [flags] "build target" 
    echo
    echo  -h, --help               shows this help
    echo  -v, --verbose            shows detailed output
    echo  -f, --force-cmake        forces re-run of CMake
    echo  -b, --build-type value   specify build type "(Debug, Release, RelWithDebInfo, MinSizeRel)"
    exit
fi

SRC_SCRIPTS_PATH="$(dirname "$CURRENT_DIR")"
SRC_PATH="$(dirname "$SRC_SCRIPTS_PATH")"
REPO_ROOT_PATH="$(dirname "$SRC_PATH")"
TARGET=$(clang -v 2>&1 | awk '/Target:/ { print $2;}')
TARGET=${TARGET:-unknown} 
BUILD_PATH="$REPO_ROOT_PATH/_build/$TARGET/$buildType/"

if $force || ! [ -e "$BUILD_PATH/CMakeCache.txt" ]
then
  cmake "$SRC_PATH" "-B$BUILD_PATH" -GNinja "-DCMAKE_BUILD_TYPE=$buildType" -DCMAKE_C_COMPILER=clang -DCMAKE_CXX_COMPILER=clang++ "-DCMAKE_INSTALL_PREFIX=$BUILD_PATH/Install"
fi

ninja -C $BUILD_PATH $verbose $target
