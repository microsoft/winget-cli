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

#ifndef UNITTEST_TESTMACROS_H
#define UNITTEST_TESTMACROS_H

#include "../config.h"
#include "AssertException.h"
#include "ExceptionMacros.h"
#include "ExecuteTest.h"
#include "MemoryOutStream.h"
#include "TestDetails.h"
#include "TestList.h"
#include "TestSuite.h"

#ifndef UNITTEST_POSIX
#define UNITTEST_THROW_SIGNALS_POSIX_ONLY
#else
#include "Posix/SignalTranslator.h"
#endif

#ifdef TEST
#error UnitTest++ redefines TEST
#endif

#ifdef TEST_EX
#error UnitTest++ redefines TEST_EX
#endif

#ifdef TEST_FIXTURE_EX
#error UnitTest++ redefines TEST_FIXTURE_EX
#endif

#ifndef CREATED_GET_TEST_LIST
#define CREATED_GET_TEST_LIST

#ifdef _WIN32
#define _DLL_EXPORT __declspec(dllexport)
#elif __APPLE__
#define _DLL_EXPORT __attribute__((visibility("default")))
#else
#define _DLL_EXPORT
#endif

namespace UnitTest
{
#if defined(__clang__)
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wreturn-type-c-linkage"
#endif
extern "C" _DLL_EXPORT TestList& __cdecl GetTestList();
#if defined(__clang__)
#pragma clang diagnostic pop
#endif
} // namespace UnitTest
#endif

