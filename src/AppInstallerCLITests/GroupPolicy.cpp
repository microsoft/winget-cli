// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "TestCommon.h"
#include "TestSettings.h"
#include "winget/GroupPolicy.h"

using namespace TestCommon;
using namespace AppInstaller::Settings;
using namespace std::string_view_literals;

namespace
{
    std::wstring GetSourceJson(std::wstring_view name, std::wstring_view arg, std::wstring_view type, std::wstring_view data, std::wstring_view identifier)
    {
        std::wstringstream json;
        json << L"{ \"Name\":\"" << name << L"\", \"Arg\":\"" << arg << L"\", \"Type\":\"" << type << L"\", \"Data\":\"" << data << L"\", \"Identifier\":\"" << identifier << L"\" }";
        return json.str();
    }
}

TEST_CASE("GroupPolicy_NoPolicies", "[groupPolicy]")
{
    auto policiesKey = RegCreateVolatileTestRoot();
    GroupPolicy groupPolicy{ policiesKey.get() };

    // Policies setting a value should be empty
    REQUIRE(!groupPolicy.GetValue<ValuePolicy::SourceAutoUpdateIntervalInMinutes>().has_value());
    REQUIRE(!groupPolicy.GetValue<ValuePolicy::AdditionalSources>().has_value());
    REQUIRE(!groupPolicy.GetValue<ValuePolicy::AllowedSources>().has_value());

    // Everything should be not configured
    for (const auto& policy : TogglePolicy::GetAllPolicies())
    {
        REQUIRE(groupPolicy.GetState(policy.GetPolicy()) == PolicyState::NotConfigured);
    }
}

TEST_CASE("GroupPolicy_UpdateInterval", "[groupPolicy]")
{
    auto policiesKey = RegCreateVolatileTestRoot();

    SECTION("Good value")
    {
        SetRegistryValue(policiesKey.get(), SourceUpdateIntervalPolicyValueName, 5);
        GroupPolicy groupPolicy{ policiesKey.get() };

        auto policy = groupPolicy.GetValue<ValuePolicy::SourceAutoUpdateIntervalInMinutes>();
        REQUIRE(policy.has_value());
        REQUIRE(*policy == 5);
    }

    SECTION("Wrong type")
    {
        SetRegistryValue(policiesKey.get(), SourceUpdateIntervalPolicyValueName, L"Wrong");
        GroupPolicy groupPolicy{ policiesKey.get() };

        auto policy = groupPolicy.GetValue<ValuePolicy::SourceAutoUpdateIntervalInMinutes>();
        REQUIRE(!policy.has_value());
    }
}

