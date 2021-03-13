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

#include <cassert>
#include <stdarg.h>

namespace UnitTest
{
TestList::TestList() : m_head(nullptr), m_tail(nullptr) {}

void TestList::Clear()
{
    m_head = nullptr;
    m_tail = nullptr;
}

void TestList::Add(Test* test)
{
    if (m_tail == 0)
    {
        assert(m_head == 0);
        m_head = test;
        m_tail = test;
    }
    else
    {
        m_tail->m_nextTest = test;
        m_tail = test;
    }
}

Test* TestList::GetFirst() const { return m_head; }

bool TestList::IsEmpty() const { return m_head == nullptr; }

ListAdder::ListAdder(TestList& list, Test* test, ...)
{
    char* arg;
    va_list argList;
    va_start(argList, test);
    for (arg = va_arg(argList, char*); arg != nullptr; arg = va_arg(argList, char*))
    {
        char* key = arg;
        arg = va_arg(argList, char*);
        if (arg != nullptr)
        {
            char* value = arg;
            test->m_properties.Add(key, value);
        }
    }
    va_end(argList);

    // If on windows we could be either desktop or winrt. Make a requires property for the correct version.
    // Only a desktop runner environment can execute a desktop test case and vice versa on winrt.
    // This starts with visual studio versions after VS 2012.
#if defined(_MSC_VER) && (_MSC_VER >= 1800)
#ifdef __cplusplus_winrt
    test->m_properties.Add("Requires", "winrt");
#else
    test->m_properties.Add("Requires", "desktop");
#endif
#endif

    list.Add(test);
}

#if defined(__clang__)
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wreturn-type-c-linkage"
#endif
extern "C" UNITTEST_LINKAGE TestList& GetTestList()
#if defined(__clang__)
#pragma clang diagnostic pop
#endif
{
    static TestList GLOBAL_TESTLIST;
    return GLOBAL_TESTLIST;
}

} // namespace UnitTest
