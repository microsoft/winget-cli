// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "winget/GroupPolicy.h"

namespace AppInstaller::GroupPolicy
{
    namespace
    {
        // TODO: Use final path
        const HKEY PoliciesKey = HKEY_CURRENT_USER; // HKEY_LOCAL_MACHINE;
        std::wstring PoliciesKeyPath = L"test\\policy"; // L"SOFTWARE\\Policies\\Microsoft\\Windows\\WinGet";
    }

    PolicyBase::PolicyBase(std::wstring_view regName, std::optional<Registry::Key> regKey)
        : m_regKey(regKey.value_or(Registry::Key::OpenIfExists(PoliciesKey, PoliciesKeyPath))), m_regName(regName) {}

    std::optional<bool> TogglePolicy::IsTrue() const
    {
        auto intValue = GetValue();
        if (!intValue.has_value())
        {
            return std::nullopt;
        }

        return *intValue;
    }

    ValuePolicy<Registry::Value::Type::DWord> GetPolicy(PolicyD policy, std::optional<Registry::Key> regKey)
    {
        // TODO: Use final reg value names
        switch (policy)
        {
        case PolicyD::SourceAutoUpdateIntervalInMinutes:
            return ValuePolicy<Registry::Value::Type::DWord>(L"SourceAutoUpdateIntervalInMinutes", regKey);
        default:
            THROW_HR(E_UNEXPECTED);
        }
    }

    ValuePolicy<Registry::Value::Type::String> GetPolicy(PolicyS policy, std::optional<Registry::Key> regKey)
    {
        // TODO: Use final reg value names
        switch (policy)
        {
        case PolicyS::ProgressBarStyle:
            return ValuePolicy<Registry::Value::Type::String>(L"ProgressBarStyle", regKey);
        default:
            THROW_HR(E_UNEXPECTED);
        }
    }

    TogglePolicy GetPolicy(TogglePolicy::Policy policy, std::optional<Registry::Key> regKey)
    {
        // TODO: Use final reg value names
        switch (policy)
        {
        case TogglePolicy::Policy::DisableWinGet:
            return TogglePolicy(L"DisableWinGet", regKey);
        case TogglePolicy::Policy::DisableSettingsCommand:
            return TogglePolicy(L"DisableSettings", regKey);
        case TogglePolicy::Policy::DisableExperimentalFeatures:
            return TogglePolicy(L"DisableExperimentalFeatures", regKey);
        case TogglePolicy::Policy::DisableLocalManifestFiles:
            return TogglePolicy(L"DisableLocalManifestFiles", regKey);
        case TogglePolicy::Policy::ExcludeDefaultSources:
            return TogglePolicy(L"ExcludeDefaultSources", regKey);
        case TogglePolicy::Policy::DisableSourceConfiguration:
            return TogglePolicy(L"ExcludeDefaultSources", regKey);
        default:
            THROW_HR(E_UNEXPECTED);
        }
    }

    ListPolicy GetPolicy(ListPolicy::Policy, std::optional<Registry::Key>)
    {
        THROW_HR(E_UNEXPECTED);
    }

    bool IsAllowed(TogglePolicy::Policy policy)
    {
        // Default to allowed if there is no policy for this
        if (policy == TogglePolicy::Policy::None)
        {
            return true;
        }

        auto togglePolicy = GetPolicy(policy);

        // Policies default to allow if not set
        auto setting = togglePolicy.GetValue();
        if (!setting.has_value())
        {
            return true;
        }

        // Return flag as-is or invert depending on the policy
        if (togglePolicy.TrueIsEnable())
        {
            return *setting;
        }
        else
        {
            return !(*setting);
        }
    }
}