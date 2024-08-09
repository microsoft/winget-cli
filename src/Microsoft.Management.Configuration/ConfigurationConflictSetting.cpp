// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "ConfigurationConflictSetting.h"
#include "ConfigurationConflictSetting.g.cpp"

namespace winrt::Microsoft::Management::Configuration::implementation
{
    void ConfigurationConflictSetting::Initialize(std::wstring_view name, const IInspectable& firstValue, const IInspectable& secondValue)
    {
        m_name = name;
        m_firstValue = firstValue;
        m_secondValue = secondValue;
    }

    hstring ConfigurationConflictSetting::Name()
    {
        return m_name;
    }

    ConfigurationConflictSetting::IInspectable ConfigurationConflictSetting::FirstValue()
    {
        return m_firstValue;
    }

    ConfigurationConflictSetting::IInspectable ConfigurationConflictSetting::SecondValue()
    {
        return m_secondValue;
    }
}
