// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "ManifestDeserializer.h"
#include "Rest/Schema/AuthenticationInfoParser.h"
#include <winget/JsonUtil.h>

using namespace AppInstaller::Manifest;

namespace AppInstaller::Repository::Rest::Schema::V1_10::Json
{
    std::optional<Manifest::ManifestInstaller> ManifestDeserializer::DeserializeInstaller(const web::json::value& installerJsonObject) const
    {
        auto result = V1_9::Json::ManifestDeserializer::DeserializeInstaller(installerJsonObject);

        if (result)
        {
            auto& installer = result.value();

            installer.AuthInfo = ParseAuthenticationInfo(installerJsonObject, ParseAuthenticationInfoType::Installer, GetManifestVersion());
        }

        return result;
    }

    Manifest::ManifestVer ManifestDeserializer::GetManifestVersion() const
    {
        return Manifest::s_ManifestVersionV1_10;
    }
}
