// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#pragma once
#include "ConfigurationSetChangeData.g.h"

namespace winrt::Microsoft::Management::Configuration::implementation
{
    struct ConfigurationSetChangeData : ConfigurationSetChangeDataT<ConfigurationSetChangeData>
    {
        using ConfigurationUnit = Configuration::ConfigurationUnit;
        using ConfigurationUnitResultInformation = Configuration::ConfigurationUnitResultInformation;

        ConfigurationSetChangeData() = default;

#if !defined(INCLUDE_ONLY_INTERFACE_METHODS)
        static Configuration::ConfigurationSetChangeData Create(ConfigurationSetState state);
        static Configuration::ConfigurationSetChangeData Create(ConfigurationUnitState state, ConfigurationUnitResultInformation resultInformation, ConfigurationUnit unit);

        void Initialize(ConfigurationSetState state);
        void Initialize(ConfigurationUnitState state, ConfigurationUnitResultInformation resultInformation, ConfigurationUnit unit);
#endif

        ConfigurationSetChangeEventType Change();
        ConfigurationSetState SetState();
        ConfigurationUnitState UnitState();
        ConfigurationUnitResultInformation ResultInformation();
        ConfigurationUnit Unit();

#if !defined(INCLUDE_ONLY_INTERFACE_METHODS)
    private:
        ConfigurationSetChangeEventType m_change = ConfigurationSetChangeEventType::Unknown;
        ConfigurationSetState m_setState = ConfigurationSetState::Unknown;
        ConfigurationUnitState m_unitState = ConfigurationUnitState::Unknown;
        ConfigurationUnitResultInformation m_resultInformation = nullptr;
        ConfigurationUnit m_unit = nullptr;
#endif
    };
}
