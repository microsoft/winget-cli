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

#ifndef UNITTEST_NO_DEFERRED_REPORTER

#include "../DeferredTestReporter.h"

namespace UnitTest
{
namespace
{
#ifndef UNITTEST_MEMORYOUTSTREAM_IS_STD_OSTRINGSTREAM
MemoryOutStream& operator<<(MemoryOutStream& lhs, const std::string& rhs)
{
    lhs << rhs.c_str();
    return lhs;
}
#endif

struct MockDeferredTestReporter : public DeferredTestReporter
{
    virtual void ReportSummary(int, int, int, float) {}
};

struct DeferredTestReporterFixture
{
    DeferredTestReporterFixture()
        : testName("UniqueTestName")
        , testSuite("UniqueTestSuite")
        , fileName("filename.h")
        , lineNumber(12)
        , details(testName.c_str(), testSuite.c_str(), fileName.c_str(), lineNumber)
    {
    }

    MockDeferredTestReporter reporter;
    std::string const testName;
    std::string const testSuite;
    std::string const fileName;
    int const lineNumber;
    TestDetails const details;
};

TEST_FIXTURE(DeferredTestReporterFixture, ReportTestStartCreatesANewDeferredTest)
{
    reporter.ReportTestStart(details);
    CHECK_EQUAL(1, (int)reporter.GetResults().size());
}

TEST_FIXTURE(DeferredTestReporterFixture, ReportTestStartCapturesTestNameAndSuite)
{
    reporter.ReportTestStart(details);

    DeferredTestResult const& result = reporter.GetResults().at(0);
    CHECK_EQUAL(testName.c_str(), result.testName);
    CHECK_EQUAL(testSuite.c_str(), result.suiteName);
}

TEST_FIXTURE(DeferredTestReporterFixture, ReportTestEndCapturesTestTime)
{
    float const elapsed = 123.45f;
    reporter.ReportTestStart(details);
    reporter.ReportTestFinish(details, true, elapsed);

    DeferredTestResult const& result = reporter.GetResults().at(0);
    CHECK_CLOSE(elapsed, result.timeElapsed, 0.0001f);
}

TEST_FIXTURE(DeferredTestReporterFixture, ReportFailureSavesFailureDetails)
{
    char const* failure = "failure";

    reporter.ReportTestStart(details);
    reporter.ReportFailure(details, failure);

    DeferredTestResult const& result = reporter.GetResults().at(0);
    CHECK(result.failed == true);
    CHECK_EQUAL(fileName.c_str(), result.failureFile);
}

TEST_FIXTURE(DeferredTestReporterFixture, ReportFailureSavesFailureDetailsForMultipleFailures)
{
    char const* failure1 = "failure 1";
    char const* failure2 = "failure 2";

    reporter.ReportTestStart(details);
    reporter.ReportFailure(details, failure1);
    reporter.ReportFailure(details, failure2);

    DeferredTestResult const& result = reporter.GetResults().at(0);
    CHECK_EQUAL(2, (int)result.failures.size());
    CHECK_EQUAL(failure1, result.failures[0].failureStr);
    CHECK_EQUAL(failure2, result.failures[1].failureStr);
}

TEST_FIXTURE(DeferredTestReporterFixture, DeferredTestReporterTakesCopyOfFailureMessage)
{
    reporter.ReportTestStart(details);

    char failureMessage[128];
    char const* goodStr = "Real failure message";
    char const* badStr = "Bogus failure message";

    using namespace std;

    strcpy(failureMessage, goodStr);
    reporter.ReportFailure(details, failureMessage);
    strcpy(failureMessage, badStr);

    DeferredTestResult const& result = reporter.GetResults().at(0);
    DeferredTestFailure const& failure = result.failures.at(0);
    CHECK_EQUAL(goodStr, failure.failureStr);
}

} // namespace
} // namespace UnitTest

#endif