TEST_CASE("GroupPolicy_Sources", "[groupPolicy]")
{
    auto policiesKey = RegCreateVolatileTestRoot();

    // Note that the following tests mix using Additional/Allowed sources policy.
    SECTION("Single source")
    {
        // We can read single source correctly
        auto additionalSourcesKey = RegCreateVolatileSubKey(policiesKey.get(), AdditionalSourcesPolicyKeyName);
        SetRegistryValue(additionalSourcesKey.get(), L"0", GetSourceJson(L"source-name", L"source-arg", L"source-type", L"source-data", L"source-identifier"), REG_SZ);
        GroupPolicy groupPolicy{ policiesKey.get() };

        auto policy = groupPolicy.GetValue<ValuePolicy::AdditionalSources>();
        REQUIRE(policy.has_value());
        REQUIRE(policy->size() == 1);
        REQUIRE(policy.value()[0].Name == "source-name");
        REQUIRE(policy.value()[0].Arg == "source-arg");
        REQUIRE(policy.value()[0].Type == "source-type");
        REQUIRE(policy.value()[0].Data == "source-data");
        REQUIRE(policy.value()[0].Identifier == "source-identifier");
    }
    SECTION("Missing field")
    {
        // A single missing field causes the source to not be read.
        // "Type" is missing here.
        std::wstring sourceJson = L"{ \"Name\":\"source_name\", \"Arg\":\"source_arg\", \"Data\":\"source_data\", \"Identifier\":\"source_identifier\" }";
        auto additionalSourcesKey = RegCreateVolatileSubKey(policiesKey.get(), AllowedSourcesPolicyKeyName);
        SetRegistryValue(additionalSourcesKey.get(), L"0", sourceJson, REG_SZ);
        GroupPolicy groupPolicy{ policiesKey.get() };

        auto policy = groupPolicy.GetValue<ValuePolicy::AllowedSources>();
        REQUIRE(policy.has_value());
        REQUIRE(policy->empty());
    }
    SECTION("Invalid field")
    {
        // A single invalid field causes the source to not be read.
        // "Data" is invalid as it is an object, not a string.
        std::wstring sourceJson = L"{ \"Name\":\"source_name\", \"Arg\":\"source_arg\", \"Data\":{}, \"Type\":\"source_type\", \"Identifier\":\"source_identifier\" }";
        auto additionalSourcesKey = RegCreateVolatileSubKey(policiesKey.get(), AdditionalSourcesPolicyKeyName);
        SetRegistryValue(additionalSourcesKey.get(), L"0", sourceJson, REG_SZ);
        GroupPolicy groupPolicy{ policiesKey.get() };

        auto policy = groupPolicy.GetValue<ValuePolicy::AdditionalSources>();
        REQUIRE(policy.has_value());
        REQUIRE(policy->empty());
    }
    SECTION("Invalid source JSON")
    {
        // An invalid source JSON causes the source to not be read.
        auto additionalSourcesKey = RegCreateVolatileSubKey(policiesKey.get(), AllowedSourcesPolicyKeyName);
        SetRegistryValue(additionalSourcesKey.get(), L"0", L"not a JSON", REG_SZ);
        GroupPolicy groupPolicy{ policiesKey.get() };

        auto policy = groupPolicy.GetValue<ValuePolicy::AllowedSources>();
        REQUIRE(policy.has_value());
        REQUIRE(policy->empty());
    }
    SECTION("Missing key")
    {
        // If the key does not exist we should not get anything.
        GroupPolicy groupPolicy{ policiesKey.get() };

        auto policy = groupPolicy.GetValue<ValuePolicy::AdditionalSources>();
        REQUIRE_FALSE(policy.has_value());
    }
    SECTION("Empty key")
    {
        // If the key is empty we should get an empty list.
        // Note that the policy editor doesn't actually create empty keys.
        auto additionalSourcesKey = RegCreateVolatileSubKey(policiesKey.get(), AllowedSourcesPolicyKeyName);
        GroupPolicy groupPolicy{ policiesKey.get() };

        auto policy = groupPolicy.GetValue<ValuePolicy::AllowedSources>();
        REQUIRE(policy.has_value());
        REQUIRE(policy->empty());
    }
    SECTION("Valid list")
    {
        // We should be able to read multiple values.
        // No specific order is required, but it will likely be the same.
        auto additionalSourcesKey = RegCreateVolatileSubKey(policiesKey.get(), AdditionalSourcesPolicyKeyName);
        SetRegistryValue(additionalSourcesKey.get(), L"0", GetSourceJson(L"s0-name", L"s0-arg", L"s0-type", L"s0-data", L"s0-identifier"), REG_SZ);
        SetRegistryValue(additionalSourcesKey.get(), L"1", GetSourceJson(L"s1-name", L"s1-arg", L"s1-type", L"s1-data", L"s1-identifier"), REG_SZ);
        SetRegistryValue(additionalSourcesKey.get(), L"2", GetSourceJson(L"s2-name", L"s2-arg", L"s2-type", L"s2-data", L"s2-identifier"), REG_SZ);
        GroupPolicy groupPolicy{ policiesKey.get() };

        auto policy = groupPolicy.GetValue<ValuePolicy::AdditionalSources>();
        REQUIRE(policy.has_value());
        REQUIRE(policy->size() == 3);

        REQUIRE(policy.value()[0].Name == "s0-name");
        REQUIRE(policy.value()[0].Arg == "s0-arg");
        REQUIRE(policy.value()[0].Type == "s0-type");
        REQUIRE(policy.value()[0].Data == "s0-data");
        REQUIRE(policy.value()[0].Identifier == "s0-identifier");

        REQUIRE(policy.value()[1].Name == "s1-name");
        REQUIRE(policy.value()[1].Arg == "s1-arg");
        REQUIRE(policy.value()[1].Type == "s1-type");
        REQUIRE(policy.value()[1].Data == "s1-data");
        REQUIRE(policy.value()[1].Identifier == "s1-identifier");

        REQUIRE(policy.value()[2].Name == "s2-name");
        REQUIRE(policy.value()[2].Arg == "s2-arg");
        REQUIRE(policy.value()[2].Type == "s2-type");
        REQUIRE(policy.value()[2].Data == "s2-data");
        REQUIRE(policy.value()[2].Identifier == "s2-identifier");
    }
    SECTION("Invalid source in list")
    {
        // If a single source is invalid we should still get all others
        auto additionalSourcesKey = RegCreateVolatileSubKey(policiesKey.get(), AdditionalSourcesPolicyKeyName);
        SetRegistryValue(additionalSourcesKey.get(), L"0", GetSourceJson(L"s0-name", L"s0-arg", L"s0-type", L"s0-data", L"s0-identifier"), REG_SZ);
        SetRegistryValue(additionalSourcesKey.get(), L"1", L"not a source", REG_SZ);
        SetRegistryValue(additionalSourcesKey.get(), L"2", GetSourceJson(L"s2-name", L"s2-arg", L"s2-type", L"s2-data", L"s2-identifier"), REG_SZ);
        GroupPolicy groupPolicy{ policiesKey.get() };

        auto policy = groupPolicy.GetValue<ValuePolicy::AdditionalSources>();
        REQUIRE(policy.has_value());
        REQUIRE(policy->size() == 2);

        REQUIRE(policy.value()[0].Name == "s0-name");
        REQUIRE(policy.value()[0].Arg == "s0-arg");
        REQUIRE(policy.value()[0].Type == "s0-type");
        REQUIRE(policy.value()[0].Data == "s0-data");
        REQUIRE(policy.value()[0].Identifier == "s0-identifier");

        REQUIRE(policy.value()[1].Name == "s2-name");
        REQUIRE(policy.value()[1].Arg == "s2-arg");
        REQUIRE(policy.value()[1].Type == "s2-type");
        REQUIRE(policy.value()[1].Data == "s2-data");
        REQUIRE(policy.value()[1].Identifier == "s2-identifier");
    }
    SECTION("Exported JSON")
    {
        // Policy should be able to use an exported JSON strings
        SourceFromPolicy source;
        source.Name = "json-name";
        source.Type = "json-type";
        source.Arg = "json-arg";
        source.Data = "json-data";
        source.Identifier = "json-id";

        auto additionalSourcesKey = RegCreateVolatileSubKey(policiesKey.get(), AllowedSourcesPolicyKeyName);
        SetRegistryValue(additionalSourcesKey.get(), L"0", AppInstaller::Utility::ConvertToUTF16(source.ToJsonString()));
        GroupPolicy groupPolicy{ policiesKey.get() };

        auto policy = groupPolicy.GetValue<ValuePolicy::AllowedSources>();
        REQUIRE(policy.has_value());
        REQUIRE(policy->size() == 1);
        REQUIRE(policy.value()[0].Name == source.Name);
        REQUIRE(policy.value()[0].Arg == source.Arg);
        REQUIRE(policy.value()[0].Type == source.Type);
        REQUIRE(policy.value()[0].Data == source.Data);
        REQUIRE(policy.value()[0].Identifier == source.Identifier);
    }
}

