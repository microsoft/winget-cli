// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#pragma once
#include "ConfigurationUnitResultInformation.g.h"

namespace winrt::Microsoft::Management::Configuration::implementation
{
    struct ConfigurationUnitResultInformation : ConfigurationUnitResultInformationT<ConfigurationUnitResultInformation>
    {
        ConfigurationUnitResultInformation() = default;

#if !defined(INCLUDE_ONLY_INTERFACE_METHODS)
        void Initialize(const Configuration::ConfigurationUnitResultInformation& other);
        void Initialize(hresult resultCode, std::wstring_view description);
        void Initialize(hresult resultCode, hstring description);
        void Initialize(hresult resultCode, ConfigurationUnitResultSource resultSource);
#endif

        hresult ResultCode() const;
        void ResultCode(hresult resultCode);

        hstring Description();
        void Description(hstring value);

        hstring Details();
        void Details(hstring value);

        ConfigurationUnitResultSource ResultSource() const;
        void ResultSource(ConfigurationUnitResultSource value);

#if !defined(INCLUDE_ONLY_INTERFACE_METHODS)
    private:
        hresult m_resultCode;
        hstring m_description;
        hstring m_details;
        ConfigurationUnitResultSource m_resultSource = ConfigurationUnitResultSource::None;
#endif
    };
}
