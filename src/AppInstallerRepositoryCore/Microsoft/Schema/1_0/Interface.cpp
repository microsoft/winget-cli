// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "Microsoft/Schema/1_0/Interface.h"

#include "Microsoft/Schema/1_0/IdTable.h"
#include "Microsoft/Schema/1_0/NameTable.h"
#include "Microsoft/Schema/1_0/MonikerTable.h"
#include "Microsoft/Schema/1_0/VersionTable.h"
#include "Microsoft/Schema/1_0/ChannelTable.h"

#include "Microsoft/Schema/1_0/PathPartTable.h"

#include "Microsoft/Schema/1_0/ManifestTable.h"

#include "Microsoft/Schema/1_0/TagsTable.h"
#include "Microsoft/Schema/1_0/CommandsTable.h"
#include "Microsoft/Schema/1_0/ProtocolsTable.h"
#include "Microsoft/Schema/1_0/ExtensionsTable.h"

namespace AppInstaller::Repository::Microsoft::Schema::V1_0
{
    Schema::Version Interface::GetVersion() const
    {
        return { 1, 0 };
    }

    void Interface::CreateTables(SQLite::Connection& connection)
    {
        SQLite::Savepoint savepoint = SQLite::Savepoint::Create(connection, "V1_0_createtables");

        IdTable::Create(connection);
        NameTable::Create(connection);
        MonikerTable::Create(connection);
        VersionTable::Create(connection);
        ChannelTable::Create(connection);

        PathPartTable::Create(connection);

        ManifestTable::Create(connection, 
            { IdTable::ValueName(), NameTable::ValueName(), MonikerTable::ValueName(), VersionTable::ValueName(), ChannelTable::ValueName(), PathPartTable::ValueName() },
            { IdTable::ValueName(), VersionTable::ValueName(), ChannelTable::ValueName() });

        TagsTable::Create(connection);
        CommandsTable::Create(connection);
        ProtocolsTable::Create(connection);
        ExtensionsTable::Create(connection);

        savepoint.Commit();
    }

    void Interface::AddManifest(SQLite::Connection& connection, const Manifest::Manifest& manifest, const std::filesystem::path& relativePath)
    {
        SQLite::Savepoint savepoint = SQLite::Savepoint::Create(connection, "addmanifest_v1_0");

        auto pathResult = PathPartTable::EnsurePathExists(connection, relativePath, true);

        // If we get false from the function, this manifest already exists in the index.
        THROW_HR_IF(HRESULT_FROM_WIN32(ERROR_ALREADY_EXISTS), !std::get<0>(pathResult));

        // Ensure that all of the 1:1 data exists.
        SQLite::rowid_t idId = IdTable::EnsureExists(connection, manifest.Id);
        SQLite::rowid_t nameId = NameTable::EnsureExists(connection, manifest.Name);
        SQLite::rowid_t monikerId = MonikerTable::EnsureExists(connection, manifest.AppMoniker);
        SQLite::rowid_t versionId = VersionTable::EnsureExists(connection, manifest.Version);
        SQLite::rowid_t channelId = ChannelTable::EnsureExists(connection, manifest.Channel);

        // Insert the manifest entry.
        SQLite::rowid_t manifestId = ManifestTable::Insert(connection, {
            { IdTable::ValueName(), idId},
            { NameTable::ValueName(), nameId },
            { MonikerTable::ValueName(), monikerId },
            { VersionTable::ValueName(), versionId },
            { ChannelTable::ValueName(), channelId },
            { PathPartTable::ValueName(), std::get<1>(pathResult) }
            });

        // Add all of the 1:N data.
        TagsTable::EnsureExistsAndInsert(connection, manifest.Tags, manifestId);
        CommandsTable::EnsureExistsAndInsert(connection, manifest.Commands, manifestId);
        ProtocolsTable::EnsureExistsAndInsert(connection, manifest.Protocols, manifestId);
        ExtensionsTable::EnsureExistsAndInsert(connection, manifest.FileExtensions, manifestId);

        savepoint.Commit();
    }

    void Interface::UpdateManifest(SQLite::Connection& connection, 
        const Manifest::Manifest& oldManifest, const std::filesystem::path& oldRelativePath, 
        const Manifest::Manifest& newManifest, const std::filesystem::path& newRelativePath)
    {

    }

    void Interface::RemoveManifest(SQLite::Connection& connection, const Manifest::Manifest& manifest, const std::filesystem::path& relativePath)
    {

    }
}
