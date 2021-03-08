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

namespace UnitTest
{
Timer::Timer() : m_threadHandle(::GetCurrentThread()), m_startTime(0)
{
#if defined(UNITTEST_WIN32) && (_MSC_VER == 1200) // VC6 doesn't have DWORD_PTR
    typedef unsigned long DWORD_PTR;
#endif

    DWORD_PTR systemMask;
    ::GetProcessAffinityMask(GetCurrentProcess(), &m_processAffinityMask, &systemMask);
    ::SetThreadAffinityMask(m_threadHandle, 1);
    ::QueryPerformanceFrequency(reinterpret_cast<LARGE_INTEGER*>(&m_frequency));
    ::SetThreadAffinityMask(m_threadHandle, m_processAffinityMask);
}

void Timer::Start() { m_startTime = GetTime(); }

double Timer::GetTimeInMs() const
{
    __int64 const elapsedTime = GetTime() - m_startTime;
    double const seconds = double(elapsedTime) / double(m_frequency);
    return seconds * 1000.0;
}

__int64 Timer::GetTime() const
{
    LARGE_INTEGER curTime;
    ::SetThreadAffinityMask(m_threadHandle, 1);
    ::QueryPerformanceCounter(&curTime);
    ::SetThreadAffinityMask(m_threadHandle, m_processAffinityMask);
    return curTime.QuadPart;
}

void TimeHelpers::SleepMs(int ms) { ::Sleep(ms); }

} // namespace UnitTest
