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

#include "../MemoryOutStream.h"
#include <climits>
#include <cstdlib>

using namespace UnitTest;
using namespace std;

namespace
{
TEST(DefaultIsEmptyString)
{
    MemoryOutStream const stream;
    CHECK(stream.GetText() != 0);
    CHECK_EQUAL("", stream.GetText());
}

TEST(StreamingTextCopiesCharacters)
{
    MemoryOutStream stream;
    stream << "Lalala";
    CHECK_EQUAL("Lalala", stream.GetText());
}

TEST(StreamingMultipleTimesConcatenatesResult)
{
    MemoryOutStream stream;
    stream << "Bork"
           << "To"
           << "Fred";
    CHECK_EQUAL("BorkToFred", stream.GetText());
}

TEST(StreamingIntWritesCorrectCharacters)
{
    MemoryOutStream stream;
    stream << (int)123;
    CHECK_EQUAL("123", stream.GetText());
}

TEST(StreamingUnsignedIntWritesCorrectCharacters)
{
    MemoryOutStream stream;
    stream << (unsigned int)123;
    CHECK_EQUAL("123", stream.GetText());
}

TEST(StreamingLongWritesCorrectCharacters)
{
    MemoryOutStream stream;
    stream << (long)(-123);
    CHECK_EQUAL("-123", stream.GetText());
}

TEST(StreamingUnsignedLongWritesCorrectCharacters)
{
    MemoryOutStream stream;
    stream << (unsigned long)123;
    CHECK_EQUAL("123", stream.GetText());
}

TEST(StreamingLongLongWritesCorrectCharacters)
{
    MemoryOutStream stream;
    stream << (long long)(ULONG_MAX)*2;
    CHECK_EQUAL("8589934590", stream.GetText());
}

TEST(StreamingUnsignedLongLongWritesCorrectCharacters)
{
    MemoryOutStream stream;
    stream << (unsigned long long)(ULONG_MAX)*2;
    CHECK_EQUAL("8589934590", stream.GetText());
}

TEST(StreamingFloatWritesCorrectCharacters)
{
    MemoryOutStream stream;
    stream << 3.1415f;
    CHECK(strstr(stream.GetText(), "3.1415"));
}

TEST(StreamingDoubleWritesCorrectCharacters)
{
    MemoryOutStream stream;
    stream << 3.1415;
    CHECK(strstr(stream.GetText(), "3.1415"));
}

TEST(StreamingPointerWritesCorrectCharacters)
{
    MemoryOutStream stream;
    int* p = (int*)0x1234;
    stream << p;
    CHECK(strstr(stream.GetText(), "1234"));
}

TEST(StreamingSizeTWritesCorrectCharacters)
{
    MemoryOutStream stream;
    size_t const s = 53124;
    stream << s;
    CHECK_EQUAL("53124", stream.GetText());
}

TEST(ClearEmptiesMemoryOutStreamContents)
{
    MemoryOutStream stream;
    stream << "Hello world";
    stream.Clear();
    CHECK_EQUAL("", stream.GetText());
}

#ifndef UNITTEST_MEMORYOUTSTREAM_IS_STD_OSTRINGSTREAM

TEST(StreamInitialCapacityIsCorrect)
{
    MemoryOutStream stream(MemoryOutStream::GROW_CHUNK_SIZE);
    CHECK_EQUAL((int)MemoryOutStream::GROW_CHUNK_SIZE, stream.GetCapacity());
}

TEST(StreamInitialCapacityIsMultipleOfGrowChunkSize)
{
    MemoryOutStream stream(MemoryOutStream::GROW_CHUNK_SIZE + 1);
    CHECK_EQUAL((int)MemoryOutStream::GROW_CHUNK_SIZE * 2, stream.GetCapacity());
}

TEST(ExceedingCapacityGrowsBuffer)
{
    MemoryOutStream stream(MemoryOutStream::GROW_CHUNK_SIZE);
    stream << "012345678901234567890123456789";
    char const* const oldBuffer = stream.GetText();
    stream << "0123456789";
    CHECK(oldBuffer != stream.GetText());
}

TEST(ExceedingCapacityGrowsBufferByGrowChunk)
{
    MemoryOutStream stream(MemoryOutStream::GROW_CHUNK_SIZE);
    stream << "0123456789012345678901234567890123456789";
    CHECK_EQUAL(MemoryOutStream::GROW_CHUNK_SIZE * 2, stream.GetCapacity());
}

TEST(WritingStringLongerThanCapacityFitsInNewBuffer)
{
    MemoryOutStream stream(8);
    stream << "0123456789ABCDEF";
    CHECK_EQUAL("0123456789ABCDEF", stream.GetText());
}

TEST(WritingIntLongerThanCapacityFitsInNewBuffer)
{
    MemoryOutStream stream(8);
    stream << "aaaa" << 123456;
    ;
    CHECK_EQUAL("aaaa123456", stream.GetText());
}

TEST(WritingFloatLongerThanCapacityFitsInNewBuffer)
{
    MemoryOutStream stream(8);
    stream << "aaaa" << 123456.0f;
    ;
    CHECK_EQUAL("aaaa123456.000000f", stream.GetText());
}

TEST(WritingSizeTLongerThanCapacityFitsInNewBuffer)
{
    MemoryOutStream stream(8);
    stream << "aaaa" << size_t(32145);
    CHECK_EQUAL("aaaa32145", stream.GetText());
}

#endif

} // namespace
