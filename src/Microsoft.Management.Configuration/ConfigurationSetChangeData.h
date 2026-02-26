// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#pragma once
#include "ConfigurationSetChangeData.g.h"
#include "ConfigurationUnitResultInformation.h"
#include <winget/ModuleCountBase.h>

namespace winrt::Microsoft::Management::Configuration::implementation
{
    struct ConfigurationSetChangeData : ConfigurationSetChangeDataT<ConfigurationSetChangeData>, AppInstaller::WinRT::ModuleCountBase
    {
        using ConfigurationUnit = Configuration::ConfigurationUnit;

        ConfigurationSetChangeData() = default;

#if !defined(INCLUDE_ONLY_INTERFACE_METHODS)
        static Configuration::ConfigurationSetChangeData Create(ConfigurationSetState state);
        static Configuration::ConfigurationSetChangeData Create(ConfigurationUnitState state, IConfigurationUnitResultInformation resultInformation, ConfigurationUnit unit);

        void Initialize(ConfigurationSetState state);
        void Initialize(ConfigurationUnitState state, IConfigurationUnitResultInformation resultInformation, ConfigurationUnit unit);
        void Initialize(const IApplyGroupMemberSettingsResult& unitResult);

        void Unit(const ConfigurationUnit& unit);
#endif

        ConfigurationSetChangeEventType Change();
        ConfigurationSetState SetState();
        ConfigurationUnitState UnitState();
        IConfigurationUnitResultInformation ResultInformation();
        ConfigurationUnit Unit();

#if !defined(INCLUDE_ONLY_INTERFACE_METHODS)
    private:
        ConfigurationSetChangeEventType m_change = ConfigurationSetChangeEventType::Unknown;
        ConfigurationSetState m_setState = ConfigurationSetState::Unknown;
        ConfigurationUnitState m_unitState = ConfigurationUnitState::Unknown;
        IConfigurationUnitResultInformation m_resultInformation;
        ConfigurationUnit m_unit = nullptr;
#endif
    };
}
