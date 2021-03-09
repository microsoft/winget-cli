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

#include "../XmlTestReporter.h"
#include <sstream>

using namespace UnitTest;
using std::ostringstream;

namespace
{
#ifndef UNITTEST_MEMORYOUTSTREAM_IS_STD_OSTRINGSTREAM

// Overload to let MemoryOutStream accept std::string
MemoryOutStream& operator<<(MemoryOutStream& s, const std::string& value)
{
    s << value.c_str();
    return s;
}

#endif

struct XmlTestReporterFixture
{
    XmlTestReporterFixture() : reporter(output) {}

    ostringstream output;
    XmlTestReporter reporter;
};

TEST_FIXTURE(XmlTestReporterFixture, MultipleCharactersAreEscaped)
{
    TestDetails const details("TestName", "suite", "filename.h", 4321);

    reporter.ReportTestStart(details);
    reporter.ReportFailure(details, "\"\"\'\'&&<<>>");
    reporter.ReportTestFinish(details, false, 0.1f);
    reporter.ReportSummary(1, 2, 3, 0.1f);

    char const* expected = "<?xml version=\"1.0\"?>"
                           "<unittest-results tests=\"1\" failedtests=\"2\" failures=\"3\" time=\"0.1\">"
                           "<test suite=\"suite\" name=\"TestName\" time=\"0.1\">"
                           "<failure message=\"filename.h(4321) : "
                           "&quot;&quot;&apos;&apos;&amp;&amp;&lt;&lt;&gt;&gt;\"/>"
                           "</test>"
                           "</unittest-results>";

    CHECK_EQUAL(expected, output.str());
}

TEST_FIXTURE(XmlTestReporterFixture, OutputIsCachedUntilReportSummaryIsCalled)
{
    TestDetails const details("", "", "", 0);

    reporter.ReportTestStart(details);
    reporter.ReportFailure(details, "message");
    reporter.ReportTestFinish(details, false, 1.0F);
    CHECK(output.str().empty());

    reporter.ReportSummary(1, 1, 1, 1.0f);
    CHECK(!output.str().empty());
}

TEST_FIXTURE(XmlTestReporterFixture, EmptyReportSummaryFormat)
{
    reporter.ReportSummary(0, 0, 0, 0.1f);

    const char* expected = "<?xml version=\"1.0\"?>"
                           "<unittest-results tests=\"0\" failedtests=\"0\" failures=\"0\" time=\"0.1\">"
                           "</unittest-results>";

    CHECK_EQUAL(expected, output.str());
}

TEST_FIXTURE(XmlTestReporterFixture, SingleSuccessfulTestReportSummaryFormat)
{
    TestDetails const details("TestName", "DefaultSuite", "", 0);

    reporter.ReportTestStart(details);
    reporter.ReportSummary(1, 0, 0, 0.1f);

    const char* expected = "<?xml version=\"1.0\"?>"
                           "<unittest-results tests=\"1\" failedtests=\"0\" failures=\"0\" time=\"0.1\">"
                           "<test suite=\"DefaultSuite\" name=\"TestName\" time=\"0\"/>"
                           "</unittest-results>";

    CHECK_EQUAL(expected, output.str());
}

TEST_FIXTURE(XmlTestReporterFixture, SingleFailedTestReportSummaryFormat)
{
    TestDetails const details("A Test", "suite", "A File", 4321);

    reporter.ReportTestStart(details);
    reporter.ReportFailure(details, "A Failure");
    reporter.ReportSummary(1, 1, 1, 0.1f);

    const char* expected = "<?xml version=\"1.0\"?>"
                           "<unittest-results tests=\"1\" failedtests=\"1\" failures=\"1\" time=\"0.1\">"
                           "<test suite=\"suite\" name=\"A Test\" time=\"0\">"
                           "<failure message=\"A File(4321) : A Failure\"/>"
                           "</test>"
                           "</unittest-results>";

    CHECK_EQUAL(expected, output.str());
}

TEST_FIXTURE(XmlTestReporterFixture, FailureMessageIsXMLEscaped)
{
    TestDetails const details("TestName", "suite", "filename.h", 4321);

    reporter.ReportTestStart(details);
    reporter.ReportFailure(details, "\"\'&<>");
    reporter.ReportTestFinish(details, false, 0.1f);
    reporter.ReportSummary(1, 1, 1, 0.1f);

    char const* expected = "<?xml version=\"1.0\"?>"
                           "<unittest-results tests=\"1\" failedtests=\"1\" failures=\"1\" time=\"0.1\">"
                           "<test suite=\"suite\" name=\"TestName\" time=\"0.1\">"
                           "<failure message=\"filename.h(4321) : &quot;&apos;&amp;&lt;&gt;\"/>"
                           "</test>"
                           "</unittest-results>";

    CHECK_EQUAL(expected, output.str());
}

TEST_FIXTURE(XmlTestReporterFixture, OneFailureAndOneSuccess)
{
    TestDetails const failedDetails("FailedTest", "suite", "fail.h", 1);
    reporter.ReportTestStart(failedDetails);
    reporter.ReportFailure(failedDetails, "expected 1 but was 2");
    reporter.ReportTestFinish(failedDetails, false, 0.1f);

    TestDetails const succeededDetails("SucceededTest", "suite", "", 0);
    reporter.ReportTestStart(succeededDetails);
    reporter.ReportTestFinish(succeededDetails, true, 1.0f);
    reporter.ReportSummary(2, 1, 1, 1.1f);

    char const* expected = "<?xml version=\"1.0\"?>"
                           "<unittest-results tests=\"2\" failedtests=\"1\" failures=\"1\" time=\"1.1\">"
                           "<test suite=\"suite\" name=\"FailedTest\" time=\"0.1\">"
                           "<failure message=\"fail.h(1) : expected 1 but was 2\"/>"
                           "</test>"
                           "<test suite=\"suite\" name=\"SucceededTest\" time=\"1\"/>"
                           "</unittest-results>";

    CHECK_EQUAL(expected, output.str());
}

TEST_FIXTURE(XmlTestReporterFixture, MultipleFailures)
{
    TestDetails const failedDetails1("FailedTest", "suite", "fail.h", 1);
    TestDetails const failedDetails2("FailedTest", "suite", "fail.h", 31);

    reporter.ReportTestStart(failedDetails1);
    reporter.ReportFailure(failedDetails1, "expected 1 but was 2");
    reporter.ReportFailure(failedDetails2, "expected one but was two");
    reporter.ReportTestFinish(failedDetails1, false, 0.1f);

    reporter.ReportSummary(1, 1, 2, 1.1f);

    char const* expected = "<?xml version=\"1.0\"?>"
                           "<unittest-results tests=\"1\" failedtests=\"1\" failures=\"2\" time=\"1.1\">"
                           "<test suite=\"suite\" name=\"FailedTest\" time=\"0.1\">"
                           "<failure message=\"fail.h(1) : expected 1 but was 2\"/>"
                           "<failure message=\"fail.h(31) : expected one but was two\"/>"
                           "</test>"
                           "</unittest-results>";

    CHECK_EQUAL(expected, output.str());
}

} // namespace

#endif
