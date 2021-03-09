#!/bin/bash
# Copyright (C) Microsoft. All rights reserved.
# Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
# =+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+
#
# configure.sh
#
# Build script for casablanca on android
#
# For the latest on this and related APIs, please see: https://github.com/Microsoft/cpprestsdk
#
# =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

set -e

# The Android NDK r10e or later may work, but we test with r18b. To download, see the following link:
# https://developer.android.com/ndk/downloads/index.html

# -----------------
# Parse args
# -----------------

DO_BOOST=1
DO_OPENSSL=1
DO_CMAKE=1
DO_CPPRESTSDK=1

BOOSTVER=1.70.0
OPENSSLVER=1.1.0j
CMAKEVER=3.14.0

API=15
STL=c++_shared

function usage {
    echo "Usage: $0 [--skip-boost] [--skip-openssl] [--skip-cpprestsdk] [-h] [--ndk <android-ndk>]"
    echo ""
    echo "    --skip-boost          Skip fetching and compiling boost"
    echo "    --skip-openssl        Skip fetching and compiling openssl"
    echo "    --skip-cpprestsdk     Skip compiling cpprestsdk"
    echo "    --boost <version>     Override the Boost version to build (default is ${BOOSTVER})"
    echo "    --openssl <version>   Override the OpenSSL version to build (default is ${OPENSSLVER})"
    echo "    --ndk <android-ndk>   If specified, overrides the ANDROID_NDK environment variable"
    echo "    -h,--help,-?          Display this information"
}

while [[ $# > 0 ]]
do
    case $1 in
    "--skip-boost")
        DO_BOOST=0
        ;;
    "--skip-openssl")
        DO_OPENSSL=0
        ;;
    "--skip-cmake")
        DO_CMAKE=0
        ;;
    "--skip-cpprestsdk")
        DO_CPPRESTSDK=0
        ;;
    "--boost")
        shift
        DO_BOOST=1
        BOOSTVER=$1
        ;;
    "--cmake")
        shift
        DO_CMAKE=1
        CMAKEVER=$1
        ;;
    "--openssl")
        shift
        DO_OPENSSL=1
        OPENSSLVER=$1
        ;;
    "--ndk")
        shift
        export ANDROID_NDK=$1
        ;;
    "-?"|"-h"|"--help")
        usage
        exit
        ;;
    *)
        usage
        exit 1
        ;;
    esac
    shift
done

# Variables setup

unset BOOST_ROOT

if [ ! -e "${ANDROID_NDK}/ndk-build" ]
then
    echo "ANDROID_NDK does not point to a valid NDK."
    echo "(current setting: '${ANDROID_NDK}')"
    exit 1
fi
NDK_DIR=`cd "${ANDROID_NDK}" && pwd`
SRC_DIR=`pwd`

if [ -z "$NCPU" ]; then
    NCPU=4
    if uname -s | grep -i "linux" > /dev/null ; then
        NCPU=`cat /proc/cpuinfo | grep -c -i processor`
    fi
fi

# -----------------------
# Identify the script dir
# -----------------------

# source:
# http://stackoverflow.com/questions/59895/can-a-bash-script-tell-what-directory-its-stored-in

SOURCE="${BASH_SOURCE[0]}"
while [ -h "$SOURCE" ]; do # resolve $SOURCE until the file is no longer a symlink
  DIR="$( cd -P "$( dirname "$SOURCE" )" && pwd )"
  SOURCE="$(readlink "$SOURCE")"
  [[ $SOURCE != /* ]] && SOURCE="$DIR/$SOURCE" # if $SOURCE was a relative symlink, we need to resolve it relative to the path where the symlink file was located
done
DIR="$( cd -P "$( dirname "$SOURCE" )" && pwd )"

if [ "$SRC_DIR" == "$DIR" ]
then
    echo "You must use a separate directory for building."
    echo "(try 'mkdir build; cd build; ../configure.sh')"
    exit 1
fi

# -------
# Openssl
# -------

# This steps are based on the official openssl build instructions
# http://wiki.openssl.org/index.php/Android
if [ "${DO_OPENSSL}" == "1" ]; then (
    if [ ! -d "openssl" ]; then mkdir openssl; fi
    cd openssl
    cp -af "${DIR}/openssl/." .
    make all ANDROID_NDK="${NDK_DIR}" ANDROID_TOOLCHAIN=clang ANDROID_ABI=armeabi-v7a OPENSSL_PREFIX=armeabi-v7a OPENSSL_VERSION=$OPENSSLVER -j $NCPU
    make all ANDROID_NDK="${NDK_DIR}" ANDROID_TOOLCHAIN=clang ANDROID_ABI=x86 OPENSSL_PREFIX=x86 OPENSSL_VERSION=$OPENSSLVER -j $NCPU
) fi

# -----
# Boost
# -----
# Uses the build script from Moritz Wundke (formerly MysticTreeGames)
# https://github.com/moritz-wundke/Boost-for-Android
# (plus the patch https://github.com/o01eg/Boost-for-Android/tree/ndk-bump-21)

if [ "${DO_BOOST}" == "1" ]; then (
    if [ ! -d 'Boost-for-Android' ]; then git clone https://github.com/o01eg/Boost-for-Android/; fi
    cd Boost-for-Android
    git checkout 7626dd6f7cab7866dce20e685d4a1b11194366a7
    PATH="$PATH:$NDK_DIR" \
    CXXFLAGS="-std=gnu++11" \
    ./build-android.sh \
        --boost=$BOOSTVER \
        --arch=armeabi-v7a,x86 \
        --with-libraries=atomic,random,date_time,filesystem,system,thread,chrono \
        "${NDK_DIR}" || exit 1
) fi

# ------
# CMake
# ------
# We update CMake because the version included with Ubuntu is too old to handle Boost 1.69.

if [ "${DO_CMAKE}" == "1" ]; then (
    if [ ! -d "cmake-${CMAKEVER}" ]; then wget https://github.com/Kitware/CMake/releases/download/v${CMAKEVER}/cmake-${CMAKEVER}-Linux-x86_64.sh; fi
    chmod +x cmake-${CMAKEVER}-Linux-x86_64.sh
    rm -rf cmake-${CMAKEVER}
    mkdir cmake-${CMAKEVER}
    cd cmake-${CMAKEVER}
    ../cmake-${CMAKEVER}-Linux-x86_64.sh --skip-license
) fi

# ----------
# casablanca
# ----------

if [ "${DO_CPPRESTSDK}" == "1" ]; then
    # Use the builtin CMake toolchain configuration that comes with the NDK
    function build_cpprestsdk { (
    rm -rf $1
        ./cmake-${CMAKEVER}/bin/cmake \
            -DCMAKE_TOOLCHAIN_FILE="${ANDROID_NDK}/build/cmake/android.toolchain.cmake" \
            -DANDROID_NDK="${ANDROID_NDK}" \
            -DANDROID_TOOLCHAIN=clang \
            -DANDROID_ABI=$2 \
            -DBOOST_VERSION="${BOOSTVER}" \
            -DCPPREST_EXCLUDE_WEBSOCKETS=ON \
            -DCMAKE_BUILD_TYPE=$3 \
            -S "${DIR}/.." \
            -B $1
        make -j $NCPU -C $1
    ) }

    # Build the cpprestsdk for each target configuration
    build_cpprestsdk build.armv7.debug armeabi-v7a Debug
    build_cpprestsdk build.armv7.release armeabi-v7a Release
    build_cpprestsdk build.x86.debug x86 Debug
    build_cpprestsdk build.x86.release x86 Release
fi
