// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#pragma once
#include <winrt/Microsoft.Management.Configuration.h>

namespace winrt::Microsoft::Management::Configuration::Processor::Dsc::implementation
{
    struct DscConfigurationUnitResultInformation : winrt::implements<DscConfigurationUnitResultInformation, winrt::Microsoft::Management::Configuration::IConfigurationUnitResultInformation>
    {
        winrt::hresult ResultCode() const;
        void ResultCode(winrt::hresult resultCode);

        winrt::hstring Description() const;
        void Description(const winrt::hstring& description);

        winrt::hstring Details() const;
        void Details(const winrt::hstring& details);

        winrt::Microsoft::Management::Configuration::ConfigurationUnitResultSource ResultSource() const;
        void ResultSource(winrt::Microsoft::Management::Configuration::ConfigurationUnitResultSource resultSource);

    private:
        winrt::hresult m_resultCode;
        winrt::hstring m_description;
        winrt::hstring m_details;
        winrt::Microsoft::Management::Configuration::ConfigurationUnitResultSource m_resultSource = winrt::Microsoft::Management::Configuration::ConfigurationUnitResultSource::None;
    };
}
