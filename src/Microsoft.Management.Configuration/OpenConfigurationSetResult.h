// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#pragma once
#include "OpenConfigurationSetResult.g.h"
#include "ConfigurationSet.h"

namespace winrt::Microsoft::Management::Configuration::implementation
{
    struct OpenConfigurationSetResult : OpenConfigurationSetResultT<OpenConfigurationSetResult>
    {
        OpenConfigurationSetResult() = default;

#if !defined(INCLUDE_ONLY_INTERFACE_METHODS)
        void Initialize(Configuration::ConfigurationSet configurationSet);
        void Initialize(hresult resultCode, hstring field = {});
#endif

        Configuration::ConfigurationSet Set();
        hresult ResultCode();
        hstring Field();

#if !defined(INCLUDE_ONLY_INTERFACE_METHODS)
    private:
        Configuration::ConfigurationSet m_set = nullptr;
        hresult m_resultCode;
        hstring m_field;
#endif
    };
}
