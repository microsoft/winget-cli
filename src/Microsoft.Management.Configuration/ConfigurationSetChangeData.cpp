// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "ConfigurationSetChangeData.h"
#include "ConfigurationSetChangeData.g.cpp"

namespace winrt::Microsoft::Management::Configuration::implementation
{
    void ConfigurationSetChangeData::Initialize(ConfigurationSetChangeEventType change)
    {
        m_change = change;
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

    ConfigurationUnitResultInformation ConfigurationSetChangeData::ResultInformation()
    {
        return m_resultInformation;
    }

    ConfigurationUnit ConfigurationSetChangeData::Unit()
    {
        return m_unit;
    }
}
