// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "ApplySettingsResultInstance.h"
#include "ApplySettingsResultInstance.g.cpp"
#include "ConfigurationUnitResultInformation.h"

namespace winrt::Microsoft::Management::Configuration::implementation
{
    ApplySettingsResultInstance::ApplySettingsResultInstance() :
        m_resultInformation(*make_self<wil::details::module_count_wrapper<implementation::ConfigurationUnitResultInformation>>())
    {
    }

    bool ApplySettingsResultInstance::RebootRequired() const
    {
        return m_rebootRequired;
    }

    void ApplySettingsResultInstance::RebootRequired(bool value)
    {
        m_rebootRequired = value;
    }

    Configuration::IConfigurationUnitResultInformation ApplySettingsResultInstance::ResultInformation()
    {
        return m_resultInformation;
    }
}
