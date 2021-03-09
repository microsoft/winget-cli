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

using namespace std;

namespace
{
TEST(CheckSucceedsOnTrue)
{
    bool failure = true;
    {
        RecordingReporter reporter;
        UnitTest::TestResults testResults(&reporter);

        ScopedCurrentTest scopedResults(testResults);
        CHECK(true);

        failure = (testResults.GetFailureCount() > 0);
    }

    CHECK(!failure);
}

TEST(CheckFailsOnFalse)
{
    bool failure = false;
    {
        RecordingReporter reporter;
        UnitTest::TestResults testResults(&reporter);
        ScopedCurrentTest scopedResults(testResults);
        CHECK(false);
        failure = (testResults.GetFailureCount() > 0);
    }

    CHECK(failure);
}

TEST(FailureReportsCorrectTestName)
{
    RecordingReporter reporter;
    {
        UnitTest::TestResults testResults(&reporter);
        ScopedCurrentTest scopedResults(testResults);
        CHECK(false);
    }

    CHECK_EQUAL(m_details.testName, reporter.lastFailedTest);
}

TEST(CheckFailureIncludesCheckContents)
{
    RecordingReporter reporter;
    {
        UnitTest::TestResults testResults(&reporter);
        ScopedCurrentTest scopedResults(testResults);
        const bool yaddayadda = false;
        CHECK(yaddayadda);
    }

    CHECK(strstr(reporter.lastFailedMessage, "yaddayadda"));
}

TEST(CheckEqualSucceedsOnEqual)
{
    bool failure = true;
    {
        RecordingReporter reporter;
        UnitTest::TestResults testResults(&reporter);
        ScopedCurrentTest scopedResults(testResults);
        CHECK_EQUAL(1, 1);
        failure = (testResults.GetFailureCount() > 0);
    }

    CHECK(!failure);
}

TEST(CheckEqualFailsOnNotEqual)
{
    bool failure = false;
    {
        RecordingReporter reporter;
        UnitTest::TestResults testResults(&reporter);
        ScopedCurrentTest scopedResults(testResults);
        CHECK_EQUAL(1, 2);
        failure = (testResults.GetFailureCount() > 0);
    }

    CHECK(failure);
}

TEST(CheckEqualFailureContainsCorrectDetails)
{
    int line = 0;
    RecordingReporter reporter;
    {
        UnitTest::TestResults testResults(&reporter);
        UnitTest::TestDetails const testDetails("testName", "suiteName", "filename", -1);
        ScopedCurrentTest scopedResults(testResults, &testDetails);

        CHECK_EQUAL(1, 123);
        line = __LINE__;
    }

    CHECK_EQUAL("testName", reporter.lastFailedTest);
    CHECK_EQUAL("suiteName", reporter.lastFailedSuite);
    CHECK_EQUAL("filename", reporter.lastFailedFile);
    CHECK_EQUAL(line, reporter.lastFailedLine);
}

int g_sideEffect = 0;
int FunctionWithSideEffects()
{
    ++g_sideEffect;
    return 1;
}

TEST(CheckEqualDoesNotHaveSideEffectsWhenPassing)
{
    g_sideEffect = 0;
    {
        UnitTest::TestResults testResults;
        ScopedCurrentTest scopedResults(testResults);
        CHECK_EQUAL(1, FunctionWithSideEffects());
    }
    CHECK_EQUAL(1, g_sideEffect);
}

TEST(CheckEqualDoesNotHaveSideEffectsWhenFailing)
{
    g_sideEffect = 0;
    {
        UnitTest::TestResults testResults;
        ScopedCurrentTest scopedResults(testResults);
        CHECK_EQUAL(2, FunctionWithSideEffects());
    }
    CHECK_EQUAL(1, g_sideEffect);
}

TEST(CheckCloseSucceedsOnEqual)
{
    bool failure = true;
    {
        RecordingReporter reporter;
        UnitTest::TestResults testResults(&reporter);
        ScopedCurrentTest scopedResults(testResults);
        CHECK_CLOSE(1.0f, 1.001f, 0.01f);
        failure = (testResults.GetFailureCount() > 0);
    }

    CHECK(!failure);
}

TEST(CheckCloseFailsOnNotEqual)
{
    bool failure = false;
    {
        RecordingReporter reporter;
        UnitTest::TestResults testResults(&reporter);
        ScopedCurrentTest scopedResults(testResults);
        CHECK_CLOSE(1.0f, 1.1f, 0.01f);
        failure = (testResults.GetFailureCount() > 0);
    }

    CHECK(failure);
}

TEST(CheckCloseFailureContainsCorrectDetails)
{
    int line = 0;
    RecordingReporter reporter;
    {
        UnitTest::TestResults testResults(&reporter);
        UnitTest::TestDetails testDetails("test", "suite", "filename", -1);
        ScopedCurrentTest scopedResults(testResults, &testDetails);

        CHECK_CLOSE(1.0f, 1.1f, 0.01f);
        line = __LINE__;
    }

    CHECK_EQUAL("test", reporter.lastFailedTest);
    CHECK_EQUAL("suite", reporter.lastFailedSuite);
    CHECK_EQUAL("filename", reporter.lastFailedFile);
    CHECK_EQUAL(line, reporter.lastFailedLine);
}

TEST(CheckCloseDoesNotHaveSideEffectsWhenPassing)
{
    g_sideEffect = 0;
    {
        UnitTest::TestResults testResults;
        ScopedCurrentTest scopedResults(testResults);
        CHECK_CLOSE(1, FunctionWithSideEffects(), 0.1f);
    }
    CHECK_EQUAL(1, g_sideEffect);
}

TEST(CheckCloseDoesNotHaveSideEffectsWhenFailing)
{
    g_sideEffect = 0;
    {
        UnitTest::TestResults testResults;
        ScopedCurrentTest scopedResults(testResults);
        CHECK_CLOSE(2, FunctionWithSideEffects(), 0.1f);
    }
    CHECK_EQUAL(1, g_sideEffect);
}

TEST(CheckArrayCloseSucceedsOnEqual)
{
    bool failure = true;
    {
        RecordingReporter reporter;
        UnitTest::TestResults testResults(&reporter);
        ScopedCurrentTest scopedResults(testResults);
        const float data[4] = {0, 1, 2, 3};
        CHECK_ARRAY_CLOSE(data, data, 4, 0.01f);
        failure = (testResults.GetFailureCount() > 0);
    }

    CHECK(!failure);
}

TEST(CheckArrayCloseFailsOnNotEqual)
{
    bool failure = false;
    {
        RecordingReporter reporter;
        UnitTest::TestResults testResults(&reporter);
        ScopedCurrentTest scopedResults(testResults);

        int const data1[4] = {0, 1, 2, 3};
        int const data2[4] = {0, 1, 3, 3};
        CHECK_ARRAY_CLOSE(data1, data2, 4, 0.01f);

        failure = (testResults.GetFailureCount() > 0);
    }

    CHECK(failure);
}

TEST(CheckArrayCloseFailureIncludesCheckExpectedAndActual)
{
    RecordingReporter reporter;
    {
        UnitTest::TestResults testResults(&reporter);
        ScopedCurrentTest scopedResults(testResults);

        int const data1[4] = {0, 1, 2, 3};
        int const data2[4] = {0, 1, 3, 3};
        CHECK_ARRAY_CLOSE(data1, data2, 4, 0.01f);
    }

    CHECK(strstr(reporter.lastFailedMessage, "xpected [ 0 1 2 3 ]"));
    CHECK(strstr(reporter.lastFailedMessage, "was [ 0 1 3 3 ]"));
}

TEST(CheckArrayCloseFailureContainsCorrectDetails)
{
    int line = 0;
    RecordingReporter reporter;
    {
        UnitTest::TestResults testResults(&reporter);
        UnitTest::TestDetails testDetails("arrayCloseTest", "arrayCloseSuite", "filename", -1);
        ScopedCurrentTest scopedResults(testResults, &testDetails);

        int const data1[4] = {0, 1, 2, 3};
        int const data2[4] = {0, 1, 3, 3};
        CHECK_ARRAY_CLOSE(data1, data2, 4, 0.01f);
        line = __LINE__;
    }

    CHECK_EQUAL("arrayCloseTest", reporter.lastFailedTest);
    CHECK_EQUAL("arrayCloseSuite", reporter.lastFailedSuite);
    CHECK_EQUAL("filename", reporter.lastFailedFile);
    CHECK_EQUAL(line, reporter.lastFailedLine);
}

TEST(CheckArrayCloseFailureIncludesTolerance)
{
    RecordingReporter reporter;
    {
        UnitTest::TestResults testResults(&reporter);
        ScopedCurrentTest scopedResults(testResults);

        float const data1[4] = {0, 1, 2, 3};
        float const data2[4] = {0, 1, 3, 3};
        CHECK_ARRAY_CLOSE(data1, data2, 4, 0.01f);
    }

    CHECK(strstr(reporter.lastFailedMessage, "0.01"));
}

TEST(CheckArrayEqualSuceedsOnEqual)
{
    bool failure = true;
    {
        RecordingReporter reporter;
        UnitTest::TestResults testResults(&reporter);
        ScopedCurrentTest scopedResults(testResults);

        const float data[4] = {0, 1, 2, 3};
        CHECK_ARRAY_EQUAL(data, data, 4);

        failure = (testResults.GetFailureCount() > 0);
    }

    CHECK(!failure);
}

TEST(CheckArrayEqualFailsOnNotEqual)
{
    bool failure = false;
    {
        RecordingReporter reporter;
        UnitTest::TestResults testResults(&reporter);
        ScopedCurrentTest scopedResults(testResults);

        int const data1[4] = {0, 1, 2, 3};
        int const data2[4] = {0, 1, 3, 3};
        CHECK_ARRAY_EQUAL(data1, data2, 4);

        failure = (testResults.GetFailureCount() > 0);
    }

    CHECK(failure);
}

TEST(CheckArrayEqualFailureIncludesCheckExpectedAndActual)
{
    RecordingReporter reporter;
    {
        UnitTest::TestResults testResults(&reporter);
        ScopedCurrentTest scopedResults(testResults);

        int const data1[4] = {0, 1, 2, 3};
        int const data2[4] = {0, 1, 3, 3};
        CHECK_ARRAY_EQUAL(data1, data2, 4);
    }

    CHECK(strstr(reporter.lastFailedMessage, "xpected [ 0 1 2 3 ]"));
    CHECK(strstr(reporter.lastFailedMessage, "was [ 0 1 3 3 ]"));
}

TEST(CheckArrayEqualFailureContainsCorrectInfo)
{
    int line = 0;
    RecordingReporter reporter;
    {
        UnitTest::TestResults testResults(&reporter);
        ScopedCurrentTest scopedResults(testResults);

        int const data1[4] = {0, 1, 2, 3};
        int const data2[4] = {0, 1, 3, 3};
        CHECK_ARRAY_EQUAL(data1, data2, 4);
        line = __LINE__;
    }

    CHECK_EQUAL("CheckArrayEqualFailureContainsCorrectInfo", reporter.lastFailedTest);
    CHECK_EQUAL(__FILE__, reporter.lastFailedFile);
    CHECK_EQUAL(line, reporter.lastFailedLine);
}

float const* FunctionWithSideEffects2()
{
    ++g_sideEffect;
    static float const data[] = {1, 2, 3, 4};
    return data;
}

TEST(CheckArrayCloseDoesNotHaveSideEffectsWhenPassing)
{
    g_sideEffect = 0;
    {
        UnitTest::TestResults testResults;
        ScopedCurrentTest scopedResults(testResults);

        const float data[] = {0, 1, 2, 3};
        CHECK_ARRAY_CLOSE(data, FunctionWithSideEffects2(), 4, 0.01f);
    }
    CHECK_EQUAL(1, g_sideEffect);
}

TEST(CheckArrayCloseDoesNotHaveSideEffectsWhenFailing)
{
    g_sideEffect = 0;
    {
        UnitTest::TestResults testResults;
        ScopedCurrentTest scopedResults(testResults);

        const float data[] = {0, 1, 3, 3};
        CHECK_ARRAY_CLOSE(data, FunctionWithSideEffects2(), 4, 0.01f);
    }

    CHECK_EQUAL(1, g_sideEffect);
}

TEST(CheckArray2DCloseSucceedsOnEqual)
{
    bool failure = true;
    {
        RecordingReporter reporter;
        UnitTest::TestResults testResults(&reporter);
        ScopedCurrentTest scopedResults(testResults);

        const float data[2][2] = {{0, 1}, {2, 3}};
        CHECK_ARRAY2D_CLOSE(data, data, 2, 2, 0.01f);

        failure = (testResults.GetFailureCount() > 0);
    }

    CHECK(!failure);
}

TEST(CheckArray2DCloseFailsOnNotEqual)
{
    bool failure = false;
    {
        RecordingReporter reporter;
        UnitTest::TestResults testResults(&reporter);
        ScopedCurrentTest scopedResults(testResults);

        int const data1[2][2] = {{0, 1}, {2, 3}};
        int const data2[2][2] = {{0, 1}, {3, 3}};
        CHECK_ARRAY2D_CLOSE(data1, data2, 2, 2, 0.01f);

        failure = (testResults.GetFailureCount() > 0);
    }

    CHECK(failure);
}

TEST(CheckArray2DCloseFailureIncludesCheckExpectedAndActual)
{
    RecordingReporter reporter;
    {
        UnitTest::TestResults testResults(&reporter);
        ScopedCurrentTest scopedResults(testResults);

        int const data1[2][2] = {{0, 1}, {2, 3}};
        int const data2[2][2] = {{0, 1}, {3, 3}};

        CHECK_ARRAY2D_CLOSE(data1, data2, 2, 2, 0.01f);
    }

    CHECK(strstr(reporter.lastFailedMessage, "xpected [ [ 0 1 ] [ 2 3 ] ]"));
    CHECK(strstr(reporter.lastFailedMessage, "was [ [ 0 1 ] [ 3 3 ] ]"));
}

TEST(CheckArray2DCloseFailureContainsCorrectDetails)
{
    int line = 0;
    RecordingReporter reporter;
    {
        UnitTest::TestResults testResults(&reporter);
        UnitTest::TestDetails testDetails("array2DCloseTest", "array2DCloseSuite", "filename", -1);
        ScopedCurrentTest scopedResults(testResults, &testDetails);

        int const data1[2][2] = {{0, 1}, {2, 3}};
        int const data2[2][2] = {{0, 1}, {3, 3}};
        CHECK_ARRAY2D_CLOSE(data1, data2, 2, 2, 0.01f);
        line = __LINE__;
    }

    CHECK_EQUAL("array2DCloseTest", reporter.lastFailedTest);
    CHECK_EQUAL("array2DCloseSuite", reporter.lastFailedSuite);
    CHECK_EQUAL("filename", reporter.lastFailedFile);
    CHECK_EQUAL(line, reporter.lastFailedLine);
}

TEST(CheckArray2DCloseFailureIncludesTolerance)
{
    RecordingReporter reporter;
    {
        UnitTest::TestResults testResults(&reporter);
        ScopedCurrentTest scopedResults(testResults);

        float const data1[2][2] = {{0, 1}, {2, 3}};
        float const data2[2][2] = {{0, 1}, {3, 3}};
        CHECK_ARRAY2D_CLOSE(data1, data2, 2, 2, 0.01f);
    }

    CHECK(strstr(reporter.lastFailedMessage, "0.01"));
}

float const* const* FunctionWithSideEffects3()
{
    ++g_sideEffect;
    static float const data1[] = {0, 1};
    static float const data2[] = {2, 3};
    static const float* const data[] = {data1, data2};
    return data;
}

TEST(CheckArray2DCloseDoesNotHaveSideEffectsWhenPassing)
{
    g_sideEffect = 0;
    {
        UnitTest::TestResults testResults;
        ScopedCurrentTest scopedResults(testResults);

        const float data[2][2] = {{0, 1}, {2, 3}};
        CHECK_ARRAY2D_CLOSE(data, FunctionWithSideEffects3(), 2, 2, 0.01f);
    }
    CHECK_EQUAL(1, g_sideEffect);
}

TEST(CheckArray2DCloseDoesNotHaveSideEffectsWhenFailing)
{
    g_sideEffect = 0;
    {
        UnitTest::TestResults testResults;
        ScopedCurrentTest scopedResults(testResults);

        const float data[2][2] = {{0, 1}, {3, 3}};
        CHECK_ARRAY2D_CLOSE(data, FunctionWithSideEffects3(), 2, 2, 0.01f);
    }
    CHECK_EQUAL(1, g_sideEffect);
}

} // namespace
