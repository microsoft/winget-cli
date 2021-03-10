// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#pragma once

#include "winget/Registry.h"
#include <string_view>

namespace AppInstaller::GroupPolicy
{
    // A registry-based policy.
    struct PolicyBase
    {
        PolicyBase(std::wstring_view regName, std::optional<Registry::Key> regKey = std::nullopt);

        virtual ~PolicyBase() = default;

        const Registry::Key& RegKey() const { return m_regKey; }
        const std::wstring& RegName() const { return m_regName; }

    private:
        // Registry key containing the policy.
        Registry::Key m_regKey;

        // Name of the registry value or key backing the policy.
        std::wstring m_regName;
    };

    // A policy that sets a single value for some setting.
    // The value of the policy is a value in the registry key.
    template <Registry::Value::Type T>
    struct ValuePolicy : public PolicyBase
    {
        ValuePolicy(std::wstring_view regName, std::optional<Registry::Key> regKey = std::nullopt) : PolicyBase(regName, regKey) {}

        // Returns the value in the registry if it exists and has the correct type
        std::optional<decltype(std::declval<Registry::Value>().GetValue<T>())> GetValue() const
        {
            auto value = RegKey()[RegName()];
            if (!value.has_value() || value->GetType() != T)
            {
                return std::nullopt;
            }

            return value->GetValue<T>();
        }
    };

    enum class PolicyD
    {
        SourceAutoUpdateIntervalInMinutes, // TODO
    };

    enum class PolicyS
    {
        ProgressBarStyle, // TODO
    };

    // A policy that acts as a toggle, e.g. to enable or disable a feature.
    // They are backed by a DWORD value with values zero and non-zero.
    struct TogglePolicy : public ValuePolicy<Registry::Value::Type::DWord>
    {
        enum class Policy
        {
            None = 0,
            DisableWinGet,
            DisableSettingsCommand,
            DisableExperimentalFeatures,
            DisableLocalManifestFiles,
            ExcludeDefaultSources,
            DisableSourceConfiguration,
        };

        TogglePolicy(std::wstring_view regName, std::optional<Registry::Key> regKey = std::nullopt, bool trueIsEnable = false) : ValuePolicy(regName, regKey), m_trueIsEnable(trueIsEnable) {}

        std::optional<bool> IsTrue() const;
        bool TrueIsEnable() const { return m_trueIsEnable; }

    private:
        // Whether a true value means to enable or disable a feature.
        bool m_trueIsEnable;
    };

    // A policy that sets a list as a value for some setting.
    // The value of the policy P is the list of sub-keys of the sub-key P of the
    // root key for policies.
    // [Root policies key]
    //   [P sub-key]
    //     [1st key]
    //     [2nd key]
    //     ...
    struct ListPolicy : public PolicyBase
    {
        enum class Policy
        {
            IncludeSources, // TODO
        };

        ListPolicy(std::wstring_view regName, std::optional<Registry::Key> regKey = std::nullopt) : PolicyBase(regName, regKey) {}
    };

    ValuePolicy<Registry::Value::Type::DWord> GetPolicy(PolicyD policy, std::optional<Registry::Key> regKey = std::nullopt);
    ValuePolicy<Registry::Value::Type::String> GetPolicy(PolicyS policy, std::optional<Registry::Key> regKey = std::nullopt);
    TogglePolicy GetPolicy(TogglePolicy::Policy policy, std::optional<Registry::Key> regKey = std::nullopt);
    ListPolicy GetPolicy(ListPolicy::Policy policy, std::optional<Registry::Key> regKey = std::nullopt);

    bool IsAllowed(TogglePolicy::Policy policy);
}