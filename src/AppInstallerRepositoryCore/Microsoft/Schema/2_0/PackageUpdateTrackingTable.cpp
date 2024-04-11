// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "PackageUpdateTrackingTable.h"
#include <winget/SQLiteStatementBuilder.h>

using namespace AppInstaller::SQLite;

namespace AppInstaller::Repository::Microsoft::Schema::V2_0
{
    using namespace std::string_view_literals;
    static constexpr std::string_view s_PUTT_Table_Name = "update_tracking"sv;
    static constexpr std::string_view s_PUTT_WriteTimeIndex_Name = "update_tracking_write_idx"sv;
    static constexpr std::string_view s_PUTT_Package = "package"sv;
    static constexpr std::string_view s_PUTT_WriteTime = "writetime"sv;
    static constexpr std::string_view s_PUTT_Manifest = "manifest"sv;
    static constexpr std::string_view s_PUTT_Hash = "hash"sv;

    std::string_view PackageUpdateTrackingTable::TableName()
    {
        return s_PUTT_Table_Name;
    }

    void PackageUpdateTrackingTable::Create(SQLite::Connection& connection)
    {
        using namespace Builder;

        StatementBuilder builder;
        builder.CreateTable(s_PUTT_Table_Name).BeginColumns();

        builder.Column(IntegerPrimaryKey());
        builder.Column(ColumnBuilder(s_PUTT_Package, Type::Text).NotNull());
        builder.Column(ColumnBuilder(s_PUTT_WriteTime, Type::Int64).NotNull());
        builder.Column(ColumnBuilder(s_PUTT_Manifest, Type::Text).NotNull());
        builder.Column(ColumnBuilder(s_PUTT_Hash, Type::Blob).NotNull());

        builder.EndColumns();

        builder.Execute(connection);

        StatementBuilder indexBuilder;
        indexBuilder.CreateIndex(s_PUTT_WriteTimeIndex_Name).On(s_PUTT_Table_Name).Columns(s_PUTT_WriteTime);
        indexBuilder.Execute(connection);
    }

    void PackageUpdateTrackingTable::EnsureExists(SQLite::Connection& connection)
    {
        if (!Exists(connection))
        {
            Create(connection);
        }
    }

    void PackageUpdateTrackingTable::Drop(SQLite::Connection& connection)
    {
        Builder::StatementBuilder dropTableBuilder;
        dropTableBuilder.DropTable(s_PUTT_Table_Name);

        dropTableBuilder.Execute(connection);
    }

    bool PackageUpdateTrackingTable::Exists(const SQLite::Connection& connection)
    {
        Builder::StatementBuilder builder;
        builder.Select(Builder::RowCount).From(Builder::Schema::MainTable).
            Where(Builder::Schema::TypeColumn).Equals(Builder::Schema::Type_Table).And(Builder::Schema::NameColumn).Equals(s_PUTT_Table_Name);

        Statement statement = builder.Prepare(connection);
        THROW_HR_IF(E_UNEXPECTED, !statement.Step());
        return statement.GetColumn<int64_t>(0) != 0;
    }

    void PackageUpdateTrackingTable::Update(SQLite::Connection& connection, ISQLiteIndex* internalIndex, const std::string& packageIdentifier)
    {
        // TODO: Pull the data on the given package and update the table
    }

    bool PackageUpdateTrackingTable::CheckConsistency(const SQLite::Connection& connection, ISQLiteIndex* internalIndex, bool log)
    {
        // Ensure that all data in the update table matches the internal index
        for (const PackageData& packageData : GetUpdatesSince(connection, 0))
        {
            auto manifestHash = Utility::SHA256::ComputeHash(packageData.Manifest);
            if (!Utility::SHA256::AreEqual(packageData.Hash, manifestHash))
            {
                if (!log)
                {
                    return false;
                }

                AICLI_LOG(Repo, Info, << "  [INVALID] value [" << s_PUTT_Hash << "] in table [" << s_PUTT_Table_Name <<
                    "] at row [" << packageData.RowID << "]; the hash of the manifest value does not match the hash in the row");
            }

            SearchRequest request;
            request.Inclusions.emplace_back(PackageMatchField::Id, MatchType::CaseInsensitive, packageData.PackageIdentifier);

            if (internalIndex->Search(connection, request).Matches.empty())
            {
                if (!log)
                {
                    return false;
                }

                AICLI_LOG(Repo, Info, << "  [INVALID] value [" << s_PUTT_Package << "] in table [" << s_PUTT_Table_Name <<
                    "] at row [" << packageData.RowID << "]; the package [" << packageData.PackageIdentifier << "] was not found in the internal index");
            }
        }

        // Ensure that all packages in the internal index are present in the update table
        // TODO:::::
    }

    std::vector<PackageUpdateTrackingTable::PackageData> PackageUpdateTrackingTable::GetUpdatesSince(const SQLite::Connection& connection, int64_t updateBaseTime)
    {
        Builder::StatementBuilder builder;
        builder.Select({ RowIDName, s_PUTT_Package, s_PUTT_WriteTime, s_PUTT_Manifest, s_PUTT_Hash }).
            From(s_PUTT_Table_Name).Where(s_PUTT_WriteTime).IsGreaterThan(updateBaseTime);

        Statement select = builder.Prepare(connection);

        std::vector<PackageData> result;

        while (select.Step())
        {
            PackageData item;
            item.RowID = select.GetColumn<rowid_t>(0);
            item.PackageIdentifier = select.GetColumn<std::string>(1);
            item.WriteTime = select.GetColumn<int64_t>(2);
            item.Manifest = select.GetColumn<std::string>(3);
            item.Hash = select.GetColumn<blob_t>(4);

            result.emplace_back(std::move(item));
        }

        return result;
    }

    SQLite::blob_t PackageUpdateTrackingTable::GetDataHash(const SQLite::Connection& connection, const std::string& packageIdentifier)
    {
        Builder::StatementBuilder builder;
        builder.Select(s_PUTT_Hash).From(s_PUTT_Table_Name).Where(s_PUTT_Package).LikeWithEscape(packageIdentifier);

        Statement select = builder.Prepare(connection);

        THROW_HR_IF(E_NOT_SET, !select.Step());

        return select.GetColumn<SQLite::blob_t>(0);
    }
}
