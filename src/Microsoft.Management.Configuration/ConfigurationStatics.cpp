// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "ConfigurationStatics.h"
#include "ConfigurationStatics.g.cpp"
#include "ConfigurationUnit.h"
#include "ConfigurationSet.h"
#include "ConfigurationProcessor.h"
#include "AppInstallerStrings.h"

namespace winrt::Microsoft::Management::Configuration::implementation
{
    constexpr std::wstring_view c_PowerShellHandlerIdentifier = L"pwsh";

    Configuration::ConfigurationUnit ConfigurationStatics::CreateConfigurationUnit()
    {
        return *make_self<wil::details::module_count_wrapper<implementation::ConfigurationUnit>>();
    }

    Configuration::ConfigurationSet ConfigurationStatics::CreateConfigurationSet()
    {
        return *make_self<wil::details::module_count_wrapper<implementation::ConfigurationSet>>();
    }

    IConfigurationSetProcessorFactory ConfigurationStatics::CreateConfigurationSetProcessorFactory(hstring const& handler)
    {
        std::wstring lowerHandler = AppInstaller::Utility::ToLower(handler);

        if (lowerHandler == c_PowerShellHandlerIdentifier)
        {
            THROW_HR(E_NOTIMPL);
        }

        AICLI_LOG(Config, Error, << "Unknown handler in CreateConfigurationSetProcessorFactory: " << AppInstaller::Utility::ConvertToUTF8(handler));
        THROW_HR(E_NOT_SET);
    }

    Configuration::ConfigurationProcessor ConfigurationStatics::CreateConfigurationProcessor(IConfigurationSetProcessorFactory const& factory)
    {
        auto result = make_self<wil::details::module_count_wrapper<implementation::ConfigurationProcessor>>();
        result->ConfigurationSetProcessorFactory(factory);
        return *result;
    }
}
