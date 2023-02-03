// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "GetConfigurationUnitSettingsResult.h"
#include "GetConfigurationUnitSettingsResult.g.cpp"

namespace winrt::Microsoft::Management::Configuration::implementation
{
    void GetConfigurationUnitSettingsResult::ResultInformation(const ConfigurationUnitResultInformation& resultInformation)
    {
        m_resultInformation = resultInformation;
    }

    ConfigurationUnitResultInformation GetConfigurationUnitSettingsResult::ResultInformation()
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
