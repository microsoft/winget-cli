// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "PinTable.h"
#include "SQLiteStatementBuilder.h"
#include "Microsoft/Schema/IPinningIndex.h"

namespace AppInstaller::Repository::Microsoft::Schema::Pinning_V1_0
{
    namespace
    {
        std::optional<Pinning::Pin> GetPinFromRow(std::string_view packageId, std::string_view sourceId, Pinning::PinType type, std::string_view gatedVersion)
        {
            switch (type)
            {
            case Pinning::PinType::Blocking:
                return Pinning::Pin::CreateBlockingPin({ packageId, sourceId });
            case Pinning::PinType::Pinning:
                return Pinning::Pin::CreatePinningPin({ packageId, sourceId });
            case Pinning::PinType::Gating:
                return Pinning::Pin::CreateGatingPin({ packageId, sourceId }, Utility::GatedVersion{ gatedVersion });
            default:
                return {}
            }
        }
    }

    using namespace std::string_view_literals;
    static constexpr std::string_view s_PinTable_Table_Name = "pin"sv;
    static constexpr std::string_view s_PinTable_PackageId_Column = "packageId"sv;
    static constexpr std::string_view s_PinTable_SourceId_Column = "sourceId"sv;
    static constexpr std::string_view s_PinTable_Type_Column = "type"sv;
    static constexpr std::string_view s_PinTable_GatedVersion_Column = "gatedVersion"sv;
    static constexpr std::string_view s_PinTable_Index = "pinIndex"sv;

    std::string_view PinTable::TableName()
    {
        return s_PinTable_Table_Name;
    }

    void PinTable::Create(SQLite::Connection& connection)
    {
        using namespace SQLite::Builder;

        SQLite::Savepoint savepoint = SQLite::Savepoint::Create(connection, "createPinTable_v1_0");

        StatementBuilder createTableBuilder;
        createTableBuilder.CreateTable(s_PinTable_Table_Name).BeginColumns();

        createTableBuilder.Column(ColumnBuilder(s_PinTable_PackageId_Column, Type::Text).NotNull());
        createTableBuilder.Column(ColumnBuilder(s_PinTable_SourceId_Column, Type::Text).NotNull());
        createTableBuilder.Column(ColumnBuilder(s_PinTable_Type_Column, Type::Int64).NotNull());
        createTableBuilder.Column(ColumnBuilder(s_PinTable_GatedVersion_Column, Type::Text));

        createTableBuilder.EndColumns();
        createTableBuilder.Execute(connection);

        // Create an index over the pairs package,source
        StatementBuilder createIndexBuilder;
        createIndexBuilder.CreateUniqueIndex(s_PinTable_Index).On(s_PinTable_Table_Name).Columns({ s_PinTable_PackageId_Column, s_PinTable_SourceId_Column });
        createIndexBuilder.Execute(connection);

        savepoint.Commit();
    }

    std::optional<SQLite::rowid_t> PinTable::SelectByPinKey(SQLite::Connection& connection, const Pinning::PinKey& pinKey)
    {
        SQLite::Builder::StatementBuilder builder;
        builder.Select(SQLite::RowIDName).From(s_PinTable_Table_Name)
            .Where(s_PinTable_PackageId_Column).Equals(pinKey.PackageId)
            .And(s_PinTable_SourceId_Column).Equals(pinKey.SourceId);

        SQLite::Statement select = builder.Prepare(connection);

        if (select.Step())
        {
            return select.GetColumn<SQLite::rowid_t>(0);
        }
        else
        {
            return {};
        }
    }

    SQLite::rowid_t PinTable::AddPin(SQLite::Connection& connection, const Pinning::Pin& pin)
    {
        SQLite::Builder::StatementBuilder builder;
        builder.InsertInto(s_PinTable_Table_Name)
            .Columns({
                s_PinTable_PackageId_Column,
                s_PinTable_SourceId_Column,
                s_PinTable_Type_Column,
                s_PinTable_GatedVersion_Column })
            .Values(
                pin.GetPackageId(),
                pin.GetSourceId(),
                pin.GetType(),
                pin.GetType() == Pinning::PinType::Gating ? pin.GetGatedVersion().ToString() : "");

        builder.Execute(connection);
        return connection.GetLastInsertRowID();

    }

    SQLite::rowid_t PinTable::RemovePinById(SQLite::Connection& connection, SQLite::rowid_t pinId)
    {
        SQLite::Builder::StatementBuilder builder;
        builder.DeleteFrom(s_PinTable_Table_Name).Where(SQLite::RowIDName).Equals(pinId);
        builder.Execute(connection);
    }

    std::optional<Pinning::Pin> PinTable::GetPinById(SQLite::Connection& connection, const SQLite::rowid_t pinId)
    {
        SQLite::Builder::StatementBuilder builder;
        builder.Select({
            s_PinTable_PackageId_Column,
            s_PinTable_SourceId_Column,
            s_PinTable_Type_Column,
            s_PinTable_GatedVersion_Column })
            .From(s_PinTable_Table_Name).Where(SQLite::RowIDName).Equals(pinId);

        SQLite::Statement select = builder.Prepare(connection);

        if (!select.Step())
        {
            return {};
        }

        auto [packageId, sourceId, type, gatedVersion] = select.GetRow<std::string, std::string, Pinning::PinType, std::string>();
        return GetPinFromRow(packageId, sourceId, type, gatedVersion);
    }

    std::vector<Pinning::Pin> PinTable::GetAllPins(SQLite::Connection& connection)
    {
        SQLite::Builder::StatementBuilder builder;
        builder.Select({
            s_PinTable_PackageId_Column,
            s_PinTable_SourceId_Column,
            s_PinTable_Type_Column,
            s_PinTable_GatedVersion_Column })
            .From(s_PinTable_Table_Name);

        SQLite::Statement select = builder.Prepare(connection);

        std::vector<Pinning::Pin> pins;
        while (select.Step())
        {
            auto [packageId, sourceId, type, gatedVersion] = select.GetRow<std::string, std::string, Pinning::PinType, std::string>();
            auto pin = GetPinFromRow(packageId, sourceId, type, gatedVersion);
            if (pin) {
                pins.push_back(std::move(pin.value()));
            }
        }

        return pins;
    }
}