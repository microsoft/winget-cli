// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "winget/GroupPolicy.h"
#include "AppInstallerLogging.h"

using namespace AppInstaller::StringResource;

namespace AppInstaller::Settings
{
    namespace
    {
        GroupPolicy& InstanceInternal(std::optional<GroupPolicy*> overridePolicy = {})
        {
            // TODO: Read from the actual registry key
            static GroupPolicy s_groupPolicy{ Registry::Key{} };
            static GroupPolicy* s_override = nullptr;

            if (overridePolicy.has_value())
            {
                s_override = overridePolicy.value();
            }

            return (s_override ? *s_override : s_groupPolicy);
        }

        template<Registry::Value::Type T>
        std::optional<decltype(std::declval<Registry::Value>().GetValue<T>())> GetRegistryValue(const Registry::Key& key, const std::string_view valueName)
        {
            if (!key)
            {
                // Key does not exist; there's nothing to return
                return std::nullopt;
            }

            auto regValue = key[valueName];
            if (!regValue.has_value())
            {
                // Value does not exist
                return std::nullopt;
            }

            auto value = regValue->TryGetValue<T>();
            if (!value.has_value())
            {
                AICLI_LOG(Core, Warning, << "Value for policy '" << valueName << "' does not have expected type");
                return std::nullopt;
            }

            return std::move(value.value());
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

        PolicyState GetStateInternal(const Registry::Key& key, TogglePolicy::Policy policy)
        {
            // Default to not configured if there is no policy for this
            if (policy == TogglePolicy::Policy::None)
            {
                return PolicyState::NotConfigured;
            }

            auto togglePolicy = TogglePolicy::GetPolicy(policy);

            // Policies are not configured if there is no registry value.
            auto setting = RegistryValueIsTrue(key, togglePolicy.RegValueName());
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

    TogglePolicy TogglePolicy::GetPolicy(TogglePolicy::Policy policy)
    {
        switch (policy)
        {
        case TogglePolicy::Policy::WinGet:
            return TogglePolicy(policy, "EnableWindowsPackageManager"sv, String::PolicyEnableWinGet);
        case TogglePolicy::Policy::Settings: return
            TogglePolicy(policy, "EnableWindowsPackageManagerSettings"sv, String::PolicyEnableWingetSettings);
        case TogglePolicy::Policy::ExperimentalFeatures:
            return TogglePolicy(policy, "EnableExperimentalFeatures"sv, String::PolicyEnableExperimentalFeatures);
        case TogglePolicy::Policy::LocalManifestFiles:
            return TogglePolicy(policy, "EnableLocalManifestFiles"sv, String::PolicyEnableLocalManifests);
        case TogglePolicy::Policy::HashOverride:
            return TogglePolicy(policy, "EnableHashOverride"sv, String::PolicyEnableHashOverride);
        case TogglePolicy::Policy::DefaultSource:
            return TogglePolicy(policy, "EnableDefaultSource"sv, String::PolicyEnableDefaultSource);
        case TogglePolicy::Policy::MSStoreSource:
            return TogglePolicy(policy, "EnableMSStoreSource"sv, String::PolicyEnableMSStoreSource);
        case TogglePolicy::Policy::AdditionalSources:
            return TogglePolicy(policy, "EnableAdditionalSources"sv, String::PolicyAdditionalSources);
        case TogglePolicy::Policy::AllowedSources:
            return TogglePolicy(policy, "EnableAllowedSources"sv, String::PolicyAllowedSources);
        default:
            THROW_HR(E_UNEXPECTED);
        }
    }

    std::vector<TogglePolicy> TogglePolicy::GetAllPolicies()
    {
        using Toggle_t = std::underlying_type_t<TogglePolicy::Policy>;

        std::vector<TogglePolicy> result;

        // Skip "None"
        for (Toggle_t i = 1 + static_cast<Toggle_t>(TogglePolicy::Policy::None); i < static_cast<Toggle_t>(TogglePolicy::Policy::Max); ++i)
        {
            result.emplace_back(GetPolicy(static_cast<Policy>(i)));
        }

        return result;
    }

    GroupPolicy::GroupPolicy(const Registry::Key& key)
    {
        ValidateAllValuePolicies(key, m_values, std::make_index_sequence<static_cast<size_t>(ValuePolicy::Max)>());

        using Toggle_t = std::underlying_type_t<TogglePolicy::Policy>;
        for (Toggle_t i = static_cast<Toggle_t>(TogglePolicy::Policy::None); i < static_cast<Toggle_t>(TogglePolicy::Policy::Max); ++i)
        {
            auto policy = static_cast<TogglePolicy::Policy>(i);
            m_toggles[policy] = GetStateInternal(key, policy);
        }
    }

    PolicyState GroupPolicy::GetState(TogglePolicy::Policy policy) const
    {
        auto itr = m_toggles.find(policy);
        if (itr == m_toggles.end())
        {
            return PolicyState::NotConfigured;
        }

        return itr->second;
    }

    bool GroupPolicy::IsEnabled(TogglePolicy::Policy policy) const
    {
        if (policy == TogglePolicy::Policy::None)
        {
            return true;
        }

        PolicyState state = GetState(policy);
        if (state == PolicyState::NotConfigured)
        {
            return TogglePolicy::GetPolicy(policy).DefaultIsEnabled();
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
        InstanceInternal(nullptr);
    }
}