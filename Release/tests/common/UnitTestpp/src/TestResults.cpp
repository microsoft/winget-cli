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

#ifndef WIN32
#include "signal.h"
#endif

namespace UnitTest
{
TestResults::TestResults(TestReporter* testReporter, bool breakOnError)
    : m_testReporter(testReporter)
    , m_totalTestCount(0)
    , m_failedTestCount(0)
    , m_failureCount(0)
    , m_currentTestFailed(false)
    , m_breakOnError(breakOnError)
{
}

void TestResults::OnTestStart(TestDetails const& test)
{
    ++m_totalTestCount;
    m_currentTestFailed = false;
    if (m_testReporter) m_testReporter->ReportTestStart(test);
}

#ifdef WIN32
#define DEBUG_BREAK() __debugbreak()
#else
#define DEBUG_BREAK() raise(SIGTRAP)
#endif

void TestResults::OnTestFailure(TestDetails const& test, char const* failure)
{
    ++m_failureCount;
    if (!m_currentTestFailed)
    {
        ++m_failedTestCount;
        std::string fullTestName(test.suiteName);
        fullTestName.append(":");
        fullTestName.append(test.testName);
        m_failedTests.push_back(fullTestName);
        m_currentTestFailed = true;
    }

    if (m_testReporter)
    {
        m_testReporter->ReportFailure(test, failure);
        if (m_breakOnError)
        {
            DEBUG_BREAK();
        }
    }
}

void TestResults::OnTestFinish(TestDetails const& test, float secondsElapsed)
{
    if (m_testReporter) m_testReporter->ReportTestFinish(test, !m_currentTestFailed, secondsElapsed);
}

int TestResults::GetTotalTestCount() const { return m_totalTestCount; }

int TestResults::GetFailedTestCount() const { return m_failedTestCount; }

int TestResults::GetFailureCount() const { return m_failureCount; }

const std::vector<std::string>& TestResults::GetFailedTests() const { return m_failedTests; }

} // namespace UnitTest
