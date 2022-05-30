// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "Microsoft/Schema/1_5/Interface.h"
#include "Microsoft/Schema/1_5/ArpVersionVirtualTable.h"
#include "Microsoft/Schema/1_0/ManifestTable.h"
#include "Microsoft/Schema/1_0/VersionTable.h"

namespace AppInstaller::Repository::Microsoft::Schema::V1_5
{
    Interface::Interface(Utility::NormalizationVersion normVersion) : V1_4::Interface(normVersion)
    {
    }

    Schema::Version Interface::GetVersion() const
    {
        return { 1, 5 };
    }

    void Interface::CreateTables(SQLite::Connection& connection, CreateOptions options)
    {
        SQLite::Savepoint savepoint = SQLite::Savepoint::Create(connection, "createtables_v1_5");

        V1_4::Interface::CreateTables(connection, options);

        V1_0::ManifestTable::AddColumn(connection, { ArpMinVersionVirtualTable::ValueName(), SQLite::Builder::Type::Int64 });
        V1_0::ManifestTable::AddColumn(connection, { ArpMaxVersionVirtualTable::ValueName(), SQLite::Builder::Type::Int64 });

        savepoint.Commit();
    }

    SQLite::rowid_t Interface::AddManifest(SQLite::Connection& connection, const Manifest::Manifest& manifest, const std::optional<std::filesystem::path>& relativePath)
    {
        SQLite::Savepoint savepoint = SQLite::Savepoint::Create(connection, "addmanifest_v1_5");

        SQLite::rowid_t manifestId = V1_4::Interface::AddManifest(connection, manifest, relativePath);

        auto arpVersionRange = manifest.GetArpVersionRange();
        Manifest::string_t arpMinVersion = arpVersionRange.IsEmpty() ? "" : arpVersionRange.GetMinVersion().ToString();
        Manifest::string_t arpMaxVersion = arpVersionRange.IsEmpty() ? "" : arpVersionRange.GetMaxVersion().ToString();
        SQLite::rowid_t arpMinVersionId = V1_0::VersionTable::EnsureExists(connection, arpMinVersion);
        SQLite::rowid_t arpMaxVersionId = V1_0::VersionTable::EnsureExists(connection, arpMaxVersion);
        V1_0::ManifestTable::UpdateValueIdById<ArpMinVersionVirtualTable>(connection, manifestId, arpMinVersionId);
        V1_0::ManifestTable::UpdateValueIdById<ArpMaxVersionVirtualTable>(connection, manifestId, arpMaxVersionId);

        savepoint.Commit();

        return manifestId;
    }

    std::pair<bool, SQLite::rowid_t> Interface::UpdateManifest(SQLite::Connection& connection, const Manifest::Manifest& manifest, const std::optional<std::filesystem::path>& relativePath)
    {
        SQLite::Savepoint savepoint = SQLite::Savepoint::Create(connection, "updatemanifest_v1_5");

        auto [indexModified, manifestId] = V1_4::Interface::UpdateManifest(connection, manifest, relativePath);

        auto [arpMinVersionInIndex, arpMaxVersionInIndex] =
            V1_0::ManifestTable::GetValuesById<ArpMinVersionVirtualTable, ArpMaxVersionVirtualTable>(connection, manifestId);

        auto arpVersionRange = manifest.GetArpVersionRange();
        Manifest::string_t arpMinVersion = arpVersionRange.IsEmpty() ? "" : arpVersionRange.GetMinVersion().ToString();
        Manifest::string_t arpMaxVersion = arpVersionRange.IsEmpty() ? "" : arpVersionRange.GetMaxVersion().ToString();

        // For cleaning up the old entries after update if applicable
        V1_0::VersionTable::id_t oldMinVersionId = -1;
        V1_0::VersionTable::id_t oldMaxVersionId = -1;

        if (arpMinVersionInIndex != arpMinVersion)
        {
            oldMinVersionId = std::get<0>(V1_0::ManifestTable::GetIdsById<ArpMinVersionVirtualTable>(connection, manifestId));
            SQLite::rowid_t arpMinVersionId = V1_0::VersionTable::EnsureExists(connection, arpMinVersion);
            V1_0::ManifestTable::UpdateValueIdById<ArpMinVersionVirtualTable>(connection, manifestId, arpMinVersionId);
            indexModified = true;
        }

        if (arpMaxVersionInIndex != arpMaxVersion)
        {
            oldMaxVersionId = std::get<0>(V1_0::ManifestTable::GetIdsById<ArpMaxVersionVirtualTable>(connection, manifestId));
            SQLite::rowid_t arpMaxVersionId = V1_0::VersionTable::EnsureExists(connection, arpMaxVersion);
            V1_0::ManifestTable::UpdateValueIdById<ArpMaxVersionVirtualTable>(connection, manifestId, arpMaxVersionId);
            indexModified = true;
        }

        if (oldMinVersionId >= 0 && NotNeeded(connection, V1_0::VersionTable::TableName(), V1_0::VersionTable::ValueName(), oldMinVersionId))
        {
            V1_0::VersionTable::DeleteById(connection, oldMinVersionId);
        }

        if (oldMaxVersionId >= 0 && oldMaxVersionId != oldMinVersionId && NotNeeded(connection, V1_0::VersionTable::TableName(), V1_0::VersionTable::ValueName(), oldMaxVersionId))
        {
            V1_0::VersionTable::DeleteById(connection, oldMaxVersionId);
        }

        savepoint.Commit();

        return { indexModified, manifestId };
     }

