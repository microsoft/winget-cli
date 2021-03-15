// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#pragma once

#include "AppInstallerLanguageUtilities.h"
#include "winget/Registry.h"
#include <string_view>

namespace AppInstaller::Settings
{
    using namespace std::string_view_literals;

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
            // ReadAndValidate() - Function that reads the value and does semantic validation.

            // For simple values:
            //  ValueName - Name of the registry value
            //  ValueType - Type of the registry value
            //  reg_value_t - Type returned by the registry when reading the value
        };

#define POLICYMAPPING_SPECIALIZATION(_policy_, _type_) \
        template <> \
        struct ValuePolicyMapping<_policy_> \
        { \
            using value_t = _type_; \
            static std::optional<value_t> ReadAndValidate(const Registry::Key& policiesKey); \
        }

#define POLICYMAPPING_VALUE_SPECIALIZATION(_policy_, _type_, _valueName_, _valueType_) \
        template<> \
        struct ValuePolicyMapping<_policy_> \
        { \
            static constexpr std::string_view ValueName = _valueName_; \
            static constexpr Registry::Value::Type ValueType = _valueType_; \
            using value_t = _type_; \
            using reg_value_t = decltype(std::declval<Registry::Value>().GetValue<ValueType>()); \
            static std::optional<value_t> ReadAndValidate(const Registry::Key& policiesKey); \
        }

        POLICYMAPPING_VALUE_SPECIALIZATION(ValuePolicy::SourceAutoUpdateIntervalInMinutes, uint32_t, "SourceAutoUpdateIntervalInMinutes"sv, Registry::Value::Type::DWord);
        POLICYMAPPING_VALUE_SPECIALIZATION(ValuePolicy::ProgressBarStyle, std::string, "ProgressBarStyle"sv, Registry::Value::Type::String);

        POLICYMAPPING_SPECIALIZATION(ValuePolicy::IncludeSources, std::vector<std::string>);
    }

    // Representation of the policies read from the registry.
    struct GroupPolicy
    {
        using ValuePoliciesMap = EnumBasedVariantMap<ValuePolicy, details::ValuePolicyMapping>;

        static GroupPolicy const& Instance()
        {
            // TODO: Use final location
            constexpr std::string_view PoliciesKeyPath = "test\\policy"; // "SOFTWARE\\Policies\\Microsoft\\Windows\\WinGet";
            static GroupPolicy groupPolicy{ Registry::Key::OpenIfExists(HKEY_CURRENT_USER /* HKEY_LOCAL_MACHINE */, PoliciesKeyPath) };
            return groupPolicy;
        }

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

        bool IsAllowed(TogglePolicy policy) const;

    private:
        std::map<TogglePolicy, bool> m_toggles;
        ValuePoliciesMap m_values;

#ifndef AICLI_DISABLE_TEST_HOOKS
    protected:
#endif
        GroupPolicy(const Registry::Key& key);
        ~GroupPolicy() = default;
    };

    inline const GroupPolicy& GroupPolicies()
    {
        return GroupPolicy::Instance();
    }
}