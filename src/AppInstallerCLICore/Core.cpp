// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "Public/AppInstallerCLICore.h"
#include "Commands/RootCommand.h"
#include "ExecutionContext.h"

using namespace winrt;
using namespace winrt::Windows::Foundation;
using namespace AppInstaller::CLI;

namespace AppInstaller::CLI
{
    namespace
    {
        // RAII class to restore the console output codepage.
        struct ConsoleOuputCPRestore
        {
            ConsoleOuputCPRestore(UINT cpToChangeTo)
            {
                m_previousCP = GetConsoleOutputCP();
                LOG_LAST_ERROR_IF(!SetConsoleOutputCP(cpToChangeTo));
            }

            ~ConsoleOuputCPRestore()
            {
                SetConsoleOutputCP(m_previousCP);
            }

            ConsoleOuputCPRestore(const ConsoleOuputCPRestore&) = delete;
            ConsoleOuputCPRestore& operator=(const ConsoleOuputCPRestore&) = delete;

            ConsoleOuputCPRestore(ConsoleOuputCPRestore&&) = delete;
            ConsoleOuputCPRestore& operator=(ConsoleOuputCPRestore&&) = delete;

        private:
            UINT m_previousCP = 0;
        };
    }

    int CoreMain(int argc, wchar_t const** argv) try
    {
        init_apartment();

        // Set output to UTF8
        ConsoleOuputCPRestore utf8CP(CP_UTF8);

        // Enable logging (*all* for now, TODO: add common arguments to allow control of logging)
        Logging::Log().EnableChannel(Logging::Channel::All);
        Logging::Log().SetLevel(Logging::Level::Verbose);
        Logging::AddFileLogger();
        Logging::EnableWilFailureTelemetry();

        Logging::Telemetry().LogStartup();

        Execution::Context context{ std::cout, std::cin };
        context.Reporter.EnableCtrlHandler();

        // Convert incoming wide char args to UTF8
        std::vector<std::string> utf8Args;
        for (int i = 1; i < argc; ++i)
        {
            utf8Args.emplace_back(Utility::ConvertToUTF8(argv[i]));
        }

        AICLI_LOG(CLI, Info, << "AppInstaller CLI invoked with arguments:" << [&]() {
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
            command->ValidateArguments(context.Args);
        }
        // Exceptions specific to parsing the arguments of a command
        catch (const CommandException& ce)
        {
            command->OutputHelp(context.Reporter, &ce);
            AICLI_LOG(CLI, Error, << "Error encountered parsing command line: " << ce.Message());
            return APPINSTALLER_CLI_ERROR_INVALID_CL_ARGUMENTS;
        }

        try
        {
            command->Execute(context);
        }
        // Exceptions that may occur in the process of executing an arbitrary command
        catch (const winrt::hresult_error& hre)
        {
            // TODO: Better error output
            std::string message = Utility::ConvertToUTF8(hre.message());
            context.Reporter.ShowMsg("An error occurred while executing the command: " + message, Execution::Reporter::Level::Error);
            AICLI_LOG(CLI, Error, << "Error encountered executing command: " << message);
            return APPINSTALLER_CLI_ERROR_COMMAND_FAILED;
        }
        catch (const std::exception& e)
        {
            // TODO: Better error output
            context.Reporter.ShowMsg("An error occurred while executing the command: " + std::string(e.what()), Execution::Reporter::Level::Error);
            AICLI_LOG(CLI, Error, << "Error encountered executing command: " << e.what());
            return APPINSTALLER_CLI_ERROR_COMMAND_FAILED;
        }

        Logging::Telemetry().LogCommandSuccess(command->FullName());
        return 0;
    }
    // End of the line exceptions that are not ever expected.
    // Telemetry cannot be reliable beyond this point, so don't let these happen.
    catch (const winrt::hresult_error&)
    {
        return APPINSTALLER_CLI_ERROR_INTERNAL_ERROR;
    }
    catch (const std::exception&)
    {
        return APPINSTALLER_CLI_ERROR_INTERNAL_ERROR;
    }
    catch (...)
    {
        return APPINSTALLER_CLI_ERROR_INTERNAL_ERROR;
    }

}
