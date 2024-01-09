// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "DscConfigurationSetProcessor.h"
#include "DscConfigurationUnitProcessorDetails.h"
#include "DscConfigurationUnitProcessor.h"

namespace winrt::Microsoft::Management::Configuration::Processor::Dsc::implementation
{
    using namespace winrt::Microsoft::Management::Configuration;

    namespace
    {
        constexpr std::wstring_view s_PowerShellGroup = L"DSC/PowerShellGroup";
    }

    DscConfigurationSetProcessor::DscConfigurationSetProcessor(ConfigurationSet const& configurationSet, winrt::weak_ref<DscConfigurationSetProcessorFactory> const& weakFactory)
        : m_configurationSet(configurationSet), m_weakFactory(weakFactory) {}

    IConfigurationUnitProcessorDetails DscConfigurationSetProcessor::GetUnitProcessorDetails(const ConfigurationUnit& unit, ConfigurationUnitDetailFlags detailFlags)
    {
        return winrt::make<DscConfigurationUnitProcessorDetails>(unit, detailFlags);
    }

    IConfigurationUnitProcessor DscConfigurationSetProcessor::CreateUnitProcessor(const ConfigurationUnit& unit)
    {
        // TODO: check schema version.

        if (unit.IsGroup())
        {
            // TODO: see if this is case sensitive in dsc.
            if (unit.Type() == s_PowerShellGroup)
            {
                return winrt::make<DscConfigurationUnitProcessorPowerShellGroup>(unit, m_weakFactory);
            }

            return winrt::make<DscConfigurationUnitProcessorGroup>(unit, m_weakFactory);
        }

        return winrt::make<DscConfigurationUnitProcessor>(unit, m_weakFactory);
    }
}
