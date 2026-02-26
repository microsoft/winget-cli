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

    void OpenConfigurationSetResult::Initialize(hresult resultCode, hstring field, hstring value, uint32_t line, uint32_t column)
    {
        m_resultCode = resultCode;
        m_field = field;
        m_value = value;
        m_line = line;
        m_column = column;
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

    uint32_t OpenConfigurationSetResult::Line()
    {
        return m_line;
    }

    uint32_t OpenConfigurationSetResult::Column()
    {
        return m_column;
    }
}
