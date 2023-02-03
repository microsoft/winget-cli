// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "ApplySettingsResult.h"
#include "ApplySettingsResult.g.cpp"
#include "ConfigurationUnitResultInformation.h"

namespace winrt::Microsoft::Management::Configuration::implementation
{
    ApplySettingsResult::ApplySettingsResult() :
        m_resultInformation(*make_self<wil::details::module_count_wrapper<implementation::ConfigurationUnitResultInformation>>())
    {
    }

    Configuration::ConfigurationUnitResultInformation ApplySettingsResult::ResultInformation()
    {
        return m_resultInformation;
    }
}
