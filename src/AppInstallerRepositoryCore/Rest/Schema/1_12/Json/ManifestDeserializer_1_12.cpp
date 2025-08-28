// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "ManifestDeserializer.h"
#include "Rest/Schema/AuthenticationInfoParser.h"
#include <winget/JsonUtil.h>

using namespace AppInstaller::Manifest;

namespace AppInstaller::Repository::Rest::Schema::V1_12::Json
{
    Manifest::InstallerTypeEnum ManifestDeserializer::ConvertToInstallerType(std::string_view in) const
    {
        std::string inStrLower = Utility::ToLower(in);

        if (inStrLower == "font")
        {
            return InstallerTypeEnum::Font;
        }

        return V1_10::Json::ManifestDeserializer::ConvertToInstallerType(inStrLower);
    }

    Manifest::ManifestVer ManifestDeserializer::GetManifestVersion() const
    {
        return Manifest::s_ManifestVersionV1_12;
    }
}
