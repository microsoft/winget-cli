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
            { IdTable::ValueName(), NameTable::ValueName(), MonikerTable::ValueName(), VersionTable::ValueName(), ChannelTable::ValueName(), PathPartTable::ValueName() });

        TagsTable::Create(connection);
        CommandsTable::Create(connection);
        ProtocolsTable::Create(connection);
        ExtensionsTable::Create(connection);

        savepoint.Commit();
    }

    void Interface::AddManifest(const Manifest::Manifest& manifest, const std::filesystem::path& relativePath)
    {
        SQLite::Savepoint savepoint = SQLite::Savepoint::Create(m_connection, "addmanifest_v1_0");

        auto pathResult = PathPartTable::EnsurePathExists(m_connection, relativePath, true);

        // If we get false from the function, this manifest already exists in the index.
        THROW_HR_IF(HRESULT_FROM_WIN32(ERROR_ALREADY_EXISTS), !std::get<0>(pathResult));

        // TODO:
        //  Convert ShortId to proper moniker name
        //  impl 1:1 ensure exists
        //  impl mani insert
        //  convert 1:Ns to vectors
        //  impl 1:N insert

        // Ensure that all of the 1:1 data exists.
        SQLite::rowid_t idId = IdTable::EnsureExists(m_connection, manifest.Id);
        SQLite::rowid_t nameId = NameTable::EnsureExists(m_connection, manifest.Name);
        SQLite::rowid_t monikerId = MonikerTable::EnsureExists(m_connection, manifest.);
        SQLite::rowid_t versionId = VersionTable::EnsureExists(m_connection, manifest.Version);
        SQLite::rowid_t channelId = ChannelTable::EnsureExists(m_connection, manifest.Channel);

        // Insert the manifest entry.
        SQLite::rowid_t manifestId = ManifestTable::Insert(m_connection, idId, nameId, monikerId, versionId, channelId, std::get<1>(pathResult));

        // Add all of the 1:N data.
        TagsTable::EnsureExistsAndInsert(m_connection, manifest.Tags, manifestId);
        CommandsTable::EnsureExistsAndInsert(m_connection, manifest.Commands, manifestId);
        ProtocolsTable::EnsureExistsAndInsert(m_connection, manifest.Protocols, manifestId);
        ExtensionsTable::EnsureExistsAndInsert(m_connection, manifest.FileExtensions, manifestId);

        savepoint.Commit();
    }

    void Interface::UpdateManifest(const Manifest::Manifest& oldManifest, const std::filesystem::path& oldRelativePath, const Manifest::Manifest& newManifest, const std::filesystem::path& newRelativePath)
    {

    }

    void Interface::RemoveManifest(const Manifest::Manifest& manifest, const std::filesystem::path& relativePath)
    {

    }
}
