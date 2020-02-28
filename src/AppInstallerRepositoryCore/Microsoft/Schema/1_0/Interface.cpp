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

namespace AppInstaller::Repository::Microsoft::Schema::V1_0
{
    namespace
    {
        // Gets an existing manifest by its rowid.
        // The return value contains the path leaf and manifest rowid, if they exist.
        std::optional<SQLite::rowid_t> GetExistingManifestId(SQLite::Connection& connection, const Manifest::Manifest& manifest)
        {
            std::optional<SQLite::rowid_t> idId = IdTable::SelectIdByValue(connection, manifest.Id);
            if (!idId)
            {
                AICLI_LOG(Repo, Info, << "Did not find an Id { " << manifest.Id << " }");
                return {};
            }

            std::optional<SQLite::rowid_t> versionId = VersionTable::SelectIdByValue(connection, manifest.Version);
            if (!versionId)
            {
                AICLI_LOG(Repo, Info, << "Did not find a Version { " << manifest.Version << " }");
                return {};
            }

            std::optional<SQLite::rowid_t> channelId = ChannelTable::SelectIdByValue(connection, manifest.Channel);
            if (!channelId)
            {
                AICLI_LOG(Repo, Info, << "Did not find a Channel { " << manifest.Channel << " }");
                return {};
            }

            auto result = ManifestTable::SelectByValueIds<IdTable, VersionTable, ChannelTable>(connection, { idId.value(), versionId.value(), channelId.value() });

            if (!result)
            {
                AICLI_LOG(Repo, Info, << "Did not find a manifest row for { " << manifest.Id << ", " << manifest.Version << ", " << manifest.Channel << " }");
            }

            return result;
        }

        // Updates the manifest column and related table based on the given value.
        template <typename Table>
        void UpdateManifestValueById(SQLite::Connection& connection, const typename Table::value_t& value, SQLite::rowid_t manifestId)
        {
            auto [oldValueId] = ManifestTable::GetIdsById<Table>(connection, manifestId);

            SQLite::rowid_t newValueId = Table::EnsureExists(connection, value);

            ManifestTable::UpdateValueIdById<Table>(connection, manifestId, newValueId);

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

        savepoint.Commit();
    }

    void Interface::AddManifest(SQLite::Connection& connection, const Manifest::Manifest& manifest, const std::filesystem::path& relativePath)
    {
        SQLite::Savepoint savepoint = SQLite::Savepoint::Create(connection, "addmanifest_v1_0");

        auto [pathAdded, pathLeafId] = PathPartTable::EnsurePathExists(connection, relativePath, true);

        // If we get false from the function, this manifest already exists in the index.
        THROW_HR_IF(HRESULT_FROM_WIN32(ERROR_ALREADY_EXISTS), !pathAdded);

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

        savepoint.Commit();
    }

    bool Interface::UpdateManifest(SQLite::Connection& connection, const Manifest::Manifest& manifest, const std::filesystem::path& relativePath)
    {
        auto manifestResult = GetExistingManifestId(connection, manifest);

        // If the manifest doesn't actually exist, fail the update.
        THROW_HR_IF(E_NOT_SET, !manifestResult);

        SQLite::rowid_t manifestId = manifestResult.value();

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

        // Update path table if necessary
        auto [existingPathLeafId] = ManifestTable::GetIdsById<PathPartTable>(connection, manifestId);
        auto [pathAdded, newPathLeafId] = PathPartTable::EnsurePathExists(connection, relativePath, true);

        if (pathAdded)
        {
            // Path was added, so we need to update the manifest table and delete the old path
            ManifestTable::UpdateValueIdById<PathPartTable>(connection, manifestId, newPathLeafId);
            PathPartTable::RemovePathById(connection, existingPathLeafId);
            indexModified = true;
        }
        else
        {
            // The path already existed, so it must either match the existing manifest path or it is an error
            THROW_HR_IF(HRESULT_FROM_WIN32(ERROR_ALREADY_EXISTS), existingPathLeafId != newPathLeafId);
        }

        // Update all 1:N tables as necessary
        indexModified = TagsTable::UpdateIfNeededByManifestId(connection, manifest.Tags, manifestId) || indexModified;
        indexModified = CommandsTable::UpdateIfNeededByManifestId(connection, manifest.Commands, manifestId) || indexModified;

        savepoint.Commit();

        return indexModified;
    }

    void Interface::RemoveManifest(SQLite::Connection& connection, const Manifest::Manifest& manifest, const std::filesystem::path&)
    {
        auto manifestResult = GetExistingManifestId(connection, manifest);

        // If the manifest doesn't actually exist, fail the remove.
        THROW_HR_IF(E_NOT_SET, !manifestResult);

        SQLite::rowid_t manifestId = manifestResult.value();

        // Get the ids of the values from the manifest table
        auto [idId, nameId, monikerId, versionId, channelId, pathLeafId] = 
            ManifestTable::GetIdsById<IdTable, NameTable, MonikerTable, VersionTable, ChannelTable, PathPartTable>(connection, manifestId);

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
        PathPartTable::RemovePathById(connection, pathLeafId);

        // Remove all of the 1:N data that is no longer referenced.
        TagsTable::DeleteIfNotNeededByManifestId(connection, manifestId);
        CommandsTable::DeleteIfNotNeededByManifestId(connection, manifestId);

        savepoint.Commit();
    }

    void Interface::PrepareForPackaging(SQLite::Connection& connection)
    {
        SQLite::Savepoint savepoint = SQLite::Savepoint::Create(connection, "prepareforpackaging_v1_0");

        IdTable::PrepareForPackaging(connection);
        NameTable::PrepareForPackaging(connection);
        MonikerTable::PrepareForPackaging(connection);
        VersionTable::PrepareForPackaging(connection);
        ChannelTable::PrepareForPackaging(connection);

        PathPartTable::PrepareForPackaging(connection);

        ManifestTable::PrepareForPackaging(connection, {
            VersionTable::ValueName(),
            ChannelTable::ValueName(),
            PathPartTable::ValueName(),
            });

        TagsTable::PrepareForPackaging(connection);
        CommandsTable::PrepareForPackaging(connection);

        savepoint.Commit();

        // Force the database to actually shrink the file size.
        // This *must* be done outside of an active transaction.
        SQLite::Builder::StatementBuilder builder;
        builder.Vacuum();
        builder.Execute(connection);
    }
}
