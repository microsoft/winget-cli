// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "Public/AppInstallerCLICore.h"
#include "Commands/RootCommand.h"
#include "ExecutionContext.h"
#include "Workflows/WorkflowBase.h"
#include <winget/UserSettings.h>
#include "Commands/InstallCommand.h"
#include "COMContext.h"
#include <AppInstallerFileLogger.h>
#include <winget/OutputDebugStringLogger.h>

#ifndef AICLI_DISABLE_TEST_HOOKS
#include <winget/Debugging.h>
#endif

using namespace winrt;
using namespace winrt::Windows::Foundation;
using namespace AppInstaller::CLI;
using namespace AppInstaller::Utility::literals;

namespace AppInstaller::CLI
{
    namespace
    {
        // RAII class to restore the console output codepage.
        struct ConsoleOutputCPRestore
        {
            ConsoleOutputCPRestore(UINT cpToChangeTo)
            {
                m_previousCP = GetConsoleOutputCP();
                LOG_LAST_ERROR_IF(!SetConsoleOutputCP(cpToChangeTo));
            }

            ~ConsoleOutputCPRestore()
            {
                SetConsoleOutputCP(m_previousCP);
            }

            ConsoleOutputCPRestore(const ConsoleOutputCPRestore&) = delete;
            ConsoleOutputCPRestore& operator=(const ConsoleOutputCPRestore&) = delete;

            ConsoleOutputCPRestore(ConsoleOutputCPRestore&&) = delete;
            ConsoleOutputCPRestore& operator=(ConsoleOutputCPRestore&&) = delete;

        private:
            UINT m_previousCP = 0;
        };

        void __CRTDECL abort_signal_handler(int)
        {
#ifndef AICLI_DISABLE_TEST_HOOKS
            if (Settings::User().Get<Settings::Setting::EnableSelfInitiatedMinidump>())
            {
                Debugging::WriteMinidump();
            }
#endif

            std::_Exit(APPINSTALLER_CLI_ERROR_INTERNAL_ERROR);
        }
    }

