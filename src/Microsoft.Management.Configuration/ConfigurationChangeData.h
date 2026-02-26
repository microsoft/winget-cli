// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#pragma once
#include "ConfigurationChangeData.g.h"

namespace winrt::Microsoft::Management::Configuration::implementation
{
    struct ConfigurationChangeData : ConfigurationChangeDataT<ConfigurationChangeData>
    {
        ConfigurationChangeData() = default;

#if !defined(INCLUDE_ONLY_INTERFACE_METHODS)
        void Initialize(ConfigurationChangeEventType change, guid instanceIdentifier, ConfigurationSetState state);
#endif

        ConfigurationChangeEventType Change();
        guid InstanceIdentifier();
        ConfigurationSetState State();

#if !defined(INCLUDE_ONLY_INTERFACE_METHODS)
    private:
        ConfigurationChangeEventType m_change{};
        guid m_instanceIdentifier{};
        ConfigurationSetState m_state{};
#endif
    };
}
