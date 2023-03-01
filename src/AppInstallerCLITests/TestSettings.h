// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#pragma once
#include <winget/Settings.h>
#include <winget/UserSettings.h>
#include <wil/resource.h>
#include <string>

namespace TestCommon
{
    // Repeat the policy values here so we can catch unintended changes in the source.
    const std::wstring WinGetPolicyValueName = L"EnableAppInstaller";
    const std::wstring WinGetSettingsPolicyValueName = L"EnableSettings";
    const std::wstring ExperimentalFeaturesPolicyValueName = L"EnableExperimentalFeatures";
    const std::wstring LocalManifestsPolicyValueName = L"EnableLocalManifestFiles";
    const std::wstring EnableHashOverridePolicyValueName = L"EnableHashOverride";
    const std::wstring EnableLocalArchiveMalwareScanOverridePolicyValueName = L"EnableLocalArchiveMalwareScanOverride";
    const std::wstring DefaultSourcePolicyValueName = L"EnableDefaultSource";
    const std::wstring MSStoreSourcePolicyValueName = L"EnableMicrosoftStoreSource";
    const std::wstring AdditionalSourcesPolicyValueName = L"EnableAdditionalSources";
    const std::wstring AllowedSourcesPolicyValueName = L"EnableAllowedSources";
    const std::wstring BypassCertificatePinningForMicrosoftStoreValueName = L"EnableBypassCertificatePinningForMicrosoftStore";

    const std::wstring SourceUpdateIntervalPolicyValueName = L"SourceAutoUpdateInterval";
    const std::wstring SourceUpdateIntervalPolicyOldValueName = L"SourceAutoUpdateIntervalInMinutes";

    const std::wstring AdditionalSourcesPolicyKeyName = L"AdditionalSources";
    const std::wstring AllowedSourcesPolicyKeyName = L"AllowedSources";

    void SetSetting(const AppInstaller::Settings::StreamDefinition& stream, std::string_view value);
    void RemoveSetting(const AppInstaller::Settings::StreamDefinition& stream);
    std::filesystem::path GetPathTo(const AppInstaller::Settings::StreamDefinition& stream);

    // This type removes the settings file on creation and destruction to ensure that a test that modifies them can do so cleanly.
    struct UserSettingsFileGuard
    {
        UserSettingsFileGuard();
        ~UserSettingsFileGuard();
    };

    [[nodiscard]] UserSettingsFileGuard DeleteUserSettingsFiles();

    struct UserSettingsTest : AppInstaller::Settings::UserSettings
    {
    };

    struct GroupPolicyTestOverride : AppInstaller::Settings::GroupPolicy
    {
        GroupPolicyTestOverride() : GroupPolicyTestOverride(RegCreateVolatileTestRoot().get()) {}
        GroupPolicyTestOverride(const AppInstaller::Registry::Key& key);
        ~GroupPolicyTestOverride();

        template<AppInstaller::Settings::ValuePolicy P>
        void SetValue(const ValueType<P>& value)
        {
            m_values.Add<P>(value);
        }

        template<AppInstaller::Settings::ValuePolicy P>
        void SetValue(ValueType<P> &&value)
        {
            m_values.Add<P>(std::move(value));
        }

        void SetState(AppInstaller::Settings::TogglePolicy::Policy policy, AppInstaller::Settings::PolicyState state);
    };

    // Matcher that lets us verify GroupPolicyExceptions.
    struct GroupPolicyExceptionMatcher : public Catch::MatcherBase<AppInstaller::Settings::GroupPolicyException>
    {
        GroupPolicyExceptionMatcher(AppInstaller::Settings::TogglePolicy::Policy policy) : m_expectedPolicy(policy) {}

        bool match(const AppInstaller::Settings::GroupPolicyException& e) const override
        {
            return e.Policy() == m_expectedPolicy;
        }

        std::string describe() const override
        {
            std::ostringstream result;
            result << "has policy == " << m_expectedPolicy;
            return result.str();
        }

    private:
        AppInstaller::Settings::TogglePolicy::Policy m_expectedPolicy;
    };

#define REQUIRE_POLICY_EXCEPTION(_expr_, _policy_)     REQUIRE_THROWS_MATCHES(_expr_, AppInstaller::Settings::GroupPolicyException, TestCommon::GroupPolicyExceptionMatcher(_policy_))
}