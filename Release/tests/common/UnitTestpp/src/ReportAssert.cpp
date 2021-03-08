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

#include "ReportAssert.h"

#include "ReportAssertImpl.h"

#ifdef UNITTEST_NO_EXCEPTIONS
#include "ReportAssertImpl.h"
#endif

namespace UnitTest
{
namespace
{
bool& AssertExpectedFlag()
{
    static bool s_assertExpected = false;
    return s_assertExpected;
}
} // namespace

UNITTEST_LINKAGE void ReportAssert(char const* description, char const* filename, int lineNumber)
{
    Detail::ReportAssertEx(CurrentTest::Results(), CurrentTest::Details(), description, filename, lineNumber);
}

namespace Detail
{
#ifdef UNITTEST_NO_EXCEPTIONS
UNITTEST_JMPBUF* GetAssertJmpBuf()
{
    static UNITTEST_JMPBUF s_jmpBuf;
    return &s_jmpBuf;
}
#endif

UNITTEST_LINKAGE void ReportAssertEx(TestResults* testResults,
                                     const TestDetails* testDetails,
                                     char const* description,
                                     char const* filename,
                                     int lineNumber)
{
    if (AssertExpectedFlag() == false)
    {
        TestDetails assertDetails(testDetails->testName, testDetails->suiteName, filename, lineNumber);
        testResults->OnTestFailure(assertDetails, description);
    }

    ExpectAssert(false);

#ifndef UNITTEST_NO_EXCEPTIONS
    throw AssertException();
#else
    UNITTEST_JUMP_TO_ASSERT_JUMP_TARGET();
#endif
}

UNITTEST_LINKAGE void ExpectAssert(bool expected) { AssertExpectedFlag() = expected; }

UNITTEST_LINKAGE bool AssertExpected() { return AssertExpectedFlag(); }

} // namespace Detail
} // namespace UnitTest
