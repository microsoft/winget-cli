// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License
#include <Unknwn.h>
#include <CoCreatableMicrosoftManagementDeploymentClass.h>
#include <winrt/Microsoft.Management.Configuration.h>
#include <ComClsids.h>
#include <AppInstallerStrings.h>
#include <winget/ConfigurationSetProcessorHandlers.h>
#include <ConfigurationSetProcessorFactoryRemoting.h>

namespace ConfigurationShim
{
    struct
    DECLSPEC_UUID(WINGET_OUTOFPROC_COM_CLSID_ConfigurationStaticFunctions)
    ConfigurationStaticFunctionsShim : winrt::implements<ConfigurationStaticFunctionsShim, winrt::Microsoft::Management::Configuration::IConfigurationStatics>
    {
        ConfigurationStaticFunctionsShim() = default;

        winrt::Microsoft::Management::Configuration::ConfigurationUnit CreateConfigurationUnit()
        {
            return m_statics.CreateConfigurationUnit();
        }

        winrt::Microsoft::Management::Configuration::ConfigurationSet CreateConfigurationSet()
        {
            return m_statics.CreateConfigurationSet();
        }

        winrt::Microsoft::Management::Configuration::IConfigurationSetProcessorFactory CreateConfigurationSetProcessorFactory(winrt::hstring const& handler)
        {
            std::wstring lowerHandler = AppInstaller::Utility::ToLower(handler);

            if (lowerHandler == AppInstaller::Configuration::PowerShellHandlerIdentifier)
            {
                return AppInstaller::CLI::ConfigurationRemoting::CreateOutOfProcessFactory();
            }

            AICLI_LOG(Config, Error, << "Unknown handler in CreateConfigurationSetProcessorFactory: " << AppInstaller::Utility::ConvertToUTF8(handler));
            THROW_HR(E_NOT_SET);
        }

        winrt::Microsoft::Management::Configuration::ConfigurationProcessor CreateConfigurationProcessor(winrt::Microsoft::Management::Configuration::IConfigurationSetProcessorFactory const& factory)
        {
            return m_statics.CreateConfigurationProcessor(factory);
        }

    private:
        winrt::Microsoft::Management::Configuration::ConfigurationStaticFunctions m_statics;
    };

    // Disable 6388 as it seems to be falsely warning
#pragma warning(push)
#pragma warning(disable : 6388)
    CoCreatableMicrosoftManagementDeploymentClass(ConfigurationStaticFunctionsShim);
#pragma warning(pop)
}
