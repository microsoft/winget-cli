// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#pragma once
#include "ManifestCommon.h"
#include "ManifestValidation.h"

#include <json/json.h>

namespace AppInstaller::Manifest::YamlParser
{
    // Forward declarations
    struct YamlManifestInfo;

    // Load manifest schema as parsed json doc
    Json::Value LoadSchemaDoc(const ManifestVer& manifestVersion, ManifestTypeEnum manifestType);

    // Validate a list of individual manifests against schema
    std::vector<ValidationError> ValidateAgainstSchema(
        const std::vector<YamlManifestInfo>& manifestList,
        const ManifestVer& manifestVersion);

    // Validate the schema header of a list of manifests
    std::vector<ValidationError> ValidateYamlManifestsSchemaHeader(
        const std::vector<YamlManifestInfo>& manifestList,
        const ManifestVer& manifestVersion,
        bool treatErrorAsWarning = true);
}
