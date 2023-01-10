// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#pragma once
#include "ConfigurationSetChangeData.g.h"

namespace winrt::Microsoft::Management::Configuration::implementation
{
    struct ConfigurationSetChangeData : ConfigurationSetChangeDataT<ConfigurationSetChangeData>
    {
        ConfigurationSetChangeData() = default;

#if !defined(INCLUDE_ONLY_INTERFACE_METHODS)
        void Initialize(ConfigurationSetChangeEventType change);
#endif

        ConfigurationSetChangeEventType Change();
        ConfigurationSetState SetState();
        ConfigurationUnitState UnitState();
        ConfigurationUnitResultInformation ResultInformation();
        ConfigurationUnit Unit();

#if !defined(INCLUDE_ONLY_INTERFACE_METHODS)
    private:
        ConfigurationSetChangeEventType m_change;
        ConfigurationSetState m_setState;
        ConfigurationUnitState m_unitState;
        ConfigurationUnitResultInformation m_resultInformation;
        ConfigurationUnit m_unit;
#endif
    };
}
