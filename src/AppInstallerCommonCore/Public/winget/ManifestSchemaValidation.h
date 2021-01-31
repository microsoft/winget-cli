// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#pragma once
#include "ManifestCommon.h"

#include <json.h>

namespace AppInstaller::Manifest::YamlParser
{
    // Forward declarations
    struct YamlManifestInfo;

    std::string LoadResourceAsString(PCWSTR resourceModuleName, PCWSTR resourceName, PCWSTR resourceType);

    Json::Value LoadSchemaDoc(const ManifestVer& manifestVersion, ManifestTypeEnum manifestType, PCWSTR resourceModuleName);

    std::vector<ValidationError> ValidateAgainstSchema(
        const std::vector<YamlManifestInfo> manifestList,
        const ManifestVer& manifestVersion,
        PCWSTR resourceModuleName);
}