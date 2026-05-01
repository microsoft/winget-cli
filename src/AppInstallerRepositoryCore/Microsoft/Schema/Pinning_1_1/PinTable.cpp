// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "PinTable.h"
#include <winget/SQLiteStatementBuilder.h>
#include "Microsoft/Schema/IPinningIndex.h"

namespace AppInstaller::Repository::Microsoft::Schema::Pinning_V1_1
{
    namespace
    {
        std::optional<Pinning::Pin> GetPinFromRow(
            std::string_view packageId,
            std::string_view sourceId,
            Pinning::PinType type,
            std::string_view version,
            std::string_view dateAdded,
            std::optional<std::string> note)
        {
            std::optional<Pinning::Pin> result;

            switch (type)
            {
            case Pinning::PinType::Blocking:
                result = Pinning::Pin::CreateBlockingPin({ packageId, sourceId });
                break;
            case Pinning::PinType::Pinning:
                result = Pinning::Pin::CreatePinningPin({ packageId, sourceId });
                break;
            case Pinning::PinType::Gating:
                result = Pinning::Pin::CreateGatingPin({ packageId, sourceId }, Utility::GatedVersion{ version });
                break;
            default:
                return {};
            }

            result->SetDateAdded(std::string{ dateAdded });
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
        SQLite::Statement addDateAdded = SQLite::Statement::Create(connection,
            "ALTER TABLE pin ADD COLUMN date_added TEXT NOT NULL DEFAULT ''");
        addDateAdded.Execute();

        SQLite::Statement addNote = SQLite::Statement::Create(connection,
            "ALTER TABLE pin ADD COLUMN note TEXT");
        addNote.Execute();
    }

    SQLite::rowid_t PinTable::AddPin(SQLite::Connection& connection, const Pinning::Pin& pin)
    {
        SQLite::Builder::StatementBuilder builder;
        const auto& pinKey = pin.GetKey();
        builder.InsertInto(s_PinTable_Table_Name)
            .Columns({
                s_PinTable_PackageId_Column,
                s_PinTable_SourceId_Column,
                s_PinTable_Type_Column,
                s_PinTable_Version_Column,
                s_PinTable_DateAdded_Column,
                s_PinTable_Note_Column })
            .Values(
                (std::string_view)pinKey.PackageId,
                pinKey.SourceId,
                pin.GetType(),
                pin.GetGatedVersion().ToString(),
                (std::string_view)pin.GetDateAdded(),
                pin.GetNote());

        builder.Execute(connection);
        return connection.GetLastInsertRowID();
    }

    bool PinTable::UpdatePinById(SQLite::Connection& connection, SQLite::rowid_t pinId, const Pinning::Pin& pin)
    {
        SQLite::Builder::StatementBuilder builder;
        const auto& pinKey = pin.GetKey();
        builder.Update(s_PinTable_Table_Name).Set()
            .Column(s_PinTable_PackageId_Column).Equals((std::string_view)pinKey.PackageId)
            .Column(s_PinTable_SourceId_Column).Equals(pinKey.SourceId)
            .Column(s_PinTable_Type_Column).Equals(pin.GetType())
            .Column(s_PinTable_Version_Column).Equals(pin.GetGatedVersion().ToString())
            .Column(s_PinTable_DateAdded_Column).Equals((std::string_view)pin.GetDateAdded());

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

        auto [packageId, sourceId, pinType, gatedVersion, dateAdded, note] =
            select.GetRow<std::string, std::string, Pinning::PinType, std::string, std::string, std::optional<std::string>>();
        return GetPinFromRow(packageId, sourceId, pinType, gatedVersion, dateAdded, std::move(note));
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
            auto [packageId, sourceId, pinType, gatedVersion, dateAdded, note] =
                select.GetRow<std::string, std::string, Pinning::PinType, std::string, std::string, std::optional<std::string>>();
            auto pin = GetPinFromRow(packageId, sourceId, pinType, gatedVersion, dateAdded, std::move(note));
            if (pin)
            {
                pins.push_back(std::move(pin.value()));
            }
        }

        return pins;
    }

}
