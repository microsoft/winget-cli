// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "GetAllConfigurationUnitsResult.h"
#include "ConfigurationUnitResultInformation.h"

namespace winrt::Microsoft::Management::Configuration::implementation
{
    GetAllConfigurationUnitsResult::GetAllConfigurationUnitsResult() :
        m_resultInformation(*make_self<wil::details::module_count_wrapper<implementation::ConfigurationUnitResultInformation>>())
    {
    }

    void GetAllConfigurationUnitsResult::ResultInformation(const IConfigurationUnitResultInformation& resultInformation)
    {
        m_resultInformation = resultInformation;
    }

    IConfigurationUnitResultInformation GetAllConfigurationUnitsResult::ResultInformation() const
    {
        return m_resultInformation;
    }

    Windows::Foundation::Collections::IVector<Configuration::ConfigurationUnit> GetAllConfigurationUnitsResult::Units()
    {
        return m_units;
    }

    void GetAllConfigurationUnitsResult::Units(Windows::Foundation::Collections::IVector<Configuration::ConfigurationUnit>&& value)
    {
        m_units = std::move(value);
    }
}
