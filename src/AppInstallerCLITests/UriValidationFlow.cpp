// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "WorkflowCommon.h"
#include "TestSettings.h"
#include "TestCommon.h"
#include <Workflows/UriValidationFlow.h>

using namespace TestCommon;
using namespace AppInstaller::CLI;
using namespace AppInstaller::CLI::Execution;
using namespace AppInstaller::CLI::Workflow;
using namespace AppInstaller::Manifest;
using namespace AppInstaller::Settings;

// Test uri with the following format: https://URI_VALIDATION/<zone>/<allow|block>
constexpr std::string_view InternetAllow = "https://URI_VALIDATION/zone3/allow"sv;
constexpr std::string_view LocalBlock = "https://URI_VALIDATION/zone0/block"sv;
constexpr std::string_view IntranetBlock = "https://URI_VALIDATION/zone1/block"sv;
constexpr std::string_view TrustedBlock = "https://URI_VALIDATION/zone2/block"sv;
constexpr std::string_view InternetBlock = "https://URI_VALIDATION/zone3/block"sv;
constexpr std::string_view UntrustedBlock = "https://URI_VALIDATION/zone4/block"sv;

#define SET_POLICY_STATE(_policy_, _state_) \
    GroupPolicyTestOverride policies; \
    policies.SetState(_policy_, _state_);

#define SET_ZONE_POLICY_STATE_AND_BLOCK_ZONE(_state_, _zone_) \
    SET_POLICY_STATE(TogglePolicy::Policy::AllowedSecurityZones, _state_); \
    std::map<SecurityZoneOptions, bool> securityZones; \
    securityZones[_zone_] = false; \
    policies.SetValue<ValuePolicy::AllowedSecurityZones>(securityZones); \

#define EXECUTE_CONTEXT_FOR_CONFIGURATION(_uri_) \
    std::ostringstream uriValidationOutput; \
    TestContext context{ uriValidationOutput, std::cin }; \
    context.Args.AddArg(Execution::Args::Type::ConfigurationFile, _uri_); \
    context << ExecuteUriValidation(UriValidationSource::Configuration); \
    INFO(uriValidationOutput.str());

#define EXECUTE_CONTEXT_FOR_PACKAGE_CATALOG_SOURCE(_uri_) \
    std::ostringstream uriValidationOutput; \
    TestContext context{ uriValidationOutput, std::cin }; \
    context.Add<Data::Manifest>(Manifest()); \
    auto source = std::make_shared<TestSource>(); \
    context.Add<Data::PackageVersion>(TestPackageVersion::Make(context.Get<Data::Manifest>(), source)); \
    ManifestInstaller installer; \
    installer.Url = _uri_; \
    context.Add<Data::Installer>(std::move(installer)); \
    context << ExecuteUriValidation(UriValidationSource::Package); \
    INFO(uriValidationOutput.str());

TEST_CASE("UriValidationFlow_Configuration_SecurityZonePolicy", "[UriValidationFlow][workflow]")
{
    SECTION("Not configured")
    {
        SET_POLICY_STATE(TogglePolicy::Policy::AllowedSecurityZones, PolicyState::NotConfigured);
        EXECUTE_CONTEXT_FOR_CONFIGURATION(InternetAllow);
        REQUIRE(S_OK == context.GetTerminationHR());
    }

    SECTION("Enabled")
    {
        SET_ZONE_POLICY_STATE_AND_BLOCK_ZONE(PolicyState::Enabled, SecurityZoneOptions::Internet);
        EXECUTE_CONTEXT_FOR_CONFIGURATION(InternetAllow);
        REQUIRE_TERMINATED_WITH(context, APPINSTALLER_CLI_ERROR_BLOCKED_BY_POLICY);
        REQUIRE_OUTPUT_HAS_LOC(uriValidationOutput, Resource::String::UriSecurityZoneBlockedByPolicy);
    }

    SECTION("Disabled")
    {
        SET_ZONE_POLICY_STATE_AND_BLOCK_ZONE(PolicyState::Disabled, SecurityZoneOptions::Internet);
        EXECUTE_CONTEXT_FOR_CONFIGURATION(InternetAllow);
        REQUIRE(S_OK == context.GetTerminationHR());
    }
}

