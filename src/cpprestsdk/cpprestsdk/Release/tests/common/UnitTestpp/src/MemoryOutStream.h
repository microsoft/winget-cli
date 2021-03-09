/***
 * This file is based on or incorporates material from the UnitTest++ r30 open source project.
 * Microsoft is not the original author of this code but has modified it and is licensing the code under
 * the MIT License. Microsoft reserves all other rights not expressly granted under the MIT License,
 * whether by implication, estoppel or otherwise.
 *
 * UnitTest++ r30
 *
 * Copyright (c) 2006 Noel Llopis and Charles Nicholson
 * Portions Copyright (c) Microsoft Corporation
 *
 * All Rights Reserved.
 *
 * MIT License
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy of this software
 * and associated documentation files (the "Software"), to deal in the Software without restriction,
 * including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so,
 * subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all copies or
 * substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED,
 * INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE
 * AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM,
 * DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 ***/

#ifndef UNITTEST_MEMORYOUTSTREAM_H
#define UNITTEST_MEMORYOUTSTREAM_H

#include "../config.h"
#include "HelperMacros.h"

#ifdef UNITTEST_MEMORYOUTSTREAM_IS_STD_OSTRINGSTREAM

#include <sstream>

namespace UnitTest
{
class MemoryOutStream : public std::ostringstream
{
public:
    UNITTEST_LINKAGE MemoryOutStream();
    UNITTEST_LINKAGE ~MemoryOutStream();
    UNITTEST_LINKAGE void Clear();
    UNITTEST_LINKAGE char const* GetText() const;

private:
    MemoryOutStream(MemoryOutStream const&);
    void operator=(MemoryOutStream const&);

    mutable std::string m_text;
};

} // namespace UnitTest

#else

#include <cstddef>

namespace UnitTest
{
class UNITTEST_LINKAGE MemoryOutStream
{
public:
    explicit MemoryOutStream(int const size = 256);
    ~MemoryOutStream();

    void Clear();
    char const* GetText() const;

    MemoryOutStream& operator<<(char const* txt);
    MemoryOutStream& operator<<(int n);
    MemoryOutStream& operator<<(long n);
    MemoryOutStream& operator<<(long long n);
    MemoryOutStream& operator<<(unsigned long n);
    MemoryOutStream& operator<<(unsigned long long n);
    MemoryOutStream& operator<<(float f);
    MemoryOutStream& operator<<(double d);
    MemoryOutStream& operator<<(void const* p);
    MemoryOutStream& operator<<(unsigned int s);

    enum
    {
        GROW_CHUNK_SIZE = 32
    };
    int GetCapacity() const;

private:
    void operator=(MemoryOutStream const&);
    void GrowBuffer(int capacity);

    int m_capacity;
    char* m_buffer;
};

} // namespace UnitTest

#endif

#endif
