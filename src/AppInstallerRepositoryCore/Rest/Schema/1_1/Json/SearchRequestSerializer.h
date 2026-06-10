// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#pragma once
#include <cpprest/json.h>
#include "Rest/Schema/1_0/Json/SearchRequestSerializer.h"

namespace AppInstaller::Repository::Rest::Schema::V1_1::Json
{
    // Search Result Serializer.
    struct SearchRequestSerializer : public V1_0::Json::SearchRequestSerializer
    {
    protected:
        std::optional<std::string_view> ConvertPackageMatchFieldToString(AppInstaller::Repository::PackageMatchField field) const override;
    };
}
