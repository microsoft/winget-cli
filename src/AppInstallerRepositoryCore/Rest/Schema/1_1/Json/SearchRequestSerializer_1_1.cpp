// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "SearchRequestSerializer.h"

namespace AppInstaller::Repository::Rest::Schema::V1_1::Json
{
    std::optional<std::string_view> SearchRequestSerializer::ConvertPackageMatchFieldToString(AppInstaller::Repository::PackageMatchField field) const
    {
        if (field == PackageMatchField::Market)
        {
            return "Market"sv;
        }

        return V1_0::Json::SearchRequestSerializer::ConvertPackageMatchFieldToString(field);
    }
}
