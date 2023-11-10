/***
 * Copyright (C) Microsoft. All rights reserved.
 * Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
 *
 * =+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+
 *
 * Pre-compiled headers
 *
 * For the latest on this and related APIs, please see: https://github.com/Microsoft/cpprestsdk
 *
 * =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
 ****/

#pragma once

#if defined(__clang__)
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wunused-local-typedef"
#pragma clang diagnostic ignored "-Winfinite-recursion"
#endif

#ifdef _WIN32
// use the debug version of the CRT if _DEBUG is defined
#ifdef _DEBUG
#define _CRTDBG_MAP_ALLOC
#include <crtdbg.h>
#endif // _DEBUG

#include <SDKDDKVer.h>
#define WIN32_LEAN_AND_MEAN // Exclude rarely-used stuff from Windows headers

#if CPPREST_TARGET_XP && _WIN32_WINNT != 0x0501
#error CPPREST_TARGET_XP implies _WIN32_WINNT == 0x0501
#endif // CPPREST_TARGET_XP && _WIN32_WINNT != 0x0501

#include <objbase.h>

#include <windows.h>

// Windows Header Files:
#ifndef __cplusplus_winrt
#include <winhttp.h>
#endif // !__cplusplus_winrt

#else // LINUX or APPLE
#define __STDC_LIMIT_MACROS
#include "pthread.h"
#include <atomic>
#include <cstdint>
#include <signal.h>
#include <stdint.h>
#include <string>
#if (defined(ANDROID) || defined(__ANDROID__)) && !defined(_LIBCPP_VERSION)
// Boost doesn't recognize libstdcpp on top of clang correctly
#include "boost/config.hpp"
#include "boost/config/stdlib/libstdcpp3.hpp"
#undef BOOST_NO_CXX11_SMART_PTR
#undef BOOST_NO_CXX11_NULLPTR
#endif
#include "boost/bind/bind.hpp"
#include "boost/thread/condition_variable.hpp"
#include "boost/thread/mutex.hpp"
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/syscall.h>
#include <unistd.h>
#endif // _WIN32

// Macro indicating the C++ Rest SDK product itself is being built.
// This is to help track how many developers are directly building from source themselves.
#define _CASA_BUILD_FROM_SRC

#include "cpprest/details/basic_types.h"
#include "cpprest/details/cpprest_compat.h"
#include "cpprest/version.h"
#include "pplx/pplxtasks.h"
#include <algorithm>
#include <array>
#include <assert.h>
#include <atomic>
#include <exception>
#include <memory>
#include <mutex>
#include <stdlib.h>
#include <vector>

// json
#include "cpprest/json.h"

// uri
#include "cpprest/base_uri.h"

// utilities
#include "cpprest/asyncrt_utils.h"
#include "cpprest/details/web_utilities.h"

// http
#include "cpprest/details/http_helpers.h"
#include "cpprest/http_client.h"
#include "cpprest/http_headers.h"
#include "cpprest/http_msg.h"

// oauth
#if !defined(_WIN32) || _WIN32_WINNT >= _WIN32_WINNT_VISTA
#include "cpprest/oauth1.h"
#endif
#include "cpprest/oauth2.h"

#if !defined(__cplusplus_winrt)
#if _WIN32_WINNT >= _WIN32_WINNT_VISTA
#include "cpprest/details/http_server.h"
#include "cpprest/details/http_server_api.h"
#include "cpprest/http_listener.h"
#endif // _WIN32_WINNT >= _WIN32_WINNT_VISTA
#endif

#if defined(__clang__)
#pragma clang diagnostic pop
#endif
