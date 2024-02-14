// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "ConfigurationStaticFunctions.h"
#include "ConfigurationStaticFunctions.g.cpp"
#include "ConfigurationUnit.h"
#include "ConfigurationSet.h"
#include "ConfigurationProcessor.h"
#include "ConfigurationParameter.h"
#include <AppInstallerStrings.h>
#include <winget/ConfigurationSetProcessorHandlers.h>

namespace winrt::Microsoft::Management::Configuration::implementation
{
    Configuration::ConfigurationUnit ConfigurationStaticFunctions::CreateConfigurationUnit()
    {
        return *make_self<wil::details::module_count_wrapper<implementation::ConfigurationUnit>>();
    }

    Configuration::ConfigurationSet ConfigurationStaticFunctions::CreateConfigurationSet()
    {
        return *make_self<wil::details::module_count_wrapper<implementation::ConfigurationSet>>();
    }

    Windows::Foundation::IAsyncOperation<IConfigurationSetProcessorFactory> ConfigurationStaticFunctions::CreateConfigurationSetProcessorFactoryAsync(hstring const& handler)
    {
        std::wstring lowerHandler = AppInstaller::Utility::ToLower(handler);

        if (lowerHandler == AppInstaller::Configuration::PowerShellHandlerIdentifier)
        {
            THROW_HR(E_NOTIMPL);
        }

        AICLI_LOG(Config, Error, << "Unknown handler in CreateConfigurationSetProcessorFactory: " << AppInstaller::Utility::ConvertToUTF8(handler));
        THROW_HR(E_NOT_SET);
    }

    Configuration::ConfigurationProcessor ConfigurationStaticFunctions::CreateConfigurationProcessor(IConfigurationSetProcessorFactory const& factory)
    {
        auto result = make_self<wil::details::module_count_wrapper<implementation::ConfigurationProcessor>>();
        result->ConfigurationSetProcessorFactory(factory);
        result->SetSupportsSchema03(WI_IsFlagSet(m_state, AppInstaller::WinRT::ConfigurationStaticsInternalsStateFlags::Configuration03));
        return *result;
    }

    Windows::Foundation::IAsyncActionWithProgress<uint32_t> ConfigurationStaticFunctions::EnsureConfigurationAvailableAsync()
    {
        THROW_HR(E_NOTIMPL);
    }

    Configuration::ConfigurationParameter ConfigurationStaticFunctions::CreateConfigurationParameter()
    {
        return *make_self<wil::details::module_count_wrapper<implementation::ConfigurationParameter>>();
    }

    HRESULT STDMETHODCALLTYPE ConfigurationStaticFunctions::SetExperimentalState(UINT32 state)
    {
        m_state = static_cast<AppInstaller::WinRT::ConfigurationStaticsInternalsStateFlags>(state);
        return S_OK;
    }
}
