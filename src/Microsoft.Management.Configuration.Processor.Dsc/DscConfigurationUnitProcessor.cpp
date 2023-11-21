// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "DscConfigurationUnitProcessor.h"
#include "TestSettingsResult.h"
#include "GetSettingsResult.h"
#include "ApplySettingsResult.h"
#include "GetAllSettingsResult.h"

namespace winrt::Microsoft::Management::Configuration::Dsc::implementation
{
    using namespace winrt::Microsoft::Management::Configuration;

    DscConfigurationUnitProcessor::DscConfigurationUnitProcessor(const ConfigurationUnit& unit) :
        m_unit(unit) {}

    ConfigurationUnit DscConfigurationUnitProcessor::Unit()
    {
        return  m_unit;
    }

    ITestSettingsResult DscConfigurationUnitProcessor::TestSettings()
    {
        return winrt::make<TestSettingsResult>(m_unit);
    }

    IGetSettingsResult DscConfigurationUnitProcessor::GetSettings()
    {
        return winrt::make<GetSettingsResult>(m_unit);
    }

    IApplySettingsResult DscConfigurationUnitProcessor::ApplySettings()
    {
        return winrt::make<ApplySettingsResult>(m_unit);
    }

    IGetAllSettingsResult DscConfigurationUnitProcessor::GetAllSettings()
    {
        return winrt::make<GetAllSettingsResult>(m_unit);
    }
}