#define SUITE(Name)                                                                                                    \
    namespace Suite##Name                                                                                              \
    {                                                                                                                  \
        namespace UnitTestSuite                                                                                        \
        {                                                                                                              \
        inline char const* GetSuiteName() { return #Name; }                                                            \
        }                                                                                                              \
    }                                                                                                                  \
    namespace Suite##Name

#ifdef _WIN32
#define TEST_EX(Name, List, ...)                                                                                       \
    class Test##Name : public UnitTest::Test                                                                           \
    {                                                                                                                  \
    public:                                                                                                            \
        Test##Name() : Test(#Name, UnitTestSuite::GetSuiteName(), __FILE__, __LINE__) {}                               \
                                                                                                                       \
    private:                                                                                                           \
        virtual void RunImpl() const;                                                                                  \
    } test##Name##Instance;                                                                                            \
                                                                                                                       \
    UnitTest::ListAdder adder##Name(List, &test##Name##Instance, __VA_ARGS__, NULL);                                   \
                                                                                                                       \
    void Test##Name::RunImpl() const

#else
#define TEST_EX(Name, List, ...)                                                                                       \
    class Test##Name : public UnitTest::Test                                                                           \
    {                                                                                                                  \
    public:                                                                                                            \
        Test##Name() : Test(#Name, UnitTestSuite::GetSuiteName(), __FILE__, __LINE__) {}                               \
                                                                                                                       \
    private:                                                                                                           \
        virtual void RunImpl() const;                                                                                  \
    } test##Name##Instance;                                                                                            \
                                                                                                                       \
    UnitTest::ListAdder adder##Name(List, &test##Name##Instance, ##__VA_ARGS__, nullptr);                              \
                                                                                                                       \
    void Test##Name::RunImpl() const
#endif

#ifdef _WIN32
#define TEST(Name, ...) TEST_EX(Name, UnitTest::GetTestList(), __VA_ARGS__)
#else
#define TEST(Name, ...) TEST_EX(Name, UnitTest::GetTestList(), ##__VA_ARGS__)
#endif

#ifdef _WIN32
#define TEST_FIXTURE_EX(Fixture, Name, List, ...)                                                                      \
    class Fixture##Name##Helper : public Fixture                                                                       \
    {                                                                                                                  \
    public:                                                                                                            \
        explicit Fixture##Name##Helper(UnitTest::TestDetails const& details) : m_details(details) {}                   \
        void RunImpl();                                                                                                \
        UnitTest::TestDetails const& m_details;                                                                        \
                                                                                                                       \
    private:                                                                                                           \
        Fixture##Name##Helper(Fixture##Name##Helper const&);                                                           \
        Fixture##Name##Helper& operator=(Fixture##Name##Helper const&);                                                \
    };                                                                                                                 \
                                                                                                                       \
    class Test##Fixture##Name : public UnitTest::Test                                                                  \
    {                                                                                                                  \
    public:                                                                                                            \
        Test##Fixture##Name() : Test(#Name, UnitTestSuite::GetSuiteName(), __FILE__, __LINE__) {}                      \
                                                                                                                       \
    private:                                                                                                           \
        virtual void RunImpl() const;                                                                                  \
    } test##Fixture##Name##Instance;                                                                                   \
                                                                                                                       \
    UnitTest::ListAdder adder##Fixture##Name(List, &test##Fixture##Name##Instance, __VA_ARGS__, NULL);                 \
                                                                                                                       \
    void Test##Fixture##Name::RunImpl() const                                                                          \
    {                                                                                                                  \
        volatile bool ctorOk = false;                                                                                  \
        UT_TRY({                                                                                                       \
            Fixture##Name##Helper fixtureHelper(m_details);                                                            \
            ctorOk = true;                                                                                             \
            UnitTest::ExecuteTest(fixtureHelper, m_details, false);                                                    \
        })                                                                                                             \
        UT_CATCH(UnitTest::AssertException, e, { (void)e; })                                                           \
        UT_CATCH(std::exception, e, {                                                                                  \
            UnitTest::MemoryOutStream stream;                                                                          \
            stream << "Unhandled exception: " << e.what();                                                             \
            UnitTest::CurrentTest::Results()->OnTestFailure(m_details, stream.GetText());                              \
        })                                                                                                             \
        UT_CATCH_ALL({                                                                                                 \
            if (ctorOk)                                                                                                \
            {                                                                                                          \
                UnitTest::CurrentTest::Results()->OnTestFailure(                                                       \
                    UnitTest::TestDetails(m_details, __LINE__),                                                        \
                    "Unhandled exception while destroying fixture " #Fixture);                                         \
            }                                                                                                          \
            else                                                                                                       \
            {                                                                                                          \
                UnitTest::CurrentTest::Results()->OnTestFailure(                                                       \
                    UnitTest::TestDetails(m_details, __LINE__),                                                        \
                    "Unhandled exception while constructing fixture " #Fixture);                                       \
            }                                                                                                          \
        })                                                                                                             \
    }                                                                                                                  \
    void Fixture##Name##Helper::RunImpl()
#else
#define TEST_FIXTURE_EX(Fixture, Name, List, ...)                                                                      \
    class Fixture##Name##Helper : public Fixture                                                                       \
    {                                                                                                                  \
    public:                                                                                                            \
        explicit Fixture##Name##Helper(UnitTest::TestDetails const& details) : m_details(details) {}                   \
        void RunImpl();                                                                                                \
        UnitTest::TestDetails const& m_details;                                                                        \
                                                                                                                       \
    private:                                                                                                           \
        Fixture##Name##Helper(Fixture##Name##Helper const&);                                                           \
        Fixture##Name##Helper& operator=(Fixture##Name##Helper const&);                                                \
    };                                                                                                                 \
                                                                                                                       \
    class Test##Fixture##Name : public UnitTest::Test                                                                  \
    {                                                                                                                  \
    public:                                                                                                            \
        Test##Fixture##Name() : Test(#Name, UnitTestSuite::GetSuiteName(), __FILE__, __LINE__) {}                      \
                                                                                                                       \
    private:                                                                                                           \
        virtual void RunImpl() const;                                                                                  \
    } test##Fixture##Name##Instance;                                                                                   \
                                                                                                                       \
    UnitTest::ListAdder adder##Fixture##Name(List, &test##Fixture##Name##Instance, ##__VA_ARGS__, NULL);               \
                                                                                                                       \
    void Test##Fixture##Name::RunImpl() const                                                                          \
    {                                                                                                                  \
        volatile bool ctorOk = false;                                                                                  \
        UT_TRY({                                                                                                       \
            Fixture##Name##Helper fixtureHelper(m_details);                                                            \
            ctorOk = true;                                                                                             \
            UnitTest::ExecuteTest(fixtureHelper, m_details, false);                                                    \
        })                                                                                                             \
        UT_CATCH(UnitTest::AssertException, e, { (void)e; })                                                           \
        UT_CATCH(std::exception, e, {                                                                                  \
            UnitTest::MemoryOutStream stream;                                                                          \
            stream << "Unhandled exception: " << e.what();                                                             \
            UnitTest::CurrentTest::Results()->OnTestFailure(m_details, stream.GetText());                              \
        })                                                                                                             \
        UT_CATCH_ALL({                                                                                                 \
            if (ctorOk)                                                                                                \
            {                                                                                                          \
                UnitTest::CurrentTest::Results()->OnTestFailure(                                                       \
                    UnitTest::TestDetails(m_details, __LINE__),                                                        \
                    "Unhandled exception while destroying fixture " #Fixture);                                         \
            }                                                                                                          \
            else                                                                                                       \
            {                                                                                                          \
                UnitTest::CurrentTest::Results()->OnTestFailure(                                                       \
                    UnitTest::TestDetails(m_details, __LINE__),                                                        \
                    "Unhandled exception while constructing fixture " #Fixture);                                       \
            }                                                                                                          \
        })                                                                                                             \
    }                                                                                                                  \
    void Fixture##Name##Helper::RunImpl()
#endif

#ifdef _WIN32
#define TEST_FIXTURE(Fixture, Name, ...) TEST_FIXTURE_EX(Fixture, Name, UnitTest::GetTestList(), __VA_ARGS__)
#else
#define TEST_FIXTURE(Fixture, Name, ...) TEST_FIXTURE_EX(Fixture, Name, UnitTest::GetTestList(), ##__VA_ARGS__)
#endif

#endif
