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

#ifndef UNITTEST_REPORTASSERTIMPL_H
#define UNITTEST_REPORTASSERTIMPL_H

#include "../config.h"
#include "HelperMacros.h"

#ifdef UNITTEST_NO_EXCEPTIONS
#include <csetjmp>
#endif

namespace UnitTest
{
class TestResults;
class TestDetails;

namespace Detail
{
UNITTEST_LINKAGE void ExpectAssert(bool expected);

UNITTEST_LINKAGE void ReportAssertEx(TestResults* testResults,
                                     const TestDetails* testDetails,
                                     char const* description,
                                     char const* filename,
                                     int lineNumber);

UNITTEST_LINKAGE bool AssertExpected();

#ifdef UNITTEST_NO_EXCEPTIONS
UNITTEST_LINKAGE UNITTEST_JMPBUF* GetAssertJmpBuf();

#ifdef UNITTEST_WIN32
#define UNITTEST_SET_ASSERT_JUMP_TARGET()                                                                              \
    __pragma(warning(push)) __pragma(warning(disable : 4611)) UNITTEST_SETJMP(*UnitTest::Detail::GetAssertJmpBuf())    \
        __pragma(warning(pop))
#else
#define UNITTEST_SET_ASSERT_JUMP_TARGET() UNITTEST_SETJMP(*UnitTest::Detail::GetAssertJmpBuf())
#endif

#define UNITTEST_JUMP_TO_ASSERT_JUMP_TARGET() UNITTEST_LONGJMP(*UnitTest::Detail::GetAssertJmpBuf(), 1)
#endif

} // namespace Detail
} // namespace UnitTest

#endif
