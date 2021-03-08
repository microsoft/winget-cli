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

#ifndef UNITTEST_CONFIG_H
#define UNITTEST_CONFIG_H

// Standard defines documented here: http://predef.sourceforge.net

#if defined(_MSC_VER)
#pragma warning(disable : 4702) // unreachable code
#pragma warning(disable : 4722) // destructor never returns, potential memory leak

#if (_MSC_VER == 1200) // VC6
#pragma warning(disable : 4786)
#pragma warning(disable : 4290)
#endif

#ifdef _USRDLL
#define UNITTEST_WIN32_DLL
#endif
#define UNITTEST_WIN32
#endif

#if defined(unix) || defined(__unix__) || defined(__unix) || defined(linux) || defined(__APPLE__) ||                   \
    defined(__NetBSD__) || defined(__OpenBSD__) || defined(__FreeBSD__)
#define UNITTEST_POSIX
#endif

#if defined(__MINGW32__)
#define UNITTEST_MINGW
#endif

// MemoryOutStream is a custom reimplementation of parts of std::ostringstream.
// Uncomment this line to have MemoryOutStream implemented in terms of std::ostringstream.
// This is useful if you are using the CHECK macros on objects that have something like this defined:
// std::ostringstream& operator<<(std::ostringstream& s, const YourObject& value)

#define UNITTEST_MEMORYOUTSTREAM_IS_STD_OSTRINGSTREAM

// DeferredTestReporter uses the STL to collect test results for subsequent export by reporters like
// XmlTestReporter.  If you don't want to use this functionality, uncomment this line and no STL
// headers or code will be compiled into UnitTest++

//#define UNITTEST_NO_DEFERRED_REPORTER

// By default, asserts that you report via UnitTest::ReportAssert() abort the current test and
// continue to the next one by throwing an exception, which unwinds the stack naturally, destroying
// all auto variables on its way back down.  If you don't want to (or can't) use exceptions for your
// platform/compiler, uncomment this line.  All exception code will be removed from UnitTest++,
// assert recovery will be done via setjmp/longjmp, and NO correct stack unwinding will happen!

//#define UNITTEST_NO_EXCEPTIONS

#include <cpprest/details/basic_types.h>
#endif