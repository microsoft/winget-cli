// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "ConfigurationChangeData.h"
#include "ConfigurationChangeData.g.cpp"

namespace winrt::Microsoft::Management::Configuration::implementation
{
    void ConfigurationChangeData::Initialize(ConfigurationChangeEventType change, guid instanceIdentifier, ConfigurationSetState state)
    {
        m_change = change;
        m_instanceIdentifier = instanceIdentifier;
        m_state = state;
    }

    ConfigurationChangeEventType ConfigurationChangeData::Change()
    {
        return m_change;
    }

    guid ConfigurationChangeData::InstanceIdentifier()
    {
        return m_instanceIdentifier;
    }

    ConfigurationSetState ConfigurationChangeData::State()
    {
        return m_state;
    }
}
