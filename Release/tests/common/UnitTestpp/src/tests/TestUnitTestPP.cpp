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

// These are sample tests that show the different features of the framework

namespace
{
TEST(ValidCheckSucceeds)
{
    bool const b = true;
    CHECK(b);
}

TEST(CheckWorksWithPointers)
{
    void* p = (void*)0x100;
    CHECK(p);
    CHECK(p != 0);
}

TEST(ValidCheckEqualSucceeds)
{
    int const x = 3;
    int const y = 3;
    CHECK_EQUAL(x, y);
}

TEST(CheckEqualWorksWithPointers)
{
    void* p = (void*)0;
    CHECK_EQUAL((void*)0, p);
}

TEST(ValidCheckCloseSucceeds)
{
    CHECK_CLOSE(2.0f, 2.001f, 0.01f);
    CHECK_CLOSE(2.001f, 2.0f, 0.01f);
}

TEST(ArrayCloseSucceeds)
{
    float const a1[] = {1, 2, 3};
    float const a2[] = {1, 2.01f, 3};
    CHECK_ARRAY_CLOSE(a1, a2, 3, 0.1f);
}

#ifndef UNITTEST_NO_EXCEPTIONS

TEST(CheckThrowMacroSucceedsOnCorrectException)
{
    struct TestException
    {
    };
    CHECK_THROW(throw TestException(), TestException);
}

TEST(CheckAssertSucceeds) { CHECK_ASSERT(UnitTest::ReportAssert("desc", "file", 0)); }

TEST(CheckThrowMacroFailsOnMissingException)
{
    class NoThrowTest : public UnitTest::Test
    {
    public:
        NoThrowTest() : Test("nothrow") {}
        void DontThrow() const {}

        virtual void RunImpl() const { CHECK_THROW(DontThrow(), int); }
    };

    UnitTest::TestResults results;
    {
        ScopedCurrentTest scopedResults(results);

        NoThrowTest test;
        test.Run();
    }

    CHECK_EQUAL(1, results.GetFailureCount());
}

TEST(CheckThrowMacroFailsOnWrongException)
{
    class WrongThrowTest : public UnitTest::Test
    {
    public:
        WrongThrowTest() : Test("wrongthrow") {}
        virtual void RunImpl() const { CHECK_THROW(throw "oops", int); }
    };

    UnitTest::TestResults results;
    {
        ScopedCurrentTest scopedResults(results);

        WrongThrowTest test;
        test.Run();
    }

    CHECK_EQUAL(1, results.GetFailureCount());
}

#endif

struct SimpleFixture
{
    SimpleFixture() { ++instanceCount; }
    ~SimpleFixture() { --instanceCount; }

    static int instanceCount;
};

int SimpleFixture::instanceCount = 0;

TEST_FIXTURE(SimpleFixture, DefaultFixtureCtorIsCalled) { CHECK(SimpleFixture::instanceCount > 0); }

TEST_FIXTURE(SimpleFixture, OnlyOneFixtureAliveAtATime) { CHECK_EQUAL(1, SimpleFixture::instanceCount); }

void CheckBool(const bool b) { CHECK(b); }

TEST(CanCallCHECKOutsideOfTestFunction) { CheckBool(true); }

} // namespace
