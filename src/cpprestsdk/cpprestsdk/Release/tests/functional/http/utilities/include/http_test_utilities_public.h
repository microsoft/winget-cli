/***
 * Copyright (C) Microsoft. All rights reserved.
 * Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
 *
 * =+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+
 *
 * http_test_utilities_public.h -- Common definitions for public http test utility headers
 *
 * =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
 ****/

#pragma once

#if !defined(_WIN32) && !defined(__cplusplus_winrt)
#define TEST_UTILITY_API
#endif // !_WIN32 && !__cplusplus_winrt

#ifndef TEST_UTILITY_API
#ifdef HTTPTESTUTILITY_EXPORTS
#define TEST_UTILITY_API __declspec(dllexport)
#else // HTTPTESTUTILITIES_EXPORTS
#define TEST_UTILITY_API __declspec(dllimport)
#endif // HTTPTESTUTILITIES_EXPORTS
#endif // TEST_UTILITY_API
