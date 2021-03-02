// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#pragma once
#include "pch.h"
#include "SearchResultsTable.h"

#include "Microsoft/Schema/1_0/ManifestTable.h"
#include "Microsoft/Schema/1_1/PackageFamilyNameTable.h"
#include "Microsoft/Schema/1_1/ProductCodeTable.h"


namespace AppInstaller::Repository::Microsoft::Schema::V1_1
{
    std::vector<int> SearchResultsTable::BuildSearchStatement(
        SQLite::Builder::StatementBuilder& builder,
        PackageMatchField field,
        std::string_view manifestAlias,
        std::string_view valueAlias,
        bool useLike) const
    {
        switch (field)
        {
        case PackageMatchField::PackageFamilyName:
            return V1_0::ManifestTable::BuildSearchStatement<PackageFamilyNameTable>(builder, manifestAlias, valueAlias, useLike);
        case PackageMatchField::ProductCode:
            return V1_0::ManifestTable::BuildSearchStatement<ProductCodeTable>(builder, manifestAlias, valueAlias, useLike);
        default:
            return V1_0::SearchResultsTable::BuildSearchStatement(builder, field, manifestAlias, valueAlias, useLike);
        }
    }
}
