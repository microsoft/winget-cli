// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#pragma once
#include "ManifestCommon.h"
#include "ManifestValidation.h"

#include <json.h>

namespace AppInstaller::Manifest::YamlParser
{
    // Forward declarations
    struct YamlManifestInfo;

    // Load an embedded resource from binary and return as std::string
    std::string LoadResourceAsString(PCWSTR resourceModuleName, PCWSTR resourceName, PCWSTR resourceType);

    // Load manifest schema as parsed json doc
    Json::Value LoadSchemaDoc(const ManifestVer& manifestVersion, ManifestTypeEnum manifestType, PCWSTR resourceModuleName);

    // resourceModuleName is the binary name where the schemas are embedded, or nullptr indicating the binary that created the process
    std::vector<ValidationError> ValidateAgainstSchema(
        const std::vector<YamlManifestInfo>& manifestList,
        const ManifestVer& manifestVersion,
        PCWSTR resourceModuleName);
}