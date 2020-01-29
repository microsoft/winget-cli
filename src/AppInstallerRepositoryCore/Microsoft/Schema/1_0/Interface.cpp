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
    namespace
    {
        // Holds the information on a potentially existing manifest.
        struct ExistingManifestInfo
        {
            SQLite::rowid_t PathLeaf;
            std::optional<SQLite::rowid_t> Manifest;
        };

        // Gets an existing manifest by its rowid.
        // The return value contains the path leaf and manifest rowid, if they exist.
        ExistingManifestInfo GetExistingManifestId(SQLite::Connection& connection, const Manifest::Manifest& manifest, const std::filesystem::path& relativePath)
        {
            auto [pathFound, pathLeafId] = PathPartTable::EnsurePathExists(connection, relativePath, false);

            // If we do not find the path, there can be no manifest
            if (!pathFound)
            {
                AICLI_LOG(Repo, Info, << "Did not find a manifest to remove at path: " << relativePath.u8string());
                return {};
            }

            ExistingManifestInfo result{};
            result.PathLeaf = pathLeafId;
            result.Manifest = ManifestTable::SelectByValueId<PathPartTable>(connection, result.PathLeaf);

            // If the manifest didn't actually exist, then remove the path
            if (!result.Manifest)
            {
                AICLI_LOG(Repo, Info, << "Did not find a manifest row for the path: " << relativePath.u8string());
                PathPartTable::RemovePathById(connection, result.PathLeaf);
                return {};
            }

            // Ensure that the given manifest matches the data in the index
            auto [idValue, versionValue, channelValue] = ManifestTable::GetValuesById<IdTable, VersionTable, ChannelTable>(connection, result.Manifest.value());

            if (idValue != manifest.Id)
            {
                AICLI_LOG(Repo, Error, << "Existing manifest in index does not match given value for Id: [" << idValue << "] != [" << manifest.Id << "]");
                THROW_HR(E_NOT_VALID_STATE);
            }

            if (versionValue != manifest.Version)
            {
                AICLI_LOG(Repo, Error, << "Existing manifest in index does not match given value for Version: [" << versionValue << "] != [" << manifest.Version << "]");
                THROW_HR(E_NOT_VALID_STATE);
            }

            if (channelValue != manifest.Channel)
            {
                AICLI_LOG(Repo, Error, << "Existing manifest in index does not match given value for Channel: [" << channelValue << "] != [" << manifest.Channel << "]");
                THROW_HR(E_NOT_VALID_STATE);
            }

            return result;
        }
    }

    Schema::Version Interface::GetVersion() const
    {
        return { 1, 0 };
    }

    void Interface::CreateTables(SQLite::Connection& connection)
    {
        SQLite::Savepoint savepoint = SQLite::Savepoint::Create(connection, "createtables_v1_0");

        IdTable::Create(connection);
        NameTable::Create(connection);
        MonikerTable::Create(connection);
        VersionTable::Create(connection);
        ChannelTable::Create(connection);

        PathPartTable::Create(connection);

        ManifestTable::Create(connection, {
            { IdTable::ValueName(), true, false }, 
            { NameTable::ValueName(), false, false },
            { MonikerTable::ValueName(), false, false },
            { VersionTable::ValueName(), true, false },
            { ChannelTable::ValueName(), true, false },
            { PathPartTable::ValueName(), false, true }
            });

        TagsTable::Create(connection);
        CommandsTable::Create(connection);
        ProtocolsTable::Create(connection);
        ExtensionsTable::Create(connection);

        savepoint.Commit();
    }

    void Interface::AddManifest(SQLite::Connection& connection, const Manifest::Manifest& manifest, const std::filesystem::path& relativePath)
    {
        SQLite::Savepoint savepoint = SQLite::Savepoint::Create(connection, "addmanifest_v1_0");

        auto [pathFound, pathLeafId] = PathPartTable::EnsurePathExists(connection, relativePath, true);

        // If we get false from the function, this manifest already exists in the index.
        THROW_HR_IF(HRESULT_FROM_WIN32(ERROR_ALREADY_EXISTS), !pathFound);

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
            { PathPartTable::ValueName(), pathLeafId }
            });

        // Add all of the 1:N data.
        TagsTable::EnsureExistsAndInsert(connection, manifest.Tags, manifestId);
        CommandsTable::EnsureExistsAndInsert(connection, manifest.Commands, manifestId);
        ProtocolsTable::EnsureExistsAndInsert(connection, manifest.Protocols, manifestId);
        ExtensionsTable::EnsureExistsAndInsert(connection, manifest.FileExtensions, manifestId);

        savepoint.Commit();
    }

    bool Interface::UpdateManifest(SQLite::Connection& connection, 
        const Manifest::Manifest& oldManifest, const std::filesystem::path& oldRelativePath, 
        const Manifest::Manifest& newManifest, const std::filesystem::path& newRelativePath)
    {
        UNREFERENCED_PARAMETER(connection);
        UNREFERENCED_PARAMETER(oldManifest);
        UNREFERENCED_PARAMETER(oldRelativePath);
        UNREFERENCED_PARAMETER(newManifest);
        UNREFERENCED_PARAMETER(newRelativePath);
        THROW_HR(E_NOTIMPL);
    }

    bool Interface::RemoveManifest(SQLite::Connection& connection, const Manifest::Manifest& manifest, const std::filesystem::path& relativePath)
    {
        SQLite::Savepoint savepoint = SQLite::Savepoint::Create(connection, "removemanifest_v1_0");

        ExistingManifestInfo manifestInfo = GetExistingManifestId(connection, manifest, relativePath);

        // If the manifest doesn't actually exist, we don't need to make any change.
        if (!manifestInfo.Manifest)
        {
            return false;
        }

        // Get the ids of the values from the manifest table
        auto [idId, nameId, monikerId, versionId, channelId] = 
            ManifestTable::GetIdsById<IdTable, NameTable, MonikerTable, VersionTable, ChannelTable>(connection, manifestInfo.Manifest.value());

        // Remove all of the 1:N data.
        TagsTable::RemoveAndEnsureNeeded(connection, manifestId);
        CommandsTable::EnsureExistsAndInsert(connection, manifest.Commands, manifestId);
        ProtocolsTable::EnsureExistsAndInsert(connection, manifest.Protocols, manifestId);
        ExtensionsTable::EnsureExistsAndInsert(connection, manifest.FileExtensions, manifestId);

        // Insert the manifest entry.
        SQLite::rowid_t manifestId = ManifestTable::Insert(connection, {
            { IdTable::ValueName(), idId},
            { NameTable::ValueName(), nameId },
            { MonikerTable::ValueName(), monikerId },
            { VersionTable::ValueName(), versionId },
            { ChannelTable::ValueName(), channelId },
            { PathPartTable::ValueName(), std::get<1>(pathResult) }
            });

        // Ensure that all of the 1:1 data exists.
        SQLite::rowid_t idId = IdTable::EnsureExists(connection, manifest.Id);
        SQLite::rowid_t nameId = NameTable::EnsureExists(connection, manifest.Name);
        SQLite::rowid_t monikerId = MonikerTable::EnsureExists(connection, manifest.AppMoniker);
        SQLite::rowid_t versionId = VersionTable::EnsureExists(connection, manifest.Version);
        SQLite::rowid_t channelId = ChannelTable::EnsureExists(connection, manifest.Channel);

        savepoint.Commit();
    }
}
