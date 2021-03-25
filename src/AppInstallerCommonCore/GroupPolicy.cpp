// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "winget/GroupPolicy.h"
#include "AppInstallerLogging.h"

namespace AppInstaller::Settings
{
    namespace
    {
        GroupPolicy& InstanceInternal(std::optional<GroupPolicy*> overridePolicy = {})
        {
            // TODO: Use final location
            constexpr std::string_view PoliciesKeyPath = "test\\policy"; // "SOFTWARE\\Policies\\Microsoft\\Windows\\WinGet";
            static GroupPolicy s_groupPolicy{ Registry::Key::OpenIfExists(HKEY_CURRENT_USER /* HKEY_LOCAL_MACHINE */, PoliciesKeyPath) };
            static GroupPolicy* s_override = nullptr;

            if (overridePolicy.has_value())
            {
                s_override = overridePolicy.value();
            }

            return (s_override ? *s_override : s_groupPolicy);
        }

        struct TogglePolicyInternal
        {
            TogglePolicyInternal(TogglePolicy policy, std::string_view regValueName, bool defaultIsEnabled = true) :
                Policy(policy), RegValueName(regValueName), DefaultIsEnabled(defaultIsEnabled) {}

            TogglePolicy Policy;
            std::string_view RegValueName;
            bool DefaultIsEnabled;

            static TogglePolicyInternal GetPolicy(TogglePolicy policy)
            {
                switch (policy)
                {
                case TogglePolicy::WinGet:
                    return TogglePolicyInternal(policy, "EnableWindowsPackageManager"sv);
                case TogglePolicy::SettingsCommand: return
                    TogglePolicyInternal(policy, "EnableSettingsCommand"sv);
                case TogglePolicy::ExperimentalFeatures:
                    return TogglePolicyInternal(policy, "EnableExperimentalFeatures"sv);
                case TogglePolicy::LocalManifestFiles:
                    return TogglePolicyInternal(policy, "EnableLocalManifestFiles"sv);
                case TogglePolicy::DefaultSource:
                    return TogglePolicyInternal(policy, "EnableDefaultSource"sv);
                case TogglePolicy::MSStoreSource:
                    return TogglePolicyInternal(policy, "EnableMSStoreSource"sv);
                case TogglePolicy::AdditionalSources:
                    return TogglePolicyInternal(policy, "EnableAdditionalSources"sv);
                case TogglePolicy::AllowedSources:
                    return TogglePolicyInternal(policy, "EnableAllowedSources"sv);
                default:
                    THROW_HR(E_UNEXPECTED);
                }
            }
        };

        template<Registry::Value::Type T>
        std::optional<decltype(std::declval<Registry::Value>().GetValue<T>())> GetRegistryValue(const Registry::Key& key, const std::string_view valueName)
        {
            if (!key)
            {
                // Key does not exist; there's nothing to return
                return std::nullopt;
            }

            auto value = key[valueName];
            if (!value.has_value())
            {
                return std::nullopt;
            }

            if (value->GetType() != T)
            {
                AICLI_LOG(Core, Warning, << "Value for policy '" << valueName << "' does not have expected type");
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

            AICLI_LOG(Core, Info, << "Found policy '" << valueName << "', Value: " << *intValue);
            return (bool)*intValue;
        }

        PolicyState GetStateInternal(const Registry::Key& key, TogglePolicy policy)
        {
            // Default to not configured if there is no policy for this
            if (policy == TogglePolicy::None)
            {
                return PolicyState::NotConfigured;
            }

            auto togglePolicy = TogglePolicyInternal::GetPolicy(policy);

            // Policies are not configured if there is no registry value.
            auto setting = RegistryValueIsTrue(key, togglePolicy.RegValueName);
            if (!setting.has_value())
            {
                return PolicyState::NotConfigured;
            }

            // Return flag as-is or invert depending on the policy
            return *setting ? PolicyState::Enabled : PolicyState::Disabled;
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

        std::optional<std::vector<std::string>> ValuePolicyMapping<ValuePolicy::AdditionalSources>::ReadAndValidate(const Registry::Key&)
        {
            // TODO
            return std::nullopt;
        }

        std::optional<std::vector<std::string>> ValuePolicyMapping<ValuePolicy::AllowedSources>::ReadAndValidate(const Registry::Key&)
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
            m_toggles[policy] = GetStateInternal(key, policy);
        }
    }

    PolicyState GroupPolicy::GetState(TogglePolicy policy) const
    {
        auto itr = m_toggles.find(policy);
        if (itr == m_toggles.end())
        {
            return PolicyState::NotConfigured;
        }

        return itr->second;
    }

    bool GroupPolicy::IsEnabled(TogglePolicy policy) const
    {
        PolicyState state = GetState(policy);
        if (state == PolicyState::NotConfigured)
        {
            return TogglePolicyInternal::GetPolicy(policy).DefaultIsEnabled;
        }

        return state == PolicyState::Enabled;
    }

    GroupPolicy const& GroupPolicy::Instance()
    {
        return InstanceInternal();
    }

    void GroupPolicy::OverrideInstance(GroupPolicy* overridePolicy)
    {
        InstanceInternal(overridePolicy);
    }

    void GroupPolicy::ResetInstance()
    {
        InstanceInternal(std::nullopt);
    }

}