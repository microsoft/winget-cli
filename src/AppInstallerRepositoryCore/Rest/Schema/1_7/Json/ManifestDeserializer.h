// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#pragma once
#include "Rest/Schema/1_6/Json/ManifestDeserializer.h"

namespace AppInstaller::Repository::Rest::Schema::V1_7::Json
{
    // Manifest Deserializer.
    struct ManifestDeserializer : public V1_6::Json::ManifestDeserializer
    {
    protected:

        std::optional<Manifest::ManifestInstaller> DeserializeInstaller(const web::json::value& installerJsonObject) const override;

        std::map<Manifest::InstallerSwitchType, Manifest::string_t> DeserializeInstallerSwitches(const web::json::value& installerSwitchesJsonObject) const override;

        Manifest::ManifestVer GetManifestVersion() const override;
    };
}
