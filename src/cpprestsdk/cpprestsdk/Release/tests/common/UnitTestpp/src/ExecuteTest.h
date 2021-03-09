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

#ifndef UNITTEST_EXECUTE_TEST_H
#define UNITTEST_EXECUTE_TEST_H

#include "../config.h"
#include "AssertException.h"
#include "CurrentTest.h"
#include "ExceptionMacros.h"
#include "MemoryOutStream.h"
#include "TestDetails.h"
#include "TestResults.h"

#ifdef UNITTEST_NO_EXCEPTIONS
#include "ReportAssertImpl.h"
#endif

#ifdef UNITTEST_POSIX
#include "Posix/SignalTranslator.h"
#endif

#include <iostream>

namespace UnitTest
{
template<typename T>
void ExecuteTest(T& testObject, TestDetails const& details, bool isMockTest)
{
    if (isMockTest == false)
    {
        CurrentTest::SetDetails(&details);
    }

#ifdef UNITTEST_NO_EXCEPTIONS
    if (UNITTEST_SET_ASSERT_JUMP_TARGET() == 0)
    {
#endif
#ifndef UNITTEST_POSIX
        UT_TRY({ testObject.RunImpl(); })
#else
    UT_TRY({
        UNITTEST_THROW_SIGNALS_POSIX_ONLY
        testObject.RunImpl();
    })
#endif
        UT_CATCH(AssertException, e, { (void)e; })
        UT_CATCH(std::exception, e, {
            MemoryOutStream stream;
            stream << "Unhandled exception: " << e.what();
            CurrentTest::Results()->OnTestFailure(details, stream.GetText());
        })
        UT_CATCH_ALL({ CurrentTest::Results()->OnTestFailure(details, "Unhandled exception: test crashed"); })
#ifdef UNITTEST_NO_EXCEPTIONS
    }
#endif
}

} // namespace UnitTest

#endif
