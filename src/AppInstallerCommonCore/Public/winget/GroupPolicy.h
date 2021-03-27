// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#pragma once

#include "AppInstallerLanguageUtilities.h"
#include "winget/Registry.h"
#include "winget/Resources.h"
#include <string_view>

using namespace std::string_view_literals;

namespace AppInstaller::Settings
{

    // A policy that sets a value for some setting.
    // The value of the policy is a value in the registry key, or is
    // made up of sub-keys for settings that are lists.
    enum class ValuePolicy
    {
        SourceAutoUpdateIntervalInMinutes,
        AdditionalSources, // TODO
        AllowedSources, // TODO
        Max,
    };

    // A policy that acts as a toggle to enable or disable a feature.
    // They are backed by a DWORD value with values 0 and 1.
    struct TogglePolicy
    {
        enum class Policy
        {
            None = 0,
            WinGet,
            Settings,
            ExperimentalFeatures,
            LocalManifestFiles,
            HashOverride,
            DefaultSource,
            MSStoreSource,
            AdditionalSources, // TODO
            AllowedSources, // TODO
            Max,
        };

        TogglePolicy(Policy policy, std::string_view regValueName, StringResource::StringId policyName, bool defaultIsEnabled = true) :
            m_policy(policy), m_regValueName(regValueName), m_policyName(policyName), m_defaultIsEnabled(defaultIsEnabled) {}

        static TogglePolicy GetPolicy(Policy policy);
        static std::vector<TogglePolicy> GetAllPolicies();

        Policy GetPolicy() const { return m_policy; }
        std::string_view RegValueName() const { return m_regValueName; }
        StringResource::StringId PolicyName() const { return m_policyName; }
        bool DefaultIsEnabled() const { return m_defaultIsEnabled; }

    private:
        Policy m_policy;
        std::string_view m_regValueName;
        StringResource::StringId m_policyName;
        bool m_defaultIsEnabled;
    };

    // Possible configuration states for a policy.
    enum class PolicyState
    {
        NotConfigured,
        Disabled,
        Enabled,
    };

    namespace details
    {

        template <ValuePolicy P>
        struct ValuePolicyMapping
        {
            // value_t - type of the policy
            // ReadAndValidate() - Function that reads the value and does semantic validation.

            // For simple values:
            //  ValueName - Name of the registry value
            //  ValueType - Type of the registry value
            //  reg_value_t - Type returned by the registry when reading the value
        };

#define POLICY_MAPPING_SPECIALIZATION(_policy_, _type_) \
        template <> \
        struct ValuePolicyMapping<_policy_> \
        { \
            using value_t = _type_; \
            static std::optional<value_t> ReadAndValidate(const Registry::Key& policiesKey); \
        }

#define POLICY_MAPPING_VALUE_SPECIALIZATION(_policy_, _type_, _valueName_, _valueType_) \
        template<> \
        struct ValuePolicyMapping<_policy_> \
        { \
            static constexpr std::string_view ValueName = _valueName_; \
            static constexpr Registry::Value::Type ValueType = _valueType_; \
            using value_t = _type_; \
            using reg_value_t = decltype(std::declval<Registry::Value>().GetValue<ValueType>()); \
            static std::optional<value_t> ReadAndValidate(const Registry::Key& policiesKey); \
        }

        POLICY_MAPPING_VALUE_SPECIALIZATION(ValuePolicy::SourceAutoUpdateIntervalInMinutes, uint32_t, "SourceAutoUpdateIntervalInMinutes"sv, Registry::Value::Type::DWord);

        // TODO: Wire up policies for sources
        POLICY_MAPPING_SPECIALIZATION(ValuePolicy::AdditionalSources, std::vector<std::string>);
        POLICY_MAPPING_SPECIALIZATION(ValuePolicy::AllowedSources, std::vector<std::string>);
    }

    // Representation of the policies read from the registry.
    struct GroupPolicy
    {
        using ValuePoliciesMap = EnumBasedVariantMap<ValuePolicy, details::ValuePolicyMapping>;

        static GroupPolicy const& Instance();

        GroupPolicy(const Registry::Key& key);
        ~GroupPolicy() = default;

        GroupPolicy() = delete;

        GroupPolicy(const GroupPolicy&) = delete;
        GroupPolicy& operator=(const GroupPolicy&) = delete;

        GroupPolicy(GroupPolicy&&) = delete;
        GroupPolicy& operator=(GroupPolicy&&) = delete;

        // Gets the policy value if it is present
        template<ValuePolicy P>
        std::optional<typename details::ValuePolicyMapping<P>::value_t> GetValue() const
        {
            if (m_values.Contains(P))
            {
                return m_values.Get<P>();
            }
            else
            {
                return std::nullopt;
            }
        }

        PolicyState GetState(TogglePolicy::Policy policy) const;

        // Checks whether a policy is enabled, using an appropriate default when not configured.
        // Should not be used when not configured means something different than enabled/disabled.
        bool IsEnabled(TogglePolicy::Policy policy) const;

    private:
        std::map<TogglePolicy::Policy, PolicyState> m_toggles;
        ValuePoliciesMap m_values;

#ifndef AICLI_DISABLE_TEST_HOOKS
    protected:
        static void OverrideInstance(GroupPolicy* gp);
        static void ResetInstance();
#endif
    };

    inline const GroupPolicy& GroupPolicies()
    {
        return GroupPolicy::Instance();
    }
}