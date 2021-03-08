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
TestDetails const g_testdetails("testname", "suitename", "filename", 123);

TEST(StartsWithNoTestsRun)
{
    TestResults results;
    CHECK_EQUAL(0, results.GetTotalTestCount());
}

TEST(RecordsNumbersOfTests)
{
    TestResults results;
    results.OnTestStart(g_testdetails);
    results.OnTestStart(g_testdetails);
    results.OnTestStart(g_testdetails);
    CHECK_EQUAL(3, results.GetTotalTestCount());
}

TEST(StartsWithNoTestsFailing)
{
    TestResults results;
    CHECK_EQUAL(0, results.GetFailureCount());
}

TEST(RecordsNumberOfFailures)
{
    TestResults results;
    results.OnTestFailure(g_testdetails, "");
    results.OnTestFailure(g_testdetails, "");
    CHECK_EQUAL(2, results.GetFailureCount());
}

TEST(RecordsNumberOfFailedTests)
{
    TestResults results;

    results.OnTestStart(g_testdetails);
    results.OnTestFailure(g_testdetails, "");
    results.OnTestFinish(g_testdetails, 0);

    results.OnTestStart(g_testdetails);
    results.OnTestFailure(g_testdetails, "");
    results.OnTestFailure(g_testdetails, "");
    results.OnTestFailure(g_testdetails, "");
    results.OnTestFinish(g_testdetails, 0);

    CHECK_EQUAL(2, results.GetFailedTestCount());
}

TEST(NotifiesReporterOfTestStartWithCorrectInfo)
{
    RecordingReporter reporter;
    TestResults results(&reporter);
    results.OnTestStart(g_testdetails);

    CHECK_EQUAL(1, reporter.testRunCount);
    CHECK_EQUAL("suitename", reporter.lastStartedSuite);
    CHECK_EQUAL("testname", reporter.lastStartedTest);
}

TEST(NotifiesReporterOfTestFailureWithCorrectInfo)
{
    RecordingReporter reporter;
    TestResults results(&reporter);

    results.OnTestFailure(g_testdetails, "failurestring");
    CHECK_EQUAL(1, reporter.testFailedCount);
    CHECK_EQUAL("filename", reporter.lastFailedFile);
    CHECK_EQUAL(123, reporter.lastFailedLine);
    CHECK_EQUAL("suitename", reporter.lastFailedSuite);
    CHECK_EQUAL("testname", reporter.lastFailedTest);
    CHECK_EQUAL("failurestring", reporter.lastFailedMessage);
}

TEST(NotifiesReporterOfCheckFailureWithCorrectInfo)
{
    RecordingReporter reporter;
    TestResults results(&reporter);

    results.OnTestFailure(g_testdetails, "failurestring");
    CHECK_EQUAL(1, reporter.testFailedCount);

    CHECK_EQUAL("filename", reporter.lastFailedFile);
    CHECK_EQUAL(123, reporter.lastFailedLine);
    CHECK_EQUAL("testname", reporter.lastFailedTest);
    CHECK_EQUAL("suitename", reporter.lastFailedSuite);
    CHECK_EQUAL("failurestring", reporter.lastFailedMessage);
}

TEST(NotifiesReporterOfTestEnd)
{
    RecordingReporter reporter;
    TestResults results(&reporter);

    results.OnTestFinish(g_testdetails, 0.1234f);
    CHECK_EQUAL(1, reporter.testFinishedCount);
    CHECK_EQUAL("testname", reporter.lastFinishedTest);
    CHECK_EQUAL("suitename", reporter.lastFinishedSuite);
    CHECK_CLOSE(0.1234f, reporter.lastFinishedTestTime, 0.0001f);
}

} // namespace
