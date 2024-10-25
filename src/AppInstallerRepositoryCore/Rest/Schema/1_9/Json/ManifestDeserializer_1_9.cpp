// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "ManifestDeserializer.h"
#include <winget/JsonUtil.h>

using namespace AppInstaller::Manifest;

namespace AppInstaller::Repository::Rest::Schema::V1_9::Json
{
    namespace
    {
        // Installer
        constexpr std::string_view ArchiveBinariesDependOnPath = "ArchiveBinariesDependOnPath"sv;
    }

    std::optional<Manifest::ManifestInstaller> ManifestDeserializer::DeserializeInstaller(const web::json::value& installerJsonObject) const
    {
        auto result = V1_7::Json::ManifestDeserializer::DeserializeInstaller(installerJsonObject);

        if (result)
        {
            auto& installer = result.value();

            installer.ArchiveBinariesDependOnPath = JSON::GetRawBoolValueFromJsonNode(installerJsonObject, JSON::GetUtilityString(ArchiveBinariesDependOnPath)).value_or(false);
        }

        return result;
    }

    Manifest::ManifestVer ManifestDeserializer::GetManifestVersion() const
    {
        return Manifest::s_ManifestVersionV1_9;
    }
}
