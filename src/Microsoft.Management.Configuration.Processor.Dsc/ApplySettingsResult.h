// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#pragma once
#include <winrt/Microsoft.Management.Configuration.h>

namespace winrt::Microsoft::Management::Configuration::Dsc::implementation
{
    struct ApplySettingsResult : winrt::implements<ApplySettingsResult, winrt::Microsoft::Management::Configuration::IApplySettingsResult>
    {
        ApplySettingsResult(const winrt::Microsoft::Management::Configuration::ConfigurationUnit& unit);

        winrt::Microsoft::Management::Configuration::ConfigurationUnit Unit();

        bool RebootRequired();
        void RebootRequired(bool value);

        winrt::Microsoft::Management::Configuration::IConfigurationUnitResultInformation ResultInformation();
        void ResultInformation(winrt::Microsoft::Management::Configuration::IConfigurationUnitResultInformation resultInformation);

    private:
        winrt::Microsoft::Management::Configuration::ConfigurationUnit m_unit;
        bool m_rebootRequired = false;
        winrt::Microsoft::Management::Configuration::IConfigurationUnitResultInformation m_resultInformation;
    };
}
