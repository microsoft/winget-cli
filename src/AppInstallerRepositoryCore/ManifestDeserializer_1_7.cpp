// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "ManifestDeserializer.h"
#include <winget/JsonUtil.h>

using namespace AppInstaller::Manifest;

namespace AppInstaller::Repository::Rest::Schema::V1_7::Json
{
    namespace
    {
        // Installer switches
        constexpr std::string_view InstallerSwitches = "InstallerSwitches"sv;
        constexpr std::string_view ScopeUser = "ScopeUser"sv;
        constexpr std::string_view ScopeMachine = "Custom"sv;

        void TryParseInstallerSwitchField(
            std::map<InstallerSwitchType, Utility::NormalizedString>& installerSwitches,
            InstallerSwitchType switchType,
            const web::json::value& switchesJsonObject,
            std::string_view switchJsonFieldName)
        {
            auto value = JSON::GetRawStringValueFromJsonNode(switchesJsonObject, JSON::GetUtilityString(switchJsonFieldName));

            if (JSON::IsValidNonEmptyStringValue(value))
            {
                installerSwitches[switchType] = value.value();
            }
        }
    }

    std::optional<Manifest::ManifestInstaller> ManifestDeserializer::DeserializeInstaller(const web::json::value& installerJsonObject) const
    {
        auto result = V1_6::Json::ManifestDeserializer::DeserializeInstaller(installerJsonObject);

        if (result)
        {
            auto& installer = result.value();

            // Installer should already have the installer.Switches field from the V1_6 Deserialization
            // Switches from V1_6 should have already been parsed
            std::optional<std::reference_wrapper<const web::json::value>> switches =
                JSON::GetJsonValueFromNode(installerJsonObject, JSON::GetUtilityString(InstallerSwitches));
            if (switches)
            {
                const auto& installerSwitches = switches.value().get();
                TryParseInstallerSwitchField(installer.Switches, InstallerSwitchType::ScopeUser, installerSwitches, ScopeUser);
                TryParseInstallerSwitchField(installer.Switches, InstallerSwitchType::ScopeMachine, installerSwitches, ScopeMachine);
            }
        }

        return result;
    }

    Manifest::ManifestVer ManifestDeserializer::GetManifestVersion() const
    {
        return Manifest::s_ManifestVersionV1_7;
    }
}
