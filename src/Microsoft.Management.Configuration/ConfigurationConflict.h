// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#pragma once
#include "ConfigurationConflict.g.h"
#include <winrt/Windows.Foundation.Collections.h>

namespace winrt::Microsoft::Management::Configuration::implementation
{
    struct ConfigurationConflict : ConfigurationConflictT<ConfigurationConflict>
    {
        ConfigurationConflict() = default;

#if !defined(INCLUDE_ONLY_INTERFACE_METHODS)
        void Initialize(ConfigurationConflictType conflict);
#endif

        ConfigurationConflictType Conflict();
        ConfigurationSet FirstSet();
        ConfigurationSet SecondSet();
        ConfigurationUnit FirstUnit();
        ConfigurationUnit SecondUnit();
        Windows::Foundation::Collections::IVectorView<ConfigurationConflictSetting> Settings();

#if !defined(INCLUDE_ONLY_INTERFACE_METHODS)
    private:
        ConfigurationConflictType m_conflict{};
        ConfigurationSet m_firstSet = nullptr;
        ConfigurationSet m_secondSet = nullptr;
        ConfigurationUnit m_firstUnit = nullptr;
        ConfigurationUnit m_secondUnit = nullptr;
        Windows::Foundation::Collections::IVector<ConfigurationConflictSetting> m_settings = nullptr;
#endif
    };
}
