// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "GetSettingsResult.h"

namespace winrt::Microsoft::Management::Configuration::Dsc::implementation
{
    using namespace winrt::Microsoft::Management::Configuration;
    using namespace Windows::Foundation::Collections;

    GetSettingsResult::GetSettingsResult(const winrt::Microsoft::Management::Configuration::ConfigurationUnit& unit)
        : m_unit(unit) {}

    ConfigurationUnit GetSettingsResult::Unit()
    {
        return m_unit;
    }

    ValueSet GetSettingsResult::Settings()
    {
        return m_settings;
    }

    void GetSettingsResult::Settings(Windows::Foundation::Collections::ValueSet settings)
    {
        m_settings = settings;
    }

    IConfigurationUnitResultInformation GetSettingsResult::ResultInformation()
    {
        return m_resultInformation;
    }

    void GetSettingsResult::ResultInformation(IConfigurationUnitResultInformation resultInformation)
    {
        m_resultInformation = resultInformation;
    }
}
