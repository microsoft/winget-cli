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

#include "../Test.h"

using namespace UnitTest;

namespace
{
struct TestRunnerFixture
{
    TestRunnerFixture() : runner(reporter) { s_testRunnerFixtureTestResults = runner.GetTestResults(); }

    static TestResults* s_testRunnerFixtureTestResults;

    RecordingReporter reporter;
    TestList list;
    TestRunner runner;
};

TestResults* TestRunnerFixture::s_testRunnerFixtureTestResults = NULL;

struct MockTest : public Test
{
    MockTest(char const* testName, bool const success_, bool const assert_, int const count_ = 1)
        : Test(testName), success(success_), asserted(assert_), count(count_)
    {
        m_isMockTest = true;
    }

    virtual void RunImpl() const
    {
        TestResults* testResults = TestRunnerFixture::s_testRunnerFixtureTestResults;

        for (int i = 0; i < count; ++i)
        {
            if (asserted)
                Detail::ReportAssertEx(testResults, &m_details, "desc", "file", 0);
            else if (!success)
                testResults->OnTestFailure(m_details, "message");
        }
    }

    bool const success;
    bool const asserted;
    int const count;
};

TEST_FIXTURE(TestRunnerFixture, TestStartIsReportedCorrectly)
{
    MockTest test("goodtest", true, false);
    list.Add(&test);

    runner.RunTestsIf(list, NULL, True(), 0);
    CHECK_EQUAL(1, reporter.testRunCount);
    CHECK_EQUAL("goodtest", reporter.lastStartedTest);
}

TEST_FIXTURE(TestRunnerFixture, TestFinishIsReportedCorrectly)
{
    MockTest test("goodtest", true, false);
    list.Add(&test);

    runner.RunTestsIf(list, NULL, True(), 0);
    CHECK_EQUAL(1, reporter.testFinishedCount);
    CHECK_EQUAL("goodtest", reporter.lastFinishedTest);
}

class SlowTest : public Test
{
public:
    SlowTest() : Test("slow", "somesuite", "filename", 123) {}
    virtual void RunImpl() const { TimeHelpers::SleepMs(20); }
};

TEST_FIXTURE(TestRunnerFixture, TestFinishIsCalledWithCorrectTime)
{
    SlowTest test;
    list.Add(&test);

    runner.RunTestsIf(list, NULL, True(), 0);
    CHECK(reporter.lastFinishedTestTime >= 0.005f && reporter.lastFinishedTestTime <= 0.050f);
}

TEST_FIXTURE(TestRunnerFixture, FailureCountIsZeroWhenNoTestsAreRun)
{
    CHECK_EQUAL(0, runner.RunTestsIf(list, NULL, True(), 0));
    CHECK_EQUAL(0, reporter.testRunCount);
    CHECK_EQUAL(0, reporter.testFailedCount);
}

TEST_FIXTURE(TestRunnerFixture, CallsReportFailureOncePerFailingTest)
{
    MockTest test1("test", false, false);
    list.Add(&test1);
    MockTest test2("test", true, false);
    list.Add(&test2);
    MockTest test3("test", false, false);
    list.Add(&test3);

    CHECK_EQUAL(2, runner.RunTestsIf(list, NULL, True(), 0));
    CHECK_EQUAL(2, reporter.testFailedCount);
}

TEST_FIXTURE(TestRunnerFixture, TestsThatAssertAreReportedAsFailing)
{
    MockTest test("test", true, true);
    list.Add(&test);

    runner.RunTestsIf(list, NULL, True(), 0);
    CHECK_EQUAL(1, reporter.testFailedCount);
}

TEST_FIXTURE(TestRunnerFixture, AssertingTestAbortsAsSoonAsAssertIsHit)
{
    MockTest test("test", false, true, 3);
    list.Add(&test);
    runner.RunTestsIf(list, NULL, True(), 0);
    CHECK_EQUAL(1, reporter.summaryFailureCount);
}

TEST_FIXTURE(TestRunnerFixture, ReporterNotifiedOfTestCount)
{
    MockTest test1("test", true, false);
    MockTest test2("test", true, false);
    MockTest test3("test", true, false);
    list.Add(&test1);
    list.Add(&test2);
    list.Add(&test3);

    runner.RunTestsIf(list, NULL, True(), 0);
    CHECK_EQUAL(3, reporter.summaryTotalTestCount);
}

TEST_FIXTURE(TestRunnerFixture, ReporterNotifiedOfFailedTests)
{
    MockTest test1("test", false, false, 2);
    MockTest test2("test", true, false);
    MockTest test3("test", false, false, 3);
    list.Add(&test1);
    list.Add(&test2);
    list.Add(&test3);

    runner.RunTestsIf(list, NULL, True(), 0);
    CHECK_EQUAL(2, reporter.summaryFailedTestCount);
}

TEST_FIXTURE(TestRunnerFixture, ReporterNotifiedOfFailures)
{
    MockTest test1("test", false, false, 2);
    MockTest test2("test", true, false);
    MockTest test3("test", false, false, 3);
    list.Add(&test1);
    list.Add(&test2);
    list.Add(&test3);

    runner.RunTestsIf(list, NULL, True(), 0);
    CHECK_EQUAL(5, reporter.summaryFailureCount);
}

TEST_FIXTURE(TestRunnerFixture, SlowTestPassesForHighTimeThreshold)
{
    SlowTest test;
    list.Add(&test);

    runner.RunTestsIf(list, NULL, True(), 0);
    CHECK_EQUAL(0, reporter.testFailedCount);
}

struct TestSuiteFixture
{
    TestSuiteFixture()
        : test1("TestInDefaultSuite")
        , test2("TestInOtherSuite", "OtherSuite")
        , test3("SecondTestInDefaultSuite")
        , runner(reporter)
    {
        list.Add(&test1);
        list.Add(&test2);
    }

