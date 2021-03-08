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
TEST(CheckEqualWithUnsignedLong)
{
    TestResults results;
    unsigned long something = 2;
    CHECK_EQUAL(something, something);
}

TEST(CheckEqualsWithStringsFailsOnDifferentStrings)
{
    char txt1[] = "Hello";
    char txt2[] = "Hallo";
    TestResults results;
    CheckEqual(results, "txt1", "txt2", txt1, txt2, TestDetails("", "", "", 0));
    CHECK_EQUAL(1, results.GetFailureCount());
}

char txt1[] = "Hello"; // non-const on purpose so no folding of duplicate data
char txt2[] = "Hello";

TEST(CheckEqualsWithStringsWorksOnContentsNonConstNonConst)
{
    char const* const p1 = txt1;
    char const* const p2 = txt2;
    TestResults results;
    CheckEqual(results, "p1", "p2", p1, p2, TestDetails("", "", "", 0));
    CHECK_EQUAL(0, results.GetFailureCount());
}

TEST(CheckEqualsWithStringsWorksOnContentsConstConst)
{
    char* const p1 = txt1;
    char* const p2 = txt2;
    TestResults results;
    CheckEqual(results, "p1", "p2", p1, p2, TestDetails("", "", "", 0));
    CHECK_EQUAL(0, results.GetFailureCount());
}

TEST(CheckEqualsWithStringsWorksOnContentsNonConstConst)
{
    char* const p1 = txt1;
    char const* const p2 = txt2;
    TestResults results;
    CheckEqual(results, "p1", "p2", p1, p2, TestDetails("", "", "", 0));
    CHECK_EQUAL(0, results.GetFailureCount());
}

TEST(CheckEqualsWithStringsWorksOnContentsConstNonConst)
{
    char const* const p1 = txt1;
    char* const p2 = txt2;
    TestResults results;
    CheckEqual(results, "p1", "p2", p1, p2, TestDetails("", "", "", 0));
    CHECK_EQUAL(0, results.GetFailureCount());
}

TEST(CheckEqualsWithStringsWorksOnContentsWithALiteral)
{
    char const* const p1 = txt1;
    TestResults results;
    CheckEqual(results, "Hello", "p1", "Hello", p1, TestDetails("", "", "", 0));
    CHECK_EQUAL(0, results.GetFailureCount());
}

TEST(CheckEqualFailureIncludesCheckExpectedAndActual)
{
    RecordingReporter reporter;
    TestResults results(&reporter);
    const int something = 2;
    CheckEqual(results, "1", "something", 1, something, TestDetails("", "", "", 0));

    using namespace std;
    CHECK(strstr(reporter.lastFailedMessage, "1=1"));
    CHECK(strstr(reporter.lastFailedMessage, "something=2"));
}

TEST(CheckEqualFailureIncludesDetails)
{
    RecordingReporter reporter;
    TestResults results(&reporter);
    TestDetails const details("mytest", "mysuite", "file.h", 101);

    CheckEqual(results, "1", "2", 1, 2, details);

    CHECK_EQUAL("mytest", reporter.lastFailedTest);
    CHECK_EQUAL("mysuite", reporter.lastFailedSuite);
    CHECK_EQUAL("file.h", reporter.lastFailedFile);
    CHECK_EQUAL(101, reporter.lastFailedLine);
}

TEST(CheckCloseTrue)
{
    TestResults results;
    CheckClose(results, 3.001f, 3.0f, 0.1f, TestDetails("", "", "", 0));
    CHECK_EQUAL(0, results.GetFailureCount());
}

TEST(CheckCloseFalse)
{
    TestResults results;
    CheckClose(results, 3.12f, 3.0f, 0.1f, TestDetails("", "", "", 0));
    CHECK_EQUAL(1, results.GetFailureCount());
}

TEST(CheckCloseWithZeroEpsilonWorksForSameNumber)
{
    TestResults results;
    CheckClose(results, 0.1f, 0.1f, 0, TestDetails("", "", "", 0));
    CHECK_EQUAL(0, results.GetFailureCount());
}

TEST(CheckCloseWithNaNFails)
{
    const unsigned int bitpattern = 0xFFFFFFFF;
    float nan;
    std::memcpy(&nan, &bitpattern, sizeof(bitpattern));

    TestResults results;
    CheckClose(results, 3.0f, nan, 0.1f, TestDetails("", "", "", 0));
    CHECK_EQUAL(1, results.GetFailureCount());
}

TEST(CheckCloseWithNaNAgainstItselfFails)
{
    const unsigned int bitpattern = 0xFFFFFFFF;
    float nan;
    std::memcpy(&nan, &bitpattern, sizeof(bitpattern));

    TestResults results;
    CheckClose(results, nan, nan, 0.1f, TestDetails("", "", "", 0));
    CHECK_EQUAL(1, results.GetFailureCount());
}

TEST(CheckCloseFailureIncludesCheckExpectedAndActual)
{
    RecordingReporter reporter;
    TestResults results(&reporter);
    const float expected = 0.9f;
    const float actual = 1.1f;
    CheckClose(results, expected, actual, 0.01f, TestDetails("", "", "", 0));

    using namespace std;
    CHECK(strstr(reporter.lastFailedMessage, "xpected 0.9"));
    CHECK(strstr(reporter.lastFailedMessage, "was 1.1"));
}

