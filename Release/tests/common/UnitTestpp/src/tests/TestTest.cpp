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

using namespace UnitTest;

namespace
{
TEST(PassingTestHasNoFailures)
{
    class PassingTest : public Test
    {
    public:
        PassingTest() : Test("passing") {}
        virtual void RunImpl() const { CHECK(true); }
    };

    TestResults results;
    {
        ScopedCurrentTest scopedResults(results);
        PassingTest().Run();
    }

    CHECK_EQUAL(0, results.GetFailureCount());
}

TEST(FailingTestHasFailures)
{
    class FailingTest : public Test
    {
    public:
        FailingTest() : Test("failing") {}
        virtual void RunImpl() const { CHECK(false); }
    };

    TestResults results;
    {
        ScopedCurrentTest scopedResults(results);
        FailingTest().Run();
    }

    CHECK_EQUAL(1, results.GetFailureCount());
}

#ifndef UNITTEST_NO_EXCEPTIONS
TEST(ThrowingTestsAreReportedAsFailures)
{
    class CrashingTest : public Test
    {
    public:
        CrashingTest() : Test("throwing") {}
        virtual void RunImpl() const { throw "Blah"; }
    };

    TestResults results;
    {
        ScopedCurrentTest scopedResult(results);
        CrashingTest().Run();
    }

    CHECK_EQUAL(1, results.GetFailureCount());
}
/*
#ifndef UNITTEST_MINGW
TEST(CrashingTestsAreReportedAsFailures)
{
    class CrashingTest : public Test
    {
    public:
        CrashingTest() : Test("crashing") {}
        virtual void RunImpl() const
        {
            reinterpret_cast< void (*)() >(0)();
        }
    };

    TestResults results;
        {
                ScopedCurrentTest scopedResult(results);
                CrashingTest().Run();
        }

        CHECK_EQUAL(1, results.GetFailureCount());
}
#endif
*/
#endif

TEST(TestWithUnspecifiedSuiteGetsDefaultSuite)
{
    Test test("test");
    CHECK(test.m_details.suiteName != NULL);
    CHECK_EQUAL("DefaultSuite", test.m_details.suiteName);
}

TEST(TestReflectsSpecifiedSuiteName)
{
    Test test("test", "testSuite");
    CHECK(test.m_details.suiteName != NULL);
    CHECK_EQUAL("testSuite", test.m_details.suiteName);
}

void Fail() { CHECK(false); }

TEST(OutOfCoreCHECKMacrosCanFailTests)
{
    TestResults results;
    {
        ScopedCurrentTest scopedResult(results);
        Fail();
    }

    CHECK_EQUAL(1, results.GetFailureCount());
}

} // namespace
