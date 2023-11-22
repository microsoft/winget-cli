// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "GetAllSettingsResult.h"

namespace winrt::Microsoft::Management::Configuration::Processor::Dsc::implementation
{
    using namespace winrt::Microsoft::Management::Configuration;
    using namespace Windows::Foundation::Collections;

    GetAllSettingsResult::GetAllSettingsResult(const ConfigurationUnit& unit)
        : m_unit(unit) {}

    ConfigurationUnit GetAllSettingsResult::Unit()
    {
        return m_unit;
    }

    IVector<ValueSet> GetAllSettingsResult::Settings()
    {
        return m_settings;
    }

    IConfigurationUnitResultInformation GetAllSettingsResult::ResultInformation()
    {
        return m_resultInformation;
    }

    void GetAllSettingsResult::ResultInformation(IConfigurationUnitResultInformation resultInformation)
    {
        m_resultInformation = resultInformation;
    }
}
