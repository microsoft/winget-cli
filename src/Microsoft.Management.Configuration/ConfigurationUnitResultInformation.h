// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#pragma once
#include "winrt/Microsoft.Management.Configuration.h"

namespace winrt::Microsoft::Management::Configuration::implementation
{
    struct ConfigurationUnitResultInformation : winrt::implements<ConfigurationUnitResultInformation, IConfigurationUnitResultInformation>
    {
        ConfigurationUnitResultInformation() = default;

#if !defined(INCLUDE_ONLY_INTERFACE_METHODS)
        void Initialize(const Configuration::IConfigurationUnitResultInformation& other);
        void Initialize(hresult resultCode, std::wstring_view description);
        void Initialize(hresult resultCode, hstring description);
        void Initialize(hresult resultCode, ConfigurationUnitResultSource resultSource);
        void Initialize(hresult resultCode, std::wstring_view description, std::wstring_view details, ConfigurationUnitResultSource resultSource);
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
