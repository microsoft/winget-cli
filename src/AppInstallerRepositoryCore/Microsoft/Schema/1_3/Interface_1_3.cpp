// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "Microsoft/Schema/1_3/Interface.h"
#include <AppInstallerSHA256.h>

#include "Microsoft/Schema/1_0/ManifestTable.h"
#include "Microsoft/Schema/1_3/HashVirtualTable.h"
#include <winget/ManifestValidation.h>

namespace AppInstaller::Repository::Microsoft::Schema::V1_3
{
    Interface::Interface(Utility::NormalizationVersion normVersion) : V1_2::Interface(normVersion)
    {
    }

    SQLite::Version Interface::GetVersion() const
    {
        return { 1, 3 };
    }

    void Interface::CreateTables(SQLite::Connection& connection, CreateOptions options)
    {
        SQLite::Savepoint savepoint = SQLite::Savepoint::Create(connection, "createtables_v1_3");

        V1_2::Interface::CreateTables(connection, options);

        V1_0::ManifestTable::AddColumn(connection, { HashVirtualTable::ValueName(), HashVirtualTable::SQLiteType() });

        savepoint.Commit();
    }

    SQLite::rowid_t Interface::AddManifest(SQLite::Connection& connection, const Manifest::Manifest& manifest, const std::optional<std::filesystem::path>& relativePath)
    {
        SQLite::Savepoint savepoint = SQLite::Savepoint::Create(connection, "addmanifest_v1_3");

        SQLite::rowid_t manifestId = V1_2::Interface::AddManifest(connection, manifest, relativePath);

        // Set the hash value if provided
        if (!manifest.StreamSha256.empty())
        {
            THROW_HR_IF(E_INVALIDARG, manifest.StreamSha256.size() != Utility::SHA256::HashBufferSizeInBytes);
            V1_0::ManifestTable::UpdateValueIdById<HashVirtualTable>(connection, manifestId, manifest.StreamSha256);
        }

        savepoint.Commit();

        return manifestId;
    }

    std::pair<bool, SQLite::rowid_t> Interface::UpdateManifest(SQLite::Connection& connection, const Manifest::Manifest& manifest, const std::optional<std::filesystem::path>& relativePath)
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

        savepoint.Commit();

        return { indexModified, manifestId };
    }

    std::optional<std::string> Interface::GetPropertyByManifestIdInternal(const SQLite::Connection& connection, SQLite::rowid_t manifestId, PackageVersionProperty property) const
    {
        switch (property)
        {
        case AppInstaller::Repository::PackageVersionProperty::ManifestSHA256Hash:
        {
            std::optional<SQLite::blob_t> hash = V1_0::ManifestTable::GetIdById<HashVirtualTable>(connection, manifestId);
            return (!hash || hash->empty()) ? std::optional<std::string>{} : Utility::SHA256::ConvertToString(hash.value());
        }
        default:
            return V1_2::Interface::GetPropertyByManifestIdInternal(connection, manifestId, property);
        }
    }
}
