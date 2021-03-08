/***
 * Copyright (C) Microsoft. All rights reserved.
 * Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
 *
 * =+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+
 *
 * os_utilities.h - defines an abstraction for common OS functions like Sleep, hiding the underlying platform.
 *
 * =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
 ****/

#pragma once

#include "common_utilities_public.h"
#include "cpprest/details/cpprest_compat.h"

namespace tests
{
namespace common
{
namespace utilities
{
class os_utilities
{
public:
    static TEST_UTILITY_API void __cdecl sleep(unsigned long ms);

    // Could use std::atomics but VS 2010 doesn't support it yet.
    static TEST_UTILITY_API unsigned long __cdecl interlocked_increment(volatile unsigned long* addend);
    static TEST_UTILITY_API long __cdecl interlocked_exchange(volatile long* target, long value);

private:
    os_utilities();
    os_utilities(const os_utilities&);
    os_utilities& operator=(const os_utilities&);
};

} // namespace utilities
} // namespace common
} // namespace tests
