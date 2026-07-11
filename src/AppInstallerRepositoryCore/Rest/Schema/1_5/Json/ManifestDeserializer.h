// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#pragma once
#include "Rest/Schema/1_4/Json/ManifestDeserializer.h"

namespace AppInstaller::Repository::Rest::Schema::V1_5::Json
{
    // Manifest Deserializer.
    struct ManifestDeserializer : public V1_4::Json::ManifestDeserializer
    {
        std::optional<Manifest::ManifestLocalization> DeserializeLocale(const web::json::value& localeJsonObject) const override;

    protected:

        Manifest::ManifestVer GetManifestVersion() const override;
    };
}
