// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "OpenConfigurationSetResult.h"
#include "OpenConfigurationSetResult.g.cpp"

namespace winrt::Microsoft::Management::Configuration::implementation
{
    void OpenConfigurationSetResult::Initialize(Configuration::ConfigurationSet configurationSet)
    {
        m_set = std::move(configurationSet);
    }

    void OpenConfigurationSetResult::Initialize(hresult resultCode, hstring field, hstring value)
    {
        m_resultCode = resultCode;
        m_field = field;
        m_value = value;
    }

    Configuration::ConfigurationSet OpenConfigurationSetResult::Set()
    {
        return m_set;
    }

    hresult OpenConfigurationSetResult::ResultCode()
    {
        return m_resultCode;
    }

    hstring OpenConfigurationSetResult::Field()
    {
        return m_field;
    }

    hstring OpenConfigurationSetResult::Value()
    {
        return m_value;
    }
}
