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

#include "../CompositeTestReporter.h"

using namespace UnitTest;

namespace
{
TEST(ZeroReportersByDefault) { CHECK_EQUAL(0, CompositeTestReporter().GetReporterCount()); }

struct MockReporter : TestReporter
{
    MockReporter()
        : testStartCalled(false)
        , testStartDetails(NULL)
        , failureCalled(false)
        , failureDetails(NULL)
        , failureStr(NULL)
        , testFinishCalled(false)
        , testFinishDetails(NULL)
        , testFinishSecondsElapsed(-1.0f)
        , summaryCalled(false)
        , summaryTotalTestCount(-1)
        , summaryFailureCount(-1)
        , summarySecondsElapsed(-1.0f)
    {
    }

    virtual void ReportTestStart(TestDetails const& test)
    {
        testStartCalled = true;
        testStartDetails = &test;
    }

    virtual void ReportFailure(TestDetails const& test, char const* failure)
    {
        failureCalled = true;
        failureDetails = &test;
        failureStr = failure;
    }

    virtual void ReportTestFinish(TestDetails const& test, bool, float secondsElapsed)
    {
        testFinishCalled = true;
        testFinishDetails = &test;
        testFinishSecondsElapsed = secondsElapsed;
    }

    virtual void ReportSummary(int totalTestCount, int failedTestCount, int failureCount, float secondsElapsed)
    {
        summaryCalled = true;
        summaryTotalTestCount = totalTestCount;
        summaryFailedTestCount = failedTestCount;
        summaryFailureCount = failureCount;
        summarySecondsElapsed = secondsElapsed;
    }

    bool testStartCalled;
    TestDetails const* testStartDetails;

    bool failureCalled;
    TestDetails const* failureDetails;
    const char* failureStr;

    bool testFinishCalled;
    TestDetails const* testFinishDetails;
    float testFinishSecondsElapsed;

    bool summaryCalled;
    int summaryTotalTestCount;
    int summaryFailedTestCount;
    int summaryFailureCount;
    float summarySecondsElapsed;
};

TEST(AddReporter)
{
    MockReporter r;
    CompositeTestReporter c;

    CHECK(c.AddReporter(&r));
    CHECK_EQUAL(1, c.GetReporterCount());
}

TEST(RemoveReporter)
{
    MockReporter r;
    CompositeTestReporter c;

    c.AddReporter(&r);
    CHECK(c.RemoveReporter(&r));
    CHECK_EQUAL(0, c.GetReporterCount());
}

struct Fixture
{
    Fixture()
    {
        c.AddReporter(&r0);
        c.AddReporter(&r1);
    }

    MockReporter r0, r1;
    CompositeTestReporter c;
};

TEST_FIXTURE(Fixture, ReportTestStartCallsReportTestStartOnAllAggregates)
{
    TestDetails t("", "", "", 0);
    c.ReportTestStart(t);

    CHECK(r0.testStartCalled);
    CHECK_EQUAL(&t, r0.testStartDetails);
    CHECK(r1.testStartCalled);
    CHECK_EQUAL(&t, r1.testStartDetails);
}

TEST_FIXTURE(Fixture, ReportFailureCallsReportFailureOnAllAggregates)
{
    TestDetails t("", "", "", 0);
    const char* failStr = "fail";
    c.ReportFailure(t, failStr);

    CHECK(r0.failureCalled);
    CHECK_EQUAL(&t, r0.failureDetails);
    CHECK_EQUAL(failStr, r0.failureStr);

    CHECK(r1.failureCalled);
    CHECK_EQUAL(&t, r1.failureDetails);
    CHECK_EQUAL(failStr, r1.failureStr);
}

TEST_FIXTURE(Fixture, ReportTestFinishCallsReportTestFinishOnAllAggregates)
{
    TestDetails t("", "", "", 0);
    const float s = 1.2345f;
    c.ReportTestFinish(t, true, s);

    CHECK(r0.testFinishCalled);
    CHECK_EQUAL(&t, r0.testFinishDetails);
    CHECK_CLOSE(s, r0.testFinishSecondsElapsed, 0.00001f);

    CHECK(r1.testFinishCalled);
    CHECK_EQUAL(&t, r1.testFinishDetails);
    CHECK_CLOSE(s, r1.testFinishSecondsElapsed, 0.00001f);
}

TEST_FIXTURE(Fixture, ReportSummaryCallsReportSummaryOnAllAggregates)
{
    TestDetails t("", "", "", 0);
    const int testCount = 3;
    const int failedTestCount = 4;
    const int failureCount = 5;
    const float secondsElapsed = 3.14159f;

    c.ReportSummary(testCount, failedTestCount, failureCount, secondsElapsed);

    CHECK(r0.summaryCalled);
    CHECK_EQUAL(testCount, r0.summaryTotalTestCount);
    CHECK_EQUAL(failedTestCount, r0.summaryFailedTestCount);
    CHECK_EQUAL(failureCount, r0.summaryFailureCount);
    CHECK_CLOSE(secondsElapsed, r0.summarySecondsElapsed, 0.00001f);

    CHECK(r1.summaryCalled);
    CHECK_EQUAL(testCount, r1.summaryTotalTestCount);
    CHECK_EQUAL(failedTestCount, r1.summaryFailedTestCount);
    CHECK_EQUAL(failureCount, r1.summaryFailureCount);
    CHECK_CLOSE(secondsElapsed, r1.summarySecondsElapsed, 0.00001f);
}

} // namespace