TEST(CheckCloseFailureIncludesTolerance)
{
    RecordingReporter reporter;
    TestResults results(&reporter);
    CheckClose(results, 2, 3, 0.01f, TestDetails("", "", "", 0));

    using namespace std;
    CHECK(strstr(reporter.lastFailedMessage, "0.01"));
}

TEST(CheckCloseFailureIncludesDetails)
{
    RecordingReporter reporter;
    TestResults results(&reporter);
    TestDetails const details("mytest", "mysuite", "header.h", 10);

    CheckClose(results, 2, 3, 0.01f, details);

    CHECK_EQUAL("mytest", reporter.lastFailedTest);
    CHECK_EQUAL("mysuite", reporter.lastFailedSuite);
    CHECK_EQUAL("header.h", reporter.lastFailedFile);
    CHECK_EQUAL(10, reporter.lastFailedLine);
}

TEST(CheckArrayEqualTrue)
{
    TestResults results;

    int const array[3] = {1, 2, 3};
    CheckArrayEqual(results, array, array, 3, TestDetails("", "", "", 0));
    CHECK_EQUAL(0, results.GetFailureCount());
}

TEST(CheckArrayEqualFalse)
{
    TestResults results;

    int const array1[3] = {1, 2, 3};
    int const array2[3] = {1, 2, 2};
    CheckArrayEqual(results, array1, array2, 3, TestDetails("", "", "", 0));
    CHECK_EQUAL(1, results.GetFailureCount());
}

TEST(CheckArrayCloseTrue)
{
    TestResults results;

    float const array1[3] = {1.0f, 1.5f, 2.0f};
    float const array2[3] = {1.01f, 1.51f, 2.01f};
    CheckArrayClose(results, array1, array2, 3, 0.02f, TestDetails("", "", "", 0));
    CHECK_EQUAL(0, results.GetFailureCount());
}

TEST(CheckArrayCloseFalse)
{
    TestResults results;

    float const array1[3] = {1.0f, 1.5f, 2.0f};
    float const array2[3] = {1.01f, 1.51f, 2.01f};
    CheckArrayClose(results, array1, array2, 3, 0.001f, TestDetails("", "", "", 0));
    CHECK_EQUAL(1, results.GetFailureCount());
}

TEST(CheckArrayCloseFailureIncludesDetails)
{
    RecordingReporter reporter;
    TestResults results(&reporter);
    TestDetails const details("arrayCloseTest", "arrayCloseSuite", "file", 1337);

    float const array1[3] = {1.0f, 1.5f, 2.0f};
    float const array2[3] = {1.01f, 1.51f, 2.01f};
    CheckArrayClose(results, array1, array2, 3, 0.001f, details);

    CHECK_EQUAL("arrayCloseTest", reporter.lastFailedTest);
    CHECK_EQUAL("arrayCloseSuite", reporter.lastFailedSuite);
    CHECK_EQUAL("file", reporter.lastFailedFile);
    CHECK_EQUAL(1337, reporter.lastFailedLine);
}

TEST(CheckArray2DCloseTrue)
{
    TestResults results;

    float const array1[3][3] = {{1.0f, 1.5f, 2.0f}, {2.0f, 2.5f, 3.0f}, {3.0f, 3.5f, 4.0f}};
    float const array2[3][3] = {{1.01f, 1.51f, 2.01f}, {2.01f, 2.51f, 3.01f}, {3.01f, 3.51f, 4.01f}};
    CheckArray2DClose(results, array1, array2, 3, 3, 0.02f, TestDetails("", "", "", 0));
    CHECK_EQUAL(0, results.GetFailureCount());
}

TEST(CheckArray2DCloseFalse)
{
    TestResults results;

    float const array1[3][3] = {{1.0f, 1.5f, 2.0f}, {2.0f, 2.5f, 3.0f}, {3.0f, 3.5f, 4.0f}};
    float const array2[3][3] = {{1.01f, 1.51f, 2.01f}, {2.01f, 2.51f, 3.01f}, {3.01f, 3.51f, 4.01f}};
    CheckArray2DClose(results, array1, array2, 3, 3, 0.001f, TestDetails("", "", "", 0));
    CHECK_EQUAL(1, results.GetFailureCount());
}

TEST(CheckCloseWithDoublesSucceeds) { CHECK_CLOSE(0.5, 0.5, 0.0001); }

TEST(CheckArray2DCloseFailureIncludesDetails)
{
    RecordingReporter reporter;
    TestResults results(&reporter);
    TestDetails const details("array2DCloseTest", "array2DCloseSuite", "file", 1234);

    float const array1[3][3] = {{1.0f, 1.5f, 2.0f}, {2.0f, 2.5f, 3.0f}, {3.0f, 3.5f, 4.0f}};
    float const array2[3][3] = {{1.01f, 1.51f, 2.01f}, {2.01f, 2.51f, 3.01f}, {3.01f, 3.51f, 4.01f}};
    CheckArray2DClose(results, array1, array2, 3, 3, 0.001f, details);

    CHECK_EQUAL("array2DCloseTest", reporter.lastFailedTest);
    CHECK_EQUAL("array2DCloseSuite", reporter.lastFailedSuite);
    CHECK_EQUAL("file", reporter.lastFailedFile);
    CHECK_EQUAL(1234, reporter.lastFailedLine);
}

} // namespace
