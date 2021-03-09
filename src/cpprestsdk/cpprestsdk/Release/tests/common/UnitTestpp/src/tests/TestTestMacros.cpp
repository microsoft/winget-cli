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
using namespace std;

#ifdef __APPLE__
extern "C" UnitTest::TestList& UnitTest::GetTestList()
{
    static TestList s_list;
    return s_list;
}
#endif

namespace
{
TestList list1;
TEST_EX(DummyTest, list1) {}

TEST(TestsAreAddedToTheListThroughMacro)
{
    CHECK(list1.GetFirst() != 0);
    CHECK(list1.GetFirst()->m_nextTest == 0);
}

#ifndef UNITTEST_NO_EXCEPTIONS

struct ThrowingThingie
{
    ThrowingThingie() : dummy(false)
    {
        if (!dummy) throw "Oops";
    }

    bool dummy;
};

TestList list2;
TEST_FIXTURE_EX(ThrowingThingie, DummyTestName, list2) {}

TEST(ExceptionsInFixtureAreReportedAsHappeningInTheFixture)
{
    RecordingReporter reporter;
    TestResults result(&reporter);
    {
        ScopedCurrentTest scopedResults(result);
        list2.GetFirst()->Run();
    }

    CHECK(strstr(reporter.lastFailedMessage, "xception"));
    CHECK(strstr(reporter.lastFailedMessage, "fixture"));
    CHECK(strstr(reporter.lastFailedMessage, "ThrowingThingie"));
}

#endif

struct DummyFixture
{
    int x;
};

// We're really testing the macros so we just want them to compile and link
SUITE(TestSuite1)
{
    TEST(SimilarlyNamedTestsInDifferentSuitesWork) {}

    TEST_FIXTURE(DummyFixture, SimilarlyNamedFixtureTestsInDifferentSuitesWork) {}
}

SUITE(TestSuite2)
{
    TEST(SimilarlyNamedTestsInDifferentSuitesWork) {}

    TEST_FIXTURE(DummyFixture, SimilarlyNamedFixtureTestsInDifferentSuitesWork) {}
}

TestList macroTestList1;
TEST_EX(MacroTestHelper1, macroTestList1) {}

TEST(TestAddedWithTEST_EXMacroGetsDefaultSuite)
{
    CHECK(macroTestList1.GetFirst() != NULL);
    CHECK_EQUAL("MacroTestHelper1", macroTestList1.GetFirst()->m_details.testName);
    CHECK_EQUAL("DefaultSuite", macroTestList1.GetFirst()->m_details.suiteName);
}

TestList macroTestList2;
TEST_FIXTURE_EX(DummyFixture, MacroTestHelper2, macroTestList2) {}

TEST(TestAddedWithTEST_FIXTURE_EXMacroGetsDefaultSuite)
{
    CHECK(macroTestList2.GetFirst() != NULL);
    CHECK_EQUAL("MacroTestHelper2", macroTestList2.GetFirst()->m_details.testName);
    CHECK_EQUAL("DefaultSuite", macroTestList2.GetFirst()->m_details.suiteName);
}

#ifndef UNITTEST_NO_EXCEPTIONS

struct FixtureCtorThrows
{
    FixtureCtorThrows() { throw "exception"; }
};

TestList throwingFixtureTestList1;
TEST_FIXTURE_EX(FixtureCtorThrows, FixtureCtorThrowsTestName, throwingFixtureTestList1) {}

TEST(FixturesWithThrowingCtorsAreFailures)
{
    CHECK(throwingFixtureTestList1.GetFirst() != NULL);
    RecordingReporter reporter;
    TestResults result(&reporter);
    {
        ScopedCurrentTest scopedResult(result);
        throwingFixtureTestList1.GetFirst()->Run();
    }

    int const failureCount = result.GetFailedTestCount();
    CHECK_EQUAL(1, failureCount);
    CHECK(strstr(reporter.lastFailedMessage, "while constructing fixture"));
}

const int FailingLine = 123;

struct FixtureCtorAsserts
{
    FixtureCtorAsserts() { UnitTest::ReportAssert("assert failure", "file", FailingLine); }
};

TestList ctorAssertFixtureTestList;
TEST_FIXTURE_EX(FixtureCtorAsserts, CorrectlyReportsAssertFailureInCtor, ctorAssertFixtureTestList) {}

TEST(CorrectlyReportsFixturesWithCtorsThatAssert)
{
    RecordingReporter reporter;
    TestResults result(&reporter);
    {
        ScopedCurrentTest scopedResults(result);
        ctorAssertFixtureTestList.GetFirst()->Run();
    }

    const int failureCount = result.GetFailedTestCount();
    CHECK_EQUAL(1, failureCount);
    CHECK_EQUAL(FailingLine, reporter.lastFailedLine);
    CHECK(strstr(reporter.lastFailedMessage, "assert failure"));
}

#endif

} // namespace

// We're really testing if it's possible to use the same suite in two files
// to compile and link successfuly (TestTestSuite.cpp has suite with the same name)
// Note: we are outside of the anonymous namespace
SUITE(SameTestSuite)
{
    TEST(DummyTest1) {}
}

#define CUR_TEST_NAME CurrentTestDetailsContainCurrentTestInfo
#define INNER_STRINGIFY(X) #X
#define STRINGIFY(X) INNER_STRINGIFY(X)

TEST(CUR_TEST_NAME)
{
    const UnitTest::TestDetails* details = CurrentTest::Details();
    CHECK_EQUAL(STRINGIFY(CUR_TEST_NAME), details->testName);
}

#undef CUR_TEST_NAME
#undef INNER_STRINGIFY
#undef STRINGIFY
