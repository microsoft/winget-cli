// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "ManifestTypes.h"

namespace AppInstaller::Manifest
{
    struct YamlManifestInfo
    {
        YAML::Node Root;
        std::string FileName;
        ManifestTypeEnum ManifestType;
    };

    std::vector<ValidationError> ValidateAgainstSchema(const std::vector<YamlManifestInfo> manifestList, const ManifestVer& manifestVersion, PCWSTR resourceModuleName);
}