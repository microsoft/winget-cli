// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "ConfigurationSetChangeData.h"
#include "ConfigurationSetChangeData.g.cpp"

namespace winrt::Microsoft::Management::Configuration::implementation
{
    Configuration::ConfigurationSetChangeData ConfigurationSetChangeData::Create(ConfigurationSetState state)
    {
        auto result = make_self<implementation::ConfigurationSetChangeData>();
        result->Initialize(state);
        return *result;
    }

    Configuration::ConfigurationSetChangeData ConfigurationSetChangeData::Create(ConfigurationUnitState state, IConfigurationUnitResultInformation resultInformation, ConfigurationUnit unit)
    {
        auto result = make_self<implementation::ConfigurationSetChangeData>();
        result->Initialize(state, resultInformation, unit);
        return *result;
    }

    void ConfigurationSetChangeData::Initialize(ConfigurationSetState state)
    {
        m_change = ConfigurationSetChangeEventType::SetStateChanged;
        m_setState = state;
    }

    void ConfigurationSetChangeData::Initialize(ConfigurationUnitState state, IConfigurationUnitResultInformation resultInformation, ConfigurationUnit unit)
    {
        m_change = ConfigurationSetChangeEventType::UnitStateChanged;
        m_setState = ConfigurationSetState::InProgress;
        m_unitState = state;
        m_resultInformation = resultInformation;
        m_unit = unit;
    }

    void ConfigurationSetChangeData::Initialize(const IApplyGroupMemberSettingsResult& unitResult)
    {
        m_change = ConfigurationSetChangeEventType::UnitStateChanged;
        m_setState = ConfigurationSetState::InProgress;
        m_unitState = unitResult.State();
        m_resultInformation = unitResult.ResultInformation();
        m_unit = unitResult.Unit();
    }

    ConfigurationSetChangeEventType ConfigurationSetChangeData::Change()
    {
        return m_change;
    }

    ConfigurationSetState ConfigurationSetChangeData::SetState()
    {
        return m_setState;
    }

    ConfigurationUnitState ConfigurationSetChangeData::UnitState()
    {
        return m_unitState;
    }

    IConfigurationUnitResultInformation ConfigurationSetChangeData::ResultInformation()
    {
        return m_resultInformation;
    }

    ConfigurationUnit ConfigurationSetChangeData::Unit()
    {
        return m_unit;
    }

    void ConfigurationSetChangeData::Unit(const ConfigurationUnit& unit)
    {
        m_unit = unit;
    }
}
