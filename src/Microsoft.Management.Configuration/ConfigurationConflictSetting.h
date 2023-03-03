// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#pragma once
#include "ConfigurationConflictSetting.g.h"

namespace winrt::Microsoft::Management::Configuration::implementation
{
    struct ConfigurationConflictSetting : ConfigurationConflictSettingT<ConfigurationConflictSetting>
    {
        ConfigurationConflictSetting() = default;

#if !defined(INCLUDE_ONLY_INTERFACE_METHODS)
        void Initialize(std::wstring_view name, const IInspectable& firstValue, const IInspectable& secondValue);
#endif

        hstring Name();
        IInspectable FirstValue();
        IInspectable SecondValue();

#if !defined(INCLUDE_ONLY_INTERFACE_METHODS)
    private:
        hstring m_name;
        IInspectable m_firstValue = nullptr;
        IInspectable m_secondValue = nullptr;
#endif
    };
}
