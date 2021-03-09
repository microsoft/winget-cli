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

#include "XmlTestReporter.h"
#include <iostream>
#include <sstream>

using std::ostream;
using std::ostringstream;
using std::string;

namespace
{
void ReplaceChar(string& str, char c, string const& replacement)
{
    for (size_t pos = str.find(c); pos != string::npos; pos = str.find(c, pos + 1))
        str.replace(pos, 1, replacement);
}

string XmlEscape(string const& value)
{
    string escaped = value;

    ReplaceChar(escaped, '&', "&amp;");
    ReplaceChar(escaped, '<', "&lt;");
    ReplaceChar(escaped, '>', "&gt;");
    ReplaceChar(escaped, '\'', "&apos;");
    ReplaceChar(escaped, '\"', "&quot;");

    return escaped;
}

string BuildFailureMessage(string const& file, int line, string const& message)
{
    ostringstream failureMessage;
    failureMessage << file << "(" << line << ") : " << message;
    return failureMessage.str();
}

} // namespace

namespace UnitTest
{
XmlTestReporter::XmlTestReporter(ostream& ostream) : m_ostream(ostream) {}

void XmlTestReporter::ReportSummary(int totalTestCount, int failedTestCount, int failureCount, float secondsElapsed)
{
    AddXmlElement(m_ostream, NULL);

    BeginResults(m_ostream, totalTestCount, failedTestCount, failureCount, secondsElapsed);

    DeferredTestResultList const& results = GetResults();
    for (DeferredTestResultList::const_iterator i = results.begin(); i != results.end(); ++i)
    {
        BeginTest(m_ostream, *i);

        if (i->failed) AddFailure(m_ostream, *i);

        EndTest(m_ostream, *i);
    }

    EndResults(m_ostream);
}

void XmlTestReporter::AddXmlElement(ostream& os, char const* encoding)
{
    os << "<?xml version=\"1.0\"";

    if (encoding != NULL) os << " encoding=\"" << encoding << "\"";

    os << "?>";
}

void XmlTestReporter::BeginResults(
    std::ostream& os, int totalTestCount, int failedTestCount, int failureCount, float secondsElapsed)
{
    os << "<unittest-results"
       << " tests=\"" << totalTestCount << "\""
       << " failedtests=\"" << failedTestCount << "\""
       << " failures=\"" << failureCount << "\""
       << " time=\"" << secondsElapsed << "\""
       << ">";
}

void XmlTestReporter::EndResults(std::ostream& os) { os << "</unittest-results>"; }

void XmlTestReporter::BeginTest(std::ostream& os, DeferredTestResult const& result)
{
    os << "<test"
       << " suite=\"" << result.suiteName << "\""
       << " name=\"" << result.testName << "\""
       << " time=\"" << result.timeElapsed << "\"";
}

void XmlTestReporter::EndTest(std::ostream& os, DeferredTestResult const& result)
{
    if (result.failed)
        os << "</test>";
    else
        os << "/>";
}

void XmlTestReporter::AddFailure(std::ostream& os, DeferredTestResult const& result)
{
    os << ">"; // close <test> element

    for (DeferredTestResult::FailureVec::const_iterator it = result.failures.begin(); it != result.failures.end(); ++it)
    {
        string const escapedMessage = XmlEscape(std::string(it->failureStr));
        string const message = BuildFailureMessage(result.failureFile, it->lineNumber, escapedMessage);

        os << "<failure"
           << " message=\"" << message << "\""
           << "/>";
    }
}

} // namespace UnitTest

#endif
