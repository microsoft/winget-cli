// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "GetAllConfigurationUnitSettingsResult.h"
#include "ConfigurationUnitResultInformation.h"

namespace winrt::Microsoft::Management::Configuration::implementation
{
    GetAllConfigurationUnitSettingsResult::GetAllConfigurationUnitSettingsResult() :
        m_resultInformation(*make_self<wil::details::module_count_wrapper<implementation::ConfigurationUnitResultInformation>>())
    {
    }

    void GetAllConfigurationUnitSettingsResult::ResultInformation(const IConfigurationUnitResultInformation& resultInformation)
    {
        m_resultInformation = resultInformation;
    }

    IConfigurationUnitResultInformation GetAllConfigurationUnitSettingsResult::ResultInformation() const
    {
        return m_resultInformation;
    }

    Windows::Foundation::Collections::IVector<Windows::Foundation::Collections::ValueSet> GetAllConfigurationUnitSettingsResult::Settings()
    {
        return m_settings;
    }

    void GetAllConfigurationUnitSettingsResult::Settings(Windows::Foundation::Collections::IVector<Windows::Foundation::Collections::ValueSet>&& value)
    {
        m_settings = std::move(value);
    }
}
