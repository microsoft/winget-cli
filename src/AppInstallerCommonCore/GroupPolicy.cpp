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
        const GroupPolicy& InstanceInternal(std::optional<GroupPolicy*> overridePolicy = {})
        {
            const static GroupPolicy s_groupPolicy{ Registry::Key::OpenIfExists(HKEY_LOCAL_MACHINE, "Software\\Policies\\Microsoft\\Windows\\AppInstaller") };
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
        void Validate(const Registry::Key& policiesKey, GroupPolicy::ValuePoliciesMap& policies)
        {
            auto value = details::ValuePolicyMapping<P>::ReadAndValidate(policiesKey);
            if (value.has_value())
            {
                policies.Add<P>(std::move(*value));
            }
        }

        template <>
        void Validate<ValuePolicy::None>(const Registry::Key&, GroupPolicy::ValuePoliciesMap&) {};

        template <size_t... P>
        void ValidateAllValuePolicies(
            const Registry::Key& policiesKey,
            GroupPolicy::ValuePoliciesMap& policies,
            std::index_sequence<P...>)
        {
            // Use folding to call each policy validate function.
            (FoldHelper{}, ..., Validate<static_cast<ValuePolicy>(P)>(policiesKey, policies));
        }

        // Reads a list from a Group Policy.
        // The list is stored in a sub-key of the policies key, and each value in that key is a list item.
        // Cases not considered by this function because we don't use them:
        //  - When the list is in an arbitrary key, not a sub key.
        //  - When the list values are mixed with other values and are identified by a prefix in their names.
        //  - When the value names are relevant.
        template<ValuePolicy P>
        std::optional<typename details::ValuePolicyMapping<P>::value_t> ReadList(const Registry::Key& policiesKey)
        {
            using Mapping = details::ValuePolicyMapping<P>;

            auto listKey = policiesKey.SubKey(Mapping::KeyName);
            if (!listKey.has_value())
            {
                return std::nullopt;
            }

            std::vector<Mapping::item_t> items;
            for (const auto& value : listKey->Values())
            {
                auto item = Mapping::ReadAndValidateItem(value);
                if (item.has_value())
                {
                    items.emplace_back(std::move(item.value()));
                }
                else
                {
                    AICLI_LOG(Core, Warning, << "Failed to read Group Policy list value. Policy [" << Mapping::KeyName << "], Value [" << value.Name() << ']');
                }
            }

            return items;
        }

        std::optional<SourceFromPolicy> ReadSourceFromRegistryValue(const Registry::Value& item)
        {
            auto jsonString = item.TryGetValue<Registry::Value::Type::String>();
            if (!jsonString.has_value())
            {
                AICLI_LOG(Core, Warning, << "Registry value is not a string");
                return std::nullopt;
            }

            int stringLength = static_cast<int>(jsonString->length());
            Json::Value sourceJson;
            Json::CharReaderBuilder charReaderBuilder;
            const std::unique_ptr<Json::CharReader> jsonReader(charReaderBuilder.newCharReader());
            Json::String jsonErrors;
            if (!jsonReader->parse(jsonString->c_str(), jsonString->c_str() + stringLength, &sourceJson, &jsonErrors))
            {
                AICLI_LOG(Core, Warning, << "Registry value does not contain a valid JSON: " << jsonErrors);
                return std::nullopt;
            }

            SourceFromPolicy source;

            auto readSourceAttribute = [&](const std::string& name, std::string SourceFromPolicy::* member)
            {
                if (sourceJson.isMember(name) && sourceJson[name].isString())
                {
                    source.*member = sourceJson[name].asString();
                    return true;
                }
                else
                {
                    AICLI_LOG(Core, Warning, << "Source JSON does not contain a string value for " << name);
                    return false;
                }
            };

            bool allRead = readSourceAttribute("Name", &SourceFromPolicy::Name)
                && readSourceAttribute("Arg", &SourceFromPolicy::Arg)
                && readSourceAttribute("Type", &SourceFromPolicy::Type)
                && readSourceAttribute("Data", &SourceFromPolicy::Data)
                && readSourceAttribute("Identifier", &SourceFromPolicy::Identifier);
            if (!allRead)
            {
                return std::nullopt;
            }

            return source;
        }
    }

    namespace details
    {
#define POLICY_MAPPING_DEFAULT_LIST_READ(_policy_) \
        std::optional<typename ValuePolicyMapping<_policy_>::value_t> ValuePolicyMapping<_policy_>::ReadAndValidate(const Registry::Key& policiesKey) \
        { \
            return ReadList<_policy_>(policiesKey); \
        }

        POLICY_MAPPING_DEFAULT_LIST_READ(ValuePolicy::AdditionalSources);
        POLICY_MAPPING_DEFAULT_LIST_READ(ValuePolicy::AllowedSources);

        std::nullopt_t ValuePolicyMapping<ValuePolicy::None>::ReadAndValidate(const Registry::Key&)
        {
            return std::nullopt;
        }

        std::optional<uint32_t> ValuePolicyMapping<ValuePolicy::SourceAutoUpdateIntervalInMinutes>::ReadAndValidate(const Registry::Key& policiesKey)
        {
            using Mapping = ValuePolicyMapping<ValuePolicy::SourceAutoUpdateIntervalInMinutes>;
            return GetRegistryValue<Mapping::ValueType>(policiesKey, Mapping::ValueName);
        }

        std::optional<SourceFromPolicy> ValuePolicyMapping<ValuePolicy::AdditionalSources>::ReadAndValidateItem(const Registry::Value& item)
        {
            return ReadSourceFromRegistryValue(item);
        }

        std::optional<SourceFromPolicy> ValuePolicyMapping<ValuePolicy::AllowedSources>::ReadAndValidateItem(const Registry::Value& item)
        {
            return ReadSourceFromRegistryValue(item);
        }
    }

    TogglePolicy TogglePolicy::GetPolicy(TogglePolicy::Policy policy)
    {
        switch (policy)
        {
        case TogglePolicy::Policy::WinGet:
            return TogglePolicy(policy, "EnableAppInstaller"sv, String::PolicyEnableWinGet);
        case TogglePolicy::Policy::Settings: return
            TogglePolicy(policy, "EnableSettings"sv, String::PolicyEnableWingetSettings);
        case TogglePolicy::Policy::ExperimentalFeatures:
            return TogglePolicy(policy, "EnableExperimentalFeatures"sv, String::PolicyEnableExperimentalFeatures);
        case TogglePolicy::Policy::LocalManifestFiles:
            return TogglePolicy(policy, "EnableLocalManifestFiles"sv, String::PolicyEnableLocalManifests);
        case TogglePolicy::Policy::HashOverride:
            return TogglePolicy(policy, "EnableHashOverride"sv, String::PolicyEnableHashOverride);
        case TogglePolicy::Policy::DefaultSource:
            return TogglePolicy(policy, "EnableDefaultSource"sv, String::PolicyEnableDefaultSource);
        case TogglePolicy::Policy::MSStoreSource:
            return TogglePolicy(policy, "EnableMicrosoftStoreSource"sv, String::PolicyEnableMSStoreSource);
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

    std::string SourceFromPolicy::ToJsonString() const
    {
        Json::Value json{ Json::ValueType::objectValue };
        json["Name"] = Name;
        json["Type"] = Type;
        json["Arg"] = Arg;
        json["Data"] = Data;
        json["Identifier"] = Identifier;

        Json::StreamWriterBuilder writerBuilder;
        writerBuilder.settings_["indentation"] = "";
        return Json::writeString(writerBuilder, json);
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