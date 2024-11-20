// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "ConfigurationCommon.h"
#include "ExecutionArgs.h"
#include <AppInstallerRuntime.h>
#include "Command.h"
#include <winrt/Microsoft.Management.Configuration.SetProcessorFactory.h>
#include <filesystem>

namespace AppInstaller::CLI
{
    using namespace winrt::Microsoft::Management::Configuration;

    namespace
    {
        constexpr std::string_view s_ModulePath_Default = "default";
        constexpr std::string_view s_ModulePath_CurrentUser = "currentuser";
        constexpr std::string_view s_ModulePath_AllUsers = "allusers";

        struct ModulePathInfo
        {
            SetProcessorFactory::PwshConfigurationProcessorLocation location;
            std::optional<std::string> customLocation;
        };

        ModulePathInfo GetModulePathInfo(Execution::Args& execArgs)
        {
            if (execArgs.Contains(Execution::Args::Type::ConfigurationModulePath))
            {
                auto modulePath = execArgs.GetArg(Execution::Args::Type::ConfigurationModulePath);

                if (Utility::CaseInsensitiveEquals(modulePath, s_ModulePath_Default))
                {
                    return { SetProcessorFactory::PwshConfigurationProcessorLocation::Default, {} };
                }
                else if (Utility::CaseInsensitiveEquals(modulePath, s_ModulePath_CurrentUser))
                {
                    return { SetProcessorFactory::PwshConfigurationProcessorLocation::CurrentUser, {} };
                }
                else if (Utility::CaseInsensitiveEquals(modulePath, s_ModulePath_AllUsers))
                {
                    return { SetProcessorFactory::PwshConfigurationProcessorLocation::AllUsers, {} };
                }
                else
                {
                    return { SetProcessorFactory::PwshConfigurationProcessorLocation::Custom, std::string(execArgs.GetArg(Execution::Args::Type::ConfigurationModulePath)) };
                }
            }

            std::filesystem::path defaultModuleRoot = Settings::User().Get<Settings::Setting::ConfigureDefaultModuleRoot>();

            if (!defaultModuleRoot.empty())
            {
                return { SetProcessorFactory::PwshConfigurationProcessorLocation::Custom, defaultModuleRoot.u8string() };
            }

            return { SetProcessorFactory::PwshConfigurationProcessorLocation::WinGetModulePath, {} };
        }
    }

    namespace Configuration
    {
        void ValidateCommonArguments(Execution::Args& execArgs, bool requireConfigurationSetChoice)
        {
            auto modulePath = GetModulePathInfo(execArgs);

            if (modulePath.location == SetProcessorFactory::PwshConfigurationProcessorLocation::AllUsers && !Runtime::IsRunningAsAdmin())
            {
                throw CommandException(Resource::String::ConfigurationAllUsersElevated);
            }

            if (modulePath.location == SetProcessorFactory::PwshConfigurationProcessorLocation::Custom)
            {
                auto path = std::filesystem::path{ modulePath.customLocation.value() };
                if (!path.is_absolute())
                {
                    throw CommandException(Resource::String::ConfigurationModulePathArgError);
                }
            }

            if (requireConfigurationSetChoice &&
                !WI_IsFlagSet(Argument::GetCategoriesPresent(execArgs), ArgTypeCategory::ConfigurationSetChoice))
            {
                throw CommandException(Resource::String::RequiredArgError("file"_liv));
            }
        }

        void SetModulePath(Execution::Context& context, IConfigurationSetProcessorFactory const& factory)
        {
            auto pwshFactory = factory.as<SetProcessorFactory::IPwshConfigurationSetProcessorFactoryProperties>();
            auto modulePath = GetModulePathInfo(context.Args);

            if (modulePath.location == SetProcessorFactory::PwshConfigurationProcessorLocation::Custom)
            {
                pwshFactory.CustomLocation(winrt::to_hstring(modulePath.customLocation.value()));
            }

            pwshFactory.Location(modulePath.location);
        }
    }
}
