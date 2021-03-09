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

#include <stdarg.h>
#include <vector>

// cstdio doesn't pull in namespace std on VC6, so we do it here.
#if defined(UNITTEST_WIN32) && (_MSC_VER == 1200)
namespace std
{
}
#endif

namespace UnitTest
{
// Function to work around outputing to the console when under WinRT.
static void PrintfWrapper(const char* format, ...)
{
    va_list args;
    va_start(args, format);

#ifdef __cplusplus_winrt
    const auto bufSize = _vscprintf(format, args) + 1; // add 1 for null termination
    std::vector<char> byteArray;
    byteArray.resize(bufSize);
    vsnprintf_s(&byteArray[0], bufSize, bufSize, format, args);

    DWORD bytesWritten;
    HANDLE h = GetStdHandle(STD_OUTPUT_HANDLE);
    WriteFile(h, &byteArray[0], (DWORD)bufSize, &bytesWritten, NULL);
#else
#ifdef _WIN32
    vfprintf_s(stdout, format, args);
#else
    vfprintf(stdout, format, args);
#endif
#endif

    va_end(args);
}

static void ChangeConsoleTextColorToRed()
{
#if defined(__cplusplus_winrt)
#elif defined(_WIN32)
    SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), 0x0004 | 0x0008);
#else
    std::cout << "\033[1;31m";
#endif
}

static void ChangeConsoleTextColorToGreen()
{
#if defined(__cplusplus_winrt)
#elif defined(_WIN32)
    SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), 0x0002 | 0x0008);
#else
    std::cout << "\033[1;32m";
#endif
}

static void ChangeConsoleTextColorToGrey()
{
#if defined(__cplusplus_winrt)
#elif defined(_WIN32)
    SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), FOREGROUND_RED | FOREGROUND_BLUE | FOREGROUND_GREEN);
#else
    std::cout << "\033[0m";
#endif
}

void TestReporterStdout::ReportFailure(TestDetails const& details, char const* failure)
{
#if defined(__APPLE__) || defined(__GNUG__)
    char const* const errorFormat = "%s:%d: error: Failure in %s: %s FAILED\n";
#else
    char const* const errorFormat = "%s(%d): error: Failure in %s: %s FAILED\n";
#endif

    ChangeConsoleTextColorToRed();
    PrintfWrapper(errorFormat, details.filename, details.lineNumber, details.testName, failure);
    ChangeConsoleTextColorToGrey();
    std::fflush(stdout);
}

void TestReporterStdout::ReportTestStart(TestDetails const& test)
{
    const char* format = "Starting test case %s:%s...\n";
    PrintfWrapper(format, test.suiteName, test.testName);
    std::fflush(stdout);
}

void TestReporterStdout::ReportTestFinish(TestDetails const& test, bool passed, float)
{
    if (passed)
    {
        const char* format = "Test case %s:%s ";
        PrintfWrapper(format, test.suiteName, test.testName);
        ChangeConsoleTextColorToGreen();
        PrintfWrapper("PASSED\n");
        ChangeConsoleTextColorToGrey();
    }
    else
    {
        ChangeConsoleTextColorToRed();
        const char* format = "Test case %s:%s FAILED\n";
        PrintfWrapper(format, test.suiteName, test.testName);
        ChangeConsoleTextColorToGrey();
    }
    std::fflush(stdout);
}

void TestReporterStdout::ReportSummary(int const, int const, int const, float) {}

} // namespace UnitTest
