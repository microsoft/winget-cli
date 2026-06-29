// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "ManifestDeserializer.h"
#include <winget/JsonUtil.h>

using namespace AppInstaller::Manifest;

namespace AppInstaller::Repository::Rest::Schema::V1_30::Json
{
    namespace
    {
        constexpr std::string_view InstallerAvailabilityMessage = "InstallerAvailabilityMessage"sv;
    }

    std::optional<Manifest::ManifestInstaller> ManifestDeserializer::DeserializeInstaller(const web::json::value& installerJsonObject) const
    {
        auto result = V1_28::Json::ManifestDeserializer::DeserializeInstaller(installerJsonObject);

        if (result)
        {
            result->InstallerAvailabilityMessage = JSON::GetRawStringValueFromJsonNode(installerJsonObject, JSON::GetUtilityString(InstallerAvailabilityMessage)).value_or("");
        }

        return result;
    }

    Manifest::InstallerTypeEnum ManifestDeserializer::ConvertToInstallerType(std::string_view in) const
    {
        std::string inStrLower = Utility::ToLower(in);

        if (inStrLower == "noinstaller")
        {
            return InstallerTypeEnum::NoInstaller;
        }

        return V1_28::Json::ManifestDeserializer::ConvertToInstallerType(inStrLower);
    }

    Manifest::ManifestVer ManifestDeserializer::GetManifestVersion() const
    {
        return Manifest::s_ManifestVersionV1_30;
    }
}
