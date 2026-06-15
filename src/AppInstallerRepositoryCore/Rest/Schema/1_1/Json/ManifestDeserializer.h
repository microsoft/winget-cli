// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#pragma once
#include "Rest/Schema/1_0/Json/ManifestDeserializer.h"

namespace AppInstaller::Repository::Rest::Schema::V1_1::Json
{
    // Manifest Deserializer.
    struct ManifestDeserializer : public V1_0::Json::ManifestDeserializer
    {
        std::vector<Manifest::AppsAndFeaturesEntry> DeserializeAppsAndFeaturesEntries(const web::json::array& entries) const override;

        std::optional<Manifest::ManifestLocalization> DeserializeLocale(const web::json::value& localeJsonObject) const override;

    protected:
        std::optional<Manifest::ManifestInstaller> DeserializeInstaller(const web::json::value& installerJsonObject) const override;

        Manifest::InstallerTypeEnum ConvertToInstallerType(std::string_view in) const override;

        virtual Manifest::ExpectedReturnCodeEnum ConvertToExpectedReturnCodeEnum(std::string_view in) const;

        virtual Manifest::ManifestInstaller::ExpectedReturnCodeInfo DeserializeExpectedReturnCodeInfo(const web::json::value& expectedReturnCodeJsonObject) const;

        Manifest::ManifestVer GetManifestVersion() const override;
    };
}
