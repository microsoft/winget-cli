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

        // Updates the manifest column and related table based on the given value.
        template <typename Table>
        void UpdateManifestValueById(SQLite::Connection& connection, const typename Table::value_t& value, SQLite::rowid_t manifestId)
        {
            auto [oldValueId] = ManifestTable::GetIdsById<Table>(connection, manifestId);

            SQLite::rowid_t newValueId = Table::EnsureExists(connection, value);

            ManifestTable::UpdateIdById<Table>(connection, manifestId, newValueId);

            Table::DeleteIfNotNeededById(connection, oldValueId);
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

    bool Interface::UpdateManifest(SQLite::Connection& connection, const Manifest::Manifest& manifest, const std::filesystem::path& relativePath)
    {
        ExistingManifestInfo manifestInfo = GetExistingManifestId(connection, manifest, relativePath);

        // If the manifest doesn't actually exist, fail the update.
        THROW_HR_IF(E_NOT_SET, !manifestInfo.Manifest);

        SQLite::rowid_t manifestId = manifestInfo.Manifest.value();

        auto [idInIndex, nameInIndex, monikerInIndex, versionInIndex, channelInIndex] =
            ManifestTable::GetValuesById<IdTable, NameTable, MonikerTable, VersionTable, ChannelTable>(connection, manifestId);

        // We know that the Id, Version, and Channel did not change based on GetExistingManifestId,
        // but we still verify that here in the event that the code there changed.
        THROW_HR_IF(E_UNEXPECTED, idInIndex != manifest.Id);
        THROW_HR_IF(E_UNEXPECTED, versionInIndex != manifest.Version);
        THROW_HR_IF(E_UNEXPECTED, channelInIndex != manifest.Channel);

        bool indexModified = false;
        SQLite::Savepoint savepoint = SQLite::Savepoint::Create(connection, "updatemanifest_v1_0");

        // If these values changed, we need to update them.
        if (nameInIndex != manifest.Name)
        {
            UpdateManifestValueById<NameTable>(connection, manifest.Name, manifestId);
            indexModified = true;
        }

        if (monikerInIndex != manifest.AppMoniker)
        {
            UpdateManifestValueById<MonikerTable>(connection, manifest.AppMoniker, manifestId);
            indexModified = true;
        }

        // Update all 1:N tables as necessary
        indexModified = TagsTable::UpdateIfNeededByManifestId(connection, manifest.Tags, manifestId) || indexModified;
        indexModified = CommandsTable::UpdateIfNeededByManifestId(connection, manifest.Commands, manifestId) || indexModified;
        indexModified = ProtocolsTable::UpdateIfNeededByManifestId(connection, manifest.Protocols, manifestId) || indexModified;
        indexModified = ExtensionsTable::UpdateIfNeededByManifestId(connection, manifest.FileExtensions, manifestId) || indexModified;

        savepoint.Commit();

        return indexModified;
    }

    void Interface::RemoveManifest(SQLite::Connection& connection, const Manifest::Manifest& manifest, const std::filesystem::path& relativePath)
    {
        ExistingManifestInfo manifestInfo = GetExistingManifestId(connection, manifest, relativePath);

        // If the manifest doesn't actually exist, fail the remove.
        THROW_HR_IF(E_NOT_SET, !manifestInfo.Manifest);

        SQLite::rowid_t manifestId = manifestInfo.Manifest.value();

        // Get the ids of the values from the manifest table
        auto [idId, nameId, monikerId, versionId, channelId] = 
            ManifestTable::GetIdsById<IdTable, NameTable, MonikerTable, VersionTable, ChannelTable>(connection, manifestId);

        SQLite::Savepoint savepoint = SQLite::Savepoint::Create(connection, "removemanifest_v1_0");

        // Remove the manifest row
        ManifestTable::DeleteById(connection, manifestId);

        // Remove all of the 1:1 data that is no longer referenced.
        IdTable::DeleteIfNotNeededById(connection, idId);
        NameTable::DeleteIfNotNeededById(connection, nameId);
        MonikerTable::DeleteIfNotNeededById(connection, monikerId);
        VersionTable::DeleteIfNotNeededById(connection, versionId);
        ChannelTable::DeleteIfNotNeededById(connection, channelId);

        // Remove the path
        PathPartTable::RemovePathById(connection, manifestInfo.PathLeaf);

        // Remove all of the 1:N data that is no longer referenced.
        TagsTable::DeleteIfNotNeededByManifestId(connection, manifestId);
        CommandsTable::DeleteIfNotNeededByManifestId(connection, manifestId);
        ProtocolsTable::DeleteIfNotNeededByManifestId(connection, manifestId);
        ExtensionsTable::DeleteIfNotNeededByManifestId(connection, manifestId);

        savepoint.Commit();
    }
}
