// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "GetConfigurationUnitSettingsResult.h"
#include "GetConfigurationUnitSettingsResult.g.cpp"
#include "ConfigurationUnitResultInformation.h"

namespace winrt::Microsoft::Management::Configuration::implementation
{
    GetConfigurationUnitSettingsResult::GetConfigurationUnitSettingsResult() :
        m_resultInformation(*make_self<wil::details::module_count_wrapper<implementation::ConfigurationUnitResultInformation>>())
    {
    }

    void GetConfigurationUnitSettingsResult::ResultInformation(const IConfigurationUnitResultInformation& resultInformation)
    {
        m_resultInformation = resultInformation;
    }

    IConfigurationUnitResultInformation GetConfigurationUnitSettingsResult::ResultInformation() const
    {
        return m_resultInformation;
    }

    Windows::Foundation::Collections::ValueSet GetConfigurationUnitSettingsResult::Settings()
    {
        return m_settings;
    }

    void GetConfigurationUnitSettingsResult::Settings(Windows::Foundation::Collections::ValueSet&& value)
    {
        m_settings = std::move(value);
    }
}
