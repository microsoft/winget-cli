// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "ApplyConfigurationUnitResult.h"
#include "ApplyConfigurationUnitResult.g.cpp"

namespace winrt::Microsoft::Management::Configuration::implementation
{
    void ApplyConfigurationUnitResult::Initialize(const ConfigurationUnit& unit, const ConfigurationUnitResultInformation& resultInformation)
    {
        m_unit = unit;
        m_resultInformation = resultInformation;
    }

    ConfigurationUnit ApplyConfigurationUnitResult::Unit()
    {
        return m_unit;
    }

    ConfigurationUnitResultInformation ApplyConfigurationUnitResult::ResultInformation()
    {
        return m_resultInformation;
    }
}