TEST_CASE("GroupPolicy_Toggle", "[groupPolicy]")
{
    auto policiesKey = RegCreateVolatileTestRoot();

    SECTION("'None' is not configured")
    {
        GroupPolicy groupPolicy{ policiesKey.get() };
        REQUIRE(groupPolicy.GetState(TogglePolicy::Policy::None) == PolicyState::NotConfigured);
        REQUIRE(groupPolicy.IsEnabled(TogglePolicy::Policy::None));
    }

    SECTION("Enabled")
    {
        SetRegistryValue(policiesKey.get(), WinGetPolicyValueName, 1);
        GroupPolicy groupPolicy{ policiesKey.get() };
        REQUIRE(groupPolicy.GetState(TogglePolicy::Policy::WinGet) == PolicyState::Enabled);
        REQUIRE(groupPolicy.IsEnabled(TogglePolicy::Policy::WinGet));
    }

    SECTION("Disabled")
    {
        SetRegistryValue(policiesKey.get(), LocalManifestsPolicyValueName, 0);
        GroupPolicy groupPolicy{ policiesKey.get() };
        REQUIRE(groupPolicy.GetState(TogglePolicy::Policy::LocalManifestFiles) == PolicyState::Disabled);
        REQUIRE_FALSE(groupPolicy.IsEnabled(TogglePolicy::Policy::LocalManifestFiles));
    }

    SECTION("Wrong type")
    {
        SetRegistryValue(policiesKey.get(), ExperimentalFeaturesPolicyValueName, L"Wrong");
        GroupPolicy groupPolicy{ policiesKey.get() };
        REQUIRE(groupPolicy.GetState(TogglePolicy::Policy::DefaultSource) == PolicyState::NotConfigured);
        REQUIRE(groupPolicy.IsEnabled(TogglePolicy::Policy::DefaultSource));
    }
}

TEST_CASE("GroupPolicy_AllEnabled", "[groupPolicy]")
{
    auto policiesKey = RegCreateVolatileTestRoot();
    SetRegistryValue(policiesKey.get(), WinGetPolicyValueName, 1);
    SetRegistryValue(policiesKey.get(), WinGetSettingsPolicyValueName, 1);
    SetRegistryValue(policiesKey.get(), ExperimentalFeaturesPolicyValueName, 1);
    SetRegistryValue(policiesKey.get(), LocalManifestsPolicyValueName, 1);
    SetRegistryValue(policiesKey.get(), EnableHashOverridePolicyValueName, 1);
    SetRegistryValue(policiesKey.get(), DefaultSourcePolicyValueName, 1);
    SetRegistryValue(policiesKey.get(), MSStoreSourcePolicyValueName, 1);;
    SetRegistryValue(policiesKey.get(), AdditionalSourcesPolicyValueName, 1);
    SetRegistryValue(policiesKey.get(), AllowedSourcesPolicyValueName, 1);

    GroupPolicy groupPolicy{ policiesKey.get() };
    for (const auto& policy : TogglePolicy::GetAllPolicies())
    {
        REQUIRE(groupPolicy.GetState(policy.GetPolicy()) == PolicyState::Enabled);
    }
}