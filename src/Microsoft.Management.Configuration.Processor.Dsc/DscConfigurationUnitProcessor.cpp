// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "DscConfigurationSetProcessorFactory.h"
#include "DscConfigurationUnitProcessor.h"
#include "TestSettingsResult.h"
#include "GetSettingsResult.h"
#include "ApplySettingsResult.h"
#include "GetAllSettingsResult.h"

namespace winrt::Microsoft::Management::Configuration::Processor::Dsc::implementation
{
    using namespace winrt::Microsoft::Management::Configuration;

    DscConfigurationUnitProcessor::DscConfigurationUnitProcessor(const ConfigurationUnit& unit, winrt::weak_ref<DscConfigurationSetProcessorFactory> const& weakFactory)
        : m_unit(unit), m_weakFactory(weakFactory)
    {
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

    DscConfigurationUnitProcessorGroup::DscConfigurationUnitProcessorGroup(const ConfigurationUnit& unit, winrt::weak_ref<DscConfigurationSetProcessorFactory> const& weakFactory)
        : m_unit(unit), m_weakFactory(weakFactory)
    {
    }

    ITestSettingsResult DscConfigurationUnitProcessorGroup::TestSettings()
    {
        return winrt::make<TestSettingsResult>(m_unit);
    }

    IGetSettingsResult DscConfigurationUnitProcessorGroup::GetSettings()
    {
        return winrt::make<GetSettingsResult>(m_unit);
    }

    IApplySettingsResult DscConfigurationUnitProcessorGroup::ApplySettings()
    {
        return winrt::make<ApplySettingsResult>(m_unit);
    }

    IGetAllSettingsResult DscConfigurationUnitProcessorGroup::GetAllSettings()
    {
        return winrt::make<GetAllSettingsResult>(m_unit);
    }

    DscConfigurationUnitProcessorPowerShellGroup::DscConfigurationUnitProcessorPowerShellGroup(const ConfigurationUnit& unit, winrt::weak_ref<DscConfigurationSetProcessorFactory> const& weakFactory)
        : m_unit(unit), m_weakFactory(weakFactory)
    {
        // TODO: Ensure install modules.
        //   This needs to go through all the m_unit.Units
        //   Create a new thing that creates a PowerShellGroup with PowerShellGet\MSFT_PSModule (and hope PowerShellGet v3 ships with something similar)
        //   Then one the first Test/Get/Apply/GetAll/Export call Set on that.
        //   Then call the method.
    }

    ITestSettingsResult DscConfigurationUnitProcessorPowerShellGroup::TestSettings()
    {
        return winrt::make<TestSettingsResult>(m_unit);
    }

    IGetSettingsResult DscConfigurationUnitProcessorPowerShellGroup::GetSettings()
    {
        return winrt::make<GetSettingsResult>(m_unit);
    }

    IApplySettingsResult DscConfigurationUnitProcessorPowerShellGroup::ApplySettings()
    {
        return winrt::make<ApplySettingsResult>(m_unit);
    }

    IGetAllSettingsResult DscConfigurationUnitProcessorPowerShellGroup::GetAllSettings()
    {
        return winrt::make<GetAllSettingsResult>(m_unit);
    }
}
