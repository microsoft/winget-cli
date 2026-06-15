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

        auto repairValue = JSON::GetRawStringValueFromJsonNode(installerSwitchesJsonObject, JSON::GetUtilityString(InstallerSwitchRepair));

        if (JSON::IsValidNonEmptyStringValue(repairValue))
        {
            installerSwitches[Manifest::InstallerSwitchType::Repair] = repairValue.value();
        }

        return installerSwitches;
    }

    Manifest::ManifestVer ManifestDeserializer::GetManifestVersion() const
    {
        return Manifest::s_ManifestVersionV1_7;
    }
}
