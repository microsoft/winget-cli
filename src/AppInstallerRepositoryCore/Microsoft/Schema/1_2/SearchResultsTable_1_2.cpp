// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#pragma once
#include "pch.h"
#include "SearchResultsTable.h"

#include "Microsoft/Schema/1_0/ManifestTable.h"
#include "Microsoft/Schema/1_2/NormalizedPackageNameTable.h"
#include "Microsoft/Schema/1_2/NormalizedPackagePublisherTable.h"


namespace AppInstaller::Repository::Microsoft::Schema::V1_2
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
        case PackageMatchField::NormalizedNameAndPublisher:
            return V1_0::ManifestTable::BuildSearchStatement<NormalizedPackageNameTable, NormalizedPackagePublisherTable>(builder, manifestAlias, valueAlias, useLike);
        default:
            return V1_1::SearchResultsTable::BuildSearchStatement(builder, field, manifestAlias, valueAlias, useLike);
        }
    }

    void SearchResultsTable::BindStatementForMatchType(SQLite::Statement& statement, const PackageMatchFilter& filter, const std::vector<int>& bindIndex)
    {
        V1_0::SearchResultsTable::BindStatementForMatchType(statement, filter, bindIndex);

        if (filter.Field == PackageMatchField::NormalizedNameAndPublisher)
        {
            BindStatementForMatchType(statement, filter.Type, bindIndex[1], filter.Additional.value());
        }
    }
}
