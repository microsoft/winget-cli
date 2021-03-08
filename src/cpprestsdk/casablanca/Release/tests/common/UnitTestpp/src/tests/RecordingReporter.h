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

#ifndef UNITTEST_RECORDINGREPORTER_H
#define UNITTEST_RECORDINGREPORTER_H

#include "../TestDetails.h"
#include "../TestReporter.h"
#include <cstring>

struct RecordingReporter : public UnitTest::TestReporter
{
private:
    enum
    {
        kMaxStringLength = 256
    };

public:
    RecordingReporter()
        : testRunCount(0)
        , testFailedCount(0)
        , lastFailedLine(0)
        , testFinishedCount(0)
        , lastFinishedTestTime(0)
        , summaryTotalTestCount(0)
        , summaryFailedTestCount(0)
        , summaryFailureCount(0)
        , summarySecondsElapsed(0)
    {
        lastStartedSuite[0] = '\0';
        lastStartedTest[0] = '\0';
        lastFailedFile[0] = '\0';
        lastFailedSuite[0] = '\0';
        lastFailedTest[0] = '\0';
        lastFailedMessage[0] = '\0';
        lastFinishedSuite[0] = '\0';
        lastFinishedTest[0] = '\0';
    }

    virtual void ReportTestStart(UnitTest::TestDetails const& test)
    {
        using namespace std;

        ++testRunCount;
        strcpy(lastStartedSuite, test.suiteName);
        strcpy(lastStartedTest, test.testName);
    }

    virtual void ReportFailure(UnitTest::TestDetails const& test, char const* failure)
    {
        using namespace std;

        ++testFailedCount;
        strcpy(lastFailedFile, test.filename);
        lastFailedLine = test.lineNumber;
        strcpy(lastFailedSuite, test.suiteName);
        strcpy(lastFailedTest, test.testName);
        strcpy(lastFailedMessage, failure);
    }

    virtual void ReportTestFinish(UnitTest::TestDetails const& test, bool, float testDuration)
    {
        using namespace std;

        ++testFinishedCount;
        strcpy(lastFinishedSuite, test.suiteName);
        strcpy(lastFinishedTest, test.testName);
        lastFinishedTestTime = testDuration;
    }

    virtual void ReportSummary(int totalTestCount, int failedTestCount, int failureCount, float secondsElapsed)
    {
        summaryTotalTestCount = totalTestCount;
        summaryFailedTestCount = failedTestCount;
        summaryFailureCount = failureCount;
        summarySecondsElapsed = secondsElapsed;
    }

    int testRunCount;
    char lastStartedSuite[kMaxStringLength];
    char lastStartedTest[kMaxStringLength];

    int testFailedCount;
    char lastFailedFile[kMaxStringLength];
    int lastFailedLine;
    char lastFailedSuite[kMaxStringLength];
    char lastFailedTest[kMaxStringLength];
    char lastFailedMessage[kMaxStringLength];

    int testFinishedCount;
    char lastFinishedSuite[kMaxStringLength];
    char lastFinishedTest[kMaxStringLength];
    float lastFinishedTestTime;

    int summaryTotalTestCount;
    int summaryFailedTestCount;
    int summaryFailureCount;
    float summarySecondsElapsed;
};

#endif
