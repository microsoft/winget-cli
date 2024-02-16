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
        // Installer
        constexpr std::string_view InstallerSwitchRepair = "Repair"sv;
        constexpr std::string_view RepairBehavior = "RepairBehavior"sv;
        constexpr std::string_view ScopeUser = "ScopeUser"sv;
        constexpr std::string_view ScopeMachine = "ScopeMachine"sv;
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

            installer.RepairBehavior = Manifest::ConvertToRepairBehaviorEnum(JSON::GetRawStringValueFromJsonNode(installerJsonObject, JSON::GetUtilityString(RepairBehavior)).value_or(""));
        }

        return result;
    }

    std::map<Manifest::InstallerSwitchType, Manifest::string_t> ManifestDeserializer::DeserializeInstallerSwitches(const web::json::value& installerSwitchesJsonObject) const
    {
        auto installerSwitches = V1_6::Json::ManifestDeserializer::DeserializeInstallerSwitches(installerSwitchesJsonObject);

        TryParseInstallerSwitchField(installerSwitches, InstallerSwitchType::Repair, installerSwitchesJsonObject, InstallerSwitchRepair);
        TryParseInstallerSwitchField(installerSwitches, InstallerSwitchType::ScopeUser, installerSwitchesJsonObject, ScopeUser);
        TryParseInstallerSwitchField(installerSwitches, InstallerSwitchType::ScopeMachine, installerSwitchesJsonObject, ScopeMachine);

        return installerSwitches;
    }

    Manifest::ManifestVer ManifestDeserializer::GetManifestVersion() const
    {
        return Manifest::s_ManifestVersionV1_7;
    }
}