TEST_CASE("UriValidationFlow_Configuration_SmartScreen", "[UriValidationFlow][workflow]")
{
    SECTION("Not configured")
    {
        SET_POLICY_STATE(TogglePolicy::Policy::BypassSmartScreenCheck, PolicyState::NotConfigured);
        EXECUTE_CONTEXT_FOR_CONFIGURATION(InternetBlock);
        REQUIRE_TERMINATED_WITH(context, APPINSTALLER_CLI_ERROR_BLOCKED_BY_REPUTATION_SERVICE);
        REQUIRE_OUTPUT_HAS_LOC(uriValidationOutput, Resource::String::UriBlockedBySmartScreen);
    }

    SECTION("Enabled")
    {
        SET_POLICY_STATE(TogglePolicy::Policy::BypassSmartScreenCheck, PolicyState::Enabled);
        EXECUTE_CONTEXT_FOR_CONFIGURATION(InternetBlock);
        REQUIRE_TERMINATED_WITH(context, APPINSTALLER_CLI_ERROR_BLOCKED_BY_REPUTATION_SERVICE);
        REQUIRE_OUTPUT_HAS_LOC(uriValidationOutput, Resource::String::UriBlockedBySmartScreen);
    }

    SECTION("Disabled")
    {
        SET_POLICY_STATE(TogglePolicy::Policy::BypassSmartScreenCheck, PolicyState::Disabled);
        EXECUTE_CONTEXT_FOR_CONFIGURATION(InternetBlock);
        REQUIRE(S_OK == context.GetTerminationHR());
    }
}

TEST_CASE("UriValidationFlow_PackageCatalogSource_SecurityZonePolicy", "[UriValidationFlow][workflow]")
{
    SECTION("Not configured")
    {
        SET_POLICY_STATE(TogglePolicy::Policy::AllowedSecurityZones, PolicyState::NotConfigured);
        EXECUTE_CONTEXT_FOR_PACKAGE_CATALOG_SOURCE(InternetAllow);
        REQUIRE(S_OK == context.GetTerminationHR());
    }

    SECTION("Enabled")
    {
        SET_ZONE_POLICY_STATE_AND_BLOCK_ZONE(PolicyState::Enabled, SecurityZoneOptions::Internet);
        EXECUTE_CONTEXT_FOR_PACKAGE_CATALOG_SOURCE(InternetAllow);
        REQUIRE_TERMINATED_WITH(context, APPINSTALLER_CLI_ERROR_BLOCKED_BY_POLICY);
        REQUIRE_OUTPUT_HAS_LOC(uriValidationOutput, Resource::String::UriSecurityZoneBlockedByPolicy);
    }

    SECTION("Disabled")
    {
        SET_ZONE_POLICY_STATE_AND_BLOCK_ZONE(PolicyState::Disabled, SecurityZoneOptions::Internet);
        EXECUTE_CONTEXT_FOR_PACKAGE_CATALOG_SOURCE(InternetAllow);
        REQUIRE(S_OK == context.GetTerminationHR());
    }
}

