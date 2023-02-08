// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "GetSettingsResult.h"
#include "GetSettingsResult.g.cpp"
#include "ConfigurationUnitResultInformation.h"

namespace winrt::Microsoft::Management::Configuration::implementation
{
    GetSettingsResult::GetSettingsResult() :
        m_resultInformation(*make_self<wil::details::module_count_wrapper<implementation::ConfigurationUnitResultInformation>>())
    {
    }

    Windows::Foundation::Collections::ValueSet GetSettingsResult::Settings()
    {
        return m_settings;
    }

    void GetSettingsResult::Settings(Windows::Foundation::Collections::ValueSet value)
    {
        m_settings = std::move(value);
    }

    Configuration::ConfigurationUnitResultInformation GetSettingsResult::ResultInformation()
    {
        return m_resultInformation;
    }
}
