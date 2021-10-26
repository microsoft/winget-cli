// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "Microsoft/Schema/1_3/Interface.h"
#include <AppInstallerSHA256.h>

#include "Microsoft/Schema/1_0/ManifestTable.h"
#include "Microsoft/Schema/1_3/DependenciesTable.h"
#include "Microsoft/Schema/1_3/HashVirtualTable.h"
#include <winget/ManifestValidation.h>

namespace AppInstaller::Repository::Microsoft::Schema::V1_3
{
    Interface::Interface(Utility::NormalizationVersion normVersion) : V1_2::Interface(normVersion)
    {
    }

    Schema::Version Interface::GetVersion() const
    {
        return { 1, 3 };
    }

    void Interface::CreateTables(SQLite::Connection& connection)
    {
        SQLite::Savepoint savepoint = SQLite::Savepoint::Create(connection, "createtables_v1_3");

        V1_2::Interface::CreateTables(connection);

        V1_0::ManifestTable::AddColumn(connection, { HashVirtualTable::ValueName(), HashVirtualTable::SQLiteType() });

        DependenciesTable::Create(connection);

        savepoint.Commit();
    }

    SQLite::rowid_t Interface::AddManifest(SQLite::Connection& connection, const Manifest::Manifest& manifest, const std::filesystem::path& relativePath)
    {
        SQLite::Savepoint savepoint = SQLite::Savepoint::Create(connection, "addmanifest_v1_3");

        SQLite::rowid_t manifestId = V1_2::Interface::AddManifest(connection, manifest, relativePath);

        // Set the hash value if provided
        if (!manifest.StreamSha256.empty())
        {
            THROW_HR_IF(E_INVALIDARG, manifest.StreamSha256.size() != Utility::SHA256::HashBufferSizeInBytes);
            V1_0::ManifestTable::UpdateValueIdById<HashVirtualTable>(connection, manifestId, manifest.StreamSha256);
        }

        DependenciesTable::AddDependencies(connection, manifest, manifestId);

        savepoint.Commit();

        return manifestId;
    }

    std::pair<bool, SQLite::rowid_t> Interface::UpdateManifest(SQLite::Connection& connection, const Manifest::Manifest& manifest, const std::filesystem::path& relativePath)
    {
        SQLite::Savepoint savepoint = SQLite::Savepoint::Create(connection, "updatemanifest_v1_3");

        auto [indexModified, manifestId] = V1_2::Interface::UpdateManifest(connection, manifest, relativePath);

        // Set the hash value if provided
        if (!manifest.StreamSha256.empty())
        {
            THROW_HR_IF(E_INVALIDARG, manifest.StreamSha256.size() != Utility::SHA256::HashBufferSizeInBytes);

            auto currentHash = std::get<0>(V1_0::ManifestTable::GetIdsById<HashVirtualTable>(connection, manifestId));

            if (currentHash.size() != Utility::SHA256::HashBufferSizeInBytes ||
                !std::equal(currentHash.begin(), currentHash.end(), manifest.StreamSha256.begin()))
            {
                V1_0::ManifestTable::UpdateValueIdById<HashVirtualTable>(connection, manifestId, manifest.StreamSha256);
                indexModified = true;
            }
        }

        bool dependenciesModified = DependenciesTable::UpdateDependencies(connection, manifest, manifestId);
        indexModified = indexModified || dependenciesModified;

        savepoint.Commit();

        return { indexModified, manifestId };
    }

    SQLite::rowid_t Interface::RemoveManifest(SQLite::Connection& connection, const Manifest::Manifest& manifest, const std::filesystem::path& relativePath)
    {
        SQLite::Savepoint savepoint = SQLite::Savepoint::Create(connection, "removemanifest_v1_3");

        SQLite::rowid_t manifestId = V1_2::Interface::RemoveManifest(connection, manifest, relativePath);

        DependenciesTable::RemoveDependencies(connection, manifestId);

        savepoint.Commit();

        return manifestId;
    }

    bool Interface::ValidateManifest(SQLite::Connection& connection, const Manifest::Manifest& manifest) const
    {
        return DependenciesTable::ValidateDependencies(connection, manifest);
    }

    bool Interface::VerifyDependenciesStructureForManifestDelete(SQLite::Connection& connection, const Manifest::Manifest& manifest) const
    {
        return DependenciesTable::VerifyDependenciesStructureForManifestDelete(connection, manifest);
    }

    std::optional<std::string> Interface::GetPropertyByManifestIdInternal(const SQLite::Connection& connection, SQLite::rowid_t manifestId, PackageVersionProperty property) const
    {
        switch (property)
        {
        case AppInstaller::Repository::PackageVersionProperty::ManifestSHA256Hash:
        {
            SQLite::blob_t hash = std::get<0>(V1_0::ManifestTable::GetIdsById<HashVirtualTable>(connection, manifestId));
            return hash.empty() ? std::optional<std::string>{} : Utility::SHA256::ConvertToString(hash);
        }
        default:
            return V1_2::Interface::GetPropertyByManifestIdInternal(connection, manifestId, property);
        }
    }

    void Interface::PrepareForPackaging(SQLite::Connection& connection, bool vacuum)
    {
        SQLite::Savepoint savepoint = SQLite::Savepoint::Create(connection, "prepareforpackaging_v1_3");

        V1_2::Interface::PrepareForPackaging(connection, false);

        DependenciesTable::PrepareForPackaging(connection);

        savepoint.Commit();

        if (vacuum)
        {
            // Force the database to actually shrink the file size.
            // This *must* be done outside of an active transaction.
            SQLite::Builder::StatementBuilder builder;
            builder.Vacuum();
            builder.Execute(connection);
        }
    }
}
