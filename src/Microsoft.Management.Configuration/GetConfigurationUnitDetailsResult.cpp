// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "GetConfigurationUnitDetailsResult.h"
#include "GetConfigurationUnitDetailsResult.g.cpp"

namespace winrt::Microsoft::Management::Configuration::implementation
{
    void GetConfigurationUnitDetailsResult::Unit(ConfigurationUnit value)
    {
        m_unit = std::move(value);
    }

    void GetConfigurationUnitDetailsResult::ResultInformation(ConfigurationUnitResultInformation value)
    {
        m_resultInformation = std::move(value);
    }

    ConfigurationUnit GetConfigurationUnitDetailsResult::Unit()
    {
        return m_unit;
    }

    ConfigurationUnitResultInformation GetConfigurationUnitDetailsResult::ResultInformation()
    {
        return m_resultInformation;
    }
}
