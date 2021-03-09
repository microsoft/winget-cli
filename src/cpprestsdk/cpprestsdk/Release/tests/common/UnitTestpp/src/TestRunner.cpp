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

#include "TestRunner.h"

#include "TestMacros.h"

#if _MSC_VER == 1600
#include <agents.h>
#include <functional>
#else
#include <future>
#endif

#if (defined(ANDROID) || defined(__ANDROID__))
#include <boost/scope_exit.hpp>
#include <jni.h>
#endif

#include <cstdlib>

#if (defined(ANDROID) || defined(__ANDROID__))
namespace crossplat
{
extern std::atomic<JavaVM*> JVM;
}
#endif

namespace UnitTest
{
TestRunner::TestRunner(TestReporter& reporter, bool breakOnError)
    : m_reporter(&reporter), m_result(new TestResults(&reporter, breakOnError)), m_timer(new Timer)
{
    m_timer->Start();
}

TestRunner::~TestRunner()
{
    delete m_result;
    delete m_timer;
}

TestResults* TestRunner::GetTestResults() { return m_result; }

int TestRunner::Finish() const
{
    float const secondsElapsed = static_cast<float>(m_timer->GetTimeInMs() / 1000.0);
    m_reporter->ReportSummary(
        m_result->GetTotalTestCount(), m_result->GetFailedTestCount(), m_result->GetFailureCount(), secondsElapsed);

    return m_result->GetFailureCount();
}

bool TestRunner::IsTestInSuite(const Test* const curTest, char const* suiteName) const
{
    using namespace std;
    return (suiteName == NULL) || !strcmp(curTest->m_details.suiteName, suiteName);
}

#if _MSC_VER == 1600
// std::future and std::thread doesn't exist on Visual Studio 2010 so fall back
// to use agent.
class TestRunnerAgent : public Concurrency::agent
{
public:
    TestRunnerAgent(std::tr1::function<void()> func) : m_func(func) {}

protected:
    void run()
    {
        Concurrency::Context::Oversubscribe(true);
        m_func();
        Concurrency::Context::Oversubscribe(false);
        done();
    }

private:
    std::tr1::function<void()> m_func;
};
#endif

// Logic to decide the timeout for individual test
// 1. If /testtimeout is specified with testrunner arguments, use that timeout.
// 2. Else, if the test has a Timeout property set, use that timeout.
// 3. If both the above properties are not specified, use the default timeout value.
int TestRunner::GetTestTimeout(Test* const curTest, int const defaultTestTimeInMs) const
{
    std::stringstream timeoutstream;
    int timeout = defaultTestTimeInMs;
    if (UnitTest::GlobalSettings::Has("testtimeout"))
    {
        timeoutstream << UnitTest::GlobalSettings::Get("testtimeout");
        timeoutstream >> timeout;
    }
    else if (curTest->m_properties.Has("Timeout"))
    {
        timeoutstream << curTest->m_properties.Get("Timeout");
        timeoutstream >> timeout;
    }
    return timeout;
}

void TestRunner::RunTest(TestResults* const result, Test* const curTest, int const defaultTestTimeInMs) const
{
    if (curTest->m_isMockTest == false) CurrentTest::SetResults(result);

    int maxTestTimeInMs = GetTestTimeout(curTest, defaultTestTimeInMs);

    Timer testTimer;
    testTimer.Start();

    result->OnTestStart(curTest->m_details);

    if (maxTestTimeInMs > 0)
    {
        bool timedOut = false;
#if _MSC_VER == 1600
        TestRunnerAgent testRunnerAgent([&]() { curTest->Run(); });
        testRunnerAgent.start();
        try
        {
            Concurrency::agent::wait(&testRunnerAgent, maxTestTimeInMs);
        }
        catch (const Concurrency::operation_timed_out&)
        {
            timedOut = true;
        }
#else
        // Timed wait requires async execution.
        auto testRunnerFuture = std::async(std::launch::async, [&]() {
#if (defined(ANDROID) || defined(__ANDROID__))
            JNIEnv* env = nullptr;
            auto result = crossplat::JVM.load()->AttachCurrentThread(&env, nullptr);
            if (result != JNI_OK)
            {
                throw std::runtime_error("Could not attach to JVM");
            }
            BOOST_SCOPE_EXIT(void) { crossplat::JVM.load()->DetachCurrentThread(); }
            BOOST_SCOPE_EXIT_END
#endif
            curTest->Run();
        });
        std::chrono::system_clock::time_point totalTime =
            std::chrono::system_clock::now() + std::chrono::milliseconds(maxTestTimeInMs);
        if (testRunnerFuture.wait_until(totalTime) == std::future_status::timeout)
        {
            timedOut = true;
        }
#endif
        if (timedOut)
        {
            MemoryOutStream stream;
            stream << "Test case timed out and is hung. Aborting all remaining test cases. ";
            stream << "Expected under " << maxTestTimeInMs << "ms.";
            result->OnTestFailure(curTest->m_details, stream.GetText());

            abort();
        }
    }
    else
    {
        curTest->Run();
    }

    double const testTimeInMs = testTimer.GetTimeInMs();
    result->OnTestFinish(curTest->m_details, static_cast<float>(testTimeInMs / 1000.0));
}

} // namespace UnitTest
