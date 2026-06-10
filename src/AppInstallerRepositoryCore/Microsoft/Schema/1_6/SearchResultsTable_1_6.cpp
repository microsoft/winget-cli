// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "SearchResultsTable.h"

#include "Microsoft/Schema/1_0/ManifestTable.h"
#include "Microsoft/Schema/1_6/UpgradeCodeTable.h"


namespace AppInstaller::Repository::Microsoft::Schema::V1_6
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
        case PackageMatchField::UpgradeCode:
            return V1_0::ManifestTable::BuildSearchStatement<UpgradeCodeTable>(builder, manifestAlias, valueAlias, useLike);
        default:
            return V1_2::SearchResultsTable::BuildSearchStatement(builder, field, manifestAlias, valueAlias, useLike);
        }
    }
}
