#!/bin/bash

# Copyright (c) Microsoft Corporation.
# Licensed under the MIT License.

# Synopsis: Simplifies build commands for the SFS Client.
#
# Description: This script will contain the build commands for the SFS Client. The default build folder will be "<git_root>/build".
# Use this on non-Windows platforms in a bash session.
#
# Example:
# $ ./scripts/build.sh
#

# Ensures script stops on errors
set -e

if [[ "${BASH_SOURCE[0]}" != "${0}" ]]; then
    error "Script is being sourced, it should be executed instead."
    return 1
fi

COLOR_RED="\033[1;31m"
COLOR_YELLOW="\033[1;33m"
BOLD_DEFAULT_COLOR="\033[1m"
NO_COLOR="\033[0m"

error() { echo -e "${COLOR_RED}$*${NO_COLOR}" >&2; exit 1; }
warn() { echo -e "${COLOR_YELLOW}$*${NO_COLOR}"; }

clean=false
enable_test_overrides="OFF"
build_tests="ON"
build_samples="ON"
build_type="Debug"

usage() { echo -e "Usage: $0 [-c|--clean] [-b|--build-type {Debug,Release}] [-t|--enable-test-overrides]
                          [--build-tests {${BOLD_DEFAULT_COLOR}ON${NO_COLOR}, OFF}] [--build-samples {${BOLD_DEFAULT_COLOR}ON${NO_COLOR}, OFF}]" 1>&2; exit 1; }

# Make sure when adding a new option to check if it requires CMake regeneration

if ! opts=$(getopt \
  --longoptions "clean,build-type:,enable-test-overrides,build-tests:,build-samples:" \
  --name "$(basename "$0")" \
  --options "cb:t" \
  -- "$@"
); then
    usage
fi

eval set "--$opts"

while [ $# -gt 0 ]; do
    case "$1" in
        -c|--clean)
            clean=true
            shift 1
            ;;
        -b|--build-type)
            shift 1
            build_type=$1
            case "$build_type" in
                Debug|Release)
                    ;;
                *)
                    usage
                    ;;
            esac
            shift 1
            ;;
        -t|--enable-test-overrides)
            enable_test_overrides="ON"
            shift 1
            ;;
        --build-tests)
            shift 1
            build_tests=$1
            case "$build_tests" in
                ON|OFF)
                    ;;
                *)
                    usage
                    ;;
            esac
            shift 1
            ;;
        --build-samples)
            shift 1
            build_samples=$1
            case "$build_samples" in
                ON|OFF)
                    ;;
                *)
                    usage
                    ;;
            esac
            shift 1
            ;;
        *)
            break
            ;;
    esac
done

script_dir="$(cd "$(dirname "${BASH_SOURCE[0]}")" > /dev/null 2>&1 && pwd)"
git_root=$(git -C "$script_dir" rev-parse --show-toplevel)

vcpkg_dir="$git_root/vcpkg"
if [ ! -d "$vcpkg_dir" ]; then
    error "vcpkg not found at $git_root/vcpkg. Source the setup.sh script first."
fi

build_folder="$git_root/build"
if $clean && [ -d "$build_folder" ]; then
    warn "Cleaning build folder before build..."
    rm -r -f "$build_folder"
fi

regenerate=false
cmake_cache_file="$build_folder/CMakeCache.txt"

test_cmake_cache_value_no_match() {
    local cmake_cache_file=$1
    local pattern=$2
    local expected_value=$3

    value=$(sed -nr "s/$pattern/\1/p" "$cmake_cache_file")
    if [ -n "$value" ] && [ "$value" == "$expected_value" ]; then
        return 1
    fi
    return 0
}

if [ -f "$cmake_cache_file" ]; then
    # Regenerate if one of the build options is set to a different value than the one passed in
    if test_cmake_cache_value_no_match "$cmake_cache_file" "^CMAKE_BUILD_TYPE:STRING=(.*)$" "$build_type"; then
        regenerate=true
    fi
    if test_cmake_cache_value_no_match "$cmake_cache_file" "^SFS_ENABLE_TEST_OVERRIDES:BOOL=(.*)$" "$enable_test_overrides"; then
        regenerate=true
    fi
    if test_cmake_cache_value_no_match "$cmake_cache_file" "^SFS_BUILD_TESTS:BOOL=(.*)$" "$build_tests"; then
        regenerate=true
    fi
    if test_cmake_cache_value_no_match "$cmake_cache_file" "^SFS_BUILD_SAMPLES:BOOL=(.*)$" "$build_samples"; then
        regenerate=true
    fi
fi

# Configure cmake if build folder doesn't exist or if the build must be regenerated.
# This creates build targets that will be used by the build command
if [ ! -d "$build_folder" ] || $regenerate ; then
    cmake \
        -S "$git_root" \
        -B "$build_folder" \
        -DCMAKE_BUILD_TYPE="$build_type" \
        -DSFS_ENABLE_TEST_OVERRIDES="$enable_test_overrides" \
        -DSFS_BUILD_TESTS="$build_tests" \
        -DSFS_BUILD_SAMPLES="$build_samples"
fi

# This is the build command. If any CMakeLists.txt files change, this will also reconfigure before building
cmake --build "$build_folder"
