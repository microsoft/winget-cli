/***
 * Copyright (C) Microsoft. All rights reserved.
 * Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
 *
 * =+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+
 *
 * Pre-compiled headers
 *
 * =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
 ****/

#pragma once

#if defined(_WIN32)
// Include first to avoid any issues with Windows.h.
#include <winsock2.h>
#endif

#if defined(_WIN32)
// Trick Boost.Asio into thinking CE, otherwise _beginthreadex will be used which is banned
// for the Windows Runtime pre VS2015. Then CreateThread will be used instead.
#if _MSC_VER < 1900
#if defined(__cplusplus_winrt)
#define UNDER_CE 1
#endif
#endif
#endif

#include "cpprest/asyncrt_utils.h"
#include "cpprest/containerstream.h"
#include "cpprest/streams.h"
#include "cpprest/uri.h"
#include "unittestpp.h"
