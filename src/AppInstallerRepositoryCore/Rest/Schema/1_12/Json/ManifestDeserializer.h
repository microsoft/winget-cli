// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#pragma once
#include "Rest/Schema/1_10/Json/ManifestDeserializer.h"

namespace AppInstaller::Repository::Rest::Schema::V1_12::Json
{
    // Manifest Deserializer.
    struct ManifestDeserializer : public V1_10::Json::ManifestDeserializer
    {
    protected:

        virtual std::map<Manifest::UninstallerSwitchType, Manifest::string_t> DeserializeUninstallerSwitches(const web::json::value& uninstallerSwitchesJsonObject) const;

        std::optional<Manifest::ManifestInstaller> DeserializeInstaller(const web::json::value& installerJsonObject) const override;

        Manifest::InstallerTypeEnum ConvertToInstallerType(std::string_view in) const override;

        Manifest::ManifestVer GetManifestVersion() const override;
    };
}