    Test test1;
    Test test2;
    Test test3;
    RecordingReporter reporter;
    TestList list;
    TestRunner runner;
};

TEST_FIXTURE(TestSuiteFixture, TestRunnerRunsAllSuitesIfNullSuiteIsPassed)
{
    runner.RunTestsIf(list, NULL, True(), 0);
    CHECK_EQUAL(2, reporter.summaryTotalTestCount);
}

TEST_FIXTURE(TestSuiteFixture, TestRunnerRunsOnlySpecifiedSuite)
{
    runner.RunTestsIf(list, "OtherSuite", True(), 0);
    CHECK_EQUAL(1, reporter.summaryTotalTestCount);
    CHECK_EQUAL("TestInOtherSuite", reporter.lastFinishedTest);
}

struct RunTestIfNameIs
{
    RunTestIfNameIs(char const* name_) : name(name_) {}

    bool operator()(const Test* const test) const
    {
        using namespace std;
        return (0 == strcmp(test->m_details.testName, name));
    }

    char const* name;
};

TEST(TestMockPredicateBehavesCorrectly)
{
    RunTestIfNameIs predicate("pass");

    Test pass("pass");
    Test fail("fail");

    CHECK(predicate(&pass));
    CHECK(!predicate(&fail));
}

TEST_FIXTURE(TestRunnerFixture, TestRunnerRunsTestsThatPassPredicate)
{
    Test should_run("goodtest");
    list.Add(&should_run);

    Test should_not_run("badtest");
    list.Add(&should_not_run);

    runner.RunTestsIf(list, NULL, RunTestIfNameIs("goodtest"), 0);
    CHECK_EQUAL(1, reporter.testRunCount);
    CHECK_EQUAL("goodtest", reporter.lastStartedTest);
}

TEST_FIXTURE(TestRunnerFixture, TestRunnerOnlyRunsTestsInSpecifiedSuiteAndThatPassPredicate)
{
    Test runningTest1("goodtest", "suite");
    Test skippedTest2("goodtest");
    Test skippedTest3("badtest", "suite");
    Test skippedTest4("badtest");

    list.Add(&runningTest1);
    list.Add(&skippedTest2);
    list.Add(&skippedTest3);
    list.Add(&skippedTest4);

    runner.RunTestsIf(list, "suite", RunTestIfNameIs("goodtest"), 0);

    CHECK_EQUAL(1, reporter.testRunCount);
    CHECK_EQUAL("goodtest", reporter.lastStartedTest);
    CHECK_EQUAL("suite", reporter.lastStartedSuite);
}

} // namespace
