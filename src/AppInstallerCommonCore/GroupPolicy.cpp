// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "winget/GroupPolicy.h"

namespace AppInstaller::Settings
{
    namespace
    {
        // TODO: Use final path
        const HKEY PoliciesKey = HKEY_CURRENT_USER; // HKEY_LOCAL_MACHINE;
        std::string_view PoliciesKeyPath = "test\\policy"; // "SOFTWARE\\Policies\\Microsoft\\Windows\\WinGet";

        struct TogglePolicyInternal
        {
            TogglePolicyInternal(TogglePolicy policy, std::string_view regValueName, bool trueIsAllow = false) :
                Policy(policy), RegValueName(regValueName), TrueIsAllow(trueIsAllow) {}

            TogglePolicy Policy;
            std::string_view RegValueName;

            // Whether a true value means to enable or disable a feature.
            bool TrueIsAllow;

            static TogglePolicyInternal GetPolicy(TogglePolicy policy)
            {
                switch (policy)
                {
                case TogglePolicy::DisableWinGet:
                    return TogglePolicyInternal(policy, "DisableWinGet"sv);
                case TogglePolicy::DisableSettingsCommand: return
                    TogglePolicyInternal(policy, "DisableSettings"sv);
                case TogglePolicy::DisableExperimentalFeatures:
                    return TogglePolicyInternal(policy, "DisableExperimentalFeatures"sv);
                case TogglePolicy::DisableLocalManifestFiles:
                    return TogglePolicyInternal(policy, "DisableLocalManifestFiles"sv);
                case TogglePolicy::ExcludeDefaultSources:
                    return TogglePolicyInternal(policy, "ExcludeDefaultSources"sv);
                case TogglePolicy::DisableSourceConfiguration:
                    return TogglePolicyInternal(policy, "DisableSourceConfiguration"sv);
                default:
                    THROW_HR(E_UNEXPECTED);
                }
            }
        };

        template<Registry::Value::Type T>
        std::optional<decltype(std::declval<Registry::Value>().GetValue<T>())> GetRegistryValue(const Registry::Key& key, const std::string_view valueName)
        {
            auto value = key[valueName];
            if (!value.has_value() || value->GetType() != T)
            {
                return std::nullopt;
            }

            return value->GetValue<T>();
        }

        std::optional<bool> RegistryValueIsTrue(const Registry::Key& key, std::string_view valueName)
        {
            auto intValue = GetRegistryValue<Registry::Value::Type::DWord>(key, valueName);
            if (!intValue.has_value())
            {
                return std::nullopt;
            }

            return (bool)*intValue;
        }

        bool IsAllowedInternal(const Registry::Key& key, TogglePolicy policy)
        {
            // Default to allowed if there is no policy for this
            if (policy == TogglePolicy::None)
            {
                return true;
            }

            auto togglePolicy = TogglePolicyInternal::GetPolicy(policy);

            // Policies default to allow if not set
            auto setting = RegistryValueIsTrue(key, togglePolicy.RegValueName);
            if (!setting.has_value())
            {
                return true;
            }

            // Return flag as-is or invert depending on the policy
            return togglePolicy.TrueIsAllow ? *setting : !(*setting);
        }

        template <ValuePolicy P>
        void Validate(
            const Registry::Key& policiesKey,
            GroupPolicy::ValuePoliciesMap& policies)
        {
            auto value = details::ValuePolicyMapping<P>::ReadAndValidate(policiesKey);
            if (value.has_value())
            {
                policies.Add<P>(std::move(*value));
            }
        }

        template <size_t... P>
        void ValidateAllValuePolicies(
            const Registry::Key& policiesKey,
            GroupPolicy::ValuePoliciesMap& policies,
            std::index_sequence<P...>)
        {
            // Use folding to call each policy validate function.
            (FoldHelper{}, ..., Validate<static_cast<ValuePolicy>(P)>(policiesKey, policies));
        }
    }

    namespace details
    {
        std::optional<uint32_t> ValuePolicyMapping<ValuePolicy::SourceAutoUpdateIntervalInMinutes>::ReadAndValidate(const Registry::Key& policiesKey)
        {
            using Mapping = ValuePolicyMapping<ValuePolicy::SourceAutoUpdateIntervalInMinutes>;
            return GetRegistryValue<Mapping::ValueType>(policiesKey , Mapping::ValueName);
        }

        std::optional<std::string> ValuePolicyMapping<ValuePolicy::ProgressBarStyle>::ReadAndValidate(const Registry::Key& policiesKey)
        {
            using Mapping = ValuePolicyMapping<ValuePolicy::ProgressBarStyle>;
            return GetRegistryValue<Mapping::ValueType>(policiesKey, Mapping::ValueName);
        }

        std::optional<std::vector<std::string>> ValuePolicyMapping<ValuePolicy::IncludeSources>::ReadAndValidate(const Registry::Key&)
        {
            // TODO
            return std::nullopt;
        }
    }

    GroupPolicy::GroupPolicy(const Registry::Key& key)
    {
        ValidateAllValuePolicies(key, m_values, std::make_index_sequence<static_cast<size_t>(ValuePolicy::Max)>());

        using Toggle_t = std::underlying_type_t<TogglePolicy>;
        for (Toggle_t i = static_cast<Toggle_t>(TogglePolicy::None); i < static_cast<Toggle_t>(TogglePolicy::Max); ++i)
        {
            auto policy = static_cast<TogglePolicy>(i);
            m_toggles[policy] = IsAllowedInternal(key, policy);
        }
    }

    bool GroupPolicy::IsAllowed(TogglePolicy policy) const
    {
        auto itr = m_toggles.find(policy);
        if (itr == m_toggles.end())
        {
            // Default to allowing if there is no known policy.
            return true;
        }

        return itr->second;
    }

    std::unique_ptr<GroupPolicy> GroupPolicy::s_instance;

    const GroupPolicy& GroupPolicy::Instance()
    {
        if (!s_instance)
        {
            s_instance = std::make_unique<GroupPolicy>(Registry::Key::OpenIfExists(PoliciesKey, PoliciesKeyPath));
        }

        return *s_instance;
    }
}