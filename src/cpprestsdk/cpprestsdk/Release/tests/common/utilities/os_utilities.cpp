/***
 * Copyright (C) Microsoft. All rights reserved.
 * Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
 *
 * =+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+
 *
 * os_utilities.cpp - defines an abstraction for common OS functions like Sleep, hiding the underlying platform.
 *
 * =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
 ****/

#include "os_utilities.h"

#ifdef WIN32
#define WIN32_LEAN_AND_MEAN // Exclude rarely-used stuff from Windows headers
#include <SDKDDKVer.h>

#include <Windows.h>
#else
#include <unistd.h>
#endif

namespace tests
{
namespace common
{
namespace utilities
{
void os_utilities::sleep(unsigned long ms)
{
#ifdef WIN32
    Sleep(ms);
#else
    usleep(ms * 1000);
#endif
}

unsigned long os_utilities::interlocked_increment(volatile unsigned long* addend)
{
#ifdef WIN32
    return InterlockedIncrement(addend);
#elif defined(__GNUC__)
    return __sync_add_and_fetch(addend, 1);
#else
#error Need to implement interlocked_increment
#endif
}

long os_utilities::interlocked_exchange(volatile long* target, long value)
{
#ifdef WIN32
    return InterlockedExchange(target, value);
#elif defined(__GNUC__)
    return __sync_lock_test_and_set(target, value);
#else
#error Need to implement interlocked_exchange
#endif
}

} // namespace utilities
} // namespace common
} // namespace tests
