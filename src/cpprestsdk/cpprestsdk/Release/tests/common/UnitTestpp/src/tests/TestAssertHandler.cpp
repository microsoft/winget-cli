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

#include "../AssertException.h"
#include <csetjmp>

using namespace UnitTest;

namespace
{
TEST(CanSetAssertExpected)
{
    Detail::ExpectAssert(true);
    CHECK(Detail::AssertExpected());

    Detail::ExpectAssert(false);
    CHECK(!Detail::AssertExpected());
}

#ifndef UNITTEST_NO_EXCEPTIONS

TEST(ReportAssertThrowsAssertException)
{
    bool caught = false;

    try
    {
        TestResults testResults;
        TestDetails testDetails("", "", "", 0);
        Detail::ReportAssertEx(&testResults, &testDetails, "", "", 0);
    }
    catch (AssertException const&)
    {
        caught = true;
    }

    CHECK(true == caught);
}

TEST(ReportAssertClearsExpectAssertFlag)
{
    RecordingReporter reporter;
    TestResults testResults(&reporter);
    TestDetails testDetails("", "", "", 0);

    try
    {
        Detail::ExpectAssert(true);
        Detail::ReportAssertEx(&testResults, &testDetails, "", "", 0);
    }
    catch (AssertException const&)
    {
    }

    CHECK(Detail::AssertExpected() == false);
    CHECK_EQUAL(0, reporter.testFailedCount);
}

TEST(ReportAssertWritesFailureToResultsAndDetailsWhenAssertIsNotExpected)
{
    const int lineNumber = 12345;
    const char* description = "description";
    const char* filename = "filename";

    RecordingReporter reporter;
    TestResults testResults(&reporter);
    TestDetails testDetails("", "", "", 0);

    try
    {
        Detail::ReportAssertEx(&testResults, &testDetails, description, filename, lineNumber);
    }
    catch (AssertException const&)
    {
    }

    CHECK_EQUAL(description, reporter.lastFailedMessage);
    CHECK_EQUAL(filename, reporter.lastFailedFile);
    CHECK_EQUAL(lineNumber, reporter.lastFailedLine);
}

TEST(ReportAssertReportsNoErrorsWhenAssertIsExpected)
{
    Detail::ExpectAssert(true);

    RecordingReporter reporter;
    TestResults testResults(&reporter);
    TestDetails testDetails("", "", "", 0);

    try
    {
        Detail::ReportAssertEx(&testResults, &testDetails, "", "", 0);
    }
    catch (AssertException const&)
    {
    }

    CHECK_EQUAL(0, reporter.testFailedCount);
}

TEST(CheckAssertMacroSetsAssertExpectationToFalseAfterRunning)
{
    Detail::ExpectAssert(true);
    CHECK_ASSERT(ReportAssert("", "", 0));
    CHECK(!Detail::AssertExpected());
    Detail::ExpectAssert(false);
}

#else

TEST(SetAssertJumpTargetReturnsFalseWhenSettingJumpTarget) { CHECK(UNITTEST_SET_ASSERT_JUMP_TARGET() == false); }

TEST(JumpToAssertJumpTarget_JumpsToSetPoint_ReturnsTrue)
{
    const volatile bool taken = !!UNITTEST_SET_ASSERT_JUMP_TARGET();

    volatile bool set = false;
    if (taken == false)
    {
        UNITTEST_JUMP_TO_ASSERT_JUMP_TARGET();
        set = true;
    }

    CHECK(set == false);
}

#endif

} // namespace
