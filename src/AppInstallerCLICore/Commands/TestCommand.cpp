// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"

#ifndef AICLI_DISABLE_TEST_HOOKS

#include "TestCommand.h"
#include "AppInstallerRuntime.h"
#include "TableOutput.h"
#include "Public/ConfigurationSetProcessorFactoryRemoting.h"
#include "Workflows/ConfigurationFlow.h"
#include "Workflows/MSStoreInstallerHandler.h"
#include <winrt/Microsoft.Management.Configuration.h>

using namespace AppInstaller::CLI::Workflow;
using namespace AppInstaller::Utility::literals;

namespace AppInstaller::CLI
{
    namespace
    {
        void LogAndReport(Execution::Context& context, const std::string& message)
        {
            AICLI_LOG(CLI, Info, << message);
            context.Reporter.Info() << message << std::endl;
        }

        HRESULT WaitForShutdown(Execution::Context& context)
        {
            LogAndReport(context, "Waiting for app shutdown event");
            if (!Execution::WaitForAppShutdownEvent())
            {
                LogAndReport(context, "Failed getting app shutdown event");
                return APPINSTALLER_CLI_ERROR_INTERNAL_ERROR;
            }

            LogAndReport(context, "Succeeded waiting for app shutdown event");
            return S_OK;
        }

        HRESULT AppShutdownWindowMessage(Execution::Context& context)
        {
            auto windowHandle = Execution::GetWindowHandle();

            if (windowHandle == NULL)
            {
                LogAndReport(context, "Window was not created");
                return APPINSTALLER_CLI_ERROR_INTERNAL_ERROR;
            }

            if (context.Args.Contains(Execution::Args::Type::Force))
            {
                LogAndReport(context, "Sending WM_QUERYENDSESSION message");
                THROW_LAST_ERROR_IF(!SendMessageTimeout(
                    windowHandle,
                    WM_QUERYENDSESSION,
                    NULL,
                    ENDSESSION_CLOSEAPP,
                    (SMTO_ABORTIFHUNG | SMTO_ERRORONEXIT),
                    5000,
                    NULL));
            }

            HRESULT hr = WaitForShutdown(context);

            if (context.Args.Contains(Execution::Args::Type::Force))
            {
                LogAndReport(context, "Sending WM_ENDSESSION message");
                THROW_LAST_ERROR_IF(!SendMessageTimeout(
                    windowHandle,
                    WM_ENDSESSION,
                    NULL,
                    ENDSESSION_CLOSEAPP,
                    (SMTO_ABORTIFHUNG | SMTO_ERRORONEXIT),
                    5000,
                    NULL));
            }

            return hr;
        }

        void EnsureDSCv3Processor(Execution::Context& context)
        {
            auto& configurationSet = context.Get<Execution::Data::ConfigurationContext>().Set();
            configurationSet.Environment().ProcessorIdentifier(L"dscv3");
        }

        void InvokeGetAllUnits(Execution::Context& context)
        {
            auto& configurationContext = context.Get<Execution::Data::ConfigurationContext>();

            winrt::Microsoft::Management::Configuration::ConfigurationUnit unit;
            unit.Type(Utility::ConvertToUTF16(context.Args.GetArg(Execution::Args::Type::ConfigurationExportResource)));

            auto result = configurationContext.Processor().GetAllUnits(unit);

            if (FAILED(result.ResultInformation().ResultCode()))
            {
                context.Reporter.Error() << "Failed to export: " << WINGET_OSTREAM_FORMAT_HRESULT(result.ResultInformation().ResultCode()) << std::endl;
                AICLI_TERMINATE_CONTEXT(result.ResultInformation().ResultCode());
            }

            for (const auto& resultUnit : result.Units())
            {
                configurationContext.Set().Units().Append(resultUnit);
            }
        }

        // Command to directly invoke the export flow.
        struct TestConfigurationExportCommand final : public Command
        {
            TestConfigurationExportCommand(std::string_view parent) : Command("config-export-units", {}, parent) {}

            std::vector<Argument> GetArguments() const override
            {
                return {
                    Argument{ Execution::Args::Type::OutputFile, Resource::String::OutputFileArgumentDescription, true },
                    Argument{ Execution::Args::Type::ConfigurationExportResource, Resource::String::ConfigureExportResource },
                };
            }

            Resource::LocString ShortDescription() const override
            {
                return "Run config export"_lis;
            }

            Resource::LocString LongDescription() const override
            {
                return "Runs the GetAllUnits configuration method to test export on a DSC v3 directly."_lis;
            }