    void Interface::RemoveManifestById(SQLite::Connection& connection, SQLite::rowid_t manifestId)
    {
        // Get the old arp version ids of the values from the manifest table
        auto [arpMinVersionId, arpMaxVersionId] =
            V1_0::ManifestTable::GetIdsById<ArpMinVersionVirtualTable, ArpMaxVersionVirtualTable>(connection, manifestId);

        SQLite::Savepoint savepoint = SQLite::Savepoint::Create(connection, "RemoveManifestById_v1_5");

        // Removes the manifest.
        V1_4::Interface::RemoveManifestById(connection, manifestId);

        // Remove the versions that are not needed.
        if (NotNeeded(connection, V1_0::VersionTable::TableName(), V1_0::VersionTable::ValueName(), arpMinVersionId))
        {
            V1_0::VersionTable::DeleteById(connection, arpMinVersionId);
        }

        if (arpMaxVersionId != arpMinVersionId && NotNeeded(connection, V1_0::VersionTable::TableName(), V1_0::VersionTable::ValueName(), arpMaxVersionId))
        {
            V1_0::VersionTable::DeleteById(connection, arpMaxVersionId);
        }

        savepoint.Commit();
    }

    bool Interface::NotNeeded(const SQLite::Connection& connection, std::string_view tableName, std::string_view valueName, SQLite::rowid_t id) const
    {
        bool result = V1_4::Interface::NotNeeded(connection, tableName, valueName, id);

        if (result && tableName == V1_0::VersionTable::TableName())
        {
            if (valueName != V1_0::VersionTable::ValueName())
            {
                result = !V1_0::ManifestTable::IsValueReferenced(connection, V1_0::VersionTable::ValueName(), id) && result;
            }
            if (valueName != ArpMinVersionVirtualTable::ValueName())
            {
                result = !V1_0::ManifestTable::IsValueReferenced(connection, ArpMinVersionVirtualTable::ValueName(), id) && result;
            }
            if (valueName != ArpMaxVersionVirtualTable::ValueName())
            {
                result = !V1_0::ManifestTable::IsValueReferenced(connection, ArpMaxVersionVirtualTable::ValueName(), id) && result;
            }
        }

        return result;
    }

    bool Interface::CheckConsistency(const SQLite::Connection& connection, bool log) const
    {
        bool result = V1_4::Interface::CheckConsistency(connection, log);

        // If the v1.4 index was consistent, or if full logging of inconsistency was requested, check the v1.5 data.
        if (result || log)
        {
            result = V1_0::ManifestTable::CheckConsistency<ArpMinVersionVirtualTable>(connection, log) && result;
        }

        if (result || log)
        {
            result = V1_0::ManifestTable::CheckConsistency<ArpMaxVersionVirtualTable>(connection, log) && result;
        }

        return result;
    }

    std::optional<std::string> Interface::GetPropertyByManifestIdInternal(const SQLite::Connection& connection, SQLite::rowid_t manifestId, PackageVersionProperty property) const
    {
        switch (property)
        {
        case AppInstaller::Repository::PackageVersionProperty::ArpMinVersion:
            return std::get<0>(V1_0::ManifestTable::GetValuesById<ArpMinVersionVirtualTable>(connection, manifestId));
        case AppInstaller::Repository::PackageVersionProperty::ArpMaxVersion:
            return std::get<0>(V1_0::ManifestTable::GetValuesById<ArpMaxVersionVirtualTable>(connection, manifestId));
        default:
            return V1_4::Interface::GetPropertyByManifestIdInternal(connection, manifestId, property);
        }
    }
}