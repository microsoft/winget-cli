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

#include "CompositeTestReporter.h"

namespace UnitTest
{
CompositeTestReporter::CompositeTestReporter() : m_reporterCount(0) {}

int CompositeTestReporter::GetReporterCount() const { return m_reporterCount; }

bool CompositeTestReporter::AddReporter(TestReporter* reporter)
{
    if (m_reporterCount == kMaxReporters) return false;

// Safe to ignore we check the size to make sure no buffer overruns before.
#if defined(_MSC_VER)
#pragma warning(push)
#pragma warning(disable : 6386)
#endif
    m_reporters[m_reporterCount++] = reporter;
#if defined(_MSC_VER)
#pragma warning(pop)
#endif

    return true;
}

bool CompositeTestReporter::RemoveReporter(TestReporter* reporter)
{
    for (int index = 0; index < m_reporterCount; ++index)
    {
        if (m_reporters[index] == reporter)
        {
            m_reporters[index] = m_reporters[m_reporterCount - 1];
            --m_reporterCount;
            return true;
        }
    }

    return false;
}

void CompositeTestReporter::ReportFailure(TestDetails const& details, char const* failure)
{
    for (int index = 0; index < m_reporterCount; ++index)
        m_reporters[index]->ReportFailure(details, failure);
}

void CompositeTestReporter::ReportTestStart(TestDetails const& test)
{
    for (int index = 0; index < m_reporterCount; ++index)
        m_reporters[index]->ReportTestStart(test);
}

void CompositeTestReporter::ReportTestFinish(TestDetails const& test, bool passed, float secondsElapsed)
{
    for (int index = 0; index < m_reporterCount; ++index)
        m_reporters[index]->ReportTestFinish(test, passed, secondsElapsed);
}

void CompositeTestReporter::ReportSummary(int totalTestCount,
                                          int failedTestCount,
                                          int failureCount,
                                          float secondsElapsed)
{
    for (int index = 0; index < m_reporterCount; ++index)
        m_reporters[index]->ReportSummary(totalTestCount, failedTestCount, failureCount, secondsElapsed);
}

} // namespace UnitTest
