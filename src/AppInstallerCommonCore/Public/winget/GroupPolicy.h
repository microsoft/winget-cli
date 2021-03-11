// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#pragma once

#include "winget/Registry.h"
#include <string_view>

namespace AppInstaller::Settings
{
    // A policy that sets a value for some setting.
    // The value of the policy is a value in the registry key, or is
    // made up of sub-keys for settings that are lists.
    enum class ValuePolicy
    {
        SourceAutoUpdateIntervalInMinutes, // TODO
        ProgressBarStyle, // TODO
        IncludeSources, // TODO
        Max,
    };

    // A policy that acts as a toggle to enable or disable a feature.
    // They are backed by a DWORD value with values zero and non-zero.
    enum class TogglePolicy
    {
        None = 0,
        DisableWinGet,
        DisableSettingsCommand,
        DisableExperimentalFeatures,
        DisableLocalManifestFiles,
        ExcludeDefaultSources,
        DisableSourceConfiguration,
        Max,
    };

    namespace details
    {
        template <ValuePolicy P>
        struct ValuePolicyMapping
        {
            // value_t - type of the policy
            // Validate - Function that reads the value and does semantic validation.
        };

#define POLICYMAPPING_SPECIALIZATION(_policy_, _type_) \
        template <> \
        struct ValuePolicyMapping<_policy_> \
        { \
            using value_t = _type_; \
            static std::optional<value_t> Validate(const Registry::Key& policiesKey); \
        }

        POLICYMAPPING_SPECIALIZATION(ValuePolicy::SourceAutoUpdateIntervalInMinutes, std::chrono::minutes);
        POLICYMAPPING_SPECIALIZATION(ValuePolicy::ProgressBarStyle, std::string);
        POLICYMAPPING_SPECIALIZATION(ValuePolicy::IncludeSources, std::vector<std::string>);
    }

    // Representation of the policies read from the registry.
    struct GroupPolicy
    {
        // using ValuePoliciesMap = EnumBasedVariantMap<ValuePolicy, details::ValuePolicyMapping>;

        static GroupPolicy const& Instance();

#ifndef AICLI_DISABLE_TEST_HOOKS
        inline static void SetPolicyRegistryKey(Registry::Key&& key)
        {
            s_instance = std::make_unique<GroupPolicy>(std::move(key));
        }
#endif

        GroupPolicy() = delete;

        GroupPolicy(const GroupPolicy&) = delete;
        GroupPolicy& operator=(const GroupPolicy&) = delete;

        GroupPolicy(GroupPolicy&&) = delete;
        GroupPolicy& operator=(GroupPolicy&&) = delete;

        GroupPolicy(const Registry::Key& key);
        ~GroupPolicy() = default;

        // Gets the policy value if it is present
        template <ValuePolicy P>
        typename std::optional<typename details::ValuePolicyMapping<P>::value_t> GetValue() const
        {
            return std::nullopt; // m_values.Get<P>(); // m_values.Contains<P>() ? m_values.Get<P>() : std::nullopt;
        }

        bool IsAllowed(TogglePolicy policy) const;

    private:
        static std::unique_ptr<GroupPolicy> s_instance;

        std::map<TogglePolicy, bool> m_toggles;
        // ValuePoliciesMap m_values;
    };

    inline const GroupPolicy& GroupPolicies()
    {
        return GroupPolicy::Instance();
    }
}