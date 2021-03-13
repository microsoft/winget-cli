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

#ifndef UNITTEST_SCOPEDCURRENTTEST_H
#define UNITTEST_SCOPEDCURRENTTEST_H

#include "../CurrentTest.h"
#include <cstddef>

class ScopedCurrentTest
{
public:
    ScopedCurrentTest()
        : m_oldTestResults(UnitTest::CurrentTest::Results()), m_oldTestDetails(UnitTest::CurrentTest::Details())
    {
    }

    explicit ScopedCurrentTest(UnitTest::TestResults& newResults, const UnitTest::TestDetails* newDetails = NULL)
        : m_oldTestResults(UnitTest::CurrentTest::Results()), m_oldTestDetails(UnitTest::CurrentTest::Details())
    {
        UnitTest::CurrentTest::Results() = &newResults;

        if (newDetails != NULL) UnitTest::CurrentTest::Details() = newDetails;
    }

    ~ScopedCurrentTest()
    {
        UnitTest::CurrentTest::Results() = m_oldTestResults;
        UnitTest::CurrentTest::Details() = m_oldTestDetails;
    }

private:
    UnitTest::TestResults* m_oldTestResults;
    const UnitTest::TestDetails* m_oldTestDetails;
};

#endif
