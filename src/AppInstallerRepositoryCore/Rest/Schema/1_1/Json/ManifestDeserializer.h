// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#pragma once
#include "Rest/Schema/1_0/Json/ManifestDeserializer.h"

namespace AppInstaller::Repository::Rest::Schema::V1_1::Json
{
    // Manifest Deserializer.
    struct ManifestDeserializer : public V1_0::Json::ManifestDeserializer
    {
        // TODO: override DeserializeLocale, DeserializeInstaller accordingly to add new v1.1 fields
    };
}
