// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "GetSettingsResultInstance.h"
#include "GetSettingsResultInstance.g.cpp"
#include "ConfigurationUnitResultInformation.h"

namespace winrt::Microsoft::Management::Configuration::implementation
{
    GetSettingsResultInstance::GetSettingsResultInstance() :
        m_resultInformation(*make_self<wil::details::module_count_wrapper<implementation::ConfigurationUnitResultInformation>>())
    {
    }

    Windows::Foundation::Collections::ValueSet GetSettingsResultInstance::Settings()
    {
        return m_settings;
    }

    void GetSettingsResultInstance::Settings(Windows::Foundation::Collections::ValueSet value)
    {
        m_settings = std::move(value);
    }

    Configuration::IConfigurationUnitResultInformation GetSettingsResultInstance::ResultInformation()
    {
        return m_resultInformation;
    }
}
