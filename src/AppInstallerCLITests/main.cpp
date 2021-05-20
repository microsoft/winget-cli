// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#define CATCH_CONFIG_RUNNER
#include <catch.hpp>
#include <winrt/Windows.Foundation.h>
#include <iostream>
#include <string>
#include <vector>

#include <Public/AppInstallerLogging.h>
#include <Public/AppInstallerTelemetry.h>
#include <Telemetry/TraceLogging.h>

#include "TestCommon.h"
#include "TestHooks.h"

using namespace winrt;
using namespace Windows::Foundation;
using namespace std::string_literals;
using namespace AppInstaller;


// Logs the the AppInstaller log target to break up individual tests
struct LoggingBreakListener : public Catch::TestEventListenerBase
{
    using TestEventListenerBase::TestEventListenerBase;

    void testCaseStarting(const Catch::TestCaseInfo& info) override
    {
        Catch::TestEventListenerBase::testCaseStarting(info);
        AICLI_LOG(Test, Info, << "========== Test Case Begins :: " << info.name << " ==========");
        TestCommon::TempFile::SetTestFailed(false);
    }

    void testCaseEnded(const Catch::TestCaseStats& testCaseStats) override
    {
        AICLI_LOG(Test, Info, << "========== Test Case Ends :: " << currentTestCaseInfo->name << " ==========");
        if (!testCaseStats.totals.delta(lastTotals).testCases.allOk())
        {
            TestCommon::TempFile::SetTestFailed(true);
        }
        lastTotals = testCaseStats.totals;
        Catch::TestEventListenerBase::testCaseEnded(testCaseStats);
    }

    Catch::Totals lastTotals{};
};
CATCH_REGISTER_LISTENER(LoggingBreakListener);

int main(int argc, char** argv)
{
    init_apartment();

    bool hasSetTestDataBasePath = false;
    bool waitBeforeReturn = false;
    bool keepSQLLogging = false;

    std::vector<char*> args;
    for (int i = 0; i < argc; ++i)
    {
        if ("-ktf"s == argv[i])
        {
            TestCommon::TempFile::SetDestructorBehavior(TestCommon::TempFileDestructionBehavior::Keep);
        }
        else if ("-seof"s == argv[i])
        {
            TestCommon::TempFile::SetDestructorBehavior(TestCommon::TempFileDestructionBehavior::ShellExecuteOnFailure);
        }
        else if ("-log"s == argv[i])
        {
            Logging::AddFileLogger();
        }
        else if ("-logto"s == argv[i])
        {
            ++i;
            Logging::AddFileLogger(argv[i]);
        }
        else if ("-tdd"s == argv[i])
        {
            ++i;
            if (i < argc)
            {
                TestCommon::TestDataFile::SetTestDataBasePath(argv[i]);
                hasSetTestDataBasePath = true;
            }
        }
        else if ("-wait"s == argv[i])
        {
            waitBeforeReturn = true;
        }
        else if ("-logsql"s == argv[i])
        {
            keepSQLLogging = true;
        }
        else
        {
            args.push_back(argv[i]);
        }
    }

    // If not set, use the current executables path
    if (!hasSetTestDataBasePath)
    {
        wchar_t fullFileName[1024];
        DWORD chars = ARRAYSIZE(fullFileName);
        if (QueryFullProcessImageNameW(GetCurrentProcess(), 0, fullFileName, &chars))
        {
            std::filesystem::path filepath{ fullFileName };
            filepath.remove_filename();
            TestCommon::TestDataFile::SetTestDataBasePath(filepath);
        }
    }

    // Enable logging, to force log string building to run.
    // Disable SQL by default, as it generates 10s of MBs of log file and
    // increases the the full test run time by 60% or more.
    // By not creating a log target, it will all be thrown away.
    Logging::Log().EnableChannel(Logging::Channel::All);
    if (!keepSQLLogging)
    {
        Logging::Log().DisableChannel(Logging::Channel::SQL);
    }
    Logging::Log().SetLevel(Logging::Level::Verbose);
    Logging::EnableWilFailureTelemetry();

    // Forcibly enable event writing to catch bugs in that code
    g_IsTelemetryProviderEnabled = true;

    // Force all tests to run against settings inside this container.
    // This prevents test runs from trashing the users actual settings.
    Runtime::TestHook_SetPathOverride(Runtime::PathName::LocalState, Runtime::GetPathTo(Runtime::PathName::LocalState) / "Tests");
    Runtime::TestHook_SetPathOverride(Runtime::PathName::UserFileSettings, Runtime::GetPathTo(Runtime::PathName::UserFileSettings) / "Tests");
    Runtime::TestHook_SetPathOverride(Runtime::PathName::StandardSettings, Runtime::GetPathTo(Runtime::PathName::StandardSettings) / "Tests");
    Runtime::TestHook_SetPathOverride(Runtime::PathName::SecureSettings, Runtime::GetPathTo(Runtime::PathName::Temp) / "WinGet_SecureSettings_Tests");

    int result = Catch::Session().run(static_cast<int>(args.size()), args.data());

    if (waitBeforeReturn)
    {
        // Wait for some input before returning
        std::cin.get();
    }

    return result;
}
