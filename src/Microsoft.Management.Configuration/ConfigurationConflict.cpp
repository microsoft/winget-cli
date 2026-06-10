// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "ConfigurationConflict.h"
#include "ConfigurationConflict.g.cpp"

namespace winrt::Microsoft::Management::Configuration::implementation
{
    void ConfigurationConflict::Initialize(ConfigurationConflictType conflict)
    {
        m_conflict = conflict;
    }

    ConfigurationConflictType ConfigurationConflict::Conflict()
    {
        return m_conflict;
    }

    ConfigurationSet ConfigurationConflict::FirstSet()
    {
        return m_firstSet;
    }

    ConfigurationSet ConfigurationConflict::SecondSet()
    {
        return m_secondSet;
    }

    ConfigurationUnit ConfigurationConflict::FirstUnit()
    {
        return m_firstUnit;
    }

    ConfigurationUnit ConfigurationConflict::SecondUnit()
    {
        return m_secondUnit;
    }

    Windows::Foundation::Collections::IVectorView<ConfigurationConflictSetting> ConfigurationConflict::Settings()
    {
        if (m_settings)
        {
            return m_settings.GetView();
        }
        else
        {
            return nullptr;
        }
    }
}
