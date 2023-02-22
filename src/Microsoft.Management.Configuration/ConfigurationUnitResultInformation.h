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
#endif

        hresult ResultCode();
        void ResultCode(hresult resultCode);

        hstring Description();
        void Description(hstring value);

#if !defined(INCLUDE_ONLY_INTERFACE_METHODS)
    private:
        hresult m_resultCode;
        hstring m_description;
#endif
    };
}
