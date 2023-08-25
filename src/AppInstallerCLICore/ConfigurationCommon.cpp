// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "ConfigurationCommon.h"
#include "ExecutionArgs.h"
#include <AppInstallerRuntime.h>
#include "Command.h"
#include <winrt/Microsoft.Management.Configuration.SetProcessorFactory.h>

namespace AppInstaller::CLI
{
    using namespace winrt::Microsoft::Management::Configuration;

    namespace
    {
        constexpr std::string_view s_ModulePath_Default = "default";
        constexpr std::string_view s_ModulePath_CurrentUser = "currentuser";
        constexpr std::string_view s_ModulePath_AllUsers = "allusers";
    }

    namespace Configuration
    {
        void ValidateCommonArguments(Execution::Args& execArgs)
        {
            if (execArgs.Contains(Execution::Args::Type::ConfigurationModulePath))
            {
                auto modulePath = execArgs.GetArg(Execution::Args::Type::ConfigurationModulePath);
                if (Utility::CaseInsensitiveEquals(modulePath, s_ModulePath_AllUsers) && !Runtime::IsRunningAsAdmin())
                {
                    throw CommandException(Resource::String::ConfigurationAllUsersElevated);
                }
            }
        }

        void SetModulePath(Execution::Context& context, IConfigurationSetProcessorFactory const& factory)
        {
            auto pwshFactory = factory.as<SetProcessorFactory::IPwshConfigurationSetProcessorFactoryProperties>();
            auto location = SetProcessorFactory::PwshConfigurationProcessorLocation::WinGetModulePath;

            // TODO: add a setting that says the default custom location.
            if (context.Args.Contains(Execution::Args::Type::ConfigurationModulePath))
            {
                auto modulePath = context.Args.GetArg(Execution::Args::Type::ConfigurationModulePath);

                if (Utility::CaseInsensitiveEquals(modulePath, s_ModulePath_Default))
                {
                    location = SetProcessorFactory::PwshConfigurationProcessorLocation::Default;
                }
                else if (Utility::CaseInsensitiveEquals(modulePath, s_ModulePath_CurrentUser))
                {
                    location = SetProcessorFactory::PwshConfigurationProcessorLocation::CurrentUser;
                }
                else if (Utility::CaseInsensitiveEquals(modulePath, s_ModulePath_AllUsers))
                {
                    location = SetProcessorFactory::PwshConfigurationProcessorLocation::AllUsers;
                }
                else
                {
                    location = SetProcessorFactory::PwshConfigurationProcessorLocation::Custom;
                    pwshFactory.CustomLocation(winrt::to_hstring(context.Args.GetArg(Execution::Args::Type::ConfigurationModulePath)));
                }
            }

            pwshFactory.Location(location);
        }
    }
}
