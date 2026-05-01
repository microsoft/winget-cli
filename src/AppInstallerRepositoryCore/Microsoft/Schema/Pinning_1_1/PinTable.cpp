// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "PinTable.h"
#include <AppInstallerDateTime.h>
#include <winget/SQLiteStatementBuilder.h>
#include "Microsoft/Schema/IPinningIndex.h"

namespace AppInstaller::Repository::Microsoft::Schema::Pinning_V1_1
{
    namespace
    {
        using PinRow = std::tuple<std::string, std::string, Pinning::PinType, std::string, std::optional<int64_t>, std::optional<std::string>>;

        std::optional<Pinning::Pin> GetPinFromRow(PinRow&& row)
        {
            auto [packageId, sourceId, type, version, epochOpt, note] = std::move(row);

            std::optional<Pinning::Pin> result;

            Pinning::PinKey key;
            key.PackageId = std::move(packageId);
            key.SourceId = std::move(sourceId);

            switch (type)
            {
            case Pinning::PinType::Blocking:
                result = Pinning::Pin::CreateBlockingPin(std::move(key));
                break;
            case Pinning::PinType::Pinning:
                result = Pinning::Pin::CreatePinningPin(std::move(key));
                break;
            case Pinning::PinType::Gating:
                result = Pinning::Pin::CreateGatingPin(std::move(key), Utility::GatedVersion{ std::move(version) });
                break;
            default:
                return {};
            }

            std::optional<std::chrono::system_clock::time_point> dateAdded;
            if (epochOpt.has_value())
            {
                dateAdded = Utility::ConvertUnixEpochToSystemClock(*epochOpt);
            }

            result->SetDateAdded(std::move(dateAdded));
            result->SetNote(std::move(note));

            return result;
        }
    }

    using namespace std::string_view_literals;
    static constexpr std::string_view s_PinTable_Table_Name = "pin"sv;
    static constexpr std::string_view s_PinTable_PackageId_Column = "package_id"sv;
    static constexpr std::string_view s_PinTable_SourceId_Column = "source_id"sv;
    static constexpr std::string_view s_PinTable_Type_Column = "type"sv;
    static constexpr std::string_view s_PinTable_Version_Column = "version"sv;
    static constexpr std::string_view s_PinTable_DateAdded_Column = "date_added"sv;
    static constexpr std::string_view s_PinTable_Note_Column = "note"sv;

    void PinTable::MigrateFrom1_0(SQLite::Connection& connection)
    {
        using namespace SQLite::Builder;

        SQLite::Savepoint savepoint = SQLite::Savepoint::Create(connection, "migratepintable_v1_0_to_v1_1_pintable");

        StatementBuilder addDateAdded;
        addDateAdded.AlterTable(s_PinTable_Table_Name).Add(s_PinTable_DateAdded_Column, Type::Integer);
        addDateAdded.Execute(connection);

        StatementBuilder addNote;
        addNote.AlterTable(s_PinTable_Table_Name).Add(s_PinTable_Note_Column, Type::Text);
        addNote.Execute(connection);

        savepoint.Commit();
    }

    SQLite::rowid_t PinTable::AddPin(SQLite::Connection& connection, const Pinning::Pin& pin)
    {
        SQLite::Builder::StatementBuilder builder;
        const auto& pinKey = pin.GetKey();

        const auto& dateAdded = pin.GetDateAdded();
        std::optional<int64_t> epochOpt = dateAdded.has_value()
            ? std::optional<int64_t>{ Utility::ConvertSystemClockToUnixEpoch(*dateAdded) }
            : std::nullopt;

        builder.InsertInto(s_PinTable_Table_Name)
            .Columns({
                s_PinTable_PackageId_Column,
                s_PinTable_SourceId_Column,
                s_PinTable_Type_Column,
                s_PinTable_Version_Column,
                s_PinTable_DateAdded_Column,
                s_PinTable_Note_Column })
            .Values(
                pinKey.PackageId,
                pinKey.SourceId,
                pin.GetType(),
                pin.GetGatedVersion().ToString(),
                epochOpt,
                pin.GetNote());

        builder.Execute(connection);
        return connection.GetLastInsertRowID();
    }

    bool PinTable::UpdatePinById(SQLite::Connection& connection, SQLite::rowid_t pinId, const Pinning::Pin& pin)
    {
        SQLite::Builder::StatementBuilder builder;
        const auto& pinKey = pin.GetKey();
        builder.Update(s_PinTable_Table_Name).Set()
            .Column(s_PinTable_PackageId_Column).Equals(pinKey.PackageId)
            .Column(s_PinTable_SourceId_Column).Equals(pinKey.SourceId)
            .Column(s_PinTable_Type_Column).Equals(pin.GetType())
            .Column(s_PinTable_Version_Column).Equals(pin.GetGatedVersion().ToString());

        // Use Unbound (= ?) for null date so SQLite stores NULL via = NULL, not the invalid SET syntax IS NULL.
        const auto& dateAdded = pin.GetDateAdded();
        if (dateAdded.has_value())
        {
            builder.Column(s_PinTable_DateAdded_Column).Equals(Utility::ConvertSystemClockToUnixEpoch(*dateAdded));
        }
        else
        {
            builder.Column(s_PinTable_DateAdded_Column).Equals(SQLite::Builder::Unbound);
        }

        // Use Unbound (= ?) for null note so SQLite stores NULL via = NULL, not the invalid SET syntax IS NULL.
        const auto& note = pin.GetNote();
        if (note.has_value())
        {
            builder.Column(s_PinTable_Note_Column).Equals(note.value());
        }
        else
        {
            builder.Column(s_PinTable_Note_Column).Equals(SQLite::Builder::Unbound);
        }

        builder.Where(SQLite::RowIDName).Equals(pinId);
        builder.Execute(connection);
        return connection.GetChanges() != 0;
    }

    std::optional<Pinning::Pin> PinTable::GetPinById(SQLite::Connection& connection, const SQLite::rowid_t pinId)
    {
        SQLite::Builder::StatementBuilder builder;
        builder.Select({
            s_PinTable_PackageId_Column,
            s_PinTable_SourceId_Column,
            s_PinTable_Type_Column,
            s_PinTable_Version_Column,
            s_PinTable_DateAdded_Column,
            s_PinTable_Note_Column })
            .From(s_PinTable_Table_Name).Where(SQLite::RowIDName).Equals(pinId);

        SQLite::Statement select = builder.Prepare(connection);

        if (!select.Step())
        {
            return {};
        }

        return GetPinFromRow(select.GetRow<std::string, std::string, Pinning::PinType, std::string, std::optional<int64_t>, std::optional<std::string>>());
    }

    std::vector<Pinning::Pin> PinTable::GetAllPins(SQLite::Connection& connection)
    {
        SQLite::Builder::StatementBuilder builder;
        builder.Select({
            s_PinTable_PackageId_Column,
            s_PinTable_SourceId_Column,
            s_PinTable_Type_Column,
            s_PinTable_Version_Column,
            s_PinTable_DateAdded_Column,
            s_PinTable_Note_Column })
            .From(s_PinTable_Table_Name);

        SQLite::Statement select = builder.Prepare(connection);

        std::vector<Pinning::Pin> pins;
        while (select.Step())
        {
            auto pin = GetPinFromRow(select.GetRow<std::string, std::string, Pinning::PinType, std::string, std::optional<int64_t>, std::optional<std::string>>());
            if (pin)
            {
                pins.push_back(std::move(pin.value()));
            }
        }

        return pins;
    }

}
