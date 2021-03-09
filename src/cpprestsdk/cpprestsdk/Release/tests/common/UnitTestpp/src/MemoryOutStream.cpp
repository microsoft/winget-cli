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

#include "stdafx.h"

#ifdef UNITTEST_MEMORYOUTSTREAM_IS_STD_OSTRINGSTREAM

namespace UnitTest
{
MemoryOutStream::MemoryOutStream() {}

MemoryOutStream::~MemoryOutStream() {}

char const* MemoryOutStream::GetText() const
{
    m_text = this->str();
    return m_text.c_str();
}

void MemoryOutStream::Clear()
{
    this->str(std::string());
    m_text = this->str();
}

} // namespace UnitTest

#else

namespace UnitTest
{
namespace
{
template<typename ValueType>
void FormatToStream(MemoryOutStream& stream, char const* format, ValueType const& value)
{
    using namespace std;

    char txt[32];
    sprintf(txt, format, value);
    stream << txt;
}

int RoundUpToMultipleOfPow2Number(int n, int pow2Number) { return (n + (pow2Number - 1)) & ~(pow2Number - 1); }

} // namespace

MemoryOutStream::MemoryOutStream(int const size) : m_capacity(0), m_buffer(0) { GrowBuffer(size); }

MemoryOutStream::~MemoryOutStream() { delete[] m_buffer; }

void MemoryOutStream::Clear() { m_buffer[0] = '\0'; }

char const* MemoryOutStream::GetText() const { return m_buffer; }

MemoryOutStream& MemoryOutStream::operator<<(char const* txt)
{
    using namespace std;

    int const bytesLeft = m_capacity - (int)strlen(m_buffer);
    int const bytesRequired = (int)strlen(txt) + 1;

    if (bytesRequired > bytesLeft)
    {
        int const requiredCapacity = bytesRequired + m_capacity - bytesLeft;
        GrowBuffer(requiredCapacity);
    }

    strcat(m_buffer, txt);
    return *this;
}

MemoryOutStream& MemoryOutStream::operator<<(int const n)
{
    FormatToStream(*this, "%i", n);
    return *this;
}

MemoryOutStream& MemoryOutStream::operator<<(long const n)
{
    FormatToStream(*this, "%li", n);
    return *this;
}

MemoryOutStream& MemoryOutStream::operator<<(unsigned long const n)
{
    FormatToStream(*this, "%lu", n);
    return *this;
}

MemoryOutStream& MemoryOutStream::operator<<(long long const n)
{
#ifdef UNITTEST_WIN32
    FormatToStream(*this, "%I64d", n);
#else
    FormatToStream(*this, "%lld", n);
#endif

    return *this;
}

MemoryOutStream& MemoryOutStream::operator<<(unsigned long long const n)
{
#ifdef UNITTEST_WIN32
    FormatToStream(*this, "%I64u", n);
#else
    FormatToStream(*this, "%llu", n);
#endif

    return *this;
}

MemoryOutStream& MemoryOutStream::operator<<(float const f)
{
    FormatToStream(*this, "%ff", f);
    return *this;
}

MemoryOutStream& MemoryOutStream::operator<<(void const* p)
{
    FormatToStream(*this, "%p", p);
    return *this;
}

MemoryOutStream& MemoryOutStream::operator<<(unsigned int const s)
{
    FormatToStream(*this, "%u", s);
    return *this;
}

MemoryOutStream& MemoryOutStream::operator<<(double const d)
{
    FormatToStream(*this, "%f", d);
    return *this;
}

int MemoryOutStream::GetCapacity() const { return m_capacity; }

void MemoryOutStream::GrowBuffer(int const desiredCapacity)
{
    int const newCapacity = RoundUpToMultipleOfPow2Number(desiredCapacity, GROW_CHUNK_SIZE);

    using namespace std;

    char* buffer = new char[newCapacity];
    if (m_buffer)
        strcpy(buffer, m_buffer);
    else
        *buffer = '\0';

    delete[] m_buffer;
    m_buffer = buffer;
    m_capacity = newCapacity;
}

} // namespace UnitTest

#endif
