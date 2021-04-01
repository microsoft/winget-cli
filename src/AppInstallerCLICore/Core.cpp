// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "Public/AppInstallerCLICore.h"
#include "Commands/RootCommand.h"
#include "ExecutionContext.h"
#include "Workflows/WorkflowBase.h"
#include <winget/UserSettings.h>
#include "Commands/InstallCommand.h"

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
    }

    int CoreMain(int argc, wchar_t const** argv) try
    {
        init_apartment();

        // Enable all logging for this phase; we will update once we have the arguments
        Logging::Log().EnableChannel(Logging::Channel::All);
        Logging::Log().SetLevel(Logging::Level::Verbose);
        Logging::AddFileLogger();
        Logging::EnableWilFailureTelemetry();

        // Set output to UTF8
        ConsoleOutputCPRestore utf8CP(CP_UTF8);

        Logging::Telemetry().LogStartup();

        // Initiate the background cleanup of the log file location.
        Logging::BeginLogFileCleanup();

        Execution::Context context{ std::cout, std::cin };
        context.EnableCtrlHandler();

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
            std::unique_ptr<Command> subCommand = command->FindSubCommand(invocation);
            while (subCommand)
            {
                command = std::move(subCommand);
                subCommand = command->FindSubCommand(invocation);
            }
            Logging::Telemetry().LogCommand(command->FullName());

            command->ParseArguments(invocation, context.Args);

            // Change logging level to Info if Verbose not requested
            if (!context.Args.Contains(Execution::Args::Type::VerboseLogs))
            {
                Logging::Log().SetLevel(Logging::Level::Info);
            }

            context.UpdateForArgs();

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
            context.Reporter.Error() << Resource::String::DisabledByGroupPolicy << " : "_liv << policy.PolicyName() << std::endl;
            return APPINSTALLER_CLI_ERROR_BLOCKED_BY_POLICY;
        }

        return Execute(context, command);
    }
    // End of the line exceptions that are not ever expected.
    // Telemetry cannot be reliable beyond this point, so don't let these happen.
    catch (...)
    {
        return APPINSTALLER_CLI_ERROR_INTERNAL_ERROR;
    }
}
