// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#pragma once
#include "Rest/Schema/1_12/Json/ManifestDeserializer.h"

namespace AppInstaller::Repository::Rest::Schema::V1_28::Json
{
    // Manifest Deserializer.
    struct ManifestDeserializer : public V1_12::Json::ManifestDeserializer
    {
    protected:

        std::optional<Manifest::ManifestInstaller> DeserializeInstaller(const web::json::value& installerJsonObject) const override;

        Manifest::ManifestVer GetManifestVersion() const override;
    };
}
