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

#ifndef UNITTEST_TESTRUNNER_H
#define UNITTEST_TESTRUNNER_H

#include "CurrentTest.h"
#include "GlobalSettings.h"
#include "Test.h"
#include "TestList.h"

namespace UnitTest
{
class TestReporter;
class TestResults;
class Timer;

struct True
{
    bool operator()(const Test* const) const { return true; }
};

class TestRunner
{
public:
    UNITTEST_LINKAGE explicit TestRunner(TestReporter& reporter, bool breakOnError = false);
    UNITTEST_LINKAGE ~TestRunner();

    template<class Predicate>
    int RunTestsIf(TestList const& list, const Predicate& predicate, int defaultTestTimeInMs) const
    {
        return RunTestsIf(list, nullptr, predicate, defaultTestTimeInMs);
    }

    template<class Predicate>
    int RunTestsIf(TestList const& list,
                   char const* suiteName,
                   const Predicate& predicate,
                   int defaultTestTimeInMs) const
    {
        Test* curTest = list.GetFirst();

        while (curTest != 0)
        {
            if (IsTestInSuite(curTest, suiteName) && predicate(curTest))
                RunTest(m_result, curTest, defaultTestTimeInMs);

            curTest = curTest->m_nextTest;
        }

        return Finish();
    }

    int RunTests(TestList const& list, char const* suiteName, int defaultTestTimeInMs) const
    {
        return RunTestsIf(list, suiteName, True(), defaultTestTimeInMs);
    }
    int RunTests(TestList const& list, int defaultTestTimeInMs) const
    {
        return RunTestsIf(list, nullptr, True(), defaultTestTimeInMs);
    }

    UNITTEST_LINKAGE TestResults* GetTestResults();

private:
    TestReporter* m_reporter;
    TestResults* m_result;
    Timer* m_timer;

    int GetTestTimeout(Test* const curTest, int const defaultTestTimeInMs) const;

    UNITTEST_LINKAGE int Finish() const;
    UNITTEST_LINKAGE bool IsTestInSuite(const Test* const curTest, char const* suiteName) const;
    UNITTEST_LINKAGE void RunTest(TestResults* const result, Test* const curTest, int const defaultTestTimeInMs) const;
};

} // namespace UnitTest

#endif