TEST_CASE("UriValidationFlow_PackageCatalogSource_SmartScreen", "[UriValidationFlow][workflow]")
{
    SECTION("Not configured")
    {
        SET_POLICY_STATE(TogglePolicy::Policy::BypassSmartScreenCheck, PolicyState::NotConfigured);
        EXECUTE_CONTEXT_FOR_PACKAGE_CATALOG_SOURCE(InternetBlock);
        REQUIRE_TERMINATED_WITH(context, APPINSTALLER_CLI_ERROR_BLOCKED_BY_REPUTATION_SERVICE);
        REQUIRE_OUTPUT_HAS_LOC(uriValidationOutput, Resource::String::UriBlockedBySmartScreen);
    }

    SECTION("Enabled")
    {
        SET_POLICY_STATE(TogglePolicy::Policy::BypassSmartScreenCheck, PolicyState::Enabled);
        EXECUTE_CONTEXT_FOR_PACKAGE_CATALOG_SOURCE(InternetBlock);
        REQUIRE_TERMINATED_WITH(context, APPINSTALLER_CLI_ERROR_BLOCKED_BY_REPUTATION_SERVICE);
        REQUIRE_OUTPUT_HAS_LOC(uriValidationOutput, Resource::String::UriBlockedBySmartScreen);
    }

    SECTION("Disabled")
    {
        SET_POLICY_STATE(TogglePolicy::Policy::BypassSmartScreenCheck, PolicyState::Disabled);
        EXECUTE_CONTEXT_FOR_PACKAGE_CATALOG_SOURCE(InternetBlock);
        REQUIRE(S_OK == context.GetTerminationHR());
    }
}

TEST_CASE("UriValidationFlow_SmartScreenZoneRequirement", "[UriValidationFlow][workflow]")
{
    // Smart screen should only be evaluated for Internet and Untrusted zones.
    SECTION("Local")
    {
        SET_POLICY_STATE(TogglePolicy::Policy::BypassSmartScreenCheck, PolicyState::Enabled);
        EXECUTE_CONTEXT_FOR_CONFIGURATION(LocalBlock);
        REQUIRE(S_OK == context.GetTerminationHR());
    }

    SECTION("Intranet")
    {
        SET_POLICY_STATE(TogglePolicy::Policy::BypassSmartScreenCheck, PolicyState::Enabled);
        EXECUTE_CONTEXT_FOR_CONFIGURATION(IntranetBlock);
        REQUIRE(S_OK == context.GetTerminationHR());
    }

    SECTION("Trusted")
    {
        SET_POLICY_STATE(TogglePolicy::Policy::BypassSmartScreenCheck, PolicyState::Enabled);
        EXECUTE_CONTEXT_FOR_CONFIGURATION(TrustedBlock);
        REQUIRE(S_OK == context.GetTerminationHR());
    }

    SECTION("Internet")
    {
        SET_POLICY_STATE(TogglePolicy::Policy::BypassSmartScreenCheck, PolicyState::Enabled);
        EXECUTE_CONTEXT_FOR_CONFIGURATION(InternetBlock);
        REQUIRE_TERMINATED_WITH(context, APPINSTALLER_CLI_ERROR_BLOCKED_BY_REPUTATION_SERVICE);
        REQUIRE_OUTPUT_HAS_LOC(uriValidationOutput, Resource::String::UriBlockedBySmartScreen);
    }

    SECTION("Untrusted")
    {
        SET_POLICY_STATE(TogglePolicy::Policy::BypassSmartScreenCheck, PolicyState::Enabled);
        EXECUTE_CONTEXT_FOR_CONFIGURATION(UntrustedBlock);
        REQUIRE_TERMINATED_WITH(context, APPINSTALLER_CLI_ERROR_BLOCKED_BY_REPUTATION_SERVICE);
        REQUIRE_OUTPUT_HAS_LOC(uriValidationOutput, Resource::String::UriBlockedBySmartScreen);
    }
}

TEST_CASE("UriValidationFlow_BlockedSecurityZoneDoesNotImpactOtherSecurityZones", "[UriValidationFlow][workflow]")
{
    SET_ZONE_POLICY_STATE_AND_BLOCK_ZONE(PolicyState::Enabled, SecurityZoneOptions::Internet);
    EXECUTE_CONTEXT_FOR_CONFIGURATION(UntrustedBlock);
    REQUIRE_TERMINATED_WITH(context, APPINSTALLER_CLI_ERROR_BLOCKED_BY_REPUTATION_SERVICE);
    REQUIRE_OUTPUT_HAS_LOC(uriValidationOutput, Resource::String::UriBlockedBySmartScreen);
}
