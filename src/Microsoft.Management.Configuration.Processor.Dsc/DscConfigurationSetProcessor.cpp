// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "DscConfigurationSetProcessor.h"
#include "DscConfigurationUnitProcessorDetails.h"
#include "DscConfigurationUnitProcessor.h"

namespace winrt::Microsoft::Management::Configuration::Processor::Dsc::implementation
{
    using namespace winrt::Microsoft::Management::Configuration;

    DscConfigurationSetProcessor::DscConfigurationSetProcessor(ConfigurationSet const& configurationSet, winrt::weak_ref<DscConfigurationSetProcessorFactory> const& weakFactory)
        : m_configurationSet(configurationSet), m_weakFactory(weakFactory) {}

    IConfigurationUnitProcessorDetails DscConfigurationSetProcessor::GetUnitProcessorDetails(const ConfigurationUnit& unit, ConfigurationUnitDetailFlags detailFlags)
    {
        return winrt::make<DscConfigurationUnitProcessorDetails>(unit, detailFlags);
    }

    IConfigurationUnitProcessor DscConfigurationSetProcessor::CreateUnitProcessor(const ConfigurationUnit& unit)
    {
        return winrt::make<DscConfigurationUnitProcessor>(unit, m_weakFactory);
    }
}
