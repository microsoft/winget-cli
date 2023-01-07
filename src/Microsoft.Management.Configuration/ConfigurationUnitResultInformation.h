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
        void Initialize(hresult resultCode, std::wstring_view description);
#endif

        hresult ResultCode();
        hstring Description();

#if !defined(INCLUDE_ONLY_INTERFACE_METHODS)
    private:
        hresult m_resultCode;
        hstring m_description;
#endif
    };
}