    int CoreMain(int argc, wchar_t const** argv) try
    {
        std::signal(SIGABRT, abort_signal_handler);

        init_apartment();

#ifndef AICLI_DISABLE_TEST_HOOKS
        // We have to do this here so the auto minidump config initialization gets caught
        Logging::OutputDebugStringLogger::Add();
        Logging::Log().EnableChannel(Logging::Channel::All);
        Logging::Log().SetLevel(Logging::Level::Verbose);

        if (Settings::User().Get<Settings::Setting::EnableSelfInitiatedMinidump>())
        {
            Debugging::EnableSelfInitiatedMinidump();
        }

        Logging::OutputDebugStringLogger::Remove();
#endif

        Logging::UseGlobalTelemetryLoggerActivityIdOnly();

        Execution::Context context{ std::cout, std::cin };
        auto previousThreadGlobals = context.SetForCurrentThread();

        // Set up debug string logging during initialization
        Logging::OutputDebugStringLogger::Add();
        Logging::Log().EnableChannel(Logging::Channel::All);
        Logging::Log().SetLevel(Logging::Level::Verbose);

        Logging::Log().EnableChannel(Settings::User().Get<Settings::Setting::LoggingChannelPreference>());
        Logging::Log().SetLevel(Settings::User().Get<Settings::Setting::LoggingLevelPreference>());
        Logging::FileLogger::Add();
        Logging::OutputDebugStringLogger::Remove();
        Logging::EnableWilFailureTelemetry();

        // Set output to UTF8
        ConsoleOutputCPRestore utf8CP(CP_UTF8);

        Logging::Telemetry().SetCaller("winget-cli");
        Logging::Telemetry().LogStartup();

#ifndef AICLI_DISABLE_TEST_HOOKS
        if (!Settings::User().Get<Settings::Setting::KeepAllLogFiles>())
#endif
        {
            // Initiate the background cleanup of the log file location.
            Logging::FileLogger::BeginCleanup();
        }

        context.EnableSignalTerminationHandler();

        context << Workflow::ReportExecutionStage(Workflow::ExecutionStage::ParseArgs);

        // Convert incoming wide char args to UTF8
        std::vector<std::string> utf8Args;
        for (int i = 1; i < argc; ++i)
        {
            utf8Args.emplace_back(Utility::ConvertToUTF8(argv[i]));
        }

        AICLI_LOG(CLI, Info, << "WinGet invoked with arguments:" << [&]() {
                std::stringstream strstr;
                for (const auto& arg : utf8Args)
                {
                    strstr << " '" << arg << '\'';
                }
                return strstr.str();
            }());

        Invocation invocation{ std::move(utf8Args) };

        // The root command is our fallback in the event of very bad or very little input
        std::unique_ptr<Command> command = std::make_unique<RootCommand>();

        try
        {
            // Block CLI execution if WinGetCommandLineInterfaces is disabled by Policy
            if (!Settings::GroupPolicies().IsEnabled(Settings::TogglePolicy::Policy::WinGetCommandLineInterfaces))
            {
                AICLI_LOG(CLI, Error, << "WinGet is disabled by group policy");
                throw Settings::GroupPolicyException(Settings::TogglePolicy::Policy::WinGetCommandLineInterfaces);
            }

            std::unique_ptr<Command> subCommand = command->FindSubCommand(invocation);
            while (subCommand)
            {
                command = std::move(subCommand);
                subCommand = command->FindSubCommand(invocation);
            }
            Logging::Telemetry().LogCommand(command->FullName());

            command->ParseArguments(invocation, context.Args);
            context.UpdateForArgs();
            context.SetExecutingCommand(command.get());
            command->ValidateArguments(context.Args);
        }
        // Exceptions specific to parsing the arguments of a command
        catch (const CommandException& ce)
        {
            command->OutputHelp(context.Reporter, &ce);
            AICLI_LOG(CLI, Error, << "Error encountered parsing command line: " << ce.Message());
            return APPINSTALLER_CLI_ERROR_INVALID_CL_ARGUMENTS;
        }
        catch (const Settings::GroupPolicyException& e)
        {
            // Report any action blocked by Group Policy.
            auto policy = Settings::TogglePolicy::GetPolicy(e.Policy());
            AICLI_LOG(CLI, Error, << "Operation blocked by Group Policy: " << policy.RegValueName());
            context.Reporter.Error() << Resource::String::DisabledByGroupPolicy(policy.PolicyName()) << std::endl;
            return APPINSTALLER_CLI_ERROR_BLOCKED_BY_POLICY;
        }
        catch (...)
        {
            return Workflow::HandleException(context, std::current_exception());
        }

        return Execute(context, command);
    }
    // End of the line exceptions that are not ever expected.
    // Telemetry cannot be reliable beyond this point, so don't let these happen.
    catch (...)
    {
        return APPINSTALLER_CLI_ERROR_INTERNAL_ERROR;
    }

    void ServerInitialize()
    {
#ifndef AICLI_DISABLE_TEST_HOOKS
        // We have to do this here so the auto minidump config initialization gets caught
        Logging::OutputDebugStringLogger::Add();
        Logging::Log().EnableChannel(Logging::Channel::All);
        Logging::Log().SetLevel(Logging::Level::Verbose);

        if (Settings::User().Get<Settings::Setting::EnableSelfInitiatedMinidump>())
        {
            Debugging::EnableSelfInitiatedMinidump();
        }

        Logging::OutputDebugStringLogger::Remove();
#endif

        AppInstaller::CLI::Execution::COMContext::SetLoggers();
    }

    void InProcInitialize()
    {
#ifndef AICLI_DISABLE_TEST_HOOKS
        // We have to do this here so the auto minidump config initialization gets caught
        Logging::OutputDebugStringLogger::Add();
        Logging::Log().EnableChannel(Logging::Channel::All);
        Logging::Log().SetLevel(Logging::Level::Verbose);

        if (Settings::User().Get<Settings::Setting::EnableSelfInitiatedMinidump>())
        {
            Debugging::EnableSelfInitiatedMinidump();
        }

        Logging::OutputDebugStringLogger::Remove();
#endif

        // Explicitly set default channel and level before user settings from PackageManagerSettings
        AppInstaller::CLI::Execution::COMContext::SetLoggers(AppInstaller::Logging::Channel::Defaults, AppInstaller::Logging::Level::Info);
    }
}
