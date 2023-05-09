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
        void Initialize(hresult resultCode, hstring field = {}, hstring value = {}, uint32_t line = 0, uint32_t column = 0);
#endif

        Configuration::ConfigurationSet Set();
        hresult ResultCode();
        hstring Field();
        hstring Value();
        uint32_t Line();
        uint32_t Column();

#if !defined(INCLUDE_ONLY_INTERFACE_METHODS)
    private:
        Configuration::ConfigurationSet m_set = nullptr;
        hresult m_resultCode;
        hstring m_field;
        hstring m_value;
        uint32_t m_line = 0;
        uint32_t m_column = 0;
#endif
    };
}