        protected:
            void ExecuteInternal(Execution::Context& context) const override
            {
                context <<
                    VerifyIsFullPackage <<
                    CreateConfigurationProcessorWithoutFactory <<
                    CreateOrOpenConfigurationSet{ "0.3" } <<
                    EnsureDSCv3Processor <<
                    CreateConfigurationProcessor <<
                    InvokeGetAllUnits <<
                    WriteConfigFile;
            }
        };

        void InvokeFindUnitProcessors(Execution::Context& context)
        {
            auto& configurationContext = context.Get<Execution::Data::ConfigurationContext>();

            winrt::Microsoft::Management::Configuration::FindUnitProcessorsOptions findOptions;

            if (context.Args.Contains(Execution::Args::Type::InstallLocation))
            {
                findOptions.SearchPaths(Utility::ConvertToUTF16(context.Args.GetArg(Execution::Args::Type::InstallLocation)));
                findOptions.SearchPathsExclusive(true);
                findOptions.UnitDetailFlags(winrt::Microsoft::Management::Configuration::ConfigurationUnitDetailFlags::Local);
            }

            auto result = configurationContext.Processor().FindUnitProcessors(findOptions);

            if (result.Size() > 0)
            {
                Execution::TableOutput<2> table(context.Reporter,
                    {
                        "Type"_lis,
                        "Description"_lis
                    });

                for (const auto& resultUnitProcessor : result)
                {
                    table.OutputLine({
                        Utility::ConvertToUTF8(resultUnitProcessor.UnitType()),
                        Utility::ConvertToUTF8(resultUnitProcessor.UnitDescription())
                        });
                }

                table.Complete();
            }
            else
            {
                context.Reporter.Info() << "No unit processors found."_lis << std::endl;
            }
        }

        // Command to directly invoke find unit processors.
        struct TestConfigurationFindUnitProcessorsCommand final : public Command
        {
            TestConfigurationFindUnitProcessorsCommand(std::string_view parent) : Command("config-find-unit-processors", {}, parent) {}

            std::vector<Argument> GetArguments() const override
            {
                return {
                    Argument{ Execution::Args::Type::InstallLocation, Resource::String::LocationArgumentDescription },
                };
            }

            Resource::LocString ShortDescription() const override
            {
                return "Run find unit processors"_lis;
            }

            Resource::LocString LongDescription() const override
            {
                return "Runs find unit processors. Search paths could be provided."_lis;
            }

        protected:
            void ExecuteInternal(Execution::Context& context) const override
            {
                context <<
                    VerifyIsFullPackage <<
                    CreateConfigurationProcessorWithoutFactory <<
                    CreateOrOpenConfigurationSet{ "0.3" } <<
                    EnsureDSCv3Processor <<
                    CreateConfigurationProcessor <<
                    InvokeFindUnitProcessors;
            }
        };
    }

    std::vector<std::unique_ptr<Command>> TestCommand::GetCommands() const
    {
        return InitializeFromMoveOnly<std::vector<std::unique_ptr<Command>>>(
            {
                std::make_unique<TestAppShutdownCommand>(FullName()),
                std::make_unique<TestConfigurationExportCommand>(FullName()),
                std::make_unique<TestConfigurationFindUnitProcessorsCommand>(FullName()),
            });
    }

    void TestCommand::ExecuteInternal(Execution::Context& context) const
    {
        UNREFERENCED_PARAMETER(context);
        Sleep(INFINITE);
    }

    Resource::LocString TestCommand::ShortDescription() const
    {
        return Utility::LocIndString("Waits infinitely"sv);
    }

    Resource::LocString TestCommand::LongDescription() const
    {
        return Utility::LocIndString("Waits infinitely. Use this if you want winget to wait forever while something is going on"sv);
    }

    std::vector<Argument> TestAppShutdownCommand::GetArguments() const
    {
        return {
            Argument::ForType(Execution::Args::Type::Force)
        };
    }

    void TestAppShutdownCommand::ExecuteInternal(Execution::Context& context) const
    {
        HRESULT hr = E_FAIL;

        // Only package context and admin won't create the window message.
        if (!Runtime::IsRunningInPackagedContext() || !Runtime::IsRunningAsAdmin())
        {
            hr = AppShutdownWindowMessage(context);
        }
        else
        {
            hr = WaitForShutdown(context);
        }

        AICLI_TERMINATE_CONTEXT(hr);
    }

    Resource::LocString TestAppShutdownCommand::ShortDescription() const
    {
        return Utility::LocIndString("Test command to verify appshutdown event."sv);
    }

    Resource::LocString TestAppShutdownCommand::LongDescription() const
    {
        return Utility::LocIndString("Test command for appshutdown. Verifies the window was created and waits for the app shutdown event"sv);
    }

}

#endif
