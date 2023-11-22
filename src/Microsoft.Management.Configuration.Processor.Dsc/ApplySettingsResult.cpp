// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "ApplySettingsResult.h"

namespace winrt::Microsoft::Management::Configuration::Processor::Dsc::implementation
{
    using namespace winrt::Microsoft::Management::Configuration;

    ApplySettingsResult::ApplySettingsResult(const ConfigurationUnit& unit) :
        m_unit(unit) {}

    ConfigurationUnit ApplySettingsResult::Unit()
    {
        return m_unit;
    }

    bool ApplySettingsResult::RebootRequired()
    {
        return m_rebootRequired;
    }
    
    void ApplySettingsResult::RebootRequired(bool value)
    {
        m_rebootRequired = value;
    }

    IConfigurationUnitResultInformation ApplySettingsResult::ResultInformation()
    {
        return m_resultInformation;
    }
    
    void ApplySettingsResult::ResultInformation(IConfigurationUnitResultInformation value)
    {
        m_resultInformation = value;
    }
}
